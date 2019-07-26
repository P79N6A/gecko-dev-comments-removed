




#ifndef MediaDecoderOwner_h_
#define MediaDecoderOwner_h_

#include "nsBuiltinDecoder.h"

namespace mozilla {

class MediaDecoderOwner
{
public:
  
  
  virtual void DownloadStalled() = 0;

  
  virtual nsresult DispatchEvent(const nsAString& aName) = 0;

  
  virtual nsresult DispatchAsyncEvent(const nsAString& aName) = 0;

  





  virtual void FireTimeUpdate(bool aPeriodic) = 0;

  
  
  virtual nsHTMLMediaElement* GetMediaElement()
  {
    return nullptr;
  }

  
  virtual bool GetPaused() = 0;

  


  virtual void NotifyAudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength,
                                    float aTime) = 0;

  
  
  
  virtual void MetadataLoaded(uint32_t aChannels,
                              uint32_t aRate,
                              bool aHasAudio,
                              const MetadataTags* aTags) = 0;

  
  
  
  
  virtual void FirstFrameLoaded(bool aResourceFullyLoaded) = 0;

  
  
  virtual void ResourceLoaded() = 0;

  
  
  virtual void NetworkError() = 0;

  
  
  virtual void DecodeError() = 0;

  
  
  virtual void LoadAborted() = 0;

  
  
  virtual void PlaybackEnded() = 0;

  
  
  virtual void SeekStarted() = 0;

  
  
  virtual void SeekCompleted() = 0;

  
  
  
  virtual void DownloadSuspended() = 0;

  
  
  
  
  
  
  
  virtual void DownloadResumed(bool aForceNetworkLoading = false) = 0;

  
  
  
  virtual void NotifyAutoplayDataReady() = 0;

  
  
  virtual void NotifySuspendedByCache(bool aIsSuspended) = 0;

  
  virtual void NotifyDecoderPrincipalChanged() = 0;

  
  
  
  
  
  virtual void UpdateReadyStateForData(nsBuiltinDecoder::NextFrameStatus aNextFrame) = 0;

  
  
  virtual mozilla::VideoFrameContainer* GetVideoFrameContainer() = 0;
};

}

#endif

