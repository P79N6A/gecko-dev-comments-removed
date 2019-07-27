




#ifdef XP_WIN

#include "windows.h"
#include "mmsystem.h"
#endif

#include "mozilla/DebugOnly.h"
#include <stdint.h>

#include "MediaDecoderStateMachine.h"
#include "MediaDecoderStateMachineScheduler.h"
#include "AudioSink.h"
#include "nsTArray.h"
#include "MediaDecoder.h"
#include "MediaDecoderReader.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"
#include "mozilla/dom/TimeRanges.h"
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

#include <algorithm>

namespace mozilla {

using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::gfx;


#undef DECODER_LOG
#undef VERBOSE_LOG

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(x, ...) \
  PR_LOG(gMediaDecoderLog, PR_LOG_DEBUG, ("Decoder=%p " x, mDecoder.get(), ##__VA_ARGS__))
#define VERBOSE_LOG(x, ...)                            \
    PR_BEGIN_MACRO                                     \
      if (!PR_GetEnv("MOZ_QUIET")) {                   \
        DECODER_LOG(x, ##__VA_ARGS__);                 \
      }                                                \
    PR_END_MACRO
#define SAMPLE_LOG(x, ...)                             \
    PR_BEGIN_MACRO                                     \
      if (PR_GetEnv("MEDIA_LOG_SAMPLES")) {            \
        DECODER_LOG(x, ##__VA_ARGS__);                 \
      }                                                \
    PR_END_MACRO
#else
#define DECODER_LOG(x, ...)
#define VERBOSE_LOG(x, ...)
#define SAMPLE_LOG(x, ...)
#endif



#define DECODER_WARN_HELPER(a, b) NS_WARNING b
#define DECODER_WARN(x, ...) \
  DECODER_WARN_HELPER(0, (nsPrintfCString("Decoder=%p " x, mDecoder.get(), ##__VA_ARGS__).get()))




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif




namespace detail {





static const uint32_t LOW_AUDIO_USECS = 300000;





const int64_t AMPLE_AUDIO_USECS = 1000000;

} 






const int64_t NO_VIDEO_AMPLE_AUDIO_DIVISOR = 8;




static const uint32_t LOW_VIDEO_FRAMES = 1;






static const int32_t LOW_VIDEO_THRESHOLD_USECS = 16000;


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





static const uint64_t ESTIMATED_DURATION_FUZZ_FACTOR_USECS = USECS_PER_S / 2;

static TimeDuration UsecsToDuration(int64_t aUsecs) {
  return TimeDuration::FromMicroseconds(aUsecs);
}

static int64_t DurationToUsecs(TimeDuration aDuration) {
  return static_cast<int64_t>(aDuration.ToSeconds() * USECS_PER_S);
}

static const uint32_t MIN_VIDEO_QUEUE_SIZE = 3;
static const uint32_t MAX_VIDEO_QUEUE_SIZE = 10;

static uint32_t sVideoQueueDefaultSize = MAX_VIDEO_QUEUE_SIZE;
static uint32_t sVideoQueueHWAccelSize = MIN_VIDEO_QUEUE_SIZE;

MediaDecoderStateMachine::MediaDecoderStateMachine(MediaDecoder* aDecoder,
                                                   MediaDecoderReader* aReader,
                                                   bool aRealTime) :
  mDecoder(aDecoder),
  mScheduler(new MediaDecoderStateMachineScheduler(
      aDecoder->GetReentrantMonitor(),
      &MediaDecoderStateMachine::TimeoutExpired, this, aRealTime)),
  mState(DECODER_STATE_DECODING_NONE),
  mPlayDuration(0),
  mStartTime(-1),
  mEndTime(-1),
  mDurationSet(false),
  mFragmentEndTime(-1),
  mReader(aReader),
  mCurrentFrameTime(0),
  mAudioStartTime(-1),
  mAudioEndTime(-1),
  mDecodedAudioEndTime(-1),
  mVideoFrameEndTime(-1),
  mDecodedVideoEndTime(-1),
  mVolume(1.0),
  mPlaybackRate(1.0),
  mPreservesPitch(true),
  mAmpleVideoFrames(MIN_VIDEO_QUEUE_SIZE),
  mLowAudioThresholdUsecs(detail::LOW_AUDIO_USECS),
  mAmpleAudioThresholdUsecs(detail::AMPLE_AUDIO_USECS),
  mQuickBufferingLowDataThresholdUsecs(detail::QUICK_BUFFERING_LOW_DATA_USECS),
  mIsAudioPrerolling(false),
  mIsVideoPrerolling(false),
  mAudioCaptured(false),
  mPositionChangeQueued(false),
  mAudioCompleted(false),
  mGotDurationFromMetaData(false),
  mDispatchedEventToDecode(false),
  mStopAudioThread(true),
  mQuickBuffering(false),
  mMinimizePreroll(false),
  mDecodeThreadWaiting(false),
  mDropAudioUntilNextDiscontinuity(false),
  mDropVideoUntilNextDiscontinuity(false),
  mDecodeToSeekTarget(false),
  mCurrentTimeBeforeSeek(0),
  mLastFrameStatus(MediaDecoderOwner::NEXT_FRAME_UNINITIALIZED),
  mDecodingFrozenAtStateDecoding(false),
  mSentLoadedMetadataEvent(false),
  mSentFirstFrameLoadedEvent(false)
{
  MOZ_COUNT_CTOR(MediaDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  static bool sPrefCacheInit = false;
  if (!sPrefCacheInit) {
    sPrefCacheInit = true;
    Preferences::AddUintVarCache(&sVideoQueueDefaultSize,
                                 "media.video-queue.default-size",
                                 MAX_VIDEO_QUEUE_SIZE);
    Preferences::AddUintVarCache(&sVideoQueueHWAccelSize,
                                 "media.video-queue.hw-accel-size",
                                 MIN_VIDEO_QUEUE_SIZE);
  }

  mBufferingWait = IsRealTime() ? 0 : 30;
  mLowDataThresholdUsecs = IsRealTime() ? 0 : detail::LOW_DATA_THRESHOLD_USECS;

#ifdef XP_WIN
  
  
  
  
  
  timeBeginPeriod(1);
#endif
}

MediaDecoderStateMachine::~MediaDecoderStateMachine()
{
  MOZ_ASSERT(NS_IsMainThread(), "Should be on main thread.");
  MOZ_COUNT_DTOR(MediaDecoderStateMachine);
  NS_ASSERTION(!mPendingWakeDecoder.get(),
               "WakeDecoder should have been revoked already");

  mReader = nullptr;

#ifdef XP_WIN
  timeEndPeriod(1);
#endif
}

bool MediaDecoderStateMachine::HasFutureAudio() {
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedUsecs() >
            mLowAudioThresholdUsecs * mPlaybackRate ||
          AudioQueue().IsFinished());
}

bool MediaDecoderStateMachine::HaveNextFrameData() {
  AssertCurrentThreadInMonitor();
  return (!HasAudio() || HasFutureAudio()) &&
         (!HasVideo() || VideoQueue().GetSize() > 0);
}

int64_t MediaDecoderStateMachine::GetDecodedAudioDuration() {
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();
  int64_t audioDecoded = AudioQueue().Duration();
  if (mAudioEndTime != -1) {
    audioDecoded += mAudioEndTime - GetMediaTime();
  }
  return audioDecoded;
}

void MediaDecoderStateMachine::SendStreamAudio(AudioData* aAudio,
                                               DecodedStreamData* aStream,
                                               AudioSegment* aOutput)
{
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();

  
  
  CheckedInt64 audioWrittenOffset = aStream->mAudioFramesWritten +
      UsecsToFrames(mInfo.mAudio.mRate, aStream->mInitialTime + mStartTime);
  CheckedInt64 frameOffset = UsecsToFrames(mInfo.mAudio.mRate, aAudio->mTime);

  if (!audioWrittenOffset.isValid() ||
      !frameOffset.isValid() ||
      
      frameOffset.value() + aAudio->mFrames <= audioWrittenOffset.value()) {
    return;
  }

  if (audioWrittenOffset.value() < frameOffset.value()) {
    int64_t silentFrames = frameOffset.value() - audioWrittenOffset.value();
    
    VERBOSE_LOG("writing %lld frames of silence to MediaStream", silentFrames);
    AudioSegment silence;
    silence.InsertNullDataAtStart(silentFrames);
    aStream->mAudioFramesWritten += silentFrames;
    audioWrittenOffset += silentFrames;
    aOutput->AppendFrom(&silence);
  }

  MOZ_ASSERT(audioWrittenOffset.value() >= frameOffset.value());

  int64_t offset = audioWrittenOffset.value() - frameOffset.value();
  size_t framesToWrite = aAudio->mFrames - offset;

  aAudio->EnsureAudioBuffer();
  nsRefPtr<SharedBuffer> buffer = aAudio->mAudioBuffer;
  AudioDataValue* bufferData = static_cast<AudioDataValue*>(buffer->Data());
  nsAutoTArray<const AudioDataValue*,2> channels;
  for (uint32_t i = 0; i < aAudio->mChannels; ++i) {
    channels.AppendElement(bufferData + i*aAudio->mFrames + offset);
  }
  aOutput->AppendFrames(buffer.forget(), channels, framesToWrite);
  VERBOSE_LOG("writing %u frames of data to MediaStream for AudioData at %lld",
              static_cast<unsigned>(framesToWrite),
              aAudio->mTime);
  aStream->mAudioFramesWritten += framesToWrite;
  aOutput->ApplyVolume(mVolume);

  aStream->mNextAudioTime = aAudio->GetEndTime();
}

static void WriteVideoToMediaStream(MediaStream* aStream,
                                    layers::Image* aImage,
                                    int64_t aEndMicroseconds,
                                    int64_t aStartMicroseconds,
                                    const IntSize& aIntrinsicSize,
                                    VideoSegment* aOutput)
{
  nsRefPtr<layers::Image> image = aImage;
  StreamTime duration =
      aStream->MicrosecondsToStreamTimeRoundDown(aEndMicroseconds) -
      aStream->MicrosecondsToStreamTimeRoundDown(aStartMicroseconds);
  aOutput->AppendFrame(image.forget(), duration, aIntrinsicSize);
}

void MediaDecoderStateMachine::SendStreamData()
{
  MOZ_ASSERT(OnStateMachineThread(), "Should be on state machine thread");
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(!mAudioSink, "Should've been stopped in CallRunStateMachine()");

  DecodedStreamData* stream = mDecoder->GetDecodedStream();

  bool finished =
      (!mInfo.HasAudio() || AudioQueue().IsFinished()) &&
      (!mInfo.HasVideo() || VideoQueue().IsFinished());
  if (mDecoder->IsSameOriginMedia()) {
    SourceMediaStream* mediaStream = stream->mStream;
    StreamTime endPosition = 0;

    if (!stream->mStreamInitialized) {
      if (mInfo.HasAudio()) {
        TrackID audioTrackId = mInfo.mAudio.mTrackInfo.mOutputId;
        AudioSegment* audio = new AudioSegment();
        mediaStream->AddAudioTrack(audioTrackId, mInfo.mAudio.mRate, 0, audio,
                                   SourceMediaStream::ADDTRACK_QUEUED);
        stream->mStream->DispatchWhenNotEnoughBuffered(audioTrackId,
            GetStateMachineThread(), GetWakeDecoderRunnable());
        stream->mNextAudioTime = mStartTime + stream->mInitialTime;
      }
      if (mInfo.HasVideo()) {
        TrackID videoTrackId = mInfo.mVideo.mTrackInfo.mOutputId;
        VideoSegment* video = new VideoSegment();
        mediaStream->AddTrack(videoTrackId, 0, video,
                              SourceMediaStream::ADDTRACK_QUEUED);
        stream->mStream->DispatchWhenNotEnoughBuffered(videoTrackId,
            GetStateMachineThread(), GetWakeDecoderRunnable());

        
        
        
        
        stream->mNextVideoTime = mStartTime + stream->mInitialTime;
      }
      mediaStream->FinishAddTracks();
      stream->mStreamInitialized = true;
    }

    if (mInfo.HasAudio()) {
      MOZ_ASSERT(stream->mNextAudioTime != -1, "Should've been initialized");
      TrackID audioTrackId = mInfo.mAudio.mTrackInfo.mOutputId;
      nsAutoTArray<nsRefPtr<AudioData>,10> audio;
      
      
      AudioQueue().GetElementsAfter(stream->mNextAudioTime, &audio);
      AudioSegment output;
      for (uint32_t i = 0; i < audio.Length(); ++i) {
        SendStreamAudio(audio[i], stream, &output);
      }
      
      
      
      if (output.GetDuration() > 0) {
        mediaStream->AppendToTrack(audioTrackId, &output);
      }
      if (AudioQueue().IsFinished() && !stream->mHaveSentFinishAudio) {
        mediaStream->EndTrack(audioTrackId);
        stream->mHaveSentFinishAudio = true;
      }
      endPosition = std::max(endPosition,
          mediaStream->TicksToTimeRoundDown(mInfo.mAudio.mRate,
                                            stream->mAudioFramesWritten));
    }

    if (mInfo.HasVideo()) {
      MOZ_ASSERT(stream->mNextVideoTime != -1, "Should've been initialized");
      TrackID videoTrackId = mInfo.mVideo.mTrackInfo.mOutputId;
      nsAutoTArray<nsRefPtr<VideoData>,10> video;
      
      
      VideoQueue().GetElementsAfter(stream->mNextVideoTime, &video);
      VideoSegment output;
      for (uint32_t i = 0; i < video.Length(); ++i) {
        VideoData* v = video[i];
        if (stream->mNextVideoTime < v->mTime) {
          VERBOSE_LOG("writing last video to MediaStream %p for %lldus",
                      mediaStream, v->mTime - stream->mNextVideoTime);
          
          

          
          
          
          
          
          
          WriteVideoToMediaStream(mediaStream, stream->mLastVideoImage,
            v->mTime, stream->mNextVideoTime, stream->mLastVideoImageDisplaySize,
              &output);
          stream->mNextVideoTime = v->mTime;
        }
        if (stream->mNextVideoTime < v->GetEndTime()) {
          VERBOSE_LOG("writing video frame %lldus to MediaStream %p for %lldus",
                      v->mTime, mediaStream, v->GetEndTime() - stream->mNextVideoTime);
          WriteVideoToMediaStream(mediaStream, v->mImage,
              v->GetEndTime(), stream->mNextVideoTime, v->mDisplay,
              &output);
          stream->mNextVideoTime = v->GetEndTime();
          stream->mLastVideoImage = v->mImage;
          stream->mLastVideoImageDisplaySize = v->mDisplay;
        } else {
          VERBOSE_LOG("skipping writing video frame %lldus (end %lldus) to MediaStream",
                      v->mTime, v->GetEndTime());
        }
      }
      if (output.GetDuration() > 0) {
        mediaStream->AppendToTrack(videoTrackId, &output);
      }
      if (VideoQueue().IsFinished() && !stream->mHaveSentFinishVideo) {
        mediaStream->EndTrack(videoTrackId);
        stream->mHaveSentFinishVideo = true;
      }
      endPosition = std::max(endPosition,
          mediaStream->MicrosecondsToStreamTimeRoundDown(
              stream->mNextVideoTime - stream->mInitialTime));
    }

    if (!stream->mHaveSentFinish) {
      stream->mStream->AdvanceKnownTracksTime(endPosition);
    }

    if (finished && !stream->mHaveSentFinish) {
      stream->mHaveSentFinish = true;
      stream->mStream->Finish();
    }
  }

  const auto clockTime = GetClock();
  while (true) {
    const AudioData* a = AudioQueue().PeekFront();
    
    
    
    
    if (a && a->mTime <= clockTime) {
      OnAudioEndTimeUpdate(std::max(mAudioEndTime, a->GetEndTime()));
      nsRefPtr<AudioData> releaseMe = AudioQueue().PopFront();
      continue;
    }
    break;
  }

  
  
  if (finished && AudioQueue().GetSize() == 0) {
    mAudioCompleted = true;
    UpdateReadyState();
  }
}

MediaDecoderStateMachine::WakeDecoderRunnable*
MediaDecoderStateMachine::GetWakeDecoderRunnable()
{
  AssertCurrentThreadInMonitor();

  if (!mPendingWakeDecoder.get()) {
    mPendingWakeDecoder = new WakeDecoderRunnable(this);
  }
  return mPendingWakeDecoder.get();
}

bool MediaDecoderStateMachine::HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs)
{
  AssertCurrentThreadInMonitor();

  if (AudioQueue().GetSize() == 0 ||
      GetDecodedAudioDuration() < aAmpleAudioUSecs) {
    return false;
  }
  if (!mAudioCaptured) {
    return true;
  }

  DecodedStreamData* stream = mDecoder->GetDecodedStream();

  if (stream && stream->mStreamInitialized && !stream->mHaveSentFinishAudio) {
    MOZ_ASSERT(mInfo.HasAudio());
    TrackID audioTrackId = mInfo.mAudio.mTrackInfo.mOutputId;
    if (!stream->mStream->HaveEnoughBuffered(audioTrackId)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(audioTrackId,
        GetStateMachineThread(), GetWakeDecoderRunnable());
  }

  return true;
}

bool MediaDecoderStateMachine::HaveEnoughDecodedVideo()
{
  AssertCurrentThreadInMonitor();

  if (static_cast<uint32_t>(VideoQueue().GetSize()) < mAmpleVideoFrames * mPlaybackRate) {
    return false;
  }

  DecodedStreamData* stream = mDecoder->GetDecodedStream();

  if (stream && stream->mStreamInitialized && !stream->mHaveSentFinishVideo) {
    MOZ_ASSERT(mInfo.HasVideo());
    TrackID videoTrackId = mInfo.mVideo.mTrackInfo.mOutputId;
    if (!stream->mStream->HaveEnoughBuffered(videoTrackId)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(videoTrackId,
        GetStateMachineThread(), GetWakeDecoderRunnable());
  }

  return true;
}

bool
MediaDecoderStateMachine::NeedToDecodeVideo()
{
  AssertCurrentThreadInMonitor();
  return IsVideoDecoding() &&
         ((mState == DECODER_STATE_SEEKING && mDecodeToSeekTarget) ||
          (mState == DECODER_STATE_DECODING_FIRSTFRAME &&
           IsVideoDecoding() && VideoQueue().GetSize() == 0) ||
          (!mMinimizePreroll && !HaveEnoughDecodedVideo()));
}

bool
MediaDecoderStateMachine::NeedToSkipToNextKeyframe()
{
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
                             (mDecodedVideoEndTime - GetClock() <
                              LOW_VIDEO_THRESHOLD_USECS * mPlaybackRate);
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
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("IsAudioSeekComplete() curTarVal=%d mAudDis=%d aqFin=%d aqSz=%d",
    mCurrentSeekTarget.IsValid(), mDropAudioUntilNextDiscontinuity, AudioQueue().IsFinished(), AudioQueue().GetSize());
  return
    !HasAudio() ||
    (mCurrentSeekTarget.IsValid() &&
     !mDropAudioUntilNextDiscontinuity &&
     (AudioQueue().IsFinished() || AudioQueue().GetSize() > 0));
}

bool
MediaDecoderStateMachine::IsVideoSeekComplete()
{
  AssertCurrentThreadInMonitor();
  SAMPLE_LOG("IsVideoSeekComplete() curTarVal=%d mVidDis=%d vqFin=%d vqSz=%d",
    mCurrentSeekTarget.IsValid(), mDropVideoUntilNextDiscontinuity, VideoQueue().IsFinished(), VideoQueue().GetSize());
  return
    !HasVideo() ||
    (mCurrentSeekTarget.IsValid() &&
     !mDropVideoUntilNextDiscontinuity &&
     (VideoQueue().IsFinished() || VideoQueue().GetSize() > 0));
}

void
MediaDecoderStateMachine::OnAudioDecoded(AudioData* aAudioSample)
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<AudioData> audio(aAudioSample);
  MOZ_ASSERT(audio);
  mAudioDataRequest.Complete();
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
      if (!mCurrentSeekTarget.IsValid()) {
        
        return;
      }
      if (audio->mDiscontinuity) {
        mDropAudioUntilNextDiscontinuity = false;
      }
      if (!mDropAudioUntilNextDiscontinuity) {
        
        
        if (mCurrentSeekTarget.mType == SeekTarget::PrevSyncPoint &&
            mCurrentSeekTarget.mTime > mCurrentTimeBeforeSeek &&
            audio->mTime < mCurrentTimeBeforeSeek) {
          
          
          
          
          
          
          mCurrentSeekTarget.mType = SeekTarget::Accurate;
        }
        if (mCurrentSeekTarget.mType == SeekTarget::PrevSyncPoint) {
          
          AudioQueue().Push(audio);
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
  MOZ_ASSERT(aSample);
  
  
  
  AudioQueue().Push(aSample);
  if (mState > DECODER_STATE_DECODING_FIRSTFRAME) {
    
    
    UpdateReadyState();
    DispatchDecodeTasksIfNeeded();
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void
MediaDecoderStateMachine::Push(VideoData* aSample)
{
  MOZ_ASSERT(aSample);
  
  
  
  VideoQueue().Push(aSample);
  if (mState > DECODER_STATE_DECODING_FIRSTFRAME) {
    
    
    UpdateReadyState();
    DispatchDecodeTasksIfNeeded();
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void
MediaDecoderStateMachine::OnNotDecoded(MediaData::Type aType,
                                       MediaDecoderReader::NotDecodedReason aReason)
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  SAMPLE_LOG("OnNotDecoded (aType=%u, aReason=%u)", aType, aReason);
  bool isAudio = aType == MediaData::AUDIO_DATA;
  MOZ_ASSERT_IF(!isAudio, aType == MediaData::VIDEO_DATA);

  if (isAudio) {
    mAudioDataRequest.Complete();
  } else {
    mVideoDataRequest.Complete();
  }

  
  if (aReason == MediaDecoderReader::DECODE_ERROR) {
    DecodeError();
    return;
  }

  
  
  if (aReason == MediaDecoderReader::WAITING_FOR_DATA) {
    MOZ_ASSERT(mReader->IsWaitForDataSupported(),
               "Readers that send WAITING_FOR_DATA need to implement WaitForData");
    WaitRequestRef(aType).Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                               &MediaDecoderReader::WaitForData, aType)
      ->RefableThen(mScheduler.get(), __func__, this,
                    &MediaDecoderStateMachine::OnWaitForDataResolved,
                    &MediaDecoderStateMachine::OnWaitForDataRejected));
    return;
  }

  if (aReason == MediaDecoderReader::CANCELED) {
    DispatchDecodeTasksIfNeeded();
    return;
  }

  
  
  MOZ_ASSERT(aReason == MediaDecoderReader::END_OF_STREAM);
  if (!isAudio && mState == DECODER_STATE_SEEKING &&
      mCurrentSeekTarget.IsValid() && mFirstVideoFrameAfterSeek) {
    
    
    
    
    VideoQueue().Push(mFirstVideoFrameAfterSeek);
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
      
      
      UpdateReadyState();
      mDecoder->GetReentrantMonitor().NotifyAll();
      
      if (mAudioCaptured) {
        ScheduleStateMachine();
      }
      return;
    }
    case DECODER_STATE_SEEKING: {
      if (!mCurrentSeekTarget.IsValid()) {
        
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
MediaDecoderStateMachine::AcquireMonitorAndInvokeDecodeError()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DecodeError();
}

void
MediaDecoderStateMachine::MaybeFinishDecodeFirstFrame()
{
  MOZ_ASSERT(OnStateMachineThread());
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
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<VideoData> video(aVideoSample);
  mVideoDataRequest.Complete();
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

      
      if (mAudioCaptured) {
        ScheduleStateMachine();
      }
      return;
    }
    case DECODER_STATE_SEEKING: {
      if (!mCurrentSeekTarget.IsValid()) {
        
        return;
      }
      if (mDropVideoUntilNextDiscontinuity) {
        if (video->mDiscontinuity) {
          mDropVideoUntilNextDiscontinuity = false;
        }
      }
      if (!mDropVideoUntilNextDiscontinuity) {
        
        
        if (mCurrentSeekTarget.mType == SeekTarget::PrevSyncPoint &&
            mCurrentSeekTarget.mTime > mCurrentTimeBeforeSeek &&
            video->mTime < mCurrentTimeBeforeSeek) {
          
          
          
          
          
          
          mCurrentSeekTarget.mType = SeekTarget::Accurate;
        }
        if (mCurrentSeekTarget.mType == SeekTarget::PrevSyncPoint) {
          
          VideoQueue().Push(video);
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
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();

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
    RefPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::SeekCompleted));
    nsresult rv = GetStateMachineThread()->Dispatch(task, NS_DISPATCH_NORMAL);
    if (NS_FAILED(rv)) {
      DecodeError();
    }
  }
}

bool
MediaDecoderStateMachine::IsAudioDecoding()
{
  AssertCurrentThreadInMonitor();
  return HasAudio() && !AudioQueue().IsFinished();
}

bool
MediaDecoderStateMachine::IsVideoDecoding()
{
  AssertCurrentThreadInMonitor();
  return HasVideo() && !VideoQueue().IsFinished();
}

void
MediaDecoderStateMachine::CheckIfDecodeComplete()
{
  AssertCurrentThreadInMonitor();
  if (mState == DECODER_STATE_SHUTDOWN ||
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

  if (NS_WARN_IF(!mReader->EnsureTaskQueue())) {
    return NS_ERROR_FAILURE;
  }

  MediaDecoderReader* cloneReader = nullptr;
  if (aCloneDonor) {
    cloneReader = aCloneDonor->mReader;
  }

  nsresult rv = mScheduler->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mReader->Init(cloneReader);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void MediaDecoderStateMachine::StopPlayback()
{
  
  
  
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());
  DECODER_LOG("StopPlayback()");

  AssertCurrentThreadInMonitor();

  mDecoder->NotifyPlaybackStopped();

  if (IsPlaying()) {
    mPlayDuration = GetClock() - mStartTime;
    SetPlayStartTime(TimeStamp());
  }
  
  
  mDecoder->GetReentrantMonitor().NotifyAll();
  NS_ASSERTION(!IsPlaying(), "Should report not playing at end of StopPlayback()");
  mDecoder->UpdateStreamBlockingForStateMachinePlaying();

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AcquireMonitorAndInvokeDispatchDecodeTasksIfNeeded);
  GetStateMachineThread()->Dispatch(event, NS_DISPATCH_NORMAL);
}

void MediaDecoderStateMachine::MaybeStartPlayback()
{
  AssertCurrentThreadInMonitor();
  if (IsPlaying()) {
    
    return;
  }

  bool playStatePermits = mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING;
  bool decodeStatePermits = mState == DECODER_STATE_DECODING || mState == DECODER_STATE_COMPLETED;
  if (!playStatePermits || !decodeStatePermits || mIsAudioPrerolling || mIsVideoPrerolling) {
    DECODER_LOG("Not starting playback [playStatePermits: %d, decodeStatePermits: %d, "
                "mIsAudioPrerolling: %d, mIsVideoPrerolling: %d]", (int) playStatePermits,
                (int) decodeStatePermits, (int) mIsAudioPrerolling, (int) mIsVideoPrerolling);
    return;
  }

  if (mDecoder->CheckDecoderCanOffloadAudio()) {
    DECODER_LOG("Offloading playback");
    return;
  }

  DECODER_LOG("MaybeStartPlayback() starting playback");

  mDecoder->NotifyPlaybackStarted();
  SetPlayStartTime(TimeStamp::Now());
  MOZ_ASSERT(IsPlaying());

  nsresult rv = StartAudioThread();
  NS_ENSURE_SUCCESS_VOID(rv);

  mDecoder->GetReentrantMonitor().NotifyAll();
  mDecoder->UpdateStreamBlockingForStateMachinePlaying();
  DispatchDecodeTasksIfNeeded();
}

void MediaDecoderStateMachine::UpdatePlaybackPositionInternal(int64_t aTime)
{
  
  
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());
  SAMPLE_LOG("UpdatePlaybackPositionInternal(%lld) (mStartTime=%lld)", aTime, mStartTime);
  AssertCurrentThreadInMonitor();

  NS_ASSERTION(mStartTime >= 0, "Should have positive mStartTime");
  mCurrentFrameTime = aTime - mStartTime;
  NS_ASSERTION(mCurrentFrameTime >= 0, "CurrentTime should be positive!");
  if (aTime > mEndTime) {
    NS_ASSERTION(mCurrentFrameTime > GetDuration(),
                 "CurrentTime must be after duration if aTime > endTime!");
    DECODER_LOG("Setting new end time to %lld", aTime);
    mEndTime = aTime;
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DurationChanged);
    NS_DispatchToMainThread(event);
  }
}

void MediaDecoderStateMachine::UpdatePlaybackPosition(int64_t aTime)
{
  UpdatePlaybackPositionInternal(aTime);

  bool fragmentEnded = mFragmentEndTime >= 0 && GetMediaTime() >= mFragmentEndTime;
  if (!mPositionChangeQueued || fragmentEnded) {
    mPositionChangeQueued = true;
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
        mDecoder,
        &MediaDecoder::PlaybackPositionChanged,
        MediaDecoderEventVisibility::Observable);
    NS_DispatchToMainThread(event);
  }

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

MediaDecoderOwner::NextFrameStatus MediaDecoderStateMachine::GetNextFrameStatus()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (IsBuffering()) {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_BUFFERING;
  } else if (IsSeeking()) {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_SEEKING;
  } else if (HaveNextFrameData()) {
    return MediaDecoderOwner::NEXT_FRAME_AVAILABLE;
  }
  return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
}

static const char* const gMachineStateStr[] = {
  "NONE",
  "DECODING_METADATA",
  "WAIT_FOR_RESOURCES",
  "DECODING_FIRSTFRAME",
  "DORMANT",
  "DECODING",
  "SEEKING",
  "BUFFERING",
  "COMPLETED",
  "SHUTDOWN"
};

void MediaDecoderStateMachine::SetState(State aState)
{
  AssertCurrentThreadInMonitor();
  if (mState == aState) {
    return;
  }
  DECODER_LOG("Change machine state from %s to %s",
              gMachineStateStr[mState], gMachineStateStr[aState]);

  mState = aState;
}

void MediaDecoderStateMachine::SetVolume(double volume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mVolume = volume;
  if (mAudioSink) {
    mAudioSink->SetVolume(mVolume);
  }
}

void MediaDecoderStateMachine::SetAudioCaptured()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  AssertCurrentThreadInMonitor();
  if (!mAudioCaptured) {
    mAudioCaptured = true;
    
    ScheduleStateMachine();
  }
}

double MediaDecoderStateMachine::GetCurrentTime() const
{
  NS_ASSERTION(NS_IsMainThread() ||
               OnStateMachineThread() ||
               OnDecodeThread(),
               "Should be on main, decode, or state machine thread.");

  return static_cast<double>(mCurrentFrameTime) / static_cast<double>(USECS_PER_S);
}

int64_t MediaDecoderStateMachine::GetCurrentTimeUs() const
{
  NS_ASSERTION(NS_IsMainThread() ||
               OnStateMachineThread() ||
               OnDecodeThread(),
               "Should be on main, decode, or state machine thread.");

  return mCurrentFrameTime;
}

bool MediaDecoderStateMachine::IsRealTime() const {
  return mScheduler->IsRealTime();
}

int64_t MediaDecoderStateMachine::GetDuration()
{
  AssertCurrentThreadInMonitor();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

int64_t MediaDecoderStateMachine::GetEndTime()
{
  if (mEndTime == -1 && mDurationSet) {
    return INT64_MAX;
  }
  return mEndTime;
}



class SeekRunnable : public nsRunnable {
public:
  SeekRunnable(MediaDecoder* aDecoder, double aSeekTarget)
    : mDecoder(aDecoder), mSeekTarget(aSeekTarget) {}
  NS_IMETHOD Run() {
    mDecoder->Seek(mSeekTarget, SeekTarget::Accurate);
    return NS_OK;
  }
private:
  nsRefPtr<MediaDecoder> mDecoder;
  double mSeekTarget;
};

void MediaDecoderStateMachine::SetDuration(int64_t aDuration)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
               "Should be on main or decode thread.");
  AssertCurrentThreadInMonitor();

  if (aDuration < 0) {
    mDurationSet = false;
    return;
  }

  mDurationSet = true;

  if (mStartTime == -1) {
    SetStartTime(0);
  }

  if (aDuration == INT64_MAX) {
    mEndTime = -1;
    return;
  }

  mEndTime = mStartTime + aDuration;

  if (mDecoder && mEndTime >= 0 && mEndTime < mCurrentFrameTime) {
    
    
    
    if (NS_IsMainThread()) {
      
      mDecoder->Seek(double(mEndTime) / USECS_PER_S, SeekTarget::Accurate);
    } else {
      
      nsCOMPtr<nsIRunnable> task =
        new SeekRunnable(mDecoder, double(mEndTime) / USECS_PER_S);
      NS_DispatchToMainThread(task);
    }
  }
}

void MediaDecoderStateMachine::UpdateEstimatedDuration(int64_t aDuration)
{
  AssertCurrentThreadInMonitor();
  int64_t duration = GetDuration();
  if (aDuration != duration &&
      mozilla::Abs(aDuration - duration) > ESTIMATED_DURATION_FUZZ_FACTOR_USECS) {
    SetDuration(aDuration);
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DurationChanged);
    NS_DispatchToMainThread(event);
  }
}

void MediaDecoderStateMachine::SetMediaEndTime(int64_t aEndTime)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread");
  AssertCurrentThreadInMonitor();

  mEndTime = aEndTime;
}

void MediaDecoderStateMachine::SetFragmentEndTime(int64_t aEndTime)
{
  AssertCurrentThreadInMonitor();

  mFragmentEndTime = aEndTime < 0 ? aEndTime : aEndTime + mStartTime;
}

bool MediaDecoderStateMachine::IsDormantNeeded()
{
  return mReader->IsDormantNeeded();
}

void MediaDecoderStateMachine::SetDormant(bool aDormant)
{
  MOZ_ASSERT(OnStateMachineThread(), "Should be on state machine thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mState == DECODER_STATE_SHUTDOWN) {
    return;
  }

  if (!mReader) {
    return;
  }

  DECODER_LOG("SetDormant=%d", aDormant);

  if (aDormant) {
    if (mState == DECODER_STATE_SEEKING) {
      if (mQueuedSeekTarget.IsValid()) {
        
      } else if (mSeekTarget.IsValid()) {
        mQueuedSeekTarget = mSeekTarget;
      } else if (mCurrentSeekTarget.IsValid()) {
        mQueuedSeekTarget = mCurrentSeekTarget;
      } else {
        mQueuedSeekTarget = SeekTarget(mCurrentFrameTime,
                                       SeekTarget::Accurate,
                                       MediaDecoderEventVisibility::Suppressed);
      }
    } else {
      mQueuedSeekTarget = SeekTarget(mCurrentFrameTime,
                                     SeekTarget::Accurate,
                                     MediaDecoderEventVisibility::Suppressed);
    }
    mSeekTarget.Reset();
    mCurrentSeekTarget.Reset();
    ScheduleStateMachine();
    SetState(DECODER_STATE_DORMANT);
    mDecoder->GetReentrantMonitor().NotifyAll();
  } else if ((aDormant != true) && (mState == DECODER_STATE_DORMANT)) {
    mDecodingFrozenAtStateDecoding = true;
    ScheduleStateMachine();
    mCurrentFrameTime = 0;
    SetState(DECODER_STATE_DECODING_NONE);
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void MediaDecoderStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  DECODER_LOG("Changed state to SHUTDOWN");
  SetState(DECODER_STATE_SHUTDOWN);
  mScheduler->ScheduleAndShutdown();
  if (mAudioSink) {
    mAudioSink->PrepareToShutdown();
  }
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void MediaDecoderStateMachine::StartDecoding()
{
  MOZ_ASSERT(OnStateMachineThread());
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

void MediaDecoderStateMachine::StartWaitForResources()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnDecodeThread(),
               "Should be on decode thread.");
  SetState(DECODER_STATE_WAIT_FOR_RESOURCES);
  DECODER_LOG("StartWaitForResources");
}

void MediaDecoderStateMachine::NotifyWaitingForResourcesStatusChanged()
{
  AssertCurrentThreadInMonitor();
  DECODER_LOG("NotifyWaitingForResourcesStatusChanged");
  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this,
      &MediaDecoderStateMachine::DoNotifyWaitingForResourcesStatusChanged));
  DecodeTaskQueue()->Dispatch(task);
}

void MediaDecoderStateMachine::DoNotifyWaitingForResourcesStatusChanged()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState != DECODER_STATE_WAIT_FOR_RESOURCES) {
    return;
  }
  DECODER_LOG("DoNotifyWaitingForResourcesStatusChanged");
  
  
  SetState(DECODER_STATE_DECODING_NONE);
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::PlayInternal()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
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

void MediaDecoderStateMachine::ResetPlayback()
{
  
  
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());

  
  
  
  
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_SEEKING ||
             mState == DECODER_STATE_SHUTDOWN ||
             mState == DECODER_STATE_DORMANT ||
             mState == DECODER_STATE_DECODING_NONE);

  
  
  
  MOZ_ASSERT(!mAudioSink);

  mVideoFrameEndTime = -1;
  mDecodedVideoEndTime = -1;
  mAudioStartTime = -1;
  mAudioEndTime = -1;
  mDecodedAudioEndTime = -1;
  mAudioCompleted = false;
  AudioQueue().Reset();
  VideoQueue().Reset();
  mFirstVideoFrameAfterSeek = nullptr;
  mDropAudioUntilNextDiscontinuity = true;
  mDropVideoUntilNextDiscontinuity = true;
}

void MediaDecoderStateMachine::NotifyDataArrived(const char* aBuffer,
                                                     uint32_t aLength,
                                                     int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

  
  
  
  
  
  
  
  
  nsRefPtr<dom::TimeRanges> buffered = new dom::TimeRanges();
  if (mDecoder->IsInfinite() && (mStartTime != -1) &&
      NS_SUCCEEDED(mDecoder->GetBuffered(buffered)))
  {
    uint32_t length = 0;
    buffered->GetLength(&length);
    if (length) {
      double end = 0;
      buffered->End(length - 1, &end);
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mEndTime = std::max<int64_t>(mEndTime, end * USECS_PER_S);
    }
  }
}

void MediaDecoderStateMachine::Seek(const SeekTarget& aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mDecodingFrozenAtStateDecoding = false;

  if (mState == DECODER_STATE_SHUTDOWN) {
    return;
  }

  
  
  if (!mDecoder->IsMediaSeekable()) {
    DECODER_WARN("Seek() function should not be called on a non-seekable state machine");
    return;
  }

  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "We should have got duration already");

