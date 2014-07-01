#include "timer.h"
#include "isr.h"
#include "task.h"

static const u32 FREQ = 1193180;
volatile u32 timer_ticks = 0;

extern tss_entry_t shared_tss;
extern "C" void sched(u32 new_context);

void scheduler(registers_t* regs)
{
    if (current_proc) {
        if (timer_ticks % 100 != 0) {
            return;
        }

        current_proc->regs = regs;
        //kprintf(" save %s at 0x%x;  ", current_proc->name, current_proc->regs);
        if (current_proc->next)
            current_proc = current_proc->next;
        else current_proc = &proctable[0];

        //very tricky!
        VirtualMemoryManager* vmm = VirtualMemoryManager::get();
        vmm->switch_page_directory(current_proc->pgdir);
        
        shared_tss.esp0 = current_proc->regs->esp;
        //kprintf(" load %s at 0x%x; ", current_proc->name, current_proc->regs);
        sched(A2I(current_proc->regs));
    }
}

static void timer_interrupt(registers_t* regs)
{
    (void)regs;
    u16 cur = get_cursor();
    set_cursor(CURSOR(70, 24));
    kprintf("T: %d", timer_ticks++);
    set_cursor(cur);

    // can not called here, cause EOI is not send to PIC
    //scheduler(regs);
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
    u32 end = timer_ticks + millisecs / HZ;
    while (timer_ticks < end) ;
}
