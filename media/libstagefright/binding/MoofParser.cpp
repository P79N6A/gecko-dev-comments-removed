



#include "mp4_demuxer/MoofParser.h"
#include "mp4_demuxer/Box.h"

namespace mp4_demuxer
{

using namespace stagefright;
using namespace mozilla;

void
MoofParser::RebuildFragmentedIndex(
  const nsTArray<mozilla::MediaByteRange>& aByteRanges)
{
  BoxContext context(mSource, aByteRanges);
  RebuildFragmentedIndex(context);
}

void
MoofParser::RebuildFragmentedIndex(BoxContext& aContext)
{
  for (Box box(&aContext, mOffset); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("moov")) {
      mInitRange = MediaByteRange(0, box.Range().mEnd);
      ParseMoov(box);
    } else if (box.IsType("moof")) {
      Moof moof(box, mTrex, mMdhd, mEdts);

      if (!mMoofs.IsEmpty()) {
        
        
        mMoofs.LastElement().FixRounding(moof);
      }

      mMoofs.AppendElement(moof);
    }
    mOffset = box.NextOffset();
  }
}

class BlockingStream : public Stream {
public:
  BlockingStream(Stream* aStream) : mStream(aStream)
  {
  }

  bool ReadAt(int64_t offset, void* data, size_t size, size_t* bytes_read)
    MOZ_OVERRIDE
  {
    return mStream->ReadAt(offset, data, size, bytes_read);
  }

  bool CachedReadAt(int64_t offset, void* data, size_t size, size_t* bytes_read)
    MOZ_OVERRIDE
  {
    return mStream->ReadAt(offset, data, size, bytes_read);
  }

  virtual bool Length(int64_t* size) MOZ_OVERRIDE
  {
    return mStream->Length(size);
  }

private:
  nsRefPtr<Stream> mStream;
};

bool
MoofParser::BlockingReadNextMoof()
{
  nsTArray<MediaByteRange> byteRanges;
  int64_t size;
  bool hasSize = mSource->Length(&size);
  byteRanges.AppendElement(
    MediaByteRange(0,hasSize ? size : std::numeric_limits<int64_t>::max()));
  mp4_demuxer::BlockingStream* stream = new BlockingStream(mSource);

  BoxContext context(stream, byteRanges);
  for (Box box(&context, mOffset); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("moof")) {
      byteRanges.Clear();
      byteRanges.AppendElement(MediaByteRange(mOffset, box.Range().mEnd));
      RebuildFragmentedIndex(context);
      return true;
    }
  }
  return false;
}


Interval<Microseconds>
MoofParser::GetCompositionRange(const nsTArray<MediaByteRange>& aByteRanges)
{
  Interval<Microseconds> compositionRange;
  BoxContext context(mSource, aByteRanges);
  for (size_t i = 0; i < mMoofs.Length(); i++) {
    Moof& moof = mMoofs[i];
    Box box(&context, moof.mRange.mStart);
    if (box.IsAvailable()) {
      compositionRange = compositionRange.Extents(moof.mTimeRange);
    }
  }
  return compositionRange;
}

bool
MoofParser::ReachedEnd()
{
  int64_t length;
  return mSource->Length(&length) && mOffset == length;
}

void
MoofParser::ParseMoov(Box& aBox)
{
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("trak")) {
      ParseTrak(box);
    } else if (box.IsType("mvex")) {
      ParseMvex(box);
    }
  }
}

void
MoofParser::ParseTrak(Box& aBox)
{
  Tkhd tkhd;
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("tkhd")) {
      tkhd = Tkhd(box);
    } else if (box.IsType("mdia")) {
      if (!mTrex.mTrackId || tkhd.mTrackId == mTrex.mTrackId) {
        ParseMdia(box, tkhd);
      }
    } else if (box.IsType("edts")) {
      mEdts = Edts(box);
    }
  }
}

void
MoofParser::ParseMdia(Box& aBox, Tkhd& aTkhd)
{
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("mdhd")) {
      mMdhd = Mdhd(box);
    }
  }
}

void
MoofParser::ParseMvex(Box& aBox)
{
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("trex")) {
      Trex trex = Trex(box);
      if (!mTrex.mTrackId || trex.mTrackId == mTrex.mTrackId) {
        mTrex = trex;
      }
    }
  }
}

Moof::Moof(Box& aBox, Trex& aTrex, Mdhd& aMdhd, Edts& aEdts) :
    mRange(aBox.Range()), mMaxRoundingError(0)
{
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("traf")) {
      ParseTraf(box, aTrex, aMdhd, aEdts);
    }
  }
}

