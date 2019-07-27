




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





static const uint32_t LOW_AUDIO_USECS = 300000;





const int64_t AMPLE_AUDIO_USECS = 1000000;






const int64_t NO_VIDEO_AMPLE_AUDIO_DIVISOR = 8;




static const uint32_t LOW_VIDEO_FRAMES = 1;






static const int32_t LOW_VIDEO_THRESHOLD_USECS = 16000;


static const int AUDIO_DURATION_USECS = 40000;





static const int THRESHOLD_FACTOR = 2;





static const int64_t LOW_DATA_THRESHOLD_USECS = 5000000;



static_assert(LOW_DATA_THRESHOLD_USECS > AMPLE_AUDIO_USECS,
              "LOW_DATA_THRESHOLD_USECS is too small");


static const uint32_t EXHAUSTED_DATA_MARGIN_USECS = 60000;










static const uint32_t QUICK_BUFFER_THRESHOLD_USECS = 2000000;



static const uint32_t QUICK_BUFFERING_LOW_DATA_USECS = 1000000;





static_assert(QUICK_BUFFERING_LOW_DATA_USECS <= AMPLE_AUDIO_USECS,
              "QUICK_BUFFERING_LOW_DATA_USECS is too large");





static const int64_t ESTIMATED_DURATION_FUZZ_FACTOR_USECS = USECS_PER_S / 2;

static TimeDuration UsecsToDuration(int64_t aUsecs) {
  return TimeDuration::FromMicroseconds(aUsecs);
}

static int64_t DurationToUsecs(TimeDuration aDuration) {
  return static_cast<int64_t>(aDuration.ToSeconds() * USECS_PER_S);
}

