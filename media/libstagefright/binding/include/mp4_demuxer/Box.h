





#ifndef BOX_H_
#define BOX_H_

#include <stdint.h>
#include "nsTArray.h"
#include "MediaResource.h"
#include "mozilla/Endian.h"
#include "mp4_demuxer/ByteReader.h"

using namespace mozilla;

namespace mp4_demuxer {

class Stream;

class BoxContext
{
public:
  BoxContext(Stream* aSource, const nsTArray<MediaByteRange>& aByteRanges)
    : mSource(aSource), mByteRanges(aByteRanges)
  {
  }

  Stream* mSource;
  const nsTArray<MediaByteRange>& mByteRanges;
};

class Box
{
public:
  Box(BoxContext* aContext, uint64_t aOffset, const Box* aParent = nullptr);

  bool IsAvailable() const { return !mRange.IsNull(); }
  uint64_t Offset() const { return mRange.mStart; }
  uint64_t Length() const { return mRange.mEnd - mRange.mStart; }
  uint64_t NextOffset() const { return mRange.mEnd; }
  const MediaByteRange& Range() const { return mRange; }

  const Box* Parent() const { return mParent; }

  bool IsType(const char* aType) const
  {
    return mType == BigEndian::readUint32(aType);
  }

  Box Next() const;
  Box FirstChild() const;
  void Read(nsTArray<uint8_t>* aDest);

private:
  bool Contains(MediaByteRange aRange) const;
  BoxContext* mContext;
  mozilla::MediaByteRange mRange;
  uint64_t mChildOffset;
  uint32_t mType;
  const Box* mParent;
};

class BoxReader
{
public:
  BoxReader(Box& aBox)
  {
    aBox.Read(&mBuffer);
    mReader.SetData(mBuffer);
  }
  ByteReader* operator->() { return &mReader; }
  ByteReader mReader;

private:
  nsTArray<uint8_t> mBuffer;
};
}

#endif
