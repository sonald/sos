#ifndef _SOS_STDLIB_H
#define _SOS_STDLIB_H 

#include <stddef.h>
#include <types.h>

BEGIN_CDECL
void* calloc(size_t count, size_t size);
void free(void *ptr);
void* malloc(size_t size);
void* realloc(void *ptr, size_t size);

END_CDECL

#endif
