




#ifndef MediaDecoderOwner_h_
#define MediaDecoderOwner_h_
#include "AbstractMediaDecoder.h"

namespace mozilla {

class VideoFrameContainer;

namespace dom {
class HTMLMediaElement;
}

class MediaDecoderOwner
{
public:
  
  
  virtual void DownloadStalled() = 0;

  
  virtual nsresult DispatchEvent(const nsAString& aName) = 0;

  
  virtual nsresult DispatchAsyncEvent(const nsAString& aName) = 0;

  





  virtual void FireTimeUpdate(bool aPeriodic) = 0;

  
  
  virtual dom::HTMLMediaElement* GetMediaElement()
  {
    return nullptr;
  }

  
  virtual bool GetPaused() = 0;

  


  virtual void NotifyAudioAvailable(float* aFrameBuffer, uint32_t aFrameBufferLength,
                                    float aTime) = 0;

  
  
  
  virtual void MetadataLoaded(int aChannels,
                              int aRate,
                              bool aHasAudio,
                              bool aHasVideo,
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

  
  
  virtual void NotifySuspendedByCache(bool aIsSuspended) = 0;

  
  virtual void NotifyDecoderPrincipalChanged() = 0;

  
  enum NextFrameStatus {
    
    NEXT_FRAME_AVAILABLE,
    
    
    NEXT_FRAME_UNAVAILABLE_BUFFERING,
    
    NEXT_FRAME_UNAVAILABLE,
    
    NEXT_FRAME_UNINITIALIZED
  };

  
  
  
  
  
  virtual void UpdateReadyStateForData(NextFrameStatus aNextFrame) = 0;

  
  
  virtual VideoFrameContainer* GetVideoFrameContainer() = 0;
};

}

#endif

