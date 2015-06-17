#include "tty.h"
#include <devices.h>
#include <kb.h>
#include <task.h>
#include <spinlock.h>
#include <display.h>
#include <ctype.h>

Spinlock ttylock {"tty"};
static TtyDevice* tty1 = nullptr;

/*	intr=^C		quit=^|		erase=del	kill=^U
	eof=^D		vtime=\0	vmin=\1		sxtc=\0
	start=^Q	stop=^S		susp=^Z		eol=\0
	reprint=^R	discard=^U	werase=^W	lnext=^V
	eol2=\0
*/
#define INIT_C_CC {'\003','\034','\177','\025','\004','\0','\1','\0','\021','\023','\032','\0','\022','\017','\027','\026','\0'}

#define _L_FLAG(tty,f)	((tty)->termios.c_lflag & f)
#define _I_FLAG(tty,f)	((tty)->termios.c_iflag & f)
#define _O_FLAG(tty,f)	((tty)->termios.c_oflag & f)

#define L_CANON(tty)	_L_FLAG((tty),ICANON)
#define L_ISIG(tty)	_L_FLAG((tty),ISIG)
#define L_ECHO(tty)	_L_FLAG((tty),ECHO)
#define L_ECHOE(tty)	_L_FLAG((tty),ECHOE)
#define L_ECHOK(tty)	_L_FLAG((tty),ECHOK)
#define L_ECHOCTL(tty)	_L_FLAG((tty),ECHOCTL)
#define L_ECHOKE(tty)	_L_FLAG((tty),ECHOKE)

#define I_UCLC(tty)	_I_FLAG((tty),IUCLC)
#define I_NLCR(tty)	_I_FLAG((tty),INLCR)
#define I_CRNL(tty)	_I_FLAG((tty),ICRNL)
#define I_NOCR(tty)	_I_FLAG((tty),IGNCR)

#define O_POST(tty)	_O_FLAG((tty),OPOST)
#define O_NLCR(tty)	_O_FLAG((tty),ONLCR)
#define O_CRNL(tty)	_O_FLAG((tty),OCRNL)
#define O_NLRET(tty)	_O_FLAG((tty),ONLRET)
#define O_LCUC(tty)	_O_FLAG((tty),OLCUC)

// blocking read or non-blocking?
char TtyDevice::read()
{
    //TODO: honor vmin & vtime
    char ch;
    if (L_CANON(tty1)) {
        while (secondaryq.empty() || !has_full_line) {
            auto oflags = ttylock.lock();
            sleep(&ttylock, &readq);
            ttylock.release(oflags);
        }

        ch = secondaryq.read();
        if (ch == '\n') has_full_line = false;
    } else {
        while (readq.empty()) {
            auto oflags = ttylock.lock();
            sleep(&ttylock, &readq);
            ttylock.release(oflags);
        }
        ch = readq.read();
    } 
    return ch;
}

bool TtyDevice::write(char ch)
{
	kputchar(ch);
	return true;
}

void tty_thread()
{
    while (tty1) {
        if (!kbd.msbuf().empty()) {
            mouse_packet_t pkt = kbd.msbuf().read();

            auto old = current_display->get_text_color();
            auto cur = current_display->get_cursor();
            current_display->set_text_color(LIGHT_BLUE);
            current_display->set_cursor({60, 2});

            kprintf(" @%d, %d, 0x%x@ ", pkt.relx, pkt.rely, pkt.flags);

            current_display->set_cursor(cur);
            current_display->set_text_color(old);
        }


        asm ("hlt");
    }
}

// call by kbd interrupt handler, do the job a line discipline would do
void tty_enqueue()
{
    if (!tty1 || kbd.kbbuf().empty()) return;

    key_packet_t pkt = kbd.kbbuf().read();
    KeyCode key = pkt.keycode;
    if (pkt.status & KB_RELEASE) return;

    switch(key) {
        case KEY_UNKNOWN:
        case KEY_LSHIFT: case KEY_RSHIFT: case KEY_LCTRL: case KEY_RCTRL:
        case KEY_LALT: case KEY_RALT: return; 
        default: break;
    }

    char ch = (char)key;
    if (pkt.status & (KB_SHIFT_DOWN | KB_CAPS_LOCK)) {
        if (key >= KEY_A && key <= KEY_Z) {
            ch = (key - 0x20);
        } 
    } else if (pkt.status & KB_CTRL_DOWN) {
        switch (key) {
            case KEY_AT: ch = '\0'; break; 
            case KEY_LEFTBRACKET: ch = '\033'; break; //^[
            case KEY_ESCAPE: ch = '\033'; break; //^[
            case KEY_BACKSLASH: ch = '\034'; break; 
            case KEY_RIGHTBRACKET: ch = '\035'; break; 
            case KEY_QUESTION: ch = KEY_BACKSPACE; break; //^?
            default:
                 if (key >= KEY_A && key < KEY_Z) {
                     ch = key - 'a' + 1; // control characters
                 }
        }
    } else {
        ch = (char)key;
        if (key == KEY_RETURN)
            ch = ('\n');
        else if (key == KEY_TAB)
            ch = ('\t');
    }

    if (ch == '\n' && I_NLCR(tty1)) {
        ch = '\r';
    }

    if (ch == '\r' && I_CRNL(tty1)) {
        ch = '\n';
    }

    if (L_ISIG(tty1)) {
        if (ch == INTR_CHAR(tty1)) {
            //TODO: send signal
            return;
        } else if (ch == QUIT_CHAR(tty1)) {
            // kill proc
            return;
        }
    }

    tty1->readq.write(ch);
    if (L_CANON(tty1)) {
        if (ch == ERASE_CHAR(tty1) || ch == '\b') {
            //do erase
            if (!tty1->secondaryq.empty() && tty1->secondaryq.last() != '\n') {
                current_display->del();
                tty1->secondaryq.drop();
            }
            return;

        } else if (ch == KILL_CHAR(tty1)) {
            //remove line
            while (!tty1->secondaryq.empty() && tty1->secondaryq.last() != '\n') {
                current_display->del();
                tty1->secondaryq.drop();
            }
            return;

        } else if (ch == WERASE_CHAR(tty1)) {
            //remove word
            while (!tty1->secondaryq.empty() && !isspace(tty1->secondaryq.last())) {
                current_display->del();
                tty1->secondaryq.drop();
            }
            return;

        } else if (ch == EOF_CHAR(tty1)) {
            ch = '\n'; // return immediately
            tty1->secondaryq.write(ch);
            tty1->has_full_line = true;
            wakeup(&tty1->readq);
            return;
        }
    } 

    if (L_ECHO(tty1)) {
        current_display->putchar(ch);
    }

    if (L_CANON(tty1)) {
        if (ch == '\n') tty1->has_full_line = true;
        tty1->secondaryq.write(ch);

        if (tty1->has_full_line) {
            wakeup(&tty1->readq);
        }
    }
}

void tty_init()
{
    tty1 = new TtyDevice;
    tty1->termios = (struct termios) {
        ICRNL,		/* change incoming CR to NL */
        OPOST|ONLCR,	/* change outgoing NL to CRNL */
        0,
        ISIG | ICANON | ECHO | ECHOCTL | ECHOKE,
        0,		/* console termio */
        INIT_C_CC
    };
    tty1->has_full_line = false;
    tty1->dev = DEVNO(TTY_MAJOR, 1);
    chr_device_register(tty1->dev, tty1);
}
