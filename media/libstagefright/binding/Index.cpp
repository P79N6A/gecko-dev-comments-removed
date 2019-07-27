



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

void
Index::ConvertByteRangesToTimeRanges(
  const nsTArray<MediaByteRange>& aByteRanges,
  nsTArray<Interval<Microseconds>>* aTimeRanges)
{
  nsTArray<stagefright::MediaSource::Indice> moofIndex;
  nsTArray<stagefright::MediaSource::Indice>* index;
  if (mMoofParser) {
    {
      MonitorAutoLock mon(mMonitor);
      mMoofParser->RebuildFragmentedIndex(aByteRanges);

      
      
      
      moofIndex = mMoofParser->mIndex;
    }
    index = &moofIndex;
  } else {
    index = &mIndex;
  }

  nsTArray<Interval<Microseconds>> timeRanges;
  RangeFinder rangeFinder(aByteRanges);

  bool hasSync = false;
  for (size_t i = 0; i < index->Length(); i++) {
    const MediaSource::Indice& indice = (*index)[i];
    if (!rangeFinder.Contains(MediaByteRange(indice.start_offset,
                                             indice.end_offset))) {
      
      
      hasSync = false;
      continue;
    }

    hasSync |= indice.sync;
    if (!hasSync) {
      continue;
    }

    
    
    size_t s = timeRanges.Length();
    if (s && timeRanges[s - 1].end == indice.start_composition) {
      timeRanges[s - 1].end = indice.end_composition;
    } else {
      timeRanges.AppendElement(Interval<Microseconds>(indice.start_composition,
                                                      indice.end_composition));
    }
  }

  
  Interval<Microseconds>::Normalize(timeRanges, aTimeRanges);
}
}
