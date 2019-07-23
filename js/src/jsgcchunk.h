






































#ifndef jsgchunk_h__
#define jsgchunk_h__

#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsutil.h"

namespace js {

#if defined(WINCE) && !defined(MOZ_MEMORY_WINCE6)
const size_t GC_CHUNK_SHIFT = 21;
#else
const size_t GC_CHUNK_SHIFT = 20;
#endif

const size_t GC_CHUNK_SIZE = size_t(1) << GC_CHUNK_SHIFT;
const size_t GC_CHUNK_MASK = GC_CHUNK_SIZE - 1;

void *
AllocGCChunk();

void
FreeGCChunk(void *p);

}

#endif 
