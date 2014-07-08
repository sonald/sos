#include "task.h"
#include "vm.h"
#include "x86.h"

proc_t proctable[MAXPROCS];

int proc_count = 0; // allocated procs
proc_t* current_proc = NULL;

proc_t* create_proc(void* entry, void* prog, size_t size, const char* name)
{
    cli();
    kassert(proc_count < MAXPROCS);

    proc_t* proc = &proctable[proc_count++];
    memset(proc, 0, sizeof(*proc));
    strcpy(proc->name, name);

    if (!current_proc) current_proc = proc;
    else { 
        proc->next = current_proc->next;
        current_proc->next = proc;
    }

    page_directory_t* pdir = vmm.create_address_space();
    //vmm.switch_page_directory(pdir);

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
    proc->user_esp = A2I(task_usr_stack0) + PGSIZE;
    
    trapframe_t* regs = (trapframe_t*)((char*)proc->kern_esp - sizeof(trapframe_t));
    regs->useresp = proc->user_esp;
    regs->esp = proc->kern_esp;
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = 0x23;
    regs->cs = 0x1b;
    regs->eip = A2I(entry);
    regs->eflags = 0x202;
    proc->regs = regs;

    proc->pid = proc_count;
    kprintf("alloc task(%d) @0x%x, ustack: 0x%x, kesp: 0x%x\n", 
            proc->pid, paddr, paddr_stack0, proc->kern_esp);

    sti();
    return proc;
}
