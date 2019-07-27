





#ifndef MOZILLA_RESOURCEQUEUE_H_
#define MOZILLA_RESOURCEQUEUE_H_

#include <algorithm>
#include "nsDeque.h"
#include "nsTArray.h"
#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetSourceBufferResourceLog();

#define SBR_DEBUG(...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#define SBR_DEBUGV(...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG+1, (__VA_ARGS__))
#else
#define SBR_DEBUG(...)
#define SBR_DEBUGV(...)
#endif

namespace mozilla {












struct ResourceItem {
  ResourceItem(const uint8_t* aData, uint32_t aSize) {
    mData.AppendElements(aData, aSize);
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    
    size_t size = aMallocSizeOf(this);

    
    size += mData.SizeOfExcludingThis(aMallocSizeOf);

    return size;
  }

  nsTArray<uint8_t> mData;
};

class ResourceQueueDeallocator : public nsDequeFunctor {
  virtual void* operator() (void* aObject) {
    delete static_cast<ResourceItem*>(aObject);
    return nullptr;
  }
};

class ResourceQueue : private nsDeque {
public:
  ResourceQueue()
    : nsDeque(new ResourceQueueDeallocator())
    , mLogicalLength(0)
    , mOffset(0)
  {
  }

  
  uint64_t GetOffset() {
    return mOffset;
  }

  
  
  uint64_t GetLength() {
    return mLogicalLength;
  }

  
  void CopyData(uint64_t aOffset, uint32_t aCount, char* aDest) {
    uint32_t offset = 0;
    uint32_t start = GetAtOffset(aOffset, &offset);
    uint32_t end = std::min(GetAtOffset(aOffset + aCount, nullptr) + 1, uint32_t(GetSize()));
    for (uint32_t i = start; i < end; ++i) {
      ResourceItem* item = ResourceAt(i);
      uint32_t bytes = std::min(aCount, uint32_t(item->mData.Length() - offset));
      if (bytes != 0) {
        memcpy(aDest, &item->mData[offset], bytes);
        offset = 0;
        aCount -= bytes;
        aDest += bytes;
      }
    }
  }

  void AppendItem(const uint8_t* aData, uint32_t aLength) {
    mLogicalLength += aLength;
    Push(new ResourceItem(aData, aLength));
  }

  
  
  
  bool Evict(uint64_t aOffset, uint32_t aThreshold) {
    bool evicted = false;
    while (GetLength() - mOffset > aThreshold) {
      ResourceItem* item = ResourceAt(0);
      if (item->mData.Length() + mOffset > aOffset) {
        break;
      }
      mOffset += item->mData.Length();
      SBR_DEBUGV("ResourceQueue(%p)::Evict(%llu, %u) removed chunk length=%u",
                 this, aOffset, aThreshold, item->mData.Length());
      delete PopFront();
      evicted = true;
    }
    return evicted;
  }

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
    
    size_t size = nsDeque::SizeOfExcludingThis(aMallocSizeOf);

    
    for (uint32_t i = 0; i < uint32_t(GetSize()); ++i) {
      const ResourceItem* item = ResourceAt(i);
      size += item->SizeOfIncludingThis(aMallocSizeOf);
    }

    return size;
  }

private:
  ResourceItem* ResourceAt(uint32_t aIndex) const {
    return static_cast<ResourceItem*>(ObjectAt(aIndex));
  }

  
  
  
  
  
  uint32_t GetAtOffset(uint64_t aOffset, uint32_t *aResourceOffset) {
    MOZ_ASSERT(aOffset >= mOffset);
    uint64_t offset = mOffset;
    for (uint32_t i = 0; i < uint32_t(GetSize()); ++i) {
      ResourceItem* item = ResourceAt(i);
      
      
      if (item->mData.Length() + offset > aOffset) {
        if (aResourceOffset) {
          *aResourceOffset = aOffset - offset;
        }
        return i;
      }
      offset += item->mData.Length();
    }
    return GetSize();
  }

  ResourceItem* PopFront() {
    return static_cast<ResourceItem*>(nsDeque::PopFront());
  }

  
  uint64_t mLogicalLength;

  
  uint64_t mOffset;
};


} 
#endif 
