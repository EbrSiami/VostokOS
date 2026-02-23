#include "sched.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "lib/panic.h"

static uint64_t tick_count = 0;

// This struct must exactly match the registers pushed in irq_common_stub
// plus the ones pushed by the CPU (RIP, CS, RFLAGS, RSP, SS)
typedef struct {
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    uint64_t irq_num, error_code;
    uint64_t rip, cs, rflags, rsp, ss;
} __attribute__((packed)) trapframe_t;

typedef struct thread {
    uint64_t rsp;
    void* stack_base;
    int id;
    struct thread* next;
} thread_t;

static thread_t* current_thread = NULL;
static int next_thread_id = 0;

void sched_init(void) {
    // Create the "Main" thread (the code currently running)
    thread_t* main_thread = (thread_t*)kmalloc(sizeof(thread_t));
    main_thread->id = next_thread_id++;
    main_thread->stack_base = NULL; // Main thread already has a stack from Limine
    main_thread->next = main_thread; // Circular list
    
    current_thread = main_thread;
    printk("[SCHED] Scheduler initialized. Main thread ID: 0\n");
}

// Dummy function if a thread accidentally returns
static void thread_exit(void) {
    int id = current_thread->id;
    printk("\n[SCHED] Thread %d exited.\n", id);
    panic("thread returned without exiting cleanly");
}

void thread_create(void (*entry_point)(void)) {
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(rflags));

    thread_t* new_thread = (thread_t*)kmalloc(sizeof(thread_t));
    new_thread->id = next_thread_id++;
    
    // Allocate 8KB for the thread's stack
    new_thread->stack_base = kmalloc(8192);
    
    // Top of the stack
    uint64_t* stack_top = (uint64_t*)((uint64_t)new_thread->stack_base + 8192);
    
    // Place the return address (thread_exit) at the top of the stack
    uint64_t* stack_ptr = stack_top;
    stack_ptr--; 
    *stack_ptr = (uint64_t)thread_exit;

    // Reserve space for the trapframe
    uint64_t tf_addr = (uint64_t)stack_ptr - sizeof(trapframe_t);
    tf_addr &= ~0x0FULL;
    trapframe_t* frame = (trapframe_t*)tf_addr;
    memset(frame, 0, sizeof(trapframe_t));

    frame->rip = (uint64_t)entry_point;
    frame->cs = 0x08;
    frame->rflags = 0x202;

    frame->rsp = (uint64_t)stack_top; 
    frame->ss = 0x10;

    new_thread->rsp = tf_addr;

    new_thread->next = current_thread->next;
    current_thread->next = new_thread;

    printk("[SCHED] Created thread ID: %d\n", new_thread->id);
    
    if (rflags & 0x200) { 
        __asm__ volatile("sti");
    }
}

uint64_t sched_tick(uint64_t current_rsp) {

    if (!current_thread || current_thread->next == current_thread)
        return 0;

    if (++tick_count % SCHED_SLICE != 0)
        return 0;  // not time to switch yet

    current_thread->rsp = current_rsp;
    current_thread = current_thread->next;
    return current_thread->rsp;
}