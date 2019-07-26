




#ifdef XP_WIN

#include "windows.h"
#include "mmsystem.h"
#endif

#include "mozilla/DebugOnly.h"
#include <stdint.h>

#include "MediaDecoderStateMachine.h"
#include "AudioStream.h"
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

#include <algorithm>

namespace mozilla {

using namespace mozilla::layers;
using namespace mozilla::dom;
using namespace mozilla::gfx;


#undef DECODER_LOG
#undef VERBOSE_LOG

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg, ...) \
  PR_LOG(gMediaDecoderLog, type, ("Decoder=%p " msg, mDecoder.get(), ##__VA_ARGS__))
#define VERBOSE_LOG(msg, ...)                          \
    PR_BEGIN_MACRO                                     \
      if (!PR_GetEnv("MOZ_QUIET")) {                   \
        DECODER_LOG(PR_LOG_DEBUG, msg, ##__VA_ARGS__); \
      }                                                \
    PR_END_MACRO
#else
#define DECODER_LOG(type, msg, ...)
#define VERBOSE_LOG(msg, ...)
#endif




#ifdef GetCurrentTime
#undef GetCurrentTime
#endif




static const uint32_t BUFFERING_WAIT_S = 30;





static const uint32_t LOW_AUDIO_USECS = 300000;





const int64_t AMPLE_AUDIO_USECS = 1000000;






const int64_t NO_VIDEO_AMPLE_AUDIO_DIVISOR = 8;






const uint32_t SILENCE_BYTES_CHUNK = 32 * 1024;




static const uint32_t LOW_VIDEO_FRAMES = 1;


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


static const uint32_t AUDIOSTREAM_MIN_WRITE_BEFORE_START_USECS = 200000;





static const int64_t ESTIMATED_DURATION_FUZZ_FACTOR_USECS = USECS_PER_S / 2;

static TimeDuration UsecsToDuration(int64_t aUsecs) {
  return TimeDuration::FromMilliseconds(static_cast<double>(aUsecs) / USECS_PER_MS);
}

static int64_t DurationToUsecs(TimeDuration aDuration) {
  return static_cast<int64_t>(aDuration.ToSeconds() * USECS_PER_S);
}

MediaDecoderStateMachine::MediaDecoderStateMachine(MediaDecoder* aDecoder,
                                                   MediaDecoderReader* aReader,
                                                   bool aRealTime) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mInRunningStateMachine(false),
  mSyncPointInMediaStream(-1),
  mSyncPointInDecodedStream(-1),
  mResetPlayStartTime(false),
  mPlayDuration(0),
  mStartTime(-1),
  mEndTime(-1),
  mFragmentEndTime(-1),
  mReader(aReader),
  mCurrentFrameTime(0),
  mAudioStartTime(-1),
  mAudioEndTime(-1),
  mVideoFrameEndTime(-1),
  mVolume(1.0),
  mPlaybackRate(1.0),
  mPreservesPitch(true),
  mBasePosition(0),
  mAmpleVideoFrames(2),
  mLowAudioThresholdUsecs(LOW_AUDIO_USECS),
  mAmpleAudioThresholdUsecs(AMPLE_AUDIO_USECS),
  mDispatchedAudioDecodeTask(false),
  mDispatchedVideoDecodeTask(false),
  mAudioCaptured(false),
  mTransportSeekable(true),
  mMediaSeekable(true),
  mPositionChangeQueued(false),
  mAudioCompleted(false),
  mGotDurationFromMetaData(false),
  mDispatchedEventToDecode(false),
  mStopAudioThread(true),
  mQuickBuffering(false),
  mMinimizePreroll(false),
  mDecodeThreadWaiting(false),
  mRealTime(aRealTime),
  mDispatchedDecodeMetadataTask(false),
  mDispatchedDecodeSeekTask(false),
  mLastFrameStatus(MediaDecoderOwner::NEXT_FRAME_UNINITIALIZED),
  mTimerId(0)
{
  MOZ_COUNT_CTOR(MediaDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  if (Preferences::GetBool("media.realtime_decoder.enabled", false) == false)
    mRealTime = false;

  mAmpleVideoFrames =
    std::max<uint32_t>(Preferences::GetUint("media.video-queue.default-size", 10), 3);

  mBufferingWait = mRealTime ? 0 : BUFFERING_WAIT_S;
  mLowDataThresholdUsecs = mRealTime ? 0 : LOW_DATA_THRESHOLD_USECS;

  mVideoPrerollFrames = mRealTime ? 0 : mAmpleVideoFrames / 2;
  mAudioPrerollUsecs = mRealTime ? 0 : LOW_AUDIO_USECS * 2;

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

  MOZ_ASSERT(!mDecodeTaskQueue, "Should be released in SHUTDOWN");
  
  MOZ_ASSERT(!mTimer, "Should be released in SHUTDOWN");
  mReader = nullptr;

#ifdef XP_WIN
  timeEndPeriod(1);
#endif
}

bool MediaDecoderStateMachine::HasFutureAudio() {
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedUsecs() > LOW_AUDIO_USECS * mPlaybackRate || AudioQueue().IsFinished());
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
}

static void WriteVideoToMediaStream(layers::Image* aImage,
                                    int64_t aDuration,
                                    const IntSize& aIntrinsicSize,
                                    VideoSegment* aOutput)
{
  nsRefPtr<layers::Image> image = aImage;
  aOutput->AppendFrame(image.forget(), aDuration, aIntrinsicSize);
}

static const TrackID TRACK_AUDIO = 1;
static const TrackID TRACK_VIDEO = 2;
static const TrackRate RATE_VIDEO = USECS_PER_S;

void MediaDecoderStateMachine::SendStreamData()
{
  NS_ASSERTION(OnDecodeThread() ||
               OnStateMachineThread(), "Should be on decode thread or state machine thread");
  AssertCurrentThreadInMonitor();

  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  if (!stream)
    return;

  if (mState == DECODER_STATE_DECODING_METADATA)
    return;

  
  
  
  
  if (mAudioThread)
    return;

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
        mediaStream->AddTrack(TRACK_AUDIO, mInfo.mAudio.mRate, 0, audio);
        stream->mStream->DispatchWhenNotEnoughBuffered(TRACK_AUDIO,
            GetStateMachineThread(), GetWakeDecoderRunnable());
      }
      if (mInfo.HasVideo()) {
        VideoSegment* video = new VideoSegment();
        mediaStream->AddTrack(TRACK_VIDEO, RATE_VIDEO, 0, video);
        stream->mStream->DispatchWhenNotEnoughBuffered(TRACK_VIDEO,
            GetStateMachineThread(), GetWakeDecoderRunnable());
      }
      stream->mStreamInitialized = true;
    }

    if (mInfo.HasAudio()) {
      nsAutoTArray<AudioData*,10> audio;
      
      
      AudioQueue().GetElementsAfter(stream->mLastAudioPacketTime, &audio);
      AudioSegment output;
      for (uint32_t i = 0; i < audio.Length(); ++i) {
        SendStreamAudio(audio[i], stream, &output);
      }
      if (output.GetDuration() > 0) {
        mediaStream->AppendToTrack(TRACK_AUDIO, &output);
      }
      if (AudioQueue().IsFinished() && !stream->mHaveSentFinishAudio) {
        mediaStream->EndTrack(TRACK_AUDIO);
        stream->mHaveSentFinishAudio = true;
      }
      minLastAudioPacketTime = std::min(minLastAudioPacketTime, stream->mLastAudioPacketTime);
      endPosition = std::max(endPosition,
          TicksToTimeRoundDown(mInfo.mAudio.mRate, stream->mAudioFramesWritten));
    }

    if (mInfo.HasVideo()) {
      nsAutoTArray<VideoData*,10> video;
      
      
      VideoQueue().GetElementsAfter(stream->mNextVideoTime, &video);
      VideoSegment output;
      for (uint32_t i = 0; i < video.Length(); ++i) {
        VideoData* v = video[i];
        if (stream->mNextVideoTime < v->mTime) {
          VERBOSE_LOG("writing last video to MediaStream %p for %lldus",
                      mediaStream, v->mTime - stream->mNextVideoTime);
          
          
          WriteVideoToMediaStream(stream->mLastVideoImage,
            v->mTime - stream->mNextVideoTime, stream->mLastVideoImageDisplaySize,
              &output);
          stream->mNextVideoTime = v->mTime;
        }
        if (stream->mNextVideoTime < v->GetEndTime()) {
          VERBOSE_LOG("writing video frame %lldus to MediaStream %p for %lldus",
                      v->mTime, mediaStream, v->GetEndTime() - stream->mNextVideoTime);
          WriteVideoToMediaStream(v->mImage,
              v->GetEndTime() - stream->mNextVideoTime, v->mDisplay,
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
        mediaStream->AppendToTrack(TRACK_VIDEO, &output);
      }
      if (VideoQueue().IsFinished() && !stream->mHaveSentFinishVideo) {
        mediaStream->EndTrack(TRACK_VIDEO);
        stream->mHaveSentFinishVideo = true;
      }
      endPosition = std::max(endPosition,
          TicksToTimeRoundDown(RATE_VIDEO, stream->mNextVideoTime - stream->mInitialTime));
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
      mAudioEndTime = std::max(mAudioEndTime, a->GetEndTime());
      delete AudioQueue().PopFront();
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
    if (!stream->mStream->HaveEnoughBuffered(TRACK_AUDIO)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(TRACK_AUDIO,
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
    if (!stream->mStream->HaveEnoughBuffered(TRACK_VIDEO)) {
      return false;
    }
    stream->mStream->DispatchWhenNotEnoughBuffered(TRACK_VIDEO,
        GetStateMachineThread(), GetWakeDecoderRunnable());
  }

  return true;
}

bool
MediaDecoderStateMachine::NeedToDecodeVideo()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  return mIsVideoDecoding &&
         !mMinimizePreroll &&
         !HaveEnoughDecodedVideo();
}

void
MediaDecoderStateMachine::DecodeVideo()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  if (mState != DECODER_STATE_DECODING && mState != DECODER_STATE_BUFFERING) {
    mDispatchedVideoDecodeTask = false;
    return;
  }

  
  
  
  if (mIsVideoPrerolling &&
      (static_cast<uint32_t>(VideoQueue().GetSize())
        >= mVideoPrerollFrames * mPlaybackRate))
  {
    mIsVideoPrerolling = false;
  }

  
  
  
  
  
  
  if (mState == DECODER_STATE_DECODING &&
      !mSkipToNextKeyFrame &&
      mIsVideoDecoding &&
      ((!mIsAudioPrerolling && mIsAudioDecoding &&
        GetDecodedAudioDuration() < mLowAudioThresholdUsecs * mPlaybackRate) ||
        (!mIsVideoPrerolling && mIsVideoDecoding &&
         
         
         GetClock() > mVideoFrameEndTime &&
        (static_cast<uint32_t>(VideoQueue().GetSize())
          < LOW_VIDEO_FRAMES * mPlaybackRate))) &&
      !HasLowUndecodedData())
  {
    mSkipToNextKeyFrame = true;
    DECODER_LOG(PR_LOG_DEBUG, "Skipping video decode to the next keyframe");
  }

  
  
  
  TimeDuration decodeTime;
  {
    int64_t currentTime = GetMediaTime();
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    TimeStamp start = TimeStamp::Now();
    mIsVideoDecoding = mReader->DecodeVideoFrame(mSkipToNextKeyFrame, currentTime);
    decodeTime = TimeStamp::Now() - start;
  }
  if (!mIsVideoDecoding) {
    
    VideoQueue().Finish();
    CheckIfDecodeComplete();
  }

  if (THRESHOLD_FACTOR * DurationToUsecs(decodeTime) > mLowAudioThresholdUsecs &&
      !HasLowUndecodedData())
  {
    mLowAudioThresholdUsecs =
      std::min(THRESHOLD_FACTOR * DurationToUsecs(decodeTime), AMPLE_AUDIO_USECS);
    mAmpleAudioThresholdUsecs = std::max(THRESHOLD_FACTOR * mLowAudioThresholdUsecs,
                                          mAmpleAudioThresholdUsecs);
    DECODER_LOG(PR_LOG_DEBUG, "Slow video decode, set mLowAudioThresholdUsecs=%lld mAmpleAudioThresholdUsecs=%lld",
                mLowAudioThresholdUsecs, mAmpleAudioThresholdUsecs);
  }

  SendStreamData();

  
  
  UpdateReadyState();

  mDispatchedVideoDecodeTask = false;
  DispatchDecodeTasksIfNeeded();
}

bool
MediaDecoderStateMachine::NeedToDecodeAudio()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  return mIsAudioDecoding &&
         !mMinimizePreroll &&
         !HaveEnoughDecodedAudio(mAmpleAudioThresholdUsecs * mPlaybackRate);
}

void
MediaDecoderStateMachine::DecodeAudio()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  if (mState != DECODER_STATE_DECODING && mState != DECODER_STATE_BUFFERING) {
    mDispatchedAudioDecodeTask = false;
    return;
  }

  
  
  
  if (mIsAudioPrerolling &&
      GetDecodedAudioDuration() >= mAudioPrerollUsecs * mPlaybackRate) {
    mIsAudioPrerolling = false;
  }

  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    mIsAudioDecoding = mReader->DecodeAudioData();
  }
  if (!mIsAudioDecoding) {
    
    AudioQueue().Finish();
    CheckIfDecodeComplete();
  }

  SendStreamData();

  
  
  mDecoder->GetReentrantMonitor().NotifyAll();

  
  
  UpdateReadyState();

  mDispatchedAudioDecodeTask = false;
  DispatchDecodeTasksIfNeeded();
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
  MOZ_ASSERT(!AudioQueue().IsFinished() || !mIsAudioDecoding);
  MOZ_ASSERT(!VideoQueue().IsFinished() || !mIsVideoDecoding);
  if (!mIsVideoDecoding && !mIsAudioDecoding) {
    
    
    mState = DECODER_STATE_COMPLETED;
    DispatchDecodeTasksIfNeeded();
    ScheduleStateMachine();
  }
  DECODER_LOG(PR_LOG_DEBUG, "CheckIfDecodeComplete %scompleted",
              ((mState == DECODER_STATE_COMPLETED) ? "" : "NOT "));
}

