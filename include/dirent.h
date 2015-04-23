#ifndef _SOS_DIRENT_H
#define _SOS_DIRENT_H

#include <types.h>
#include <sos/limits.h>

struct dirent {
	long		d_ino;
	off_t		d_off;
	unsigned short	d_reclen;
	char		d_name[NAMELEN+1];
};

#endif
