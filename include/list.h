#ifndef _SOS_LIST_H
#define _SOS_LIST_H 

#include "types.h"

struct list_head 
{
    struct list_head* prev;
    struct list_head* next;
};

#define LIST_INIT(lh) \
    struct list_head lh = { &(lh), &(lh) }

static inline bool list_empty(struct list_head* lh)
{
    return lh->next == lh;
}

static inline void list_add(struct list_head* lh, struct list_head* elem)
{
    lh->next->prev = elem;
    elem->prev = lh;
    elem->next = lh->next;
    lh->next = elem;
}

static inline void list_del(struct list_head* lh)
{
    if (!lh || list_empty(lh)) return;
    auto* next = lh->next;
    auto* prev = lh->prev;
    prev->next = next;
    next->prev = prev;
    lh->next = nullptr;
    lh->prev = nullptr;
}

#define list_entry(ptr, type, member) \
    (type*)((char*)(ptr)-(char*)&((type*)0)->member)

#endif
