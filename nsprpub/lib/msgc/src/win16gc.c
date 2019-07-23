




































#if defined(WIN16)
#include <windows.h>
#endif
#include "prtypes.h"
#include <stdlib.h>

#define MAX_SEGMENT_SIZE (65536l - 4096l)








void _MD_InitGC(void) {}

extern void *
_MD_GrowGCHeap(PRUint32 *sizep)
{
    void *addr;

    if( *sizep > MAX_SEGMENT_SIZE ) {
        *sizep = MAX_SEGMENT_SIZE;
    }

    addr = malloc((size_t)*sizep);
    return addr;
}

HINSTANCE _pr_hInstance;

int CALLBACK LibMain( HINSTANCE hInst, WORD wDataSeg, 
                      WORD cbHeapSize, LPSTR lpszCmdLine )
{
    _pr_hInstance = hInst;
    return TRUE;
}


