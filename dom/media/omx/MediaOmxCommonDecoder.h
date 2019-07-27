





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

  virtual void FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                MediaDecoderEventVisibility aEventVisibility) override;
  virtual void ChangeState(PlayState aState) override;
  virtual void CallSeek(const SeekTarget& aTarget) override;
  virtual void SetVolume(double aVolume) override;
  virtual int64_t CurrentPosition() override;
  virtual MediaDecoderOwner::NextFrameStatus NextFrameStatus() override;
  virtual void SetElementVisibility(bool aIsVisible) override;
  virtual void SetPlatformCanOffloadAudio(bool aCanOffloadAudio) override;
  virtual bool CheckDecoderCanOffloadAudio() override;
  virtual void AddOutputStream(ProcessedMediaStream* aStream,
                               bool aFinishWhenEnded) override;
  virtual void SetPlaybackRate(double aPlaybackRate) override;

  void AudioOffloadTearDown();

  virtual MediaDecoderStateMachine* CreateStateMachine() override;

  virtual MediaOmxCommonReader* CreateReader() = 0;
  virtual MediaDecoderStateMachine* CreateStateMachineFromReader(MediaOmxCommonReader* aReader) = 0;

  void NotifyOffloadPlayerPositionChanged() { UpdateLogicalPosition(); }

protected:
  virtual ~MediaOmxCommonDecoder();
  void PauseStateMachine();
  void ResumeStateMachine();

  MediaOmxCommonReader* mReader;

  
  android::sp<android::MediaSource> mAudioTrack;

  nsAutoPtr<AudioOffloadPlayerBase> mAudioOffloadPlayer;

  
  bool mCanOffloadAudio;

  
  
  bool mFallbackToStateMachine;

  
  bool mIsCaptured;
};

} 

#endif 
