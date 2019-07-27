



#ifndef MOOF_PARSER_H_
#define MOOF_PARSER_H_

#include "mp4_demuxer/AtomType.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "MediaResource.h"

namespace mp4_demuxer {

class Stream;
class Box;
class BoxContext;
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
  explicit Tkhd(Box& aBox);

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
  explicit Mdhd(Box& aBox);

  Microseconds ToMicroseconds(int64_t aTimescaleUnits)
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
  explicit Trex(uint32_t aTrackId)
    : mFlags(0)
    , mTrackId(aTrackId)
    , mDefaultSampleDescriptionIndex(0)
    , mDefaultSampleDuration(0)
    , mDefaultSampleSize(0)
    , mDefaultSampleFlags(0)
  {
  }

  explicit Trex(Box& aBox);

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
  explicit Tfhd(Trex& aTrex) : Trex(aTrex), mBaseDataOffset(0) {}
  Tfhd(Box& aBox, Trex& aTrex);

  uint64_t mBaseDataOffset;
};

class Tfdt
{
public:
  Tfdt() : mBaseMediaDecodeTime(0) {}
  explicit Tfdt(Box& aBox);

  uint64_t mBaseMediaDecodeTime;
};

class Edts
{
public:
  Edts() : mMediaStart(0) {}
  explicit Edts(Box& aBox);

  int64_t mMediaStart;
};

struct Sample
{
  mozilla::MediaByteRange mByteRange;
  mozilla::MediaByteRange mCencRange;
  Microseconds mDecodeTime;
  Interval<Microseconds> mCompositionRange;
  bool mSync;
};

class Saiz
{
public:
  Saiz(Box& aBox);

  AtomType mAuxInfoType;
  uint32_t mAuxInfoTypeParameter;
  nsTArray<uint8_t> mSampleInfoSize;
};

class Saio
{
public:
  Saio(Box& aBox);

  AtomType mAuxInfoType;
  uint32_t mAuxInfoTypeParameter;
  nsTArray<uint64_t> mOffsets;
};

class AuxInfo {
public:
  AuxInfo(int64_t aMoofOffset, Saiz& aSaiz, Saio& aSaio);
  bool GetByteRanges(nsTArray<MediaByteRange>* aByteRanges);

private:

  int64_t mMoofOffset;
  Saiz& mSaiz;
  Saio& mSaio;
};

class Moof
{
public:
  Moof(Box& aBox, Trex& aTrex, Mdhd& aMdhd, Edts& aEdts);
  bool GetAuxInfo(AtomType aType, nsTArray<MediaByteRange>* aByteRanges);
  void FixRounding(const Moof& aMoof);

  mozilla::MediaByteRange mRange;
  mozilla::MediaByteRange mMdatRange;
  Interval<Microseconds> mTimeRange;
  nsTArray<Sample> mIndex;

  nsTArray<Saiz> mSaizs;
  nsTArray<Saio> mSaios;

private:
  void ParseTraf(Box& aBox, Trex& aTrex, Mdhd& aMdhd, Edts& aEdts);
  void ParseTrun(Box& aBox, Tfhd& aTfhd, Tfdt& aTfdt, Mdhd& aMdhd, Edts& aEdts);
  void ParseSaiz(Box& aBox);
  void ParseSaio(Box& aBox);
  bool ProcessCenc();
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
  void RebuildFragmentedIndex(BoxContext& aContext);
  Interval<Microseconds> GetCompositionRange(
    const nsTArray<mozilla::MediaByteRange>& aByteRanges);
  bool ReachedEnd();
  void ParseMoov(Box& aBox);
  void ParseTrak(Box& aBox);
  void ParseMdia(Box& aBox, Tkhd& aTkhd);
  void ParseMvex(Box& aBox);

  bool BlockingReadNextMoof();

  mozilla::MediaByteRange mInitRange;
  nsRefPtr<Stream> mSource;
  uint64_t mOffset;
  nsTArray<uint64_t> mMoofOffsets;
  Mdhd mMdhd;
  Trex mTrex;
  Tfdt mTfdt;
  Edts mEdts;
  nsTArray<Moof> mMoofs;
};
}

#endif
