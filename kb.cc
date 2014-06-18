#include "kb.h"
#include "isr.h"

enum KB_ENCODER_IO {
    KB_ENC_INPUT_BUF =   0x60,
    KB_ENC_CMD_REG   =   0x60
};
 
enum KB_CTRL_IO {
    KB_CTRL_STATS_REG =   0x64,
    KB_CTRL_CMD_REG   =   0x64
};

enum KB_CTRL_STATS_MASK {
    KB_CTRL_STATS_MASK_OUT_BUF   =   0x01, 
    KB_CTRL_STATS_MASK_IN_BUF    =   0x02,
    KB_CTRL_STATS_MASK_SYSTEM    =   0x04,
    KB_CTRL_STATS_MASK_CMD_DATA  =   0x08,
    KB_CTRL_STATS_MASK_LOCKED    =   0x10,
    KB_CTRL_STATS_MASK_AUX_BUF   =   0x20,
    KB_CTRL_STATS_MASK_TIMEOUT   =   0x40,
    KB_CTRL_STATS_MASK_PARITY    =   0x80 
};

#define KB_CTRL_STATS_OUT_BUF_EMPTY 0    
#define KB_CTRL_STATS_OUT_BUF_READY  1    

#define KB_CTRL_STATS_IN_BUF_READY 0
#define KB_CTRL_STATS_IN_BUF_FULL  1

enum KB_ENC_CMDS {
    KBE_CMD_SET_LEDS              =   0xED,
    KBE_CMD_ECHO                  =   0xEE,
    KBE_CMD_SCAN_CODE_SET         =   0xF0,
    KBE_CMD_ID                    =   0xF2,
    KBE_CMD_AUTODELAY             =   0xF3,
    KBE_CMD_ENABLE                =   0xF4,
    KBE_CMD_RESETWAIT             =   0xF5,
    KBE_CMD_RESETSCAN             =   0xF6,
    KBE_CMD_ALL_AUTO              =   0xF7,
    KBE_CMD_ALL_MAKEBREAK         =   0xF8,
    KBE_CMD_ALL_MAKEONLY          =   0xF9,
    KBE_CMD_ALL_MAKEBREAK_AUTO    =   0xFA,
    KBE_CMD_SINGLE_AUTOREPEAT     =   0xFB,
    KBE_CMD_SINGLE_MAKEBREAK      =   0xFC,
    KBE_CMD_SINGLE_BREAKONLY      =   0xFD,
    KBE_CMD_RESEND                =   0xFE,
    KBE_CMD_RESET                 =   0xFF
};


enum KB_CTRL_CMDS {
    KB_CTRL_CMD_READ             =   0x20,
    KB_CTRL_CMD_WRITE            =   0x60,
    KB_CTRL_CMD_SELF_TEST        =   0xAA,
    KB_CTRL_CMD_INTERFACE_TEST   =   0xAB,
    KB_CTRL_CMD_DISABLE          =   0xAD,
    KB_CTRL_CMD_ENABLE           =   0xAE,
    KB_CTRL_CMD_READ_IN_PORT     =   0xC0,
    KB_CTRL_CMD_READ_OUT_PORT    =   0xD0,
    KB_CTRL_CMD_WRITE_OUT_PORT   =   0xD1,
    KB_CTRL_CMD_READ_TEST_INPUTS =   0xE0,
    KB_CTRL_CMD_SYSTEM_RESET     =   0xFE,
    KB_CTRL_CMD_MOUSE_DISABLE    =   0xA7,
    KB_CTRL_CMD_MOUSE_ENABLE     =   0xA8,
    KB_CTRL_CMD_MOUSE_PORT_TEST  =   0xA9,
    KB_CTRL_CMD_MOUSE_WRITE      =   0xD4
};