bool MediaDecoderStateMachine::IsPlaying()
{
  AssertCurrentThreadInMonitor();

  return !mPlayStartTime.IsNull();
}



static void
StartAudioStreamPlaybackIfNeeded(AudioStream* aStream)
{
  
  if (static_cast<double>(aStream->GetWritten()) / aStream->GetRate() >=
      static_cast<double>(AUDIOSTREAM_MIN_WRITE_BEFORE_START_USECS) / USECS_PER_S) {
    aStream->Start();
  }
}

static void WriteSilence(AudioStream* aStream, uint32_t aFrames)
{
  uint32_t numSamples = aFrames * aStream->GetChannels();
  nsAutoTArray<AudioDataValue, 1000> buf;
  buf.SetLength(numSamples);
  memset(buf.Elements(), 0, numSamples * sizeof(AudioDataValue));
  aStream->Write(buf.Elements(), aFrames);

  StartAudioStreamPlaybackIfNeeded(aStream);
}

void MediaDecoderStateMachine::AudioLoop()
{
  NS_ASSERTION(OnAudioThread(), "Should be on audio thread.");
  DECODER_LOG(PR_LOG_DEBUG, "Begun audio thread/loop");
  int64_t audioDuration = 0;
  int64_t audioStartTime = -1;
  uint32_t channels, rate;
  double volume = -1;
  bool setVolume;
  double playbackRate = -1;
  bool setPlaybackRate;
  bool preservesPitch;
  bool setPreservesPitch;
  AudioChannel audioChannel;

  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioCompleted = false;
    audioStartTime = mAudioStartTime;
    NS_ASSERTION(audioStartTime != -1, "Should have audio start time by now");
    channels = mInfo.mAudio.mChannels;
    rate = mInfo.mAudio.mRate;

    audioChannel = mDecoder->GetAudioChannel();
    volume = mVolume;
    preservesPitch = mPreservesPitch;
    playbackRate = mPlaybackRate;
  }

  {
    
    
    
    RefPtr<AudioStream> audioStream(new AudioStream());
    audioStream->Init(channels, rate, audioChannel, AudioStream::HighLatency);
    audioStream->SetVolume(volume);
    if (audioStream->SetPreservesPitch(preservesPitch) != NS_OK) {
      NS_WARNING("Setting the pitch preservation failed at AudioLoop start.");
    }
    if (playbackRate != 1.0) {
      NS_ASSERTION(playbackRate != 0,
                   "Don't set the playbackRate to 0 on an AudioStream.");
      if (audioStream->SetPlaybackRate(playbackRate) != NS_OK) {
        NS_WARNING("Setting the playback rate failed at AudioLoop start.");
      }
    }

    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mAudioStream = audioStream.forget();
    }
  }

  while (1) {
    
    
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      NS_ASSERTION(mState != DECODER_STATE_DECODING_METADATA,
                   "Should have meta data before audio started playing.");
      while (mState != DECODER_STATE_SHUTDOWN &&
             !mStopAudioThread &&
             (!IsPlaying() ||
              mState == DECODER_STATE_BUFFERING ||
              (AudioQueue().GetSize() == 0 &&
               !AudioQueue().AtEndOfStream())))
      {
        if (!IsPlaying() && !mAudioStream->IsPaused()) {
          mAudioStream->Pause();
        }
        mon.Wait();
      }

      
      
      if (mState == DECODER_STATE_SHUTDOWN ||
          mStopAudioThread ||
          AudioQueue().AtEndOfStream())
      {
        break;
      }

      
      
      setVolume = volume != mVolume;
      volume = mVolume;

      
      setPlaybackRate = playbackRate != mPlaybackRate;
      playbackRate = mPlaybackRate;

      
      setPreservesPitch = preservesPitch != mPreservesPitch;
      preservesPitch = mPreservesPitch;

      if (IsPlaying() && mAudioStream->IsPaused()) {
        mAudioStream->Resume();
      }
    }

    if (setVolume) {
      mAudioStream->SetVolume(volume);
    }
    if (setPlaybackRate) {
      NS_ASSERTION(playbackRate != 0,
                   "Don't set the playbackRate to 0 in the AudioStreams");
      if (mAudioStream->SetPlaybackRate(playbackRate) != NS_OK) {
        NS_WARNING("Setting the playback rate failed in AudioLoop.");
      }
    }
    if (setPreservesPitch) {
      if (mAudioStream->SetPreservesPitch(preservesPitch) != NS_OK) {
        NS_WARNING("Setting the pitch preservation failed in AudioLoop.");
      }
    }
    NS_ASSERTION(AudioQueue().GetSize() > 0,
                 "Should have data to play");
    
    
    const AudioData* s = AudioQueue().PeekFront();

    
    
    CheckedInt64 playedFrames = UsecsToFrames(audioStartTime, rate) +
                                              audioDuration;
    
    
    CheckedInt64 sampleTime = UsecsToFrames(s->mTime, rate);
    CheckedInt64 missingFrames = sampleTime - playedFrames;
    if (!missingFrames.isValid() || !sampleTime.isValid()) {
      NS_WARNING("Int overflow adding in AudioLoop()");
      break;
    }

    int64_t framesWritten = 0;
    if (missingFrames.value() > 0) {
      
      
      
      
      missingFrames = std::min<int64_t>(UINT32_MAX, missingFrames.value());
      VERBOSE_LOG("playing %d frames of silence", int32_t(missingFrames.value()));
      framesWritten = PlaySilence(static_cast<uint32_t>(missingFrames.value()),
                                  channels, playedFrames.value());
    } else {
      framesWritten = PlayFromAudioQueue(sampleTime.value(), channels);
    }
    audioDuration += framesWritten;
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      CheckedInt64 playedUsecs = FramesToUsecs(audioDuration, rate) + audioStartTime;
      if (!playedUsecs.isValid()) {
        NS_WARNING("Int overflow calculating audio end time");
        break;
      }
      mAudioEndTime = playedUsecs.value();
    }
  }
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    if (AudioQueue().AtEndOfStream() &&
        mState != DECODER_STATE_SHUTDOWN &&
        !mStopAudioThread)
    {
      
      
      mAudioStream->Start();
      
      
      bool seeking = false;
      {
        int64_t oldPosition = -1;
        int64_t position = GetMediaTime();
        while (oldPosition != position &&
               mAudioEndTime - position > 0 &&
               mState != DECODER_STATE_SEEKING &&
               mState != DECODER_STATE_SHUTDOWN)
        {
          const int64_t DRAIN_BLOCK_USECS = 100000;
          Wait(std::min(mAudioEndTime - position, DRAIN_BLOCK_USECS));
          oldPosition = position;
          position = GetMediaTime();
        }
        seeking = mState == DECODER_STATE_SEEKING;
      }

      if (!seeking && !mAudioStream->IsPaused()) {
        {
          ReentrantMonitorAutoExit exit(mDecoder->GetReentrantMonitor());
          mAudioStream->Drain();
        }
      }
    }
  }
  DECODER_LOG(PR_LOG_DEBUG, "Reached audio stream end.");
  {
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioStream->Shutdown();
    mAudioStream = nullptr;
    if (!mAudioCaptured) {
      mAudioCompleted = true;
      UpdateReadyState();
      
      mDecoder->GetReentrantMonitor().NotifyAll();
    }
  }

  DECODER_LOG(PR_LOG_DEBUG, "Audio stream finished playing, audio thread exit");
}

