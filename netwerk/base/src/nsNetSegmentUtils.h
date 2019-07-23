




































#ifndef nsNetSegmentUtils_h__
#define nsNetSegmentUtils_h__

#include "necko-config.h"
#include "nsIOService.h"

#ifdef NECKO_SMALL_BUFFERS
#define NET_DEFAULT_SEGMENT_SIZE  2048
#define NET_DEFAULT_SEGMENT_COUNT 4
#else
#define NET_DEFAULT_SEGMENT_SIZE  4096
#define NET_DEFAULT_SEGMENT_COUNT 16
#endif





static inline nsIMemory *
net_GetSegmentAlloc(PRUint32 segsize)
{
    return (segsize == NET_DEFAULT_SEGMENT_SIZE)
                     ? nsIOService::gBufferCache
                     : nsnull;
}




static inline void
net_ResolveSegmentParams(PRUint32 &segsize, PRUint32 &segcount)
{
    if (!segsize)
        segsize = NET_DEFAULT_SEGMENT_SIZE;
    if (!segcount)
        segcount = NET_DEFAULT_SEGMENT_COUNT;
}

#endif 
