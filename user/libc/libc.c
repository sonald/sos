#include <fcntl.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

_syscall0(pid_t, getpid);
_syscall0(pid_t, fork);
_syscall3(int, write, int, fd, const void*, buf, size_t, nbyte);
/*_syscall2(int, creat, const char*, filename, mode_t, mode)*/
/*_syscall2(int, fcntl, int, fildes, int, cmd);*/

_syscall3(int, open, const char*, path, int, flags, int, mode);
_syscall1(int, close, int, fd);
_syscall3(int, read, int, fildes, void*, buf, size_t, nbyte);
_syscall3(int, exec, const char*, path, char *const*, argv, char *const*, envp);
_syscall1(int, sleep, int, millisecs);
_syscall0(int, exit);
_syscall0(int, wait);
_syscall3(int, readdir, unsigned int, fd, struct dirent *, dirp, unsigned int, count);
_syscall1(int, dup, int, fd);
_syscall2(int, dup2, int, fd, int, fd2);
_syscall1(int, pipe, int*, fds); // int fd[2]
_syscall0(int, kdump); 
_syscall1(void*, sbrk, int, inc);
_syscall3(off_t, lseek, int, fd, off_t, offset, int, whence);

int execve(const char *path, char *const argv[], char *const envp[])
{
    return exec(path, argv, envp);
}

extern int main(int argc, char* argv[]);

extern void _init();
extern void _fini();
extern void __cxa_finalize(void * d);

void _start(int argc, char* argv[])
{
    _init();
    main(argc, argv);
    _fini();
    __cxa_finalize(NULL);
    exit();
}

#ifdef __cplusplus
}
#endif