MediaDecoderStateMachine::MediaDecoderStateMachine(MediaDecoder* aDecoder,
                                                   MediaDecoderReader* aReader,
                                                   bool aRealTime) :
  mDecoder(aDecoder),
  mScheduler(new MediaDecoderStateMachineScheduler(
      aDecoder->GetReentrantMonitor(),
      &MediaDecoderStateMachine::TimeoutExpired,
      MOZ_THIS_IN_INITIALIZER_LIST(), aRealTime)),
  mState(DECODER_STATE_DECODING_NONE),
  mSyncPointInMediaStream(-1),
  mSyncPointInDecodedStream(-1),
  mPlayDuration(0),
  mStartTime(-1),
  mEndTime(-1),
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
  mAmpleVideoFrames(2),
  mLowAudioThresholdUsecs(LOW_AUDIO_USECS),
  mAmpleAudioThresholdUsecs(AMPLE_AUDIO_USECS),
  mAudioRequestPending(false),
  mVideoRequestPending(false),
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
  mWaitingForDecoderSeek(false),
  mCurrentTimeBeforeSeek(0),
  mLastFrameStatus(MediaDecoderOwner::NEXT_FRAME_UNINITIALIZED),
  mDecodingFrozenAtStateMetadata(false),
  mDecodingFrozenAtStateDecoding(false)
{
  MOZ_COUNT_CTOR(MediaDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  mAmpleVideoFrames =
    std::max<uint32_t>(Preferences::GetUint("media.video-queue.default-size", 10), 3);

  mBufferingWait = mScheduler->IsRealTime() ? 0 : mReader->GetBufferingWait();
  mLowDataThresholdUsecs = mScheduler->IsRealTime() ? 0 : LOW_DATA_THRESHOLD_USECS;

  mVideoPrerollFrames = mScheduler->IsRealTime() ? 0 : mAmpleVideoFrames / 2;
  mAudioPrerollUsecs = mScheduler->IsRealTime() ? 0 : LOW_AUDIO_USECS * 2;

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
  NS_ASSERTION(OnDecodeThread() || OnStateMachineThread(),
               "Should be on decode thread or state machine thread");
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
  NS_ASSERTION(OnDecodeThread() ||
               OnStateMachineThread(), "Should be on decode thread or state machine thread");
  AssertCurrentThreadInMonitor();

  if (aAudio->mTime <= aStream->mLastAudioPacketTime) {
    
    return;
  }
  aStream->mLastAudioPacketTime = aAudio->mTime;
  aStream->mLastAudioPacketEndTime = aAudio->GetEndTime();

  
  
  CheckedInt64 audioWrittenOffset = UsecsToFrames(mInfo.mAudio.mRate,
      aStream->mInitialTime + mStartTime) + aStream->mAudioFramesWritten;
  CheckedInt64 frameOffset = UsecsToFrames(mInfo.mAudio.mRate, aAudio->mTime);
  if (!audioWrittenOffset.isValid() || !frameOffset.isValid())
    return;
  if (audioWrittenOffset.value() < frameOffset.value()) {
    
    VERBOSE_LOG("writing %d frames of silence to MediaStream",
                int32_t(frameOffset.value() - audioWrittenOffset.value()));
    AudioSegment silence;
    silence.InsertNullDataAtStart(frameOffset.value() - audioWrittenOffset.value());
    aStream->mAudioFramesWritten += silence.GetDuration();
    aOutput->AppendFrom(&silence);
  }

  int64_t offset;
  if (aStream->mAudioFramesWritten == 0) {
    NS_ASSERTION(frameOffset.value() <= audioWrittenOffset.value(),
                 "Otherwise we'd have taken the write-silence path");
    
    offset = audioWrittenOffset.value() - frameOffset.value();
  } else {
    
    offset = 0;
  }

  if (offset >= aAudio->mFrames)
    return;

  aAudio->EnsureAudioBuffer();
  nsRefPtr<SharedBuffer> buffer = aAudio->mAudioBuffer;
  AudioDataValue* bufferData = static_cast<AudioDataValue*>(buffer->Data());
  nsAutoTArray<const AudioDataValue*,2> channels;
  for (uint32_t i = 0; i < aAudio->mChannels; ++i) {
    channels.AppendElement(bufferData + i*aAudio->mFrames + offset);
  }
  aOutput->AppendFrames(buffer.forget(), channels, aAudio->mFrames);
  VERBOSE_LOG("writing %d frames of data to MediaStream for AudioData at %lld",
              aAudio->mFrames - int32_t(offset), aAudio->mTime);
  aStream->mAudioFramesWritten += aAudio->mFrames - int32_t(offset);
  aOutput->ApplyVolume(mVolume);
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
  NS_ASSERTION(OnDecodeThread() || OnStateMachineThread(),
               "Should be on decode thread or state machine thread");
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState != DECODER_STATE_DECODING_NONE);

  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  if (!stream) {
    return;
  }

  if (mState == DECODER_STATE_DECODING_METADATA ||
      mState == DECODER_STATE_DECODING_FIRSTFRAME) {
    return;
  }

  
  
  
  if (mAudioSink) {
    return;
  }

  int64_t minLastAudioPacketTime = INT64_MAX;
  bool finished =
      (!mInfo.HasAudio() || AudioQueue().IsFinished()) &&
      (!mInfo.HasVideo() || VideoQueue().IsFinished());
  if (mDecoder->IsSameOriginMedia()) {
    SourceMediaStream* mediaStream = stream->mStream;
    StreamTime endPosition = 0;

    if (!stream->mStreamInitialized) {
      if (mInfo.HasAudio()) {
        AudioSegment* audio = new AudioSegment();
        mediaStream->AddAudioTrack(kAudioTrack, mInfo.mAudio.mRate, 0, audio);
        stream->mStream->DispatchWhenNotEnoughBuffered(kAudioTrack,
            GetStateMachineThread(), GetWakeDecoderRunnable());
      }
      if (mInfo.HasVideo()) {
        VideoSegment* video = new VideoSegment();
        mediaStream->AddTrack(kVideoTrack, 0, video);
        stream->mStream->DispatchWhenNotEnoughBuffered(kVideoTrack,
            GetStateMachineThread(), GetWakeDecoderRunnable());
      }
      stream->mStreamInitialized = true;
    }

    if (mInfo.HasAudio()) {
      nsAutoTArray<nsRefPtr<AudioData>,10> audio;
      
      
      AudioQueue().GetElementsAfter(stream->mLastAudioPacketTime, &audio);
      AudioSegment output;
      for (uint32_t i = 0; i < audio.Length(); ++i) {
        SendStreamAudio(audio[i], stream, &output);
      }
      if (output.GetDuration() > 0) {
        mediaStream->AppendToTrack(kAudioTrack, &output);
      }
      if (AudioQueue().IsFinished() && !stream->mHaveSentFinishAudio) {
        mediaStream->EndTrack(kAudioTrack);
        stream->mHaveSentFinishAudio = true;
      }
      minLastAudioPacketTime = std::min(minLastAudioPacketTime, stream->mLastAudioPacketTime);
      endPosition = std::max(endPosition,
          mediaStream->TicksToTimeRoundDown(mInfo.mAudio.mRate,
                                            stream->mAudioFramesWritten));
    }

    if (mInfo.HasVideo()) {
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
        mediaStream->AppendToTrack(kVideoTrack, &output);
      }
      if (VideoQueue().IsFinished() && !stream->mHaveSentFinishVideo) {
        mediaStream->EndTrack(kVideoTrack);
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

  if (mAudioCaptured) {
    
    while (true) {
      const AudioData* a = AudioQueue().PeekFront();
      
      
      
      
      
      
      if (!a || a->GetEndTime() >= minLastAudioPacketTime)
        break;
      OnAudioEndTimeUpdate(std::max(mAudioEndTime, a->GetEndTime()));
      nsRefPtr<AudioData> releaseMe = AudioQueue().PopFront();
    }

    if (finished) {
      mAudioCompleted = true;
      UpdateReadyState();
    }
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
    if (!stream->mStream->HaveEnoughBuffered(kAudioTrack)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(kAudioTrack,
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
    if (!stream->mStream->HaveEnoughBuffered(kVideoTrack)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(kVideoTrack,
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
          (!mMinimizePreroll && !HaveEnoughDecodedVideo()));
}

bool
MediaDecoderStateMachine::NeedToSkipToNextKeyframe()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_DECODING ||
             mState == DECODER_STATE_BUFFERING ||
             mState == DECODER_STATE_SEEKING);

  
  if (!IsVideoDecoding() || mState == DECODER_STATE_BUFFERING ||
      mState == DECODER_STATE_SEEKING) {
    return false;
  }

  
  
  if (mDecoder->GetDecodedStream() && !HasAudio()) {
    DECODER_LOG("Video-only decoded stream, set skipToNextKeyFrame to false");
    return false;
  }

  
  
  
  
  
  
  bool isLowOnDecodedAudio = !mIsAudioPrerolling && IsAudioDecoding() &&
                             (GetDecodedAudioDuration() <
                              mLowAudioThresholdUsecs * mPlaybackRate);
  bool isLowOnDecodedVideo = !mIsVideoPrerolling &&
                             (mDecodedVideoEndTime - GetClock() <
                              LOW_VIDEO_THRESHOLD_USECS * mPlaybackRate);
  if ((isLowOnDecodedAudio || isLowOnDecodedVideo) && !HasLowUndecodedData()) {
    DECODER_LOG("Skipping video decode to the next keyframe");
    return true;
  }

  return false;
}

void
MediaDecoderStateMachine::DecodeVideo()
{
  int64_t currentTime = 0;
  bool skipToNextKeyFrame = false;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

    if (mState != DECODER_STATE_DECODING &&
        mState != DECODER_STATE_BUFFERING &&
        mState != DECODER_STATE_SEEKING) {
      mVideoRequestPending = false;
      DispatchDecodeTasksIfNeeded();
      return;
    }

    
    
    
    if (mIsVideoPrerolling &&
        (static_cast<uint32_t>(VideoQueue().GetSize())
          >= mVideoPrerollFrames * mPlaybackRate))
    {
      mIsVideoPrerolling = false;
    }

    skipToNextKeyFrame = NeedToSkipToNextKeyframe();

    currentTime = mState == DECODER_STATE_SEEKING ? 0 : GetMediaTime();

    
    
    
    mVideoDecodeStartTime = TimeStamp::Now();
  }

  SAMPLE_LOG("DecodeVideo() queued=%i, decoder-queued=%o, skip=%i, time=%lld",
             VideoQueue().GetSize(), mReader->SizeOfVideoQueueInFrames(), skipToNextKeyFrame, currentTime);

  mReader->RequestVideoData(skipToNextKeyFrame, currentTime)
         ->Then(DecodeTaskQueue(), __func__, this,
                &MediaDecoderStateMachine::OnVideoDecoded,
                &MediaDecoderStateMachine::OnVideoNotDecoded);
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
          (!mMinimizePreroll &&
          !HaveEnoughDecodedAudio(mAmpleAudioThresholdUsecs * mPlaybackRate) &&
          (mState != DECODER_STATE_SEEKING || mDecodeToSeekTarget)));
}

void
MediaDecoderStateMachine::DecodeAudio()
{
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

    if (mState != DECODER_STATE_DECODING &&
        mState != DECODER_STATE_BUFFERING &&
        mState != DECODER_STATE_SEEKING) {
      mAudioRequestPending = false;
      DispatchDecodeTasksIfNeeded();
      mon.NotifyAll();
      return;
    }

    
    
    
    if (mIsAudioPrerolling &&
        GetDecodedAudioDuration() >= mAudioPrerollUsecs * mPlaybackRate) {
      mIsAudioPrerolling = false;
    }
  }

  SAMPLE_LOG("DecodeAudio() queued=%i, decoder-queued=%o",
             AudioQueue().GetSize(), mReader->SizeOfAudioQueueInFrames());

  mReader->RequestAudioData()->Then(DecodeTaskQueue(), __func__, this,
                                    &MediaDecoderStateMachine::OnAudioDecoded,
                                    &MediaDecoderStateMachine::OnAudioNotDecoded);
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
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<AudioData> audio(aAudioSample);
  MOZ_ASSERT(audio);
  mAudioRequestPending = false;
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

    case DECODER_STATE_BUFFERING:
    case DECODER_STATE_DECODING: {
      
      Push(audio);
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
    SendStreamData();
    
    
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
    SendStreamData();
    
    
    UpdateReadyState();
    DispatchDecodeTasksIfNeeded();
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void
MediaDecoderStateMachine::OnNotDecoded(MediaData::Type aType,
                                       MediaDecoderReader::NotDecodedReason aReason)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  SAMPLE_LOG("OnNotDecoded (aType=%u, aReason=%u)", aType, aReason);
  bool isAudio = aType == MediaData::AUDIO_DATA;
  MOZ_ASSERT_IF(!isAudio, aType == MediaData::VIDEO_DATA);

  
  if (isAudio) {
    mAudioRequestPending = false;
  } else {
    mVideoRequestPending = false;
  }

  
  if (aReason == MediaDecoderReader::DECODE_ERROR) {
    DecodeError();
    return;
  }

  
  
  
  if (aReason == MediaDecoderReader::WAITING_FOR_DATA) {
    bool outOfSamples = isAudio ? !AudioQueue().GetSize() : !VideoQueue().GetSize();
    if (outOfSamples) {
      StartBuffering();
    }

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
  isAudio ? AudioQueue().Finish() : VideoQueue().Finish();
  switch (mState) {
    case DECODER_STATE_DECODING_FIRSTFRAME: {
      MaybeFinishDecodeFirstFrame();
      return;
    }

    case DECODER_STATE_BUFFERING:
    case DECODER_STATE_DECODING: {
      CheckIfDecodeComplete();
      SendStreamData();
      
      
      UpdateReadyState();
      mDecoder->GetReentrantMonitor().NotifyAll();
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
  MOZ_ASSERT(OnDecodeThread());
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  nsRefPtr<VideoData> video(aVideoSample);
  mVideoRequestPending = false;
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

    case DECODER_STATE_BUFFERING:
    case DECODER_STATE_DECODING: {
      Push(video);
      
      
      
      
      TimeDuration decodeTime = TimeStamp::Now() - mVideoDecodeStartTime;
      if (THRESHOLD_FACTOR * DurationToUsecs(decodeTime) > mLowAudioThresholdUsecs &&
          !HasLowUndecodedData())
      {
        mLowAudioThresholdUsecs =
          std::min(THRESHOLD_FACTOR * DurationToUsecs(decodeTime), AMPLE_AUDIO_USECS);
        mAmpleAudioThresholdUsecs = std::max(THRESHOLD_FACTOR * mLowAudioThresholdUsecs,
                                              mAmpleAudioThresholdUsecs);
        DECODER_LOG("Slow video decode, set mLowAudioThresholdUsecs=%lld mAmpleAudioThresholdUsecs=%lld",
                    mLowAudioThresholdUsecs, mAmpleAudioThresholdUsecs);
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
    nsresult rv = DecodeTaskQueue()->Dispatch(task);
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

  DispatchDecodeTasksIfNeeded();
}

void MediaDecoderStateMachine::SetSyncPointForMediaStream()
{
  AssertCurrentThreadInMonitor();

  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  if (!stream) {
    return;
  }

  mSyncPointInMediaStream = stream->GetLastOutputTime();
  mSyncPointInDecodedStream = mStartTime + mPlayDuration;
}

void MediaDecoderStateMachine::ResyncMediaStreamClock()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mDecoder->GetDecodedStream());

  if (IsPlaying()) {
    SetPlayStartTime(TimeStamp::Now());
    mPlayDuration = GetCurrentTimeViaMediaStreamSync() - mStartTime;
  }
}

int64_t MediaDecoderStateMachine::GetCurrentTimeViaMediaStreamSync() const
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mSyncPointInDecodedStream >= 0, "Should have set up sync point");
  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  int64_t streamDelta = stream->GetLastOutputTime() - mSyncPointInMediaStream;
  return mSyncPointInDecodedStream + streamDelta;
}

void MediaDecoderStateMachine::StartPlayback()
{
  DECODER_LOG("StartPlayback()");

  NS_ASSERTION(!IsPlaying(), "Shouldn't be playing when StartPlayback() is called");
  AssertCurrentThreadInMonitor();

  if (mDecoder->CheckDecoderCanOffloadAudio()) {
    DECODER_LOG("Offloading playback");
    return;
  }

  mDecoder->NotifyPlaybackStarted();
  SetPlayStartTime(TimeStamp::Now());

  NS_ASSERTION(IsPlaying(), "Should report playing by end of StartPlayback()");
  nsresult rv = StartAudioThread();
  NS_ENSURE_SUCCESS_VOID(rv);

  mDecoder->GetReentrantMonitor().NotifyAll();
  mDecoder->UpdateStreamBlockingForStateMachinePlaying();
  DispatchDecodeTasksIfNeeded();
}

void MediaDecoderStateMachine::UpdatePlaybackPositionInternal(int64_t aTime)
{
  SAMPLE_LOG("UpdatePlaybackPositionInternal(%lld) (mStartTime=%lld)", aTime, mStartTime);
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine thread.");
  AssertCurrentThreadInMonitor();

  NS_ASSERTION(mStartTime >= 0, "Should have positive mStartTime");
  mCurrentFrameTime = aTime - mStartTime;
  NS_ASSERTION(mCurrentFrameTime >= 0, "CurrentTime should be positive!");
  if (aTime > mEndTime) {
    NS_ASSERTION(mCurrentFrameTime > GetDuration(),
                 "CurrentTime must be after duration if aTime > endTime!");
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
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::PlaybackPositionChanged);
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

void MediaDecoderStateMachine::SetAudioCaptured(bool aCaptured)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (!mAudioCaptured && aCaptured && !mStopAudioThread) {
    
    
    
    
    ScheduleStateMachine();
  }
  mAudioCaptured = aCaptured;
}

double MediaDecoderStateMachine::GetCurrentTime() const
{
  NS_ASSERTION(NS_IsMainThread() ||
               OnStateMachineThread() ||
               OnDecodeThread(),
               "Should be on main, decode, or state machine thread.");

  return static_cast<double>(mCurrentFrameTime) / static_cast<double>(USECS_PER_S);
}

int64_t MediaDecoderStateMachine::GetDuration()
{
  AssertCurrentThreadInMonitor();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

void MediaDecoderStateMachine::SetDuration(int64_t aDuration)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
               "Should be on main or decode thread.");
  AssertCurrentThreadInMonitor();

  if (aDuration == -1) {
    return;
  }

  if (mStartTime == -1) {
    SetStartTime(0);
  }

  mEndTime = mStartTime + aDuration;
}

void MediaDecoderStateMachine::UpdateEstimatedDuration(int64_t aDuration)
{
  AssertCurrentThreadInMonitor();
  int64_t duration = GetDuration();
  if (aDuration != duration &&
      std::abs(aDuration - duration) > ESTIMATED_DURATION_FUZZ_FACTOR_USECS) {
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
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  AssertCurrentThreadInMonitor();

  if (!mReader) {
    return;
  }

  DECODER_LOG("SetDormant=%d", aDormant);

  if (aDormant) {
    ScheduleStateMachine();
    SetState(DECODER_STATE_DORMANT);
    mDecoder->GetReentrantMonitor().NotifyAll();
  } else if ((aDormant != true) && (mState == DECODER_STATE_DORMANT)) {
    mDecodingFrozenAtStateMetadata = true;
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
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
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

  
  mIsAudioPrerolling = true;
  mIsVideoPrerolling = true;

  
  DispatchDecodeTasksIfNeeded();

  ScheduleStateMachine();
}

void MediaDecoderStateMachine::StartWaitForResources()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  AssertCurrentThreadInMonitor();
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

void MediaDecoderStateMachine::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    DECODER_LOG("Changed state from BUFFERING to DECODING");
    SetState(DECODER_STATE_DECODING);
    mDecodeStartTime = TimeStamp::Now();
  }
  if (mDecodingFrozenAtStateDecoding) {
    mDecodingFrozenAtStateDecoding = false;
    DispatchDecodeTasksIfNeeded();
  }
  
  
  mMinimizePreroll = false;
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::ResetPlayback()
{
  AssertCurrentThreadInMonitor();
  MOZ_ASSERT(mState == DECODER_STATE_SEEKING ||
             mState == DECODER_STATE_SHUTDOWN ||
             mState == DECODER_STATE_DORMANT);

  
  
  
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

  
  
  NS_ASSERTION(mState != DECODER_STATE_SEEKING,
               "We shouldn't already be seeking");
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "We should have got duration already");

  if (mState < DECODER_STATE_DECODING) {
    DECODER_LOG("Seek() Not Enough Data to continue at this stage, queuing seek");
    mQueuedSeekTarget = aTarget;
    return;
  }
  mQueuedSeekTarget.Reset();

  StartSeek(aTarget);
}

void
MediaDecoderStateMachine::EnqueueStartQueuedSeekTask()
{
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::StartQueuedSeek);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void
MediaDecoderStateMachine::StartQueuedSeek()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (!mQueuedSeekTarget.IsValid()) {
    return;
  }
  StartSeek(mQueuedSeekTarget);
  mQueuedSeekTarget.Reset();
}

void
MediaDecoderStateMachine::StartSeek(const SeekTarget& aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  AssertCurrentThreadInMonitor();

  MOZ_ASSERT(mState >= DECODER_STATE_DECODING);

  if (mState == DECODER_STATE_SHUTDOWN) {
    return;
  }

  
  NS_ASSERTION(mStartTime != -1, "Should know start time by now");
  NS_ASSERTION(mEndTime != -1, "Should know end time by now");
  int64_t seekTime = aTarget.mTime + mStartTime;
  seekTime = std::min(seekTime, mEndTime);
  seekTime = std::max(mStartTime, seekTime);
  NS_ASSERTION(seekTime >= mStartTime && seekTime <= mEndTime,
               "Can only seek in range [0,duration]");
  mSeekTarget = SeekTarget(seekTime, aTarget.mType);

  DECODER_LOG("Changed state to SEEKING (to %lld)", mSeekTarget.mTime);
  SetState(DECODER_STATE_SEEKING);
  mDecoder->RecreateDecodedStreamIfNecessary(seekTime - mStartTime);
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::StopAudioThread()
{
  NS_ASSERTION(OnDecodeThread() || OnStateMachineThread(),
               "Should be on decode thread or state machine thread");
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
    
    
    SendStreamData();
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
  nsresult rv = DecodeTaskQueue()->Dispatch(task);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

void
MediaDecoderStateMachine::SetReaderIdle()
{
#ifdef PR_LOGGING
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    DECODER_LOG("SetReaderIdle() audioQueue=%lld videoQueue=%lld",
                GetDecodedAudioDuration(),
                VideoQueue().Duration());
  }
#endif
  MOZ_ASSERT(OnDecodeThread());
  mReader->SetIdle();
}

void
MediaDecoderStateMachine::DispatchDecodeTasksIfNeeded()
{
  AssertCurrentThreadInMonitor();

  if (mState != DECODER_STATE_DECODING &&
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

  SAMPLE_LOG("DispatchDecodeTasksIfNeeded needAudio=%d dispAudio=%d needVideo=%d dispVid=%d needIdle=%d",
             needToDecodeAudio, mAudioRequestPending,
             needToDecodeVideo, mVideoRequestPending,
             needIdle);

  if (needToDecodeAudio) {
    EnsureAudioDecodeTaskQueued();
  }
  if (needToDecodeVideo) {
    EnsureVideoDecodeTaskQueued();
  }


  if (needIdle) {
    RefPtr<nsIRunnable> event = NS_NewRunnableMethod(
        this, &MediaDecoderStateMachine::SetReaderIdle);
    nsresult rv = DecodeTaskQueue()->Dispatch(event.forget());
    if (NS_FAILED(rv) && mState != DECODER_STATE_SHUTDOWN) {
      DECODER_WARN("Failed to dispatch event to set decoder idle state");
    }
  }
}

nsresult
MediaDecoderStateMachine::EnqueueDecodeSeekTask()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  AssertCurrentThreadInMonitor();

  if (mState != DECODER_STATE_SEEKING ||
      !mSeekTarget.IsValid() ||
      mCurrentSeekTarget.IsValid()) {
    return NS_OK;
  }
  mCurrentSeekTarget = mSeekTarget;
  mSeekTarget.Reset();
  mDropAudioUntilNextDiscontinuity = HasAudio();
  mDropVideoUntilNextDiscontinuity = HasVideo();

  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeSeek));
  nsresult rv = DecodeTaskQueue()->Dispatch(task);
  if (NS_FAILED(rv)) {
    DECODER_WARN("Dispatch DecodeSeek task failed.");
    mCurrentSeekTarget.Reset();
    DecodeError();
  }
  return rv;
}

