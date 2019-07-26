





#ifndef mozHunspellAllocator_h__
#define mozHunspellAllocator_h__

#include "nsIMemoryReporter.h"

class HunspellAllocator : public mozilla::CountingAllocatorBase<HunspellAllocator>
{
};

#endif
