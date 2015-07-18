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
        void write(const T& v);

        bool full() const { return _used == _sz; }
        bool empty() const { return _used == 0; }
        int sz() const { return _sz; }
        int remain() const { return _sz - _used; }

        void clear();
        T peek();
        T last();
        T drop(); // remove last input if any

    private:
        T _buf[N];
        size_t _h {0}, _t {0};
        size_t _used {0}, _sz {N};
        Spinlock _lock {"ringbuf"};
};

template <typename T, size_t N>
T RingBuffer<T, N>::read()
{
    auto eflags = _lock.lock();
    if (empty()) return T();

    T& v = _buf[_t];
    _t = (_t + 1) % N;
    _used--;
    _lock.release(eflags);
    return v;
}

template <typename T, size_t N>
T RingBuffer<T, N>::last()
{
    auto eflags = _lock.lock();
    if (empty()) return T();

    T& v = _buf[_h];
    _lock.release(eflags);
    return v;
}

template <typename T, size_t N>
T RingBuffer<T, N>::peek()
{
    auto eflags = _lock.lock();
    if (empty()) return T();
    T& v = _buf[_t];
    _lock.release(eflags);
    return v;
}

template <typename T, size_t N>
void RingBuffer<T, N>::write(const T& v)
{
    auto eflags = _lock.lock();
    if (full()) { read(); } // eat oldest

    _buf[_h] = v;
    _h = (_h + 1) % N;
    _used++;
    _lock.release(eflags);
}

template <typename T, size_t N>
T RingBuffer<T, N>::drop()
{
    auto eflags = _lock.lock();
    if (empty()) return T();

    _h--;
    if (_h < 0) _h = N-1;
    T& v = _buf[_h];
    _used--;
    _lock.release(eflags);
    return v;
}

template <typename T, size_t N>
void RingBuffer<T, N>::clear()
{
    auto eflags = _lock.lock();
    _h = _t = 0;
    _sz = N;
    _used = 0;
    _lock.release(eflags);
}
#endif

