#include <stdlib.h>
#include <unistd.h>
#include <printf.h>
#include <string.h>

BEGIN_CDECL

#define PGSIZE 4096

// very first brk
static void* __data_end = NULL;
static void* __kheap_ptr = NULL;

#define KHEAD_SIZE  20  // ignore data field
#define ALIGN(sz, align) ((((u32)(sz)-1)/(align))*(align)+(align))

struct kheap_block_head
{
    u32 size;
    struct kheap_block_head* prev;
    struct kheap_block_head* next;
    void* ptr;
    u32 used;
    char data[1];
};

static void* ksbrk(size_t size)
{
    if (!__data_end) {
        __data_end = sbrk(0);
    }
    void* ptr = sbrk(size);
    if ((int)ptr == -1) {
        printf("ksbrk failed\n");
        return NULL;
    }

    kheap_block_head* newh = (kheap_block_head*)ptr;
    newh->used = 0;
    newh->next = newh->prev = NULL;
    newh->size = size - KHEAD_SIZE;
    newh->ptr = newh->data;

    return ptr;
}

static void split_block(kheap_block_head* h, size_t size)
{
    kheap_block_head *b = (kheap_block_head*)(h->data + size);
    b->used = 0;
    b->size = h->size - size - KHEAD_SIZE;
    b->ptr = b->data;

    b->next = h->next;
    h->next = b;
    b->prev = h;

    if (b->next) b->next->prev = b;
    h->size = size;
}


static kheap_block_head* align_block(kheap_block_head* h, int align)
{
    auto newh = (kheap_block_head*)(ALIGN(&h->data, align)-KHEAD_SIZE);
    int gap = (char*)newh - (char*)h;
    // kprintf("aligned %x to 0x%x, gap = %d\n", h, newh, gap);
    if (gap >= KHEAD_SIZE+4) {
        split_block(h, gap-KHEAD_SIZE);
        h = h->next;

    } else {
        //drop little memory, which cause frag that never can be regain
        if (h == __kheap_ptr) __kheap_ptr = newh;
        else if (h->prev) {
            auto prev = h->prev;
            prev->size += gap;
            prev->next = newh;
        }
        if (h->next) h->next->prev = newh;
        // h and newh may overlay, so copy aside
        kheap_block_head tmp = *h;
        *newh = tmp;
        newh->size -= gap;
        newh->ptr = newh->data;
    }
    return newh;
}

// merge h with next
static kheap_block_head* merge_block(kheap_block_head* h)
{
    if (h->next->used) return h;

    auto next = h->next;
    h->size += KHEAD_SIZE + next->size;
    h->next = next->next;
    if (h->next) h->next->prev = h;
    return h;
}

static kheap_block_head* find_block(kheap_block_head** last, size_t size, int align)
{
    kheap_block_head* p = (kheap_block_head*)__kheap_ptr;
    while (p && !(p->used == 0 && p->size >= size + (ALIGN(p->data, align) - (u32)p->data))) {
        *last = p;
        p = p->next;
    }

    return p;
}

static bool aligned(void* ptr, int align)
{
    return ((u32)ptr & (align-1)) == 0;
}

void* calloc(size_t count, size_t size)
{
    size_t sz = count * size;
    if (sz < 0) return NULL;
    void* ptr = malloc(sz);
    memset(ptr, 0, sz);
    return ptr;
}

void free(void *ptr)
{
    if (!ptr) return;
    kheap_block_head* h = (kheap_block_head*)((char*)ptr - KHEAD_SIZE);

    h->used = 0;
    if (h->prev && h->prev->used == 0) {
        h = merge_block(h->prev);
    }

    if (h->next) {
        h = merge_block(h);
    }
}

#define PGROUNDUP(sz)  ((((u32)sz)+PGSIZE-1) & ~(PGSIZE-1))

void* malloc(size_t size)
{
    if (!__kheap_ptr) __kheap_ptr = ksbrk(PGSIZE);

    size_t realsize = size; 
    int align = 4;

    kheap_block_head* last = NULL;
    kheap_block_head* h = find_block(&last, realsize, align);
    if (!h) {
        char* end = (char*)last->data + last->size;
        realsize += ALIGN(end + KHEAD_SIZE, align) - (u32)end;
        h = (kheap_block_head*)ksbrk(PGROUNDUP(realsize));
        if (!h) {
            return NULL;
        }

        last->next = h;
        h->prev = last;

        if (h->prev && h->prev->used == 0) {
            h = merge_block(h->prev);
        }
    }

    if (!aligned(h->data, align)) {
        h = align_block(h, align);
    }

    if (h->size - realsize >= KHEAD_SIZE + 4) {
        split_block(h, realsize);
    }

    h->used = 1;
    return h->data;
}

void* realloc(void *ptr, size_t size)
{
    if (!ptr) return malloc(size);
    kheap_block_head* h = (kheap_block_head*)((char*)ptr - KHEAD_SIZE);
    //sanity check
    if (h->used == 0 || h->ptr != h->data) {
        return NULL;
    }

    if (h->size >= size) return ptr;
    size = size ? size : 1;
    void* newptr = malloc(size);
    if (!newptr) return NULL;
    memcpy(newptr, ptr, h->size);
    free(ptr);
    return newptr;
}

END_CDECL
