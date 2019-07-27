





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

extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) MOZ_LOG(gMediaDecoderLog, type, msg)

MediaOmxCommonDecoder::MediaOmxCommonDecoder()
  : MediaDecoder()
  , mReader(nullptr)
  , mCanOffloadAudio(false)
  , mFallbackToStateMachine(false)
  , mIsCaptured(false)
{
  mDormantSupported = true;
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
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
  return (mCanOffloadAudio && !mFallbackToStateMachine &&
          !mIsCaptured && mPlaybackRate == 1.0);
}

void
MediaOmxCommonDecoder::FirstFrameLoaded(nsAutoPtr<MediaInfo> aInfo,
                                        MediaDecoderEventVisibility aEventVisibility)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mShuttingDown) {
    return;
  }

  MediaDecoder::FirstFrameLoaded(aInfo, aEventVisibility);

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  if (!CheckDecoderCanOffloadAudio()) {
    DECODER_LOG(LogLevel::Debug, ("In %s Offload Audio check failed",
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
  if (err != OK) {
    mAudioOffloadPlayer = nullptr;
    mFallbackToStateMachine = true;
    DECODER_LOG(LogLevel::Debug, ("In %s Unable to start offload audio %d."
      "Switching to normal mode", __PRETTY_FUNCTION__, err));
    return;
  }
  PauseStateMachine();
  if (mLogicallySeeking) {
    SeekTarget target = SeekTarget(mLogicalPosition,
                                   SeekTarget::Accurate,
                                   MediaDecoderEventVisibility::Observable);
    mSeekRequest.DisconnectIfExists();
    mSeekRequest.Begin(mAudioOffloadPlayer->Seek(target)
      ->Then(AbstractThread::MainThread(), __func__, static_cast<MediaDecoder*>(this),
             &MediaDecoder::OnSeekResolved, &MediaDecoder::OnSeekRejected));
  }
  
  ChangeState(mPlayState);
}

void
MediaOmxCommonDecoder::PauseStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();
  DECODER_LOG(LogLevel::Debug, ("%s", __PRETTY_FUNCTION__));

  if (mShuttingDown) {
    return;
  }

  if (!GetStateMachine()) {
    return;
  }
  
  RefPtr<nsRunnable> event =
    NS_NewRunnableMethodWithArg<bool>(
      GetStateMachine(),
      &MediaDecoderStateMachine::SetDormant,
      true);
  GetStateMachine()->TaskQueue()->Dispatch(event.forget());
}

void
MediaOmxCommonDecoder::ResumeStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  DECODER_LOG(LogLevel::Debug, ("%s current time %f", __PRETTY_FUNCTION__, mLogicalPosition));

  if (mShuttingDown) {
    return;
  }

  if (!GetStateMachine()) {
    return;
  }

  mFallbackToStateMachine = true;
  mAudioOffloadPlayer = nullptr;
  SeekTarget target = SeekTarget(mLogicalPosition,
                                 SeekTarget::Accurate,
                                 MediaDecoderEventVisibility::Suppressed);
  
  RefPtr<nsRunnable> event =
    NS_NewRunnableMethodWithArg<SeekTarget>(
      GetStateMachine(),
      &MediaDecoderStateMachine::Seek,
      target);
  GetStateMachine()->TaskQueue()->Dispatch(event.forget());

  mNextState = mPlayState;
  ChangeState(PLAY_STATE_LOADING);
  
  event =
    NS_NewRunnableMethodWithArg<bool>(
      GetStateMachine(),
      &MediaDecoderStateMachine::SetDormant,
      false);
  GetStateMachine()->TaskQueue()->Dispatch(event.forget());
  UpdateLogicalPosition();
}

void
MediaOmxCommonDecoder::AudioOffloadTearDown()
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG(LogLevel::Debug, ("%s", __PRETTY_FUNCTION__));

  
  
  if (mAudioOffloadPlayer) {
    ResumeStateMachine();
  }
}

void
MediaOmxCommonDecoder::AddOutputStream(ProcessedMediaStream* aStream,
                                       bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());

  mIsCaptured = true;

  if (mAudioOffloadPlayer) {
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
    ResumeStateMachine();
  }

  MediaDecoder::SetPlaybackRate(aPlaybackRate);
}

void
MediaOmxCommonDecoder::ChangeState(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  
  
  
  MediaDecoder::ChangeState(aState);

  if (!mAudioOffloadPlayer) {
    return;
  }

  status_t err = mAudioOffloadPlayer->ChangeState(aState);
  if (err != OK) {
    ResumeStateMachine();
    return;
  }
}

void
MediaOmxCommonDecoder::CallSeek(const SeekTarget& aTarget)
{
  if (!mAudioOffloadPlayer) {
    MediaDecoder::CallSeek(aTarget);
    return;
  }

  mSeekRequest.DisconnectIfExists();
  mSeekRequest.Begin(mAudioOffloadPlayer->Seek(aTarget)
    ->Then(AbstractThread::MainThread(), __func__, static_cast<MediaDecoder*>(this),
           &MediaDecoder::OnSeekResolved, &MediaDecoder::OnSeekRejected));
}

int64_t
MediaOmxCommonDecoder::CurrentPosition()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mAudioOffloadPlayer) {
    return MediaDecoder::CurrentPosition();
  }

  ReentrantMonitorAutoEnter mon(GetReentrantMonitor());
  return mAudioOffloadPlayer->GetMediaTimeUs();
}

void
MediaOmxCommonDecoder::SetElementVisibility(bool aIsVisible)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (mAudioOffloadPlayer) {
    mAudioOffloadPlayer->SetElementVisibility(aIsVisible);
  }
}

MediaDecoderOwner::NextFrameStatus
MediaOmxCommonDecoder::NextFrameStatus()
{
  MOZ_ASSERT(NS_IsMainThread());
  return mAudioOffloadPlayer ? mAudioOffloadPlayer->GetNextFrameStatus()
                             : MediaDecoder::NextFrameStatus();
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
