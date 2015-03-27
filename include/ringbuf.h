#ifndef _SOS_LIB_RINGBUF_H
#define _SOS_LIB_RINGBUF_H 

#include "types.h"
#include "common.h"
#include "spinlock.h"

template <typename T, size_t N = 4>
class RingBuffer
{
    public:
        T read();
        void write(T& v);
        bool full() const { return _used == _sz; }
        bool empty() const { return _used == 0; }

    private:
        T _buf[N];
        size_t _h {0}, _t {0};
        size_t _used {0}, _sz {N};
        Spinlock _lock {"ringbuf"};
};

template <typename T, size_t N>
T RingBuffer<T, N>::read()
{
    if (empty()) {
        kassert(_h == _t);
        return T();
    }

    auto eflags = _lock.lock();
    T& v = _buf[_t];
    _t = (_t + 1) % N;
    _used--;
    _lock.release(eflags);
    return v;
}

template <typename T, size_t N>
void RingBuffer<T, N>::write(T& v)
{
    if (full()) return; // drop ? or drop oldest ?
    auto eflags = _lock.lock();
    _buf[_h] = v;
    _h = (_h + 1) % N;
    _used++;
    _lock.release(eflags);
}

#endif

