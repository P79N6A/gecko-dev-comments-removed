



#ifndef MOOF_PARSER_H_
#define MOOF_PARSER_H_

#include "mp4_demuxer/mp4_demuxer.h"
#include "MediaResource.h"

namespace mp4_demuxer {

class Stream;
class Box;
class Moof;

class Tkhd
{
public:
  Tkhd()
    : mCreationTime(0)
    , mModificationTime(0)
    , mTrackId(0)
    , mDuration(0)
  {
  }
  Tkhd(Box& aBox);

  uint64_t mCreationTime;
  uint64_t mModificationTime;
  uint32_t mTrackId;
  uint64_t mDuration;
};

class Mdhd
{
public:
  Mdhd()
    : mCreationTime(0)
    , mModificationTime(0)
    , mTimescale(0)
    , mDuration(0)
  {
  }
  Mdhd(Box& aBox);

  Microseconds ToMicroseconds(uint64_t aTimescaleUnits)
  {
    return aTimescaleUnits * 1000000ll / mTimescale;
  }

  uint64_t mCreationTime;
  uint64_t mModificationTime;
  uint32_t mTimescale;
  uint64_t mDuration;
};

class Trex
{
public:
  Trex(uint32_t aTrackId)
    : mFlags(0)
    , mTrackId(aTrackId)
    , mDefaultSampleDescriptionIndex(0)
    , mDefaultSampleDuration(0)
    , mDefaultSampleSize(0)
    , mDefaultSampleFlags(0)
  {
  }

  Trex(Box& aBox);

  uint32_t mFlags;
  uint32_t mTrackId;
  uint32_t mDefaultSampleDescriptionIndex;
  uint32_t mDefaultSampleDuration;
  uint32_t mDefaultSampleSize;
  uint32_t mDefaultSampleFlags;
};

class Tfhd : public Trex
{
public:
  Tfhd(Trex& aTrex) : Trex(aTrex), mBaseDataOffset(0) {}
  Tfhd(Box& aBox, Trex& aTrex);

  uint64_t mBaseDataOffset;
};

class Tfdt
{
public:
  Tfdt() : mBaseMediaDecodeTime(0) {}
  Tfdt(Box& aBox);

  uint64_t mBaseMediaDecodeTime;
};

struct Sample
{
  mozilla::MediaByteRange mByteRange;
  Interval<Microseconds> mCompositionRange;
  bool mSync;
};

class Moof
{
public:
  Moof(Box& aBox, Trex& aTrex, Mdhd& aMdhd);
  void FixRounding(const Moof& aMoof);

  mozilla::MediaByteRange mRange;
  mozilla::MediaByteRange mMdatRange;
  Interval<Microseconds> mTimeRange;
  nsTArray<Sample> mIndex;

private:
  void ParseTraf(Box& aBox, Trex& aTrex, Mdhd& aMdhd);
  void ParseTrun(Box& aBox, Tfhd& aTfhd, Tfdt& aTfdt, Mdhd& aMdhd);
  uint64_t mMaxRoundingError;
};

class MoofParser
{
public:
  MoofParser(Stream* aSource, uint32_t aTrackId)
    : mSource(aSource), mOffset(0), mTrex(aTrackId)
  {
    
    
  }
  void RebuildFragmentedIndex(
    const nsTArray<mozilla::MediaByteRange>& aByteRanges);
  Interval<Microseconds> GetCompositionRange();
  bool ReachedEnd();
  void ParseMoov(Box& aBox);
  void ParseTrak(Box& aBox);
  void ParseMdia(Box& aBox, Tkhd& aTkhd);
  void ParseMvex(Box& aBox);

  nsRefPtr<Stream> mSource;
  uint64_t mOffset;
  nsTArray<uint64_t> mMoofOffsets;
  Mdhd mMdhd;
  Trex mTrex;
  Tfdt mTfdt;
  nsTArray<Moof> mMoofs;
};
}

#endif