  if (mState < DECODER_STATE_DECODING) {
    DECODER_LOG("Seek() Not Enough Data to continue at this stage, queuing seek");
    mQueuedSeekTarget = aTarget;
    return;
  }
  mQueuedSeekTarget.Reset();

  
  int64_t end = GetEndTime();
  NS_ASSERTION(mStartTime != -1, "Should know start time by now");
  NS_ASSERTION(end != -1, "Should know end time by now");
  int64_t seekTime = aTarget.mTime + mStartTime;
  seekTime = std::min(seekTime, end);
  seekTime = std::max(mStartTime, seekTime);
  NS_ASSERTION(seekTime >= mStartTime && seekTime <= end,
               "Can only seek in range [0,duration]");
  mSeekTarget = SeekTarget(seekTime, aTarget.mType, aTarget.mEventVisibility);

  DECODER_LOG("Changed state to SEEKING (to %lld)", mSeekTarget.mTime);
  SetState(DECODER_STATE_SEEKING);

  
  
  
  
  if (mAudioCaptured) {
    mDecoder->RecreateDecodedStream(seekTime - mStartTime);
  }

  ScheduleStateMachine();
}

void
MediaDecoderStateMachine::EnqueueStartQueuedSeekTask()
{
  MOZ_ASSERT(mQueuedSeekTarget.IsValid());
  MOZ_ASSERT(mState >= DECODER_STATE_DECODING, "MDSM::Seek will requeue this seek!");
  DECODER_LOG("Applying queued seek to %lld\n", mQueuedSeekTarget.mTime);
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethodWithArg<SeekTarget>(this, &MediaDecoderStateMachine::Seek, mQueuedSeekTarget);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void MediaDecoderStateMachine::StopAudioThread()
{
  
  
  MOZ_ASSERT(OnStateMachineThread() || OnDecodeThread());
  AssertCurrentThreadInMonitor();

  if (mStopAudioThread) {
    
    while (mAudioSink) {
      mDecoder->GetReentrantMonitor().Wait();
    }
    return;
  }

  mStopAudioThread = true;
  
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mAudioSink) {
    DECODER_LOG("Shutdown audio thread");
    mAudioSink->PrepareToShutdown();
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mAudioSink->Shutdown();
    }
    mAudioSink = nullptr;
  }
  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

