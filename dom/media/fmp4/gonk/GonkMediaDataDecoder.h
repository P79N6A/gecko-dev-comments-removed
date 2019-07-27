





#if !defined(GonkMediaDataDecoder_h_)
#define GonkMediaDataDecoder_h_
#include "mozilla/RefPtr.h"
#include "MP4Reader.h"

namespace android {
class MediaCodecProxy;
} 

namespace mozilla {
class MediaRawData;


class GonkDecoderManager {
public:
  GonkDecoderManager(MediaTaskQueue* aTaskQueue);

  virtual ~GonkDecoderManager() {}

  
  
  virtual android::sp<android::MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) = 0;

  
  virtual nsresult Input(MediaRawData* aSample);

  
  
  
  
  
  
  virtual nsresult Output(int64_t aStreamOffset,
                          nsRefPtr<MediaData>& aOutput) = 0;

  
  
  
  virtual nsresult Flush();

  
  bool HasQueuedSample() {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    return mQueueSample.Length();
  }

  void ClearQueuedSample() {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    mQueueSample.Clear();
  }

protected:
  
  
  virtual bool PerformFormatSpecificProcess(MediaRawData* aSample) { return true; }

  
  virtual android::status_t SendSampleToOMX(MediaRawData* aSample) = 0;

  
  
  
  nsTArray<nsRefPtr<MediaRawData>> mQueueSample;

  RefPtr<MediaTaskQueue> mTaskQueue;
};






class GonkMediaDataDecoder : public MediaDataDecoder {
public:
  GonkMediaDataDecoder(GonkDecoderManager* aDecoderManager,
                       FlushableMediaTaskQueue* aTaskQueue,
                       MediaDataDecoderCallback* aCallback);

  ~GonkMediaDataDecoder();

  virtual nsresult Init() override;

  virtual nsresult Input(MediaRawData* aSample);

  virtual nsresult Flush() override;

  virtual nsresult Drain() override;

  virtual nsresult Shutdown() override;

  virtual bool IsWaitingMediaResources() override;

private:

  
  
  
  
  void ProcessDecode(MediaRawData* aSample);

  
  
  void ProcessOutput();

  
  
  void ProcessDrain();

  RefPtr<FlushableMediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  android::sp<android::MediaCodecProxy> mDecoder;
  nsAutoPtr<GonkDecoderManager> mManager;

  
  
  int64_t mLastStreamOffset;
  
  bool mSignaledEOS;
  
  bool mDrainComplete;
};

} 

#endif 