uint32_t MediaDecoderStateMachine::PlaySilence(uint32_t aFrames,
                                                   uint32_t aChannels,
                                                   uint64_t aFrameOffset)

{
  NS_ASSERTION(OnAudioThread(), "Only call on audio thread.");
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  uint32_t maxFrames = SILENCE_BYTES_CHUNK / aChannels / sizeof(AudioDataValue);
  uint32_t frames = std::min(aFrames, maxFrames);
  WriteSilence(mAudioStream, frames);
  return frames;
}

uint32_t MediaDecoderStateMachine::PlayFromAudioQueue(uint64_t aFrameOffset,
                                                      uint32_t aChannels)
{
  NS_ASSERTION(OnAudioThread(), "Only call on audio thread.");
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  nsAutoPtr<AudioData> audio(AudioQueue().PopFront());
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    NS_WARN_IF_FALSE(IsPlaying(), "Should be playing");
    
    
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
  int64_t offset = -1;
  uint32_t frames = 0;
  VERBOSE_LOG("playing %d frames of data to stream for AudioData at %lld",
              audio->mFrames, audio->mTime);
  mAudioStream->Write(audio->mAudioData,
                      audio->mFrames);

  aChannels = mAudioStream->GetOutChannels();

  StartAudioStreamPlaybackIfNeeded(mAudioStream);

  offset = audio->mOffset;
  frames = audio->mFrames;

  if (offset != -1) {
    mDecoder->UpdatePlaybackOffset(offset);
  }
  return frames;
}

