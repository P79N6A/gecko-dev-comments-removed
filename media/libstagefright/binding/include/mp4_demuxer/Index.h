



#ifndef INDEX_H_
#define INDEX_H_

#include "media/stagefright/MediaSource.h"
#include "mozilla/Monitor.h"
#include "mp4_demuxer/mp4_demuxer.h"

namespace mp4_demuxer
{

template <typename T> class Interval;
class MoofParser;

class Index
{
public:
  Index(const stagefright::Vector<stagefright::MediaSource::Indice>& aIndex,
        Stream* aSource, uint32_t aTrackId);
  void ConvertByteRangesToTimeRanges(
    const nsTArray<mozilla::MediaByteRange>& aByteRanges,
    nsTArray<Interval<Microseconds>>* aTimeRanges);

private:
  mozilla::Monitor mMonitor;
  nsTArray<stagefright::MediaSource::Indice> mIndex;
  nsAutoPtr<MoofParser> mMoofParser;
};
}

#endif
