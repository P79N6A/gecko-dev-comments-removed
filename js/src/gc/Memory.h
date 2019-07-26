





#ifndef gc_Memory_h
#define gc_Memory_h

#include <stddef.h>

struct JSRuntime;

namespace js {
namespace gc {

class SystemPageAllocator
{
  public:
    
    
    SystemPageAllocator();

    size_t systemPageSize() { return pageSize; }
    size_t systemAllocGranularity() { return allocGranularity; }

    
    void *mapAlignedPages(size_t size, size_t alignment);
    void unmapPages(void *p, size_t size);

    
    
    bool markPagesUnused(void *p, size_t size);

    
    
    
    bool markPagesInUse(void *p, size_t size);

    
    static size_t GetPageFaultCount();

    
    
    static void *AllocateMappedContent(int fd, size_t offset, size_t length, size_t alignment);

    
    static void DeallocateMappedContent(void *p, size_t length);

  private:
    bool decommitEnabled();
    void *mapAlignedPagesSlow(size_t size, size_t alignment);

    
    
    size_t              pageSize;

    
    size_t              allocGranularity;
};

} 
} 

#endif 
