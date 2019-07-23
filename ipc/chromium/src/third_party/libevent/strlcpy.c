




























#if defined(LIBC_SCCS) && !defined(lint)
static char *rcsid = "$OpenBSD: strlcpy.c,v 1.5 2001/05/13 15:40:16 deraadt Exp $";
#endif 

#include <sys/types.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif 

#ifndef HAVE_STRLCPY
#include "strlcpy-internal.h"






size_t
_event_strlcpy(dst, src, siz)
	char *dst;
	const char *src;
	size_t siz;
{
	register char *d = dst;
	register const char *s = src;
	register size_t n = siz;

	
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	
	if (n == 0) {
		if (siz != 0)
			*d = '\0';		
		while (*s++)
			;
	}

	return(s - src - 1);	
}
#endif
