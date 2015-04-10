#ifndef _SOS_FONT_H
#define _SOS_FONT_H 

typedef struct font_info_s {
    int xadvance, yadvance;
} font_info_t;


extern font_info_t builtin_fontinfo;
extern const char* builtin_font[];
#endif
