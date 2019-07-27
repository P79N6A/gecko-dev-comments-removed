





#include "mp4_demuxer/Box.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "mozilla/Endian.h"
#include <algorithm>

using namespace mozilla;

namespace mp4_demuxer {



static uint32_t
BoxOffset(AtomType aType)
{
  const uint32_t FULLBOX_OFFSET = 4;

  if (aType == AtomType("mp4a") || aType == AtomType("enca")) {
    
    return 28;
  } else if (aType == AtomType("mp4v") || aType == AtomType("encv")) {
    
    return 78;
  } else if (aType == AtomType("stsd")) {
    
    
    
    return FULLBOX_OFFSET + 4;
  }

  return 0;
}

Box::Box(BoxContext* aContext, uint64_t aOffset, const Box* aParent)
  : mContext(aContext), mParent(aParent)
{
  uint8_t header[8];
  MediaByteRange headerRange(aOffset, aOffset + sizeof(header));
  if (mParent && !mParent->mRange.Contains(headerRange)) {
    return;
  }

  const MediaByteRange* byteRange;
  for (int i = 0; ; i++) {
    if (i == mContext->mByteRanges.Length()) {
      return;
    }

    byteRange = &mContext->mByteRanges[i];
    if (byteRange->Contains(headerRange)) {
      break;
    }
  }

  size_t bytes;
  if (!mContext->mSource->CachedReadAt(aOffset, header, sizeof(header),
                                       &bytes) ||
      bytes != sizeof(header)) {
    return;
  }

  uint64_t size = BigEndian::readUint32(header);
  if (size == 1) {
    uint8_t bigLength[8];
    MediaByteRange bigLengthRange(headerRange.mEnd,
                                  headerRange.mEnd + sizeof(bigLength));
    if ((mParent && !mParent->mRange.Contains(bigLengthRange)) ||
        !byteRange->Contains(bigLengthRange) ||
        !mContext->mSource->CachedReadAt(aOffset, bigLength,
                                         sizeof(bigLength), &bytes) ||
        bytes != sizeof(bigLength)) {
      return;
    }
    size = BigEndian::readUint64(bigLength);
    mBodyOffset = bigLengthRange.mEnd;
  } else {
    mBodyOffset = headerRange.mEnd;
  }

  mType = BigEndian::readUint32(&header[4]);
  mChildOffset = mBodyOffset + BoxOffset(mType);

  MediaByteRange boxRange(aOffset, aOffset + size);
  if (mChildOffset > boxRange.mEnd ||
      (mParent && !mParent->mRange.Contains(boxRange)) ||
      !byteRange->Contains(boxRange)) {
    return;
  }

  mRange = boxRange;
}

Box::Box()
  : mContext(nullptr)
{}

Box
Box::Next() const
{
  MOZ_ASSERT(IsAvailable());
  return Box(mContext, mRange.mEnd, mParent);
}

Box
Box::FirstChild() const
{
  MOZ_ASSERT(IsAvailable());
  if (mChildOffset == mRange.mEnd) {
    return Box();
  }
  return Box(mContext, mChildOffset, this);
}

bool
Box::Read(nsTArray<uint8_t>* aDest)
{
  return Read(aDest, mRange);
}

bool
Box::Read(nsTArray<uint8_t>* aDest, const MediaByteRange& aRange)
{
  int64_t length;
  if (!mContext->mSource->Length(&length)) {
    
    
    length = std::min(aRange.mEnd - mChildOffset, uint64_t(32 * 1024 * 1024));
  } else {
    length = aRange.mEnd - mChildOffset;
  }
  aDest->SetLength(length);
  size_t bytes;
  if (!mContext->mSource->CachedReadAt(mChildOffset, aDest->Elements(),
                                       aDest->Length(), &bytes) ||
      bytes != aDest->Length()) {
    
    NS_WARNING("Read failed in mp4_demuxer::Box::Read()");
    aDest->Clear();
    return false;
  }
  return true;
}
}
