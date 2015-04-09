#include <fcntl.h>
#include <unistd.h>

_syscall0(pid_t, getpid);
_syscall0(pid_t, fork);
_syscall3(int, write, int, fd, const void*, buf, size_t, nbyte);
/*_syscall2(int, creat, const char*, filename, mode_t, mode)*/
/*_syscall2(int, fcntl, int, fildes, int, cmd);*/

_syscall3(int, open, const char*, path, int, flags, int, mode);
_syscall1(int, close, int, fd);
_syscall3(int, read, int, fildes, void*, buf, size_t, nbyte);
_syscall3(int, exec, const char*, path, char *const*, argv, char *const*, envp);

int execve(const char *path, char *const argv[], char *const envp[])
{
    return exec(path, argv, envp);
}
