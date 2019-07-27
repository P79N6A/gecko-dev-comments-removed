





#ifndef mozHunspellAllocator_h__
#define mozHunspellAllocator_h__

#include "mozilla/CountingAllocatorBase.h"

class HunspellAllocator : public mozilla::CountingAllocatorBase<HunspellAllocator>
{
};

#endif
