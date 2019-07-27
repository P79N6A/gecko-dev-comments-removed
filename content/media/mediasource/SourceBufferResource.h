





#ifndef MOZILLA_SOURCEBUFFERRESOURCE_H_
#define MOZILLA_SOURCEBUFFERRESOURCE_H_

#include "MediaCache.h"
#include "MediaResource.h"
#include "ResourceQueue.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"
#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsIPrincipal.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nscore.h"
#include "prlog.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetMediaSourceLog();
extern PRLogModuleInfo* GetMediaSourceAPILog();

#define MSE_DEBUG(...) PR_LOG(GetMediaSourceLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define MSE_DEBUG(...)
#endif

#define UNIMPLEMENTED() MSE_DEBUG("SourceBufferResource(%p): UNIMPLEMENTED FUNCTION at line %d", this, __LINE__)

class nsIStreamListener;

namespace mozilla {

class MediaDecoder;

namespace dom {

class SourceBuffer;

}  

class SourceBufferResource MOZ_FINAL : public MediaResource
{
public:
  SourceBufferResource(nsIPrincipal* aPrincipal,
                       const nsACString& aType);
  virtual nsresult Close() MOZ_OVERRIDE;
  virtual void Suspend(bool aCloseImmediately) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void Resume() MOZ_OVERRIDE { UNIMPLEMENTED(); }

  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() MOZ_OVERRIDE
  {
    return nsCOMPtr<nsIPrincipal>(mPrincipal).forget();
  }

  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder) MOZ_OVERRIDE
  {
    UNIMPLEMENTED();
    return nullptr;
  }

  virtual void SetReadMode(MediaCacheStream::ReadMode aMode) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void SetPlaybackRate(uint32_t aBytesPerSecond) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE;
  virtual void StartSeekingForMetadata() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void EndSeekingForMetadata() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual int64_t Tell() MOZ_OVERRIDE { return mOffset; }
  virtual void Pin() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void Unpin() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual double GetDownloadRate(bool* aIsReliable) MOZ_OVERRIDE { UNIMPLEMENTED(); *aIsReliable = false; return 0; }
  virtual int64_t GetLength() MOZ_OVERRIDE { return mInputBuffer.GetLength(); }
  virtual int64_t GetNextCachedData(int64_t aOffset) MOZ_OVERRIDE { return GetLength() == aOffset ? -1 : aOffset; }
  virtual int64_t GetCachedDataEnd(int64_t aOffset) MOZ_OVERRIDE { return GetLength(); }
  virtual bool IsDataCachedToEndOfResource(int64_t aOffset) MOZ_OVERRIDE { UNIMPLEMENTED(); return false; }
  virtual bool IsSuspendedByCache() MOZ_OVERRIDE { UNIMPLEMENTED(); return false; }
  virtual bool IsSuspended() MOZ_OVERRIDE { UNIMPLEMENTED(); return false; }
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount) MOZ_OVERRIDE;
  virtual bool IsTransportSeekable() MOZ_OVERRIDE { UNIMPLEMENTED(); return true; }
  virtual nsresult Open(nsIStreamListener** aStreamListener) MOZ_OVERRIDE { UNIMPLEMENTED(); return NS_ERROR_FAILURE; }

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
  ~SourceBufferResource();
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

#undef UNIMPLEMENTED

#endif 
