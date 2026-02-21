#ifndef PCI_H
#define PCI_H

#include <stdint.h>
#include <stdbool.h>

// Standard PCI Header fields
typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t  revision_id;
    uint8_t  prog_if;
    uint8_t  subclass;
    uint8_t  class_code;
    uint8_t  cache_line_size;
    uint8_t  latency_timer;
    uint8_t  header_type;
    uint8_t  bist;
} __attribute__((packed)) pci_device_header_t;

// The Device Registry Entry
typedef struct {
    uint8_t  bus;
    uint8_t  device;
    uint8_t  function;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t  class_code;
    uint8_t  subclass;
    uint8_t  prog_if;
    // Volatile pointer to Uncacheable ECAM memory
    volatile pci_device_header_t* config_space; 
} pci_device_t;

void pci_init(void);

// Lookup functions for drivers
pci_device_t* pci_get_device(uint16_t vendor_id, uint16_t device_id);
pci_device_t* pci_get_device_by_class(uint8_t class_code, uint8_t subclass);

#endif