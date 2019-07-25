






































#ifndef jsgchunk_h__
#define jsgchunk_h__

#include "jsapi.h"
#include "jsprvtd.h"
#include "jsutil.h"

namespace js {

#if defined(WINCE) && !defined(MOZ_MEMORY_WINCE6)
const size_t GC_CHUNK_SHIFT = 21;
#else
const size_t GC_CHUNK_SHIFT = 20;
#endif

const size_t GC_CHUNK_SIZE = size_t(1) << GC_CHUNK_SHIFT;
const size_t GC_CHUNK_MASK = GC_CHUNK_SIZE - 1;

#if defined(XP_WIN) && defined(_M_X64)
bool InitNtAllocAPIs();
#endif

void *
AllocGCChunk();

void
FreeGCChunk(void *p);

}

#endif 
