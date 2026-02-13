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
ISR_NOERR 31

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
    call isr_handler
    
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

# IDT flush function
.global idt_flush
.type idt_flush, @function
idt_flush:
    lidt (%rdi)
    ret
    