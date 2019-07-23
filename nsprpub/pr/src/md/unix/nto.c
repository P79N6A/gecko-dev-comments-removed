




































#include "primpl.h"

#include <setjmp.h>


int socketpair (int foo, int foo2, int foo3, int sv[2])
{
	printf("error in socketpair\n");
	exit (-1);
}

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#ifndef _PR_PTHREADS
    if (isCurrent) {
    (void) setjmp(CONTEXT(t));
    }

    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}
