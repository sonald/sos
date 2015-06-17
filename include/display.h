#ifndef _SOS_DISPLAY_H
#define _SOS_DISPLAY_H 

#include "types.h"
#include "graphics.h"
#include "spinlock.h"

// http://www.rapidtables.com/web/color/RGB_Color.htm
enum Color {
    BLACK =       0,   // BLACK   
    BLUE =        1,   // BLUE    
    GREEN =       2,   // GREEN   
    CYAN =        3,   // CYAN    0x00ffff
    RED =         4,   // RED     
    MAGENTA =     5,   // MAGENTA   0xff00ff
    BROWN =       6,   // BROWN     0xa52a2a
    LIGHT_GRAY =  7,   // LIGHT GRAY 0xd3d3d3
    DARK_GRAY =   8,   // DARK GRAY 0xbebebe
    LIGHT_BLUE =  9,   // LIGHT BLUE  0xadd8e6
    LIGHT_GREEN = 10,  // LIGHT GREEN 0x90ee90
    LIGHT_CYAN =  11,  // LIGHT CYAN  0xe0ffff
    LIGHT_RED =   12,  // LIGHT RED  0xcd5c5c?
    LIGHT_MAGENTA = 13,  // LIGHT MAGENTA  0xee00ee
    LIGHT_BROWN = 14,  // LIGHT BROWN  0xff4040
    WHITE =       15,  // WHITE ,

    // below is not supported in text mode
    YELLOW = 16, // 0xffff00
    LIGHT_YELLOW = 17, // 0xfafad2
    PURPLE = 18, //0x800080

    MAXCOLOR=PURPLE
};

class Display 
{
    public:
        virtual Color get_text_color() = 0;
        virtual void set_text_color(Color) = 0;
        virtual void set_cursor(position_t cur) = 0;
        virtual position_t get_cursor() = 0;
        virtual void clear() = 0;

        virtual void scroll(int lines) = 0;
        virtual void putchar(char c) = 0;

        virtual void del() = 0;
};

class Console: public Display 
{
    public:
        Color get_text_color() override;
        void set_text_color(Color) override;
        void set_cursor(position_t cur) override;
        position_t get_cursor() override;
        void clear() override;

        void putchar(char c) override;
        void scroll(int lines) override;
        void del() override;

    private:
        void set_phy_cursor(int x, int y);
        int _cx {0}, _cy {0};
        u8 _attrib {0x0F};
        Spinlock _lock {"console"};
};

class GraphicDisplay: public Display 
{
    public:
        Color get_text_color() override;
        void set_text_color(Color) override;
        void set_cursor(position_t cur) override;
        position_t get_cursor() override;
        void clear() override;
        void putchar(char c) override;
        void scroll(int lines) override;
        void del() override;

    private:
        position_t _cursor {0, 0};
        int _rows {34}, _cols {88};
        Rgb _clr {colormap[RED]};
        Color _clridx {RED};
        Spinlock _lock {"graphdisplay"};
};

extern Console console;
extern GraphicDisplay graph_console;
extern Display* current_display;
extern void video_mode_test();

#endif