nsresult
MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  if (NeedToDecodeAudio()) {
    return EnsureAudioDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureAudioDecodeTaskQueued()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  SAMPLE_LOG("EnsureAudioDecodeTaskQueued isDecoding=%d dispatched=%d",
              IsAudioDecoding(), mAudioRequestPending);

  if (mState >= DECODER_STATE_COMPLETED) {
    return NS_OK;
  }

  MOZ_ASSERT(mState > DECODER_STATE_DECODING_FIRSTFRAME);

  if (IsAudioDecoding() && !mAudioRequestPending && !mWaitingForDecoderSeek) {
    RefPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeAudio));
    nsresult rv = DecodeTaskQueue()->Dispatch(task);
    if (NS_SUCCEEDED(rv)) {
      mAudioRequestPending = true;
    } else {
      DECODER_WARN("Failed to dispatch task to decode audio");
    }
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  if (NeedToDecodeVideo()) {
    return EnsureVideoDecodeTaskQueued();
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::EnsureVideoDecodeTaskQueued()
{
  AssertCurrentThreadInMonitor();

  SAMPLE_LOG("EnsureVideoDecodeTaskQueued isDecoding=%d dispatched=%d",
             IsVideoDecoding(), mVideoRequestPending);

  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  if (mState >= DECODER_STATE_COMPLETED) {
    return NS_OK;
  }

  MOZ_ASSERT(mState > DECODER_STATE_DECODING_FIRSTFRAME);

  if (IsVideoDecoding() && !mVideoRequestPending && !mWaitingForDecoderSeek) {
    RefPtr<nsIRunnable> task(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeVideo));
    nsresult rv = DecodeTaskQueue()->Dispatch(task);
    if (NS_SUCCEEDED(rv)) {
      mVideoRequestPending = true;
    } else {
      DECODER_WARN("Failed to dispatch task to decode video");
    }
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::StartAudioThread()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
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
  return pushed + AudioQueue().Duration();
}

bool MediaDecoderStateMachine::HasLowDecodedData(int64_t aAudioUsecs)
{
  AssertCurrentThreadInMonitor();
  
  
  
  
  return ((IsAudioDecoding() && AudioDecodedUsecs() < aAudioUsecs) ||
         (IsVideoDecoding() &&
          static_cast<uint32_t>(VideoQueue().GetSize()) < LOW_VIDEO_FRAMES));
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
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  if (mState == DECODER_STATE_SHUTDOWN) {
    
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

  mReader->PreReadMetadata();

  if (mReader->IsWaitingMediaResources()) {
    StartWaitForResources();
    return NS_OK;
  }

  nsresult res;
  MediaInfo info;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    res = mReader->ReadMetadata(&info, getter_Transfers(mMetadataTags));
  }

  if (NS_SUCCEEDED(res)) {
    if (mState == DECODER_STATE_DECODING_METADATA &&
        mReader->IsWaitingMediaResources()) {
      
      StartWaitForResources();
      
      return NS_OK;
    }
  }

  if (NS_SUCCEEDED(res)) {
    mDecoder->SetMediaSeekable(mReader->IsMediaSeekable());
  }

  mInfo = info;

  if (NS_FAILED(res) || (!info.HasValidMedia())) {
    DECODER_WARN("ReadMetadata failed, res=%x HasValidMedia=%d", res, info.HasValidMedia());
    return NS_ERROR_FAILURE;
  }
  mDecoder->StartProgressUpdates();
  mGotDurationFromMetaData = (GetDuration() != -1);

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
  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new MetadataEventRunner(mDecoder, info, mMetadataTags);
  NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);
}

