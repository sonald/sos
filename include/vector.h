#ifndef _SOS_VECTOR_H
#define _SOS_VECTOR_H 

#ifdef _SOS_KERNEL_
#include <common.h>
#endif

#include <string.h>

template <class T>
class Vector 
{
    public:
        Vector() {
            _cap = 16;
            _sz = 0;
            _buf = new T[_cap];
        }

        ~Vector() {
            delete _buf;
        }

        void push_back(const T& t) {
            if (_sz + 1 == _cap) {
                realloc(_cap*2);
            }

            _buf[_sz++] = t;
        }

        T& operator[](int pos) {
            return _buf[pos];
        }

        const T& operator[](int pos) const {
            return _buf[pos];
        }

        int size() const { return _sz; }

        T remove(int pos) {
            T t;
            if (pos >= 0 && pos < _sz) {
                t = _buf[pos];
                memmove(_buf+pos, _buf+pos+1, sizeof(T)*(_sz-pos-1));
                _sz--;
            }
            return t;
        }

        void insert(int pos, const T& t) {
            if (_sz + 1 >= _cap) realloc(_cap*2);
            if (pos >= 0 && pos < _sz) {
                memmove(_buf+pos+1, _buf+pos, sizeof(T)*(_sz-pos));
                _sz++;
                _buf[pos] = t;
            }
        }

        void pop_back() {
            if (_sz > 0) --_sz;
        }

        void clear() {
            _sz = 0;
        }

    private:
        int _cap; 
        int _sz;

        T* _buf;

        void realloc(int sz) {
            auto* newp = new T[sz];
            memcpy(newp, _buf, sizeof(T)*_cap);
            delete _buf;
            _buf = newp;
            _cap = sz;
        }
};

#endif

