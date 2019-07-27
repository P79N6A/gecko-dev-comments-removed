





#if !defined(GonkMediaDataDecoder_h_)
#define GonkMediaDataDecoder_h_
#include "mp4_demuxer/mp4_demuxer.h"
#include "mozilla/RefPtr.h"
#include "MP4Reader.h"

namespace android {
class MediaCodecProxy;
} 

namespace mozilla {


class GonkDecoderManager {
public:
  GonkDecoderManager(MediaTaskQueue* aTaskQueue);

  virtual ~GonkDecoderManager() {}

  
  
  virtual android::sp<android::MediaCodecProxy> Init(MediaDataDecoderCallback* aCallback) = 0;

  
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample);

  
  
  
  
  
  
  virtual nsresult Output(int64_t aStreamOffset,
                          nsRefPtr<MediaData>& aOutput) = 0;

  
  
  
  virtual nsresult Flush();

  virtual void AllocateMediaResources() {}

  virtual void ReleaseMediaResources() {}

  
  bool HasQueuedSample() {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    return mQueueSample.Length();
  }

  void ClearQueuedSample() {
    MOZ_ASSERT(mTaskQueue->IsCurrentThreadIn());
    mQueueSample.Clear();
  }

protected:
  
  
  virtual void PerformFormatSpecificProcess(mp4_demuxer::MP4Sample* aSample) {}

  
  virtual android::status_t SendSampleToOMX(mp4_demuxer::MP4Sample* aSample) = 0;

  
  
  
  nsTArray<nsAutoPtr<mp4_demuxer::MP4Sample>> mQueueSample;

  RefPtr<MediaTaskQueue> mTaskQueue;
};






class GonkMediaDataDecoder : public MediaDataDecoder {
public:
  GonkMediaDataDecoder(GonkDecoderManager* aDecoderManager,
                       MediaTaskQueue* aTaskQueue,
                       MediaDataDecoderCallback* aCallback);

  ~GonkMediaDataDecoder();

  virtual nsresult Init() MOZ_OVERRIDE;

  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample);

  virtual nsresult Flush() MOZ_OVERRIDE;

  virtual nsresult Drain() MOZ_OVERRIDE;

  virtual nsresult Shutdown() MOZ_OVERRIDE;

  virtual bool IsWaitingMediaResources() MOZ_OVERRIDE;

  virtual bool IsDormantNeeded() { return true;}

  virtual void AllocateMediaResources() MOZ_OVERRIDE;

  virtual void ReleaseMediaResources() MOZ_OVERRIDE;

private:

  
  
  
  
  void ProcessDecode(mp4_demuxer::MP4Sample* aSample);

  
  
  void ProcessOutput();

  
  
  void ProcessDrain();

  RefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  android::sp<android::MediaCodecProxy> mDecoder;
  nsAutoPtr<GonkDecoderManager> mManager;

  
  
  int64_t mLastStreamOffset;
  
  bool mSignaledEOS;
  
  bool mDrainComplete;
};

} 

#endif 
