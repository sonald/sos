/**
 * c++ runtime
 * http://wiki.osdev.org/C%2B%2B
 */

#include "common.h"
#include "vm.h"

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

// not thread safe
// should add a mutex-like guard with a test-and-set primitive.
namespace __cxxabiv1 
{
    /* guard variables */

    /* The ABI requires a 64-bit type.  */
    __extension__ typedef int __guard __attribute__((mode(__DI__)));

    extern "C" int __cxa_guard_acquire (__guard *g) 
    {
        return !*(char *)(g);
    }

    extern "C" void __cxa_guard_release (__guard *g)
    {
        *(char *)g = 1;
    }

    extern "C" void __cxa_guard_abort (__guard *)
    {

    }
}


void operator delete(void *ptr)
{
    vmm.kfree(ptr);
}

void* operator new(size_t len)
{
    return vmm.kmalloc(len, 1);
}

void operator delete[](void *ptr)
{
    ::operator delete(ptr);
}

void* operator new[](size_t len)
{
    return ::operator new(len);
}

