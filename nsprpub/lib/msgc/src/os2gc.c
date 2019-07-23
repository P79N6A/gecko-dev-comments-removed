








































#include "prlog.h"

#include <stdlib.h>


#define MAX_SEGMENT_SIZE    (65536L - 4096L)







void _MD_InitGC() {}

void *_MD_GrowGCHeap(PRUint32 *sizep)
{
    void *addr;

    if ( *sizep > MAX_SEGMENT_SIZE )
    {
        *sizep = MAX_SEGMENT_SIZE;
    }

    addr = malloc((size_t)*sizep);
    return addr;
}


PRBool _MD_ExtendGCHeap(char *base, PRInt32 oldSize, PRInt32 newSize) {
  
  return PR_FALSE;
}


void _MD_FreeGCSegment(void *base, PRInt32 len)
{
   if (base)
   {
       free(base);
   }
}