void
MediaDecoderStateMachine::CallDecodeFirstFrame()
{
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
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  MOZ_ASSERT(mState == DECODER_STATE_DECODING_FIRSTFRAME);
  DECODER_LOG("DecodeFirstFrame started");

  if (HasAudio()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded));
    AudioQueue().AddPopListener(decodeTask, DecodeTaskQueue());
  }
  if (HasVideo()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded));
    VideoQueue().AddPopListener(decodeTask, DecodeTaskQueue());
  }

  if (mScheduler->IsRealTime()) {
    SetStartTime(0);
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else if (mDecodingFrozenAtStateMetadata) {
    SetStartTime(mStartTime);
    nsresult res = FinishDecodeFirstFrame();
    NS_ENSURE_SUCCESS(res, res);
  } else {
    if (HasAudio()) {
      ReentrantMonitorAutoExit unlock(mDecoder->GetReentrantMonitor());
      mReader->RequestAudioData()->Then(DecodeTaskQueue(), __func__, this,
                                        &MediaDecoderStateMachine::OnAudioDecoded,
                                        &MediaDecoderStateMachine::OnAudioNotDecoded);
    }
    if (HasVideo()) {
      mVideoDecodeStartTime = TimeStamp::Now();
      ReentrantMonitorAutoExit unlock(mDecoder->GetReentrantMonitor());
      mReader->RequestVideoData(false, 0)
             ->Then(DecodeTaskQueue(), __func__, this,
                    &MediaDecoderStateMachine::OnVideoDecoded,
                    &MediaDecoderStateMachine::OnVideoNotDecoded);
    }
  }

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::FinishDecodeFirstFrame()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  DECODER_LOG("FinishDecodeFirstFrame");

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }

  if (!mScheduler->IsRealTime() && !mDecodingFrozenAtStateMetadata) {
    const VideoData* v = VideoQueue().PeekFront();
    const AudioData* a = AudioQueue().PeekFront();
    SetStartTime(mReader->ComputeStartTime(v, a));
    if (VideoQueue().GetSize()) {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      RenderVideoFrame(VideoQueue().PeekFront(), TimeStamp::Now());
    }
  }

  NS_ASSERTION(mStartTime != -1, "Must have start time");
  MOZ_ASSERT((!HasVideo() && !HasAudio()) ||
               !(mDecoder->IsMediaSeekable() && mDecoder->IsTransportSeekable()) ||
               mEndTime != -1,
             "Active seekable media should have end time");
  MOZ_ASSERT(!(mDecoder->IsMediaSeekable() && mDecoder->IsTransportSeekable()) ||
               GetDuration() != -1,
             "Seekable media should have duration");
  DECODER_LOG("Media goes from %lld to %lld (duration %lld) "
              "transportSeekable=%d, mediaSeekable=%d",
              mStartTime, mEndTime, GetDuration(),
              mDecoder->IsTransportSeekable(), mDecoder->IsMediaSeekable());

  mDecodingFrozenAtStateMetadata = false;

  if (HasAudio() && !HasVideo()) {
    
    
    
    mAmpleAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mLowAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
  }

  
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    mReader->ReadUpdatedMetadata(&mInfo);
  }

  nsAutoPtr<MediaInfo> info(new MediaInfo());
  *info = mInfo;
  nsCOMPtr<nsIRunnable> event;
  if (!mGotDurationFromMetaData) {
    
    
    event =
      new MetadataUpdatedEventRunner(mDecoder,
                                     info,
                                     mMetadataTags);
  } else {
    
    event =
      new FirstFrameLoadedEventRunner(mDecoder, info);
  }
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

  if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
    StartDecoding();
  }

  
  
  
  CheckIfDecodeComplete();

  if ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_COMPLETED) &&
      mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
      !IsPlaying())
  {
    StartPlayback();
  }

  if (mQueuedSeekTarget.IsValid()) {
    EnqueueStartQueuedSeekTask();
  }

  return NS_OK;
}

