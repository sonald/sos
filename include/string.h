#ifndef _STRING_H
#define _STRING_H 

#include "types.h"

void* memcpy(void* dst, const void* src, int n);
void* memset(void *b, int c, int len);
int strlen(const char* s);
char* strcpy(char* dst, const char* src);
int strcmp(const char *s1, const char *s2);
int strncmp(const char *s1, const char *s2, size_t n);

#endif
