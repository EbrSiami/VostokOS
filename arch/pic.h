#ifndef PIC_H
#define PIC_H

#include <stdint.h>

// Here is the PIC ports
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

// and of course the PIC commands
#define PIC_EOI         0x20    //end of interrupts

// now lets initialize and remap the PIC
void pic_remap(uint8_t offset1, uint8_t offset2);

// sending the E01 (end of interrupt) to PIC
void pic_send_eoi(uint8_t irq);

//enabling, disabling specifie IRQ lines
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);

//disable PIC Completely (if using APIC, that i prefer)
void pic_disable(void);

#endif