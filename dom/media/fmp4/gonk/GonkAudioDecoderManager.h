





#if !defined(GonkAudioDecoderManager_h_)
#define GonkAudioDecoderManager_h_

#include "mozilla/RefPtr.h"
#include "MP4Reader.h"
#include "GonkMediaDataDecoder.h"

using namespace android;

namespace android {
struct MOZ_EXPORT ALooper;
class MOZ_EXPORT MediaBuffer;
} 

namespace mozilla {

class GonkAudioDecoderManager : public GonkDecoderManager {
typedef android::MediaCodecProxy MediaCodecProxy;
public:
  GonkAudioDecoderManager(const mp4_demuxer::AudioDecoderConfig& aConfig);
  ~GonkAudioDecoderManager();

  virtual android::sp<MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) MOZ_OVERRIDE;

  virtual nsresult Output(int64_t aStreamOffset,
                          nsAutoPtr<MediaData>& aOutput) MOZ_OVERRIDE;
private:

  nsresult CreateAudioData(int64_t aStreamOffset,
                              AudioData** aOutData);

  void ReleaseAudioBuffer();
  
  android::sp<MediaCodecProxy> mDecoder;

  const uint32_t mAudioChannels;
  const uint32_t mAudioRate;
  const uint32_t mAudioProfile;
  nsTArray<uint8_t> mUserData;

  MediaDataDecoderCallback*  mReaderCallback;
  android::MediaBuffer* mAudioBuffer;
  android::sp<ALooper> mLooper;
};

} 

#endif 
