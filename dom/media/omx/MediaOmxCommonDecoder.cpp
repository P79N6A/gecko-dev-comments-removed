





#include "MediaOmxCommonDecoder.h"

#include <stagefright/MediaSource.h>

#include "AudioOffloadPlayerBase.h"
#include "MediaDecoderStateMachine.h"
#include "MediaOmxCommonReader.h"

#ifdef MOZ_AUDIO_OFFLOAD
#include "AudioOffloadPlayer.h"
#endif

using namespace android;

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define DECODER_LOG(type, msg)
#endif

MediaOmxCommonDecoder::MediaOmxCommonDecoder()
  : MediaDecoder()
  , mReader(nullptr)
  , mCanOffloadAudio(false)
  , mFallbackToStateMachine(false)
{
#ifdef PR_LOGGING
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
#endif
}

MediaOmxCommonDecoder::~MediaOmxCommonDecoder() {}

void
MediaOmxCommonDecoder::SetPlatformCanOffloadAudio(bool aCanOffloadAudio)
{
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  mCanOffloadAudio = aCanOffloadAudio;
}

bool
MediaOmxCommonDecoder::CheckDecoderCanOffloadAudio()
{
  return (mCanOffloadAudio && !mFallbackToStateMachine && !mOutputStreams.Length() &&
      mInitialPlaybackRate == 1.0);
}

void
MediaOmxCommonDecoder::FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                        MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());
  MediaDecoder::FirstFrameLoaded(aInfo, aEventVisibility);

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!CheckDecoderCanOffloadAudio()) {
    DECODER_LOG(PR_LOG_DEBUG, ("In %s Offload Audio check failed",
        __PRETTY_FUNCTION__));
    return;
  }

#ifdef MOZ_AUDIO_OFFLOAD
  mAudioOffloadPlayer = new AudioOffloadPlayer(this);
#endif
  if (!mAudioOffloadPlayer) {
    return;
  }

  mAudioOffloadPlayer->SetSource(mReader->GetAudioOffloadTrack());
  status_t err = mAudioOffloadPlayer->Start(false);
  if (err == OK) {
    PauseStateMachine();
    
    ChangeState(mPlayState);
    return;
  }

  mAudioOffloadPlayer = nullptr;
  mFallbackToStateMachine = true;
  DECODER_LOG(PR_LOG_DEBUG, ("In %s Unable to start offload audio %d."
      "Switching to normal mode", __PRETTY_FUNCTION__, err));
}

void
MediaOmxCommonDecoder::PauseStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  DECODER_LOG(PR_LOG_DEBUG, ("%s", __PRETTY_FUNCTION__));
  if (!mDecoderStateMachine) {
    return;
  }
  
  RefPtr<nsRunnable> event =
    NS_NewRunnableMethodWithArg<bool>(
      mDecoderStateMachine,
      &MediaDecoderStateMachine::SetDormant,
      true);
  mDecoderStateMachine->TaskQueue()->Dispatch(event);
}

void
MediaOmxCommonDecoder::ResumeStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  DECODER_LOG(PR_LOG_DEBUG, ("%s current time %f", __PRETTY_FUNCTION__,
      mCurrentTime));

  if (!mDecoderStateMachine) {
    return;
  }

  mFallbackToStateMachine = true;
  mAudioOffloadPlayer = nullptr;
  int64_t timeUsecs = 0;
  SecondsToUsecs(mCurrentTime, timeUsecs);
  mRequestedSeekTarget = SeekTarget(timeUsecs, SeekTarget::Accurate);

  mNextState = mPlayState;
  ChangeState(PLAY_STATE_LOADING);
  
  RefPtr<nsRunnable> event =
    NS_NewRunnableMethodWithArg<bool>(
      mDecoderStateMachine,
      &MediaDecoderStateMachine::SetDormant,
      false);
  mDecoderStateMachine->TaskQueue()->Dispatch(event);
}

void
MediaOmxCommonDecoder::AudioOffloadTearDown()
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG(PR_LOG_DEBUG, ("%s", __PRETTY_FUNCTION__));

  
  
  if (mAudioOffloadPlayer) {
    
    PlaybackPositionChanged();
    ResumeStateMachine();
  }
}

void
MediaOmxCommonDecoder::AddOutputStream(ProcessedMediaStream* aStream,
                                       bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mAudioOffloadPlayer) {
    
    PlaybackPositionChanged();
    ResumeStateMachine();
  }

  MediaDecoder::AddOutputStream(aStream, aFinishWhenEnded);
}

void
MediaOmxCommonDecoder::SetPlaybackRate(double aPlaybackRate)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mAudioOffloadPlayer &&
      ((aPlaybackRate != 0.0) || (aPlaybackRate != 1.0))) {
    
    PlaybackPositionChanged();
    ResumeStateMachine();
  }

  MediaDecoder::SetPlaybackRate(aPlaybackRate);
}

void
MediaOmxCommonDecoder::ChangeState(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  MediaDecoder::ChangeState(aState);

  if (mAudioOffloadPlayer) {
    status_t err = mAudioOffloadPlayer->ChangeState(aState);
    if (err != OK) {
      ResumeStateMachine();
    }
  }
}

void
MediaOmxCommonDecoder::ApplyStateToStateMachine(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  
  if (!mAudioOffloadPlayer) {
    MediaDecoder::ApplyStateToStateMachine(aState);
  }
}

void
MediaOmxCommonDecoder::PlaybackPositionChanged(MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mAudioOffloadPlayer) {
    MediaDecoder::PlaybackPositionChanged();
    return;
  }

  if (!mOwner || mShuttingDown) {
    return;
  }

  double lastTime = mCurrentTime;
  {
    ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
    mCurrentTime = mAudioOffloadPlayer->GetMediaTimeSecs();
  }
  if (mOwner &&
      (aEventVisibility != MediaDecoderEventVisibility::Suppressed) &&
      lastTime != mCurrentTime) {
    FireTimeUpdate();
  }
}

void
MediaOmxCommonDecoder::SetElementVisibility(bool aIsVisible)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mAudioOffloadPlayer) {
    mAudioOffloadPlayer->SetElementVisibility(aIsVisible);
  }
}

void
MediaOmxCommonDecoder::UpdateReadyStateForData()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mAudioOffloadPlayer) {
    MediaDecoder::UpdateReadyStateForData();
    return;
  }

  if (!mOwner || mShuttingDown)
    return;
  mOwner->UpdateReadyStateForData(mAudioOffloadPlayer->GetNextFrameStatus());
}

void
MediaOmxCommonDecoder::SetVolume(double aVolume)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mAudioOffloadPlayer) {
    MediaDecoder::SetVolume(aVolume);
    return;
  }
  mAudioOffloadPlayer->SetVolume(aVolume);
}

MediaDecoderStateMachine*
MediaOmxCommonDecoder::CreateStateMachine()
{
  mReader = CreateReader();
  if (mReader != nullptr) {
    mReader->SetAudioChannel(GetAudioChannel());
  }
  return CreateStateMachineFromReader(mReader);
}

} 
