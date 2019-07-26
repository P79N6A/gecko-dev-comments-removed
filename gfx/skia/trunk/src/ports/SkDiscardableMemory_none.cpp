






#include "SkDiscardableMemoryPool.h"
#include "SkTypes.h"

SkDiscardableMemory* SkDiscardableMemory::Create(size_t bytes) {
    return SkGetGlobalDiscardableMemoryPool()->create(bytes);
}
