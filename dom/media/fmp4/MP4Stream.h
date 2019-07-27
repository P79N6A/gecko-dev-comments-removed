





#ifndef MP4_STREAM_H_
#define MP4_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"

#include "MediaResource.h"

#include "mozilla/Maybe.h"

namespace mozilla {

class Monitor;

class MP4Stream : public mp4_demuxer::Stream {
public:
  explicit MP4Stream(MediaResource* aResource);
  virtual ~MP4Stream();
  bool BlockingReadAt(int64_t aOffset, void* aBuffer, size_t aCount, size_t* aBytesRead);
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

  void Pin() { mResource->Pin(); }
  void Unpin() { mResource->Unpin(); }

private:
  nsRefPtr<MediaResource> mResource;
  Maybe<ReadRecord> mFailedRead;
};

}

#endif
