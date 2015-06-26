#ifndef _SOS_PRINTF_H
#define _SOS_PRINTF_H 

#include "types.h"
#include "stdarg.h"

int vprintf(const char* fmt, va_list args);
int printf(const char* fmt, ...);

#endif
