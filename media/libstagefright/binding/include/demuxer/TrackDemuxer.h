



#ifndef TRACK_DEMUXER_H_
#define TRACK_DEMUXER_H_

template <class T> struct already_AddRefed;

namespace mozilla {

class MediaRawData;
class MediaByteRange;

typedef int64_t Microseconds;

class TrackDemuxer {
public:
  TrackDemuxer() {}
  virtual ~TrackDemuxer() {}

  virtual void Seek(Microseconds aTime) = 0;

  
  virtual already_AddRefed<MediaRawData> DemuxSample() = 0;

  
  
  virtual Microseconds GetNextKeyframeTime() = 0;
};

}

#endif
