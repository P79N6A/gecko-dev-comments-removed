




#ifdef XP_WIN

#include "windows.h"
#include "mmsystem.h"
#endif

#include "mozilla/DebugOnly.h"
#include <stdint.h>

#include "MediaDecoderStateMachine.h"
#include "MediaTimer.h"
#include "AudioSink.h"
#include "nsTArray.h"
#include "MediaDecoder.h"
#include "MediaDecoderReader.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"
#include "TimeUnits.h"
#include "nsDeque.h"
#include "AudioSegment.h"
#include "VideoSegment.h"
#include "ImageContainer.h"
#include "nsComponentManagerUtils.h"
#include "nsITimer.h"
#include "nsContentUtils.h"
#include "MediaShutdownManager.h"
#include "SharedThreadPool.h"
#include "MediaTaskQueue.h"
#include "nsIEventTarget.h"
#include "prenv.h"
#include "mozilla/Preferences.h"
#include "gfx2DGlue.h"
#include "nsPrintfCString.h"
#include "DOMMediaStream.h"
#include "DecodedStream.h"
#include "mozilla/Logging.h"

#include <algorithm>

namespace mozilla {

using namespace mozilla::dom;
using namespace mozilla::layers;
using namespace mozilla::media;

#define NS_DispatchToMainThread(...) CompileError_UseAbstractThreadDispatchInstead


#undef LOG
#undef DECODER_LOG
#undef VERBOSE_LOG

#define LOG(m, l, x, ...) \
  MOZ_LOG(m, l, ("Decoder=%p " x, mDecoder.get(), ##__VA_ARGS__))
#define DECODER_LOG(x, ...) \
  LOG(gMediaDecoderLog, LogLevel::Debug, x, ##__VA_ARGS__)
#define VERBOSE_LOG(x, ...) \
  LOG(gMediaDecoderLog, LogLevel::Verbose, x, ##__VA_ARGS__)
#define SAMPLE_LOG(x, ...) \
  LOG(gMediaSampleLog, LogLevel::Debug, x, ##__VA_ARGS__)



#define DECODER_WARN_HELPER(a, b) NS_WARNING b
#define DECODER_WARN(x, ...) \
  DECODER_WARN_HELPER(0, (nsPrintfCString("Decoder=%p " x, mDecoder.get(), ##__VA_ARGS__).get()))




namespace detail {





static const uint32_t LOW_AUDIO_USECS = 300000;





const int64_t AMPLE_AUDIO_USECS = 1000000;

} 






const int64_t NO_VIDEO_AMPLE_AUDIO_DIVISOR = 8;




static const uint32_t LOW_VIDEO_FRAMES = 2;





static const int32_t LOW_VIDEO_THRESHOLD_USECS = 60000;


static const int AUDIO_DURATION_USECS = 40000;





static const int THRESHOLD_FACTOR = 2;

namespace detail {





static const int64_t LOW_DATA_THRESHOLD_USECS = 5000000;



static_assert(LOW_DATA_THRESHOLD_USECS > AMPLE_AUDIO_USECS,
              "LOW_DATA_THRESHOLD_USECS is too small");

} 


static const uint32_t EXHAUSTED_DATA_MARGIN_USECS = 60000;










static const uint32_t QUICK_BUFFER_THRESHOLD_USECS = 2000000;

namespace detail {



static const uint32_t QUICK_BUFFERING_LOW_DATA_USECS = 1000000;





static_assert(QUICK_BUFFERING_LOW_DATA_USECS <= AMPLE_AUDIO_USECS,
              "QUICK_BUFFERING_LOW_DATA_USECS is too large");

} 

static TimeDuration UsecsToDuration(int64_t aUsecs) {
  return TimeDuration::FromMicroseconds(aUsecs);
}

static int64_t DurationToUsecs(TimeDuration aDuration) {
  return static_cast<int64_t>(aDuration.ToSeconds() * USECS_PER_S);
}

static const uint32_t MIN_VIDEO_QUEUE_SIZE = 3;
static const uint32_t MAX_VIDEO_QUEUE_SIZE = 10;
static const uint32_t SCARCE_VIDEO_QUEUE_SIZE = 1;
static const uint32_t VIDEO_QUEUE_SEND_TO_COMPOSITOR_SIZE = 9999;

static uint32_t sVideoQueueDefaultSize = MAX_VIDEO_QUEUE_SIZE;
static uint32_t sVideoQueueHWAccelSize = MIN_VIDEO_QUEUE_SIZE;
static uint32_t sVideoQueueSendToCompositorSize = VIDEO_QUEUE_SEND_TO_COMPOSITOR_SIZE;

MediaDecoderStateMachine::MediaDecoderStateMachine(MediaDecoder* aDecoder,
                                                   MediaDecoderReader* aReader,
                                                   bool aRealTime) :
  mDecoder(aDecoder),
  mTaskQueue(new MediaTaskQueue(GetMediaThreadPool(MediaThreadType::PLAYBACK),
                                 true)),
  mWatchManager(this, mTaskQueue),
  mProducerID(ImageContainer::AllocateProducerID()),
  mRealTime(aRealTime),
  mDispatchedStateMachine(false),
  mDelayedScheduler(this),
  mState(DECODER_STATE_DECODING_NONE, "MediaDecoderStateMachine::mState"),
  mPlayDuration(0),
  mBuffered(mTaskQueue, TimeIntervals(), "MediaDecoderStateMachine::mBuffered (Mirror)"),
  mDuration(mTaskQueue, NullableTimeUnit(), "MediaDecoderStateMachine::mDuration (Canonical"),
  mCurrentFrameID(0),
  mEstimatedDuration(mTaskQueue, NullableTimeUnit(),
                    "MediaDecoderStateMachine::mEstimatedDuration (Mirror)"),
  mExplicitDuration(mTaskQueue, Maybe<double>(),
                    "MediaDecoderStateMachine::mExplicitDuration (Mirror)"),
  mObservedDuration(TimeUnit(), "MediaDecoderStateMachine::mObservedDuration"),
  mPlayState(mTaskQueue, MediaDecoder::PLAY_STATE_LOADING,
             "MediaDecoderStateMachine::mPlayState (Mirror)"),
  mNextPlayState(mTaskQueue, MediaDecoder::PLAY_STATE_PAUSED,
                 "MediaDecoderStateMachine::mNextPlayState (Mirror)"),
  mLogicallySeeking(mTaskQueue, false,
             "MediaDecoderStateMachine::mLogicallySeeking (Mirror)"),
  mIsShutdown(mTaskQueue, false,
              "MediaDecoderStateMachine::mIsShutdown (Canonical)"),
  mNextFrameStatus(mTaskQueue, MediaDecoderOwner::NEXT_FRAME_UNINITIALIZED,
                   "MediaDecoderStateMachine::mNextFrameStatus (Canonical)"),
  mFragmentEndTime(-1),
  mReader(aReader),
  mCurrentPosition(mTaskQueue, 0, "MediaDecoderStateMachine::mCurrentPosition (Canonical)"),
  mAudioEndTime(-1),
  mDecodedAudioEndTime(-1),
  mVideoFrameEndTime(-1),
  mDecodedVideoEndTime(-1),
  mVolume(mTaskQueue, 1.0, "MediaDecoderStateMachine::mVolume (Mirror)"),
  mPlaybackRate(1.0),
  mLogicalPlaybackRate(mTaskQueue, 1.0, "MediaDecoderStateMachine::mLogicalPlaybackRate (Mirror)"),
  mPreservesPitch(mTaskQueue, true, "MediaDecoderStateMachine::mPreservesPitch (Mirror)"),
  mLowAudioThresholdUsecs(detail::LOW_AUDIO_USECS),
  mAmpleAudioThresholdUsecs(detail::AMPLE_AUDIO_USECS),
  mQuickBufferingLowDataThresholdUsecs(detail::QUICK_BUFFERING_LOW_DATA_USECS),
  mIsAudioPrerolling(false),
  mIsVideoPrerolling(false),
  mAudioCaptured(false),
  mPositionChangeQueued(false),
  mAudioCompleted(false, "MediaDecoderStateMachine::mAudioCompleted"),
  mNotifyMetadataBeforeFirstFrame(false),
  mDispatchedEventToDecode(false),
  mQuickBuffering(false),
  mMinimizePreroll(false),
  mDecodeThreadWaiting(false),
  mDropAudioUntilNextDiscontinuity(false),
  mDropVideoUntilNextDiscontinuity(false),
  mDecodeToSeekTarget(false),
  mCurrentTimeBeforeSeek(0),
  mCorruptFrames(30),
  mDisabledHardwareAcceleration(false),
  mDecodingFrozenAtStateDecoding(false),
  mSentLoadedMetadataEvent(false),
  mSentFirstFrameLoadedEvent(false),
  mSentPlaybackEndedEvent(false),
  mDecodedStream(new DecodedStream(mAudioQueue, mVideoQueue))
{
  MOZ_COUNT_CTOR(MediaDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(this, &MediaDecoderStateMachine::InitializationTask);
  mTaskQueue->Dispatch(r.forget());

  static bool sPrefCacheInit = false;
  if (!sPrefCacheInit) {
    sPrefCacheInit = true;
    Preferences::AddUintVarCache(&sVideoQueueDefaultSize,
                                 "media.video-queue.default-size",
                                 MAX_VIDEO_QUEUE_SIZE);
    Preferences::AddUintVarCache(&sVideoQueueHWAccelSize,
                                 "media.video-queue.hw-accel-size",
                                 MIN_VIDEO_QUEUE_SIZE);
    Preferences::AddUintVarCache(&sVideoQueueSendToCompositorSize,
                                 "media.video-queue.send-to-compositor-size",
                                 VIDEO_QUEUE_SEND_TO_COMPOSITOR_SIZE);
  }

  mBufferingWait = IsRealTime() ? 0 : 15;
  mLowDataThresholdUsecs = IsRealTime() ? 0 : detail::LOW_DATA_THRESHOLD_USECS;

#ifdef XP_WIN
  
  
  
  
  
  timeBeginPeriod(1);
#endif

  AudioQueue().AddPopListener(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::OnAudioPopped),
    mTaskQueue);

  VideoQueue().AddPopListener(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::OnVideoPopped),
    mTaskQueue);
}

MediaDecoderStateMachine::~MediaDecoderStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on main thread.");
  MOZ_COUNT_DTOR(MediaDecoderStateMachine);

  mReader = nullptr;

#ifdef XP_WIN
  timeEndPeriod(1);
#endif
}

void
MediaDecoderStateMachine::InitializationTask()
{
  MOZ_ASSERT(OnTaskQueue());

  
  mBuffered.Connect(mReader->CanonicalBuffered());
  mEstimatedDuration.Connect(mDecoder->CanonicalEstimatedDuration());
  mExplicitDuration.Connect(mDecoder->CanonicalExplicitDuration());
  mPlayState.Connect(mDecoder->CanonicalPlayState());
  mNextPlayState.Connect(mDecoder->CanonicalNextPlayState());
  mLogicallySeeking.Connect(mDecoder->CanonicalLogicallySeeking());
  mVolume.Connect(mDecoder->CanonicalVolume());
  mLogicalPlaybackRate.Connect(mDecoder->CanonicalPlaybackRate());
  mPreservesPitch.Connect(mDecoder->CanonicalPreservesPitch());

  
  mWatchManager.Watch(mBuffered, &MediaDecoderStateMachine::BufferedRangeUpdated);
  mWatchManager.Watch(mState, &MediaDecoderStateMachine::UpdateNextFrameStatus);
  mWatchManager.Watch(mAudioCompleted, &MediaDecoderStateMachine::UpdateNextFrameStatus);
  mWatchManager.Watch(mVolume, &MediaDecoderStateMachine::VolumeChanged);
  mWatchManager.Watch(mLogicalPlaybackRate, &MediaDecoderStateMachine::LogicalPlaybackRateChanged);
  mWatchManager.Watch(mPreservesPitch, &MediaDecoderStateMachine::PreservesPitchChanged);
  mWatchManager.Watch(mEstimatedDuration, &MediaDecoderStateMachine::RecomputeDuration);
  mWatchManager.Watch(mExplicitDuration, &MediaDecoderStateMachine::RecomputeDuration);
  mWatchManager.Watch(mObservedDuration, &MediaDecoderStateMachine::RecomputeDuration);
  mWatchManager.Watch(mPlayState, &MediaDecoderStateMachine::PlayStateChanged);
  mWatchManager.Watch(mLogicallySeeking, &MediaDecoderStateMachine::LogicallySeekingChanged);
}

bool MediaDecoderStateMachine::HasFutureAudio()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedUsecs() >
            mLowAudioThresholdUsecs * mPlaybackRate ||
          AudioQueue().IsFinished());
}

bool MediaDecoderStateMachine::HaveNextFrameData()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  return (!HasAudio() || HasFutureAudio()) &&
         (!HasVideo() || VideoQueue().GetSize() > 1);
}

int64_t MediaDecoderStateMachine::GetDecodedAudioDuration()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  int64_t audioDecoded = AudioQueue().Duration();
  if (AudioEndTime() != -1) {
    audioDecoded += AudioEndTime() - GetMediaTime();
  }
  return audioDecoded;
}

void MediaDecoderStateMachine::SendStreamData()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(!mAudioSink, "Should've been stopped in RunStateMachine()");

  bool finished = mDecodedStream->SendData(mVolume, mDecoder->IsSameOriginMedia());

  if (mInfo.HasAudio()) {
    CheckedInt64 playedUsecs = mDecodedStream->AudioEndTime();
    if (playedUsecs.isValid()) {
      OnAudioEndTimeUpdate(playedUsecs.value());
    }
  }

  const auto clockTime = GetClock();
  while (true) {
    const AudioData* a = AudioQueue().PeekFront();
    
    
    
    
    if (a && a->mTime <= clockTime) {
      nsRefPtr<AudioData> releaseMe = AudioQueue().PopFront();
      continue;
    }
    break;
  }

  
  
  if (finished && AudioQueue().GetSize() == 0) {
    mAudioCompleted = true;
  }
}

bool MediaDecoderStateMachine::HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (AudioQueue().GetSize() == 0 ||
      GetDecodedAudioDuration() < aAmpleAudioUSecs) {
    return false;
  }

  
  
  
  return true;
}

bool MediaDecoderStateMachine::HaveEnoughDecodedVideo()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (VideoQueue().GetSize() - 1 < GetAmpleVideoFrames() * mPlaybackRate) {
    return false;
  }