nsresult MediaDecoderStateMachine::Init(MediaDecoderStateMachine* aCloneDonor)
{
  MOZ_ASSERT(NS_IsMainThread());

  RefPtr<SharedThreadPool> decodePool(
    SharedThreadPool::Get(NS_LITERAL_CSTRING("Media Decode"),
                          Preferences::GetUint("media.num-decode-threads", 25)));
  NS_ENSURE_TRUE(decodePool, NS_ERROR_FAILURE);

  RefPtr<SharedThreadPool> stateMachinePool(
    SharedThreadPool::Get(NS_LITERAL_CSTRING("Media State Machine"), 1));
  NS_ENSURE_TRUE(stateMachinePool, NS_ERROR_FAILURE);

  mDecodeTaskQueue = new MediaTaskQueue(decodePool.forget());
  NS_ENSURE_TRUE(mDecodeTaskQueue, NS_ERROR_FAILURE);

  MediaDecoderReader* cloneReader = nullptr;
  if (aCloneDonor) {
    cloneReader = aCloneDonor->mReader;
  }

  mStateMachineThreadPool = stateMachinePool;

  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mTimer->SetTarget(GetStateMachineThread());
  NS_ENSURE_SUCCESS(rv, rv);

  return mReader->Init(cloneReader);
}

void MediaDecoderStateMachine::StopPlayback()
{
  DECODER_LOG(PR_LOG_DEBUG, "StopPlayback()");

  AssertCurrentThreadInMonitor();

  mDecoder->NotifyPlaybackStopped();

  if (IsPlaying()) {
    mPlayDuration = GetClock();
    mPlayStartTime = TimeStamp();
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

int64_t MediaDecoderStateMachine::GetCurrentTimeViaMediaStreamSync()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mSyncPointInDecodedStream >= 0, "Should have set up sync point");
  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  StreamTime streamDelta = stream->GetLastOutputTime() - mSyncPointInMediaStream;
  return mSyncPointInDecodedStream + MediaTimeToMicroseconds(streamDelta);
}

void MediaDecoderStateMachine::StartPlayback()
{
  DECODER_LOG(PR_LOG_DEBUG, "StartPlayback()");

  NS_ASSERTION(!IsPlaying(), "Shouldn't be playing when StartPlayback() is called");
  AssertCurrentThreadInMonitor();

  mDecoder->NotifyPlaybackStarted();
  mPlayStartTime = TimeStamp::Now();

  NS_ASSERTION(IsPlaying(), "Should report playing by end of StartPlayback()");
  if (NS_FAILED(StartAudioThread())) {
    NS_WARNING("Failed to create audio thread");
  }
  mDecoder->GetReentrantMonitor().NotifyAll();
  mDecoder->UpdateStreamBlockingForStateMachinePlaying();
}

void MediaDecoderStateMachine::UpdatePlaybackPositionInternal(int64_t aTime)
{
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
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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
  if (IsBuffering() || IsSeeking()) {
    return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_BUFFERING;
  } else if (HaveNextFrameData()) {
    return MediaDecoderOwner::NEXT_FRAME_AVAILABLE;
  }
  return MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE;
}

void MediaDecoderStateMachine::SetVolume(double volume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mVolume = volume;
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

  if (mStartTime != -1) {
    mEndTime = mStartTime + aDuration;
  } else {
    mStartTime = 0;
    mEndTime = aDuration;
  }
}

void MediaDecoderStateMachine::UpdateEstimatedDuration(int64_t aDuration)
{
  AssertCurrentThreadInMonitor();
  int64_t duration = GetDuration();
  if (aDuration != duration &&
      abs(aDuration - duration) > ESTIMATED_DURATION_FUZZ_FACTOR_USECS) {
    SetDuration(aDuration);
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DurationChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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

void MediaDecoderStateMachine::SetTransportSeekable(bool aTransportSeekable)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
      "Should be on main thread or the decoder thread.");
  AssertCurrentThreadInMonitor();

  mTransportSeekable = aTransportSeekable;
}

void MediaDecoderStateMachine::SetMediaSeekable(bool aMediaSeekable)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
      "Should be on main thread or the decoder thread.");

  mMediaSeekable = aMediaSeekable;
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

  if (aDormant) {
    ScheduleStateMachine();
    mState = DECODER_STATE_DORMANT;
    mDecoder->GetReentrantMonitor().NotifyAll();
  } else if ((aDormant != true) && (mState == DECODER_STATE_DORMANT)) {
    ScheduleStateMachine();
    mStartTime = 0;
    mCurrentFrameTime = 0;
    mState = DECODER_STATE_DECODING_METADATA;
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
}

void MediaDecoderStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  DECODER_LOG(PR_LOG_DEBUG, "Changed state to SHUTDOWN");
  ScheduleStateMachine();
  mState = DECODER_STATE_SHUTDOWN;
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
  mState = DECODER_STATE_DECODING;

  mDecodeStartTime = TimeStamp::Now();

  
  
  mIsVideoDecoding = HasVideo() && !VideoQueue().IsFinished();
  mIsAudioDecoding = HasAudio() && !AudioQueue().IsFinished();

  CheckIfDecodeComplete();
  if (mState == DECODER_STATE_COMPLETED) {
    return;
  }

  
  mSkipToNextKeyFrame = false;
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
  mState = DECODER_STATE_WAIT_FOR_RESOURCES;
}

void MediaDecoderStateMachine::NotifyWaitingForResourcesStatusChanged()
{
  AssertCurrentThreadInMonitor();
  if (mState != DECODER_STATE_WAIT_FOR_RESOURCES ||
      mReader->IsWaitingMediaResources()) {
    return;
  }
  
  
  mState = DECODER_STATE_DECODING_METADATA;
  EnqueueDecodeMetadataTask();
}

void MediaDecoderStateMachine::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    DECODER_LOG(PR_LOG_DEBUG, "Changed state from BUFFERING to DECODING");
    mState = DECODER_STATE_DECODING;
    mDecodeStartTime = TimeStamp::Now();
  }
  
  
  mMinimizePreroll = false;
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::ResetPlayback()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mVideoFrameEndTime = -1;
  mAudioStartTime = -1;
  mAudioEndTime = -1;
  mAudioCompleted = false;
}

