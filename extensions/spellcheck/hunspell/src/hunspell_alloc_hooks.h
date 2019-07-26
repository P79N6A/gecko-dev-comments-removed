
































#ifndef alloc_hooks_h__
#define alloc_hooks_h__














#include "mozilla/mozalloc.h"
#include "mozHunspellAllocator.h"

#define malloc(size) HunspellAllocator::CountingMalloc(size)
#define calloc(count, size) HunspellAllocator::CountingCalloc(count, size)
#define free(ptr) HunspellAllocator::CountingFree(ptr)
#define realloc(ptr, size) HunspellAllocator::CountingRealloc(ptr, size)

#endif
