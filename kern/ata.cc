#include "ata.h"
#include "x86.h"
#include "common.h"
#include "isr.h"
#include "string.h"

#define NR_ATA 4
/* Primary Master Controller Registers
 * 1F0 (Read and Write): Data Register
 * 1F1 (Read): Error Register
 * 1F1 (Write): Features Register
 * 1F2 (Read and Write): Sector Count Register
 * 1F3 (Read and Write): LBA Low Register
 * 1F4 (Read and Write): LBA Mid Register
 * 1F5 (Read and Write): LBA High Register
 * 1F6 (Read and Write): Drive/Head Register
 * 1F7 (Read): Status Register
 * 1F7 (Write): Command Register
 * 3F6 (Read): Alternate Status Register
 * 3F6 (Write): Device Control Register
 */

#define ATA_STATUS_REG 0x7
#define ATA_DRIVE_REG 0x6
#define ATA_DATA_REG 0x0
#define ATA_CMD_REG 0x7

#define ATA_CMD_IDENTIFY 0xEC

PATADevice patas[NR_ATA];

static void ata_wait(int iobase)
{
    while ((inb(ATA_STATUS_REG+iobase) & (ATA_BSY|ATA_DRDY)) != ATA_DRDY)
        ;
}

PATADevice::PATADevice()
{
}

void PATADevice::init(int bus, bool master)
{
    this->_bus = bus;
    this->_master = master;
    this->_iobase = bus == 0 ? 0x1f0 : 0x170;
    this->_ioctl = bus == 0 ? 0x3f6 : 0x376;

    //ata_wait();
    outb(_iobase+ATA_DRIVE_REG, (master?0:1)<<4);
    outb(_iobase+ATA_CMD_REG, ATA_CMD_IDENTIFY);

    for (int i = 0; i < 1000; i++) {
        if (inb(_iobase+ATA_STATUS_REG) != 0) {
            this->_valid = true;
            break;
        }
    }

    u8 r = 0;
    while (!((r = inb(_iobase+ATA_STATUS_REG)) & (ATA_DRQ|ATA_ERR)))
        ;

    uint32_t sectors = 0;
    char modal[41];
    if (r & ATA_ERR) { this->_valid = false; }
    else {
        uint16_t data[256];
        for (size_t i = 0; i < ARRAYLEN(data); i++) {
            data[i] = inw(_iobase+ATA_DATA_REG);
        }

        memcpy(modal, &data[27], 40);
        modal[40] = '\0';
        sectors = *(uint32_t*)&data[60];

    }
    kprintf("IDE %d %s %sdetected, sectors %d, modal (%s)", bus, master?"master":"slave",
            _valid?"":"not ", sectors, modal);
}

void pata_probe()
{
    kprintf("Probing IDE hard drives\n");
    patas[0].init(0, true);
    //patas[1].init(0, false);
    picenable(IRQ_ATA1);
    //picenable(IRQ_ATA2);
}