enum KB_ERROR {
    KB_ERR_BUF_OVERRUN           =   0,
    KB_ERR_ID_RET                =   0x83AB,
    KB_ERR_BAT                   =   0xAA,   //note: can also be L. shift key make code
    KB_ERR_ECHO_RET              =   0xEE,
    KB_ERR_ACK                   =   0xFA,
    KB_ERR_BAT_FAILED            =   0xFC,
    KB_ERR_DIAG_FAILED           =   0xFD,
    KB_ERR_RESEND_CMD            =   0xFE,
    KB_ERR_KEY                   =   0xFF
};


//! original xt scan code set. Array index==make code
//! change what keys the scan code corrospond to if your scan code set is different
static KeyCode _xtkb_scancode_std[] = {

    //! key         scancode
    KEY_UNKNOWN,    //0
    KEY_ESCAPE,     //1
    KEY_1,          //2
    KEY_2,          //3
    KEY_3,          //4
    KEY_4,          //5
    KEY_5,          //6
    KEY_6,          //7
    KEY_7,          //8
    KEY_8,          //9
    KEY_9,          //0xa
    KEY_0,          //0xb
    KEY_MINUS,      //0xc
    KEY_EQUAL,      //0xd
    KEY_BACKSPACE,  //0xe
    KEY_TAB,        //0xf
    KEY_Q,          //0x10
    KEY_W,          //0x11
    KEY_E,          //0x12
    KEY_R,          //0x13
    KEY_T,          //0x14
    KEY_Y,          //0x15
    KEY_U,          //0x16
    KEY_I,          //0x17
    KEY_O,          //0x18
    KEY_P,          //0x19
    KEY_LEFTBRACKET,//0x1a
    KEY_RIGHTBRACKET,//0x1b
    KEY_RETURN,     //0x1c
    KEY_LCTRL,      //0x1d
    KEY_A,          //0x1e
    KEY_S,          //0x1f
    KEY_D,          //0x20
    KEY_F,          //0x21
    KEY_G,          //0x22
    KEY_H,          //0x23
    KEY_J,          //0x24
    KEY_K,          //0x25
    KEY_L,          //0x26
    KEY_SEMICOLON,  //0x27
    KEY_QUOTE,      //0x28
    KEY_GRAVE,      //0x29
    KEY_LSHIFT,     //0x2a
    KEY_BACKSLASH,  //0x2b
    KEY_Z,          //0x2c
    KEY_X,          //0x2d
    KEY_C,          //0x2e
    KEY_V,          //0x2f
    KEY_B,          //0x30
    KEY_N,          //0x31
    KEY_M,          //0x32
    KEY_COMMA,      //0x33
    KEY_DOT,        //0x34
    KEY_SLASH,      //0x35
    KEY_RSHIFT,     //0x36
    KEY_KP_ASTERISK,//0x37
    KEY_RALT,       //0x38
    KEY_SPACE,      //0x39
    KEY_CAPSLOCK,   //0x3a
    KEY_F1,         //0x3b
    KEY_F2,         //0x3c
    KEY_F3,         //0x3d
    KEY_F4,         //0x3e
    KEY_F5,         //0x3f
    KEY_F6,         //0x40
    KEY_F7,         //0x41
    KEY_F8,         //0x42
    KEY_F9,         //0x43
    KEY_F10,        //0x44
    KEY_KP_NUMLOCK, //0x45
    KEY_SCROLLLOCK, //0x46
    KEY_HOME,       //0x47
    KEY_KP_8,       //0x48  //keypad up arrow
    KEY_PAGEUP,     //0x49
    KEY_KP_2,       //0x50  //keypad down arrow
    KEY_KP_3,       //0x51  //keypad page down
    KEY_KP_0,       //0x52  //keypad insert key
    KEY_KP_DECIMAL, //0x53  //keypad delete key
    KEY_UNKNOWN,    //0x54
    KEY_UNKNOWN,    //0x55
    KEY_UNKNOWN,    //0x56
    KEY_F11,        //0x57
    KEY_F12         //0x58
};

Keyboard* Keyboard::_instance = NULL;

//FIXME: this is a *HACK*: operator new is not implement yet
static u32 _kb_base[sizeof(Keyboard)];

