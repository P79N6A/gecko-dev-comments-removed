





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
AllocateMappedObject(int fd, int *new_fd, size_t offset, size_t length,
                     size_t alignment, size_t header);


void
DeallocateMappedObject(int fd, void *p, size_t length);

} 
} 

#endif 
