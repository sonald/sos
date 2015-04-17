#include "x86.h"
#include "blkio.h"
#include "string.h"
#include "devices.h"
#include "task.h"
#include "sched.h"
#include "spinlock.h"

BlockIOManager bio;
Buffer* waitq = nullptr;
Spinlock biolock("bio");

void BlockIOManager::init()
{
    _cache = (Buffer*)vmm.kmalloc(sizeof(Buffer) * NR_BUFFERS, 1);
    memset(_cache, 0, sizeof(Buffer) * NR_BUFFERS);
    Buffer& p = _cache[0];
    p.next = &p;
    p.prev = &p;
    for (int i = 1; i  <NR_BUFFERS; i++) {
        auto& q = _cache[i];
        q.flags = 0;
        q.next = p.next;
        p.next = &q;
        q.next->prev = &q;
        q.prev = &p;
    }
}

Buffer* BlockIOManager::allocBuffer(dev_t dev, sector_t sect)
{
    auto oldflags = biolock.lock();
recheck:
    for (int i = 0; i < NR_BUFFERS; i++) {
        if (_cache[i].dev == dev && _cache[i].sector == sect) {
            if (!(_cache[i].flags & BUF_BUSY)) {
                _cache[i].flags |= BUF_BUSY;
                //kprintf(" (BIO:reget) ");
                biolock.release(oldflags);
                if (oldflags & FL_IF) sti();
                return &_cache[i];
            }

            sleep(&biolock, &_cache[i]);
            goto recheck;
        }
    }

    for (int i = 0; i < NR_BUFFERS; i++) {
        Buffer* bp = &_cache[i];
        if ((bp->flags & BUF_BUSY) == 0 && (bp->flags & BUF_DIRTY) == 0) {
            bp->dev = dev;
            bp->sector = sect;
            bp->flags = BUF_BUSY | ~BUF_FULL;
            biolock.release(oldflags);
            return bp;
        }
    }

    panic("no buffer avail\n");
    return nullptr;
}

Buffer* BlockIOManager::read(dev_t dev, sector_t sect)
{
    Buffer* bp = allocBuffer(dev, sect);
    if (bp->flags & BUF_FULL) {
        return bp;
    }
    auto* device = blk_device_get(dev);
    kassert(device != nullptr);
    device->read(bp);
    bp->flags |= BUF_FULL;
    return bp;
}

bool BlockIOManager::write(Buffer* bufp)
{
    (void)bufp;
    return false;
}

void BlockIOManager::release(Buffer* bufp)
{
    auto oldflags = biolock.lock();
    //kprintf("[BIO: %s release] ", current->name);
    kassert(bufp && (bufp->flags & BUF_BUSY));
    bufp->flags &= ~BUF_BUSY;
    wakeup(bufp);
    biolock.release(oldflags);
}

