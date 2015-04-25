#include "tty.h"
#include <devices.h>
#include <kb.h>
#include <task.h>
#include <spinlock.h>

Spinlock ttylock {"tty"};

// blocking read or non-blocking?
char TtyDevice::read()
{
	while (1) {
		while (kbd.kbbuf().empty()) {
			auto oflags = ttylock.lock();
			// kprintf("tty: sleep\n");
			sleep(&ttylock, &kbd.kbbuf());
			ttylock.release(oflags);
		}

		key_packet_t pkt = kbd.kbbuf().read();
		KeyCode key = pkt.keycode;
		if (key == KEY_UNKNOWN || (pkt.status & KB_RELEASE))
			continue;

		switch(key) {
			case KEY_LSHIFT:
			case KEY_RSHIFT:
			case KEY_LCTRL:
			case KEY_RCTRL:
			case KEY_LALT:
			case KEY_RALT: break;

			default:
			if (pkt.status & KB_SHIFT_DOWN) {
				if (key >= KEY_A && key <= KEY_Z) {
					return (char)(key - 0x20);
				} else if (key >= KEY_0 && key <= KEY_9) {

				}
			} else {
				if (key == KEY_RETURN)
					return ('\n');
				else if (key == KEY_TAB)
					return ('\t');
				else if (key == KEY_BACKSPACE)
					return ('\b');
				else
					return (char)key;
			}
		}
	}

	return -1;
}

bool TtyDevice::write(char ch)
{
	kputchar(ch);
	return true;
}

void tty_init()
{
	auto* tty1 = new TtyDevice;
	tty1->dev = DEVNO(TTY_MAJOR, 1);
	chr_device_register(tty1->dev, tty1);
}