void
Moof::ParseTraf(Box& aBox, Trex& aTrex, Mdhd& aMdhd, Edts& aEdts)
{
  Tfhd tfhd(aTrex);
  Tfdt tfdt;
  for (Box box = aBox.FirstChild(); box.IsAvailable(); box = box.Next()) {
    if (box.IsType("tfhd")) {
      tfhd = Tfhd(box, aTrex);
    } else if (box.IsType("tfdt")) {
      if (!aTrex.mTrackId || tfhd.mTrackId == aTrex.mTrackId) {
        tfdt = Tfdt(box);
      }
    } else if (box.IsType("trun")) {
      if (!aTrex.mTrackId || tfhd.mTrackId == aTrex.mTrackId) {
        ParseTrun(box, tfhd, tfdt, aMdhd, aEdts);
      }
    }
  }
}

void
Moof::FixRounding(const Moof& aMoof) {
  Microseconds gap = aMoof.mTimeRange.start - mTimeRange.end;
  if (gap > 0 && gap <= mMaxRoundingError) {
    mTimeRange.end = aMoof.mTimeRange.start;
  }
}

class CtsComparator
{
public:
  bool Equals(Sample* const aA, Sample* const aB) const
  {
    return aA->mCompositionRange.start == aB->mCompositionRange.start;
  }
  bool
  LessThan(Sample* const aA, Sample* const aB) const
  {
    return aA->mCompositionRange.start < aB->mCompositionRange.start;
  }
};

void
Moof::ParseTrun(Box& aBox, Tfhd& aTfhd, Tfdt& aTfdt, Mdhd& aMdhd, Edts& aEdts)
{
  if (!aMdhd.mTimescale) {
    return;
  }

  BoxReader reader(aBox);
  uint32_t flags = reader->ReadU32();
  if ((flags & 0x404) == 0x404) {
    
    reader->DiscardRemaining();
    return;
  }
  uint8_t version = flags >> 24;

  uint32_t sampleCount = reader->ReadU32();
  if (sampleCount == 0) {
    return;
  }

  uint64_t offset = aTfhd.mBaseDataOffset + (flags & 1 ? reader->ReadU32() : 0);
  bool hasFirstSampleFlags = flags & 4;
  uint32_t firstSampleFlags = hasFirstSampleFlags ? reader->ReadU32() : 0;
  uint64_t decodeTime = aTfdt.mBaseMediaDecodeTime;
  nsTArray<Interval<Microseconds>> timeRanges;
  for (size_t i = 0; i < sampleCount; i++) {
    uint32_t sampleDuration =
      flags & 0x100 ? reader->ReadU32() : aTfhd.mDefaultSampleDuration;
    uint32_t sampleSize =
      flags & 0x200 ? reader->ReadU32() : aTfhd.mDefaultSampleSize;
    uint32_t sampleFlags =
      flags & 0x400 ? reader->ReadU32() : hasFirstSampleFlags && i == 0
                                            ? firstSampleFlags
                                            : aTfhd.mDefaultSampleFlags;
    int64_t ctsOffset = 0;
    if (flags & 0x800) {
      if (version == 0) {
        ctsOffset = reader->ReadU32();
      } else {
        ctsOffset = reader->Read32();
      }
    }

    Sample sample;
    sample.mByteRange = MediaByteRange(offset, offset + sampleSize);
    offset += sampleSize;

    sample.mDecodeTime = decodeTime;
    sample.mCompositionRange = Interval<Microseconds>(
      aMdhd.ToMicroseconds((int64_t)decodeTime + ctsOffset - aEdts.mMediaStart),
      aMdhd.ToMicroseconds((int64_t)decodeTime + ctsOffset + sampleDuration - aEdts.mMediaStart));
    decodeTime += sampleDuration;

    sample.mSync = !(sampleFlags & 0x1010000);

    mIndex.AppendElement(sample);

    mMdatRange = mMdatRange.Extents(sample.mByteRange);
  }
  mMaxRoundingError += aMdhd.ToMicroseconds(sampleCount);

  nsTArray<Sample*> ctsOrder;
  for (int i = 0; i < mIndex.Length(); i++) {
    ctsOrder.AppendElement(&mIndex[i]);
  }
  ctsOrder.Sort(CtsComparator());

  for (int i = 0; i < ctsOrder.Length(); i++) {
    if (i + 1 < ctsOrder.Length()) {
      ctsOrder[i]->mCompositionRange.end = ctsOrder[i + 1]->mCompositionRange.start;
    }
  }
  mTimeRange = Interval<Microseconds>(ctsOrder[0]->mCompositionRange.start,
      ctsOrder.LastElement()->mCompositionRange.end);
}

