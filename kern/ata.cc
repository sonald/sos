#include "ata.h"
#include "x86.h"
#include "common.h"
#include "isr.h"
#include "string.h"

static void ata_wait(int iobase)
{
    while ((inb(ATA_STATUS_REG+iobase) & (ATA_BSY|ATA_DRDY)) != ATA_DRDY)
        ;
}

static inline bool ata_ready(int iobase)
{
    uint8_t r = 0;
    while (!((r = inb(iobase+ATA_STATUS_REG)) & (ATA_DRQ|ATA_ERR)))
        ;

    return !(r & ATA_ERR);
}

PATADevice::PATADevice()
{
}

void PATADevice::_outb(u16 port, u8 val)
{
    outb(port+_iobase, val);
}

u8 PATADevice::_inb(u16 port)
{
    return inb(port+_iobase);
}

u16 PATADevice::_inw(u16 port)
{
    return inw(port+_iobase);
}

void PATADevice::init(int bus, bool master)
{
    this->_bus = bus;
    this->_master = master;
    this->_iobase = bus == 0 ? 0x1f0 : 0x170;
    this->_ioctl = bus == 0 ? 0x3f6 : 0x376;

    //ata_wait();
    _outb(ATA_DRIVE_REG, (master?0:1)<<4);
    _outb(ATA_CMD_REG, ATA_CMD_IDENTIFY);

    for (int i = 0; i < 1000; i++) {
        if (_inb(ATA_STATUS_REG) != 0) {
            this->_valid = true;
            break;
        }
    }

    char model[41];
    char serial[21];

    if (!ata_ready(_iobase)) { this->_valid = false; }
    else {
        uint16_t data[256];
        for (size_t i = 0; i < ARRAYLEN(data); i++) {
            data[i] = _inw(ATA_DATA_REG);
        }

        auto fn = [](uint16_t* src, char* dst, size_t sz) {
            for (size_t i = 0; i < sz; i++, src++) {
                dst[i*2] = *src >> 8;
                dst[i*2+1] = *src & 0xff;
            }
            
            for (size_t i = sz*2; i > 0; i--) {
                if (dst[i] <= 0x20) 
                    dst[i] = 0;
                else break;
            }
        };

        fn(&data[10], (char*)&serial, 10);
        fn(&data[27], (char*)&model, 20);
        _sectors = data[60] | (data[61] << 16);
    }
    kprintf("IDE %d %s %sdetected, sectors %d, model (%s) serial (%s)", 
            bus, master?"master":"slave",
            _valid?"":"not ", _sectors, model, serial);
}

bool PATADevice::read(Buffer* bufp)
{
    sector_t sect = bufp->sector;
    _outb(ATA_SECTOR_COUNT_REG, 1);
    _outb(ATA_LBA_LO_REG, sect & 0xff);
    _outb(ATA_LBA_MI_REG, (sect >> 8) & 0xff);
    _outb(ATA_LBA_HI_REG, (sect >> 16) & 0xff);
    _outb(ATA_DRIVE_REG, 0xE0 | ((_master?0:1)<<4) | ((sect>>24) & 0x0f));

    _outb(ATA_CMD_REG, ATA_CMD_READ);
    //TODO: wait for interrupt, so sleep .... ,now just polling
    
    if (!ata_ready(_iobase)) return false;
    for (size_t i = 0; i < 256; i++) {
        uint16_t d = _inw(ATA_DATA_REG);
        bufp->data[i*2] = d;
        bufp->data[i*2+1] = d >> 8;
    }
    return true;
}

bool PATADevice::write(Buffer* bufp)
{
}

void ata_init()
{
    kprintf("Probing IDE hard drives\n");
    auto* pata0 = new PATADevice;
    pata0->init(0, true);
    blk_device_register(DEVNO(IDE_MAJOR, 0), pata0);

    //patas[1].init(0, false);
    picenable(IRQ_ATA1);
    //picenable(IRQ_ATA2);
    //blk_device_register(DEVNO(IDE_MAJOR, 64), pata1);
}

