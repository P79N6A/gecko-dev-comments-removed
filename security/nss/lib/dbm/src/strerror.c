






























#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strerror.c	8.1 (Berkeley) 6/4/93";
#endif 

#include <string.h>

#ifdef _DLL
#define sys_nerr    (*_sys_nerr_dll)
#endif

#ifndef HAVE_STRERROR
#ifndef _AFXDLL
char *
strerror(num)
	int num;
{
	extern int sys_nerr;
	extern char *sys_errlist[];
#define	UPREFIX	"Unknown error: "
	static char ebuf[40] = UPREFIX;		
	register unsigned int errnum;
	register char *p, *t;
	char tmp[40];

	errnum = num;				
	if (errnum < sys_nerr)
		return(sys_errlist[errnum]);

	
	t = tmp;
	do {
		*t++ = "0123456789"[errnum % 10];
	} while (errnum /= 10);
	for (p = ebuf + sizeof(UPREFIX) - 1;;) {
		*p++ = *--t;
		if (t <= tmp)
			break;
	}
	return(ebuf);
}

#endif
#endif 
