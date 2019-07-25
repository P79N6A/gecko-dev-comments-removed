






































#ifndef jsgchunk_h__
#define jsgchunk_h__

#include "jsprvtd.h"
#include "jsutil.h"

namespace js {

const size_t GC_CHUNK_SHIFT = 20;
const size_t GC_CHUNK_SIZE = size_t(1) << GC_CHUNK_SHIFT;
const size_t GC_CHUNK_MASK = GC_CHUNK_SIZE - 1;

JS_FRIEND_API(void *)
AllocGCChunk();

JS_FRIEND_API(void)
FreeGCChunk(void *p);

class GCChunkAllocator {
  public:
    GCChunkAllocator() {}
    
    void *alloc() {
        void *chunk = doAlloc();
        JS_ASSERT(!(reinterpret_cast<jsuword>(chunk) & GC_CHUNK_MASK));
        return chunk;
    }

    void free_(void *chunk) {
        JS_ASSERT(chunk);
        JS_ASSERT(!(reinterpret_cast<jsuword>(chunk) & GC_CHUNK_MASK));
        doFree(chunk);
    }
    
  private:
    virtual void *doAlloc() {
        return AllocGCChunk();
    }
    
    virtual void doFree(void *chunk) {
        FreeGCChunk(chunk);
    }

    
    GCChunkAllocator(const GCChunkAllocator &);
    void operator=(const GCChunkAllocator &);
};

extern GCChunkAllocator defaultGCChunkAllocator;

}

#endif 