void MediaDecoderStateMachine::DecodeSeek()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  if (mState != DECODER_STATE_SEEKING) {
    return;
  }

  
  
  
  
  
  
  
  
  int64_t seekTime = mCurrentSeekTarget.mTime;
  mDecoder->StopProgressUpdates();

  bool currentTimeChanged = false;
  mCurrentTimeBeforeSeek = GetMediaTime();
  if (mCurrentTimeBeforeSeek != seekTime) {
    currentTimeChanged = true;
    
    
    
    StopPlayback();
    UpdatePlaybackPositionInternal(seekTime);
  }

  
  
  
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    nsCOMPtr<nsIRunnable> startEvent =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStarted);
    NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
  }
  if (mState != DECODER_STATE_SEEKING) {
    
    return;
  }

  mDecodeToSeekTarget = false;

  if (!currentTimeChanged) {
    DECODER_LOG("Seek !currentTimeChanged...");
    nsresult rv = DecodeTaskQueue()->Dispatch(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::SeekCompleted));
    if (NS_FAILED(rv)) {
      DecodeError();
    }
  } else {
    
    
    
    StopAudioThread();
    ResetPlayback();

    nsresult res;
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      
      
      res = mReader->ResetDecode();
      if (NS_SUCCEEDED(res)) {
        mReader->Seek(seekTime, mStartTime, mEndTime, mCurrentTimeBeforeSeek)
               ->Then(DecodeTaskQueue(), __func__, this,
                      &MediaDecoderStateMachine::OnSeekCompleted,
                      &MediaDecoderStateMachine::OnSeekFailed);
      }
    }
    if (NS_FAILED(res)) {
      DecodeError();
      return;
    }
    mWaitingForDecoderSeek = true;
  }
}

