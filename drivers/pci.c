#include "pci.h"
#include "acpi.h"
#include "../lib/printk.h"
#include "../mm/vmm.h"
#include "../mm/pmm.h"

// MCFG Allocation Structure
typedef struct {
    uint64_t base_address;
    uint16_t pci_segment_group;
    uint8_t  start_bus_number;
    uint8_t  end_bus_number;
    uint32_t reserved;
} __attribute__((packed)) mcfg_allocation_t;

// The Device Registry
#define PCI_MAX_DEVICES 256
static pci_device_t pci_devices[PCI_MAX_DEVICES];
static int pci_device_count = 0;

// Virtual address where we will map ECAM
// We use a high virtual address far away from the kernel and HHDM
#define ECAM_VIRT_BASE 0xffffc00000000000

// Helper to map ECAM physically to Uncacheable Virtual Memory
static void map_ecam_bus(uint64_t phys_base, uint8_t bus) {
    // 1 Bus = 32 Devices * 8 Functions * 4096 Bytes = 1MB
    uint64_t bus_phys = phys_base + (bus << 20);
    uint64_t bus_virt = ECAM_VIRT_BASE + (bus << 20);

    // Map 1MB (256 pages) as MMIO (Uncacheable, No Execute)
    for (int i = 0; i < 256; i++) {
        vmm_map(vmm_get_kernel_pml4(), 
                bus_virt + (i * PAGE_SIZE), 
                bus_phys + (i * PAGE_SIZE), 
                PTE_MMIO);
    }
}

static void pci_check_device(uint8_t bus, uint8_t device, uint8_t function) {
    if (pci_device_count >= PCI_MAX_DEVICES) return;

    // Calculate virtual ECAM address
    uint64_t offset = ((uint64_t)bus << 20) | ((uint64_t)device << 15) | ((uint64_t)function << 12);
    volatile pci_device_header_t* hdr = (volatile pci_device_header_t*)(ECAM_VIRT_BASE + offset);

    // Vendor ID 0xFFFF means device doesn't exist
    if (hdr->vendor_id == 0xFFFF) return;

    // Register the device
    pci_device_t* dev = &pci_devices[pci_device_count++];
    dev->bus = bus;
    dev->device = device;
    dev->function = function;
    dev->vendor_id = hdr->vendor_id;
    dev->device_id = hdr->device_id;
    dev->class_code = hdr->class_code;
    dev->subclass = hdr->subclass;
    dev->prog_if = hdr->prog_if;
    dev->config_space = hdr;

    printk("[PCI] Registered: Bus %02x Dev %02x Func %x | Ven:%04x Dev:%04x Class:%02x\n",
           bus, device, function, dev->vendor_id, dev->device_id, dev->class_code);

    // Check Multi-Function (Only if we are currently checking function 0)
    if (function == 0 && (hdr->header_type & 0x80)) {
        for (uint8_t f = 1; f < 8; f++) {
            pci_check_device(bus, device, f);
        }
    }
}

void pci_init(void) {
    acpi_mcfg_t* mcfg = (acpi_mcfg_t*)acpi_find_table("MCFG");
    if (!mcfg) {
        printk("[PCI] MCFG not found! ECAM not supported.\n");
        return;
    }

    int entries = (mcfg->header.length - sizeof(acpi_mcfg_t)) / sizeof(mcfg_allocation_t);
    mcfg_allocation_t* allocs = (mcfg_allocation_t*)((uint64_t)mcfg + sizeof(acpi_mcfg_t));

    printk("[PCI] Enumerating via ECAM...\n");

    for (int i = 0; i < entries; i++) {
        uint64_t phys_base = allocs[i].base_address;
        
        for (uint16_t bus = allocs[i].start_bus_number; bus <= allocs[i].end_bus_number; bus++) {
            // Map this specific 1MB bus chunk as Uncacheable
            map_ecam_bus(phys_base, bus);
            
            for (uint8_t device = 0; device < 32; device++) {
                pci_check_device(bus, device, 0);
            }
        }
    }
    printk("[PCI] Total devices registered: %d\n", pci_device_count);
}

// Lookup by Vendor/Device (Good for specific drivers like a specific Realtek NIC)
pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && pci_devices[i].device_id == device_id) {
            return &pci_devices[i];
        }
    }
    return NULL;
}

// Lookup by Class/Subclass (Good for generic AHCI, NVMe, XHCI drivers)
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].class_code == class_code && pci_devices[i].subclass == subclass) {
            return &pci_devices[i];
        }
    }
    return NULL;
}