  return true;
}

bool
MediaDecoderStateMachine::NeedToDecodeVideo()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("NeedToDecodeVideo() isDec=%d decToTar=%d minPrl=%d seek=%d enufVid=%d",
             IsVideoDecoding(), mDecodeToSeekTarget, mMinimizePreroll,
             mState == DECODER_STATE_SEEKING,
             HaveEnoughDecodedVideo());
  return IsVideoDecoding() &&
         ((mState == DECODER_STATE_SEEKING && mDecodeToSeekTarget) ||
          (mState == DECODER_STATE_DECODING_FIRSTFRAME &&
           IsVideoDecoding() && VideoQueue().GetSize() == 0) ||
          (!mMinimizePreroll && !HaveEnoughDecodedVideo()));
}

bool
MediaDecoderStateMachine::NeedToSkipToNextKeyframe()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
    return false;
  }
  MOZ_ASSERT(mState == DECODER_STATE_DECODING ||
             mState == DECODER_STATE_BUFFERING ||
             mState == DECODER_STATE_SEEKING);

  
  if (!IsVideoDecoding() || mState == DECODER_STATE_BUFFERING ||
      mState == DECODER_STATE_SEEKING) {
    return false;
  }

  
  
  if (mAudioCaptured && !HasAudio()) {
    return false;
  }

  
  
  
  
  
  
  
  
  bool isLowOnDecodedAudio = !mReader->IsAsync() &&
                             !mIsAudioPrerolling && IsAudioDecoding() &&
                             (GetDecodedAudioDuration() <
                              mLowAudioThresholdUsecs * mPlaybackRate);
  bool isLowOnDecodedVideo = !mIsVideoPrerolling &&
                             ((GetClock() - mDecodedVideoEndTime) * mPlaybackRate >
                              LOW_VIDEO_THRESHOLD_USECS);
  bool lowUndecoded = HasLowUndecodedData();

  if ((isLowOnDecodedAudio || isLowOnDecodedVideo) && !lowUndecoded) {
    DECODER_LOG("Skipping video decode to the next keyframe lowAudio=%d lowVideo=%d lowUndecoded=%d async=%d",
                isLowOnDecodedAudio, isLowOnDecodedVideo, lowUndecoded, mReader->IsAsync());
    return true;
  }

  return false;
}

bool
MediaDecoderStateMachine::NeedToDecodeAudio()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("NeedToDecodeAudio() isDec=%d decToTar=%d minPrl=%d seek=%d enufAud=%d",
             IsAudioDecoding(), mDecodeToSeekTarget, mMinimizePreroll,
             mState == DECODER_STATE_SEEKING,
             HaveEnoughDecodedAudio(mAmpleAudioThresholdUsecs * mPlaybackRate));

  return IsAudioDecoding() &&
         ((mState == DECODER_STATE_SEEKING && mDecodeToSeekTarget) ||
          (mState == DECODER_STATE_DECODING_FIRSTFRAME &&
           IsAudioDecoding() && AudioQueue().GetSize() == 0) ||
          (!mMinimizePreroll &&
          !HaveEnoughDecodedAudio(mAmpleAudioThresholdUsecs * mPlaybackRate) &&
          (mState != DECODER_STATE_SEEKING || mDecodeToSeekTarget)));
}

bool
MediaDecoderStateMachine::IsAudioSeekComplete()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("IsAudioSeekComplete() curTarVal=%d mAudDis=%d aqFin=%d aqSz=%d",
    mCurrentSeek.Exists(), mDropAudioUntilNextDiscontinuity, AudioQueue().IsFinished(), AudioQueue().GetSize());
  return
    !HasAudio() ||
    (mCurrentSeek.Exists() &&
     !mDropAudioUntilNextDiscontinuity &&
     (AudioQueue().IsFinished() || AudioQueue().GetSize() > 0));
}

bool
MediaDecoderStateMachine::IsVideoSeekComplete()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("IsVideoSeekComplete() curTarVal=%d mVidDis=%d vqFin=%d vqSz=%d",
    mCurrentSeek.Exists(), mDropVideoUntilNextDiscontinuity, VideoQueue().IsFinished(), VideoQueue().GetSize());
  return
    !HasVideo() ||
    (mCurrentSeek.Exists() &&
     !mDropVideoUntilNextDiscontinuity &&
     (VideoQueue().IsFinished() || VideoQueue().GetSize() > 0));
}

void
MediaDecoderStateMachine::OnAudioDecoded(AudioData* aAudioSample)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<AudioData> audio(aAudioSample);
  MOZ_ASSERT(audio);
  mAudioDataRequest.Complete();
  aAudioSample->AdjustForStartTime(StartTime());
  mDecodedAudioEndTime = audio->GetEndTime();

  SAMPLE_LOG("OnAudioDecoded [%lld,%lld] disc=%d",
             (audio ? audio->mTime : -1),
             (audio ? audio->GetEndTime() : -1),
             (audio ? audio->mDiscontinuity : 0));

  switch (mState) {
    case DECODER_STATE_DECODING_FIRSTFRAME: {
      Push(audio);
      MaybeFinishDecodeFirstFrame();
      return;
    }

    case DECODER_STATE_BUFFERING: {
      
      
      Push(audio);
      ScheduleStateMachine();
      return;
    }

    case DECODER_STATE_DECODING: {
      Push(audio);
      if (mIsAudioPrerolling && DonePrerollingAudio()) {
        StopPrerollingAudio();
      }
      
      if (mAudioCaptured) {
        ScheduleStateMachine();
      }
      return;
    }

    case DECODER_STATE_SEEKING: {
      if (!mCurrentSeek.Exists()) {
        
        return;
      }
      if (audio->mDiscontinuity) {
        mDropAudioUntilNextDiscontinuity = false;
      }
      if (!mDropAudioUntilNextDiscontinuity) {
        
        
        if (mCurrentSeek.mTarget.mType == SeekTarget::PrevSyncPoint &&
            mCurrentSeek.mTarget.mTime > mCurrentTimeBeforeSeek &&
            audio->mTime < mCurrentTimeBeforeSeek) {
          
          
          
          
          
          
          mCurrentSeek.mTarget.mType = SeekTarget::Accurate;
        }
        if (mCurrentSeek.mTarget.mType == SeekTarget::PrevSyncPoint) {
          
          Push(audio);
        } else {
          
          
          if (NS_FAILED(DropAudioUpToSeekTarget(audio))) {
            DecodeError();
            return;
          }
        }
      }
      CheckIfSeekComplete();
      return;
    }
    default: {
      
      return;
    }
  }
}

void
MediaDecoderStateMachine::Push(AudioData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(aSample);
  
  
  
  AudioQueue().Push(aSample);
  UpdateNextFrameStatus();
  DispatchDecodeTasksIfNeeded();

  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void
MediaDecoderStateMachine::PushFront(AudioData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(aSample);

  AudioQueue().PushFront(aSample);
  UpdateNextFrameStatus();
}

void
MediaDecoderStateMachine::Push(VideoData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(aSample);
  
  
  
  aSample->mFrameID = ++mCurrentFrameID;
  VideoQueue().Push(aSample);
  UpdateNextFrameStatus();
  DispatchDecodeTasksIfNeeded();

  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void
MediaDecoderStateMachine::PushFront(VideoData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(aSample);

  aSample->mFrameID = ++mCurrentFrameID;
  VideoQueue().PushFront(aSample);
  UpdateNextFrameStatus();
}

void
MediaDecoderStateMachine::OnAudioPopped()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  UpdateNextFrameStatus();
  DispatchAudioDecodeTaskIfNeeded();
}

void
MediaDecoderStateMachine::OnVideoPopped()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  UpdateNextFrameStatus();
  DispatchVideoDecodeTaskIfNeeded();
  
  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void
MediaDecoderStateMachine::OnNotDecoded(MediaData::Type aType,
                                       MediaDecoderReader::NotDecodedReason aReason)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  SAMPLE_LOG("OnNotDecoded (aType=%u, aReason=%u)", aType, aReason);
  bool isAudio = aType == MediaData::AUDIO_DATA;
  MOZ_ASSERT_IF(!isAudio, aType == MediaData::VIDEO_DATA);

  if (isAudio) {
    mAudioDataRequest.Complete();
  } else {
    mVideoDataRequest.Complete();
  }
  if (IsShutdown()) {
    
    return;
  }

  
  if (aReason == MediaDecoderReader::DECODE_ERROR) {
    DecodeError();
    return;
  }

  
  
  if (aReason == MediaDecoderReader::WAITING_FOR_DATA) {
    MOZ_ASSERT(mReader->IsWaitForDataSupported(),
               "Readers that send WAITING_FOR_DATA need to implement WaitForData");
    nsRefPtr<MediaDecoderStateMachine> self = this;
    WaitRequestRef(aType).Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                               &MediaDecoderReader::WaitForData, aType)
      ->Then(TaskQueue(), __func__,
             [self] (MediaData::Type aType) -> void {
               ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
               self->WaitRequestRef(aType).Complete();
               self->DispatchDecodeTasksIfNeeded();
             },
             [self] (WaitForDataRejectValue aRejection) -> void {
               ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
               self->WaitRequestRef(aRejection.mType).Complete();
             }));

    return;
  }

  if (aReason == MediaDecoderReader::CANCELED) {
    DispatchDecodeTasksIfNeeded();
    return;
  }

  
  
  MOZ_ASSERT(aReason == MediaDecoderReader::END_OF_STREAM);
  if (!isAudio && mState == DECODER_STATE_SEEKING &&
      mCurrentSeek.Exists() && mFirstVideoFrameAfterSeek) {
    
    
    
    
    Push(mFirstVideoFrameAfterSeek);
    mFirstVideoFrameAfterSeek = nullptr;
  }
  if (isAudio) {
    AudioQueue().Finish();
    StopPrerollingAudio();
  } else {
    VideoQueue().Finish();
    StopPrerollingVideo();
  }
  switch (mState) {
    case DECODER_STATE_DECODING_FIRSTFRAME: {
      MaybeFinishDecodeFirstFrame();
      return;
    }

    case DECODER_STATE_BUFFERING:
    case DECODER_STATE_DECODING: {
      CheckIfDecodeComplete();
      mDecoder->GetReentrantMonitor().NotifyAll();
      
      if (mAudioCaptured) {
        ScheduleStateMachine();
      }
      return;
    }
    case DECODER_STATE_SEEKING: {
      if (!mCurrentSeek.Exists()) {
        
        return;
      }

      if (isAudio) {
        mDropAudioUntilNextDiscontinuity = false;
      } else {
        mDropVideoUntilNextDiscontinuity = false;
      }

      CheckIfSeekComplete();
      return;
    }
    default: {
      return;
    }
  }
}

