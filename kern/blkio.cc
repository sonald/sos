#include "blkio.h"
#include "string.h"
#include "ata.h"
#include "isr.h"

BlockIOManager bio;
Buffer* waitq = nullptr;
PATADevice patas[NR_ATA];

void BlockIOManager::init()
{
    memset(_cache, 0, sizeof _cache);
    Buffer& p = _cache[0];
    p.next = &p;
    p.prev = &p;
    for (int i = 1; i  <NR_BUFFERS; i++) {
        auto& q = _cache[i];
        q.flags = BUF_EMPTY;
        q.next = p.next;
        p.next = &q;
        q.next->prev = &q;
        q.prev = &p;
    }

    kprintf("Probing IDE hard drives\n");
    patas[0].init(0, true);
    //patas[1].init(0, false);
    picenable(IRQ_ATA1);
    //picenable(IRQ_ATA2);
}

Buffer* BlockIOManager::allocBuffer(dev_t dev, sector_t sect)
{
recheck:
    for (int i = 0; i  <NR_BUFFERS; i++) {
        if (_cache[i].dev == dev && _cache[i].sector == sect) {
            if (!(_cache[i].flags & BUF_BUSY)) {
                _cache[i].flags |= BUF_BUSY;
                return &_cache[i];
            }

            //sleep(&_cache[i]);
            goto recheck;
        }
    }

    for (int i = 0; i  <NR_BUFFERS; i++) {
        Buffer* bp = &_cache[i];
        if ((bp->flags & BUF_BUSY) == 0 && (bp->flags & BUF_DIRTY) == 0) {
            bp->dev = dev;
            bp->sector = sect;
            bp->flags |= BUF_BUSY;
            return bp;
        }
    }

    panic("no buffer avail\n");
    return nullptr;
}

Buffer* BlockIOManager::read(dev_t dev, sector_t sect)
{
    Buffer* bp = allocBuffer(dev, sect);
    patas[dev-1].read(bp);
    return bp;
}

bool BlockIOManager::write(Buffer* bufp)
{
    return false;
}