void MediaDecoderStateMachine::NotifyDataArrived(const char* aBuffer,
                                                     uint32_t aLength,
                                                     int64_t aOffset)
{
  NS_ASSERTION(NS_IsMainThread(), "Only call on main thread");
  mReader->NotifyDataArrived(aBuffer, aLength, aOffset);

  
  
  
  
  
  dom::TimeRanges buffered;
  if (mDecoder->IsInfinite() &&
      NS_SUCCEEDED(mDecoder->GetBuffered(&buffered)))
  {
    uint32_t length = 0;
    buffered.GetLength(&length);
    if (length) {
      double end = 0;
      buffered.End(length - 1, &end);
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      mEndTime = std::max<int64_t>(mEndTime, end * USECS_PER_S);
    }
  }
}

void MediaDecoderStateMachine::Seek(const SeekTarget& aTarget)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  if (!mMediaSeekable) {
    return;
  }
  
  
  NS_ASSERTION(mState != DECODER_STATE_SEEKING,
               "We shouldn't already be seeking");
  NS_ASSERTION(mState >= DECODER_STATE_DECODING,
               "We should have loaded metadata");

  
  NS_ASSERTION(mStartTime != -1, "Should know start time by now");
  NS_ASSERTION(mEndTime != -1, "Should know end time by now");
  int64_t seekTime = aTarget.mTime + mStartTime;
  seekTime = std::min(seekTime, mEndTime);
  seekTime = std::max(mStartTime, seekTime);
  NS_ASSERTION(seekTime >= mStartTime && seekTime <= mEndTime,
               "Can only seek in range [0,duration]");
  mSeekTarget = SeekTarget(seekTime, aTarget.mType);

  mBasePosition = seekTime - mStartTime;
  DECODER_LOG(PR_LOG_DEBUG, "Changed state to SEEKING (to %lld)", mSeekTarget.mTime);
  mState = DECODER_STATE_SEEKING;
  if (mDecoder->GetDecodedStream()) {
    mDecoder->RecreateDecodedStream(seekTime - mStartTime);
  }
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::StopAudioThread()
{
  NS_ASSERTION(OnDecodeThread() ||
               OnStateMachineThread(), "Should be on decode thread or state machine thread");
  AssertCurrentThreadInMonitor();

  if (mStopAudioThread) {
    
    return;
  }

  mStopAudioThread = true;
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mAudioThread) {
    DECODER_LOG(PR_LOG_DEBUG, "Shutdown audio thread");
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mAudioThread->Shutdown();
    }
    mAudioThread = nullptr;
    
    
    SendStreamData();
  }
}

nsresult
MediaDecoderStateMachine::EnqueueDecodeMetadataTask()
{
  AssertCurrentThreadInMonitor();

  if (mState != DECODER_STATE_DECODING_METADATA ||
      mDispatchedDecodeMetadataTask) {
    return NS_OK;
  }
  nsresult rv = mDecodeTaskQueue->Dispatch(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::CallDecodeMetadata));
  if (NS_SUCCEEDED(rv)) {
    mDispatchedDecodeMetadataTask = true;
  } else {
    NS_WARNING("Dispatch ReadMetadata task failed.");
    return rv;
  }

  return NS_OK;
}