void
MediaDecoderStateMachine::MaybeFinishDecodeFirstFrame()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if ((IsAudioDecoding() && AudioQueue().GetSize() == 0) ||
      (IsVideoDecoding() && VideoQueue().GetSize() == 0)) {
    return;
  }
  if (NS_FAILED(FinishDecodeFirstFrame())) {
    DecodeError();
  }
}

void
MediaDecoderStateMachine::OnVideoDecoded(VideoData* aVideoSample)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<VideoData> video(aVideoSample);
  MOZ_ASSERT(video);
  mVideoDataRequest.Complete();
  aVideoSample->AdjustForStartTime(StartTime());
  mDecodedVideoEndTime = video ? video->GetEndTime() : mDecodedVideoEndTime;

  SAMPLE_LOG("OnVideoDecoded [%lld,%lld] disc=%d",
             (video ? video->mTime : -1),
             (video ? video->GetEndTime() : -1),
             (video ? video->mDiscontinuity : 0));

  switch (mState) {
    case DECODER_STATE_DECODING_FIRSTFRAME: {
      Push(video);
      MaybeFinishDecodeFirstFrame();
      return;
    }

    case DECODER_STATE_BUFFERING: {
      
      
      Push(video);
      ScheduleStateMachine();
      return;
    }

    case DECODER_STATE_DECODING: {
      Push(video);
      if (mIsVideoPrerolling && DonePrerollingVideo()) {
        StopPrerollingVideo();
      }

      
      
      
      
      
      
      
      if (mAudioCaptured || VideoQueue().GetSize() == 1) {
        ScheduleStateMachine();
      }

      
      
      
      
      
      if (mReader->IsAsync()) {
        return;
      }
      TimeDuration decodeTime = TimeStamp::Now() - mVideoDecodeStartTime;
      if (THRESHOLD_FACTOR * DurationToUsecs(decodeTime) > mLowAudioThresholdUsecs &&
          !HasLowUndecodedData())
      {
        mLowAudioThresholdUsecs =
          std::min(THRESHOLD_FACTOR * DurationToUsecs(decodeTime), mAmpleAudioThresholdUsecs);
        mAmpleAudioThresholdUsecs = std::max(THRESHOLD_FACTOR * mLowAudioThresholdUsecs,
                                              mAmpleAudioThresholdUsecs);
        DECODER_LOG("Slow video decode, set mLowAudioThresholdUsecs=%lld mAmpleAudioThresholdUsecs=%lld",
                    mLowAudioThresholdUsecs, mAmpleAudioThresholdUsecs);
      }
      return;
    }
    case DECODER_STATE_SEEKING: {
      if (!mCurrentSeek.Exists()) {
        
        return;
      }
      if (mDropVideoUntilNextDiscontinuity) {
        if (video->mDiscontinuity) {
          mDropVideoUntilNextDiscontinuity = false;
        }
      }
      if (!mDropVideoUntilNextDiscontinuity) {
        
        
        if (mCurrentSeek.mTarget.mType == SeekTarget::PrevSyncPoint &&
            mCurrentSeek.mTarget.mTime > mCurrentTimeBeforeSeek &&
            video->mTime < mCurrentTimeBeforeSeek) {
          
          
          
          
          
          
          mCurrentSeek.mTarget.mType = SeekTarget::Accurate;
        }
        if (mCurrentSeek.mTarget.mType == SeekTarget::PrevSyncPoint) {
          
          Push(video);
        } else {
          
          
          if (NS_FAILED(DropVideoUpToSeekTarget(video))) {
            DecodeError();
            return;
          }
        }
      }
      CheckIfSeekComplete();
      return;
    }
    default: {
      
      return;
    }
  }
}

void
MediaDecoderStateMachine::CheckIfSeekComplete()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_SEEKING);

  const bool videoSeekComplete = IsVideoSeekComplete();
  if (HasVideo() && !videoSeekComplete) {
    
    if (NS_FAILED(EnsureVideoDecodeTaskQueued())) {
      DECODER_WARN("Failed to request video during seek");
      DecodeError();
    }
  }

  const bool audioSeekComplete = IsAudioSeekComplete();
  if (HasAudio() && !audioSeekComplete) {
    
    if (NS_FAILED(EnsureAudioDecodeTaskQueued())) {
      DECODER_WARN("Failed to request audio during seek");
      DecodeError();
    }
  }

  SAMPLE_LOG("CheckIfSeekComplete() audioSeekComplete=%d videoSeekComplete=%d",
             audioSeekComplete, videoSeekComplete);

  if (audioSeekComplete && videoSeekComplete) {
    mDecodeToSeekTarget = false;
    SeekCompleted();
  }
}

bool
MediaDecoderStateMachine::IsAudioDecoding()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  return HasAudio() && !AudioQueue().IsFinished();
}

bool
MediaDecoderStateMachine::IsVideoDecoding()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  return HasVideo() && !VideoQueue().IsFinished();
}

void
MediaDecoderStateMachine::CheckIfDecodeComplete()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (IsShutdown() ||
      mState == DECODER_STATE_SEEKING ||
      mState == DECODER_STATE_COMPLETED) {
    
    
    return;
  }
  if (!IsVideoDecoding() && !IsAudioDecoding()) {
    
    
    SetState(DECODER_STATE_COMPLETED);
    DispatchDecodeTasksIfNeeded();
    ScheduleStateMachine();
  }
  DECODER_LOG("CheckIfDecodeComplete %scompleted",
              ((mState == DECODER_STATE_COMPLETED) ? "" : "NOT "));
}

bool MediaDecoderStateMachine::IsPlaying() const
{
  AssertCurrentThreadInMonitor();
  return !mPlayStartTime.IsNull();
}

nsresult MediaDecoderStateMachine::Init(MediaDecoderStateMachine* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread());

  MediaDecoderReader* cloneReader = nullptr;
  if (aCloneDonor) {
    cloneReader = aCloneDonor->mReader;
  }

  nsresult rv = mReader->Init(cloneReader);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void MediaDecoderStateMachine::StopPlayback()
{
  MOZ_ASSERT(OnTaskQueue());
  DECODER_LOG("StopPlayback()");

  AssertCurrentThreadInMonitor();

  mDecoder->DispatchPlaybackStopped();

  if (IsPlaying()) {
    RenderVideoFrames(1);
    mPlayDuration = GetClock();
    SetPlayStartTime(TimeStamp());
  }
  
  
  mDecoder->GetReentrantMonitor().NotifyAll();
  NS_ASSERTION(!IsPlaying(), "Should report not playing at end of StopPlayback()");

  DispatchDecodeTasksIfNeeded();
}

void MediaDecoderStateMachine::MaybeStartPlayback()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING ||
             mState == DECODER_STATE_COMPLETED);

  if (IsPlaying()) {
    
    return;
  }

  bool playStatePermits = mPlayState == MediaDecoder::PLAY_STATE_PLAYING;
  if (!playStatePermits || mIsAudioPrerolling || mIsVideoPrerolling) {
    DECODER_LOG("Not starting playback [playStatePermits: %d, "
                "mIsAudioPrerolling: %d, mIsVideoPrerolling: %d]",
                (int) playStatePermits, (int) mIsAudioPrerolling, (int) mIsVideoPrerolling);
    return;
  }

  if (mDecoder->CheckDecoderCanOffloadAudio()) {
    DECODER_LOG("Offloading playback");
    return;
  }

  DECODER_LOG("MaybeStartPlayback() starting playback");

  mDecoder->DispatchPlaybackStarted();
  SetPlayStartTime(TimeStamp::Now());
  MOZ_ASSERT(IsPlaying());

  nsresult rv = StartAudioThread();
  NS_ENSURE_SUCCESS_VOID(rv);

  
  
  if (mAudioCaptured) {
    mDecodedStream->StartPlayback(GetMediaTime(), mInfo);
  }

  mDecoder->GetReentrantMonitor().NotifyAll();
  DispatchDecodeTasksIfNeeded();
}

void MediaDecoderStateMachine::UpdatePlaybackPositionInternal(int64_t aTime)
{
  MOZ_ASSERT(OnTaskQueue());
  SAMPLE_LOG("UpdatePlaybackPositionInternal(%lld)", aTime);
  AssertCurrentThreadInMonitor();

  mCurrentPosition = aTime;
  NS_ASSERTION(mCurrentPosition >= 0, "CurrentTime should be positive!");
  mObservedDuration = std::max(mObservedDuration.Ref(),
                               TimeUnit::FromMicroseconds(mCurrentPosition.Ref()));
}

void MediaDecoderStateMachine::UpdatePlaybackPosition(int64_t aTime)
{
  MOZ_ASSERT(OnTaskQueue());
  UpdatePlaybackPositionInternal(aTime);

  bool fragmentEnded = mFragmentEndTime >= 0 && GetMediaTime() >= mFragmentEndTime;
  mMetadataManager.DispatchMetadataIfNeeded(mDecoder, aTime);

  if (fragmentEnded) {
    StopPlayback();
  }
}

void MediaDecoderStateMachine::ClearPositionChangeFlag()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  AssertCurrentThreadInMonitor();

  mPositionChangeQueued = false;
}

static const char* const gMachineStateStr[] = {
  "NONE",
  "DECODING_METADATA",
  "WAIT_FOR_RESOURCES",
  "WAIT_FOR_CDM",
  "DECODING_FIRSTFRAME",
  "DORMANT",
  "DECODING",
  "SEEKING",
  "BUFFERING",
  "COMPLETED",
  "SHUTDOWN",
  "ERROR"
};

void MediaDecoderStateMachine::SetState(State aState)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (mState == aState) {
    return;
  }
  DECODER_LOG("Change machine state from %s to %s",
              gMachineStateStr[mState], gMachineStateStr[aState]);

  mState = aState;

  mIsShutdown = mState == DECODER_STATE_ERROR || mState == DECODER_STATE_SHUTDOWN;

  
  mSentPlaybackEndedEvent = false;
}

void MediaDecoderStateMachine::VolumeChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mAudioSink) {
    mAudioSink->SetVolume(mVolume);
  }
}

