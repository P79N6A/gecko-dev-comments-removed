



#include "mp4_demuxer/BufferStream.h"
#include <algorithm>

namespace mp4_demuxer {

BufferStream::BufferStream(const uint8_t* aData, size_t aLength)
  : mData(aData), mLength(aLength)
{

}

 bool
BufferStream::ReadAt(int64_t aOffset, void* aData, size_t aLength,
                     size_t* aBytesRead)
{
  if (aOffset > mLength) {
    return false;
  }
  *aBytesRead = std::min(aLength, mLength - (size_t) aOffset);
  memcpy(aData, mData + aOffset, *aBytesRead);
  return true;
}

 bool
BufferStream::Length(int64_t* aLength)
{
  *aLength = mLength;
  return true;
}

}
