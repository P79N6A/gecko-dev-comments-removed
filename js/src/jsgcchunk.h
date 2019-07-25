






































#ifndef jsgchunk_h__
#define jsgchunk_h__

#include "jsprvtd.h"
#include "jsutil.h"

namespace js {

const size_t GC_CHUNK_SHIFT = 20;
const size_t GC_CHUNK_SIZE = size_t(1) << GC_CHUNK_SHIFT;
const size_t GC_CHUNK_MASK = GC_CHUNK_SIZE - 1;

void *
AllocGCChunk();

void
FreeGCChunk(void *p);

}

#endif 
