



#ifndef TRACK_DEMUXER_H_
#define TRACK_DEMUXER_H_

namespace mp4_demuxer { class MP4Sample; }

namespace mozilla {

class MediaByteRange;

typedef int64_t Microseconds;

class MediaSample {
public:
  explicit MediaSample(mp4_demuxer::MP4Sample* aMp4Sample) : mMp4Sample(aMp4Sample)
  {
  }

  nsAutoPtr<mp4_demuxer::MP4Sample> mMp4Sample;
};

class TrackDemuxer {
public:
  TrackDemuxer() {}
  virtual ~TrackDemuxer() {}

  virtual void Seek(Microseconds aTime) = 0;

  
  virtual MediaSample* DemuxSample() = 0;

  
  
  virtual Microseconds GetNextKeyframeTime() = 0;
};

}

#endif
