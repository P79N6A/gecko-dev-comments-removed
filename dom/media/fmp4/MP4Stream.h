





#ifndef MP4_STREAM_H_
#define MP4_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"

#include "mozilla/Monitor.h"

namespace mozilla {

class MediaResource;

class MP4Stream : public mp4_demuxer::Stream {
public:
  explicit MP4Stream(MediaResource* aResource, Monitor* aDemuxerMonitor);
  virtual ~MP4Stream();
  virtual bool ReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                      size_t* aBytesRead) MOZ_OVERRIDE;
  virtual bool CachedReadAt(int64_t aOffset, void* aBuffer, size_t aCount,
                            size_t* aBytesRead) MOZ_OVERRIDE;
  virtual bool Length(int64_t* aSize) MOZ_OVERRIDE;

private:
  nsRefPtr<MediaResource> mResource;
  Monitor* mDemuxerMonitor;
};

}

#endif