nsresult
MediaDecoderStateMachine::EnqueueDecodeMetadataTask()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_METADATA);

  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::CallDecodeMetadata));
  nsresult rv = DecodeTaskQueue()->Dispatch(task);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnqueueDecodeFirstFrameTask()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_FIRSTFRAME);

  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::CallDecodeFirstFrame));
  nsresult rv = GetStateMachineThread()->Dispatch(task, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

void
MediaDecoderStateMachine::SetReaderIdle()
{
  MOZ_ASSERT(OnDecodeThread());
  DECODER_LOG("Invoking SetReaderIdle()");
  mReader->SetIdle();
}

void
MediaDecoderStateMachine::AcquireMonitorAndInvokeDispatchDecodeTasksIfNeeded()
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DispatchDecodeTasksIfNeeded();
}

void
MediaDecoderStateMachine::DispatchDecodeTasksIfNeeded()
{
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

  bool needIdle = !mDecoder->IsLogicallyPlaying() &&
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
    DECODER_LOG("Dispatching SetReaderIdle() audioQueue=%lld videoQueue=%lld",
                GetDecodedAudioDuration(),
                VideoQueue().Duration());
    RefPtr<nsIRunnable> event = NS_NewRunnableMethod(
        this, &MediaDecoderStateMachine::SetReaderIdle);
    nsresult rv = DecodeTaskQueue()->Dispatch(event.forget());
    if (NS_FAILED(rv) && mState != DECODER_STATE_SHUTDOWN) {
      DECODER_WARN("Failed to dispatch event to set decoder idle state");
    }
  }
}

