#ifndef ACPI_H
#define ACPI_H

#include <stdint.h>
#include <stddef.h>

// Standard ACPI Header
typedef struct {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed)) acpi_sdt_header_t;

// RSDP (Root System Description Pointer)
typedef struct {
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;
    // XSDP Extension (Revision 2.0+)
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_t;

// MADT (Multiple APIC Description Table)
typedef struct {
    acpi_sdt_header_t header;
    uint32_t local_apic_address;
    uint32_t flags;
    // Followed by variable length records...
} __attribute__((packed)) acpi_madt_t;

// MADT Entry Header
typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) acpi_madt_entry_t;

// MADT Entry Types
#define MADT_LAPIC   0
#define MADT_IOAPIC  1
#define MADT_ISO     2
#define MADT_NMI     4

// Type 0: Processor Local APIC
typedef struct {
    acpi_madt_entry_t header;
    uint8_t processor_id;
    uint8_t apic_id;
    uint32_t flags; // Bit 0 = Processor Enabled
} __attribute__((packed)) acpi_madt_lapic_t;

// Type 1: I/O APIC
typedef struct {
    acpi_madt_entry_t header;
    uint8_t ioapic_id;
    uint8_t reserved;
    uint32_t ioapic_address;
    uint32_t gsi_base;
} __attribute__((packed)) acpi_madt_ioapic_t;

// Type 2: Interrupt Source Override
typedef struct {
    acpi_madt_entry_t header;
    uint8_t bus;    // Always 0 (ISA)
    uint8_t source; // IRQ on the PIC (e.g., 1 for Keyboard)
    uint32_t gsi;   // Global System Interrupt (Input to IOAPIC)
    uint16_t flags; // Polarity and Trigger mode
} __attribute__((packed)) acpi_madt_iso_t;

// MCFG (PCI Express Memory Mapped Configuration)
typedef struct {
    acpi_sdt_header_t header;
    uint64_t reserved;
    // Followed by allocation structures...
} __attribute__((packed)) acpi_mcfg_t;

// Initialize ACPI subsystem
void acpi_init(void);

// Find a table by signature (e.g., "APIC", "MCFG")
void* acpi_find_table(const char* signature);

#endif