void
MediaDecoderStateMachine::OnSeekCompleted()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mWaitingForDecoderSeek = false;

  
  
  mDecodeToSeekTarget = true;
  DispatchDecodeTasksIfNeeded();
}

void
MediaDecoderStateMachine::OnSeekFailed(nsresult aResult)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mWaitingForDecoderSeek = false;
  
  
  
  if (NS_FAILED(aResult)) {
    DecodeError();
  }
}

void
MediaDecoderStateMachine::SeekCompleted()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  
  
  AutoSetOnScopeExit<SeekTarget> reset(mCurrentSeekTarget, SeekTarget());

  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
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
    newCurrentTime = mAudioStartTime = audio ? audio->mTime : seekTime;
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
  if (GetMediaTime() == mEndTime && !isLiveStream) {
    
    
    
    DECODER_LOG("Changed state from SEEKING (to %lld) to COMPLETED", seekTime);
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStoppedAtEnd);
    
    
    SetState(DECODER_STATE_COMPLETED);
    DispatchDecodeTasksIfNeeded();
  } else {
    DECODER_LOG("Changed state from SEEKING (to %lld) to DECODING", seekTime);
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStopped);
    StartDecoding();
  }

  
  UpdatePlaybackPositionInternal(newCurrentTime);
  if (mDecoder->GetDecodedStream()) {
    SetSyncPointForMediaStream();
  }

  
  DECODER_LOG("Seek completed, mCurrentFrameTime=%lld", mCurrentFrameTime);

  mCurrentSeekTarget = SeekTarget();

  
  
  
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
  mReader->Shutdown()->Then(GetStateMachineThread(), __func__, this,
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
      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        
        DecodeTaskQueue()->AwaitIdle();
        mReader->ReleaseMediaResources();
      }
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

      if (mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
          !IsPlaying()) {
        
        
        StartPlayback();
      }

      AdvanceFrame();
      NS_ASSERTION(mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING ||
                   IsStateMachineScheduled() ||
                   mPlaybackRate == 0.0, "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_BUFFERING: {
      TimeStamp now = TimeStamp::Now();
      NS_ASSERTION(!mBufferingStart.IsNull(), "Must know buffering start time.");

      
      
      
      TimeDuration elapsed = now - mBufferingStart;
      bool isLiveStream = resource->GetLength() == -1;
      if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
            elapsed < TimeDuration::FromSeconds(mBufferingWait * mPlaybackRate) &&
            (mQuickBuffering ? HasLowDecodedData(QUICK_BUFFERING_LOW_DATA_USECS)
                             : HasLowUndecodedData(mBufferingWait * USECS_PER_S)) &&
            mDecoder->IsExpectingMoreData())
      {
        DECODER_LOG("Buffering: wait %ds, timeout in %.3lfs %s",
                    mBufferingWait, mBufferingWait - elapsed.ToSeconds(),
                    (mQuickBuffering ? "(quick exit)" : ""));
        ScheduleStateMachine(USECS_PER_S);
        return NS_OK;
      } else {
        DECODER_LOG("Changed state from BUFFERING to DECODING");
        DECODER_LOG("Buffered for %.3lfs", (now - mBufferingStart).ToSeconds());
        StartDecoding();
      }

      
      mDecoder->GetReentrantMonitor().NotifyAll();
      UpdateReadyState();
      if (mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
          !IsPlaying())
      {
        StartPlayback();
      }
      NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_SEEKING: {
      return EnqueueDecodeSeekTask();
    }

    case DECODER_STATE_COMPLETED: {
      
      
      
      if (VideoQueue().GetSize() > 0 ||
          (HasAudio() && !mAudioCompleted) ||
          (mDecoder->GetDecodedStream() && !mDecoder->GetDecodedStream()->IsFinished()))
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
      
      
      if (mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
          !mDecoder->GetDecodedStream()) {
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
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  AssertCurrentThreadInMonitor();

  {
    
    
    
    RefPtr<nsIRunnable> task;
    task = NS_NewRunnableMethod(mReader, &MediaDecoderReader::ResetDecode);

    
    
    
    
    
    
    
    
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    DecodeTaskQueue()->FlushAndDispatch(task);
  }

  
  
  
  mAudioRequestPending = false;
  mVideoRequestPending = false;

  
  
  
  ResetPlayback();
}

void MediaDecoderStateMachine::RenderVideoFrame(VideoData* aData,
                                                TimeStamp aTarget)
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
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
    MOZ_ASSERT(container->GetFrameDelay() >= 0 || mScheduler->IsRealTime());
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
  MOZ_ASSERT(HasAudio() && !mAudioCaptured);
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
  } else if (mDecoder->GetDecodedStream()) {
    clock_time = GetCurrentTimeViaMediaStreamSync();
  } else {
    if (HasAudio() && !mAudioCompleted && !mAudioCaptured) {
      clock_time = GetAudioClock();
    } else {
      
      clock_time = GetVideoStreamPosition();
    }
    
    
    NS_ASSERTION(GetMediaTime() <= clock_time || mPlaybackRate <= 0,
      "Clock should go forwards if the playback rate is > 0.");
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
    while (mScheduler->IsRealTime() || clock_time >= frame->mTime) {
      mVideoFrameEndTime = frame->GetEndTime();
#ifdef PR_LOGGING
      if (currentFrame) {
        VERBOSE_LOG("discarding video frame mTime=%lld clock_time=%lld (%d so far)",
                    currentFrame->mTime, clock_time, ++droppedFrames);
      }
#endif
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
      HasLowDecodedData(remainingTime + EXHAUSTED_DATA_MARGIN_USECS) &&
      mDecoder->IsExpectingMoreData()) {
    if (JustExitedQuickBuffering() || HasLowUndecodedData()) {
      if (currentFrame) {
        VideoQueue().PushFront(currentFrame);
      }
      StartBuffering();
      
      
      
      ScheduleStateMachine(USECS_PER_S);
      return;
    }
  }

  
  
  if (!IsPlaying() && ((mFragmentEndTime >= 0 && clock_time < mFragmentEndTime) || mFragmentEndTime < 0)) {
    StartPlayback();
  }

  if (currentFrame) {
    
    int64_t delta = currentFrame->mTime - clock_time;
    TimeStamp presTime = nowTime + TimeDuration::FromMicroseconds(delta / mPlaybackRate);
    NS_ASSERTION(currentFrame->mTime >= mStartTime, "Should have positive frame time");
    
    
    int64_t frameTime = currentFrame->mTime - mStartTime;
    if (frameTime > 0  || (frameTime == 0 && mPlayDuration == 0) ||
        mScheduler->IsRealTime()) {
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

  
  
  
  if (mVideoFrameEndTime != -1 || mAudioEndTime != -1) {
    
    int64_t t = std::min(clock_time, std::max(mVideoFrameEndTime, mAudioEndTime));
    
    
    if (t > GetMediaTime()) {
      UpdatePlaybackPosition(t);
    }
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
    if (mGotDurationFromMetaData) {
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

  
  
  RefPtr<nsIRunnable> task(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AcquireMonitorAndInvokeDecodeError));
  nsresult rv = DecodeTaskQueue()->Dispatch(task);
  if (NS_FAILED(rv)) {
    DECODER_WARN("Failed to dispatch AcquireMonitorAndInvokeDecodeError");
  }
}

} 


#undef DECODER_LOG
#undef VERBOSE_LOG
#undef DECODER_WARN
#undef DECODER_WARN_HELPER
