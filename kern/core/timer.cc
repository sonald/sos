#include "timer.h"
#include "x86.h"
#include "isr.h"
#include "task.h"
#include "spinlock.h"
#include "display.h"

static const u32 FREQ = 1193180;
volatile u32 timer_ticks = 0;

extern tss_entry_t shared_tss;
extern "C" void switch_to(kcontext_t** old, kcontext_t* next);
extern void setup_tss(u32 stack);
extern "C" void flush_tss();

Spinlock schedlock("sched");

void scheduler()
{    
    //NOTE: should never spin in interrupt context! so check IF flag 
    //to determine if in irq context. this is not a good way.
    auto oldflags = schedlock.lock();
    if (current) {
        if (current->need_resched) current->need_resched = false;
        
        auto* old = current;
        bool rewind = true;
        proc_t* tsk = old->next ? old->next: task_init;
        while (tsk) {
            if (old != tsk && tsk->state == TASK_READY) {
                current = tsk;
                break;
            }
            tsk = tsk->next;
            if (!tsk && rewind) {
                rewind = false;
                tsk = &tasks[0];
            }
        }

        kassert(current->state == TASK_READY);
        kassert( old != current );

        // kprintf("(sched: %s(%d) -> %s(%d)) ", old->name, old->pid,
        //         current->name, current->pid);
        //very tricky!
        setup_tss(current->kern_esp);
        flush_tss();
        vmm.switch_page_directory(current->pgdir);
        
        switch_to(&old->kctx, current->kctx);
    }

    schedlock.release(oldflags);
}

struct timeout_s {
    uint32_t end_tick;
    struct timeout_s* next;
};

static timeout_t* tm_head = NULL;

timeout_t* add_timeout(int millisecs)
{
    timeout_t* tm = new timeout_t;
    tm->end_tick = timer_ticks + millisecs * HZ / 1000;
    tm->next = tm_head;
    tm_head = tm;
    return tm;
}

static void timer_interrupt(trapframe_t* regs)
{
    (void)regs;
    timer_ticks++;
    auto old = current_display->get_text_color();
    current_display->set_text_color(RED);
    auto cur = current_display->get_cursor();
    current_display->set_cursor({70, 0});

    timeout_t** tm = &tm_head;
    while (*tm) {
        if ((*tm)->end_tick <= timer_ticks) {
            wakeup((void*)tm);
            auto* p = *tm;
            wakeup(p);
            *tm = (*tm)->next;
            delete p;
        } else tm = &(*tm)->next;
    }
    kprintf("T: %d", timer_ticks/HZ);

    current_display->set_cursor(cur);
    current_display->set_text_color(old);
}

void init_timer()
{
    register_isr_handler(IRQ0, timer_interrupt);

    u32 div = FREQ / HZ;
    outb(0x43, 0x36);

    /*Divisor has to be sent byte-wise, so split here into upper/lower bytes.*/
    u8 l = (u8)(div & 0xff);
    u8 h = (u8)((div>>8) & 0xff);

    // Send the frequency divisor.
    outb(0x40, l);
    outb(0x40, h);
}

void busy_wait(int millisecs)
{
    u32 end = timer_ticks + millisecs * HZ / 1000;
    while (timer_ticks < end) {
        asm volatile ("hlt");
        asm volatile ("nop");
    }
       
}
