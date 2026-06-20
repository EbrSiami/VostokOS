.section .text
.global gdt_flush
.type gdt_flush, @function

# Changing C signature to: extern void gdt_flush(struct gdt_ptr *gdt_ptr_struct);
# %rdi still holds the memory address of the struct.
gdt_flush:
    lgdt (%rdi)
    
    pushq $0x08          
    leaq flush_cs(%rip), %rax
    pushq %rax
    lretq
    
flush_cs:
    # Reload data segment registers.
    # 0x10 is the Kernel Data Segment selector (Index 2 in GDT, GDT[2] * 8 = 0x10)
    movw $0x10, %ax      
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %ss
    
    # NOTE: Loading GS with kernel data selector works for now, 
    # but in the future GS will likely be reserved for per-CPU data or Userspace TLS (via swapgs).
    movw %ax, %gs
    
    ret
    