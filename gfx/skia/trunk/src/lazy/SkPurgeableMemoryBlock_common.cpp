






#include "SkPurgeableMemoryBlock.h"

SkPurgeableMemoryBlock* SkPurgeableMemoryBlock::Create(size_t size) {
    SkASSERT(IsSupported());
    if (!IsSupported()) {
        return NULL;
    }
    return SkNEW_ARGS(SkPurgeableMemoryBlock, (size));
}
