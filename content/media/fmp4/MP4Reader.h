





#if !defined(MP4Reader_h_)
#define MP4Reader_h_

#include "MediaDecoderReader.h"
#include "nsAutoPtr.h"
#include "PlatformDecoderModule.h"
#include "mp4_demuxer/mp4_demuxer.h"
#include "mp4_demuxer/box_definitions.h"

#include <deque>

namespace mozilla {

namespace dom {
class TimeRanges;
}

typedef std::deque<mp4_demuxer::MP4Sample*> MP4SampleQueue;

class MP4Stream;

class MP4Reader : public MediaDecoderReader
{
public:
  MP4Reader(AbstractMediaDecoder* aDecoder);

  virtual ~MP4Reader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor) MOZ_OVERRIDE;

  virtual bool DecodeAudioData() MOZ_OVERRIDE;
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual bool HasAudio() MOZ_OVERRIDE;
  virtual bool HasVideo() MOZ_OVERRIDE;

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags) MOZ_OVERRIDE;

  virtual nsresult Seek(int64_t aTime,
                        int64_t aStartTime,
                        int64_t aEndTime,
                        int64_t aCurrentTime) MOZ_OVERRIDE;

  virtual void OnDecodeThreadStart() MOZ_OVERRIDE;
  virtual void OnDecodeThreadFinish() MOZ_OVERRIDE;

private:

  MP4SampleQueue& SampleQueue(mp4_demuxer::TrackType aTrack);

  
  
  mp4_demuxer::MP4Sample* PopSample(mp4_demuxer::TrackType aTrack);

  bool Decode(mp4_demuxer::TrackType aTrack,
              nsAutoPtr<MediaData>& aOutData);

  MediaDataDecoder* Decoder(mp4_demuxer::TrackType aTrack);

  bool SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed);

  nsAutoPtr<mp4_demuxer::MP4Demuxer> mDemuxer;
  nsAutoPtr<MP4Stream> mMP4Stream;
  nsAutoPtr<PlatformDecoderModule> mPlatform;
  nsAutoPtr<MediaDataDecoder> mVideoDecoder;
  nsAutoPtr<MediaDataDecoder> mAudioDecoder;

  MP4SampleQueue mCompressedAudioQueue;
  MP4SampleQueue mCompressedVideoQueue;

  layers::LayersBackend mLayersBackendType;

  bool mHasAudio;
  bool mHasVideo;

};

} 

#endif
