#include "apic.h"
#include "pic.h"
#include "cpu.h"
#include "../drivers/acpi.h"
#include "../lib/printk.h"
#include "../lib/panic.h"
#include "../mm/vmm.h" 

// IRQ to Vector mappings
#define IRQ_TIMER    0
#define IRQ_KEYBOARD 1
#define IRQ_MOUSE    12

#define VEC_TIMER    32
#define VEC_KEYBOARD 33
#define VEC_MOUSE    44

#define IA32_APIC_BASE_MSR_EXTD (1 << 10)

extern uint64_t hhdm_offset;

static void* lapic_base = NULL;
static void* ioapic_base = NULL;
static uint32_t ioapic_id = 0;
static uint32_t ioapic_gsi_count = 0; 
static bool apic_ready = false; 

// IOAPIC Spinlock
static uint32_t ioapic_lock = 0;

// Interrupt Source Overrides
static uint32_t irq_overrides[16];
static uint16_t irq_flags[16]; 

static inline void* p2v(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

// Atomic helpers
static inline uint64_t save_irq_disable(void) {
    uint64_t flags;
    asm volatile("pushfq; pop %0; cli" : "=r"(flags) : : "memory");
    return flags;
}

static inline void restore_irq(uint64_t flags) {
    if (flags & (1 << 9)) {
        asm volatile("sti" : : : "memory");
    }
}

static inline void spin_lock(uint32_t* lock) {
    uint32_t timeout = 100000000; 
    while (__atomic_test_and_set(lock, __ATOMIC_ACQUIRE)) {
        asm volatile("pause");
        if (--timeout == 0) {
            panic("APIC: IOAPIC Spinlock deadlock detected!");
        }
    }
}

static inline void spin_unlock(uint32_t* lock) {
    __atomic_clear(lock, __ATOMIC_RELEASE);
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *((volatile uint32_t*)((uint64_t)lapic_base + reg));
}

static inline void lapic_write(uint32_t reg, uint32_t value) {
    *((volatile uint32_t*)((uint64_t)lapic_base + reg)) = value;
}

static inline void ioapic_write_unlocked(uint32_t reg, uint32_t value) {
    volatile uint32_t* idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* dat = (volatile uint32_t*)((uint64_t)ioapic_base + 0x10);
    *idx = reg;
    *dat = value;
}

static uint32_t ioapic_read(uint32_t reg) {
    uint64_t rflags = save_irq_disable();
    spin_lock(&ioapic_lock);
    
    volatile uint32_t* idx = (volatile uint32_t*)ioapic_base;
    volatile uint32_t* dat = (volatile uint32_t*)((uint64_t)ioapic_base + 0x10);
    *idx = reg;
    uint32_t val = *dat;
    
    spin_unlock(&ioapic_lock);
    restore_irq(rflags);
    return val;
}

uint32_t apic_get_id(void) {
    if (lapic_base == NULL) {
        panic("APIC: apic_get_id() called before lapic_base was initialized!");
    }
    return lapic_read(LAPIC_ID) >> 24;
}

bool ioapic_set_gsi(uint32_t gsi, uint8_t vector, uint16_t flags, bool masked) {
    if (gsi >= ioapic_gsi_count) {
        printk("[APIC] ERROR: Attempted to map GSI %d (Max valid is %d)\n", gsi, ioapic_gsi_count - 1);
        return false;
    }

    uint32_t low_part = vector;
    uint16_t polarity = flags & 0x3;
    uint16_t trigger = (flags >> 2) & 0x3;

    if (polarity == 0x3) low_part |= (1 << 13); // Active Low
    if (trigger == 0x3)  low_part |= (1 << 15); // Level Triggered
    if (masked)          low_part |= (1 << 16); // Masked

    uint32_t dest_id = apic_get_id();
    uint32_t high_part = dest_id << 24;
    uint32_t reg = IOREDTBL + (gsi * 2);
    
    uint64_t rflags = save_irq_disable();
    spin_lock(&ioapic_lock);

    ioapic_write_unlocked(reg, 0x10000); // Mask before modifying
    ioapic_write_unlocked(reg + 1, high_part);
    ioapic_write_unlocked(reg, low_part); // Unmasking/configuring

    spin_unlock(&ioapic_lock);
    restore_irq(rflags);

    return true;
}

bool ioapic_set_isa_irq(uint8_t isa_irq, uint8_t vector, bool masked) {
    if (isa_irq > 15) return false;
    
    uint32_t gsi = irq_overrides[isa_irq];
    uint16_t flags = irq_flags[isa_irq];
    
    if (!ioapic_set_gsi(gsi, vector, flags, masked)) {
        return false;
    }
    
    printk("[APIC] Mapped ISA IRQ %d -> GSI %d -> Vector %d -> CPU %d (Masked: %s)\n", 
           isa_irq, gsi, vector, apic_get_id(), masked ? "Yes" : "No");
           
    return true;
}

void apic_init(void) {
    acpi_madt_t* madt = (acpi_madt_t*)acpi_find_table("APIC");
    if (!madt) panic("APIC: MADT table not found!");

    pic_disable();

    uint64_t msr_base = rdmsr(IA32_APIC_BASE_MSR);
    if (msr_base & IA32_APIC_BASE_MSR_EXTD) {
        panic("APIC: CPU is in x2APIC mode, but this OS driver only supports xAPIC (MMIO)!");
    }

    if (!(msr_base & IA32_APIC_BASE_MSR_ENABLE)) {
        msr_base |= IA32_APIC_BASE_MSR_ENABLE;
        wrmsr(IA32_APIC_BASE_MSR, msr_base);
    }
    
    uint64_t lapic_phys = msr_base & ~(uint64_t)0xFFF;
    lapic_base = p2v(lapic_phys);
    
    // Fixed: Uses vmm_map with correct args and PTE flags
    vmm_map(vmm_get_kernel_pml4(), (uint64_t)lapic_base, lapic_phys, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_PWT);

    uint32_t ver = lapic_read(LAPIC_VER);
    if ((ver & 0xFF) < 0x10) panic("APIC: External APIC not supported!");

    for (int i = 0; i < 16; i++) {
        irq_overrides[i] = i;
        irq_flags[i] = 0;
    }

    uint8_t* start = (uint8_t*)(madt + 1);
    uint8_t* end = (uint8_t*)madt + madt->header.length;
    uint8_t* p = start;

    uint32_t bsp_apic_id = apic_get_id();
    uint8_t bsp_acpi_uid = 0xFF; 
    bool bsp_uid_found = false; 

    // MADT Pass 1 - Find the ACPI UID of the current Boot Processor
    while (p < end) {
        acpi_madt_entry_t* entry = (acpi_madt_entry_t*)p;
        if (entry->length == 0) panic("APIC: Malformed MADT entry with length 0!");
        
        // Use MADT_LAPIC instead of 0
        if (entry->type == MADT_LAPIC) { 
            acpi_madt_lapic_t* lapic = (acpi_madt_lapic_t*)entry;
            if (lapic->apic_id == bsp_apic_id) {
                bsp_acpi_uid = lapic->processor_id; // Fixed naming
                bsp_uid_found = true;
            }
        }
        p += entry->length;
    }

    if (!bsp_uid_found) {
        printk("[APIC] Warning: BSP APIC ID not found in MADT! NMI routing may be broadcast-only.\n");
    }

    // MADT Pass 2 - Parse overrides, IOAPIC base, and NMIs
    uint64_t ioapic_phys = 0;
    uint32_t lint0_config = 0x10000;
    uint32_t lint1_config = 0x10000;
    p = start;

    while (p < end) {
        acpi_madt_entry_t* entry = (acpi_madt_entry_t*)p;
        switch (entry->type) {
            case MADT_IOAPIC: {
                acpi_madt_ioapic_t* io = (acpi_madt_ioapic_t*)entry;
                if (ioapic_phys == 0) {
                    ioapic_phys = io->ioapic_address; 
                    ioapic_id = io->ioapic_id;
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
            case MADT_LAPIC_NMI: {
                acpi_madt_lapic_nmi_t* nmi = (acpi_madt_lapic_nmi_t*)entry;
                // Fixed naming
                if (nmi->processor_id == 0xFF || nmi->processor_id == bsp_acpi_uid) {
                    uint32_t lvt = 0x400; 
                    uint16_t nmi_pol = nmi->flags & 0x3;
                    uint16_t nmi_trig = (nmi->flags >> 2) & 0x3;
                    
                    if (nmi_pol == 0x3) lvt |= (1 << 13);
                    if (nmi_trig == 0x3) lvt |= (1 << 15);
                    
                    if (nmi->lint == 0) lint0_config = lvt;
                    if (nmi->lint == 1) lint1_config = lvt;
                }
                break;
            }
        }
        p += entry->length;
    }

    if (ioapic_phys == 0) panic("APIC: No IOAPIC found in MADT!");
    if (ioapic_phys & 0xFFF) panic("APIC: IOAPIC physical address not page-aligned!");
    
    ioapic_base = p2v(ioapic_phys);
    
    // Fixed: Uses vmm_map with correct args and PTE flags
    vmm_map(vmm_get_kernel_pml4(), (uint64_t)ioapic_base, ioapic_phys, PTE_PRESENT | PTE_RW | PTE_PCD | PTE_PWT);
    
    uint32_t ioapic_ver = ioapic_read(IOAPICVER);
    ioapic_gsi_count = ((ioapic_ver >> 16) & 0xFF) + 1;
    
    printk("[APIC] IOAPIC enabled at %p (ID: %d), GSI Count: %d\n", ioapic_base, ioapic_id, ioapic_gsi_count);

    // Secure the LAPIC Local Vectors
    lapic_write(LAPIC_LVT_TIMER, 0x10000);
    lapic_write(LAPIC_LVT_ERROR, 0x10000);
    lapic_write(LAPIC_LVT_LINT0, lint0_config);
    lapic_write(LAPIC_LVT_LINT1, lint1_config);
    
    lapic_write(LAPIC_SVR, 0xFF | (1 << 8)); 
    printk("[APIC] Local APIC fully enabled at %p\n", lapic_base);

    ioapic_set_isa_irq(IRQ_TIMER, VEC_TIMER, false);
    ioapic_set_isa_irq(IRQ_KEYBOARD, VEC_KEYBOARD, false);
    ioapic_set_isa_irq(IRQ_MOUSE, VEC_MOUSE, false);
    
    apic_ready = true;
}

void apic_send_eoi(void) {
    if (!apic_ready) {
        printk("[APIC] Warning: apic_send_eoi() called before APIC initialized.\n");
        return;
    }
    lapic_write(LAPIC_EOI, 0);
}