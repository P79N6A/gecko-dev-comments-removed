





#include "mp4_demuxer/MP4TrackDemuxer.h"

using namespace mozilla;

namespace mp4_demuxer {

void
MP4AudioDemuxer::Seek(Microseconds aTime)
{
  mDemuxer->SeekAudio(aTime);
}

already_AddRefed<MediaRawData>
MP4AudioDemuxer::DemuxSample()
{
  nsRefPtr<MediaRawData> sample(mDemuxer->DemuxAudioSample());
  return sample.forget();
}

Microseconds
MP4AudioDemuxer::GetNextKeyframeTime()
{
  return -1;
}

void
MP4VideoDemuxer::Seek(Microseconds aTime)
{
  mDemuxer->SeekVideo(aTime);
}

already_AddRefed<MediaRawData>
MP4VideoDemuxer::DemuxSample()
{
  nsRefPtr<MediaRawData> sample(mDemuxer->DemuxVideoSample());
  return sample.forget();
}

Microseconds
MP4VideoDemuxer::GetNextKeyframeTime()
{
  return mDemuxer->GetNextKeyframeTime();
}

}
