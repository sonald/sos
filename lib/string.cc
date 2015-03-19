#include "string.h"

void * memcpy(void * dst, const void * src, size_t n)
{
    asm (
        "cld \n"
        "rep movsb \n"
        ::"D"(dst), "S"(src), "c"(n): "cc", "memory");
    return dst;
}

void* memmove(void* dst, const void * src, size_t n)
{
    if (dst < src) {
        asm (
            "cld \n"
            "rep movsb \n"
            ::"D"(dst), "S"(src), "c"(n): "cc", "memory");
    } else {
        asm (
            "std \n"
            "rep movsb \n"
            ::"D"((char*)dst+n-1), "S"((char*)src+n-1), "c"(n): "cc", "memory");
    }
    return dst;
}

void* memset(void* dst, int c, size_t count)
{
    asm (
        "cld \n"
        "rep stosb \n"
        ::"D"(dst), "a"(c), "c"(count): "cc", "memory");
    return dst;
}

int memcmp(const void *s1, const void * s2, size_t count)
{
    register int ret asm ("eax");
    asm (
        "cld \n"
        "repe cmpsb \n"
        "je 1f \n"
        "movl $1, %0 \n"
        "jl 1f \n"
        "negl %0 \n"
        "1: "
        :"=a"(ret):"0"(0), "D"(s1), "S"(s2), "c"(count));
    return ret;
}


size_t strlen(const char* s)
{
    register size_t ret asm ("ecx");
    asm (
        "cld \n"
        "repne scasb \n"
        "notl %0 \n"
        "decl %0 "
        :"=c"(ret):"a"(0), "D"(s), "0"(-1));
    return ret;
}

char* strcpy(char* dst, const char* src)
{
    asm (
        "cld \n"
        "1:lodsb \n"
        "stosb \n"
        "testb %%al, %%al \n"
        "jnz 1b "
        ::"D"(dst), "S"(src)
        :"eax");
    return dst;
}

char* strncpy(char* dst, const char* src, size_t count)
{
    asm (
        "cld \n"
        "1: decl %2 \n"
        "js 2f \n"
        "lodsb \n"
        "stosb \n"
        "testb %%al, %%al \n"
        "jne 1b \n"
        "rep stosb \n"
        "2: "
        ::"D"(dst), "S"(src), "c"(count)
        :"eax");
    return dst;
}

char * strcat(char * dst, const char *src)
{
    asm (
        "cld \n"
        "repne scasb \n" // reach end of dst
        "decl %0 \n"
        "1: lodsb \n"
        "stosb \n"
        "test %%al, %%al \n"
        "jnz 1b \n"
        ::"D"(dst), "S"(src), "a"(0), "c"(0xffffffff));
    return dst;
}

char * strncat(char * dst, const char *src, size_t count)
{
    asm (
        "cld \n"
        "repne scasb \n" // reach end of dst
        "decl %0 \n"
        "mov %4, %3 \n"
        "1: decl %3 \n"
        "js 2f \n"
        "lodsb \n"
        "stosb \n"
        "test %%al, %%al \n"
        "jnz 1b \n"
        "2: xor %2, %2 \n"
        "stosb "
        ::"D"(dst), "S"(src), "a"(0), "c"(0xffffffff), "g"(count));
    return dst;
}

int strcmp(const char *s1, const char *s2)
{
    register int ret asm("eax");
    asm (
        "cld \n"
        "1: lodsb \n"
        "scasb \n"
        "jne 2f \n"
        "testb %%al, %%al \n"
        "jnz 1b \n"
        "xorl %%eax, %%eax \n"
        "jmp 3f \n"
        "2: movl $1, %0 \n"
        "jg 3f \n"
        "negl %0 \n"
        "3: "
        :"=a"(ret) :"S"(s1), "D"(s2));
    return ret;
}

int strncmp(const char *s1, const char *s2, size_t count)
{
    register int ret asm("eax");
    asm (
        "cld \n"
        "1: decl %3 \n"
        "js 2f \n"
        "lodsb \n"
        "scasb \n"
        "jne 3f \n"
        "testb %%al, %%al \n"
        "jne 1b \n"
        "2: xorl %%eax, %%eax \n"
        "jmp 4f \n"
        "3: movl $1, %0 \n"
        "jg 4f \n"
        "negl %0 \n"
        "4: "
        :"=a"(ret) :"S"(s1), "D"(s2), "c"(count));
    return ret;
}

char* strchr(const char *s, int c)
{
    char* ret = 0;
    asm (
        "cld \n"
        "1: lodsb \n"
        "cmpb %%al, %%dl \n"
        "je 2f \n"
        "testb %%al, %%al \n"
        "jne 1b \n"
        "movl $1, %0 \n"
        "2: decl %0 \n"
        :"=S"(ret):"d"(c), "S"(s)
        :"al"
        );
    return ret;
}

char* strrchr(const char *s, int c)
{
    char* ret = 0;
    asm (
            "cld \n"
            "movb %%al, %%ah \n"
            "1: lodsb \n"
            "cmpb %%al, %%ah \n"
            "jne 2f \n"
            "movl %2, %0 \n"
            "decl %0 \n"
            "2: testb %%al, %%al \n"
            "jne 1b \n"
            :"=d"(ret):"0"(0), "S"(s), "a"(c)
        );
    return ret;
}