void
MediaDecoderStateMachine::InitiateSeek()
{
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();

  mCurrentSeekTarget = mSeekTarget;
  mSeekTarget.Reset();
  mDropAudioUntilNextDiscontinuity = HasAudio();
  mDropVideoUntilNextDiscontinuity = HasVideo();

  mDecoder->StopProgressUpdates();
  mCurrentTimeBeforeSeek = GetMediaTime();

  
  
  
  StopPlayback();
  UpdatePlaybackPositionInternal(mCurrentSeekTarget.mTime);

  
  
  
  nsCOMPtr<nsIRunnable> startEvent =
      NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
        mDecoder,
        &MediaDecoder::SeekingStarted,
        mCurrentSeekTarget.mEventVisibility);
  NS_DispatchToMainThread(startEvent, NS_DISPATCH_NORMAL);

  
  
  
  StopAudioThread();
  ResetPlayback();

  
  ResetDecode();

  
  mSeekRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                    &MediaDecoderReader::Seek, mCurrentSeekTarget.mTime,
                                    GetEndTime())
    ->RefableThen(mScheduler.get(), __func__, this,
                  &MediaDecoderStateMachine::OnSeekCompleted,
                  &MediaDecoderStateMachine::OnSeekFailed));
}

nsresult
MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded()
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (NeedToDecodeAudio()) {
    return EnsureAudioDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureAudioDecodeTaskQueued()
{
  MOZ_ASSERT(OnStateMachineThread());
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
    ->RefableThen(mScheduler.get(), __func__, this,
                  &MediaDecoderStateMachine::OnAudioDecoded,
                  &MediaDecoderStateMachine::OnAudioNotDecoded));

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded()
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (NeedToDecodeVideo()) {
    return EnsureVideoDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureVideoDecodeTaskQueued()
{
  MOZ_ASSERT(OnStateMachineThread());
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

  
  
  
  mVideoDecodeStartTime = TimeStamp::Now();

  SAMPLE_LOG("Queueing video task - queued=%i, decoder-queued=%o, skip=%i, time=%lld",
             VideoQueue().GetSize(), mReader->SizeOfVideoQueueInFrames(), skipToNextKeyFrame,
             currentTime);

  mVideoDataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(), __func__,
                                         &MediaDecoderReader::RequestVideoData,
                                         skipToNextKeyFrame, currentTime)
    ->RefableThen(mScheduler.get(), __func__, this,
                  &MediaDecoderStateMachine::OnVideoDecoded,
                  &MediaDecoderStateMachine::OnVideoNotDecoded));
  return NS_OK;
}

