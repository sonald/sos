#ifndef _TYPES_H
#define _TYPES_H 

#include <stddef.h>
#if !defined __cplusplus
#include <stdbool.h>
#endif
#include <stdint.h>

#if defined __cplusplus

#define BEGIN_CDECL extern "C" {
#define END_CDECL }

#else

#define BEGIN_CDECL 
#define END_CDECL

#endif

#if !defined uint32_t
typedef unsigned int u32;
typedef int s32;
typedef unsigned short u16;
typedef short s16;
typedef unsigned char u8;
typedef char s8;
#else
typedef uint32_t u32;
typedef int32_t s32;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint8_t u8;
typedef int8_t s8;
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif

typedef int pid_t;
typedef unsigned short uid_t;
typedef unsigned char gid_t;
typedef uint32_t dev_t;
typedef unsigned short ino_t;
typedef unsigned short mode_t;
typedef unsigned short umode_t;
typedef unsigned char nlink_t;
typedef int daddr_t;
typedef long off_t;
typedef int64_t loff_t;
typedef unsigned char u_char;
typedef unsigned short ushort;
typedef int ssize_t;
typedef long time_t;

typedef struct { int quot,rem; } div_t;
typedef struct { long quot,rem; } ldiv_t;

#endif
