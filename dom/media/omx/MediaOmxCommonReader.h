





#ifndef MEDIA_OMX_COMMON_READER_H
#define MEDIA_OMX_COMMON_READER_H

#include "MediaDecoderReader.h"

#include <utils/RefBase.h>

#include "mozilla/dom/AudioChannelBinding.h"

namespace android {
struct MOZ_EXPORT MediaSource;
} 

namespace mozilla {

class AbstractMediaDecoder;

class MediaOmxCommonReader : public MediaDecoderReader
{
public:
  MediaOmxCommonReader(AbstractMediaDecoder* aDecoder);

  void SetAudioChannel(dom::AudioChannel aAudioChannel) {
    mAudioChannel = aAudioChannel;
  }

  virtual android::sp<android::MediaSource> GetAudioOffloadTrack() = 0;

#ifdef MOZ_AUDIO_OFFLOAD
  
  
  
  void CheckAudioOffload();
#endif

protected:
  dom::AudioChannel mAudioChannel;
};

} 

#endif 
