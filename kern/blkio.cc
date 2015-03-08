#include "blkio.h"
#include "string.h"
#include "devices.h"
#include "task.h"
#include "sched.h"

BlockIOManager bio;
Buffer* waitq = nullptr;

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
            current_proc->channel = &_cache[i];
            current_proc->state = TASK_SLEEP;
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
    auto* device = blk_device_get(dev);
    kassert(device != nullptr);
    device->read(bp);
    return bp;
}

bool BlockIOManager::write(Buffer* bufp)
{
    return false;
}

