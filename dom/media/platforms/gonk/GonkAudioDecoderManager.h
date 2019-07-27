





#if !defined(GonkAudioDecoderManager_h_)
#define GonkAudioDecoderManager_h_

#include "mozilla/RefPtr.h"
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
  GonkAudioDecoderManager(const AudioInfo& aConfig);

  virtual ~GonkAudioDecoderManager() override;

  virtual android::sp<MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) override;

  virtual nsresult Input(MediaRawData* aSample) override;

  virtual nsresult Output(int64_t aStreamOffset,
                          nsRefPtr<MediaData>& aOutput) override;

  virtual nsresult Flush() override;

  virtual bool HasQueuedSample() override;

private:
  bool PerformFormatSpecificProcess(MediaRawData* aSample);

  nsresult CreateAudioData(int64_t aStreamOffset,
                              AudioData** aOutData);

  void ReleaseAudioBuffer();

  const uint32_t mAudioChannels;
  const uint32_t mAudioRate;
  const uint32_t mAudioProfile;
  bool mUseAdts;

  MediaDataDecoderCallback*  mReaderCallback;
  android::MediaBuffer* mAudioBuffer;
  android::sp<ALooper> mLooper;

  
  android::sp<android::MediaCodecProxy> mDecoder;

  
  Monitor mMonitor;

  
  
  
  nsTArray<nsRefPtr<MediaRawData>> mQueueSample;
};

} 

#endif 