nsresult
MediaDecoderStateMachine::StartAudioThread()
{
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();
  if (mAudioCaptured) {
    NS_ASSERTION(mStopAudioThread, "mStopAudioThread must always be true if audio is captured");
    return NS_OK;
  }

  mStopAudioThread = false;
  if (HasAudio() && !mAudioSink) {
    
    mAudioEndTime = mAudioStartTime;
    MOZ_ASSERT(mAudioStartTime == GetMediaTime());
    mAudioCompleted = false;
    mAudioSink = new AudioSink(this, mAudioStartTime,
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
  NS_ASSERTION(HasAudio(),
               "Should only call AudioDecodedUsecs() when we have audio");
  
  
  
  int64_t pushed = (mAudioEndTime != -1) ? (mAudioEndTime - GetMediaTime()) : 0;

  
  
  if (IsRealTime()) {
    return pushed + FramesToUsecs(AudioQueue().FrameCount(), mInfo.mAudio.mRate).value();
  }
  return pushed + AudioQueue().Duration();
}

bool MediaDecoderStateMachine::HasLowDecodedData(int64_t aAudioUsecs)
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mReader->UseBufferingHeuristics());
  
  
  
  
  return ((IsAudioDecoding() && AudioDecodedUsecs() < aAudioUsecs) ||
         (IsVideoDecoding() &&
          static_cast<uint32_t>(VideoQueue().GetSize()) < LOW_VIDEO_FRAMES));
}

bool MediaDecoderStateMachine::OutOfDecodedAudio()
{
    return IsAudioDecoding() && !AudioQueue().IsFinished() &&
           AudioQueue().GetSize() == 0 &&
           (!mAudioSink || !mAudioSink->HasUnplayedFrames());
}

bool MediaDecoderStateMachine::HasLowUndecodedData()
{
  return HasLowUndecodedData(mLowDataThresholdUsecs);
}

bool MediaDecoderStateMachine::HasLowUndecodedData(int64_t aUsecs)
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mState > DECODER_STATE_DECODING_FIRSTFRAME,
               "Must have loaded first frame for GetBuffered() to work");

  
  
  
  if (GetDuration() < 0) {
    return false;
  }

  nsRefPtr<dom::TimeRanges> buffered = new dom::TimeRanges();
  nsresult rv = mReader->GetBuffered(buffered.get());
  NS_ENSURE_SUCCESS(rv, false);

  int64_t endOfDecodedVideoData = INT64_MAX;
  if (HasVideo() && !VideoQueue().AtEndOfStream()) {
    endOfDecodedVideoData = VideoQueue().Peek() ? VideoQueue().Peek()->GetEndTime() : mVideoFrameEndTime;
  }
  int64_t endOfDecodedAudioData = INT64_MAX;
  if (HasAudio() && !AudioQueue().AtEndOfStream()) {
    
    
    
    endOfDecodedAudioData = mDecodedAudioEndTime;
  }
  int64_t endOfDecodedData = std::min(endOfDecodedVideoData, endOfDecodedAudioData);

  return endOfDecodedData != INT64_MAX &&
         !buffered->Contains(static_cast<double>(endOfDecodedData) / USECS_PER_S,
                             static_cast<double>(std::min(endOfDecodedData + aUsecs, GetDuration())) / USECS_PER_S);
}

void
MediaDecoderStateMachine::DecodeError()
{
  AssertCurrentThreadInMonitor();
  if (mState == DECODER_STATE_SHUTDOWN) {
    
    return;
  }

  
  
  if (!OnDecodeThread()) {
    RefPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AcquireMonitorAndInvokeDecodeError));
    nsresult rv = DecodeTaskQueue()->Dispatch(task);

    if (NS_FAILED(rv)) {
      DECODER_WARN("Failed to dispatch AcquireMonitorAndInvokeDecodeError");
    }
    return;
  }

  
  
  
  DECODER_WARN("Decode error, changed state to SHUTDOWN due to error");
  SetState(DECODER_STATE_SHUTDOWN);
  mScheduler->ScheduleAndShutdown();
  mDecoder->GetReentrantMonitor().NotifyAll();

  
  
  
  
  
  
 {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
  }
}

void
MediaDecoderStateMachine::CallDecodeMetadata()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState != DECODER_STATE_DECODING_METADATA) {
    return;
  }
  if (NS_FAILED(DecodeMetadata())) {
    DECODER_WARN("Decode metadata failed, shutting down decoder");
    DecodeError();
  }
}

nsresult MediaDecoderStateMachine::DecodeMetadata()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_METADATA);
  DECODER_LOG("Decoding Media Headers");

  nsresult res;
  MediaInfo info;
  bool isAwaitingResources = false;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    mReader->PreReadMetadata();

    if (mReader->IsWaitingMediaResources()) {
      StartWaitForResources();
      return NS_OK;
    }
    res = mReader->ReadMetadata(&info, getter_Transfers(mMetadataTags));
    isAwaitingResources = mReader->IsWaitingMediaResources();
  }

  if (NS_SUCCEEDED(res) &&
      mState == DECODER_STATE_DECODING_METADATA &&
      isAwaitingResources) {
    
    StartWaitForResources();
    
    return NS_OK;
  }

  if (NS_FAILED(res) || (!info.HasValidMedia())) {
    DECODER_WARN("ReadMetadata failed, res=%x HasValidMedia=%d", res, info.HasValidMedia());
    return NS_ERROR_FAILURE;
  }

  if (NS_SUCCEEDED(res)) {
    mDecoder->SetMediaSeekable(mReader->IsMediaSeekable());
  }

  mInfo = info;

  if (HasVideo()) {
    mAmpleVideoFrames = (mReader->IsAsync() && mInfo.mVideo.mIsHardwareAccelerated)
      ? std::max<uint32_t>(sVideoQueueHWAccelSize, MIN_VIDEO_QUEUE_SIZE)
      : std::max<uint32_t>(sVideoQueueDefaultSize, MIN_VIDEO_QUEUE_SIZE);
    DECODER_LOG("Video decode isAsync=%d HWAccel=%d videoQueueSize=%d",
                mReader->IsAsync(),
                mInfo.mVideo.mIsHardwareAccelerated,
                mAmpleVideoFrames);
  }

  mDecoder->StartProgressUpdates();
  mGotDurationFromMetaData = (GetDuration() != -1) || mDurationSet;

  if (mGotDurationFromMetaData) {
    
    
    EnqueueLoadedMetadataEvent();
  }

  if (mState == DECODER_STATE_DECODING_METADATA) {
    SetState(DECODER_STATE_DECODING_FIRSTFRAME);
    res = EnqueueDecodeFirstFrameTask();
    if (NS_FAILED(res)) {
      return NS_ERROR_FAILURE;
    }
  }
  ScheduleStateMachine();

  return NS_OK;
}

