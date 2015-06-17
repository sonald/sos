#ifndef _SOS_TTY_H
#define _SOS_TTY_H

#include <types.h>
#include <devices.h>
#include <termios.h>
#include <ringbuf.h>

using TtyBuffer = RingBuffer<char, 1024>;

#define INTR_CHAR(tty) ((tty)->termios.c_cc[VINTR])
#define QUIT_CHAR(tty) ((tty)->termios.c_cc[VQUIT])
#define ERASE_CHAR(tty) ((tty)->termios.c_cc[VERASE])
#define KILL_CHAR(tty) ((tty)->termios.c_cc[VKILL])
#define WERASE_CHAR(tty) ((tty)->termios.c_cc[VWERASE])
#define EOF_CHAR(tty) ((tty)->termios.c_cc[VEOF])
#define START_CHAR(tty) ((tty)->termios.c_cc[VSTART])
#define STOP_CHAR(tty) ((tty)->termios.c_cc[VSTOP])
#define SUSPEND_CHAR(tty) ((tty)->termios.c_cc[VSUSP])

class TtyDevice: public CharDevice
{
public:
    char read() override;
    bool write(char ch) override;

    struct termios termios;
    bool has_full_line;
    pid_t pgrp; // foreground process group
    TtyBuffer readq;
    TtyBuffer writeq;
    TtyBuffer secondaryq;
};

extern void tty_init();
extern void tty_thread();

#endif // _SOS_TTY_H
