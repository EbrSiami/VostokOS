.section .text

# Macro for ISRs without error codes
.macro ISR_NOERR num
.global isr\num
isr\num:
    pushq $0                # Push dummy error code
    pushq $\num             # Push interrupt number
    jmp isr_common_stub
.endm

# Macro for ISRs with error codes
.macro ISR_ERR num
.global isr\num
isr\num:
    pushq $\num             # Push interrupt number (error code already pushed)
    jmp isr_common_stub
.endm

# Macro for IRQ handlers
.macro IRQ num, mapped
.global irq\num
irq\num:
    pushq $0                # Dummy error code
    pushq $\mapped          # Push IRQ number
    jmp irq_common_stub
.endm

# Define all ISRs
ISR_NOERR 0
ISR_NOERR 1
ISR_NOERR 2
ISR_NOERR 3
ISR_NOERR 4
ISR_NOERR 5
ISR_NOERR 6
ISR_NOERR 7
ISR_ERR   8
ISR_NOERR 9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_ERR   17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_ERR   21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_ERR   30
ISR_NOERR 31

# Define all IRQs (0-15 mapped to 32-47)
IRQ 0, 32
IRQ 1, 33
IRQ 2, 34
IRQ 3, 35
IRQ 4, 36
IRQ 5, 37
IRQ 6, 38
IRQ 7, 39
IRQ 8, 40
IRQ 9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47

# Common ISR stub
isr_common_stub:
    # Save all registers
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    
    # Call C handler
    # Arguments: rdi = ISR number, rsi = error code
    cld
    movq 120(%rsp), %rdi    # Get ISR number from stack
    movq 128(%rsp), %rsi    # Get error code from stack

    # Aligning the stack before calling a C function
    movq %rsp, %rbp         # current RSP value
    andq $-16, %rsp         # Align to 16 bytes (down)
    
    call isr_handler

    movq %rbp, %rsp         # Restore the original RSP
    
    # Restore registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    
    # Clean up error code and ISR number
    addq $16, %rsp
    
    iretq

# Common IRQ stub
irq_common_stub:
    # 1. Save all registers (15 registers)
    pushq %rax
    pushq %rbx
    pushq %rcx
    pushq %rdx
    pushq %rsi
    pushq %rdi
    pushq %rbp
    pushq %r8
    pushq %r9
    pushq %r10
    pushq %r11
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    
    cld
    # 2. Preparing the arguments
    movq 120(%rsp), %rdi    # IRQ Number
    movq %rsp, %rsi         # Current RSP

    # 3. Stack alignment using RBP
    movq %rsp, %rbp         # Copy of current RSP in RBPس
    andq $-16, %rsp         # Alignment to 16 bytes

    call irq_handler

    # 4. Return the stack to its pre-alignment state
    movq %rbp, %rsp

    # 5. Check for Context Switch
    test %rax, %rax
    jz .no_switch
    movq %rax, %rsp

.no_switch:
    # Restore registers
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %r11
    popq %r10
    popq %r9
    popq %r8
    popq %rbp
    popq %rdi
    popq %rsi
    popq %rdx
    popq %rcx
    popq %rbx
    popq %rax
    
    # Clean up error code and IRQ number
    addq $16, %rsp   
    iretq

# IDT flush function
.global idt_flush
.type idt_flush, @function
idt_flush:
    lidt (%rdi)
    ret
    