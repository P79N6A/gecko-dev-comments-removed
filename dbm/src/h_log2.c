

































#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)hash_log2.c	8.2 (Berkeley) 5/31/94";
#endif 

#include <stdio.h>
#ifndef macintosh
#include <sys/types.h>
#endif
#include "mcom_db.h"

uint32 __log2(uint32 num)
{
	register uint32 i, limit;

	limit = 1;
	for (i = 0; limit < num; limit = limit << 1, i++) {}
	return (i);
}
