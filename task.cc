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

    void* task_usr_stack0 = (void*)0x08100000; 
    u32 paddr_stack0 = v2p(vmm->alloc_page());
    vmm->map_pages(pdir, task_usr_stack0, PGSIZE, paddr_stack0, PDE_USER|PDE_WRITABLE);

    void* task_kern_stack = vmm->alloc_page();

    proc->entry = vaddr;
    proc->regs.useresp = (u32)task_usr_stack0 + PGSIZE;
    proc->regs.esp = (u32)task_kern_stack;
    registers_t& regs = proc->regs;
    regs.ss = regs.ds = regs.fs = regs.gs = 0x23;
    regs.cs = 0x1b;
    regs.eip = (u32)vaddr;
    regs.eflags = 0x1200;
    proc->pid = proc_count;
    kprintf("alloc task: pid %d, eip 0x%x, stack: 0x%x\n", 
            proc->pid, paddr, paddr_stack0);
    //vmm->dump_page_directory(vmm->current_directory());

    return proc;
}
