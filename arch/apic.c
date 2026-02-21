#include "apic.h"
#include "pic.h"
#include "cpu.h"
#include "../drivers/acpi.h"
#include "../lib/printk.h"
#include "../lib/panic.h"
#include "../mm/vmm.h" 

extern uint64_t hhdm_offset;

static void* lapic_base = NULL;
static void* ioapic_base = NULL;
static uint32_t ioapic_id = 0;

// Interrupt Source Overrides
static uint32_t irq_overrides[16];
static uint16_t irq_flags[16]; 

static inline void* p2v(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *((volatile uint32_t*)((uint64_t)lapic_base + reg));
}

static inline void lapic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)((uint64_t)lapic_base + reg)) = value;
}

static uint32_t ioapic_read(uint32_t reg) {
    volatile uint32_t* idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* dat = (volatile uint32_t*)((uint64_t)ioapic_base + 0x10);
    *idx = reg;
    return *dat;
}

static void ioapic_write(uint32_t reg, uint32_t value) {
    volatile uint32_t* idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* dat = (volatile uint32_t*)((uint64_t)ioapic_base + 0x10);
    *idx = reg;
    *dat = value;
}

uint32_t apic_get_id(void) {
    if (lapic_base == NULL) {
        panic("APIC: apic_get_id() called before lapic_base was initialized!");
    }
    return lapic_read(LAPIC_ID) >> 24;
}

void ioapic_set_entry(uint8_t legacy_irq, uint8_t vector) {
    uint32_t gsi = irq_overrides[legacy_irq];
    uint16_t flags = irq_flags[legacy_irq];

    // Build Low Part
    uint32_t low_part = vector;
    
    // Active Low? (Flags 0x3 = 0011b)
    if ((flags & 0x3) == 0x3) { 
        low_part |= (1 << 13);
    }
    
    // Level Triggered? (Flags 0xC = 1100b)
    if ((flags & 0xC) == 0xC) { 
        low_part |= (1 << 15);
    }

    // Build High Part (Destination APIC ID)
    // For now, route to the current CPU (BSP)
    uint32_t dest_id = apic_get_id();
    uint32_t high_part = dest_id << 24;

    // CRITICAL: Mask the entry before modifying it to prevent race conditions
    // or half-written states firing interrupts.
    uint32_t reg = IOREDTBL + (gsi * 2);
    ioapic_write(reg, 0x10000); // Write Mask bit (Bit 16)

    // Write High Part first
    ioapic_write(reg + 1, high_part);

    // Write Low Part (Unmasking it)
    ioapic_write(reg, low_part);
    
    printk("[APIC] Mapped IRQ %d -> GSI %d -> Vector %d -> CPU %d\n", 
           legacy_irq, gsi, vector, dest_id);
}

void apic_init(void) {
    // 1. Get MADT
    acpi_madt_t* madt = (acpi_madt_t*)acpi_find_table("APIC");
    if (!madt) panic("APIC: MADT table not found!");

    // 2. Disable Legacy PIC
    pic_disable();

    // 3. Enable Local APIC via MSR
    // This is the authoritative source for the LAPIC address
    uint64_t msr_base = rdmsr(IA32_APIC_BASE_MSR);
    if (!(msr_base & IA32_APIC_BASE_MSR_ENABLE)) {
        msr_base |= IA32_APIC_BASE_MSR_ENABLE;
        wrmsr(IA32_APIC_BASE_MSR, msr_base);
    }
    uint64_t lapic_phys = msr_base & ~(uint64_t)0xFFF;
    lapic_base = p2v(lapic_phys);

    uint32_t ver = lapic_read(LAPIC_VER);
    if ((ver & 0xFF) < 0x10) {
        panic("APIC: External APIC not supported!");
    }

    // 4. Initialize ISO defaults
    for (int i = 0; i < 16; i++) {
        irq_overrides[i] = i;
        irq_flags[i] = 0;
    }

    // 5. Parse MADT
    uint8_t* start = (uint8_t*)(madt + 1);
    uint8_t* end = (uint8_t*)madt + madt->header.length;

    while (start < end) {
        acpi_madt_entry_t* entry = (acpi_madt_entry_t*)start;
        switch (entry->type) {
            case MADT_IOAPIC: {
                acpi_madt_ioapic_t* io = (acpi_madt_ioapic_t*)entry;
                if (ioapic_base == NULL) {
                    ioapic_base = p2v(io->ioapic_address);
                    ioapic_id = io->ioapic_id;
                    printk("[APIC] IOAPIC found at %p (ID: %d) for CPU ID: %d\n", 
                           ioapic_base, ioapic_id, apic_get_id());

                    uint32_t ioapic_ver = ioapic_read(IOAPICVER);
                    uint8_t max_redir = (ioapic_ver >> 16) & 0xFF;
                    printk("[APIC] IOAPIC version: %d, Max redirection entries: %d\n", 
                           ioapic_ver & 0xFF, max_redir + 1);

                }
                break;
            }
            case MADT_ISO: {
                acpi_madt_iso_t* iso = (acpi_madt_iso_t*)entry;
                if (iso->source < 16) {
                    irq_overrides[iso->source] = iso->gsi;
                    irq_flags[iso->source] = iso->flags;
                }
                break;
            }
        }
        start += entry->length;
    }

    if (!ioapic_base) panic("APIC: No IOAPIC found!");

    // 6. Mask LVT Entries (Safety First!)
    // Ensure no garbage interrupts fire before we are ready
    lapic_write(LAPIC_LVT_TIMER, 0x10000);
    lapic_write(LAPIC_LVT_LINT0, 0x10000);
    lapic_write(LAPIC_LVT_LINT1, 0x10000);
    lapic_write(LAPIC_LVT_ERROR, 0x10000);
    
    // 7. Enable LAPIC via SVR
    // Map Spurious Vector to 0xFF (255) and set Bit 8 (Software Enable)
    lapic_write(LAPIC_SVR, 0x1FF); 

    printk("[APIC] Local APIC fully enabled at %p\n", lapic_base);

    // 8. Configure IOAPIC
    ioapic_set_entry(0, 32); // Timer
    ioapic_set_entry(1, 33); // Keyboard
}

void apic_send_eoi(void) {
    if (lapic_base) {
        lapic_write(LAPIC_EOI, 0);
    }
}