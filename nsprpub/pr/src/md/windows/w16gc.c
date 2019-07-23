




































#include "primpl.h"
#include <sys/timeb.h>

PRWord *
_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np) 
{
    if (isCurrent) 
    {
        _MD_SAVE_CONTEXT(t);
    }
    





    *np = 0;

    return (PRWord *) CONTEXT(t);
}

#if 0
#ifndef SPORT_MODEL

#define MAX_SEGMENT_SIZE (65536l - 4096l)








extern void *
_MD_GrowGCHeap(uint32 *sizep)
{
    void *addr;

    if( *sizep > MAX_SEGMENT_SIZE ) {
        *sizep = MAX_SEGMENT_SIZE;
    }

    addr = malloc((size_t)*sizep);
    return addr;
}

#endif 
#endif 

