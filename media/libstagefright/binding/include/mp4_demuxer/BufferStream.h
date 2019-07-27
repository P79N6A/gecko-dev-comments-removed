



#ifndef BUFFER_STREAM_H_
#define BUFFER_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"
#include "nsTArray.h"

namespace mp4_demuxer {

class BufferStream : public Stream
{
public:
  


  BufferStream();

  virtual bool ReadAt(int64_t aOffset, void* aData, size_t aLength,
                      size_t* aBytesRead) override;
  virtual bool CachedReadAt(int64_t aOffset, void* aData, size_t aLength,
                            size_t* aBytesRead) override;
  virtual bool Length(int64_t* aLength) override;

  virtual void DiscardBefore(int64_t aOffset) override;

  void AppendBytes(const uint8_t* aData, size_t aLength);

  mozilla::MediaByteRange GetByteRange();

private:
  int64_t mStartOffset;
  nsTArray<uint8_t> mData;
};
}

#endif
