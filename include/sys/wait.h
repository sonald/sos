#ifndef _SOS_SYS_WAIT_H
#define _SOS_SYS_WAIT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <types.h>

#define WNOHANG     1
#define WUNTRACED   2

/* A status looks like:
      <code> == 0, child has exited, info is the exit value
      <code> == 1..7e, child has exited, info is the signal number.
      <code> == 7f, child has stopped, info was the signal number.
      <code> == 80, there was a core dump.
*/
   
#define WIFEXITED(w)	(((w) & 0xff) == 0)
#define WIFSIGNALED(w)	(((w) & 0x7f) > 0 && (((w) & 0x7f) < 0x7f))
#define WIFSTOPPED(w)	(((w) & 0xff) == 0x7f)
#define WEXITSTATUS(w)	(((w) >> 8) & 0xff)
#define WTERMSIG(w)	((w) & 0x7f)
#define WSTOPSIG	WEXITSTATUS

#ifdef __cplusplus
};
#endif

#endif
