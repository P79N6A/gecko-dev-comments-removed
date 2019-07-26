




#if !defined(MediaOmxDecoder_h_)
#define MediaOmxDecoder_h_

#include "base/basictypes.h"
#include "MediaDecoder.h"
#include "MediaOmxReader.h"
#include "AudioOffloadPlayerBase.h"

namespace mozilla {

class MediaOmxDecoder : public MediaDecoder
{
  typedef android::MediaSource MediaSource;
public:
  MediaOmxDecoder();
  virtual MediaDecoder* Clone();
  virtual MediaDecoderStateMachine* CreateStateMachine();

  virtual void MetadataLoaded(int aChannels,
                              int aRate,
                              bool aHasAudio,
                              bool aHasVideo,
                              MetadataTags* aTags);
  virtual void ChangeState(PlayState aState);
  virtual void ApplyStateToStateMachine(PlayState aState);
  virtual void SetVolume(double aVolume);
  virtual void PlaybackPositionChanged();
  virtual void UpdateReadyStateForData();
  virtual void SetElementVisibility(bool aIsVisible);
  virtual void SetCanOffloadAudio(bool aCanOffloadAudio);
  virtual void AddOutputStream(ProcessedMediaStream* aStream,
                               bool aFinishWhenEnded);
  virtual void SetPlaybackRate(double aPlaybackRate);

  void AudioOffloadTearDown();
  int64_t GetSeekTime() { return mRequestedSeekTarget.mTime; }
  void ResetSeekTime() { mRequestedSeekTarget.Reset(); }

private:
  void PauseStateMachine();
  void ResumeStateMachine();

  MediaOmxReader* mReader;

  
  android::sp<MediaSource> mAudioTrack;

  nsAutoPtr<AudioOffloadPlayerBase> mAudioOffloadPlayer;

  
  bool mCanOffloadAudio;

  
  
  bool mFallbackToStateMachine;
};

} 

#endif
