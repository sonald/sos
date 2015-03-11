#include "spinlock.h"
#include "x86.h"
#include "task.h"

void Spinlock::lock()
{
    while (bts(&_locked) == 1) {
        while (_locked) {
            asm volatile ("nop; pause");
        }
    }

#ifdef DEBUG_SPINLOCK
    auto old = get_text_color();
    set_text_color(COLOR(BROWN, WHITE));
    kprintf("[%s:L %s] ", _name, current->name);
    set_text_color(old);
#endif
}

void Spinlock::release()
{
    asm volatile ("lock btr $0, %0":"+m"(_locked));
#ifdef DEBUG_SPINLOCK
    auto old = get_text_color();
    set_text_color(COLOR(BROWN, WHITE));
    kprintf("[%s:R %s] ", _name, current->name);
    set_text_color(old);
#endif
}

bool Spinlock::locked() const
{
    return _locked > 0;
}

