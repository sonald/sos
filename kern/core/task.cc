#include "task.h"
#include "string.h"
#include "vm.h"
#include "x86.h"
#include "errno.h"
#include "elf.h"
#include "spinlock.h"
#include "sched.h"
#include "timer.h"
#include <vfs.h>
#include <sys.h>
#include <sos/limits.h>

extern "C" void trap_return();

Spinlock tasklock("task");

// this contains not-only user mode process and also kernel-threads
proc_t tasks[MAXPROCS];

proc_t* current = NULL;
static int next_pid = 0;

void sleep(Spinlock* lk, void* chan)
{
    current->channel = chan;
    current->state = TASK_SLEEP;
    //kprintf(" (%s:sleep) ", current->name);
    lk->release();
    scheduler(); 
    lk->lock();
    //kprintf(" (%s:awaked) ", current->name);
}

void wakeup(void* chan)
{
    auto oldflags = tasklock.lock();
    auto* tsk = &tasks[0];
    while (tsk) {
        if (tsk->state == TASK_SLEEP && tsk->channel == chan) {
            tsk->state = TASK_READY;
            tsk->channel = NULL;
            // kprintf("wakeup %s(%d)  ", tsk->name, tsk->pid);
        }
        tsk = tsk->next;
    }
    tasklock.release(oldflags);
}

static proc_t* find_free_process()
{
    auto oldflags = tasklock.lock();
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
            p->state = TASK_CREATE;
            break;
        }
    }

    tasklock.release(oldflags);
    return p;
}

int sys_getppid()
{
    kassert(current);
    return current->ppid;
}

int sys_getpid()
{
    kassert(current);
    return current->pid;
}

int sys_sleep(int millisecs)
{
    auto oldflags = tasklock.lock();
    kassert(current);
    kassert(current->channel == NULL);
    current->state = TASK_SLEEP;
    current->need_resched = true;
    current->channel = add_timeout(millisecs);
    tasklock.release(oldflags);

    if (current->need_resched) scheduler();
    return 0;
}

int sys_fork() 
{
    proc_t* proc = find_free_process();
    if (!proc) return -1;

    auto oldflags = tasklock.lock();
    *proc = *current;
    proc->state = TASK_CREATE;

    proc->ppid = current->pid;
    proc->pid = next_pid;

    if (!current) current = proc;
    else { 
        proc->next = current->next;
        current->next = proc;
    }

    proc->pgdir = vmm.copy_page_directory(current->pgdir);

    void* task_kern_stack = vmm.alloc_page();
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    *(proc->regs) = *(current->regs);
    proc->regs->eax = 0; // return of child process

    proc->kctx = (kcontext_t*)((char*)proc->regs - sizeof(kcontext_t));
    *(proc->kctx) = *(current->kctx);
    proc->kctx->eip = A2I(trap_return);

    // setup user stack (only one-page, will and should expand at PageFault)
    void* task_usr_stack0 = (void*)USTACK; 
    page_t* pte = vmm.walk(proc->pgdir, task_usr_stack0, false);
    kassert(pte == NULL);
    void* new_stack = vmm.alloc_page();
    vmm.map_pages(proc->pgdir, task_usr_stack0, PGSIZE,
            v2p(new_stack), PDE_USER|PDE_WRITABLE);

    // user stack copied
    pte = vmm.walk(current->pgdir, task_usr_stack0, false);
    kassert(pte && pte->present && pte->user);
    void* old_stack = p2v(pte->frame * PGSIZE);
    memcpy(new_stack, old_stack, PGSIZE);

    kprintf("fork %d -> %d\n", current->pid, next_pid);
    kassert(proc->regs->useresp == current->regs->useresp);
    //kprintf("RET: uesp: 0x%x, eip: 0x%x\n", proc->regs->useresp, proc->regs->eip);
    proc->state = TASK_READY;
    tasklock.release(oldflags);
    return proc->pid;
}

