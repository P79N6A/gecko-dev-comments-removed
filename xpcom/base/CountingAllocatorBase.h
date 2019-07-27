





#ifndef CountingAllocatorBase_h
#define CountingAllocatorBase_h

#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"
#include "nsIMemoryReporter.h"

namespace mozilla {























template<typename T>
class CountingAllocatorBase
{
public:
  CountingAllocatorBase()
  {
#ifdef DEBUG
    
    
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
  }

  static size_t
  MemoryAllocated()
  {
    return sAmount;
  }

  static void*
  CountingMalloc(size_t size)
  {
    void* p = malloc(size);
    sAmount += MallocSizeOfOnAlloc(p);
    return p;
  }

  static void*
  CountingCalloc(size_t nmemb, size_t size)
  {
    void* p = calloc(nmemb, size);
    sAmount += MallocSizeOfOnAlloc(p);
    return p;
  }

  static void*
  CountingRealloc(void* p, size_t size)
  {
    size_t oldsize = MallocSizeOfOnFree(p);
    void *pnew = realloc(p, size);
    if (pnew) {
      size_t newsize = MallocSizeOfOnAlloc(pnew);
      sAmount += newsize - oldsize;
    } else if (size == 0) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      sAmount -= oldsize;
    } else {
      
    }
    return pnew;
  }

  
  
  
  static void*
  CountingFreeingRealloc(void* p, size_t size)
  {
    if (size == 0) {
      CountingFree(p);
      return nullptr;
    }
    return CountingRealloc(p, size);
  }

  static void
  CountingFree(void* p)
  {
    sAmount -= MallocSizeOfOnFree(p);
    free(p);
  }

private:
  
  
  static Atomic<size_t> sAmount;

  MOZ_DEFINE_MALLOC_SIZE_OF_ON_ALLOC(MallocSizeOfOnAlloc)
  MOZ_DEFINE_MALLOC_SIZE_OF_ON_FREE(MallocSizeOfOnFree)
};

} 

#endif
