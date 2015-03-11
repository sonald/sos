#include "timer.h"
#include "x86.h"
#include "isr.h"
#include "task.h"
#include "spinlock.h"

static const u32 FREQ = 1193180;
volatile u32 timer_ticks = 0;

extern tss_entry_t shared_tss;
extern "C" void switch_to(kcontext_t** old, kcontext_t* next);
extern void setup_tss(u32 stack);
extern "C" void flush_tss();

Spinlock schedlock("sched");

void scheduler(trapframe_t* regs)
{
    //NOTE: should never spin in interrupt context! so check IF flag 
    //to determine if in irq context. this is not a good way.
    auto flags = readflags();
    if ((flags & FL_IF)) {
        schedlock.lock();
    }
    if (current) {
        if (current->need_resched) current->need_resched = false;
        current->regs = regs;
        
        auto* old = current;
        bool rewind = old->next != NULL;
        proc_t* tsk = old->next ? old->next: &tasks[0];
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

        if (old == current) return;
        //kprintf("(sched: %s -> %s) ", old->name, current->name);
        //very tricky!
        setup_tss(current->kern_esp);
        flush_tss();
        vmm.switch_page_directory(current->pgdir);
        
        switch_to(&old->kctx, current->kctx);
    }
    if (schedlock.locked())
        schedlock.release();
}

static void timer_interrupt(trapframe_t* regs)
{
    (void)regs;
    timer_ticks++;
    auto old = get_text_color();
    set_text_color(COLOR(LIGHT_CYAN, WHITE));
    u16 cur = get_cursor();
    set_cursor(CURSOR(70, 0));
    kprintf("T: %d", timer_ticks/HZ);
    set_cursor(cur);
    set_text_color(old);
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
