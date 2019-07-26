




#if !defined(MediaOmxReader_h_)
#define MediaOmxReader_h_

#include "MediaResource.h"
#include "MediaDecoderReader.h"
#include "nsRect.h"
#include "mozilla/dom/AudioChannelBinding.h"
#include <ui/GraphicBuffer.h>
#include <stagefright/MediaSource.h>

namespace android {
class OmxDecoder;
class MOZ_EXPORT MediaExtractor;
}

namespace mozilla {

namespace dom {
  class TimeRanges;
}

class AbstractMediaDecoder;

class MediaOmxReader : public MediaDecoderReader
{
  nsCString mType;
  bool mHasVideo;
  bool mHasAudio;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;
  int64_t mVideoSeekTimeUs;
  int64_t mAudioSeekTimeUs;
  int32_t mSkipCount;
  dom::AudioChannel mAudioChannel;
  android::sp<android::MediaSource> mAudioOffloadTrack;

protected:
  android::sp<android::OmxDecoder> mOmxDecoder;
  android::sp<android::MediaExtractor> mExtractor;

  
  
  
  
  virtual nsresult InitOmxDecoder();

  
  
  virtual void EnsureActive();

public:
  MediaOmxReader(AbstractMediaDecoder* aDecoder);
  ~MediaOmxReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    return mHasAudio;
  }

  virtual bool HasVideo()
  {
    return mHasVideo;
  }

  virtual bool IsWaitingMediaResources();

  virtual bool IsDormantNeeded();
  virtual void ReleaseMediaResources();

  virtual void ReleaseDecoder() MOZ_OVERRIDE;

  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
  virtual nsresult Seek(int64_t aTime, int64_t aStartTime, int64_t aEndTime, int64_t aCurrentTime);

  virtual void SetIdle() MOZ_OVERRIDE;

  void SetAudioChannel(dom::AudioChannel aAudioChannel) {
    mAudioChannel = aAudioChannel;
  }

  android::sp<android::MediaSource> GetAudioOffloadTrack() {
    return mAudioOffloadTrack;
  }

#ifdef MOZ_AUDIO_OFFLOAD
  
  
  
  void CheckAudioOffload();
#endif
};

} 

#endif
