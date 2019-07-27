





#ifndef MOZILLA_RESOURCEQUEUE_H_
#define MOZILLA_RESOURCEQUEUE_H_

#include <algorithm>
#include "nsDeque.h"
#include "MediaData.h"
#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetSourceBufferResourceLog();


#ifdef _MSC_VER
#define __func__ __FUNCTION__
#endif

#define SBR_DEBUG(arg, ...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG, ("ResourceQueue(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#define SBR_DEBUGV(arg, ...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG+1, ("ResourceQueue(%p)::%s: " arg, this, __func__, ##__VA_ARGS__))
#else
#define SBR_DEBUG(...)
#define SBR_DEBUGV(...)
#endif

namespace mozilla {












struct ResourceItem {
  explicit ResourceItem(LargeDataBuffer* aData)
  : mData(aData)
  {
  }

  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
    
    size_t size = aMallocSizeOf(this);

    
    size += mData->SizeOfExcludingThis(aMallocSizeOf);

    return size;
  }

  nsRefPtr<LargeDataBuffer> mData;
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
      uint32_t bytes = std::min(aCount, uint32_t(item->mData->Length() - offset));
      if (bytes != 0) {
        memcpy(aDest, &(*item->mData)[offset], bytes);
        offset = 0;
        aCount -= bytes;
        aDest += bytes;
      }
    }
  }

  void AppendItem(LargeDataBuffer* aData) {
    mLogicalLength += aData->Length();
    Push(new ResourceItem(aData));
  }

  
  
  uint32_t Evict(uint64_t aOffset, uint32_t aSizeToEvict) {
    SBR_DEBUG("Evict(aOffset=%llu, aSizeToEvict=%u)",
              aOffset, aSizeToEvict);
    return EvictBefore(std::min(aOffset, mOffset + (uint64_t)aSizeToEvict));
  }

  uint32_t EvictBefore(uint64_t aOffset) {
    SBR_DEBUG("EvictBefore(%llu)", aOffset);
    uint32_t evicted = 0;
    while (ResourceItem* item = ResourceAt(0)) {
      SBR_DEBUG("item=%p length=%d offset=%llu",
                item, item->mData->Length(), mOffset);
      if (item->mData->Length() + mOffset >= aOffset) {
        if (aOffset <= mOffset) {
          break;
        }
        uint32_t offset = aOffset - mOffset;
        mOffset += offset;
        evicted += offset;
        nsRefPtr<LargeDataBuffer> data = new LargeDataBuffer;
        data->AppendElements(item->mData->Elements() + offset,
                             item->mData->Length() - offset);
        item->mData = data;
        break;
      }
      mOffset += item->mData->Length();
      evicted += item->mData->Length();
      delete PopFront();
    }
    return evicted;
  }

  uint32_t EvictAll() {
    SBR_DEBUG("EvictAll()");
    uint32_t evicted = 0;
    while (ResourceItem* item = ResourceAt(0)) {
      SBR_DEBUG("item=%p length=%d offset=%llu",
                item, item->mData->Length(), mOffset);
      mOffset += item->mData->Length();
      evicted += item->mData->Length();
      delete PopFront();
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

#if defined(DEBUG)
  void Dump(const char* aPath) {
    for (uint32_t i = 0; i < uint32_t(GetSize()); ++i) {
      ResourceItem* item = ResourceAt(i);

      char buf[255];
      PR_snprintf(buf, sizeof(buf), "%s/%08u.bin", aPath, i);
      FILE* fp = fopen(buf, "wb");
      if (!fp) {
        return;
      }
      fwrite(item->mData->Elements(), item->mData->Length(), 1, fp);
      fclose(fp);
    }
  }
#endif

private:
  ResourceItem* ResourceAt(uint32_t aIndex) const {
    return static_cast<ResourceItem*>(ObjectAt(aIndex));
  }

  
  
  
  
  
  uint32_t GetAtOffset(uint64_t aOffset, uint32_t *aResourceOffset) {
    MOZ_RELEASE_ASSERT(aOffset >= mOffset);
    uint64_t offset = mOffset;
    for (uint32_t i = 0; i < uint32_t(GetSize()); ++i) {
      ResourceItem* item = ResourceAt(i);
      
      
      if (item->mData->Length() + offset > aOffset) {
        if (aResourceOffset) {
          *aResourceOffset = aOffset - offset;
        }
        return i;
      }
      offset += item->mData->Length();
    }
    return GetSize();
  }

  ResourceItem* PopFront() {
    return static_cast<ResourceItem*>(nsDeque::PopFront());
  }

  
  uint64_t mLogicalLength;

  
  uint64_t mOffset;
};

#undef SBR_DEBUG
#undef SBR_DEBUGV

} 
#endif 
