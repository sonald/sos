#include "task.h"

task_t tasks[MAXPROCS];
proc_t proctable[MAXPROCS];

int proc_count = 0; // allocated procs
proc_t* current_proc = NULL;

proc_t* create_proc(void* entry, const char* name)
{
    kassert(proc_count < MAXPROCS);

    proc_t* proc = &proctable[proc_count++];
    memset(proc, 0, sizeof(*proc));
    strcpy(proc->name, name);

    if (!current_proc) current_proc = proc;
    else current_proc->next = proc;

    VirtualMemoryManager* vmm = VirtualMemoryManager::get();
    page_directory_t* pdir = vmm->create_address_space();
    vmm->switch_page_directory(pdir);

    proc->pgdir = pdir;
    void* vaddr = (void*)0x08000000; 
    u32 paddr = v2p(vmm->alloc_page());
    vmm->map_pages(pdir, vaddr, PGSIZE, paddr, PDE_USER|PDE_WRITABLE);
    memcpy(vaddr, entry, PGSIZE);

    void* task_usr_stack0 = (void*)0xB0000000; 
    u32 paddr_stack0 = v2p(vmm->alloc_page());
    vmm->map_pages(pdir, task_usr_stack0, PGSIZE, paddr_stack0, PDE_USER|PDE_WRITABLE);

    void* task_kern_stack = vmm->alloc_page();

    proc->entry = vaddr;
    proc->kern_esp = A2I(task_kern_stack) + PGSIZE;
    proc->user_esp = A2I(task_usr_stack0) + PGSIZE;
    
    registers_t* regs = (registers_t*)((char*)proc->kern_esp - sizeof(registers_t));
    regs->useresp = proc->user_esp;
    regs->esp = proc->kern_esp;
    regs->ss = regs->es = regs->ds = regs->fs = regs->gs = 0x23;
    regs->cs = 0x1b;
    regs->eip = A2I(vaddr);
    regs->eflags = 0x202;
    proc->regs = regs;

    proc->pid = proc_count;
    kprintf("alloc task(%d) @0x%x, ustack: 0x%x, kesp: 0x%x\n", 
            proc->pid, paddr, paddr_stack0, proc->kern_esp);

    return proc;
}
