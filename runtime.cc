/**
 * c++ runtime
 * http://wiki.osdev.org/C%2B%2B
 */

#include "common.h"

#define MAX_ATEXIT_ENTRIES 128

extern "C" void __cxa_pure_virtual()
{
    // do nothing
}

struct cxa_ref_s
{
    void (*f)(void*);
    void* arg;
    void* dso;
};

cxa_ref_s refs[MAX_ATEXIT_ENTRIES];
int refs_len = 0;

/**
 * dummy, no need to do anything
 */
extern "C" int __cxa_atexit(void (*func) (void *), void * arg, void * dso_handle)
{
    if (refs_len >= MAX_ATEXIT_ENTRIES) return -1;
    refs[refs_len].f = func;
    refs[refs_len].arg = arg;
    refs[refs_len++].dso = dso_handle;

    return 0;
}

extern "C" void __cxa_finalize(void * d)
{
    kprintf("__cxa_finalize: 0x%x\n", (u32)d);

    if (!d) {
        for (int i = refs_len-1; i >= 0; --i) {
            if (refs[i].f)
                refs[i].f(refs[i].arg);
            kprintf("destructing %d ", i);
        }
        return;
    }

    // else 
    for (int i = refs_len-1; i >= 0; --i) {
        if (refs[i].f == d) {
            refs[i].f(refs[i].arg);
            refs[i].f = NULL;
            kprintf("destructing %d ", i);
        }
    }
}


