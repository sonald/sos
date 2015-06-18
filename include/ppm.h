#ifndef _SOS_PPM_H
#define _SOS_PPM_H  

typedef struct ppm_s {
    char magic[2];
    int width, height;
    int max_color;
    char data[0];
} ppm_t;

ppm_t* ppm_load(const char* path);

#endif
