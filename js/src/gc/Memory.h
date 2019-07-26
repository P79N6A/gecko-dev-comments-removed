





#ifndef gc_Memory_h
#define gc_Memory_h

#include <stddef.h>
#include "jsgc.h"

namespace js {
namespace gc {



void InitMemorySubsystem();


void *MapAlignedPages(size_t size, size_t alignment);
void UnmapPages(void *p, size_t size);



bool MarkPagesUnused(void *p, size_t size);




bool MarkPagesInUse(void *p, size_t size);


size_t GetPageFaultCount();

} 
} 

#endif 
