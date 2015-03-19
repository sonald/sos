#ifndef _SOS_DISPLAY_H
#define _SOS_DISPLAY_H 

#include "types.h"
#include "graphics.h"

#define COLOR(fg, bg) ((((fg) & 0x0f) | ((bg) & 0xf0)) & 0xff)
enum Color {
    BLACK =       0,   // BLACK   
    BLUE =        1,   // BLUE    
    GREEN =       2,   // GREEN   
    CYAN =        3,   // CYAN    
    RED =         4,   // RED     
    MAGENTA =     5,   // MAGENTA 
    BROWN =       6,   // BROWN   
    LIGHT_GREY =  7,   // LIGHT GREY 
    DARK_GREY =   8,   // DARK GREY
    LIGHT_BLUE =  9,   // LIGHT BLUE
    LIGHT_GREEN = 10,  // LIGHT GREEN
    LIGHT_CYAN =  11,  // LIGHT CYAN
    LIGHT_RED =   12,  // LIGHT RED
    LIGHT_MAGENTA = 13,  // LIGHT MAGENTA
    LIGHT_BROWN = 14,  // LIGHT BROWN
    WHITE =       15,  // WHITE
};

class Display 
{
    public:
        virtual u8 get_text_color() = 0;
        virtual void set_text_color(u8) = 0;
        virtual void set_cursor(position_t cur) = 0;
        virtual position_t get_cursor() = 0;
        virtual void clear() = 0;

        virtual void scroll(int lines) = 0;
        virtual void putchar(char c) = 0;
};

class Console: public Display 
{
    public:
        u8 get_text_color() override;
        void set_text_color(u8) override;
        void set_cursor(position_t cur) override;
        position_t get_cursor() override;
        void clear() override;

        void putchar(char c) override;
        void scroll(int lines) override;

    private:
        void set_phy_cursor(int x, int y);

        int _cx {0}, _cy {0};
        u8 _attrib {0x0F};
};

class GraphicDisplay: public Display 
{
    public:
        u8 get_text_color() override;
        void set_text_color(u8) override;
        void set_cursor(position_t cur) override;
        position_t get_cursor() override;
        void clear() override;
        void putchar(char c) override;
        void scroll(int lines) override;

    private:

        position_t _cursor {0, 0};
        int _rows {34}, _cols {88};
        Rgb _clr {0xff, 0xff, 0x0};
};

extern Console console;
extern GraphicDisplay graph_console;
extern Display* current_display;

#endif