void MediaDecoderStateMachine::RecomputeDuration()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  TimeUnit duration;
  if (mExplicitDuration.Ref().isSome()) {
    double d = mExplicitDuration.Ref().ref();
    if (IsNaN(d)) {
      
      
      return;
    }
    
    
    duration = TimeUnit::FromSeconds(d);
  } else if (mEstimatedDuration.Ref().isSome()) {
    duration = mEstimatedDuration.Ref().ref();
  } else if (mInfo.mMetadataDuration.isSome()) {
    duration = mInfo.mMetadataDuration.ref();
  } else {
    return;
  }

  if (duration < mObservedDuration.Ref()) {
    duration = mObservedDuration;
  }

  MOZ_ASSERT(duration.ToMicroseconds() >= 0);
  mDuration = Some(duration);
}

void MediaDecoderStateMachine::SetDormant(bool aDormant)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (IsShutdown()) {
    return;
  }

  if (!mReader) {
    return;
  }

  DECODER_LOG("SetDormant=%d", aDormant);

  if (aDormant) {
    if (mState == DECODER_STATE_SEEKING) {
      if (mQueuedSeek.Exists()) {
        
      } else if (mPendingSeek.Exists()) {
        mQueuedSeek.Steal(mPendingSeek);
      } else if (mCurrentSeek.Exists()) {
        mQueuedSeek.Steal(mCurrentSeek);
      } else {
        mQueuedSeek.mTarget = SeekTarget(mCurrentPosition,
                                         SeekTarget::Accurate,
                                         MediaDecoderEventVisibility::Suppressed);
        
        
        nsRefPtr<MediaDecoder::SeekPromise> unused = mQueuedSeek.mPromise.Ensure(__func__);
      }
    } else {
      mQueuedSeek.mTarget = SeekTarget(mCurrentPosition,
                                       SeekTarget::Accurate,
                                       MediaDecoderEventVisibility::Suppressed);
      
      
      nsRefPtr<MediaDecoder::SeekPromise> unused = mQueuedSeek.mPromise.Ensure(__func__);
    }
    mPendingSeek.RejectIfExists(__func__);
    mCurrentSeek.RejectIfExists(__func__);
    SetState(DECODER_STATE_DORMANT);
    if (IsPlaying()) {
      StopPlayback();
    }

    Reset();

    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> r = NS_NewRunnableMethod(mReader, &MediaDecoderReader::ReleaseMediaResources);
    DecodeTaskQueue()->Dispatch(r.forget());
    mDecoder->GetReentrantMonitor().NotifyAll();
  } else if ((aDormant != true) && (mState == DECODER_STATE_DORMANT)) {
    mDecodingFrozenAtStateDecoding = true;
    ScheduleStateMachine();
    mCurrentPosition = 0;
    SetState(DECODER_STATE_DECODING_NONE);
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void MediaDecoderStateMachine::Shutdown()
{
  MOZ_ASSERT(OnTaskQueue());

  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  ScheduleStateMachine();
  SetState(DECODER_STATE_SHUTDOWN);
  if (mAudioSink) {
    mAudioSink->PrepareToShutdown();
  }

  mQueuedSeek.RejectIfExists(__func__);
  mPendingSeek.RejectIfExists(__func__);
  mCurrentSeek.RejectIfExists(__func__);

  if (IsPlaying()) {
    StopPlayback();
  }

  Reset();

  
  if (mStartTimeRendezvous) {
    mStartTimeRendezvous->Destroy();
  }

  
  
  ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__, &MediaDecoderReader::Shutdown)
    ->Then(TaskQueue(), __func__, this,
           &MediaDecoderStateMachine::FinishShutdown,
           &MediaDecoderStateMachine::FinishShutdown);
  DECODER_LOG("Shutdown started");
}

void MediaDecoderStateMachine::StartDecoding()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState == DECODER_STATE_DECODING) {
    return;
  }
  SetState(DECODER_STATE_DECODING);

  mDecodeStartTime = TimeStamp::Now();

  CheckIfDecodeComplete();
  if (mState == DECODER_STATE_COMPLETED) {
    return;
  }

  
  mIsAudioPrerolling = !DonePrerollingAudio();
  mIsVideoPrerolling = !DonePrerollingVideo();

  
  DispatchDecodeTasksIfNeeded();

  ScheduleStateMachine();
}

void MediaDecoderStateMachine::NotifyWaitingForResourcesStatusChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DECODER_LOG("NotifyWaitingForResourcesStatusChanged");

  if (mState == DECODER_STATE_WAIT_FOR_RESOURCES) {
    
    SetState(DECODER_STATE_DECODING_NONE);
    ScheduleStateMachine();
  } else if (mState == DECODER_STATE_WAIT_FOR_CDM &&
             !mReader->IsWaitingOnCDMResource()) {
    SetState(DECODER_STATE_DECODING_FIRSTFRAME);
    EnqueueDecodeFirstFrameTask();
  }
}

void MediaDecoderStateMachine::PlayStateChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
  if (mPlayState != MediaDecoder::PLAY_STATE_PLAYING) {
    return;
  }

  
  
  
  if (mMinimizePreroll) {
    mMinimizePreroll = false;
    DispatchDecodeTasksIfNeeded();
  }

  if (mDecodingFrozenAtStateDecoding) {
    mDecodingFrozenAtStateDecoding = false;
    DispatchDecodeTasksIfNeeded();
  }

  
  
  
  
  
  
  
  if (mState != DECODER_STATE_DECODING && mState != DECODER_STATE_BUFFERING &&
      mState != DECODER_STATE_COMPLETED)
  {
    DECODER_LOG("Unexpected state - Bailing out of PlayInternal()");
    return;
  }

  
  
  
  if (mState == DECODER_STATE_BUFFERING) {
    StartDecoding();
  }

  ScheduleStateMachine();
}

void MediaDecoderStateMachine::LogicallySeekingChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::BufferedRangeUpdated()
{
  MOZ_ASSERT(OnTaskQueue());

  
  
  
  
  
  if (!mBuffered.Ref().IsInvalid()) {
    bool exists;
    media::TimeUnit end{mBuffered.Ref().GetEnd(&exists)};
    if (exists) {
      mObservedDuration = std::max(mObservedDuration.Ref(), end);
    }
  }
}

nsRefPtr<MediaDecoder::SeekPromise>
MediaDecoderStateMachine::Seek(SeekTarget aTarget)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mDecodingFrozenAtStateDecoding = false;

  if (IsShutdown()) {
    return MediaDecoder::SeekPromise::CreateAndReject( true, __func__);
  }

  
  
  if (!mDecoder->IsMediaSeekable()) {
    DECODER_WARN("Seek() function should not be called on a non-seekable state machine");
    return MediaDecoder::SeekPromise::CreateAndReject( true, __func__);
  }

  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "We should have got duration already");

  if (mState < DECODER_STATE_DECODING) {
    DECODER_LOG("Seek() Not Enough Data to continue at this stage, queuing seek");
    mQueuedSeek.RejectIfExists(__func__);
    mQueuedSeek.mTarget = aTarget;
    return mQueuedSeek.mPromise.Ensure(__func__);
  }
  mQueuedSeek.RejectIfExists(__func__);
  mPendingSeek.RejectIfExists(__func__);
  mPendingSeek.mTarget = aTarget;

  DECODER_LOG("Changed state to SEEKING (to %lld)", mPendingSeek.mTarget.mTime);
  SetState(DECODER_STATE_SEEKING);
  ScheduleStateMachine();

  return mPendingSeek.mPromise.Ensure(__func__);
}

void MediaDecoderStateMachine::StopAudioThread()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (mAudioSink) {
    DECODER_LOG("Shutdown audio thread");
    mAudioSink->PrepareToShutdown();
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mAudioSink->Shutdown();
    }
    mAudioSink = nullptr;
  }
}

nsresult
MediaDecoderStateMachine::EnqueueDecodeFirstFrameTask()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_FIRSTFRAME);

  nsCOMPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::CallDecodeFirstFrame));
  TaskQueue()->Dispatch(task.forget());
  return NS_OK;
}

void
MediaDecoderStateMachine::DispatchDecodeTasksIfNeeded()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (mState != DECODER_STATE_DECODING &&
      mState != DECODER_STATE_DECODING_FIRSTFRAME &&
      mState != DECODER_STATE_BUFFERING &&
      mState != DECODER_STATE_SEEKING) {
    return;
  }

  if (mState == DECODER_STATE_DECODING && mDecodingFrozenAtStateDecoding) {
    DECODER_LOG("DispatchDecodeTasksIfNeeded return due to "
                "mFreezeDecodingAtStateDecoding");
    return;
  }
  
  
  
  
  
  
  
  
  
  
  
  

  const bool needToDecodeAudio = NeedToDecodeAudio();
  const bool needToDecodeVideo = NeedToDecodeVideo();

  
  MOZ_ASSERT(mState != DECODER_STATE_COMPLETED ||
             (!needToDecodeAudio && !needToDecodeVideo));

  bool needIdle = !IsLogicallyPlaying() &&
                  mState != DECODER_STATE_SEEKING &&
                  !needToDecodeAudio &&
                  !needToDecodeVideo &&
                  !IsPlaying();

  SAMPLE_LOG("DispatchDecodeTasksIfNeeded needAudio=%d audioStatus=%s needVideo=%d videoStatus=%s needIdle=%d",
             needToDecodeAudio, AudioRequestStatus(),
             needToDecodeVideo, VideoRequestStatus(),
             needIdle);

  if (needToDecodeAudio) {
    EnsureAudioDecodeTaskQueued();
  }
  if (needToDecodeVideo) {
    EnsureVideoDecodeTaskQueued();
  }

  if (needIdle) {
    DECODER_LOG("Dispatching SetIdle() audioQueue=%lld videoQueue=%lld",
                GetDecodedAudioDuration(),
                VideoQueue().Duration());
    nsCOMPtr<nsIRunnable> task = NS_NewRunnableMethod(mReader, &MediaDecoderReader::SetIdle);
    DecodeTaskQueue()->Dispatch(task.forget());
  }
}

void
MediaDecoderStateMachine::InitiateSeek()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  mCurrentSeek.RejectIfExists(__func__);
  mCurrentSeek.Steal(mPendingSeek);

  
  int64_t end = Duration().ToMicroseconds();
  NS_ASSERTION(end != -1, "Should know end time by now");
  int64_t seekTime = mCurrentSeek.mTarget.mTime;
  seekTime = std::min(seekTime, end);
  seekTime = std::max(int64_t(0), seekTime);
  NS_ASSERTION(seekTime >= 0 && seekTime <= end,
               "Can only seek in range [0,duration]");
  mCurrentSeek.mTarget.mTime = seekTime;

  if (mAudioCaptured) {
    mDecodedStream->RecreateData();
  }

  mDropAudioUntilNextDiscontinuity = HasAudio();
  mDropVideoUntilNextDiscontinuity = HasVideo();

  mDecoder->StopProgressUpdates();
  mCurrentTimeBeforeSeek = GetMediaTime();

  
  
  
  StopPlayback();
  UpdatePlaybackPositionInternal(mCurrentSeek.mTarget.mTime);

  nsCOMPtr<nsIRunnable> startEvent =
      NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
        mDecoder,
        &MediaDecoder::SeekingStarted,
        mCurrentSeek.mTarget.mEventVisibility);
  AbstractThread::MainThread()->Dispatch(startEvent.forget());

  
  Reset();

  
  nsRefPtr<MediaDecoderStateMachine> self = this;
  mSeekRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                    &MediaDecoderReader::Seek, mCurrentSeek.mTarget.mTime,
                                    Duration().ToMicroseconds())
    ->Then(TaskQueue(), __func__,
           [self] (int64_t) -> void {
             ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
             self->mSeekRequest.Complete();
             
             
             self->mDecodeToSeekTarget = true;
             self->DispatchDecodeTasksIfNeeded();
           }, [self] (nsresult aResult) -> void {
             ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
             self->mSeekRequest.Complete();
             MOZ_ASSERT(NS_FAILED(aResult), "Cancels should also disconnect mSeekRequest");
             self->DecodeError();
           }));
}

