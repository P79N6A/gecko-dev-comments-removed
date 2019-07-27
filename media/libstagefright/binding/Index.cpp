



#include "mp4_demuxer/Index.h"
#include "mp4_demuxer/Interval.h"
#include "mp4_demuxer/MoofParser.h"
#include "media/stagefright/MediaSource.h"
#include "MediaResource.h"

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
  : mMonitor("mp4_demuxer::Index")
{
  if (aIndex.isEmpty()) {
    mMoofParser = new MoofParser(aSource, aTrackId);
  } else {
    mIndex.AppendElements(&aIndex[0], aIndex.size());
  }
}

Index::~Index() {}

void
Index::UpdateMoofIndex(const nsTArray<MediaByteRange>& aByteRanges)
{
  if (!mMoofParser) {
    return;
  }

  MonitorAutoLock mon(mMonitor);
  mMoofParser->RebuildFragmentedIndex(aByteRanges);
}

void
Index::ConvertByteRangesToTimeRanges(
  const nsTArray<MediaByteRange>& aByteRanges,
  nsTArray<Interval<Microseconds>>* aTimeRanges)
{
  MonitorAutoLock mon(mMonitor);

  RangeFinder rangeFinder(aByteRanges);
  nsTArray<Interval<Microseconds>> timeRanges;

  nsTArray<nsTArray<stagefright::MediaSource::Indice>*> indexes;
  if (mMoofParser) {
    
    
    
    for (int i = 0; i < mMoofParser->mMoofs.Length(); i++) {
      Moof& moof = mMoofParser->mMoofs[i];

      
      if (rangeFinder.Contains(moof.mRange)) {
        if (rangeFinder.Contains(moof.mMdatRange)) {
          timeRanges.AppendElements(moof.mTimeRanges);
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
    nsTArray<stagefright::MediaSource::Indice>* index = indexes[i];
    for (size_t j = 0; j < index->Length(); j++) {
      const MediaSource::Indice& indice = (*index)[j];
      if (!rangeFinder.Contains(
             MediaByteRange(indice.start_offset, indice.end_offset))) {
        
        
        hasSync = false;
        continue;
      }

      hasSync |= indice.sync;
      if (!hasSync) {
        continue;
      }

      Interval<Microseconds>::SemiNormalAppend(
        timeRanges, Interval<Microseconds>(indice.start_composition,
                                           indice.end_composition));
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

      if (!moof.mTimeRanges.IsEmpty() && moof.mTimeRanges[0].end > aTime) {
        offset = std::min(offset, uint64_t(std::min(moof.mRange.mStart,
                                                    moof.mMdatRange.mStart)));
      }
    }
  } else {
    
    
    for (size_t i = 0; i < mIndex.Length(); i++) {
      const MediaSource::Indice& indice = mIndex[i];
      if (aTime >= indice.start_composition) {
        offset = std::min(offset, indice.start_offset);
      }
    }
  }
  return offset;
}
}
