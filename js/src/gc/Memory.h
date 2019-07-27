





#ifndef gc_Memory_h
#define gc_Memory_h

#include <stddef.h>

namespace js {
namespace gc {



void InitMemorySubsystem();

size_t SystemPageSize();


void* MapAlignedPages(size_t size, size_t alignment);
void UnmapPages(void* p, size_t size);



bool MarkPagesUnused(void* p, size_t size);




bool MarkPagesInUse(void* p, size_t size);


size_t GetPageFaultCount();



void* AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment);


void DeallocateMappedContent(void* p, size_t length);

void* TestMapAlignedPagesLastDitch(size_t size, size_t alignment);

} 
} 

#endif 
