#include "timer.h"
#include "isr.h"

static const u32 FREQ = 1193180;
static volatile u32 timer_ticks = 0;

static void timer_interrupt(registers_t* regs)
{
    (void)regs;
    u16 cur = get_cursor();
    set_cursor(CURSOR(70, 24));
    kprintf("T: %d", timer_ticks++);
    set_cursor(cur);
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
