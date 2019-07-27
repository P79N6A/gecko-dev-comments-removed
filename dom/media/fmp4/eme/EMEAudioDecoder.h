





#ifndef EMEAACDecoder_h_
#define EMEAACDecoder_h_

#include "PlatformDecoderModule.h"
#include "mp4_demuxer/DecoderData.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsServiceManagerUtils.h"
#include "GMPAudioHost.h"
#include "GMPAudioDecoderProxy.h"
#include "SamplesWaitingForKey.h"

namespace mozilla {

class EMEAudioDecoder : public MediaDataDecoder
                      , public GMPAudioDecoderCallbackProxy
{
  typedef mp4_demuxer::MP4Sample MP4Sample;
  typedef mp4_demuxer::AudioDecoderConfig AudioDecoderConfig;
public:
  EMEAudioDecoder(CDMProxy* aProxy,
                  const AudioDecoderConfig& aConfig,
                  MediaTaskQueue* aTaskQueue,
                  MediaDataDecoderCallback* aCallback);

  ~EMEAudioDecoder();

  
  virtual nsresult Init() MOZ_OVERRIDE;
  virtual nsresult Input(MP4Sample* aSample) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual nsresult Drain() MOZ_OVERRIDE;
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual void Decoded(const nsTArray<int16_t>& aPCM,
                       uint64_t aTimeStamp,
                       uint32_t aChannels,
                       uint32_t aRate) MOZ_OVERRIDE;
  virtual void InputDataExhausted() MOZ_OVERRIDE;
  virtual void DrainComplete() MOZ_OVERRIDE;
  virtual void ResetComplete() MOZ_OVERRIDE;
  virtual void Error(GMPErr aErr) MOZ_OVERRIDE;
  virtual void Terminated() MOZ_OVERRIDE;

private:

  class DeliverSample : public nsRunnable {
  public:
    DeliverSample(EMEAudioDecoder* aDecoder,
                  mp4_demuxer::MP4Sample* aSample)
      : mDecoder(aDecoder)
      , mSample(aSample)
    {}

    NS_IMETHOD Run() {
      mDecoder->GmpInput(mSample.forget());
      return NS_OK;
    }
  private:
    nsRefPtr<EMEAudioDecoder> mDecoder;
    nsAutoPtr<mp4_demuxer::MP4Sample> mSample;
  };

  class InitTask : public nsRunnable {
  public:
    explicit InitTask(EMEAudioDecoder* aDecoder)
      : mDecoder(aDecoder)
    {}
    NS_IMETHOD Run() {
      mResult = mDecoder->GmpInit();
      return NS_OK;
    }
    nsresult mResult;
    EMEAudioDecoder* mDecoder;
  };

  nsresult GmpInit();
  nsresult GmpInput(MP4Sample* aSample);
  void GmpFlush();
  void GmpDrain();
  void GmpShutdown();

#ifdef DEBUG
  bool IsOnGMPThread() {
    return NS_GetCurrentThread() == mGMPThread;
  }
#endif

  uint32_t mAudioRate;
  uint32_t mAudioBytesPerSample;
  uint32_t mAudioChannels;
  bool mMustRecaptureAudioPosition;
  int64_t mAudioFrameSum;
  int64_t mAudioFrameOffset;
  int64_t mStreamOffset;

  nsCOMPtr<mozIGeckoMediaPluginService> mMPS;
  nsCOMPtr<nsIThread> mGMPThread;
  nsRefPtr<CDMProxy> mProxy;
  GMPAudioDecoderProxy* mGMP;

  const mp4_demuxer::AudioDecoderConfig& mConfig;
  nsRefPtr<MediaTaskQueue> mTaskQueue;
  MediaDataDecoderCallback* mCallback;

  nsRefPtr<SamplesWaitingForKey> mSamplesWaitingForKey;

  Monitor mMonitor;
  bool mFlushComplete;

#ifdef DEBUG
  bool mIsShutdown;
#endif
};

} 

#endif
