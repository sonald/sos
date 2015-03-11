#include "task.h"
#include "string.h"
#include "vm.h"
#include "x86.h"
#include "errno.h"
#include "elf.h"
#include "spinlock.h"
#include "sched.h"
#include "timer.h"

extern "C" void trap_return();

Spinlock tasklock("task");

// this contains not-only user mode process and also kernel-threads
proc_t tasks[MAXPROCS];

proc_t* current_proc = NULL;
static int next_pid = 0;

void sleep(Spinlock* lk, void* chan)
{
    current_proc->channel = chan;
    current_proc->state = TASK_SLEEP;
    kprintf(" (%s:sleep) ", current_proc->name);
    lk->release();
    scheduler(current_proc->regs); 
    lk->lock();
    kprintf(" (%s:awaked) ");
}

void wakeup(void* chan)
{
    if ((readflags() & FL_IF))
        tasklock.lock();
    auto* tsk = &tasks[0];
    while (tsk) {
        if (tsk->state == TASK_SLEEP && tsk->channel == chan) {
            tsk->state = TASK_READY;
            tsk->channel = NULL;
        }
        tsk = tsk->next;
    }
    tasklock.release();
}

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

int sys_sleep()
{
    kassert(current_proc);
    current_proc->state = TASK_SLEEP;
    current_proc->need_resched = true;
    return 0;
}

int sys_fork() 
{
    proc_t* proc = find_free_process();
    if (!proc) return -1;

    *proc = *current_proc;
    proc->state = TASK_CREATE;

    proc->ppid = current_proc->pid;
    proc->pid = next_pid;

    if (!current_proc) current_proc = proc;
    else { 
        proc->next = current_proc->next;
        current_proc->next = proc;
    }

    proc->pgdir = vmm.copy_page_directory(current_proc->pgdir);

    void* task_kern_stack = vmm.alloc_page();
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    *(proc->regs) = *(current_proc->regs);
    proc->regs->eax = 0; // return of child process

    proc->kctx = (kcontext_t*)((char*)proc->regs - sizeof(kcontext_t));
    *(proc->kctx) = *(current_proc->kctx);
    proc->kctx->eip = A2I(trap_return);

    // setup user stack (only one-page, will and should expand at PageFault)
    void* task_usr_stack0 = (void*)USTACK; 
    void* new_stack = vmm.alloc_page();
    vmm.map_pages(proc->pgdir, task_usr_stack0, PGSIZE,
            v2p(new_stack), PDE_USER|PDE_WRITABLE);

    // user stack copied
    page_t* pte = vmm.walk(current_proc->pgdir, task_usr_stack0, false);
    kassert(pte && pte->present && pte->user);
    void* old_stack = p2v(pte->frame * PGSIZE);
    memcpy(new_stack, old_stack, PGSIZE);

    kassert(proc->regs->useresp == current_proc->regs->useresp);

    kprintf("fork %d -> %d\n", current_proc->pid, next_pid);
    //kprintf("RET: uesp: 0x%x, eip: 0x%x\n", proc->regs->useresp, proc->regs->eip);
    proc->state = TASK_READY;
    return next_pid;
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
    memcpy(tmp, code, size);

    return 0;
}

int sys_execve(const char *path, char *const argv[], char *const envp[])
{
    (void)argv;
    (void)envp;

    kprintf("path(0x%x): %s\n", path, path);
    FileSystem* ramfs = devices[RAMFS_MAJOR];

    inode_t* ramfs_root = ramfs->root();
    inode_t* ip = ramfs->dir_lookup(ramfs_root, path);
    kassert(ip != NULL);

    char* buf = new char[ip->size];
    int len = ramfs->read(ip, buf, ip->size, 0);
    if (len < 0) {
        kprintf("load %s failed\n", path);
        return -ENOENT;
    }

    elf_header_t* elf = (elf_header_t*)buf;
    if (elf->e_magic != ELF_MAGIC) {
        kprintf("invalid elf file\n");
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

        load_proc(current_proc, prog, ph[i].p_memsz, PDE_USER);
        break;
    }

    strcpy(current_proc->name, path);
    kprintf("execv task(%d, %s)\n", current_proc->pid, path);

    current_proc->entry = (void*)elf->e_entry;
    
    trapframe_t* regs = current_proc->regs;
    memset(regs, 0, sizeof regs);
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = 0x23;
    regs->cs = 0x1b;
    regs->eip = elf->e_entry;
    regs->eflags = 0x202;
    regs->useresp = current_proc->user_esp;
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

    kassert(current_proc == NULL);
    current_proc = proc;

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

    proc->ppid = current_proc->pid;
    proc->pid = next_pid;

    if (!current_proc) current_proc = proc;
    else { 
        proc->next = current_proc->next;
        current_proc->next = proc;
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