nsresult
MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  if (NeedToDecodeAudio()) {
    return EnsureAudioDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureAudioDecodeTaskQueued()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  SAMPLE_LOG("EnsureAudioDecodeTaskQueued isDecoding=%d status=%s",
              IsAudioDecoding(), AudioRequestStatus());

  if (mState != DECODER_STATE_DECODING &&
      mState != DECODER_STATE_DECODING_FIRSTFRAME &&
      mState != DECODER_STATE_BUFFERING &&
      mState != DECODER_STATE_SEEKING) {
    return NS_OK;
  }

  if (!IsAudioDecoding() || mAudioDataRequest.Exists() ||
      mAudioWaitRequest.Exists() || mSeekRequest.Exists()) {
    return NS_OK;
  }

  SAMPLE_LOG("Queueing audio task - queued=%i, decoder-queued=%o",
             AudioQueue().GetSize(), mReader->SizeOfAudioQueueInFrames());

  mAudioDataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(),
                                         __func__, &MediaDecoderReader::RequestAudioData)
    ->Then(TaskQueue(), __func__, this,
           &MediaDecoderStateMachine::OnAudioDecoded,
           &MediaDecoderStateMachine::OnAudioNotDecoded));

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  if (NeedToDecodeVideo()) {
    return EnsureVideoDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureVideoDecodeTaskQueued()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  SAMPLE_LOG("EnsureVideoDecodeTaskQueued isDecoding=%d status=%s",
             IsVideoDecoding(), VideoRequestStatus());

  if (mState != DECODER_STATE_DECODING &&
      mState != DECODER_STATE_DECODING_FIRSTFRAME &&
      mState != DECODER_STATE_BUFFERING &&
      mState != DECODER_STATE_SEEKING) {
    return NS_OK;
  }

  if (!IsVideoDecoding() || mVideoDataRequest.Exists() ||
      mVideoWaitRequest.Exists() || mSeekRequest.Exists()) {
    return NS_OK;
  }

  bool skipToNextKeyFrame = NeedToSkipToNextKeyframe();
  int64_t currentTime = mState == DECODER_STATE_SEEKING ? 0 : GetMediaTime();
  bool forceDecodeAhead = static_cast<uint32_t>(VideoQueue().GetSize()) <= SCARCE_VIDEO_QUEUE_SIZE;

  
  
  
  mVideoDecodeStartTime = TimeStamp::Now();

  SAMPLE_LOG("Queueing video task - queued=%i, decoder-queued=%o, skip=%i, time=%lld",
             VideoQueue().GetSize(), mReader->SizeOfVideoQueueInFrames(), skipToNextKeyFrame,
             currentTime);

  mVideoDataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                         &MediaDecoderReader::RequestVideoData,
                                         skipToNextKeyFrame, currentTime, forceDecodeAhead)
    ->Then(TaskQueue(), __func__, this,
           &MediaDecoderStateMachine::OnVideoDecoded,
           &MediaDecoderStateMachine::OnVideoNotDecoded));
  return NS_OK;
}

nsresult
MediaDecoderStateMachine::StartAudioThread()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (mAudioCaptured) {
    MOZ_ASSERT(!mAudioSink);
    return NS_OK;
  }

  if (HasAudio() && !mAudioSink) {
    auto audioStartTime = GetMediaTime();
    
    mAudioEndTime = audioStartTime;
    mAudioCompleted = false;
    mAudioSink = new AudioSink(this, audioStartTime,
                               mInfo.mAudio, mDecoder->GetAudioChannel());
    
    
    nsresult rv = mAudioSink->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    mAudioSink->SetVolume(mVolume);
    mAudioSink->SetPlaybackRate(mPlaybackRate);
    mAudioSink->SetPreservesPitch(mPreservesPitch);
  }
  return NS_OK;
}

int64_t MediaDecoderStateMachine::AudioDecodedUsecs()
{
  MOZ_ASSERT(OnTaskQueue());
  NS_ASSERTION(HasAudio(),
               "Should only call AudioDecodedUsecs() when we have audio");
  
  
  
  int64_t pushed = (AudioEndTime() != -1) ? (AudioEndTime() - GetMediaTime()) : 0;

  
  
  if (IsRealTime()) {
    return pushed + FramesToUsecs(AudioQueue().FrameCount(), mInfo.mAudio.mRate).value();
  }
  return pushed + AudioQueue().Duration();
}

bool MediaDecoderStateMachine::HasLowDecodedData(int64_t aAudioUsecs)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mReader->UseBufferingHeuristics());
  
  
  
  
  return ((IsAudioDecoding() && AudioDecodedUsecs() < aAudioUsecs) ||
         (IsVideoDecoding() &&
          static_cast<uint32_t>(VideoQueue().GetSize()) < LOW_VIDEO_FRAMES));
}

bool MediaDecoderStateMachine::OutOfDecodedAudio()
{
    MOZ_ASSERT(OnTaskQueue());
    return IsAudioDecoding() && !AudioQueue().IsFinished() &&
           AudioQueue().GetSize() == 0 &&
           (!mAudioSink || !mAudioSink->HasUnplayedFrames());
}

bool MediaDecoderStateMachine::HasLowUndecodedData()
{
  MOZ_ASSERT(OnTaskQueue());
  return HasLowUndecodedData(mLowDataThresholdUsecs);
}

bool MediaDecoderStateMachine::HasLowUndecodedData(int64_t aUsecs)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mState > DECODER_STATE_DECODING_FIRSTFRAME,
               "Must have loaded first frame for mBuffered to be valid");

  
  
  
  if (Duration().IsInfinite()) {
    return false;
  }

  if (mBuffered.Ref().IsInvalid()) {
    return false;
  }

  int64_t endOfDecodedVideoData = INT64_MAX;
  if (HasVideo() && !VideoQueue().AtEndOfStream()) {
    endOfDecodedVideoData = VideoQueue().Peek() ? VideoQueue().Peek()->GetEndTime() : mVideoFrameEndTime;
  }
  int64_t endOfDecodedAudioData = INT64_MAX;
  if (HasAudio() && !AudioQueue().AtEndOfStream()) {
    
    
    
    endOfDecodedAudioData = mDecodedAudioEndTime;
  }
  int64_t endOfDecodedData = std::min(endOfDecodedVideoData, endOfDecodedAudioData);
  if (Duration().ToMicroseconds() < endOfDecodedData) {
    
    return false;
  }
  media::TimeInterval interval(media::TimeUnit::FromMicroseconds(endOfDecodedData),
                               media::TimeUnit::FromMicroseconds(std::min(endOfDecodedData + aUsecs, Duration().ToMicroseconds())));
  return endOfDecodedData != INT64_MAX && !mBuffered.Ref().Contains(interval);
}

void
MediaDecoderStateMachine::DecodeError()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (IsShutdown()) {
    
    return;
  }

  
  
  SetState(DECODER_STATE_ERROR);
  ScheduleStateMachine();
  DECODER_WARN("Decode error, changed state to ERROR");

  
  
  mDecoder->GetReentrantMonitor().NotifyAll();

  
  
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
  AbstractThread::MainThread()->Dispatch(event.forget());
}

void
MediaDecoderStateMachine::OnMetadataRead(MetadataHolder* aMetadata)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_METADATA);
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mMetadataRequest.Complete();

  mDecoder->SetMediaSeekable(mReader->IsMediaSeekable());
  mInfo = aMetadata->mInfo;
  mMetadataTags = aMetadata->mTags.forget();
  nsRefPtr<MediaDecoderStateMachine> self = this;

  
  
  if (!mStartTimeRendezvous) {
    mStartTimeRendezvous = new StartTimeRendezvous(TaskQueue(), HasAudio(), HasVideo(),
                                                   mReader->ForceZeroStartTime() || IsRealTime());

    mStartTimeRendezvous->AwaitStartTime()->Then(TaskQueue(), __func__,
      [self] () -> void {
        NS_ENSURE_TRUE_VOID(!self->IsShutdown());
        self->mReader->DispatchSetStartTime(self->StartTime());
      },
      [] () -> void { NS_WARNING("Setting start time on reader failed"); }
    );
  }

  if (mInfo.mMetadataDuration.isSome()) {
    RecomputeDuration();
  } else if (mInfo.mUnadjustedMetadataEndTime.isSome()) {
    mStartTimeRendezvous->AwaitStartTime()->Then(TaskQueue(), __func__,
      [self] () -> void {
        NS_ENSURE_TRUE_VOID(!self->IsShutdown());
        TimeUnit unadjusted = self->mInfo.mUnadjustedMetadataEndTime.ref();
        TimeUnit adjustment = TimeUnit::FromMicroseconds(self->StartTime());
        self->mInfo.mMetadataDuration.emplace(unadjusted - adjustment);
        self->RecomputeDuration();
      }, [] () -> void { NS_WARNING("Adjusting metadata end time failed"); }
    );
  }

  if (HasVideo()) {
    DECODER_LOG("Video decode isAsync=%d HWAccel=%d videoQueueSize=%d",
                mReader->IsAsync(),
                mReader->VideoIsHardwareAccelerated(),
                GetAmpleVideoFrames());
  }

  mDecoder->StartProgressUpdates();

  
  
  
  
  
  
  mNotifyMetadataBeforeFirstFrame = mDuration.Ref().isSome() || mReader->IsWaitingOnCDMResource();
  if (mNotifyMetadataBeforeFirstFrame) {
    EnqueueLoadedMetadataEvent();
  }

  if (mReader->IsWaitingOnCDMResource()) {
    
    
    SetState(DECODER_STATE_WAIT_FOR_CDM);
    return;
  }

  SetState(DECODER_STATE_DECODING_FIRSTFRAME);
  EnqueueDecodeFirstFrameTask();
  ScheduleStateMachine();
}

void
MediaDecoderStateMachine::OnMetadataNotRead(ReadMetadataFailureReason aReason)
{
  MOZ_ASSERT(OnTaskQueue());
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_METADATA);
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mMetadataRequest.Complete();

  if (aReason == ReadMetadataFailureReason::WAITING_FOR_RESOURCES) {
    SetState(DECODER_STATE_WAIT_FOR_RESOURCES);
  } else {
    MOZ_ASSERT(aReason == ReadMetadataFailureReason::METADATA_ERROR);
    DECODER_WARN("Decode metadata failed, shutting down decoder");
    DecodeError();
  }
}

void
MediaDecoderStateMachine::EnqueueLoadedMetadataEvent()
{
  MOZ_ASSERT(OnTaskQueue());
  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  MediaDecoderEventVisibility visibility = mSentLoadedMetadataEvent?
                                    MediaDecoderEventVisibility::Suppressed :
                                    MediaDecoderEventVisibility::Observable;
  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new MetadataEventRunner(mDecoder, info, mMetadataTags, visibility);
  AbstractThread::MainThread()->Dispatch(metadataLoadedEvent.forget());
  mSentLoadedMetadataEvent = true;
}

void
MediaDecoderStateMachine::EnqueueFirstFrameLoadedEvent()
{
  MOZ_ASSERT(OnTaskQueue());
  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  MediaDecoderEventVisibility visibility = mSentFirstFrameLoadedEvent?
                                    MediaDecoderEventVisibility::Suppressed :
                                    MediaDecoderEventVisibility::Observable;
  nsCOMPtr<nsIRunnable> event =
    new FirstFrameLoadedEventRunner(mDecoder, info, visibility);
  AbstractThread::MainThread()->Dispatch(event.forget());
  mSentFirstFrameLoadedEvent = true;
}

