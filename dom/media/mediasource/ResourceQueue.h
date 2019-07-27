





#ifndef MOZILLA_RESOURCEQUEUE_H_
#define MOZILLA_RESOURCEQUEUE_H_

#include "nsDeque.h"
#include "MediaData.h"

namespace mozilla {












struct ResourceItem {
  explicit ResourceItem(MediaLargeByteBuffer* aData);
  size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const;
  nsRefPtr<MediaLargeByteBuffer> mData;
};

class ResourceQueue : private nsDeque {
public:
  ResourceQueue();

  
  uint64_t GetOffset();

  
  
  uint64_t GetLength();

  
  void CopyData(uint64_t aOffset, uint32_t aCount, char* aDest);

  void AppendItem(MediaLargeByteBuffer* aData);

  
  
  uint32_t Evict(uint64_t aOffset, uint32_t aSizeToEvict);

  uint32_t EvictBefore(uint64_t aOffset);

  uint32_t EvictAll();

  size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const;

#if defined(DEBUG)
  void Dump(const char* aPath);
#endif

private:
  ResourceItem* ResourceAt(uint32_t aIndex) const;

  
  
  
  
  
  uint32_t GetAtOffset(uint64_t aOffset, uint32_t *aResourceOffset);

  ResourceItem* PopFront();

  
  uint64_t mLogicalLength;

  
  uint64_t mOffset;
};

} 
#endif 
