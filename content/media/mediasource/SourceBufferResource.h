





#ifndef MOZILLA_SOURCEBUFFERRESOURCE_H_
#define MOZILLA_SOURCEBUFFERRESOURCE_H_

#include <algorithm>
#include "MediaCache.h"
#include "MediaResource.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsIPrincipal.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "nsDeque.h"
#include "nscore.h"

class nsIStreamListener;

namespace mozilla {

class MediaDecoder;

namespace dom {

class SourceBuffer;

}  

class SourceBufferResource MOZ_FINAL : public MediaResource
{
private:
  
  
  
  
  
  
  
  
  
  
  
  
  struct ResourceItem {
    ResourceItem(uint8_t const* aData, uint32_t aSize) {
      mData.AppendElements(aData, aSize);
    }
    nsTArray<uint8_t> mData;

    size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
      
      size_t size = aMallocSizeOf(this);

      
      size += mData.SizeOfExcludingThis(aMallocSizeOf);

      return size;
    }
  };

  class ResourceQueueDeallocator : public nsDequeFunctor {
    virtual void* operator() (void* aObject) {
      delete static_cast<ResourceItem*>(aObject);
      return nullptr;
    }
  };

  class ResourceQueue : private nsDeque {
  public:
    ResourceQueue() :
      nsDeque(new ResourceQueueDeallocator()),
      mLogicalLength(0),
      mOffset(0)
    {
    }

    
    inline uint64_t GetOffset() {
      return mOffset;
    }

    
    
    inline uint64_t GetLength() {
      return mLogicalLength;
    }

    
    inline void CopyData(uint64_t aOffset, uint32_t aCount, char* aDest) {
      uint32_t offset = 0;
      uint32_t start = GetAtOffset(aOffset, &offset);
      uint32_t end = std::min(GetAtOffset(aOffset + aCount, nullptr) + 1, GetSize());
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

    inline void PushBack(ResourceItem* aItem) {
      mLogicalLength += aItem->mData.Length();
      nsDeque::Push(aItem);
    }

    
    
    
    inline bool Evict(uint64_t aOffset, uint32_t aThreshold) {
      bool evicted = false;
      while (GetLength() - mOffset > aThreshold) {
        ResourceItem* item = ResourceAt(0);
        if (item->mData.Length() + mOffset > aOffset) {
          break;
        }
        mOffset += item->mData.Length();
        delete PopFront();
        evicted = true;
      }
      return evicted;
    }

    size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
      
      size_t size = nsDeque::SizeOfExcludingThis(aMallocSizeOf);

      
      for (int32_t i = 0; i < nsDeque::GetSize(); ++i) {
        const ResourceItem* item =
            static_cast<const ResourceItem*>(nsDeque::ObjectAt(i));
        size += item->SizeOfIncludingThis(aMallocSizeOf);
      }

      return size;
    }

  private:
    
    inline uint32_t GetSize() {
      return nsDeque::GetSize();
    }

    inline ResourceItem* ResourceAt(uint32_t aIndex) {
      return static_cast<ResourceItem*>(nsDeque::ObjectAt(aIndex));
    }

    
    
    
    
    
    inline uint32_t GetAtOffset(uint64_t aOffset, uint32_t *aResourceOffset) {
      MOZ_ASSERT(aOffset >= mOffset);
      uint64_t offset = mOffset;
      for (uint32_t i = 0; i < GetSize(); ++i) {
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

    inline ResourceItem* PopFront() {
      return static_cast<ResourceItem*>(nsDeque::PopFront());
    }

    
    uint64_t mLogicalLength;

    
    uint64_t mOffset;
  };

public:
  SourceBufferResource(nsIPrincipal* aPrincipal,
                       const nsACString& aType);
protected:
  ~SourceBufferResource();

public:
  virtual nsresult Close() MOZ_OVERRIDE;
  virtual void Suspend(bool aCloseImmediately) MOZ_OVERRIDE {}
  virtual void Resume() MOZ_OVERRIDE {}

  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() MOZ_OVERRIDE
  {
    return nsCOMPtr<nsIPrincipal>(mPrincipal).forget();
  }

  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder) MOZ_OVERRIDE
  {
    return nullptr;
  }

  virtual void SetReadMode(MediaCacheStream::ReadMode aMode) MOZ_OVERRIDE {}
  virtual void SetPlaybackRate(uint32_t aBytesPerSecond) MOZ_OVERRIDE {}
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE;
  virtual void StartSeekingForMetadata() MOZ_OVERRIDE { }
  virtual void EndSeekingForMetadata() MOZ_OVERRIDE {}
  virtual int64_t Tell() MOZ_OVERRIDE { return mOffset; }
  virtual void Pin() MOZ_OVERRIDE {}
  virtual void Unpin() MOZ_OVERRIDE {}
  virtual double GetDownloadRate(bool* aIsReliable) MOZ_OVERRIDE { return 0; }
  virtual int64_t GetLength() MOZ_OVERRIDE { return mInputBuffer.GetLength(); }
  virtual int64_t GetNextCachedData(int64_t aOffset) MOZ_OVERRIDE { return GetLength() == aOffset ? -1 : aOffset; }
  virtual int64_t GetCachedDataEnd(int64_t aOffset) MOZ_OVERRIDE { return GetLength(); }
  virtual bool IsDataCachedToEndOfResource(int64_t aOffset) MOZ_OVERRIDE { return false; }
  virtual bool IsSuspendedByCache() MOZ_OVERRIDE { return false; }
  virtual bool IsSuspended() MOZ_OVERRIDE { return false; }
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount) MOZ_OVERRIDE;
  virtual bool IsTransportSeekable() MOZ_OVERRIDE { return true; }
  virtual nsresult Open(nsIStreamListener** aStreamListener) MOZ_OVERRIDE { return NS_ERROR_FAILURE; }

  virtual nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) MOZ_OVERRIDE
  {
    if (mInputBuffer.GetLength()) {
      aRanges.AppendElement(MediaByteRange(mInputBuffer.GetOffset(),
                                           mInputBuffer.GetLength()));
    }
    return NS_OK;
  }

  virtual const nsCString& GetContentType() const MOZ_OVERRIDE { return mType; }

  virtual size_t SizeOfExcludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    ReentrantMonitorAutoEnter mon(mMonitor);

    
    
    size_t size = MediaResource::SizeOfExcludingThis(aMallocSizeOf);
    size += mType.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    size += mInputBuffer.SizeOfExcludingThis(aMallocSizeOf);

    return size;
  }

  virtual size_t SizeOfIncludingThis(
                      MallocSizeOf aMallocSizeOf) const MOZ_OVERRIDE
  {
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
  }

  
  void AppendData(const uint8_t* aData, uint32_t aLength);
  void Ended();
  
  
  bool EvictData(uint32_t aThreshold);

  
  void EvictBefore(uint64_t aOffset);

private:
  nsresult SeekInternal(int64_t aOffset);

  nsCOMPtr<nsIPrincipal> mPrincipal;
  const nsCString mType;

  
  
  
  
  mutable ReentrantMonitor mMonitor;

  
  ResourceQueue mInputBuffer;

  uint64_t mOffset;
  bool mClosed;
  bool mEnded;
};

} 
#endif 
