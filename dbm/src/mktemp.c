






























#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)mktemp.c	8.1 (Berkeley) 6/4/93";
#endif 

#ifdef macintosh
#include <unix.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include "mcom_db.h"

#ifndef _WINDOWS
#include <unistd.h>
#endif

#ifdef _WINDOWS
#include <process.h>
#include "winfile.h"
#endif

static int _gettemp(char *path, register int *doopen, int extraFlags);

int
mkstemp(char *path)
{
#ifdef XP_OS2
	FILE *temp = tmpfile();

	return (temp ? fileno(temp) : -1);
#else
	int fd;

	return (_gettemp(path, &fd, 0) ? fd : -1);
#endif
}

int
mkstempflags(char *path, int extraFlags)
{
	int fd;

	return (_gettemp(path, &fd, extraFlags) ? fd : -1);
}

#ifdef WINCE 
char *
mktemp(char *path)
{
	return(_gettemp(path, (int *)NULL, 0) ? path : (char *)NULL);
}
#endif




static int 
_gettemp(char *path, register int *doopen, int extraFlags)
{    
#if !defined(_WINDOWS) || defined(_WIN32)
	extern int errno;                    
#endif
	register char *start, *trv;
	struct stat sbuf;
	unsigned int pid;

	pid = getpid();
	for (trv = path; *trv; ++trv);		
	while (*--trv == 'X') {
		*trv = (pid % 10) + '0';
		pid /= 10;
	}

	



	for (start = trv + 1;; --trv) {
		char saved;
		if (trv <= path)
			break;
		saved = *trv;
		if (saved == '/' || saved == '\\') {
			int rv;
			*trv = '\0';
			rv = stat(path, &sbuf);
			*trv = saved;
			if (rv)
				return(0);
			if (!S_ISDIR(sbuf.st_mode)) {
				errno = ENOTDIR;
				return(0);
			}
			break;
		}
	}

	for (;;) {
		if (doopen) {
			if ((*doopen =
			    open(path, O_CREAT|O_EXCL|O_RDWR|extraFlags, 0600)) >= 0)
				return(1);
			if (errno != EEXIST)
				return(0);
		}
		else if (stat(path, &sbuf))
			return(errno == ENOENT ? 1 : 0);

		
		for (trv = start;;) {
			if (!*trv)
				return(0);
			if (*trv == 'z')
				*trv++ = 'a';
			else {
				if (isdigit(*trv))
					*trv = 'a';
				else
					++*trv;
				break;
			}
		}
	}
	
}
