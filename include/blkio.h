#ifndef _BLKIO_H
#define _BLKIO_H 

#include "common.h"

#define NR_BUFFERS 512
#define BYTES_PER_SECT 512

typedef uint32_t sector_t;

enum BufferFlags {
    BUF_EMPTY = 0x01,
    BUF_DIRTY = 0x02,
    BUF_BUSY  = 0x04
};

struct Buffer {
    sector_t sector;
    dev_t dev;
    uint16_t flags;
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

    private:
        Buffer _cache[NR_BUFFERS];
        Buffer* allocBuffer(dev_t dev, sector_t sect);
};

extern BlockIOManager bio;
#endif
