





#if !defined(GonkMediaDataDecoder_h_)
#define GonkMediaDataDecoder_h_
#include "PlatformDecoderModule.h"

namespace android {
class MediaCodecProxy;
} 

namespace mozilla {
class MediaRawData;


class GonkDecoderManager {
public:
  virtual ~GonkDecoderManager() {}

  
  
  virtual android::sp<android::MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) = 0;

  
  virtual nsresult Input(MediaRawData* aSample) = 0;

  
  
  
  
  
  
  virtual nsresult Output(int64_t aStreamOffset,
                          nsRefPtr<MediaData>& aOutput) = 0;

  
  virtual nsresult Flush() = 0;

  
  virtual bool HasQueuedSample() = 0;

protected:
  nsRefPtr<MediaByteBuffer> mCodecSpecificData;

  nsAutoCString mMimeType;
};






class GonkMediaDataDecoder : public MediaDataDecoder {
public:
  GonkMediaDataDecoder(GonkDecoderManager* aDecoderManager,
                       FlushableTaskQueue* aTaskQueue,
                       MediaDataDecoderCallback* aCallback);

  ~GonkMediaDataDecoder();

  virtual nsresult Init() override;

  virtual nsresult Input(MediaRawData* aSample);

  virtual nsresult Flush() override;

  virtual nsresult Drain() override;

  virtual nsresult Shutdown() override;

private:

  
  
  
  
  void ProcessDecode(MediaRawData* aSample);

  
  
  void ProcessOutput();

  
  
  void ProcessDrain();

  RefPtr<FlushableTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  android::sp<android::MediaCodecProxy> mDecoder;
  nsAutoPtr<GonkDecoderManager> mManager;

  
  
  int64_t mLastStreamOffset;
  
  bool mSignaledEOS;
  
  bool mDrainComplete;
};

} 

#endif 
