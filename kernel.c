#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "display/framebuffer.h"
#include "display/terminal.h"
#include "lib/printk.h"
#include "arch/gdt.h"
#include "arch/idt.h"
#include "arch/apic.h"
#include "drivers/keyboard.h"
#include "shell/shell.h"
#include "drivers/timer.h"
#include "mm/pmm.h"
#include "mm/vmm.h"
#include "lib/panic.h"
#include "mm/heap.h"
#include "drivers/acpi.h"
#include "drivers/pci.h"
#include "kernel/sched.h"

__attribute__((used, section(".requests")))
static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

__attribute__((used, section(".requests")))
static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

uint64_t hhdm_offset = 0;

static struct limine_memmap_response *memmap_response = NULL;

struct limine_memmap_response* get_memory_map(void) {
    return memmap_response;
}

static void hcf(void) {
    __asm__ ("cli");
    for (;;) {
        __asm__ ("hlt");
    }
}

// void task_a(void) {
//     for (;;) {
//         printk("FUCK ISRAEL\n");
//         timer_sleep_ms(1000);
//     }
// }

void _start(void) {

    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    if (memmap_request.response != NULL) {
        memmap_response = (struct limine_memmap_response*)memmap_request.response;
    }

    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    
    // Initialize framebuffer and terminal
    fb_init((uint32_t*)fb->address, fb->width, fb->height, fb->pitch, fb->bpp);
    terminal_init();
    
    printk("=== VostokOS Kernel ===\n\n");
    
    if (hhdm_request.response != NULL) {
        hhdm_offset = hhdm_request.response->offset;
        printk("[KERNEL] HHDM Offset: 0x%llx\n", hhdm_offset);
    } else {
        printk("[ERROR] HHDM response is NULL! System halted.\n");
        hcf();
    }

    if (memmap_request.response != NULL) {
        memmap_response = (struct limine_memmap_response*)memmap_request.response;
        printk("[KERNEL] Memory map retrieved.\n");
    }

        if (memmap_response != NULL) {
        printk("[KERNEL] Initializing PMM...\n");
        pmm_init(memmap_response, hhdm_offset);
    } else {
        printk("[ERROR] Memory map is NULL! Cannot init PMM.\n");
        hcf();
    }

    // Initialize VMM
    printk("[KERNEL] Initializing VMM...\n");
    vmm_init();
    
    // Initialize Heap
    printk("[KERNEL] Initializing Heap...\n");
    kheap_init();

    // Initialize ACPI
    printk("[KERNEL] Initializing ACPI...\n");
    acpi_init();

    // Initialize CPU structures
    gdt_init();
    idt_init();

    // Initialize APIC
    printk("[KERNEL] Initializing APIC...\n");
    apic_init();

    sched_init();

    // here we can add our threads by thread_create(task_x);
    
    printk("[KERNEL] Initializing PCI...\n");
    pci_init();

    // Initialize keyboard
    keyboard_init();

    timer_init(100);  // 100 Hz (10ms per tick)

    // Enable interrupts
    __asm__ volatile ("sti");

    printk("\n[KERNEL] Interrupts enabled!\n");
    printk("[KERNEL] System initialized successfully\n");
    
    shell_init();
    
    // Main loop - just wait for interrupts
    for (;;) {
        __asm__ volatile ("cli");
        if (keyboard_has_char()) {
            __asm__ volatile ("sti"); // Re-enable safely
            char c = keyboard_get_char();
            shell_process_char(c);
        } else {
            __asm__ volatile ("sti; hlt"); 
        }
    }
}