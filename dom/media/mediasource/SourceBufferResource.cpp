





#include "SourceBufferResource.h"

#include <algorithm>

#include "nsISeekableStream.h"
#include "nsISupports.h"
#include "prlog.h"
#include "MediaData.h"

#ifdef PR_LOGGING
PRLogModuleInfo* GetSourceBufferResourceLog()
{
  static PRLogModuleInfo* sLogModule;
  if (!sLogModule) {
    sLogModule = PR_NewLogModule("SourceBufferResource");
  }
  return sLogModule;
}

#define SBR_DEBUG(arg, ...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG, ("SourceBufferResource(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))
#define SBR_DEBUGV(arg, ...) PR_LOG(GetSourceBufferResourceLog(), PR_LOG_DEBUG+1, ("SourceBufferResource(%p:%s)::%s: " arg, this, mType.get(), __func__, ##__VA_ARGS__))
#else
#define SBR_DEBUG(...)
#define SBR_DEBUGV(...)
#endif

namespace mozilla {

nsresult
SourceBufferResource::Close()
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  SBR_DEBUG("Close");
  
  mClosed = true;
  mon.NotifyAll();
  return NS_OK;
}

nsresult
SourceBufferResource::Read(char* aBuffer, uint32_t aCount, uint32_t* aBytes)
{
  SBR_DEBUGV("Read(aBuffer=%p, aCount=%u, aBytes=%p)",
             aBuffer, aCount, aBytes);
  ReentrantMonitorAutoEnter mon(mMonitor);

  return ReadInternal(aBuffer, aCount, aBytes,  true);
}

nsresult
SourceBufferResource::ReadInternal(char* aBuffer, uint32_t aCount, uint32_t* aBytes, bool aMayBlock)
{
  mMonitor.AssertCurrentThreadIn();
  MOZ_ASSERT_IF(!aMayBlock, aBytes);

  
  
  
  uint64_t readOffset = mOffset;

  while (aMayBlock &&
         !mEnded &&
         readOffset + aCount > static_cast<uint64_t>(GetLength())) {
    SBR_DEBUGV("waiting for data");
    mMonitor.Wait();
    
    
    if (readOffset < mInputBuffer.GetOffset()) {
      return NS_ERROR_FAILURE;
    }
  }

  uint32_t available = GetLength() - readOffset;
  uint32_t count = std::min(aCount, available);
  SBR_DEBUGV("readOffset=%llu GetLength()=%u available=%u count=%u mEnded=%d",
             readOffset, GetLength(), available, count, mEnded);
  if (available == 0) {
    SBR_DEBUGV("reached EOF");
    *aBytes = 0;
    return NS_OK;
  }

  mInputBuffer.CopyData(readOffset, count, aBuffer);
  *aBytes = count;

  
  
  
  mOffset = readOffset + count;

  return NS_OK;
}

nsresult
SourceBufferResource::ReadAt(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes)
{
  SBR_DEBUG("ReadAt(aOffset=%lld, aBuffer=%p, aCount=%u, aBytes=%p)",
            aOffset, aBytes, aCount, aBytes);
  ReentrantMonitorAutoEnter mon(mMonitor);
  return ReadAtInternal(aOffset, aBuffer, aCount, aBytes,  true);
}

nsresult
SourceBufferResource::ReadAtInternal(int64_t aOffset, char* aBuffer, uint32_t aCount, uint32_t* aBytes,
                                     bool aMayBlock)
{
  mMonitor.AssertCurrentThreadIn();
  nsresult rv = SeekInternal(aOffset);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return ReadInternal(aBuffer, aCount, aBytes, aMayBlock);
}