void
MediaDecoderStateMachine::CallDecodeFirstFrame()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState != DECODER_STATE_DECODING_FIRSTFRAME) {
    return;
  }
  if (NS_FAILED(DecodeFirstFrame())) {
    DECODER_WARN("Decode failed to start, shutting down decoder");
    DecodeError();
  }
}

nsresult
MediaDecoderStateMachine::DecodeFirstFrame()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_FIRSTFRAME);
  DECODER_LOG("DecodeFirstFrame started");

  if (IsRealTime()) {
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else if (mSentFirstFrameLoadedEvent) {
    
    
    
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else {
    if (HasAudio()) {
      mAudioDataRequest.Begin(
        ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                       &MediaDecoderReader::RequestAudioData)
        ->Then(TaskQueue(), __func__, mStartTimeRendezvous.get(),
               &StartTimeRendezvous::ProcessFirstSample<AudioDataPromise>,
               &StartTimeRendezvous::FirstSampleRejected<AudioData>)
        ->CompletionPromise()
        ->Then(TaskQueue(), __func__, this,
               &MediaDecoderStateMachine::OnAudioDecoded,
               &MediaDecoderStateMachine::OnAudioNotDecoded)
      );
    }
    if (HasVideo()) {
      mVideoDecodeStartTime = TimeStamp::Now();
      mVideoDataRequest.Begin(
        ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                       &MediaDecoderReader::RequestVideoData, false, int64_t(0), false)
        ->Then(TaskQueue(), __func__, mStartTimeRendezvous.get(),
               &StartTimeRendezvous::ProcessFirstSample<VideoDataPromise>,
               &StartTimeRendezvous::FirstSampleRejected<VideoData>)
        ->CompletionPromise()
        ->Then(TaskQueue(), __func__, this,
               &MediaDecoderStateMachine::OnVideoDecoded,
               &MediaDecoderStateMachine::OnVideoNotDecoded));
    }
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::FinishDecodeFirstFrame()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  DECODER_LOG("FinishDecodeFirstFrame");

  if (IsShutdown()) {
    return NS_ERROR_FAILURE;
  }

  if (!IsRealTime() && !mSentFirstFrameLoadedEvent) {
    RenderVideoFrames(1);
  }

  
  if (mDuration.Ref().isNothing()) {
    mDuration = Some(TimeUnit::FromInfinity());
  }

  DECODER_LOG("Media duration %lld, "
              "transportSeekable=%d, mediaSeekable=%d",
              Duration().ToMicroseconds(), mDecoder->IsTransportSeekable(), mDecoder->IsMediaSeekable());

  if (HasAudio() && !HasVideo()) {
    
    
    
    mAmpleAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mLowAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mQuickBufferingLowDataThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
  }

  
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    mReader->ReadUpdatedMetadata(&mInfo);
  }

  if (!mNotifyMetadataBeforeFirstFrame) {
    
    EnqueueLoadedMetadataEvent();
  }
  EnqueueFirstFrameLoadedEvent();

  if (mQueuedSeek.Exists()) {
    mPendingSeek.Steal(mQueuedSeek);
    SetState(DECODER_STATE_SEEKING);
    ScheduleStateMachine();
  } else if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
    
    StartDecoding();
  }

  return NS_OK;
}

void
MediaDecoderStateMachine::SeekCompleted()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(mState == DECODER_STATE_SEEKING);

  int64_t seekTime = mCurrentSeek.mTarget.mTime;
  int64_t newCurrentTime = seekTime;

  
  nsRefPtr<VideoData> video = VideoQueue().PeekFront();
  if (seekTime == Duration().ToMicroseconds()) {
    newCurrentTime = seekTime;
  } else if (HasAudio()) {
    AudioData* audio = AudioQueue().PeekFront();
    
    
    
    
    
    
    int64_t videoStart = video ? video->mTime : seekTime;
    int64_t audioStart = audio ? audio->mTime : seekTime;
    newCurrentTime = std::min(audioStart, videoStart);
  } else {
    newCurrentTime = video ? video->mTime : seekTime;
  }
  mPlayDuration = newCurrentTime;

  mDecoder->StartProgressUpdates();

  
  
  

  bool isLiveStream = mDecoder->GetResource()->IsLiveStream();
  if (mPendingSeek.Exists()) {
    
    
    DECODER_LOG("A new seek came along while we were finishing the old one - staying in SEEKING");
    SetState(DECODER_STATE_SEEKING);
  } else if (GetMediaTime() == Duration().ToMicroseconds() && !isLiveStream) {
    
    
    
    DECODER_LOG("Changed state from SEEKING (to %lld) to COMPLETED", seekTime);
    
    
    SetState(DECODER_STATE_COMPLETED);
    DispatchDecodeTasksIfNeeded();
  } else {
    DECODER_LOG("Changed state from SEEKING (to %lld) to DECODING", seekTime);
    StartDecoding();
  }

  
  UpdatePlaybackPositionInternal(newCurrentTime);

  
  DECODER_LOG("Seek completed, mCurrentPosition=%lld", mCurrentPosition.Ref());

  
  
  
  mQuickBuffering = false;

  mCurrentSeek.Resolve(mState == DECODER_STATE_COMPLETED, __func__);
  ScheduleStateMachine();

  if (video) {
    RenderVideoFrames(1);
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::Invalidate);
    AbstractThread::MainThread()->Dispatch(event.forget());
  }
}

class DecoderDisposer
{
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(DecoderDisposer)
  DecoderDisposer(MediaDecoder* aDecoder, MediaDecoderStateMachine* aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}

  void OnTaskQueueShutdown()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mStateMachine);
    MOZ_ASSERT(mDecoder);
    mStateMachine->BreakCycles();
    mDecoder->BreakCycles();
    mStateMachine = nullptr;
    mDecoder = nullptr;
  }

private:
  virtual ~DecoderDisposer() {}
  nsRefPtr<MediaDecoder> mDecoder;
  nsRefPtr<MediaDecoderStateMachine> mStateMachine;
};

void
MediaDecoderStateMachine::FinishShutdown()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
  AudioQueue().ClearListeners();
  VideoQueue().ClearListeners();

  
  mBuffered.DisconnectIfConnected();
  mEstimatedDuration.DisconnectIfConnected();
  mExplicitDuration.DisconnectIfConnected();
  mPlayState.DisconnectIfConnected();
  mNextPlayState.DisconnectIfConnected();
  mLogicallySeeking.DisconnectIfConnected();
  mVolume.DisconnectIfConnected();
  mLogicalPlaybackRate.DisconnectIfConnected();
  mPreservesPitch.DisconnectIfConnected();
  mDuration.DisconnectAll();
  mIsShutdown.DisconnectAll();
  mNextFrameStatus.DisconnectAll();
  mCurrentPosition.DisconnectAll();

  
  mWatchManager.Shutdown();

  MOZ_ASSERT(mState == DECODER_STATE_SHUTDOWN,
             "How did we escape from the shutdown state?");
  
  
  
  
  
  
  
  
  
  
  
  
  
  DECODER_LOG("Shutting down state machine task queue");
  RefPtr<DecoderDisposer> disposer = new DecoderDisposer(mDecoder, this);
  TaskQueue()->BeginShutdown()->Then(AbstractThread::MainThread(), __func__,
                                     disposer.get(),
                                     &DecoderDisposer::OnTaskQueueShutdown,
                                     &DecoderDisposer::OnTaskQueueShutdown);
}

nsresult MediaDecoderStateMachine::RunStateMachine()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mDelayedScheduler.Reset(); 
  mDispatchedStateMachine = false;

  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource, NS_ERROR_NULL_POINTER);

  switch (mState) {
    case DECODER_STATE_ERROR:
    case DECODER_STATE_SHUTDOWN:
    case DECODER_STATE_DORMANT:
    case DECODER_STATE_WAIT_FOR_CDM:
    case DECODER_STATE_WAIT_FOR_RESOURCES:
      return NS_OK;

    case DECODER_STATE_DECODING_NONE: {
      SetState(DECODER_STATE_DECODING_METADATA);
      ScheduleStateMachine();
      return NS_OK;
    }

    case DECODER_STATE_DECODING_METADATA: {
      if (!mMetadataRequest.Exists()) {
        DECODER_LOG("Dispatching AsyncReadMetadata");
        mMetadataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                              &MediaDecoderReader::AsyncReadMetadata)
          ->Then(TaskQueue(), __func__, this,
                 &MediaDecoderStateMachine::OnMetadataRead,
                 &MediaDecoderStateMachine::OnMetadataNotRead));

      }
      return NS_OK;
    }

    case DECODER_STATE_DECODING_FIRSTFRAME: {
      
      return NS_OK;
    }

    case DECODER_STATE_DECODING: {
      if (mPlayState != MediaDecoder::PLAY_STATE_PLAYING && IsPlaying())
      {
        
        
        StopPlayback();
      }

      
      MaybeStartPlayback();

      UpdateRenderedVideoFrames();
      NS_ASSERTION(!IsPlaying() ||
                   mLogicallySeeking ||
                   IsStateMachineScheduled(),
                   "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_BUFFERING: {
      TimeStamp now = TimeStamp::Now();
      NS_ASSERTION(!mBufferingStart.IsNull(), "Must know buffering start time.");

      
      
      
      if (mReader->UseBufferingHeuristics()) {
        TimeDuration elapsed = now - mBufferingStart;
        bool isLiveStream = resource->IsLiveStream();
        if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
              elapsed < TimeDuration::FromSeconds(mBufferingWait * mPlaybackRate) &&
              (mQuickBuffering ? HasLowDecodedData(mQuickBufferingLowDataThresholdUsecs)
                               : HasLowUndecodedData(mBufferingWait * USECS_PER_S)) &&
              mDecoder->IsExpectingMoreData())
        {
          DECODER_LOG("Buffering: wait %ds, timeout in %.3lfs %s",
                      mBufferingWait, mBufferingWait - elapsed.ToSeconds(),
                      (mQuickBuffering ? "(quick exit)" : ""));
          ScheduleStateMachineIn(USECS_PER_S);
          return NS_OK;
        }
      } else if (OutOfDecodedAudio() || OutOfDecodedVideo()) {
        MOZ_ASSERT(mReader->IsWaitForDataSupported(),
                   "Don't yet have a strategy for non-heuristic + non-WaitForData");
        DispatchDecodeTasksIfNeeded();
        MOZ_ASSERT_IF(!mMinimizePreroll && OutOfDecodedAudio(), mAudioDataRequest.Exists() || mAudioWaitRequest.Exists());
        MOZ_ASSERT_IF(!mMinimizePreroll && OutOfDecodedVideo(), mVideoDataRequest.Exists() || mVideoWaitRequest.Exists());
        DECODER_LOG("In buffering mode, waiting to be notified: outOfAudio: %d, "
                    "mAudioStatus: %s, outOfVideo: %d, mVideoStatus: %s",
                    OutOfDecodedAudio(), AudioRequestStatus(),
                    OutOfDecodedVideo(), VideoRequestStatus());
        return NS_OK;
      }

      DECODER_LOG("Changed state from BUFFERING to DECODING");
      DECODER_LOG("Buffered for %.3lfs", (now - mBufferingStart).ToSeconds());
      StartDecoding();

      
      mDecoder->GetReentrantMonitor().NotifyAll();
      NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_SEEKING: {
      if (mPendingSeek.Exists()) {
        InitiateSeek();
      }
      return NS_OK;
    }

    case DECODER_STATE_COMPLETED: {
      if (mPlayState != MediaDecoder::PLAY_STATE_PLAYING && IsPlaying()) {
        StopPlayback();
      }
      
      
      
      if (VideoQueue().GetSize() > 1 ||
          (HasAudio() && !mAudioCompleted) ||
          (mAudioCaptured && !mDecodedStream->IsFinished()))
      {
        
        MaybeStartPlayback();
        UpdateRenderedVideoFrames();
        NS_ASSERTION(!IsPlaying() ||
                     mLogicallySeeking ||
                     IsStateMachineScheduled(),
                     "Must have timer scheduled");
        return NS_OK;
      }

      
      
      StopPlayback();

      if (mState != DECODER_STATE_COMPLETED) {
        
        
        NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
        return NS_OK;
      }

      if (mPlayState == MediaDecoder::PLAY_STATE_PLAYING &&
          !mSentPlaybackEndedEvent)
      {
        int64_t clockTime = std::max(AudioEndTime(), mVideoFrameEndTime);
        clockTime = std::max(int64_t(0), std::max(clockTime, Duration().ToMicroseconds()));
        UpdatePlaybackPosition(clockTime);

        nsCOMPtr<nsIRunnable> event =
          NS_NewRunnableMethod(mDecoder, &MediaDecoder::PlaybackEnded);
        AbstractThread::MainThread()->Dispatch(event.forget());

        mSentPlaybackEndedEvent = true;
      }

      
      
      StopAudioThread();
      mDecodedStream->StopPlayback();

      return NS_OK;
    }
  }

  return NS_OK;
}

