



#ifndef BUFFER_STREAM_H_
#define BUFFER_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"

namespace mp4_demuxer {

class BufferStream : public Stream
{
public:
  


  BufferStream(const uint8_t* aData, size_t aLength);

  virtual bool ReadAt(int64_t aOffset, void* aData, size_t aLength,
                      size_t* aBytesRead) MOZ_OVERRIDE;
  virtual bool Length(int64_t* aLength) MOZ_OVERRIDE;

private:
  const uint8_t* mData;
  size_t mLength;
};
}

#endif
