





#ifndef gc_Memory_h
#define gc_Memory_h

#include <stddef.h>

struct JSRuntime;

namespace js {
namespace gc {



void
InitMemorySubsystem(JSRuntime *rt);


void *
MapAlignedPages(JSRuntime *rt, size_t size, size_t alignment);

void
UnmapPages(JSRuntime *rt, void *p, size_t size);



bool
MarkPagesUnused(JSRuntime *rt, void *p, size_t size);




bool
MarkPagesInUse(JSRuntime *rt, void *p, size_t size);


size_t
GetPageFaultCount();



void *
AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment);


void
DeallocateMappedContent(void *p, size_t length);

} 
} 

#endif 