void
MediaDecoderStateMachine::Reset()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  DECODER_LOG("MediaDecoderStateMachine::Reset");

  
  
  
  
  MOZ_ASSERT(IsShutdown() ||
             mState == DECODER_STATE_SEEKING ||
             mState == DECODER_STATE_DORMANT ||
             mState == DECODER_STATE_DECODING_NONE);

  
  
  
  StopAudioThread();
  mDecodedStream->StopPlayback();

  mVideoFrameEndTime = -1;
  mDecodedVideoEndTime = -1;
  mAudioEndTime = -1;
  mDecodedAudioEndTime = -1;
  mAudioCompleted = false;
  AudioQueue().Reset();
  VideoQueue().Reset();
  mFirstVideoFrameAfterSeek = nullptr;
  mDropAudioUntilNextDiscontinuity = true;
  mDropVideoUntilNextDiscontinuity = true;
  mDecodeToSeekTarget = false;

  mMetadataRequest.DisconnectIfExists();
  mAudioDataRequest.DisconnectIfExists();
  mAudioWaitRequest.DisconnectIfExists();
  mVideoDataRequest.DisconnectIfExists();
  mVideoWaitRequest.DisconnectIfExists();
  mSeekRequest.DisconnectIfExists();

  nsCOMPtr<nsIRunnable> resetTask =
    NS_NewRunnableMethod(mReader, &MediaDecoderReader::ResetDecode);
  DecodeTaskQueue()->Dispatch(resetTask.forget());
}

void MediaDecoderStateMachine::CheckTurningOffHardwareDecoder(VideoData* aData)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  
  if (aData->mImage && !aData->mImage->IsValid()) {
    MediaDecoder::FrameStatistics& frameStats = mDecoder->GetFrameStatistics();
    frameStats.NotifyCorruptFrame();
    
    
    
    mCorruptFrames.insert(10);
    if (!mDisabledHardwareAcceleration &&
        mReader->VideoIsHardwareAccelerated() &&
        frameStats.GetPresentedFrames() > 30 &&
        mCorruptFrames.mean() >= 1 ) {
        nsCOMPtr<nsIRunnable> task =
          NS_NewRunnableMethod(mReader, &MediaDecoderReader::DisableHardwareAcceleration);
        DecodeTaskQueue()->Dispatch(task.forget());
      mDisabledHardwareAcceleration = true;
      gfxCriticalNote << "Too many dropped/corrupted frames, disabling DXVA";
    }
  } else {
    mCorruptFrames.insert(0);
  }
}

void MediaDecoderStateMachine::RenderVideoFrames(int32_t aMaxFrames,
                                                 int64_t aClockTime,
                                                 const TimeStamp& aClockTimeStamp)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  nsAutoTArray<nsRefPtr<VideoData>,16> frames;
  VideoQueue().GetFirstElements(aMaxFrames, &frames);
  if (frames.IsEmpty() || !container) {
    return;
  }

  nsAutoTArray<ImageContainer::NonOwningImage,16> images;
  TimeStamp lastFrameTime;
  for (uint32_t i = 0; i < frames.Length(); ++i) {
    VideoData* frame = frames[i];
    frame->mSentToCompositor = true;

    int64_t frameTime = frame->mTime;
    if (frameTime < 0) {
      
      continue;
    }

    TimeStamp t;
    if (aMaxFrames > 1) {
      MOZ_ASSERT(!aClockTimeStamp.IsNull());
      int64_t delta = frame->mTime - aClockTime;
      t = aClockTimeStamp +
          TimeDuration::FromMicroseconds(delta / mPlaybackRate);
      if (!lastFrameTime.IsNull() && t <= lastFrameTime) {
        
        
        
        
        continue;
      }
      lastFrameTime = t;
    }

    ImageContainer::NonOwningImage* img = images.AppendElement();
    img->mTimeStamp = t;
    img->mImage = frame->mImage;
    img->mFrameID = frame->mFrameID;
    img->mProducerID = mProducerID;

    VERBOSE_LOG("playing video frame %lld (id=%d) (queued=%i, state-machine=%i, decoder-queued=%i)",
                frame->mTime, frame->mFrameID,
                VideoQueue().GetSize() + mReader->SizeOfVideoQueueInFrames(),
                VideoQueue().GetSize(), mReader->SizeOfVideoQueueInFrames());
  }

  container->SetCurrentFrames(frames[0]->mDisplay, images);
}

void MediaDecoderStateMachine::ResyncAudioClock()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (IsPlaying()) {
    SetPlayStartTime(TimeStamp::Now());
    mPlayDuration = GetAudioClock();
  }
}

int64_t
MediaDecoderStateMachine::GetAudioClock() const
{
  MOZ_ASSERT(OnTaskQueue());
  
  
  
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(HasAudio() && !mAudioCompleted && IsPlaying());
  
  
  MOZ_ASSERT(mAudioSink);
  return mAudioSink->GetPosition();
}

int64_t MediaDecoderStateMachine::GetStreamClock() const
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  return mDecodedStream->GetPosition();
}

int64_t MediaDecoderStateMachine::GetVideoStreamPosition(TimeStamp aTimeStamp) const
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (!IsPlaying()) {
    return mPlayDuration;
  }

  
  int64_t delta = DurationToUsecs(aTimeStamp - mPlayStartTime);
  
  delta *= mPlaybackRate;
  return mPlayDuration + delta;
}

int64_t MediaDecoderStateMachine::GetClock(TimeStamp* aTimeStamp) const
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  
  
  
  
  int64_t clock_time = -1;
  TimeStamp t;
  if (!IsPlaying()) {
    clock_time = mPlayDuration;
  } else {
    if (mAudioCaptured) {
      clock_time = GetStreamClock();
    } else if (HasAudio() && !mAudioCompleted) {
      clock_time = GetAudioClock();
    } else {
      t = TimeStamp::Now();
      
      clock_time = GetVideoStreamPosition(t);
    }
    NS_ASSERTION(GetMediaTime() <= clock_time, "Clock should go forwards.");
  }
  if (aTimeStamp) {
    *aTimeStamp = t.IsNull() ? TimeStamp::Now() : t;
  }

  return clock_time;
}

void MediaDecoderStateMachine::UpdateRenderedVideoFrames()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();

  if (!IsPlaying() || mLogicallySeeking) {
    return;
  }

  if (mAudioCaptured) {
    SendStreamData();
  }

  TimeStamp nowTime;
  const int64_t clockTime = GetClock(&nowTime);
  
  
  
  NS_ASSERTION(clockTime >= 0, "Should have positive clock time.");
  int64_t remainingTime = AUDIO_DURATION_USECS;
  if (VideoQueue().GetSize() > 0) {
    nsRefPtr<VideoData> currentFrame = VideoQueue().PopFront();
    int32_t framesRemoved = 0;
    while (VideoQueue().GetSize() > 0) {
      VideoData* nextFrame = VideoQueue().PeekFront();
      if (!IsRealTime() && nextFrame->mTime > clockTime) {
        remainingTime = nextFrame->mTime - clockTime;
        break;
      }
      ++framesRemoved;
      if (!currentFrame->mSentToCompositor) {
        mDecoder->NotifyDecodedFrames(0, 0, 1);
        VERBOSE_LOG("discarding video frame mTime=%lld clock_time=%lld",
                    currentFrame->mTime, clockTime);
      }
      CheckTurningOffHardwareDecoder(currentFrame);
      currentFrame = VideoQueue().PopFront();

    }
    VideoQueue().PushFront(currentFrame);
    if (framesRemoved > 0) {
      OnPlaybackOffsetUpdate(currentFrame->mOffset);
      mVideoFrameEndTime = currentFrame->GetEndTime();
      MediaDecoder::FrameStatistics& frameStats = mDecoder->GetFrameStatistics();
      frameStats.NotifyPresentedFrame();
    }
  }

  RenderVideoFrames(sVideoQueueSendToCompositorSize, clockTime, nowTime);

  
  
  if (mState == DECODER_STATE_DECODING &&
      mPlayState == MediaDecoder::PLAY_STATE_PLAYING &&
      mDecoder->IsExpectingMoreData()) {
    bool shouldBuffer;
    if (mReader->UseBufferingHeuristics()) {
      shouldBuffer = HasLowDecodedData(remainingTime + EXHAUSTED_DATA_MARGIN_USECS) &&
                     (JustExitedQuickBuffering() || HasLowUndecodedData());
    } else {
      MOZ_ASSERT(mReader->IsWaitForDataSupported());
      shouldBuffer = (OutOfDecodedAudio() && mAudioWaitRequest.Exists()) ||
                     (OutOfDecodedVideo() && mVideoWaitRequest.Exists());
    }
    if (shouldBuffer) {
      StartBuffering();
      
      
      
      ScheduleStateMachineIn(USECS_PER_S);
      return;
    }
  }

  
  
  
  if (mVideoFrameEndTime != -1 || AudioEndTime() != -1) {
    
    int64_t t = std::min(clockTime, std::max(mVideoFrameEndTime, AudioEndTime()));
    
    
    if (t > GetMediaTime()) {
      UpdatePlaybackPosition(t);
    }
  }
  
  
  
  

  int64_t delay = std::max<int64_t>(1, remainingTime / mPlaybackRate);
  ScheduleStateMachineIn(delay);
}

