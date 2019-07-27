



#include "mp4_demuxer/Index.h"
#include "mp4_demuxer/Interval.h"
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

void
Index::Init(const stagefright::Vector<MediaSource::Indice>& aIndex)
{
  MOZ_ASSERT(mIndex.IsEmpty());
  if (!aIndex.isEmpty()) {
    mIndex.AppendElements(&aIndex[0], aIndex.size());
  }
}

void
Index::ConvertByteRangesToTimeRanges(
  const nsTArray<MediaByteRange>& aByteRanges,
  nsTArray<Interval<Microseconds>>* aTimeRanges)
{
  nsTArray<Interval<Microseconds>> timeRanges;
  RangeFinder rangeFinder(aByteRanges);

  bool hasSync = false;
  for (size_t i = 0; i < mIndex.Length(); i++) {
    const MediaSource::Indice& indice = mIndex[i];
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
