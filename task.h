#ifndef _TASK_H
#define _TASK_H 

#include "isr.h"
#include "vm.h"

#define MAXPROCS 16
#define PROC_NAME_LEN 31

typedef s32 pid_t;

typedef struct task_s {
    void (*entry)();
    char name[32];
} task_t;

typedef struct proc_s {
    registers_t* regs;

    pid_t pid;
    char name[32];

    u32 kern_esp;
    u32 user_esp;
    void* entry;

    page_directory_t* pgdir;
    struct proc_s* next;
} proc_t;

extern proc_t* current_proc;

extern proc_t proctable[MAXPROCS];

proc_t* create_proc(void* entry, const char* name);

#endif