nsresult
MediaDecoderStateMachine::DropVideoUpToSeekTarget(VideoData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  nsRefPtr<VideoData> video(aSample);
  MOZ_ASSERT(video);
  DECODER_LOG("DropVideoUpToSeekTarget() frame [%lld, %lld]",
              video->mTime, video->GetEndTime());
  MOZ_ASSERT(mCurrentSeek.Exists());
  const int64_t target = mCurrentSeek.mTarget.mTime;

  
  
  if (target >= video->GetEndTime()) {
    DECODER_LOG("DropVideoUpToSeekTarget() pop video frame [%lld, %lld] target=%lld",
                video->mTime, video->GetEndTime(), target);
    mFirstVideoFrameAfterSeek = video;
  } else {
    if (target >= video->mTime && video->GetEndTime() >= target) {
      
      
      
      nsRefPtr<VideoData> temp = VideoData::ShallowCopyUpdateTimestamp(video, target);
      video = temp;
    }
    mFirstVideoFrameAfterSeek = nullptr;

    DECODER_LOG("DropVideoUpToSeekTarget() found video frame [%lld, %lld] containing target=%lld",
                video->mTime, video->GetEndTime(), target);

    PushFront(video);
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::DropAudioUpToSeekTarget(AudioData* aSample)
{
  MOZ_ASSERT(OnTaskQueue());
  nsRefPtr<AudioData> audio(aSample);
  MOZ_ASSERT(audio &&
             mCurrentSeek.Exists() &&
             mCurrentSeek.mTarget.mType == SeekTarget::Accurate);

  CheckedInt64 startFrame = UsecsToFrames(audio->mTime,
                                          mInfo.mAudio.mRate);
  CheckedInt64 targetFrame = UsecsToFrames(mCurrentSeek.mTarget.mTime,
                                           mInfo.mAudio.mRate);
  if (!startFrame.isValid() || !targetFrame.isValid()) {
    return NS_ERROR_FAILURE;
  }
  if (startFrame.value() + audio->mFrames <= targetFrame.value()) {
    
    
    return NS_OK;
  }
  if (startFrame.value() > targetFrame.value()) {
    
    
    
    
    
    
    
    DECODER_WARN("Audio not synced after seek, maybe a poorly muxed file?");
    Push(audio);
    return NS_OK;
  }

  
  
  
  NS_ASSERTION(targetFrame.value() >= startFrame.value(),
               "Target must at or be after data start.");
  NS_ASSERTION(targetFrame.value() < startFrame.value() + audio->mFrames,
               "Data must end after target.");

  int64_t framesToPrune = targetFrame.value() - startFrame.value();
  if (framesToPrune > audio->mFrames) {
    
    
    DECODER_WARN("Can't prune more frames that we have!");
    return NS_ERROR_FAILURE;
  }
  uint32_t frames = audio->mFrames - static_cast<uint32_t>(framesToPrune);
  uint32_t channels = audio->mChannels;
  nsAutoArrayPtr<AudioDataValue> audioData(new AudioDataValue[frames * channels]);
  memcpy(audioData.get(),
         audio->mAudioData.get() + (framesToPrune * channels),
         frames * channels * sizeof(AudioDataValue));
  CheckedInt64 duration = FramesToUsecs(frames, mInfo.mAudio.mRate);
  if (!duration.isValid()) {
    return NS_ERROR_FAILURE;
  }
  nsRefPtr<AudioData> data(new AudioData(audio->mOffset,
                                         mCurrentSeek.mTarget.mTime,
                                         duration.value(),
                                         frames,
                                         audioData.forget(),
                                         channels,
                                         audio->mRate));
  PushFront(data);

  return NS_OK;
}

void MediaDecoderStateMachine::UpdateNextFrameStatus()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  MediaDecoderOwner::NextFrameStatus status;
  const char* statusString;
  if (mState <= DECODER_STATE_DECODING_FIRSTFRAME) {
    status = MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
    statusString = "NEXT_FRAME_UNAVAILABLE";
  } else if (IsBuffering()) {
    status = MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_BUFFERING;
    statusString = "NEXT_FRAME_UNAVAILABLE_BUFFERING";
  } else if (IsSeeking()) {
    status = MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_SEEKING;
    statusString = "NEXT_FRAME_UNAVAILABLE_SEEKING";
  } else if (HaveNextFrameData()) {
    status = MediaDecoderOwner::NEXT_FRAME_AVAILABLE;
    statusString = "NEXT_FRAME_AVAILABLE";
  } else {
    status = MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
    statusString = "NEXT_FRAME_UNAVAILABLE";
  }

  if (status != mNextFrameStatus) {
    DECODER_LOG("Changed mNextFrameStatus to %s", statusString);
  }

  mNextFrameStatus = status;
}

bool MediaDecoderStateMachine::JustExitedQuickBuffering()
{
  MOZ_ASSERT(OnTaskQueue());
  return !mDecodeStartTime.IsNull() &&
    mQuickBuffering &&
    (TimeStamp::Now() - mDecodeStartTime) < TimeDuration::FromMicroseconds(QUICK_BUFFER_THRESHOLD_USECS);
}

void MediaDecoderStateMachine::StartBuffering()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mState != DECODER_STATE_DECODING) {
    
    
    
    
    
    return;
  }

  if (IsPlaying()) {
    StopPlayback();
  }

  TimeDuration decodeDuration = TimeStamp::Now() - mDecodeStartTime;
  
  
  
  mQuickBuffering =
    !JustExitedQuickBuffering() &&
    decodeDuration < UsecsToDuration(QUICK_BUFFER_THRESHOLD_USECS);
  mBufferingStart = TimeStamp::Now();

  SetState(DECODER_STATE_BUFFERING);
  DECODER_LOG("Changed state from DECODING to BUFFERING, decoded for %.3lfs",
              decodeDuration.ToSeconds());
  MediaDecoder::Statistics stats = mDecoder->GetStatistics();
  DECODER_LOG("Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
              stats.mPlaybackRate/1024, stats.mPlaybackRateReliable ? "" : " (unreliable)",
              stats.mDownloadRate/1024, stats.mDownloadRateReliable ? "" : " (unreliable)");
}

void MediaDecoderStateMachine::SetPlayStartTime(const TimeStamp& aTimeStamp)
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  mPlayStartTime = aTimeStamp;

  if (mAudioSink) {
    mAudioSink->SetPlaying(!mPlayStartTime.IsNull());
  }
  
  
  
  mDecodedStream->SetPlaying(!mPlayStartTime.IsNull());
}

void MediaDecoderStateMachine::ScheduleStateMachineWithLockAndWakeDecoder()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DispatchAudioDecodeTaskIfNeeded();
  DispatchVideoDecodeTaskIfNeeded();
}

void
MediaDecoderStateMachine::ScheduleStateMachine()
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (mDispatchedStateMachine) {
    return;
  }
  mDispatchedStateMachine = true;

  nsCOMPtr<nsIRunnable> task =
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::RunStateMachine);
  TaskQueue()->Dispatch(task.forget());
}

void
MediaDecoderStateMachine::ScheduleStateMachineIn(int64_t aMicroseconds)
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(OnTaskQueue());          
                                      
                                      
  MOZ_ASSERT(aMicroseconds > 0);
  if (mDispatchedStateMachine) {
    return;
  }

  
  if (IsRealTime()) {
    aMicroseconds = std::min(aMicroseconds, int64_t(40000));
  }

  TimeStamp now = TimeStamp::Now();
  TimeStamp target = now + TimeDuration::FromMicroseconds(aMicroseconds);

  SAMPLE_LOG("Scheduling state machine for %lf ms from now", (target - now).ToMilliseconds());
  mDelayedScheduler.Ensure(target);
}

bool MediaDecoderStateMachine::OnDecodeTaskQueue() const
{
  return !DecodeTaskQueue() || DecodeTaskQueue()->IsCurrentThreadIn();
}

bool MediaDecoderStateMachine::OnTaskQueue() const
{
  return TaskQueue()->IsCurrentThreadIn();
}

bool MediaDecoderStateMachine::IsStateMachineScheduled() const
{
  MOZ_ASSERT(OnTaskQueue());
  return mDispatchedStateMachine || mDelayedScheduler.IsScheduled();
}

void
MediaDecoderStateMachine::LogicalPlaybackRateChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mLogicalPlaybackRate == 0) {
    
    return;
  }

  
  
  
  if (!HasAudio() && IsPlaying()) {
    
    
    TimeStamp now = TimeStamp::Now();
    mPlayDuration = GetVideoStreamPosition(now);
    SetPlayStartTime(now);
  }

  mPlaybackRate = mLogicalPlaybackRate;
  if (mAudioSink) {
    mAudioSink->SetPlaybackRate(mPlaybackRate);
  }

  ScheduleStateMachine();
}

void MediaDecoderStateMachine::PreservesPitchChanged()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mAudioSink) {
    mAudioSink->SetPreservesPitch(mPreservesPitch);
  }
}

bool MediaDecoderStateMachine::IsShutdown()
{
  MOZ_ASSERT(OnTaskQueue());
  return mIsShutdown;
}

void MediaDecoderStateMachine::QueueMetadata(int64_t aPublishTime,
                                             nsAutoPtr<MediaInfo> aInfo,
                                             nsAutoPtr<MetadataTags> aTags)
{
  MOZ_ASSERT(OnDecodeTaskQueue());
  AssertCurrentThreadInMonitor();
  TimedMetadata* metadata = new TimedMetadata;
  metadata->mPublishTime = aPublishTime;
  metadata->mInfo = aInfo.forget();
  metadata->mTags = aTags.forget();
  mMetadataManager.QueueMetadata(metadata);
}

int64_t
MediaDecoderStateMachine::AudioEndTime() const
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  if (mAudioSink) {
    return mAudioSink->GetEndTime();
  }
  
  
  MOZ_ASSERT(mAudioCaptured || !mAudioCompleted);
  return mAudioEndTime;
}

void MediaDecoderStateMachine::OnAudioEndTimeUpdate(int64_t aAudioEndTime)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(aAudioEndTime >= AudioEndTime());
  mAudioEndTime = aAudioEndTime;
}

void MediaDecoderStateMachine::OnPlaybackOffsetUpdate(int64_t aPlaybackOffset)
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mDecoder->UpdatePlaybackOffset(aPlaybackOffset);
}

void MediaDecoderStateMachine::OnAudioSinkComplete()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mAudioCaptured) {
    return;
  }
  ResyncAudioClock();
  mAudioCompleted = true;
  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void MediaDecoderStateMachine::OnAudioSinkError()
{
  MOZ_ASSERT(OnTaskQueue());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  if (mAudioCaptured) {
    return;
  }

  ResyncAudioClock();
  mAudioCompleted = true;

  
  if (HasVideo()) {
    return;
  }

  
  
  DecodeError();
}

uint32_t MediaDecoderStateMachine::GetAmpleVideoFrames() const
{
  MOZ_ASSERT(OnTaskQueue());
  AssertCurrentThreadInMonitor();
  return (mReader->IsAsync() && mReader->VideoIsHardwareAccelerated())
    ? std::max<uint32_t>(sVideoQueueHWAccelSize, MIN_VIDEO_QUEUE_SIZE)
    : std::max<uint32_t>(sVideoQueueDefaultSize, MIN_VIDEO_QUEUE_SIZE);
}

void MediaDecoderStateMachine::DispatchAudioCaptured()
{
  nsRefPtr<MediaDecoderStateMachine> self = this;
  nsCOMPtr<nsIRunnable> r = NS_NewRunnableFunction([self] () -> void
  {
    MOZ_ASSERT(self->OnTaskQueue());
    ReentrantMonitorAutoEnter mon(self->mDecoder->GetReentrantMonitor());
    if (!self->mAudioCaptured) {
      
      self->StopAudioThread();
      
      
      self->mAudioEndTime = -1;
      self->mAudioCaptured = true;
      
      
      if (self->IsPlaying()) {
        self->mDecodedStream->StartPlayback(self->GetMediaTime(), self->mInfo);
      }
      self->ScheduleStateMachine();
    }
  });
  TaskQueue()->Dispatch(r.forget());
}

void MediaDecoderStateMachine::AddOutputStream(ProcessedMediaStream* aStream,
                                               bool aFinishWhenEnded)
{
  MOZ_ASSERT(NS_IsMainThread());
  DECODER_LOG("AddOutputStream aStream=%p!", aStream);
  mDecodedStream->Connect(aStream, aFinishWhenEnded);
  DispatchAudioCaptured();
}

} 


#undef LOG
#undef DECODER_LOG
#undef VERBOSE_LOG
#undef DECODER_WARN
#undef DECODER_WARN_HELPER

#undef NS_DispatchToMainThread
