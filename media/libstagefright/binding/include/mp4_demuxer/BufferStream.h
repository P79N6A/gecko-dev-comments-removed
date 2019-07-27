



#ifndef BUFFER_STREAM_H_
#define BUFFER_STREAM_H_

#include "mp4_demuxer/mp4_demuxer.h"
#include "nsTArray.h"
#include "ResourceQueue.h"

namespace mp4_demuxer {

class BufferStream : public Stream
{
public:
  


  BufferStream();

  bool ReadAt(int64_t aOffset, void* aData, size_t aLength,
              size_t* aBytesRead);
  bool CachedReadAt(int64_t aOffset, void* aData, size_t aLength,
                    size_t* aBytesRead);
  bool Length(int64_t* aLength);

  void DiscardBefore(int64_t aOffset);

  void AppendBytes(const uint8_t* aData, size_t aLength);
  void AppendData(mozilla::ResourceItem* aItem);

  mozilla::MediaByteRange GetByteRange();

  int64_t mStartOffset;
  int64_t mLogicalLength;
  int64_t mStartIndex;
  nsTArray<nsRefPtr<mozilla::ResourceItem>> mData;
};
}

#endif
