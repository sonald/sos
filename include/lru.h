#ifndef _SOS_LRU_H
#define _SOS_LRU_H 

#include <map.h>

// generic LRU cache 

template <class K, class V, class F = KeyHash<K>, class Eq = KeyEqual<K>>
class LRUCache {
    public:
        LRUCache(int cap = 16): _cap{cap} {
            _l.prev = &_l;
            _l.next = &_l;
        }

        ~LRUCache() {
            Node* p = _l.next;
            while (p != &_l) {
                auto* d = p;
                p = p->next;
                delete d;
            }
        }

        V first() {
            if (_h.size() > 0) return _l.next->v;
            return V();
        }

        size_t size() const { return _h.size(); }

        bool has(const K& k) const { return _h.contains(k); }

        V get(const K& k) {
            if (!_h.contains(k)) return V();

            promote(k);
            return _h.find(k)->v;
        }

        void set(const K& k, const V& v) {
            Node* n = NULL;
            if (_h.contains(k)) {
                promote(k);
                Node* p = _h.find(k);
                p->v = v;
                return;
            } 
            
            if (_h.size() == _cap) {
                Node* last = _l.prev;
                _h.erase(last->k);
                last->next->prev = last->prev;
                last->prev->next = last->next;
                n = last;
            } else n = new Node;

            n->k = k;
            n->v = v;

            _h.insert(k, n);
            n->next = _l.next;
            n->prev = &_l;
            _l.next->prev = n;
            _l.next = n;
        }

    private:
        struct Node {
            K k;
            V v;
            struct Node* next, *prev;
        };

        int _cap; // capacity
        HashMap<K, Node*, F, Eq> _h;
        Node _l {K(), V(), NULL, NULL};

        void promote(const K& k) {
            Node* p = _h.find(k);
            p->prev->next = p->next;
            p->next->prev = p->prev;

            p->next = _l.next;
            p->prev = &_l;
            _l.next->prev = p;
            _l.next = p;
        }
};

#endif