static void debug_print(Keyboard* kb)
{
    KeyCode key = kb->last_key();
    if (key == KEY_UNKNOWN)
        return;

    if (kb->shift_down()) {
        if (key >= KEY_A && key <= KEY_Z) {
            kputchar(key - 0x20);
        } else if (key >= KEY_0 && key <= KEY_9){
        }
    } else 
        kputchar(key);
}

static void keyboard_irq(registers_t* regs)
{
    static bool _is_extended = false;
    (void)regs;

    Keyboard* kb = Keyboard::get();
    if (!kb->can_read()) return;

    u8 data = kb->kbe_read();
    if (data == 0xE0) {
        _is_extended = true;
        return;
    }

    if (_is_extended) {
        //TODO:
        _is_extended = false;
    } else {
        if (data & 0x80) {
            //Break Code
            switch(_xtkb_scancode_std[data & 0x7f]) {
                case KEY_LSHIFT:
                case KEY_RSHIFT:
                    kb->set_shift_down(false); break;

                case KEY_LCTRL:
                case KEY_RCTRL:
                    kb->set_ctrl_down(false); break;

                case KEY_LALT:
                case KEY_RALT:
                    kb->set_alt_down(false); break;
            }
        } else {
            //Make Code
            kb->set_last_key(_xtkb_scancode_std[data]);
            switch(kb->last_key()) {
                case KEY_LSHIFT:
                case KEY_RSHIFT:
                    kb->set_shift_down(true); break;

                case KEY_LCTRL:
                case KEY_RCTRL:
                    kb->set_ctrl_down(true); break;

                case KEY_LALT:
                case KEY_RALT:
                    kb->set_alt_down(true); break;
            }

            debug_print(kb);
        }
    }
}

Keyboard* Keyboard::get()
{
    if (!_instance) 
        _instance = new((void*)&_kb_base) Keyboard;
    return _instance;
}

void Keyboard::init()
{
    kbe_send(KBE_CMD_ENABLE);
    set_leds(false, false, false);
    register_isr_handler(IRQ1, keyboard_irq);
}

Keyboard::Keyboard()
{
    _scroll_lock_on = false;
    _num_lock_on = false;
    _caps_lock_on = false;

}

void Keyboard::kbe_send(u8 cmd)
{
    while (1) {
        if ((kbc_read() & KB_CTRL_STATS_MASK_IN_BUF) == 0)
            break;
        kputs("kbc_read wait");
    }
    outb(KB_ENC_CMD_REG, cmd);
}

void Keyboard::kbc_send(u8 cmd)
{
    while (1) {
        if ((kbc_read() & KB_CTRL_STATS_MASK_IN_BUF) == 0)
            break;
        kputs("kbc_read wait");
    }
    outb(KB_CTRL_CMD_REG, cmd);
}

bool Keyboard::can_read()
{
    return (kbc_read() & KB_CTRL_STATS_MASK_OUT_BUF) == 1;
}

u8 Keyboard::kbe_wait_and_read()
{
    while (1) {
        if ((kbc_read() & KB_CTRL_STATS_MASK_OUT_BUF) == 1)
            break;
        kputs("kbc_read wait");
    }
    return inb(KB_ENC_INPUT_BUF);
}

u8 Keyboard::kbe_read()
{
    return inb(KB_ENC_INPUT_BUF);
}

u8 Keyboard::kbc_read()
{
    return inb(KB_CTRL_STATS_REG);
}

//Bit 0: Scroll lock LED (0: off 1:on)
//Bit 1: Num lock LED (0: off 1:on)
//Bit 2: Caps lock LED (0: off 1:on)
void Keyboard::set_leds(bool scroll, bool num, bool caps)
{
    u8 data = 0;
    data |= (scroll ? 0x1 : 0);
    data |= (num ? 0x2 : 0);
    data |= (caps ? 0x3 : 0);

    kbe_send(KBE_CMD_SET_LEDS);
    kbe_send(data);

    _scroll_lock_on = scroll;
    _num_lock_on = num;
    _caps_lock_on = caps;
}

