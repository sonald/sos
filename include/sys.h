#ifndef _SOS_SYS_H
#define _SOS_SYS_H

#include <types.h>
#include <stat.h>
#include <sos/signal.h>

extern int sys_getppid();
extern int sys_fork();
extern int sys_sleep(int);
extern int sys_getpid();
extern int sys_wait(int*);
extern int sys_waitpid(pid_t pid, int *status, int options);
extern int sys_exit(int);
extern int sys_execve(const char *path, char *const argv[], char *const envp[]);
extern int sys_open(const char *path, int flags, int mode);
extern int sys_close(int);
extern int sys_mmap(struct file *, struct vm_area_struct *);
extern int sys_write(int fd, const void *buf, size_t nbyte);
extern int sys_read(int fd, void *buf, size_t nbyte);
extern int sys_readdir(unsigned int fd, struct dirent *dirp, unsigned int count);
extern int sys_mount(const char *, const char *, const char *, unsigned long, const void *);
extern int sys_unmount(const char *target);
extern int sys_dup(int);
extern int sys_dup2(int fd, int fd2);
extern int sys_pipe(int fd[2]);
extern int sys_kdump(); // for debug
extern uint32_t sys_sbrk(int inc);
extern off_t sys_lseek(int fd, off_t offset, int whence);
extern int sys_stat(const char *pathname, struct stat *buf);
extern int sys_fstat(int fd, struct stat *buf);
extern int sys_lstat(const char *pathname, struct stat *buf);
extern int sys_kill(pid_t pid, int sig);
extern int sys_signal(int signum, unsigned long handler);
extern int sys_sigaction(int signum, const struct sigaction * act,
        struct sigaction * oldact);
extern int sys_sigpending(sigset_t *set);
extern int sys_sigprocmask(int how, sigset_t *set, sigset_t *oldset);
extern int sys_sigsuspend(sigset_t *sigmask);
//right now, I should take care of it specially cause there is 
//no way I can pass a trap frame to a syscall easily.
extern int sys_sigreturn(uint32_t);
extern int sys_chdir(const char *path);
extern int sys_fchdir(int fd);
extern char* sys_getcwd(char *buf, size_t size);

#endif
