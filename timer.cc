#include "timer.h"
#include "isr.h"
#include "task.h"

static const u32 FREQ = 1193180;
static volatile u32 timer_ticks = 0;

static void timer_interrupt(registers_t* regs)
{
    (void)regs;
    u16 cur = get_cursor();
    set_cursor(CURSOR(70, 24));
    kprintf("T: %d", timer_ticks++);
    set_cursor(cur);
    
    if (current_proc) {
        if (timer_ticks % 100 != 0) {
            return;
        }

        current_proc->regs = *regs;
        if (current_proc->next)
            current_proc = current_proc->next;
        else current_proc = &proctable[0];

        //very tricky!
        *regs = current_proc->regs;
        VirtualMemoryManager* vmm = VirtualMemoryManager::get();
        vmm->switch_page_directory(current_proc->pgdir);
        //kprintf(" (-> %s: %d) ", current_proc->name, current_proc->pid);
    }
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
