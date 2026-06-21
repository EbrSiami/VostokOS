#include "panic.h"
#include "printk.h"
#include "../arch/idt.h"

extern void printk_force_unlock(void);

void panic_impl(const char *message, const char *file, int line, struct interrupt_registers *regs) {

    __asm__ volatile("cli");

    printk_force_unlock();

    printk("\n\n====================================================================\n");
    printk("                         KERNEL PANIC                               \n");
    printk("====================================================================\n");
    printk("Message:  %s\n", message);
    printk("Location: %s at line %d\n", file, line);

    if (regs != 0) {
        uint64_t cr2_val = 0;
        if (regs->int_no == 14) { // Page Fault
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2_val));
        }

        printk("--------------------------------------------------------------------\n");
        printk("EXCEPTION CPU DUMP (Vector: %lld, Error Code: 0x%llx)\n", regs->int_no, regs->error_code);
        if (regs->int_no == 14) {
            printk("Faulting Linear Address (CR2): 0x%016llx\n", cr2_val);
        }
        printk("--------------------------------------------------------------------\n");
        printk("RIP: 0x%016llx   CS:  0x%02llx   RFLAGS: 0x%08llx\n", regs->rip, regs->cs, regs->rflags);
        printk("RSP: 0x%016llx   SS:  0x%02llx   RBP:    0x%016llx\n", regs->rsp, regs->ss, regs->rbp);
        printk("RAX: 0x%016llx   RBX: 0x%016llx   RCX:    0x%016llx\n", regs->rax, regs->rbx, regs->rcx);
        printk("RDX: 0x%016llx   RSI: 0x%016llx   RDI:    0x%016llx\n", regs->rdx, regs->rsi, regs->rdi);
        printk("R8:  0x%016llx   R9:  0x%016llx   R10:    0x%016llx\n", regs->r8,  regs->r9,  regs->r10);
        printk("R11: 0x%016llx   R12: 0x%016llx   R13:    0x%016llx\n", regs->r11, regs->r12, regs->r13);
        printk("R14: 0x%016llx   R15: 0x%016llx\n", regs->r14, regs->r15);
    }
    
    printk("====================================================================\n");
    printk("System Halted Safely.\n");

    for (;;) {
        __asm__ volatile("hlt");
    }
}