void
MediaDecoderStateMachine::SetReaderIdle()
{
#ifdef PR_LOGGING
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    DECODER_LOG(PR_LOG_DEBUG, "SetReaderIdle() audioQueue=%lld videoQueue=%lld",
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

  
  
  
  
  
  
  
  
  
  
  
  

  const bool needToDecodeAudio = NeedToDecodeAudio();
  const bool needToDecodeVideo = NeedToDecodeVideo();

  
  MOZ_ASSERT(mState != DECODER_STATE_COMPLETED ||
             (!needToDecodeAudio && !needToDecodeVideo));

  bool needIdle = !mDecoder->IsLogicallyPlaying() &&
                  mState != DECODER_STATE_SEEKING &&
                  !needToDecodeAudio &&
                  !needToDecodeVideo &&
                  !IsPlaying();

  if (needToDecodeAudio) {
    EnsureAudioDecodeTaskQueued();
  }
  if (needToDecodeVideo) {
    EnsureVideoDecodeTaskQueued();
  }

  if (needIdle) {
    RefPtr<nsIRunnable> event = NS_NewRunnableMethod(
        this, &MediaDecoderStateMachine::SetReaderIdle);
    nsresult rv = mDecodeTaskQueue->Dispatch(event.forget());
    if (NS_FAILED(rv) && mState != DECODER_STATE_SHUTDOWN) {
      NS_WARNING("Failed to dispatch event to set decoder idle state");
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
      mDispatchedDecodeSeekTask) {
    return NS_OK;
  }
  nsresult rv = mDecodeTaskQueue->Dispatch(
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeSeek));
  if (NS_SUCCEEDED(rv)) {
    mDispatchedDecodeSeekTask = true;
  } else {
    NS_WARNING("Dispatch DecodeSeek task failed.");
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
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  if (mState >= DECODER_STATE_COMPLETED) {
    return NS_OK;
  }

  MOZ_ASSERT(mState > DECODER_STATE_DECODING_METADATA);

  if (mIsAudioDecoding && !mDispatchedAudioDecodeTask) {
    nsresult rv = mDecodeTaskQueue->Dispatch(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeAudio));
    if (NS_SUCCEEDED(rv)) {
      mDispatchedAudioDecodeTask = true;
    } else {
      NS_WARNING("Failed to dispatch task to decode audio");
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
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");

  if (mState >= DECODER_STATE_COMPLETED) {
    return NS_OK;
  }

  MOZ_ASSERT(mState > DECODER_STATE_DECODING_METADATA);

  if (mIsVideoDecoding && !mDispatchedVideoDecodeTask) {
    nsresult rv = mDecodeTaskQueue->Dispatch(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeVideo));
    if (NS_SUCCEEDED(rv)) {
      mDispatchedVideoDecodeTask = true;
    } else {
      NS_WARNING("Failed to dispatch task to decode video");
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
  if (HasAudio() && !mAudioThread) {
    nsresult rv = NS_NewNamedThread("Media Audio",
                                    getter_AddRefs(mAudioThread),
                                    nullptr,
                                    MEDIA_THREAD_STACK_SIZE);
    if (NS_FAILED(rv)) {
      DECODER_LOG(PR_LOG_WARNING, "Changed state to SHUTDOWN because failed to create audio thread");
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }

    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AudioLoop);
    mAudioThread->Dispatch(event, NS_DISPATCH_NORMAL);
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
  
  
  
  
  return ((HasAudio() &&
           !AudioQueue().IsFinished() &&
           AudioDecodedUsecs() < aAudioUsecs)
          ||
         (HasVideo() &&
          !VideoQueue().IsFinished() &&
          static_cast<uint32_t>(VideoQueue().GetSize()) < LOW_VIDEO_FRAMES));
}

bool MediaDecoderStateMachine::HasLowUndecodedData()
{
  return HasLowUndecodedData(mLowDataThresholdUsecs);
}

bool MediaDecoderStateMachine::HasLowUndecodedData(double aUsecs)
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "Must have loaded metadata for GetBuffered() to work");

  bool reliable;
  double bytesPerSecond = mDecoder->ComputePlaybackRate(&reliable);
  if (!reliable) {
    
    return false;
  }

  MediaResource* stream = mDecoder->GetResource();
  int64_t currentPos = stream->Tell();
  int64_t requiredPos = currentPos + int64_t((aUsecs/1000000.0)*bytesPerSecond);
  int64_t length = stream->GetLength();
  if (length >= 0) {
    requiredPos = std::min(requiredPos, length);
  }

  return stream->GetCachedDataEnd(currentPos) < requiredPos;
}

void
MediaDecoderStateMachine::DecodeError()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  
  
  DECODER_LOG(PR_LOG_WARNING, "Decode error, changed state to SHUTDOWN");
  ScheduleStateMachine();
  mState = DECODER_STATE_SHUTDOWN;
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
  AutoSetOnScopeExit<bool> unsetOnExit(mDispatchedDecodeMetadataTask, false);
  if (mState != DECODER_STATE_DECODING_METADATA) {
    return;
  }
  if (NS_FAILED(DecodeMetadata())) {
    DECODER_LOG(PR_LOG_WARNING, "Decode metadata failed, shutting down decoder");
    DecodeError();
  }
}

nsresult MediaDecoderStateMachine::DecodeMetadata()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  DECODER_LOG(PR_LOG_DEBUG, "Decoding Media Headers");
  if (mState != DECODER_STATE_DECODING_METADATA) {
    return NS_ERROR_FAILURE;
  }

  nsresult res;
  MediaInfo info;
  MetadataTags* tags;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    res = mReader->ReadMetadata(&info, &tags);
  }
  if (NS_SUCCEEDED(res) &&
      mState == DECODER_STATE_DECODING_METADATA &&
      mReader->IsWaitingMediaResources()) {
    
    StartWaitForResources();
    return NS_OK;
  }

  mInfo = info;

  if (NS_FAILED(res) || (!info.HasValidMedia())) {
    return NS_ERROR_FAILURE;
  }
  mDecoder->StartProgressUpdates();
  mGotDurationFromMetaData = (GetDuration() != -1);

  VideoData* videoData = FindStartTime();
  if (videoData) {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    RenderVideoFrame(videoData, TimeStamp::Now());
  }

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }

  NS_ASSERTION(mStartTime != -1, "Must have start time");
  MOZ_ASSERT((!HasVideo() && !HasAudio()) ||
              !(mMediaSeekable && mTransportSeekable) || mEndTime != -1,
              "Active seekable media should have end time");
  MOZ_ASSERT(!(mMediaSeekable && mTransportSeekable) ||
             GetDuration() != -1, "Seekable media should have duration");
  DECODER_LOG(PR_LOG_DEBUG, "Media goes from %lld to %lld (duration %lld) "
              "transportSeekable=%d, mediaSeekable=%d",
              mStartTime, mEndTime, GetDuration(), mTransportSeekable, mMediaSeekable);

  if (HasAudio() && !HasVideo()) {
    
    
    
    mAmpleAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
    mLowAudioThresholdUsecs /= NO_VIDEO_AMPLE_AUDIO_DIVISOR;
  }

  
  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new AudioMetadataEventRunner(mDecoder,
                                 mInfo.mAudio.mChannels,
                                 mInfo.mAudio.mRate,
                                 HasAudio(),
                                 HasVideo(),
                                 tags);
  NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

  if (HasAudio()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchAudioDecodeTaskIfNeeded));
    AudioQueue().AddPopListener(decodeTask, mDecodeTaskQueue);
  }
  if (HasVideo()) {
    RefPtr<nsIRunnable> decodeTask(
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DispatchVideoDecodeTaskIfNeeded));
    VideoQueue().AddPopListener(decodeTask, mDecodeTaskQueue);
  }

  if (mState == DECODER_STATE_DECODING_METADATA) {
    DECODER_LOG(PR_LOG_DEBUG, "Changed state from DECODING_METADATA to DECODING");
    StartDecoding();
  }

  
  
  
  CheckIfDecodeComplete();

  if ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_COMPLETED) &&
      mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
      !IsPlaying())
  {
    StartPlayback();
  }

  return NS_OK;
}

void MediaDecoderStateMachine::DecodeSeek()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  AutoSetOnScopeExit<bool> unsetOnExit(mDispatchedDecodeSeekTask, false);
  if (mState != DECODER_STATE_SEEKING) {
    return;
  }

  
  
  
  
  
  
  
  
  int64_t seekTime = mSeekTarget.mTime;
  mDecoder->StopProgressUpdates();

  bool currentTimeChanged = false;
  const int64_t mediaTime = GetMediaTime();
  if (mediaTime != seekTime) {
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

  int64_t newCurrentTime = seekTime;
  if (currentTimeChanged) {
    
    
    
    StopAudioThread();
    ResetPlayback();
    nsresult res;
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      
      
      res = mReader->Seek(seekTime,
                          mStartTime,
                          mEndTime,
                          mediaTime);

      if (NS_SUCCEEDED(res) && mSeekTarget.mType == SeekTarget::Accurate) {
        res = mReader->DecodeToTarget(seekTime);
      }
    }

    if (NS_SUCCEEDED(res)) {
      int64_t nextSampleStartTime = 0;
      VideoData* video = nullptr;
      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        video = mReader->FindStartTime(nextSampleStartTime);
      }

      
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
    } else {
      DecodeError();
    }
  }
  mDecoder->StartProgressUpdates();
  if (mState == DECODER_STATE_DECODING_METADATA ||
      mState == DECODER_STATE_DORMANT ||
      mState == DECODER_STATE_SHUTDOWN) {
    return;
  }

  
  
  

  nsCOMPtr<nsIRunnable> stopEvent;
  bool isLiveStream = mDecoder->GetResource()->GetLength() == -1;
  if (GetMediaTime() == mEndTime && !isLiveStream) {
    
    
    
    DECODER_LOG(PR_LOG_DEBUG, "Changed state from SEEKING (to %lld) to COMPLETED", seekTime);
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStoppedAtEnd);
    
    
    mState = DECODER_STATE_COMPLETED;
    mIsAudioDecoding = false;
    mIsVideoDecoding = false;
    DispatchDecodeTasksIfNeeded();
  } else {
    DECODER_LOG(PR_LOG_DEBUG, "Changed state from SEEKING (to %lld) to DECODING", seekTime);
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStopped);
    StartDecoding();
  }

  if (newCurrentTime != mediaTime) {
    UpdatePlaybackPositionInternal(newCurrentTime);
    if (mDecoder->GetDecodedStream()) {
      SetSyncPointForMediaStream();
    }
  }

  
  DECODER_LOG(PR_LOG_DEBUG, "Seek completed, mCurrentFrameTime=%lld", mCurrentFrameTime);

  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
  }

  
  
  
  mQuickBuffering = false;

  ScheduleStateMachine();
}


