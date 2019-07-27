





#ifndef MEDIA_OMX_COMMON_DECODER_H
#define MEDIA_OMX_COMMON_DECODER_H

#include "MediaDecoder.h"

namespace android {
struct MOZ_EXPORT MediaSource;
} 

namespace mozilla {

class AudioOffloadPlayerBase;
class MediaOmxCommonReader;

class MediaOmxCommonDecoder : public MediaDecoder
{
public:
  MediaOmxCommonDecoder();

  virtual void MetadataLoaded(MediaInfo* aInfo,
                              MetadataTags* aTags);
  virtual void ChangeState(PlayState aState);
  virtual void ApplyStateToStateMachine(PlayState aState);
  virtual void SetVolume(double aVolume);
  virtual void PlaybackPositionChanged();
  virtual void UpdateReadyStateForData();
  virtual void SetElementVisibility(bool aIsVisible);
  virtual void SetPlatformCanOffloadAudio(bool aCanOffloadAudio);
  virtual bool CheckDecoderCanOffloadAudio();
  virtual void AddOutputStream(ProcessedMediaStream* aStream,
                               bool aFinishWhenEnded);
  virtual void SetPlaybackRate(double aPlaybackRate);

  void AudioOffloadTearDown();

  virtual MediaDecoderStateMachine* CreateStateMachine();

  virtual MediaOmxCommonReader* CreateReader() = 0;
  virtual MediaDecoderStateMachine* CreateStateMachine(MediaOmxCommonReader* aReader) = 0;

protected:
  virtual ~MediaOmxCommonDecoder();
  void PauseStateMachine();
  void ResumeStateMachine();

  MediaOmxCommonReader* mReader;

  
  android::sp<android::MediaSource> mAudioTrack;

  nsAutoPtr<AudioOffloadPlayerBase> mAudioOffloadPlayer;

  
  bool mCanOffloadAudio;

  
  
  bool mFallbackToStateMachine;
};

} 

#endif 
