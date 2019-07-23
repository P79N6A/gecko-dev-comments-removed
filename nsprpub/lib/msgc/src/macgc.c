




































#include "primpl.h"
#include "MacMemAllocator.h"

void _MD_InitGC() {}

void *_MD_GrowGCHeap(size_t *sizep)
{
	void			*heapPtr = NULL;
	size_t			heapSize = *sizep;
	
	
	
	
	heapPtr = malloc(heapSize);
	
	if (heapPtr == NULL) {		
		FreeMemoryStats		stats;
		
		memtotal(heapSize, &stats);		
		
		if (stats.maxBlockSize < heapSize)
			heapSize = stats.maxBlockSize;
		
		heapPtr = malloc(heapSize);
		
		if (heapPtr == NULL) 			
			heapSize = 0;
	}
	
	*sizep = heapSize;
	return heapPtr;
}


void _MD_FreeGCSegment(void *base, int32 )
{
	free(base);
}