class nsDecoderDisposeEvent : public nsRunnable {
public:
  nsDecoderDisposeEvent(already_AddRefed<MediaDecoder> aDecoder,
                        already_AddRefed<MediaDecoderStateMachine> aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}
  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
    mStateMachine->ReleaseDecoder();
    mDecoder->ReleaseStateMachine();
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
      
      
      
      
      if (mAudioThread) {
        MOZ_ASSERT(mStopAudioThread);
        return NS_OK;
      }

      
      
      
      AudioQueue().ClearListeners();
      VideoQueue().ClearListeners();

      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        
        mDecodeTaskQueue->Shutdown();
        mDecodeTaskQueue = nullptr;
        mReader->ReleaseMediaResources();
      }
      
      
      mPendingWakeDecoder = nullptr;

      MOZ_ASSERT(mState == DECODER_STATE_SHUTDOWN,
                 "How did we escape from the shutdown state?");
      
      
      
      
      
      
      
      
      
      
      
      
      
      GetStateMachineThread()->Dispatch(
        new nsDispatchDisposeEvent(mDecoder, this), NS_DISPATCH_NORMAL);

      mTimer->Cancel();
      mTimer = nullptr;
      return NS_OK;
    }

    case DECODER_STATE_DORMANT: {
      if (IsPlaying()) {
        StopPlayback();
      }
      StopAudioThread();
      
      
      mPendingWakeDecoder = nullptr;
      {
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        
        mDecodeTaskQueue->AwaitIdle();
        mReader->ReleaseMediaResources();
      }
      return NS_OK;
    }

    case DECODER_STATE_WAIT_FOR_RESOURCES: {
      return NS_OK;
    }

    case DECODER_STATE_DECODING_METADATA: {
      
      return EnqueueDecodeMetadataTask();
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
            !mDecoder->IsDataCachedToEndOfResource() &&
            !resource->IsSuspended())
      {
        DECODER_LOG(PR_LOG_DEBUG, "Buffering: wait %ds, timeout in %.3lfs %s",
                    mBufferingWait, mBufferingWait - elapsed.ToSeconds(),
                    (mQuickBuffering ? "(quick exit)" : ""));
        ScheduleStateMachine(USECS_PER_S);
        return NS_OK;
      } else {
        DECODER_LOG(PR_LOG_DEBUG, "Changed state from BUFFERING to DECODING");
        DECODER_LOG(PR_LOG_DEBUG, "Buffered for %.3lfs", (now - mBufferingStart).ToSeconds());
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
        int64_t videoTime = HasVideo() ? mVideoFrameEndTime : 0;
        int64_t clockTime = std::max(mEndTime, std::max(videoTime, GetAudioClock()));
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

void MediaDecoderStateMachine::RenderVideoFrame(VideoData* aData,
                                                TimeStamp aTarget)
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  mDecoder->GetReentrantMonitor().AssertNotCurrentThreadIn();

  if (aData->mDuplicate) {
    return;
  }

  VERBOSE_LOG("playing video frame %lld", aData->mTime);

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->SetCurrentFrame(ThebesIntSize(aData->mDisplay), aData->mImage,
                               aTarget);
  }
}

int64_t
MediaDecoderStateMachine::GetAudioClock()
{
  
  
  
  AssertCurrentThreadInMonitor();
  if (!HasAudio() || mAudioCaptured)
    return -1;
  if (!mAudioStream) {
    
    return mAudioStartTime;
  }
  int64_t t = mAudioStream->GetPosition();
  return (t == -1) ? -1 : t + mAudioStartTime;
}

int64_t MediaDecoderStateMachine::GetVideoStreamPosition()
{
  AssertCurrentThreadInMonitor();

  if (!IsPlaying()) {
    return mPlayDuration + mStartTime;
  }

  
  if (mResetPlayStartTime) {
    mPlayStartTime = TimeStamp::Now();
    mResetPlayStartTime = false;
  }

  int64_t pos = DurationToUsecs(TimeStamp::Now() - mPlayStartTime) + mPlayDuration;
  pos -= mBasePosition;
  NS_ASSERTION(pos >= 0, "Video stream position should be positive.");
  return mBasePosition + pos * mPlaybackRate + mStartTime;
}

int64_t MediaDecoderStateMachine::GetClock()
{
  AssertCurrentThreadInMonitor();

  
  
  
  
  int64_t clock_time = -1;
  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  if (!IsPlaying()) {
    clock_time = mPlayDuration + mStartTime;
  } else if (stream) {
    clock_time = GetCurrentTimeViaMediaStreamSync();
  } else {
    int64_t audio_time = GetAudioClock();
    if (HasAudio() && !mAudioCompleted && audio_time != -1) {
      clock_time = audio_time;
      
      
      mPlayDuration = clock_time - mStartTime;
      mPlayStartTime = TimeStamp::Now();
    } else {
      
      clock_time = GetVideoStreamPosition();
      
      NS_ASSERTION(mCurrentFrameTime <= clock_time || mPlaybackRate <= 0,
          "Clock should go forwards if the playback rate is > 0.");
    }
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

  int64_t clock_time = GetClock();
  
  
  int64_t remainingTime = AUDIO_DURATION_USECS;
  NS_ASSERTION(clock_time >= mStartTime, "Should have positive clock time.");
  nsAutoPtr<VideoData> currentFrame;
#ifdef PR_LOGGING
  int32_t droppedFrames = 0;
#endif
  if (VideoQueue().GetSize() > 0) {
    VideoData* frame = VideoQueue().PeekFront();
    while (mRealTime || clock_time >= frame->mTime) {
      mVideoFrameEndTime = frame->GetEndTime();
      currentFrame = frame;
#ifdef PR_LOGGING
      VERBOSE_LOG("discarding video frame %lld", frame->mTime);
      if (droppedFrames++) {
        VERBOSE_LOG("discarding video frame %lld (%d so far)", frame->mTime, droppedFrames-1);
      }
#endif
      VideoQueue().PopFront();
      
      
      mDecoder->GetReentrantMonitor().NotifyAll();
      mDecoder->UpdatePlaybackOffset(frame->mOffset);
      if (VideoQueue().GetSize() == 0)
        break;
      frame = VideoQueue().PeekFront();
    }
    
    
    if (frame && !currentFrame) {
      int64_t now = IsPlaying() ? clock_time : mPlayDuration;

      remainingTime = frame->mTime - now;
    }
  }

  
  
  MediaResource* resource = mDecoder->GetResource();
  if (mState == DECODER_STATE_DECODING &&
      mDecoder->GetState() == MediaDecoder::PLAY_STATE_PLAYING &&
      HasLowDecodedData(remainingTime + EXHAUSTED_DATA_MARGIN_USECS) &&
      !mDecoder->IsDataCachedToEndOfResource() &&
      !resource->IsSuspended()) {
    if (JustExitedQuickBuffering() || HasLowUndecodedData()) {
      if (currentFrame) {
        VideoQueue().PushFront(currentFrame.forget());
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
    
    TimeStamp presTime = mPlayStartTime - UsecsToDuration(mPlayDuration) +
                          UsecsToDuration(currentFrame->mTime - mStartTime);
    NS_ASSERTION(currentFrame->mTime >= mStartTime, "Should have positive frame time");
    {
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
    
    clock_time = std::min(clock_time, std::max(mVideoFrameEndTime, mAudioEndTime));
    if (clock_time > GetMediaTime()) {
      
      
      
      
      UpdatePlaybackPosition(clock_time);
    }
  }

  
  
  
  
  UpdateReadyState();

  ScheduleStateMachine(remainingTime);
}

void MediaDecoderStateMachine::Wait(int64_t aUsecs) {
  NS_ASSERTION(OnAudioThread(), "Only call on the audio thread");
  AssertCurrentThreadInMonitor();
  TimeStamp end = TimeStamp::Now() + UsecsToDuration(std::max<int64_t>(USECS_PER_MS, aUsecs));
  TimeStamp now;
  while ((now = TimeStamp::Now()) < end &&
         mState != DECODER_STATE_SHUTDOWN &&
         mState != DECODER_STATE_SEEKING &&
         !mStopAudioThread &&
         IsPlaying())
  {
    int64_t ms = static_cast<int64_t>(NS_round((end - now).ToSeconds() * 1000));
    if (ms == 0 || ms > UINT32_MAX) {
      break;
    }
    mDecoder->GetReentrantMonitor().Wait(PR_MillisecondsToInterval(static_cast<uint32_t>(ms)));
  }
}

VideoData* MediaDecoderStateMachine::FindStartTime()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  AssertCurrentThreadInMonitor();
  int64_t startTime = 0;
  mStartTime = 0;
  VideoData* v = nullptr;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    v = mReader->FindStartTime(startTime);
  }
  if (startTime != 0) {
    mStartTime = startTime;
    if (mGotDurationFromMetaData) {
      NS_ASSERTION(mEndTime != -1,
                   "We should have mEndTime as supplied duration here");
      
      
      
      mEndTime = mStartTime + mEndTime;
    }
  }
  
  
  
  mAudioStartTime = mStartTime;
  DECODER_LOG(PR_LOG_DEBUG, "Media start time is %lld", mStartTime);
  return v;
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
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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

  
  
  
  
  
  
  
  
  
  UpdateReadyState();
  mState = DECODER_STATE_BUFFERING;
  DECODER_LOG(PR_LOG_DEBUG, "Changed state from DECODING to BUFFERING, decoded for %.3lfs",
              decodeDuration.ToSeconds());
#ifdef PR_LOGGING
  MediaDecoder::Statistics stats = mDecoder->GetStatistics();
  DECODER_LOG(PR_LOG_DEBUG, "Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
              stats.mPlaybackRate/1024, stats.mPlaybackRateReliable ? "" : " (unreliable)",
              stats.mDownloadRate/1024, stats.mDownloadRateReliable ? "" : " (unreliable)");
#endif
}

nsresult MediaDecoderStateMachine::GetBuffered(dom::TimeRanges* aBuffered) {
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource, NS_ERROR_FAILURE);
  resource->Pin();
  nsresult res = mReader->GetBuffered(aBuffered, mStartTime);
  resource->Unpin();
  return res;
}

nsresult MediaDecoderStateMachine::CallRunStateMachine()
{
  AssertCurrentThreadInMonitor();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  
  if (mAudioCaptured) {
    StopAudioThread();
  }

  MOZ_ASSERT(!mInRunningStateMachine, "State machine cycles must run in sequence!");
  mTimeout = TimeStamp();
  mInRunningStateMachine = true;
  nsresult res = RunStateMachine();
  mInRunningStateMachine = false;
  return res;
}

nsresult MediaDecoderStateMachine::TimeoutExpired(int aTimerId)
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread(), "Must be on state machine thread");
  mTimer->Cancel();
  if (mTimerId == aTimerId) {
    return CallRunStateMachine();
  } else {
    return NS_OK;
  }
}

