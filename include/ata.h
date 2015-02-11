#ifndef _ATA_H
#define _ATA_H 

#include "types.h"

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

class PATADevice {
    public:
        PATADevice();
        
        bool valid() const { return _valid; }
        void init(int bus, bool master);

    private:
        bool _valid {false};
        dev_t _dev;
        int _bus;
        int _master;
        int _iobase;
        int _ioctl;
};

void pata_probe();

#endif /* ATA_H */

