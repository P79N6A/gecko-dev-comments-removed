




































#ifndef nsNetSegmentUtils_h__
#define nsNetSegmentUtils_h__

#include "nsIOService.h"






static inline nsIMemory *
net_GetSegmentAlloc(PRUint32 segsize)
{
    return (segsize == nsIOService::gDefaultSegmentSize)
                     ? nsIOService::gBufferCache : nsnull;
}




static inline void
net_ResolveSegmentParams(PRUint32 &segsize, PRUint32 &segcount)
{
    if (!segsize)
        segsize = nsIOService::gDefaultSegmentSize;

    if (!segcount)
        segcount = nsIOService::gDefaultSegmentCount;
}

#endif 
