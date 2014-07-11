#include "task.h"
#include "vm.h"
#include "x86.h"
#include "errno.h"

// this contains not-only user mode process and also kernel-threads
proc_t tasks[MAXPROCS];

proc_t* current_proc = NULL;
static int next_pid = 0;

static proc_t* find_free_process()
{
repeat:
    if ((++next_pid) < 0) next_pid = 1; // wrapped
    for (int i = 0; i < MAXPROCS; ++i) {
        if (tasks[i].state != TASK_UNUSED && tasks[i].pid == next_pid) 
            goto repeat;
    }

    proc_t* p = NULL;
    for (int i = 0; i < MAXPROCS; ++i) {
        if (tasks[i].state == TASK_UNUSED) {
            p = &tasks[i];
            break;
        }
    }

    return p;
}

int sys_getppid()
{
    kassert(current_proc);
    return current_proc->ppid;
}


int sys_getpid()
{
    kassert(current_proc);
    return current_proc->pid;
}

int sys_fork() 
{
    proc_t* proc = find_free_process();
    if (!proc) return -1;

    *proc = *current_proc;
    proc->state = TASK_CREATE;

    proc->ppid = current_proc->pid;
    proc->pid = next_pid;
    int len = strlen(proc->name);
    proc->name[len++] = '@';
    proc->name[len] = '\0';

    if (!current_proc) current_proc = proc;
    else { 
        proc->next = current_proc->next;
        current_proc->next = proc;
    }

    void* task_kern_stack = vmm.kmalloc(PGSIZE);
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    *(proc->regs) = *(current_proc->regs);

    proc->pgdir = vmm.copy_page_directory(current_proc->pgdir);

    kprintf("fork %d -> %d\n", current_proc->pid, next_pid);
    proc->state = TASK_READY;
    return proc->pid;
}

void tasks_init()
{
    memset(tasks, 0, sizeof tasks);
    for (int i = 0; i < MAXPROCS; ++i) {
        tasks[i].state = TASK_UNUSED;
    }
}

proc_t* create_proc(void* entry, void* prog, size_t size, const char* name)
{
    proc_t* proc = find_free_process();
    kassert(proc);
    memset(proc, 0, sizeof(*proc));
    strcpy(proc->name, name);

    if (!current_proc) current_proc = proc;
    else { 
        proc->next = current_proc->next;
        current_proc->next = proc;
    }

    page_directory_t* pdir = vmm.create_address_space();

    proc->pgdir = pdir;
    void* vaddr = (void*)UCODE; 
    void* tmp = vmm.kmalloc(size, PGSIZE);
    u32 paddr = v2p(tmp);
    vmm.map_pages(pdir, vaddr, size, paddr, PDE_USER|PDE_WRITABLE);
    // copy from tmp avoid a page directory switch 
    memcpy(tmp, prog, size);

    void* task_usr_stack0 = (void*)USTACK; 
    u32 paddr_stack0 = v2p(vmm.kmalloc(PGSIZE));
    vmm.map_pages(pdir, task_usr_stack0, PGSIZE, paddr_stack0, PDE_USER|PDE_WRITABLE);

    void* task_kern_stack = vmm.kmalloc(PGSIZE);

    proc->entry = entry;
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->user_esp = USTACK_TOP;
    
    trapframe_t* regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    regs->useresp = proc->user_esp;
    regs->esp = proc->kern_esp;
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = 0x23;
    regs->cs = 0x1b;
    regs->eip = A2I(entry);
    regs->eflags = 0x202;
    proc->regs = regs;

    proc->state = TASK_READY;
    proc->pid = next_pid;
    kprintf("alloc task(%d, 0x%x) @0x%x, ustack: 0x%x, kesp: 0x%x\n", 
            proc->pid, proc, paddr, paddr_stack0, proc->kern_esp);

    return proc;
}