void
MediaDecoderStateMachine::EnqueueLoadedMetadataEvent()
{
  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  MediaDecoderEventVisibility visibility = mSentLoadedMetadataEvent?
                                    MediaDecoderEventVisibility::Suppressed :
                                    MediaDecoderEventVisibility::Observable;
  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new MetadataEventRunner(mDecoder, info, mMetadataTags, visibility);
  NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);
  mSentLoadedMetadataEvent = true;
}

void
MediaDecoderStateMachine::EnqueueFirstFrameLoadedEvent()
{
  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  MediaDecoderEventVisibility visibility = mSentFirstFrameLoadedEvent?
                                    MediaDecoderEventVisibility::Suppressed :
                                    MediaDecoderEventVisibility::Observable;
  nsCOMPtr<nsIRunnable> event =
    new FirstFrameLoadedEventRunner(mDecoder, info, visibility);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  mSentFirstFrameLoadedEvent = true;
}

void
MediaDecoderStateMachine::CallDecodeFirstFrame()
{
  MOZ_ASSERT(OnStateMachineThread());
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
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_FIRSTFRAME);
  DECODER_LOG("DecodeFirstFrame started");

  if (HasAudio()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded));
    AudioQueue().AddPopListener(decodeTask, GetStateMachineThread());
  }
  if (HasVideo()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded));
    VideoQueue().AddPopListener(decodeTask, GetStateMachineThread());
  }

  if (IsRealTime()) {
    SetStartTime(0);
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else if (mSentFirstFrameLoadedEvent) {
    
    
    
    SetStartTime(mStartTime);
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else {
    if (HasAudio()) {
      mAudioDataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(),
                                             __func__, &MediaDecoderReader::RequestAudioData)
        ->RefableThen(mScheduler.get(), __func__, this,
                      &MediaDecoderStateMachine::OnAudioDecoded,
                      &MediaDecoderStateMachine::OnAudioNotDecoded));
    }
    if (HasVideo()) {
      mVideoDecodeStartTime = TimeStamp::Now();
      mVideoDataRequest.Begin(ProxyMediaCall(DecodeTaskQueue(), mReader.get(),
                                             __func__, &MediaDecoderReader::RequestVideoData, false, int64_t(0))
        ->RefableThen(mScheduler.get(), __func__, this,
                      &MediaDecoderStateMachine::OnVideoDecoded,
                      &MediaDecoderStateMachine::OnVideoNotDecoded));
    }
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::FinishDecodeFirstFrame()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(OnStateMachineThread());
  DECODER_LOG("FinishDecodeFirstFrame");

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }

  if (!IsRealTime() && !mSentFirstFrameLoadedEvent) {
    const VideoData* v = VideoQueue().PeekFront();
    const AudioData* a = AudioQueue().PeekFront();
    SetStartTime(mReader->ComputeStartTime(v, a));
    if (VideoQueue().GetSize()) {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      RenderVideoFrame(VideoQueue().PeekFront(), TimeStamp::Now());
    }
  }

  NS_ASSERTION(mStartTime != -1, "Must have start time");
  MOZ_ASSERT(!(mDecoder->IsMediaSeekable() && mDecoder->IsTransportSeekable()) ||
               (GetDuration() != -1) || mDurationSet,
             "Seekable media should have duration");
  DECODER_LOG("Media goes from %lld to %lld (duration %lld) "
              "transportSeekable=%d, mediaSeekable=%d",
              mStartTime, mEndTime, GetDuration(),
              mDecoder->IsTransportSeekable(), mDecoder->IsMediaSeekable());

  if (HasAudio() && !HasVideo()) {
    
    
    
    mAmpleAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mLowAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mQuickBufferingLowDataThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
  }

  
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    mReader->ReadUpdatedMetadata(&mInfo);
  }

  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  if (!mGotDurationFromMetaData) {
    
    
    EnqueueLoadedMetadataEvent();
    EnqueueFirstFrameLoadedEvent();
  } else {
    
    EnqueueFirstFrameLoadedEvent();
  }

  if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
    StartDecoding();
  }

  
  
  
  CheckIfDecodeComplete();
  MaybeStartPlayback();

  if (mQueuedSeekTarget.IsValid()) {
    EnqueueStartQueuedSeekTask();
  }

  return NS_OK;
}

void
MediaDecoderStateMachine::OnSeekCompleted(int64_t aTime)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(OnStateMachineThread());
  mSeekRequest.Complete();

  
  
  mDecodeToSeekTarget = true;

  
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AcquireMonitorAndInvokeDispatchDecodeTasksIfNeeded);
  GetStateMachineThread()->Dispatch(event, NS_DISPATCH_NORMAL);
}

void
MediaDecoderStateMachine::OnSeekFailed(nsresult aResult)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(OnStateMachineThread());
  mSeekRequest.Complete();
  MOZ_ASSERT(NS_FAILED(aResult), "Cancels should also disconnect mSeekRequest");
  DecodeError();
}

void
MediaDecoderStateMachine::SeekCompleted()
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
  
  AutoSetOnScopeExit<SeekTarget> reset(mCurrentSeekTarget, SeekTarget());

  if (mState != DECODER_STATE_SEEKING) {
    return;
  }

  int64_t seekTime = mCurrentSeekTarget.mTime;
  int64_t newCurrentTime = mCurrentSeekTarget.mTime;

  
  VideoData* video = VideoQueue().PeekFront();
  if (seekTime == mEndTime) {
    newCurrentTime = mAudioStartTime = seekTime;
  } else if (HasAudio()) {
    AudioData* audio = AudioQueue().PeekFront();
    
    
    
    
    
    
    int64_t videoStart = video ? video->mTime : seekTime;
    int64_t audioStart = audio ? audio->mTime : seekTime;
    newCurrentTime = mAudioStartTime =
        std::min(std::min(audioStart, videoStart), seekTime);
  } else {
    newCurrentTime = video ? video->mTime : seekTime;
  }
  mPlayDuration = newCurrentTime - mStartTime;

  if (HasVideo()) {
    if (video) {
      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        RenderVideoFrame(video, TimeStamp::Now());
      }
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(mDecoder, &MediaDecoder::Invalidate);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }
  }

  MOZ_ASSERT(mState != DECODER_STATE_DECODING_NONE);

  mDecoder->StartProgressUpdates();
  if (mState == DECODER_STATE_DECODING_METADATA ||
      mState == DECODER_STATE_DECODING_FIRSTFRAME ||
      mState == DECODER_STATE_DORMANT ||
      mState == DECODER_STATE_SHUTDOWN) {
    return;
  }

  
  
  

  nsCOMPtr<nsIRunnable> stopEvent;
  bool isLiveStream = mDecoder->GetResource()->GetLength() == -1;
  if (mSeekTarget.IsValid()) {
    
    
    DECODER_LOG("A new seek came along while we were finishing the old one - staying in SEEKING");
    SetState(DECODER_STATE_SEEKING);
  } else if (GetMediaTime() == mEndTime && !isLiveStream) {
    
    
    
    DECODER_LOG("Changed state from SEEKING (to %lld) to COMPLETED", seekTime);
    stopEvent = NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
                  mDecoder,
                  &MediaDecoder::SeekingStoppedAtEnd,
                  mCurrentSeekTarget.mEventVisibility);
    
    
    SetState(DECODER_STATE_COMPLETED);
    DispatchDecodeTasksIfNeeded();
  } else {
    DECODER_LOG("Changed state from SEEKING (to %lld) to DECODING", seekTime);
    stopEvent = NS_NewRunnableMethodWithArg<MediaDecoderEventVisibility>(
                  mDecoder,
                  &MediaDecoder::SeekingStopped,
                  mCurrentSeekTarget.mEventVisibility);
    StartDecoding();
  }

  
  UpdatePlaybackPositionInternal(newCurrentTime);

  
  DECODER_LOG("Seek completed, mCurrentFrameTime=%lld", mCurrentFrameTime);

  
  
  
  mQuickBuffering = false;

  
  
  mScheduler->FreezeScheduling();
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
  }

  ScheduleStateMachine();
  mScheduler->ThawScheduling();
}


class nsDecoderDisposeEvent : public nsRunnable {
public:
  nsDecoderDisposeEvent(already_AddRefed<MediaDecoder> aDecoder,
                        already_AddRefed<MediaDecoderStateMachine> aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}
  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
    MOZ_ASSERT(mStateMachine);
    MOZ_ASSERT(mDecoder);
    mStateMachine->BreakCycles();
    mDecoder->BreakCycles();
    mStateMachine = nullptr;
    mDecoder = nullptr;
    return NS_OK;
  }
private:
  nsRefPtr<MediaDecoder> mDecoder;
  nsRefPtr<MediaDecoderStateMachine> mStateMachine;
};





class nsDispatchDisposeEvent : public nsRunnable {
public:
  nsDispatchDisposeEvent(MediaDecoder* aDecoder,
                         MediaDecoderStateMachine* aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}
  NS_IMETHOD Run() {
    NS_DispatchToMainThread(new nsDecoderDisposeEvent(mDecoder.forget(),
                                                      mStateMachine.forget()));
    return NS_OK;
  }
private:
  nsRefPtr<MediaDecoder> mDecoder;
  nsRefPtr<MediaDecoderStateMachine> mStateMachine;
};

void
MediaDecoderStateMachine::ShutdownReader()
{
  MOZ_ASSERT(OnDecodeThread());
  mReader->Shutdown()->Then(mScheduler.get(), __func__, this,
                            &MediaDecoderStateMachine::FinishShutdown,
                            &MediaDecoderStateMachine::FinishShutdown);
}

