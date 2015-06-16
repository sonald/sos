#ifndef _SOS_TTY_H
#define _SOS_TTY_H

#include <types.h>
#include <devices.h>
#include <termios.h>
#include <ringbuf.h>

class TtyDevice: public CharDevice
{
    char read() override;
    bool write(char ch) override;
};

extern void tty_init();

#endif // _SOS_TTY_H
