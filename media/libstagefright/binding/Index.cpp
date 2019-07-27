



#include "mp4_demuxer/Index.h"
#include "mp4_demuxer/Interval.h"
#include "mp4_demuxer/MoofParser.h"
#include "media/stagefright/MediaSource.h"
#include "MediaResource.h"

#include <algorithm>
#include <limits>

using namespace stagefright;
using namespace mozilla;

namespace mp4_demuxer
{

class MOZ_STACK_CLASS RangeFinder
{
public:
  
  
  
  RangeFinder(const nsTArray<mozilla::MediaByteRange>& ranges)
    : mRanges(ranges), mIndex(0)
  {
    
  }

  bool Contains(MediaByteRange aByteRange);

private:
  const nsTArray<MediaByteRange>& mRanges;
  size_t mIndex;
};

bool
RangeFinder::Contains(MediaByteRange aByteRange)
{
  if (!mRanges.Length()) {
    return false;
  }

  if (mRanges[mIndex].Contains(aByteRange)) {
    return true;
  }

  if (aByteRange.mStart < mRanges[mIndex].mStart) {
    
    do {
      if (!mIndex) {
        return false;
      }
      --mIndex;
      if (mRanges[mIndex].Contains(aByteRange)) {
        return true;
      }
    } while (aByteRange.mStart < mRanges[mIndex].mStart);

    return false;
  }

  while (aByteRange.mEnd > mRanges[mIndex].mEnd) {
    if (mIndex == mRanges.Length() - 1) {
      return false;
    }
    ++mIndex;
    if (mRanges[mIndex].Contains(aByteRange)) {
      return true;
    }
  }

  return false;
}

Index::Index(const stagefright::Vector<MediaSource::Indice>& aIndex,
             Stream* aSource, uint32_t aTrackId)
{
  if (aIndex.isEmpty()) {
    mMoofParser = new MoofParser(aSource, aTrackId);
  } else {
    for (size_t i = 0; i < aIndex.size(); i++) {
      const MediaSource::Indice& indice = aIndex[i];
      Sample sample;
      sample.mByteRange = MediaByteRange(indice.start_offset,
                                         indice.end_offset);
      sample.mCompositionRange = Interval<Microseconds>(indice.start_composition,
                                                        indice.end_composition);
      sample.mSync = indice.sync;
      mIndex.AppendElement(sample);
    }
  }
}

Index::~Index() {}

void
Index::UpdateMoofIndex(const nsTArray<MediaByteRange>& aByteRanges)
{
  if (!mMoofParser) {
    return;
  }

  mMoofParser->RebuildFragmentedIndex(aByteRanges);
}

Microseconds
Index::GetEndCompositionIfBuffered(const nsTArray<MediaByteRange>& aByteRanges)
{
  nsTArray<Sample>* index;
  if (mMoofParser) {
    if (!mMoofParser->ReachedEnd() || mMoofParser->mMoofs.IsEmpty()) {
      return 0;
    }
    index = &mMoofParser->mMoofs.LastElement().mIndex;
  } else {
    index = &mIndex;
  }

  Microseconds lastComposition = 0;
  RangeFinder rangeFinder(aByteRanges);
  for (size_t i = index->Length(); i--;) {
    const Sample& sample = (*index)[i];
    if (!rangeFinder.Contains(sample.mByteRange)) {
      return 0;
    }
    lastComposition = std::max(lastComposition, sample.mCompositionRange.end);
    if (sample.mSync) {
      return lastComposition;
    }
  }
  return 0;
}

void
Index::ConvertByteRangesToTimeRanges(
  const nsTArray<MediaByteRange>& aByteRanges,
  nsTArray<Interval<Microseconds>>* aTimeRanges)
{
  RangeFinder rangeFinder(aByteRanges);
  nsTArray<Interval<Microseconds>> timeRanges;

  nsTArray<nsTArray<Sample>*> indexes;
  if (mMoofParser) {
    
    
    
    for (int i = 0; i < mMoofParser->mMoofs.Length(); i++) {
      Moof& moof = mMoofParser->mMoofs[i];

      
      if (rangeFinder.Contains(moof.mRange)) {
        if (rangeFinder.Contains(moof.mMdatRange)) {
          Interval<Microseconds>::SemiNormalAppend(timeRanges, moof.mTimeRange);
        } else {
          indexes.AppendElement(&moof.mIndex);
        }
      }
    }
  } else {
    indexes.AppendElement(&mIndex);
  }

  bool hasSync = false;
  for (size_t i = 0; i < indexes.Length(); i++) {
    nsTArray<Sample>* index = indexes[i];
    for (size_t j = 0; j < index->Length(); j++) {
      const Sample& sample = (*index)[j];
      if (!rangeFinder.Contains(sample.mByteRange)) {
        
        
        hasSync = false;
        continue;
      }

      hasSync |= sample.mSync;
      if (!hasSync) {
        continue;
      }

      Interval<Microseconds>::SemiNormalAppend(timeRanges,
                                               sample.mCompositionRange);
    }
  }

  
  Interval<Microseconds>::Normalize(timeRanges, aTimeRanges);
}

uint64_t
Index::GetEvictionOffset(Microseconds aTime)
{
  uint64_t offset = std::numeric_limits<uint64_t>::max();
  if (mMoofParser) {
    
    
    for (int i = 0; i < mMoofParser->mMoofs.Length(); i++) {
      Moof& moof = mMoofParser->mMoofs[i];

      if (moof.mTimeRange.Length() && moof.mTimeRange.end > aTime) {
        offset = std::min(offset, uint64_t(std::min(moof.mRange.mStart,
                                                    moof.mMdatRange.mStart)));
      }
    }
  } else {
    
    
    for (size_t i = 0; i < mIndex.Length(); i++) {
      const Sample& sample = mIndex[i];
      if (aTime >= sample.mCompositionRange.end) {
        offset = std::min(offset, uint64_t(sample.mByteRange.mEnd));
      }
    }
  }
  return offset;
}
}