void
MediaDecoderStateMachine::FinishShutdown()
{
  MOZ_ASSERT(OnStateMachineThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
  AudioQueue().ClearListeners();
  VideoQueue().ClearListeners();

  
  
  mPendingWakeDecoder = nullptr;

  MOZ_ASSERT(mState == DECODER_STATE_SHUTDOWN,
             "How did we escape from the shutdown state?");
  
  
  
  
  
  
  
  
  
  
  
  
  
  GetStateMachineThread()->Dispatch(
    new nsDispatchDisposeEvent(mDecoder, this), NS_DISPATCH_NORMAL);

  DECODER_LOG("Dispose Event Dispatched");
}

nsresult MediaDecoderStateMachine::RunStateMachine()
{
  AssertCurrentThreadInMonitor();

  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource, NS_ERROR_NULL_POINTER);

  switch (mState) {
    case DECODER_STATE_SHUTDOWN: {
      if (IsPlaying()) {
        StopPlayback();
      }

      StopAudioThread();
      FlushDecoding();

      
      
      RefPtr<nsIRunnable> task;
      task = NS_NewRunnableMethod(this, &MediaDecoderStateMachine::ShutdownReader);
      DebugOnly<nsresult> rv = DecodeTaskQueue()->Dispatch(task);
      MOZ_ASSERT(NS_SUCCEEDED(rv));

      DECODER_LOG("Shutdown started");
      return NS_OK;
    }

    case DECODER_STATE_DORMANT: {
      if (IsPlaying()) {
        StopPlayback();
      }
      StopAudioThread();
      FlushDecoding();
      
      
      mPendingWakeDecoder = nullptr;
      DebugOnly<nsresult> rv = DecodeTaskQueue()->Dispatch(
      NS_NewRunnableMethod(mReader, &MediaDecoderReader::ReleaseMediaResources));
      MOZ_ASSERT(NS_SUCCEEDED(rv));
      return NS_OK;
    }

    case DECODER_STATE_WAIT_FOR_RESOURCES: {
      return NS_OK;
    }

    case DECODER_STATE_DECODING_NONE: {
      SetState(DECODER_STATE_DECODING_METADATA);
      
      return EnqueueDecodeMetadataTask();
    }

    case DECODER_STATE_DECODING_METADATA: {
      return NS_OK;
    }

    case DECODER_STATE_DECODING_FIRSTFRAME: {
      
      return NS_OK;
    }

    case DECODER_STATE_DECODING: {
      if (mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING &&
          IsPlaying())
      {
        
        
        StopPlayback();
      }

      
      MaybeStartPlayback();

      AdvanceFrame();
      NS_ASSERTION(mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING ||
                   IsStateMachineScheduled() ||
                   mPlaybackRate == 0.0, "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_BUFFERING: {
      TimeStamp now = TimeStamp::Now();
      NS_ASSERTION(!mBufferingStart.IsNull(), "Must know buffering start time.");

      
      
      
      if (mReader->UseBufferingHeuristics()) {
        TimeDuration elapsed = now - mBufferingStart;
        bool isLiveStream = resource->GetLength() == -1;
        if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
              elapsed < TimeDuration::FromSeconds(mBufferingWait * mPlaybackRate) &&
              (mQuickBuffering ? HasLowDecodedData(mQuickBufferingLowDataThresholdUsecs)
                               : HasLowUndecodedData(mBufferingWait * USECS_PER_S)) &&
              mDecoder->IsExpectingMoreData())
        {
          DECODER_LOG("Buffering: wait %ds, timeout in %.3lfs %s",
                      mBufferingWait, mBufferingWait - elapsed.ToSeconds(),
                      (mQuickBuffering ? "(quick exit)" : ""));
          ScheduleStateMachine(USECS_PER_S);
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
      UpdateReadyState();
      MaybeStartPlayback();
      NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_SEEKING: {
      if (mSeekTarget.IsValid()) {
        InitiateSeek();
      }
      return NS_OK;
    }

    case DECODER_STATE_COMPLETED: {
      
      
      
      if (VideoQueue().GetSize() > 0 ||
          (HasAudio() && !mAudioCompleted) ||
          (mAudioCaptured && !mDecoder->GetDecodedStream()->IsFinished()))
      {
        AdvanceFrame();
        NS_ASSERTION(mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING ||
                     mPlaybackRate == 0 ||
                     IsStateMachineScheduled(),
                     "Must have timer scheduled");
        return NS_OK;
      }

      
      
      StopPlayback();

      if (mState != DECODER_STATE_COMPLETED) {
        
        
        NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
        return NS_OK;
      }

      StopAudioThread();

      if (mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING) {
        int64_t clockTime = std::max(mAudioEndTime, mVideoFrameEndTime);
        clockTime = std::max(int64_t(0), std::max(clockTime, mEndTime));
        UpdatePlaybackPosition(clockTime);

        {
          
          
          
          
          ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
          nsCOMPtr<nsIRunnable> event =
            NS_NewRunnableMethod(mDecoder, &MediaDecoder::PlaybackEnded);
          NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
        }
      }
      return NS_OK;
    }
  }

  return NS_OK;
}

void
MediaDecoderStateMachine::FlushDecoding()
{
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();

  
  
  
  ResetDecode();
  {
    
    
    
    
    
    
    
    
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    DecodeTaskQueue()->AwaitIdle();
  }

  
  
  
  ResetPlayback();
}

void
MediaDecoderStateMachine::ResetDecode()
{
  MOZ_ASSERT(OnStateMachineThread());
  AssertCurrentThreadInMonitor();

  mAudioDataRequest.DisconnectIfExists();
  mAudioWaitRequest.DisconnectIfExists();
  mVideoDataRequest.DisconnectIfExists();
  mVideoWaitRequest.DisconnectIfExists();
  mSeekRequest.DisconnectIfExists();

  mDecodeToSeekTarget = false;

  RefPtr<nsRunnable> resetTask =
    NS_NewRunnableMethod(mReader, &MediaDecoderReader::ResetDecode);
  DecodeTaskQueue()->Dispatch(resetTask);
}

void MediaDecoderStateMachine::RenderVideoFrame(VideoData* aData,
                                                TimeStamp aTarget)
{
  MOZ_ASSERT(OnStateMachineThread());
  mDecoder->GetReentrantMonitor().AssertNotCurrentThreadIn();

  if (aData->mDuplicate) {
    return;
  }

  VERBOSE_LOG("playing video frame %lld (queued=%i, state-machine=%i, decoder-queued=%i)",
              aData->mTime, VideoQueue().GetSize() + mReader->SizeOfVideoQueueInFrames(),
              VideoQueue().GetSize(), mReader->SizeOfVideoQueueInFrames());

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->SetCurrentFrame(ThebesIntSize(aData->mDisplay), aData->mImage,
                               aTarget);
    MOZ_ASSERT(container->GetFrameDelay() >= 0 || IsRealTime());
  }
}

void MediaDecoderStateMachine::ResyncAudioClock()
{
  AssertCurrentThreadInMonitor();
  if (IsPlaying()) {
    SetPlayStartTime(TimeStamp::Now());
    mPlayDuration = GetAudioClock() - mStartTime;
  }
}

int64_t
MediaDecoderStateMachine::GetAudioClock() const
{
  
  
  
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(HasAudio() && !mAudioCompleted);
  return mAudioStartTime +
         (mAudioSink ? mAudioSink->GetPosition() : 0);
}

int64_t MediaDecoderStateMachine::GetVideoStreamPosition() const
{
  AssertCurrentThreadInMonitor();

  if (!IsPlaying()) {
    return mPlayDuration + mStartTime;
  }

  
  int64_t delta = DurationToUsecs(TimeStamp::Now() - mPlayStartTime);
  
  delta *= mPlaybackRate;
  return mStartTime + mPlayDuration + delta;
}

int64_t MediaDecoderStateMachine::GetClock() const
{
  AssertCurrentThreadInMonitor();

  
  
  
  
  int64_t clock_time = -1;
  if (!IsPlaying()) {
    clock_time = mPlayDuration + mStartTime;
  } else {
    if (mAudioCaptured) {
      clock_time = mStartTime + mDecoder->GetDecodedStream()->GetClock();
    } else if (HasAudio() && !mAudioCompleted) {
      clock_time = GetAudioClock();
    } else {
      
      clock_time = GetVideoStreamPosition();
    }
    
    
    NS_ASSERTION(GetMediaTime() <= clock_time ||
                 mPlaybackRate <= 0 ||
                 (mAudioCaptured && mState == DECODER_STATE_SEEKING),
      "Clock should go forwards.");
  }

  return clock_time;
}

void MediaDecoderStateMachine::AdvanceFrame()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(!HasAudio() || mAudioStartTime != -1,
               "Should know audio start time if we have audio.");

  if (mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING) {
    return;
  }

  
  
  if (mPlaybackRate == 0.0) {
    return;
  }

  if (mAudioCaptured) {
    SendStreamData();
  }

  const int64_t clock_time = GetClock();
  TimeStamp nowTime = TimeStamp::Now();
  
  
  int64_t remainingTime = AUDIO_DURATION_USECS;
  NS_ASSERTION(clock_time >= mStartTime, "Should have positive clock time.");
  nsRefPtr<VideoData> currentFrame;
  if (VideoQueue().GetSize() > 0) {
    VideoData* frame = VideoQueue().PeekFront();
#ifdef PR_LOGGING
    int32_t droppedFrames = 0;
#endif
    while (IsRealTime() || clock_time >= frame->mTime) {
      mVideoFrameEndTime = frame->GetEndTime();
      if (currentFrame) {
        mDecoder->NotifyDecodedFrames(0, 0, 1);
#ifdef PR_LOGGING
        VERBOSE_LOG("discarding video frame mTime=%lld clock_time=%lld (%d so far)",
                    currentFrame->mTime, clock_time, ++droppedFrames);
#endif
      }
      currentFrame = frame;
      nsRefPtr<VideoData> releaseMe = VideoQueue().PopFront();
      
      
      mDecoder->GetReentrantMonitor().NotifyAll();
      OnPlaybackOffsetUpdate(frame->mOffset);
      if (VideoQueue().GetSize() == 0)
        break;
      frame = VideoQueue().PeekFront();
    }
    
    
    if (frame && !currentFrame) {
      int64_t now = IsPlaying() ? clock_time : mStartTime + mPlayDuration;

      remainingTime = frame->mTime - now;
    }
  }

  
  
  if (mState == DECODER_STATE_DECODING &&
      mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
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
      if (currentFrame) {
        VideoQueue().PushFront(currentFrame);
      }
      StartBuffering();
      
      
      
      ScheduleStateMachine(USECS_PER_S);
      return;
    }
  }

  
  
  if ((mFragmentEndTime >= 0 && clock_time < mFragmentEndTime) || mFragmentEndTime < 0) {
    MaybeStartPlayback();
  }

  
  
  
  if (mVideoFrameEndTime != -1 || mAudioEndTime != -1) {
    
    int64_t t = std::min(clock_time, std::max(mVideoFrameEndTime, mAudioEndTime));
    
    
    if (t > GetMediaTime()) {
      UpdatePlaybackPosition(t);
    }
  }
  
  
  
  

  if (currentFrame) {
    
    int64_t delta = currentFrame->mTime - clock_time;
    TimeStamp presTime = nowTime + TimeDuration::FromMicroseconds(delta / mPlaybackRate);
    NS_ASSERTION(currentFrame->mTime >= mStartTime, "Should have positive frame time");
    
    
    int64_t frameTime = currentFrame->mTime - mStartTime;
    if (frameTime > 0  || (frameTime == 0 && mPlayDuration == 0) ||
        IsRealTime()) {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      
      
      RenderVideoFrame(currentFrame, presTime);
    }
    
    
    
    if (!IsPlaying()) {
      ScheduleStateMachine();
      return;
    }
    MediaDecoder::FrameStatistics& frameStats = mDecoder->GetFrameStatistics();
    frameStats.NotifyPresentedFrame();
    remainingTime = currentFrame->GetEndTime() - clock_time;
    currentFrame = nullptr;
  }

  
  
  
  
  UpdateReadyState();

  ScheduleStateMachine(remainingTime / mPlaybackRate);
}

