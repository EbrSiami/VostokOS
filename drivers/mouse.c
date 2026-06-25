#include "mouse.h"
#include "../lib/printk.h"
#include "../display/framebuffer.h"
#include "../arch/idt.h"

#define PS2_CMD_PORT    0x64
#define PS2_DATA_PORT   0x60

static mouse_state_t mouse;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

// Wait until we can write to the controller
static void mouse_wait_write(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_CMD_PORT) & 2) == 0) return;
    }
}

// Wait until there is data to read
static void mouse_wait_read(void) {
    int timeout = 100000;
    while (timeout--) {
        if ((inb(PS2_CMD_PORT) & 1) == 1) return;
    }
}

// Send a command specifically to the mouse
static void mouse_write(uint8_t write) {
    mouse_wait_write();
    outb(PS2_CMD_PORT, 0xD4); // Tell controller we are talking to the mouse
    mouse_wait_write();
    outb(PS2_DATA_PORT, write);
}

static uint8_t mouse_read(void) {
    mouse_wait_read();
    return inb(PS2_DATA_PORT);
}

void mouse_init(void) {
    mouse.x = fb_get()->width / 2;
    mouse.y = fb_get()->height / 2;
    mouse.left_click = false;
    mouse.right_click = false;
    mouse.middle_click = false;

    // 1. Enable auxiliary device (mouse)
    mouse_wait_write();
    outb(PS2_CMD_PORT, 0xA8);
    
    // 2. Enable IRQ12 by modifying Compaq status byte
    mouse_wait_write();
    outb(PS2_CMD_PORT, 0x20); // Command to read status byte
    uint8_t status = mouse_read();
    status |= (1 << 1); // Enable IRQ12
    status &= ~(1 << 5); // Disable mouse clock line
    
    mouse_wait_write();
    outb(PS2_CMD_PORT, 0x60); // Command to write status byte
    mouse_wait_write();
    outb(PS2_DATA_PORT, status);
    
    // 3. Set mouse to use default settings
    mouse_write(0xF6);
    mouse_read(); // Acknowledge (0xFA)
    
    // 4. Enable data reporting
    mouse_write(0xF4);
    mouse_read(); // Acknowledge (0xFA)

    irq_register_handler(12, mouse_irq_handler);
    
    printk("[MOUSE] PS/2 Mouse initialized.\n");
}

void mouse_handler(void) {
    while (1) {
        uint8_t status = inb(PS2_CMD_PORT);

        if ((status & 0x01) == 0) break; // No data

        // if bit 5 is clear, this byte belongs to the keyboard!
        if ((status & 0x20) == 0) break; 

        uint8_t data = inb(PS2_DATA_PORT);

        switch (mouse_cycle) {
            case 0:
                // Sync byte: Bit 3 is always 1 in a valid packet
                if (data & 0x08) {
                    mouse_packet[0] = data;
                    mouse_cycle++;
                }
                break;
            case 1:
                mouse_packet[1] = data;
                mouse_cycle++;
                break;
            case 2:
                mouse_packet[2] = data;
                
                // We have a full packet, process it!
                mouse.left_click = mouse_packet[0] & 0x01;
                mouse.right_click = mouse_packet[0] & 0x02;
                mouse.middle_click = mouse_packet[0] & 0x04;

                // X movement (Byte 1)
                int d_x = mouse_packet[1];
                // If X sign bit is set, it's negative (two's complement)
                if (mouse_packet[0] & 0x10) d_x -= 256; 
                
                // Y movement (Byte 2)
                int d_y = mouse_packet[2];
                // If Y sign bit is set, it's negative
                if (mouse_packet[0] & 0x20) d_y -= 256;

                // Update coordinates (PS/2 Y axis goes up, screen Y axis goes down)
                mouse.x += d_x;
                mouse.y -= d_y;

                // Clamp to screen bounds
                framebuffer_t* fb = fb_get();
                if (mouse.x < 0) mouse.x = 0;
                if (mouse.x >= (int)fb->width) mouse.x = fb->width - 1;
                if (mouse.y < 0) mouse.y = 0;
                if (mouse.y >= (int)fb->height) mouse.y = fb->height - 1;

                mouse_cycle = 0;
                break;
        }
    }
}

mouse_state_t* mouse_get_state(void) {
    return &mouse;
}

uint64_t mouse_irq_handler(uint64_t current_rsp)
{
    (void)current_rsp;

    mouse_handler();

    return 0;
}