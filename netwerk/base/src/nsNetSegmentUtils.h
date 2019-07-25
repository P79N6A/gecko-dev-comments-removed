



#ifndef nsNetSegmentUtils_h__
#define nsNetSegmentUtils_h__

#include "nsIOService.h"




static inline void
net_ResolveSegmentParams(uint32_t &segsize, uint32_t &segcount)
{
    if (!segsize)
        segsize = nsIOService::gDefaultSegmentSize;

    if (!segcount)
        segcount = nsIOService::gDefaultSegmentCount;
}

#endif 