Tkhd::Tkhd(Box& aBox)
{
  BoxReader reader(aBox);
  uint32_t flags = reader->ReadU32();
  uint8_t version = flags >> 24;
  if (version == 0) {
    mCreationTime = reader->ReadU32();
    mModificationTime = reader->ReadU32();
    mTrackId = reader->ReadU32();
    uint32_t reserved = reader->ReadU32();
    NS_ASSERTION(!reserved, "reserved should be 0");
    mDuration = reader->ReadU32();
  } else if (version == 1) {
    mCreationTime = reader->ReadU64();
    mModificationTime = reader->ReadU64();
    mTrackId = reader->ReadU32();
    uint32_t reserved = reader->ReadU32();
    NS_ASSERTION(!reserved, "reserved should be 0");
    mDuration = reader->ReadU64();
  }
  
  reader->DiscardRemaining();
}

Mdhd::Mdhd(Box& aBox)
{
  BoxReader reader(aBox);
  uint32_t flags = reader->ReadU32();
  uint8_t version = flags >> 24;
  if (version == 0) {
    mCreationTime = reader->ReadU32();
    mModificationTime = reader->ReadU32();
    mTimescale = reader->ReadU32();
    mDuration = reader->ReadU32();
  } else if (version == 1) {
    mCreationTime = reader->ReadU64();
    mModificationTime = reader->ReadU64();
    mTimescale = reader->ReadU32();
    mDuration = reader->ReadU64();
  }
  
  reader->ReadU32();
}

Trex::Trex(Box& aBox)
{
  BoxReader reader(aBox);
  mFlags = reader->ReadU32();
  mTrackId = reader->ReadU32();
  mDefaultSampleDescriptionIndex = reader->ReadU32();
  mDefaultSampleDuration = reader->ReadU32();
  mDefaultSampleSize = reader->ReadU32();
  mDefaultSampleFlags = reader->ReadU32();
}

Tfhd::Tfhd(Box& aBox, Trex& aTrex) : Trex(aTrex)
{
  MOZ_ASSERT(aBox.IsType("tfhd"));
  MOZ_ASSERT(aBox.Parent()->IsType("traf"));
  MOZ_ASSERT(aBox.Parent()->Parent()->IsType("moof"));

  BoxReader reader(aBox);
  mFlags = reader->ReadU32();
  mBaseDataOffset =
    mFlags & 1 ? reader->ReadU32() : aBox.Parent()->Parent()->Offset();
  mTrackId = reader->ReadU32();
  if (mFlags & 2) {
    mDefaultSampleDescriptionIndex = reader->ReadU32();
  }
  if (mFlags & 8) {
    mDefaultSampleDuration = reader->ReadU32();
  }
  if (mFlags & 0x10) {
    mDefaultSampleSize = reader->ReadU32();
  }
  if (mFlags & 0x20) {
    mDefaultSampleFlags = reader->ReadU32();
  }
}

Tfdt::Tfdt(Box& aBox)
{
  BoxReader reader(aBox);
  uint32_t flags = reader->ReadU32();
  uint8_t version = flags >> 24;
  if (version == 0) {
    mBaseMediaDecodeTime = reader->ReadU32();
  } else if (version == 1) {
    mBaseMediaDecodeTime = reader->ReadU64();
  }
  reader->DiscardRemaining();
}

Edts::Edts(Box& aBox)
  : mMediaStart(0)
{
  Box child = aBox.FirstChild();
  if (!child.IsType("elst")) {
    return;
  }

  BoxReader reader(child);
  uint32_t flags = reader->ReadU32();
  uint8_t version = flags >> 24;

  uint32_t entryCount = reader->ReadU32();
  NS_ASSERTION(entryCount == 1, "Can't handle videos with multiple edits");
  if (entryCount != 1) {
    reader->DiscardRemaining();
    return;
  }

  uint64_t segment_duration;
  if (version == 1) {
    segment_duration = reader->ReadU64();
    mMediaStart = reader->Read64();
  } else {
    segment_duration = reader->ReadU32();
    mMediaStart = reader->Read32();
  }
  NS_ASSERTION(segment_duration == 0, "Can't handle edits with fixed durations");
  reader->DiscardRemaining();
}
}
