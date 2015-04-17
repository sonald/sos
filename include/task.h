#ifndef _TASK_H
#define _TASK_H 

#include "isr.h"
#include "vm.h"
#include "vfs.h"
#include "spinlock.h"
#include <sos/limits.h>

#define PROC_NAME_LEN 31

typedef s32 pid_t;

enum proc_state {
    TASK_UNUSED = 1,
    TASK_CREATE,
    TASK_READY,
    TASK_RUNNING,
    TASK_SLEEP,
};

typedef struct proc_s {
    trapframe_t* regs;
    kcontext_t* kctx;

    pid_t pid;
    pid_t ppid;
    char name[32];

    u32 kern_esp;
    u32 user_esp;
    void* entry;
    enum proc_state state;
    bool need_resched;

    page_directory_t* pgdir;

    File* files[FILES_PER_PROC];

    void* channel; // sleep on
    struct proc_s* next;
} proc_t;

extern proc_t* current;
extern proc_t tasks[MAXPROCS];
void tasks_init();

void sleep(Spinlock* lk, void* chan);
void wakeup(void* chan);

proc_t* prepare_userinit(void* prog);
proc_t* create_proc(void* entry, void* proc, size_t size, const char* name);

using kthread_t = void (*)();
proc_t* create_kthread(const char* name, kthread_t prog);

#endif
