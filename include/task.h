#ifndef _TASK_H
#define _TASK_H 

#include "isr.h"
#include "vm.h"
#include "vfs.h"

#define MAXPROCS 16
#define PROC_NAME_LEN 31

#define FILES_PER_PROC  16 // max filer per task

typedef s32 pid_t;

enum proc_state {
    TASK_UNUSED,
    TASK_CREATE,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEP,
};

typedef struct proc_s {
    trapframe_t* regs;

    pid_t pid;
    pid_t ppid;
    char name[32];

    u32 kern_esp;
    u32 user_esp;
    void* entry;
    enum proc_state state;

    page_directory_t* pgdir;

    File* files[FILES_PER_PROC];

    struct proc_s* next;
} proc_t;

extern proc_t* current_proc;
extern proc_t tasks[MAXPROCS];
int sys_getppid();
int sys_getpid();
int sys_fork(); 
void tasks_init();

proc_t* create_proc(void* entry, void* proc, size_t size, const char* name);

#endif
