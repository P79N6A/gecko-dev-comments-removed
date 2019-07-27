





#ifndef MP4_STREAM_H_
#define MP4_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"

#include "MediaResource.h"

#include "mozilla/Maybe.h"
#include "mozilla/Monitor.h"

namespace mozilla {

class Monitor;

class MP4Stream : public mp4_demuxer::Stream {
public:
  explicit MP4Stream(MediaResource* aResource);
  virtual ~MP4Stream();
  bool BlockingReadIntoCache(int64_t aOffset, size_t aCount, Monitor* aToUnlock);
  virtual bool ReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                      size_t* aBytesRead) MOZ_OVERRIDE;
  virtual bool CachedReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                            size_t* aBytesRead) MOZ_OVERRIDE;
  virtual bool Length(int64_t* aSize) MOZ_OVERRIDE;

  struct ReadRecord {
    ReadRecord(int64_t aOffset, size_t aCount) : mOffset(aOffset), mCount(aCount) {}
    bool operator==(const ReadRecord& aOther) { return mOffset == aOther.mOffset && mCount == aOther.mCount; }
    int64_t mOffset;
    size_t mCount;
  };
  bool LastReadFailed(ReadRecord* aOut)
  {
    if (mFailedRead.isSome()) {
      *aOut = mFailedRead.ref();
      return true;
    }

    return false;
  }

  void ClearFailedRead() { mFailedRead.reset(); }

  void Pin()
  {
    mResource->Pin();
    ++mPinCount;
  }

  void Unpin()
  {
    mResource->Unpin();
    MOZ_ASSERT(mPinCount);
    --mPinCount;
    if (mPinCount == 0) {
      mCache.Clear();
    }
  }

private:
  nsRefPtr<MediaResource> mResource;
  Maybe<ReadRecord> mFailedRead;
  uint32_t mPinCount;

  struct CacheBlock {
    CacheBlock(int64_t aOffset, size_t aCount)
      : mOffset(aOffset), mCount(aCount), mBuffer(nullptr) {}
    int64_t mOffset;
    size_t mCount;

    bool Init()
    {
      mBuffer = new (fallible) char[mCount];
      return !!mBuffer;
    }

    char* Buffer()
    {
      MOZ_ASSERT(mBuffer.get());
      return mBuffer.get();
    }

  private:
    nsAutoArrayPtr<char> mBuffer;
  };
  nsTArray<CacheBlock> mCache;
};

}

#endif
