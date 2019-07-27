





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

#define UNIMPLEMENTED() { /* Logging this is too spammy to do by default */ }

class nsIStreamListener;

namespace mozilla {

class MediaDecoder;
class LargeDataBuffer;

namespace dom {

class SourceBuffer;

}  

class SourceBufferResource MOZ_FINAL : public MediaResource
{
  
  
public:
  explicit SourceBufferResource(const nsACString& aType);
  virtual nsresult Close() MOZ_OVERRIDE;
  virtual void Suspend(bool aCloseImmediately) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void Resume() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() MOZ_OVERRIDE { UNIMPLEMENTED(); return nullptr; }
  virtual already_AddRefed<MediaResource> CloneData(MediaDecoder* aDecoder) MOZ_OVERRIDE { UNIMPLEMENTED(); return nullptr; }
  virtual void SetReadMode(MediaCacheStream::ReadMode aMode) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void SetPlaybackRate(uint32_t aBytesPerSecond) MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual nsresult Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult ReadAt(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes) MOZ_OVERRIDE;
  virtual nsresult Seek(int32_t aWhence, int64_t aOffset) MOZ_OVERRIDE;
  virtual int64_t Tell() MOZ_OVERRIDE { return mOffset; }
  virtual void Pin() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual void Unpin() MOZ_OVERRIDE { UNIMPLEMENTED(); }
  virtual double GetDownloadRate(bool* aIsReliable) MOZ_OVERRIDE { UNIMPLEMENTED(); *aIsReliable = false; return 0; }
  virtual int64_t GetLength() MOZ_OVERRIDE { return mInputBuffer.GetLength(); }
  virtual int64_t GetNextCachedData(int64_t aOffset) MOZ_OVERRIDE {
    ReentrantMonitorAutoEnter mon(mMonitor);
    MOZ_ASSERT(aOffset >= 0);
    if (uint64_t(aOffset) < mInputBuffer.GetOffset()) {
      return mInputBuffer.GetOffset();
    } else if (aOffset == GetLength()) {
      return -1;
    }
    return aOffset;
  }
  virtual int64_t GetCachedDataEnd(int64_t aOffset) MOZ_OVERRIDE { UNIMPLEMENTED(); return -1; }
  virtual bool IsDataCachedToEndOfResource(int64_t aOffset) MOZ_OVERRIDE { return false; }
  virtual bool IsSuspendedByCache() MOZ_OVERRIDE { UNIMPLEMENTED(); return false; }
  virtual bool IsSuspended() MOZ_OVERRIDE { UNIMPLEMENTED(); return false; }
  virtual nsresult ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount) MOZ_OVERRIDE;
  virtual bool IsTransportSeekable() MOZ_OVERRIDE { UNIMPLEMENTED(); return true; }
  virtual nsresult Open(nsIStreamListener** aStreamListener) MOZ_OVERRIDE { UNIMPLEMENTED(); return NS_ERROR_FAILURE; }

  virtual nsresult GetCachedRanges(nsTArray<MediaByteRange>& aRanges) MOZ_OVERRIDE
  {
    ReentrantMonitorAutoEnter mon(mMonitor);
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

  
  
  void AppendData(LargeDataBuffer* aData);
  void Ended();
  
  
  uint32_t EvictData(uint64_t aPlaybackOffset, uint32_t aThreshold);

  
  void EvictBefore(uint64_t aOffset);

  
  uint32_t EvictAll();

  
  int64_t GetSize() {
    ReentrantMonitorAutoEnter mon(mMonitor);
    return mInputBuffer.GetLength() - mInputBuffer.GetOffset();
  }

#if defined(DEBUG)
  void Dump(const char* aPath) {
    mInputBuffer.Dump(aPath);
  }
#endif

private:
  ~SourceBufferResource();
  nsresult SeekInternal(int64_t aOffset);
  nsresult ReadInternal(char* aBuffer, uint32_t aCount, uint32_t* aBytes, bool aMayBlock);
  nsresult ReadAtInternal(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes, bool aMayBlock);

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
