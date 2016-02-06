#ifndef _SOS_MAP_H
#define _SOS_MAP_H

#include <types.h>
#include <string.h>

using hash_t = uint32_t;
constexpr int DEFAULT_TABLE_SIZE = 127; // prime

template <class K>
struct KeyHash {
    hash_t operator()(const K& key) const {
        return static_cast<hash_t>(key) % DEFAULT_TABLE_SIZE;
    }
};

template <class K>
struct KeyEqual {
    bool operator()(const K& k1, const K& k2) const {
        return k1 == k2;
    }
};

//BKDR hash
struct CStringHash {
    hash_t operator()(const char* key) const {
        unsigned int seed = 131; // 31 131 13131 1313131 ...
        hash_t h = 0;
        while (*key) {
            h = h * seed + *key++;
        }
        return (h & 0x7fffffff);
    }
};

struct CStringEqual {
    bool operator()(const char* s1, const char* s2) const {
        return strcmp(s1, s2) == 0;
    }
};

template <class K, class V, class F = KeyHash<K>, class Eq = KeyEqual<K>>
class HashMap
{
    public:
        HashMap() {
            memset(_buckets, 0, sizeof _buckets);
        }

        ~HashMap() {
            for (int i = 0; i < DEFAULT_TABLE_SIZE; i++) {
                node_t* p = _buckets[i];
                while (p) {
                    auto* d = p;
                    p = p->next;
                    delete d;
                }
            }
            _sz = 0;
        }

        int size() const { return _sz; }
        bool contains(const K& key) const {
            auto h = _hfn(key) % DEFAULT_TABLE_SIZE;
            auto* p = _buckets[h];
            while (p) {
                if (_heq(p->k, key)) return true;
                p = p->next;
            }
            return false;
        }

        V find(const K& key) const {
            auto h = _hfn(key) % DEFAULT_TABLE_SIZE;
            auto* p = _buckets[h];
            while (p) {
                if (_heq(p->k, key)) return p->v;
                p = p->next;
            }

            return V();
        }

        V erase(const K& key) {
            auto h = _hfn(key) % DEFAULT_TABLE_SIZE;
            auto** p = &_buckets[h];

            V ret;
            while (*p) {
                if (_heq((*p)->k, key)) {
                    auto* d = *p;
                    ret = d->v;
                    *p = d->next;
                    delete d;
                    --_sz;
                } else
                    p = &(*p)->next;
            }

            return ret;
        }

        void insert(const K& key, const V& val) {
            auto h = _hfn(key) % DEFAULT_TABLE_SIZE;
            auto* q = new node_t;
            q->k = key;
            q->v = val;
            q->next = _buckets[h];
            _buckets[h] = q;
            _sz++;
        }

    private:
        typedef struct node_ {
            K k;
            V v;
            struct node_* next;
        } node_t;

        int _sz {0};
        node_t* _buckets[DEFAULT_TABLE_SIZE];
        F _hfn;
        Eq _heq;
};

#endif
