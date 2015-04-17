#ifndef _BLKIO_H
#define _BLKIO_H 

#include <types.h>

#define NR_BUFFERS 2048
#define BYTES_PER_SECT 512

typedef uint32_t sector_t;

enum BufferFlags {
    BUF_FULL = 0x01,  //  loaded from disk
    BUF_DIRTY = 0x02, // need to write to disk
    BUF_BUSY  = 0x04  // hold by a task
};

struct Buffer {
    sector_t sector;
    dev_t dev;
    uint8_t flags;
    Buffer* next;
    Buffer* prev;
    Buffer* waitq;
    uint8_t data[BYTES_PER_SECT];
};

class BlockIOManager {
    public:
        void init();
        Buffer* read(dev_t dev, sector_t sect);
        bool write(Buffer* bufp);
        void release(Buffer* bufp);

    private:
        Buffer* _cache;
        Buffer* allocBuffer(dev_t dev, sector_t sect);
};

extern BlockIOManager bio;
#endif
