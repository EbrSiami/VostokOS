#include "acpi.h"
#include "../limine.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../lib/panic.h"

// Request RSDP from Limine
__attribute__((used, section(".requests")))
static volatile struct limine_rsdp_request rsdp_request = {
    .id = LIMINE_RSDP_REQUEST,
    .revision = 0
};

extern uint64_t hhdm_offset;

static acpi_rsdp_t* rsdp = NULL;
static acpi_sdt_header_t* xsdt = NULL;
static acpi_sdt_header_t* rsdt = NULL;
static int use_xsdt = 0;

// Helper: Validate Checksum
static int validate_checksum(void* ptr, size_t length) {
    uint8_t* bytes = (uint8_t*)ptr;
    uint8_t sum = 0;
    for (size_t i = 0; i < length; i++) {
        sum += bytes[i];
    }
    return sum == 0;
}

// Helper: Convert Phys to Virt (HHDM)
static inline void* p2v(uint64_t phys) {
    return (void*)(phys + hhdm_offset);
}

static inline void outw(uint16_t port, uint16_t value) {
    __asm__ volatile ("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void outb(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

void acpi_init(void) {
    if (rsdp_request.response == NULL || rsdp_request.response->address == NULL) {
        panic("ACPI: RSDP not found via Limine!");
    }

    // Limine returns a Virtual Address for the RSDP request response
    rsdp = (acpi_rsdp_t*)rsdp_request.response->address;
    
    printk("[ACPI] RSDP found at %p (Rev: %d)\n", rsdp, rsdp->revision);

    // Prefer XSDT (64-bit) over RSDT (32-bit)
    if (rsdp->revision >= 2 && rsdp->xsdt_address != 0) {
        use_xsdt = 1;
        xsdt = (acpi_sdt_header_t*)p2v(rsdp->xsdt_address);
        
        if (!validate_checksum(xsdt, xsdt->length)) {
            panic("ACPI: XSDT checksum failed!");
        }
        printk("[ACPI] Using XSDT at %p\n", xsdt);
    } else {
        use_xsdt = 0;
        rsdt = (acpi_sdt_header_t*)p2v((uint64_t)rsdp->rsdt_address);
        
        if (!validate_checksum(rsdt, rsdt->length)) {
            panic("ACPI: RSDT checksum failed!");
        }
        printk("[ACPI] Using RSDT at %p\n", rsdt);
    }
}

void* acpi_find_table(const char* signature) {
    acpi_sdt_header_t* header = NULL;

    if (use_xsdt) {
        int entries = (xsdt->length - sizeof(acpi_sdt_header_t)) / 8;
        uint64_t* pointers = (uint64_t*)((uint64_t)xsdt + sizeof(acpi_sdt_header_t));

        for (int i = 0; i < entries; i++) {
            header = (acpi_sdt_header_t*)p2v(pointers[i]);
            if (strncmp(header->signature, signature, 4) == 0) {
                goto found;
            }
        }
    } else {
        int entries = (rsdt->length - sizeof(acpi_sdt_header_t)) / 4;
        uint32_t* pointers = (uint32_t*)((uint64_t)rsdt + sizeof(acpi_sdt_header_t));

        for (int i = 0; i < entries; i++) {
            header = (acpi_sdt_header_t*)p2v((uint64_t)pointers[i]);
            if (strncmp(header->signature, signature, 4) == 0) {
                goto found;
            }
        }
    }
    return NULL;

found:
    if (!validate_checksum(header, header->length)) {
        printk("[ACPI] Warning: Table %.4s found but checksum failed!\n", signature);
        return NULL;
    }
    return (void*)header;
}

void acpi_reboot(void) {
    acpi_fadt_t* fadt = (acpi_fadt_t*)acpi_find_table("FACP");
    if (!fadt) {
        printk("[ACPI] FADT not found, cannot reboot!\n");
        return;
    }

    // Check if ACPI reboot is supported (Bit 10 of flags)
    if (!(fadt->flags & (1 << 10))) {
        printk("[ACPI] Hardware doesn't support ACPI reboot.\n");
        return;
    }

    // Address space 1 means System I/O
    if (fadt->reset_reg.address_space == 1) {
        uint16_t port = (uint16_t)fadt->reset_reg.address;
        uint8_t value = fadt->reset_value;
        __asm__ volatile("outb %0, %1" :: "a"(value), "Nd"(port));
    }
    
    // Fallback
    for(;;) __asm__("cli; hlt");
}

void acpi_shutdown(void) {
    
    printk("[ACPI] Shutdown requested. (Requires AML Parser for real hardware)\n");
    
    // QEMU Fallback
    __asm__ volatile("outw %0, %1" :: "a"((uint16_t)0x2000), "Nd"((uint16_t)0x604));
    
    printk("System halted. Please power off manually.\n");
    for(;;) __asm__("cli; hlt");
}