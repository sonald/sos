#ifndef _SOS_SPINLOCK_H
#define _SOS_SPINLOCK_H 

#include "types.h"

// uniprocess spinlock
class Spinlock {
    public:
        Spinlock(const char* name): _name(name) {}
        void lock();
        void release();
        bool locked() const;

    private:
        uint32_t _locked;
        const char* _name; // for debug
};

#endif
