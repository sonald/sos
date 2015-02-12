#ifndef _ATA_H
#define _ATA_H 

#include "types.h"
#include "blkio.h"

#define NR_ATA 4

/*
 * BSY (busy)
 * DRDY (device ready)
 * DF (Device Fault)
 * DSC (seek complete)
 * DRQ (Data Transfer Requested)
 * CORR (data corrected)
 * IDX (index mark)
 * ERR (error)
 */
enum ATA_STATUS {
    ATA_BSY = 0x80,
    ATA_DRDY = 0x40,
    ATA_DF = 0x20,
    ATA_DSC = 0x10,
    ATA_DRQ = 0x08,
    ATA_CORR = 0x04,
    ATA_IDX = 0x02,
    ATA_ERR = 0x01
};

/* 
 * BBK (Bad Block)
 * UNC (Uncorrectable data error)
 * MC (Media Changed)
 * IDNF (ID mark Not Found)
 * MCR (Media Change Requested)
 * ABRT (command aborted)
 * TK0NF (Track 0 Not Found)
 * AMNF (Address Mark Not Found)
 */

enum ATA_ERROR {
    ATA_ERR_BBK = 0x80,
    ATA_ERR_UNC = 0x40,
    ATA_ERR_MC = 0x20,
    ATA_ERR_IDNF = 0x10,
    ATA_ERR_MCR = 0x08,
    ATA_ERR_ABRT = 0x04,
    ATA_ERR_TK0NF = 0x02,
    ATA_ERR_AMNF = 0x01,
};

enum ATAFlags {
    LBA28 = 0x01,
    LBA48 = 0x02,
};

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

#define ATA_DATA_REG 0x0
#define ATA_SECTOR_COUNT_REG 0x2
#define ATA_LBA_LO_REG 0x3
#define ATA_LBA_MI_REG 0x4
#define ATA_LBA_HI_REG 0x5
#define ATA_DRIVE_REG 0x6
#define ATA_STATUS_REG 0x7
#define ATA_CMD_REG 0x7

#define ATA_CMD_IDENTIFY 0xEC
#define ATA_CMD_READ 0x20
#define ATA_CMD_WRITE 0x30


class PATADevice {
    public:
        PATADevice();
        
        bool valid() const { return _valid; }
        void init(int bus, bool master);

        bool read(Buffer* bufp);        
        bool write(Buffer* bufp);

    private:
        bool _valid {false};
        int _bus; // 0 or 1
        int _master; // or slave
        int _iobase;
        int _ioctl; // device control reg
        ATAFlags _flags;

        uint32_t _sectors; // lba28 sectors total

        
        void _outb(u16 port, u8 val);
        u8 _inb(u16 port);
        u16 _inw(u16 port);
};

#endif /* ATA_H */