static int load_proc(proc_t* proc, void* code, size_t size, int flags)
{
    void* vaddr = (void*)UCODE; 
    if (size < PGSIZE) size = PGSIZE;
    void* tmp = vmm.kmalloc(size, PGSIZE);
    u32 paddr = v2p(tmp);
    vmm.unmap_pages(proc->pgdir, vaddr, size, paddr, false);
    vmm.map_pages(proc->pgdir, vaddr, size, paddr, flags);
    // copy from tmp avoid a page directory switch 
    //kprintf("  %s: copy code %x to %x \n", __func__, code, tmp);
    memcpy(tmp, code, size);

    return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
    (void)argv;
    (void)envp;

    auto oldflags = tasklock.lock();
    kprintf("path(0x%x): %s\n", path, path);

    inode_t* ip = vfs.namei(path);
    kassert(ip != NULL);

    char* buf = new char[ip->size];

    int fd = sys_open(path, O_RDONLY, 0);
    int len = sys_read(fd, buf, ip->size);
    sys_close(fd);
    if (len < 0) {
        kprintf("load %s failed\n", path);
        tasklock.release(oldflags);
        return -ENOENT;
    }

    elf_header_t* elf = (elf_header_t*)buf;
    if (elf->e_magic != ELF_MAGIC) {
        kprintf("invalid elf file\n");
        tasklock.release(oldflags);
        return -EINVAL;
    }

    kprintf("elf: 0x%x, entry: 0x%x, ph: %d\n", elf, elf->e_entry, elf->e_phnum);
    elf_prog_header_t* ph = (elf_prog_header_t*)((char*)elf + elf->e_phoff);
    for (int i = 0; i < elf->e_phnum; ++i) {
        kprintf("off: 0x%x, pa: 0x%x, va: 0x%x, fsz: 0x%x, msz: 0x%x\n", 
                ph[i].p_offset, ph[i].p_pa, ph[i].p_va, ph[i].p_filesz, ph[i].p_memsz);
        if (ph[i].p_type != ELF_PROG_LOAD) {
            continue;
        }

        char* prog = (char*)elf + ph[i].p_offset;
        if (ph[i].p_filesz > ph[i].p_memsz) {
            kprintf("warning: size doesn't fit.\n");
            continue; 
        }

        load_proc(current, prog, ph[i].p_memsz, PDE_USER);
        break;
    }

    strncpy(current->name, path, sizeof current->name - 1);
    kprintf("execv task(%d, %s)\n", current->pid, path);

    current->entry = (void*)elf->e_entry;
    
    trapframe_t* regs = current->regs;
    regs->eip = elf->e_entry;
    regs->useresp = USTACK_TOP;

    tasklock.release(oldflags);
    return 0;
}

void tasks_init()
{
    memset(tasks, 0, sizeof tasks);
    for (int i = 0; i < MAXPROCS; ++i) {
        tasks[i].state = TASK_UNUSED;
    }
}

proc_t* prepare_userinit(void* prog)
{
    proc_t* proc = find_free_process();
    memset(proc, 0, sizeof(*proc));
    strcpy(proc->name, "init");

    kassert(current == NULL);
    current = proc;
    kassert(proc == &tasks[0]);

    page_directory_t* pdir = vmm.create_address_space();

    proc->pgdir = pdir;
    void* vaddr = (void*)UCODE; 
    void* tmp = vmm.alloc_page();
    u32 paddr = v2p(tmp);
    vmm.map_pages(pdir, vaddr, PGSIZE, paddr, PDE_USER);
    // copy from tmp avoid a page directory switch 
    memcpy(tmp, prog, PGSIZE);

    void* task_usr_stack0 = (void*)USTACK; 
    u32 paddr_stack0 = v2p(vmm.alloc_page());
    vmm.map_pages(pdir, task_usr_stack0, PGSIZE, paddr_stack0, PDE_USER|PDE_WRITABLE);

    void* task_kern_stack = vmm.alloc_page();

    proc->entry = vaddr;
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->user_esp = USTACK_TOP;
    
    trapframe_t* regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    memset(regs, 0, sizeof regs);
    regs->useresp = proc->user_esp;
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = 0x23;
    regs->cs = 0x1b;
    regs->eip = A2I(vaddr);
    regs->eflags = 0x202;
    proc->regs = regs;

    proc->kctx = (kcontext_t*)((char*)proc->regs - sizeof(kcontext_t));
    memset(proc->kctx, 0, sizeof(*proc->kctx));
    proc->kctx->eip = A2I(trap_return);

    proc->ppid = 0;
    proc->pid = next_pid;
    proc->state = TASK_READY;
    kprintf("%s(%d, 0x%x) @0x%x, ustack: 0x%x, kesp: 0x%x\n", 
            __func__, proc->pid, proc, paddr, paddr_stack0, proc->kern_esp);

    return proc;
}

proc_t* create_kthread(const char* name, kthread_t prog)
{
    proc_t* proc = find_free_process();
    if (!proc) return NULL;

    memset(proc, 0, sizeof(*proc));
    proc->state = TASK_CREATE;

    proc->ppid = current->pid;
    proc->pid = next_pid;

    if (!current) current = proc;
    else { 
        proc->next = current->next;
        current->next = proc;
    }
    strncpy(proc->name, name, sizeof proc->name - 1);

    proc->pgdir = vmm.kernel_page_directory();
    kassert(proc->pgdir);

    void* task_kern_stack = vmm.alloc_page();

    proc->entry = (void*)prog;
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->user_esp = 0;
    
    trapframe_t* regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    memset(regs, 0, sizeof regs);
    regs->useresp = proc->user_esp;
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = SEG_KDATA*8;
    regs->cs = SEG_KCODE*8;
    regs->eip = A2I(prog);
    regs->eflags = 0x202;
    proc->regs = regs;

    proc->kctx = (kcontext_t*)((char*)proc->regs - sizeof(kcontext_t));
    memset(proc->kctx, 0, sizeof(*proc->kctx));
    proc->kctx->eip = A2I(trap_return);

    proc->state = TASK_READY;
    kprintf("%s(%d, %s)\n", __func__, proc->pid, proc->name);
    return proc;
}
