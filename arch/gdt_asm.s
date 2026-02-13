.section .text
.global gdt_flush
.type gdt_flush, @function

gdt_flush:
    # Load GDT
    lgdt (%rdi)
    
    # Reload code segment by doing a far jump
    # We need to push the new CS and the return address
    pushq $0x08          # Kernel code segment selector (entry 1 in GDT)
    leaq flush_cs(%rip), %rax
    pushq %rax
    lretq
    
flush_cs:
    # Reload data segment registers
    movw $0x10, %ax      # Kernel data segment selector (entry 2 in GDT)
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss
    
    ret
    