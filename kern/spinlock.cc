#include "spinlock.h"
#include "x86.h"
#include "task.h"

//#define DEBUG_SPINLOCK

uint32_t Spinlock::lock()
{
    auto oldflags = readflags();
    cli();
#if CONFIG_SMP
    while (bts(&_locked) == 1) {
        while (_locked) {
            asm volatile ("nop; pause");
        }
    }
#else 
    bts(&_locked);
#endif

#ifdef DEBUG_SPINLOCK
    auto old = get_text_color();
    set_text_color(COLOR(BROWN, WHITE));
    kprintf("[%s:L %s (IF %d)] ", _name, current?current->name:"",
            (oldflags&FL_IF)?1:0);
    set_text_color(old);
#endif

    return oldflags;
}

void Spinlock::release(uint32_t oldflags)
{
#ifdef DEBUG_SPINLOCK
    auto old = get_text_color();
    set_text_color(COLOR(BROWN, WHITE));
    kprintf("[%s:R %s (IF %d)] ", _name, current?current->name:"",
            (oldflags&FL_IF)?1:0);
    set_text_color(old);
#endif
    asm volatile ("lock btr $0, %0":"+m"(_locked));
    if (oldflags) writeflags(oldflags);
}

bool Spinlock::locked() const
{
    return _locked > 0;
}