void MediaDecoderStateMachine::ScheduleStateMachineWithLockAndWakeDecoder() {
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  DispatchAudioDecodeTaskIfNeeded();
  DispatchVideoDecodeTaskIfNeeded();
}

class TimerEvent : public nsITimerCallback, public nsRunnable {
  NS_DECL_THREADSAFE_ISUPPORTS
public:
  TimerEvent(MediaDecoderStateMachine* aStateMachine, int aTimerId)
    : mStateMachine(aStateMachine), mTimerId(aTimerId) {}

  NS_IMETHOD Run() MOZ_OVERRIDE {
    return mStateMachine->TimeoutExpired(mTimerId);
  }

  NS_IMETHOD Notify(nsITimer* aTimer) {
    return mStateMachine->TimeoutExpired(mTimerId);
  }
private:
  const nsRefPtr<MediaDecoderStateMachine> mStateMachine;
  int mTimerId;
};

NS_IMPL_ISUPPORTS(TimerEvent, nsITimerCallback, nsIRunnable);

nsresult MediaDecoderStateMachine::ScheduleStateMachine(int64_t aUsecs) {
  AssertCurrentThreadInMonitor();
  NS_ABORT_IF_FALSE(GetStateMachineThread(),
    "Must have a state machine thread to schedule");

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }
  aUsecs = std::max<int64_t>(aUsecs, 0);

  TimeStamp timeout = TimeStamp::Now() + UsecsToDuration(aUsecs);
  if (!mTimeout.IsNull() && timeout >= mTimeout) {
    
    
    return NS_OK;
  }

  uint32_t ms = static_cast<uint32_t>((aUsecs / USECS_PER_MS) & 0xFFFFFFFF);
  if (mRealTime && ms > 40) {
    ms = 40;
  }

  
  

  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<TimerEvent> event = new TimerEvent(this, mTimerId+1);

  if (ms == 0) {
    
    
    
    rv = GetStateMachineThread()->Dispatch(event, NS_DISPATCH_NORMAL);
  } else if (OnStateMachineThread()) {
    rv = mTimer->InitWithCallback(event, ms, nsITimer::TYPE_ONE_SHOT);
  } else {
    MOZ_ASSERT(false, "non-zero delay timer should be only scheduled in state machine thread");
  }

  if (NS_SUCCEEDED(rv)) {
    mTimeout = timeout;
    ++mTimerId;
  } else {
    NS_WARNING("Failed to schedule state machine");
  }

  return rv;
}

bool MediaDecoderStateMachine::OnDecodeThread() const
{
  return mDecodeTaskQueue->IsCurrentThreadIn();
}

bool MediaDecoderStateMachine::OnStateMachineThread() const
{
  bool rv = false;
  mStateMachineThreadPool->IsOnCurrentThread(&rv);
  return rv;
}

nsIEventTarget* MediaDecoderStateMachine::GetStateMachineThread()
{
  return mStateMachineThreadPool->GetEventTarget();
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

  
  if (!HasAudio()) {
    
    if (mState == DECODER_STATE_SEEKING) {
      mBasePosition = mSeekTarget.mTime - mStartTime;
    } else {
      mBasePosition = GetVideoStreamPosition();
    }
    mPlayDuration = mBasePosition;
    mResetPlayStartTime = true;
    mPlayStartTime = TimeStamp::Now();
  }

  mPlaybackRate = aPlaybackRate;
}

void MediaDecoderStateMachine::SetPreservesPitch(bool aPreservesPitch)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  mPreservesPitch = aPreservesPitch;
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
                                             int aChannels,
                                             int aRate,
                                             bool aHasAudio,
                                             bool aHasVideo,
                                             MetadataTags* aTags)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  AssertCurrentThreadInMonitor();
  TimedMetadata* metadata = new TimedMetadata;
  metadata->mPublishTime = aPublishTime;
  metadata->mChannels = aChannels;
  metadata->mRate = aRate;
  metadata->mHasAudio = aHasAudio;
  metadata->mHasVideo = aHasVideo;
  metadata->mTags = aTags;
  mMetadataManager.QueueMetadata(metadata);
}

} 


#undef DECODER_LOG
#undef VERBOSE_LOG
