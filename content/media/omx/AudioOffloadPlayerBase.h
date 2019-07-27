


















#ifndef AUDIO_OFFLOAD_PLAYER_BASE_H_
#define AUDIO_OFFLOAD_PLAYER_BASE_H_

#include "MediaDecoder.h"
#include "MediaDecoderOwner.h"

namespace mozilla {





class AudioOffloadPlayerBase
{
  typedef android::status_t status_t;
  typedef android::MediaSource MediaSource;

public:
  virtual ~AudioOffloadPlayerBase() {};

  
  virtual void SetSource(const android::sp<MediaSource> &aSource) {}

  
  
  virtual status_t Start(bool aSourceAlreadyStarted = false)
  {
    return android::NO_INIT;
  }

  virtual status_t ChangeState(MediaDecoder::PlayState aState)
  {
    return android::NO_INIT;
  }

  virtual void SetVolume(double aVolume) {}

  virtual double GetMediaTimeSecs() { return 0; }

  
  virtual void SetElementVisibility(bool aIsVisible) {}

  
  
  
  virtual MediaDecoderOwner::NextFrameStatus GetNextFrameStatus()
  {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
  }
};

} 

#endif 
