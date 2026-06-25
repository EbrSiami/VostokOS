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
#include "fs/vfs.h"
#include "fs/tar.h"
#include "drivers/mouse.h"
#include "gui/bmp.h"

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

__attribute__((used, section(".requests")))
static volatile struct limine_module_request module_request = {
    .id = LIMINE_MODULE_REQUEST,
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

// void task_b(void) {
//     for (;;) {
//         printk("FUCK USA\n");
//         timer_sleep_ms(1500);
//     }
// }

void _start(void) {
    // 1. Critical Check: Framebuffer (Required for any visual output)
    if (framebuffer_request.response == NULL || 
        framebuffer_request.response->framebuffer_count < 1) {
        hcf();
    }

    // Initialize display early to see boot logs
    struct limine_framebuffer *fb = framebuffer_request.response->framebuffers[0];
    fb_init((uint32_t*)fb->address, fb->width, fb->height, fb->pitch, fb->bpp);
    terminal_init();
    
    printk("=== VostokOS Kernel ===\n\n");

    // 2. Critical Check: HHDM (Higher Half Direct Map)
    if (hhdm_request.response != NULL) {
        hhdm_offset = hhdm_request.response->offset;
        printk("[KERNEL] HHDM Offset: 0x%llx\n", hhdm_offset);
    } else {
        printk("[ERROR] HHDM response is NULL! System halted.\n");
        hcf();
    }

    // 3. Critical Check: Memory Map (Required for PMM)
    if (memmap_request.response != NULL) {
        memmap_response = memmap_request.response;
        printk("[KERNEL] Memory map retrieved.\n");
        
        // Initialize Physical Memory Manager
        printk("[KERNEL] Initializing PMM...\n");
        pmm_init(memmap_response, hhdm_offset);
    } else {
        printk("[ERROR] Memory map is NULL! Cannot initialize PMM.\n");
        hcf();
    }

    // 4. Memory Subsystems
    printk("[KERNEL] Initializing VMM...\n");
    vmm_init();
    
    printk("[KERNEL] Initializing Heap...\n");
    kheap_init();

    // 5. Filesystem and Modules
    vfs_init();

    if (module_request.response != NULL && module_request.response->module_count > 0) {
        struct limine_file *module = module_request.response->modules[0];
        printk("[KERNEL] Found module: %s (Size: %llu bytes)\n", module->path, module->size);
        tar_init(module->address);
    } else {
        printk("[KERNEL] Warning: No ramdisk module loaded.\n");
    }

    // 6. Architecture & CPU Initialization
    printk("[KERNEL] Initializing ACPI...\n");
    acpi_init();

    gdt_init();
    idt_init();

    printk("[KERNEL] Initializing APIC...\n");
    apic_init();

    // 7. Scheduling and Multitasking
    sched_init();
    
    // here we can add our threads by thread_create(task_x);
    // thread_create(task_a);
    // thread_create(task_b);

    // 8. Hardware Drivers
    printk("[KERNEL] Initializing PCI...\n");
    pci_init();

    keyboard_init();
    timer_init(100); // 100 Hz
    
    // 9. Enable Interrupts (Only after IDT and Drivers are ready)
    __asm__ volatile ("sti");
    printk("\n[KERNEL] Interrupts enabled!\n");
    
    // 10. GUI and Assets
    fb_enable_double_buffering();
    mouse_init();

    uint32_t* cursor_img = NULL;
    int cursor_w = 0, cursor_h = 0;
    
    vfs_node_t* cursor_file = vfs_open("cursor.bmp");
    if (cursor_file) {
        cursor_img = bmp_load(cursor_file->data, &cursor_w, &cursor_h);
        printk("[KERNEL] Cursor loaded successfully.\n");
    } else {
        printk("[KERNEL] Warning: cursor.bmp not found.\n");
    }

    // 11. Finalize Shell
    printk("[KERNEL] System initialized successfully.\n\n");
    shell_init();
    
    static uint32_t mouse_bg[64 * 64];
    static int old_mouse_x = -1;
    static int old_mouse_y = -1;
    static int old_mouse_w = 0;
    static int old_mouse_h = 0;

    // Main loop - The core of our future Window Manager
    for (;;) {
        __asm__ volatile ("cli");

        if (keyboard_has_char()) {
            __asm__ volatile ("sti"); 
            char c = keyboard_get_char();
            shell_process_char(c);
        } else {
            __asm__ volatile ("sti; hlt"); 
        }

        mouse_state_t* m = mouse_get_state();
        
        int cur_w = cursor_img ? cursor_w : 10;
        int cur_h = cursor_img ? cursor_h : 10;

        if (old_mouse_x != -1) {
            for (int y = 0; y < old_mouse_h; y++) {
                for (int x = 0; x < old_mouse_w; x++) {
                    fb_put_pixel(old_mouse_x + x, old_mouse_y + y, mouse_bg[y * old_mouse_w + x]);
                }
            }
        }

        for (int y = 0; y < cur_h; y++) {
            for (int x = 0; x < cur_w; x++) {
                mouse_bg[y * cur_w + x] = fb_get_pixel(m->x + x, m->y + y);
            }
        }

        if (cursor_img) {
            fb_blit(m->x, m->y, cursor_w, cursor_h, cursor_img);
        } else {
            uint32_t color = m->left_click ? 0xFF00FF00 : 0xFFFF0000;
            fb_fill_rect(m->x, m->y, 10, 10, color);
        }

        old_mouse_x = m->x;
        old_mouse_y = m->y;
        old_mouse_w = cur_w;
        old_mouse_h = cur_h;

        fb_swap();
    }
}