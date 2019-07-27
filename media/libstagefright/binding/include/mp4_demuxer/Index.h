



#ifndef INDEX_H_
#define INDEX_H_

#include "media/stagefright/MediaSource.h"
#include "mp4_demuxer/mp4_demuxer.h"

namespace mp4_demuxer
{

template <typename T> class Interval;

class Index
{
public:
  Index() {}

  void Init(
    const stagefright::Vector<stagefright::MediaSource::Indice>& aIndex);
  void ConvertByteRangesToTimeRanges(
    const nsTArray<mozilla::MediaByteRange>& aByteRanges,
    nsTArray<Interval<Microseconds>>* aTimeRanges);

private:
  nsTArray<stagefright::MediaSource::Indice> mIndex;
};
}

#endif
