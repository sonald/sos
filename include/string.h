#ifndef _STRING_H
#define _STRING_H 

#include "types.h"

void * memcpy(void * dst, const void * src, size_t n);
void* memset(void* dst, int c, size_t count);
void* memmove(void* dst, const void * src, size_t n);
int memcmp(const void *s1, const void * s2, size_t count);

size_t strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t count);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);
char* strncat(char * dst, const char *src, size_t count);
char* strcat(char * dst, const char *src);

char* strchr(const char *s, int c);
char* strrchr(const char *s, int c);
#endif
