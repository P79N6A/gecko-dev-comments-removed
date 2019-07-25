






































#ifndef jsgchunk_h__
#define jsgchunk_h__

#include "jsprvtd.h"
#include "jsutil.h"

namespace js {
namespace gc {

const size_t ChunkShift = 20;
const size_t ChunkSize = size_t(1) << ChunkShift;
const size_t ChunkMask = ChunkSize - 1;

void *
AllocChunk();

void
FreeChunk(void *p);

bool
CommitMemory(void *addr, size_t size);

bool
DecommitMemory(void *addr, size_t size);

} 
} 

#endif 