nsresult
MediaDecoderStateMachine::DropVideoUpToSeekTarget(VideoData* aSample)
{
  nsRefPtr<VideoData> video(aSample);
  MOZ_ASSERT(video);
  DECODER_LOG("DropVideoUpToSeekTarget() frame [%lld, %lld] dup=%d",
              video->mTime, video->GetEndTime(), video->mDuplicate);
  const int64_t target = mCurrentSeekTarget.mTime;

  
  
  
  
  
  
  if (video->mDuplicate &&
      mFirstVideoFrameAfterSeek &&
      !mFirstVideoFrameAfterSeek->mDuplicate) {
    nsRefPtr<VideoData> temp =
      VideoData::ShallowCopyUpdateTimestampAndDuration(mFirstVideoFrameAfterSeek,
                                                       video->mTime,
                                                       video->mDuration);
    video = temp;
  }

  
  
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

    VideoQueue().PushFront(video);
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::DropAudioUpToSeekTarget(AudioData* aSample)
{
  nsRefPtr<AudioData> audio(aSample);
  MOZ_ASSERT(audio &&
             mCurrentSeekTarget.IsValid() &&
             mCurrentSeekTarget.mType == SeekTarget::Accurate);

  CheckedInt64 startFrame = UsecsToFrames(audio->mTime,
                                          mInfo.mAudio.mRate);
  CheckedInt64 targetFrame = UsecsToFrames(mCurrentSeekTarget.mTime,
                                           mInfo.mAudio.mRate);
  if (!startFrame.isValid() || !targetFrame.isValid()) {
    return NS_ERROR_FAILURE;
  }
  if (startFrame.value() + audio->mFrames <= targetFrame.value()) {
    
    
    return NS_OK;
  }
  if (startFrame.value() > targetFrame.value()) {
    
    
    
    
    
    
    
    DECODER_WARN("Audio not synced after seek, maybe a poorly muxed file?");
    AudioQueue().Push(audio);
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
                                         mCurrentSeekTarget.mTime,
                                         duration.value(),
                                         frames,
                                         audioData.forget(),
                                         channels,
                                         audio->mRate));
  AudioQueue().PushFront(data);

  return NS_OK;
}

void MediaDecoderStateMachine::SetStartTime(int64_t aStartTimeUsecs)
{
  AssertCurrentThreadInMonitor();
  DECODER_LOG("SetStartTime(%lld)", aStartTimeUsecs);
  mStartTime = 0;
  if (aStartTimeUsecs != 0) {
    mStartTime = aStartTimeUsecs;
    if (mGotDurationFromMetaData && GetEndTime() != INT64_MAX) {
      NS_ASSERTION(mEndTime != -1,
                   "We should have mEndTime as supplied duration here");
      
      
      
      mEndTime = mStartTime + mEndTime;
    }
  }

  
  
  mReader->SetStartTime(mStartTime);

  
  
  
  mAudioStartTime = mStartTime;
  DECODER_LOG("Set media start time to %lld", mStartTime);
}

void MediaDecoderStateMachine::UpdateReadyState() {
  AssertCurrentThreadInMonitor();

  MediaDecoderOwner::NextFrameStatus nextFrameStatus = GetNextFrameStatus();
  
  
  
  
  if (nextFrameStatus == mLastFrameStatus) {
    return;
  }
  mLastFrameStatus = nextFrameStatus;

  







  nsCOMPtr<nsIRunnable> event;
  event = NS_NewRunnableMethod(mDecoder, &MediaDecoder::UpdateReadyStateForData);
  NS_DispatchToMainThread(event);
}

bool MediaDecoderStateMachine::JustExitedQuickBuffering()
{
  return !mDecodeStartTime.IsNull() &&
    mQuickBuffering &&
    (TimeStamp::Now() - mDecodeStartTime) < TimeDuration::FromMicroseconds(QUICK_BUFFER_THRESHOLD_USECS);
}

void MediaDecoderStateMachine::StartBuffering()
{
  AssertCurrentThreadInMonitor();

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
  UpdateReadyState();
  DECODER_LOG("Changed state from DECODING to BUFFERING, decoded for %.3lfs",
              decodeDuration.ToSeconds());
#ifdef PR_LOGGING
  MediaDecoder::Statistics stats = mDecoder->GetStatistics();
  DECODER_LOG("Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
              stats.mPlaybackRate/1024, stats.mPlaybackRateReliable ? "" : " (unreliable)",
              stats.mDownloadRate/1024, stats.mDownloadRateReliable ? "" : " (unreliable)");
#endif
}

void MediaDecoderStateMachine::SetPlayStartTime(const TimeStamp& aTimeStamp)
{
  AssertCurrentThreadInMonitor();
  mPlayStartTime = aTimeStamp;
  if (!mAudioSink) {
    return;
  }
  if (!mPlayStartTime.IsNull()) {
    mAudioSink->StartPlayback();
  } else {
    mAudioSink->StopPlayback();
  }
}

nsresult MediaDecoderStateMachine::CallRunStateMachine()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  
  if (mAudioCaptured) {
    StopAudioThread();
  }

  return RunStateMachine();
}

nsresult MediaDecoderStateMachine::TimeoutExpired(void* aClosure)
{
  MediaDecoderStateMachine* p = static_cast<MediaDecoderStateMachine*>(aClosure);
  return p->CallRunStateMachine();
}

void MediaDecoderStateMachine::ScheduleStateMachineWithLockAndWakeDecoder() {
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DispatchAudioDecodeTaskIfNeeded();
  DispatchVideoDecodeTaskIfNeeded();
}

nsresult MediaDecoderStateMachine::ScheduleStateMachine(int64_t aUsecs) {
  return mScheduler->Schedule(aUsecs);
}

bool MediaDecoderStateMachine::OnDecodeThread() const
{
  return !DecodeTaskQueue() || DecodeTaskQueue()->IsCurrentThreadIn();
}

bool MediaDecoderStateMachine::OnStateMachineThread() const
{
  return mScheduler->OnStateMachineThread();
}

nsIEventTarget* MediaDecoderStateMachine::GetStateMachineThread() const
{
  return mScheduler->GetStateMachineThread();
}

bool MediaDecoderStateMachine::IsStateMachineScheduled() const
{
  return mScheduler->IsScheduled();
}

void MediaDecoderStateMachine::SetPlaybackRate(double aPlaybackRate)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(aPlaybackRate != 0,
      "PlaybackRate == 0 should be handled before this function.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mPlaybackRate == aPlaybackRate) {
    return;
  }

  
  
  
  if (!HasAudio() && IsPlaying()) {
    
    
    mPlayDuration = GetVideoStreamPosition() - mStartTime;
    SetPlayStartTime(TimeStamp::Now());
  }

  mPlaybackRate = aPlaybackRate;
  if (mAudioSink) {
    mAudioSink->SetPlaybackRate(mPlaybackRate);
  }
}

void MediaDecoderStateMachine::SetPreservesPitch(bool aPreservesPitch)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mPreservesPitch = aPreservesPitch;
  if (mAudioSink) {
    mAudioSink->SetPreservesPitch(mPreservesPitch);
  }
}

void
MediaDecoderStateMachine::SetMinimizePrerollUntilPlaybackStarts()
{
  AssertCurrentThreadInMonitor();
  DECODER_LOG("SetMinimizePrerollUntilPlaybackStarts()");
  mMinimizePreroll = true;
}

bool MediaDecoderStateMachine::IsShutdown()
{
  AssertCurrentThreadInMonitor();
  return GetState() == DECODER_STATE_SHUTDOWN;
}

void MediaDecoderStateMachine::QueueMetadata(int64_t aPublishTime,
                                             nsAutoPtr<MediaInfo> aInfo,
                                             nsAutoPtr<MetadataTags> aTags)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  AssertCurrentThreadInMonitor();
  TimedMetadata* metadata = new TimedMetadata;
  metadata->mPublishTime = aPublishTime;
  metadata->mInfo = aInfo.forget();
  metadata->mTags = aTags.forget();
  mMetadataManager.QueueMetadata(metadata);
}

void MediaDecoderStateMachine::OnAudioEndTimeUpdate(int64_t aAudioEndTime)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  MOZ_ASSERT(aAudioEndTime >= mAudioEndTime);
  mAudioEndTime = aAudioEndTime;
}

void MediaDecoderStateMachine::OnPlaybackOffsetUpdate(int64_t aPlaybackOffset)
{
  mDecoder->UpdatePlaybackOffset(aPlaybackOffset);
}

void MediaDecoderStateMachine::OnAudioSinkComplete()
{
  AssertCurrentThreadInMonitor();
  if (mAudioCaptured) {
    return;
  }
  ResyncAudioClock();
  mAudioCompleted = true;
  UpdateReadyState();
  
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void MediaDecoderStateMachine::OnAudioSinkError()
{
  AssertCurrentThreadInMonitor();
  
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

} 


#undef DECODER_LOG
#undef VERBOSE_LOG
#undef DECODER_WARN
#undef DECODER_WARN_HELPER
