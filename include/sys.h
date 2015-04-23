#ifndef _SOS_SYS_H
#define _SOS_SYS_H

#include <types.h>

extern int sys_getppid();
extern int sys_fork();
extern int sys_sleep(int);
extern int sys_getpid();
extern int sys_wait();
extern int sys_exit();
extern int sys_execve(const char *path, char *const argv[],
        char *const envp[]);
extern int sys_open(const char *path, int flags, int mode);
extern int sys_close(int);
extern int sys_mmap(struct file *, struct vm_area_struct *);
extern int sys_write(int fd, const void *buf, size_t nbyte);
extern int sys_read(int fd, void *buf, size_t nbyte);
extern int sys_readdir(unsigned int fd, struct dirent *dirp,
        unsigned int count);
extern int sys_mount(const char *, const char *,
        const char *, unsigned long, const void *);
extern int sys_unmount(const char *target);

#endif
