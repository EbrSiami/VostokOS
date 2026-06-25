#include "sched.h"
#include "../arch/idt.h"
#include "../mm/heap.h"
#include "../lib/string.h"
#include "../lib/printk.h"
#include "../lib/panic.h"
#include "../drivers/timer.h"

typedef enum {
    THREAD_RUNNING,
    THREAD_SLEEPING
} thread_state_t;

typedef struct thread {
    uint64_t rsp;
    void* stack_base;
    int id;
    thread_state_t state;      // thread current state
    uint64_t wake_up_tick;     // when it must wake up
    struct thread* next;
} thread_t;

static thread_t* current_thread = NULL;
static thread_t* idle_thread = NULL;
static int next_thread_id = 0;

// The Idle Task: Runs only when all other threads are sleeping
static void idle_task(void) {
    for (;;) {
        __asm__ volatile("sti; hlt");
    }
}

// Dummy function if a thread accidentally returns
static void thread_exit(void) {
    int id = current_thread->id;
    printk("\n[SCHED] Thread %d exited.\n", id);
    panic("thread returned without exiting cleanly");
}

void sched_init(void) {
    // Create the "Main" thread (the code currently running)
    thread_t* main_thread = (thread_t*)kmalloc(sizeof(thread_t));
    main_thread->id = next_thread_id++;
    main_thread->stack_base = NULL; // Main thread already has a stack from Limine
    main_thread->state = THREAD_RUNNING;
    main_thread->next = main_thread; // Circular list
    
    current_thread = main_thread;

    // Create the idle thread
    thread_create(idle_task);
    idle_thread = current_thread->next; // It was just inserted after current
    
    printk("[SCHED] Scheduler initialized. Main thread ID: 0\n");
}

void thread_create(void (*entry_point)(void)) {
    uint64_t rflags;
    __asm__ volatile("pushfq; pop %0; cli" : "=r"(rflags));
    
    thread_t* new_thread = (thread_t*)kmalloc(sizeof(thread_t));
    new_thread->id = next_thread_id++;
    new_thread->state = THREAD_RUNNING;

    // Use kcalloc to zero the stack. Real hardware will crash if 
    // uninitialized garbage is popped into segment registers during iretq.
    new_thread->stack_base = kcalloc(1, 8192);
    uint64_t* stack_top = (uint64_t*)((uint64_t)new_thread->stack_base + 8192);
    stack_top = (uint64_t*)((uint64_t)stack_top & ~0xF);

    uint64_t* stack_ptr = stack_top;
    stack_ptr--; 
    *stack_ptr = (uint64_t)thread_exit;

    uint64_t tf_addr = (uint64_t)stack_ptr - sizeof(struct interrupt_registers);
    struct interrupt_registers* frame = (struct interrupt_registers*)tf_addr;
    memset(frame, 0, sizeof(struct interrupt_registers));

    frame->rip = (uint64_t)entry_point;
    frame->cs = 0x08;
    frame->rflags = 0x202;

    frame->rsp = (uint64_t)stack_ptr; 
    frame->ss = 0x10;

    new_thread->rsp = tf_addr;

    new_thread->next = current_thread->next;
    current_thread->next = new_thread;

    printk("[SCHED] Created thread ID: %d\n", new_thread->id);

    if (rflags & 0x200) { 
        __asm__ volatile("sti");
    }
}

// it will be called by timer_sleep_ms()
void sched_sleep_current_thread(uint64_t wake_tick) {
    if (current_thread) {
        current_thread->state = THREAD_SLEEPING;
        current_thread->wake_up_tick = wake_tick;
        sched_yield();
    }
}

uint64_t sched_tick(uint64_t current_rsp) {
    if (!current_thread) return current_rsp;

    current_thread->rsp = current_rsp;

    // find the next thread that is not sleeping
    thread_t* next_t = current_thread->next;
    thread_t* start_search = next_t;
    thread_t* selected_thread = NULL;

    do {
        // if thread is sleeping, check if it should wake up
        if (next_t->state == THREAD_SLEEPING) {
            if (timer_get_ticks() >= next_t->wake_up_tick) {
                next_t->state = THREAD_RUNNING;
            }
        }

        // if we found a running thread, break
        if (next_t->state == THREAD_RUNNING && next_t != idle_thread) {
            selected_thread = next_t;
            break;
        }

        next_t = next_t->next;
    } while (next_t != start_search);

    // If a normal thread is ready, run it. Otherwise, run the idle thread.
    if (selected_thread) {
        current_thread = selected_thread;
    } else {
        current_thread = idle_thread;
    }

    return current_thread->rsp;
}

void sched_yield(void) {
    // use dedicated soft interrupt for 250
    __asm__ volatile("int $250"); 
}