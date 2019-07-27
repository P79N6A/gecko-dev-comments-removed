



#ifndef BUFFER_STREAM_H_
#define BUFFER_STREAM_H_

#include "mp4_demuxer/Stream.h"
#include "nsTArray.h"
#include "MediaResource.h"

namespace mozilla {
class MediaByteBuffer;
}

namespace mp4_demuxer {

class BufferStream : public Stream
{
public:
  


  BufferStream();
  explicit BufferStream(mozilla::MediaByteBuffer* aBuffer);

  virtual bool ReadAt(int64_t aOffset, void* aData, size_t aLength,
                      size_t* aBytesRead) override;
  virtual bool CachedReadAt(int64_t aOffset, void* aData, size_t aLength,
                            size_t* aBytesRead) override;
  virtual bool Length(int64_t* aLength) override;

  virtual void DiscardBefore(int64_t aOffset) override;

  bool AppendBytes(const uint8_t* aData, size_t aLength);

  mozilla::MediaByteRange GetByteRange();

private:
  ~BufferStream();
  int64_t mStartOffset;
  nsRefPtr<mozilla::MediaByteBuffer> mData;
};
}

#endif