nsresult
SourceBufferResource::Seek(int32_t aWhence, int64_t aOffset)
{
  SBR_DEBUG("Seek(aWhence=%d, aOffset=%lld)",
            aWhence, aOffset);
  ReentrantMonitorAutoEnter mon(mMonitor);

  int64_t newOffset = mOffset;
  switch (aWhence) {
  case nsISeekableStream::NS_SEEK_END:
    newOffset = GetLength() - aOffset;
    break;
  case nsISeekableStream::NS_SEEK_CUR:
    newOffset += aOffset;
    break;
  case nsISeekableStream::NS_SEEK_SET:
    newOffset = aOffset;
    break;
  }

  SBR_DEBUGV("newOffset=%lld GetOffset()=%llu GetLength()=%llu)",
             newOffset, mInputBuffer.GetOffset(), GetLength());
  nsresult rv = SeekInternal(newOffset);
  mon.NotifyAll();
  return rv;
}

nsresult
SourceBufferResource::SeekInternal(int64_t aOffset)
{
  mMonitor.AssertCurrentThreadIn();

  if (mClosed ||
      aOffset < 0 ||
      uint64_t(aOffset) < mInputBuffer.GetOffset() ||
      aOffset > GetLength()) {
    return NS_ERROR_FAILURE;
  }

  mOffset = aOffset;
  return NS_OK;
}

nsresult
SourceBufferResource::ReadFromCache(char* aBuffer, int64_t aOffset, uint32_t aCount)
{
  SBR_DEBUG("ReadFromCache(aBuffer=%p, aOffset=%lld, aCount=%u)",
            aBuffer, aOffset, aCount);
  ReentrantMonitorAutoEnter mon(mMonitor);
  uint32_t bytesRead;
  int64_t oldOffset = mOffset;
  nsresult rv = ReadAtInternal(aOffset, aBuffer, aCount, &bytesRead,  false);
  mOffset = oldOffset; 
  NS_ENSURE_SUCCESS(rv, rv);

  
  return bytesRead == aCount ? NS_OK : NS_ERROR_FAILURE;
}

uint32_t
SourceBufferResource::EvictData(uint64_t aPlaybackOffset, uint32_t aThreshold)
{
  SBR_DEBUG("EvictData(aPlaybackOffset=%llu,"
            "aThreshold=%u)", aPlaybackOffset, aThreshold);
  ReentrantMonitorAutoEnter mon(mMonitor);
  uint32_t result = mInputBuffer.Evict(aPlaybackOffset, aThreshold);
  if (result > 0) {
    
    
    mon.NotifyAll();
  }
  return result;
}

void
SourceBufferResource::EvictBefore(uint64_t aOffset)
{
  SBR_DEBUG("EvictBefore(aOffset=%llu)", aOffset);
  ReentrantMonitorAutoEnter mon(mMonitor);
  
  if (aOffset < mOffset) {
    mInputBuffer.EvictBefore(aOffset);
  }
  
  
  mon.NotifyAll();
}

uint32_t
SourceBufferResource::EvictAll()
{
  SBR_DEBUG("EvictAll()");
  ReentrantMonitorAutoEnter mon(mMonitor);
  return mInputBuffer.EvictAll();
}

void
SourceBufferResource::AppendData(LargeDataBuffer* aData)
{
  SBR_DEBUG("AppendData(aData=%p, aLength=%u)",
            aData->Elements(), aData->Length());
  ReentrantMonitorAutoEnter mon(mMonitor);
  mInputBuffer.AppendItem(aData);
  mEnded = false;
  mon.NotifyAll();
}

void
SourceBufferResource::Ended()
{
  SBR_DEBUG("");
  ReentrantMonitorAutoEnter mon(mMonitor);
  mEnded = true;
  mon.NotifyAll();
}

SourceBufferResource::~SourceBufferResource()
{
  SBR_DEBUG("");
  MOZ_COUNT_DTOR(SourceBufferResource);
}

SourceBufferResource::SourceBufferResource(const nsACString& aType)
  : mType(aType)
  , mMonitor("mozilla::SourceBufferResource::mMonitor")
  , mOffset(0)
  , mClosed(false)
  , mEnded(false)
{
  SBR_DEBUG("");
  MOZ_COUNT_CTOR(SourceBufferResource);
}

#undef SBR_DEBUG
#undef SBR_DEBUGV
} 
