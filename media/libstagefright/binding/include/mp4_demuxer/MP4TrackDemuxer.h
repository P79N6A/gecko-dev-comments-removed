



#ifndef MP4_TRACK_DEMUXER_H_
#define MP4_TRACK_DEMUXER_H_

#include "mozilla/Attributes.h"
#include "demuxer/TrackDemuxer.h"

namespace mp4_demuxer
{

class MP4AudioDemuxer : public mozilla::TrackDemuxer {
public:
  explicit MP4AudioDemuxer(MP4Demuxer* aDemuxer) : mDemuxer(aDemuxer) {}
  virtual void Seek(Microseconds aTime) MOZ_OVERRIDE;
  virtual mozilla::MediaSample* DemuxSample() MOZ_OVERRIDE;
  virtual Microseconds GetNextKeyframeTime() MOZ_OVERRIDE;

private:
  nsRefPtr<MP4Demuxer> mDemuxer;
};

class MP4VideoDemuxer : public mozilla::TrackDemuxer {
public:
  explicit MP4VideoDemuxer(MP4Demuxer* aDemuxer) : mDemuxer(aDemuxer) {}
  virtual void Seek(Microseconds aTime) MOZ_OVERRIDE;
  virtual mozilla::MediaSample* DemuxSample() MOZ_OVERRIDE;
  virtual Microseconds GetNextKeyframeTime() MOZ_OVERRIDE;

private:
  nsRefPtr<MP4Demuxer> mDemuxer;
};

}

#endif
