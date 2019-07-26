




#include "mozilla/DebugOnly.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Util.h"

#include "MediaDecoderStateMachine.h"
#include <limits>
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

#include "prenv.h"
#include "mozilla/Preferences.h"
#include <algorithm>

namespace mozilla {

using namespace mozilla::layers;
using namespace mozilla::dom;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif




static const uint32_t BUFFERING_WAIT_S = 30;





static const uint32_t LOW_AUDIO_USECS = 300000;





const int64_t AMPLE_AUDIO_USECS = 1000000;






const uint32_t SILENCE_BYTES_CHUNK = 32 * 1024;




static const uint32_t LOW_VIDEO_FRAMES = 1;


static const int AUDIO_DURATION_USECS = 40000;





static const int THRESHOLD_FACTOR = 2;





static const int64_t LOW_DATA_THRESHOLD_USECS = 5000000;



PR_STATIC_ASSERT(LOW_DATA_THRESHOLD_USECS > AMPLE_AUDIO_USECS);


static const uint32_t EXHAUSTED_DATA_MARGIN_USECS = 60000;










static const uint32_t QUICK_BUFFER_THRESHOLD_USECS = 2000000;



static const uint32_t QUICK_BUFFERING_LOW_DATA_USECS = 1000000;





PR_STATIC_ASSERT(QUICK_BUFFERING_LOW_DATA_USECS <= AMPLE_AUDIO_USECS);


static const uint32_t AUDIOSTREAM_MIN_WRITE_BEFORE_START_USECS = 200000;

static TimeDuration UsecsToDuration(int64_t aUsecs) {
  return TimeDuration::FromMilliseconds(static_cast<double>(aUsecs) / USECS_PER_MS);
}

static int64_t DurationToUsecs(TimeDuration aDuration) {
  return static_cast<int64_t>(aDuration.ToSeconds() * USECS_PER_S);
}




class StateMachineTracker
{
private:
  StateMachineTracker() :
    mMonitor("media.statemachinetracker"),
    mStateMachineCount(0),
    mDecodeThreadCount(0),
    mStateMachineThread(nullptr)
  {
     MOZ_COUNT_CTOR(StateMachineTracker);
     NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  } 
 
  ~StateMachineTracker()
  {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

    MOZ_COUNT_DTOR(StateMachineTracker);
  }

public:
  
  
  
  
  
  static StateMachineTracker& Instance();
 
  
  
  void EnsureGlobalStateMachine();

  
  
  void CleanupGlobalStateMachine();

  
  nsIThread* GetGlobalStateMachineThread()
  {
    ReentrantMonitorAutoEnter mon(mMonitor);
    NS_ASSERTION(mStateMachineThread, "Should have non-null state machine thread!");
    return mStateMachineThread;
  }

  
  
  
  
  
  nsresult RequestCreateDecodeThread(MediaDecoderStateMachine* aStateMachine);

  
  
  nsresult CancelCreateDecodeThread(MediaDecoderStateMachine* aStateMachine);

  
  
  static const uint32_t MAX_DECODE_THREADS = 25;

  
  
  
  uint32_t GetDecodeThreadCount();

  
  
  
  void NoteDecodeThreadDestroyed();

#ifdef DEBUG
  
  
  bool IsQueued(MediaDecoderStateMachine* aStateMachine);
#endif

private:
  
  
  static StateMachineTracker* sInstance;

  
  
  ReentrantMonitor mMonitor;

  
  
  
  uint32_t mStateMachineCount;

  
  
  
  uint32_t mDecodeThreadCount;

  
  
  
  nsIThread* mStateMachineThread;

  
  
  nsDeque mPending;
};

StateMachineTracker* StateMachineTracker::sInstance = nullptr;

StateMachineTracker& StateMachineTracker::Instance()
{
  if (!sInstance) {
    NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
    sInstance = new StateMachineTracker();
  }
  return *sInstance;
}

void StateMachineTracker::EnsureGlobalStateMachine() 
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mMonitor);
  if (mStateMachineCount == 0) {
    NS_ASSERTION(!mStateMachineThread, "Should have null state machine thread!");
    DebugOnly<nsresult> rv = NS_NewNamedThread("Media State", &mStateMachineThread, nullptr);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "Can't create media state machine thread");
  }
  mStateMachineCount++;
}

#ifdef DEBUG
bool StateMachineTracker::IsQueued(MediaDecoderStateMachine* aStateMachine)
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  int32_t size = mPending.GetSize();
  for (int i = 0; i < size; ++i) {
    MediaDecoderStateMachine* m =
      static_cast<MediaDecoderStateMachine*>(mPending.ObjectAt(i));
    if (m == aStateMachine) {
      return true;
    }
  }
  return false;
}
#endif

void StateMachineTracker::CleanupGlobalStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ABORT_IF_FALSE(mStateMachineCount > 0,
    "State machine ref count must be > 0");
  mStateMachineCount--;
  if (mStateMachineCount == 0) {
    LOG(PR_LOG_DEBUG, ("Destroying media state machine thread"));
    NS_ASSERTION(mPending.GetSize() == 0, "Shouldn't all requests be handled by now?");
    {
      ReentrantMonitorAutoEnter mon(mMonitor);
      nsCOMPtr<nsIRunnable> event = new ShutdownThreadEvent(mStateMachineThread);
      NS_RELEASE(mStateMachineThread);
      mStateMachineThread = nullptr;
      NS_DispatchToMainThread(event);

      NS_ASSERTION(mDecodeThreadCount == 0, "Decode thread count must be zero.");
      sInstance = nullptr;
    }
    delete this;
  }
}

void StateMachineTracker::NoteDecodeThreadDestroyed()
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  --mDecodeThreadCount;
  while (mDecodeThreadCount < MAX_DECODE_THREADS && mPending.GetSize() > 0) {
    MediaDecoderStateMachine* m =
      static_cast<MediaDecoderStateMachine*>(mPending.PopFront());
    nsresult rv;
    {
      ReentrantMonitorAutoExit exitMon(mMonitor);
      rv = m->StartDecodeThread();
    }
    if (NS_SUCCEEDED(rv)) {
      ++mDecodeThreadCount;
    }
  }
}

uint32_t StateMachineTracker::GetDecodeThreadCount()
{
  ReentrantMonitorAutoEnter mon(mMonitor);
  return mDecodeThreadCount;
}

nsresult StateMachineTracker::CancelCreateDecodeThread(MediaDecoderStateMachine* aStateMachine) {
  ReentrantMonitorAutoEnter mon(mMonitor);
  int32_t size = mPending.GetSize();
  for (int32_t i = 0; i < size; ++i) {
    void* m = static_cast<MediaDecoderStateMachine*>(mPending.ObjectAt(i));
    if (m == aStateMachine) {
      mPending.RemoveObjectAt(i);
      break;
    }
  }
  NS_ASSERTION(!IsQueued(aStateMachine), "State machine should no longer have queued request.");
  return NS_OK;
}

nsresult StateMachineTracker::RequestCreateDecodeThread(MediaDecoderStateMachine* aStateMachine)
{
  NS_ENSURE_STATE(aStateMachine);
  ReentrantMonitorAutoEnter mon(mMonitor);
  if (mPending.GetSize() > 0 || mDecodeThreadCount + 1 >= MAX_DECODE_THREADS) {
    
    
    
    
    mPending.Push(aStateMachine);
    return NS_OK;
  }
  nsresult rv;
  {
    ReentrantMonitorAutoExit exitMon(mMonitor);
    rv = aStateMachine->StartDecodeThread();
  }
  if (NS_SUCCEEDED(rv)) {
    ++mDecodeThreadCount;
  }
  NS_ASSERTION(mDecodeThreadCount <= MAX_DECODE_THREADS,
                "Should keep to thread limit!");
  return NS_OK;
}

MediaDecoderStateMachine::MediaDecoderStateMachine(MediaDecoder* aDecoder,
                                                   MediaDecoderReader* aReader,
                                                   bool aRealTime) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mResetPlayStartTime(false),
  mPlayDuration(0),
  mStartTime(-1),
  mEndTime(-1),
  mSeekTime(0),
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
  mAudioCaptured(false),
  mTransportSeekable(true),
  mMediaSeekable(true),
  mPositionChangeQueued(false),
  mAudioCompleted(false),
  mGotDurationFromMetaData(false),
  mStopDecodeThread(true),
  mDecodeThreadIdle(false),
  mStopAudioThread(true),
  mQuickBuffering(false),
  mIsRunning(false),
  mRunAgain(false),
  mDispatchedRunEvent(false),
  mDecodeThreadWaiting(false),
  mRealTime(aRealTime),
  mDidThrottleAudioDecoding(false),
  mDidThrottleVideoDecoding(false),
  mRequestedNewDecodeThread(false),
  mEventManager(aDecoder),
  mLastFrameStatus(MediaDecoderOwner::NEXT_FRAME_UNINITIALIZED)
{
  MOZ_COUNT_CTOR(MediaDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  StateMachineTracker::Instance().EnsureGlobalStateMachine();

  
  if (Preferences::GetBool("media.realtime_decoder.enabled", false) == false)
    mRealTime = false;

  mBufferingWait = mRealTime ? 0 : BUFFERING_WAIT_S;
  mLowDataThresholdUsecs = mRealTime ? 0 : LOW_DATA_THRESHOLD_USECS;

  
  
  
#if defined(MOZ_WIDGET_GONK) || defined(MOZ_MEDIA_PLUGINS)
  
  
  
  
  
  mAmpleVideoFrames = Preferences::GetUint("media.video-queue.default-size", 3);
#else
  mAmpleVideoFrames = Preferences::GetUint("media.video-queue.default-size", 10);
#endif
  if (mAmpleVideoFrames < 2) {
    mAmpleVideoFrames = 2;
  }
}

MediaDecoderStateMachine::~MediaDecoderStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MOZ_COUNT_DTOR(MediaDecoderStateMachine);
  NS_ASSERTION(!mPendingWakeDecoder.get(),
               "WakeDecoder should have been revoked already");
  NS_ASSERTION(!StateMachineTracker::Instance().IsQueued(this),
    "Should not have a pending request for a new decode thread");
  NS_ASSERTION(!mRequestedNewDecodeThread,
    "Should not have (or flagged) a pending request for a new decode thread");
  if (mTimer)
    mTimer->Cancel();
  mTimer = nullptr;
  mReader = nullptr;
 
  StateMachineTracker::Instance().CleanupGlobalStateMachine();
}

bool MediaDecoderStateMachine::HasFutureAudio() const {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedUsecs() > LOW_AUDIO_USECS * mPlaybackRate || mReader->AudioQueue().IsFinished());
}

bool MediaDecoderStateMachine::HaveNextFrameData() const {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  return (!HasAudio() || HasFutureAudio()) &&
         (!HasVideo() || mReader->VideoQueue().GetSize() > 0);
}

int64_t MediaDecoderStateMachine::GetDecodedAudioDuration() {
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  int64_t audioDecoded = mReader->AudioQueue().Duration();
  if (mAudioEndTime != -1) {
    audioDecoded += mAudioEndTime - GetMediaTime();
  }
  return audioDecoded;
}

void MediaDecoderStateMachine::DecodeThreadRun()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mReader->OnDecodeThreadStart();
  
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

    if (mState == DECODER_STATE_DECODING_METADATA &&
        NS_FAILED(DecodeMetadata())) {
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "Should be in shutdown state if metadata loading fails.");
      LOG(PR_LOG_DEBUG, ("Decode metadata failed, shutting down decode thread"));
    }

    while (mState != DECODER_STATE_SHUTDOWN &&
           mState != DECODER_STATE_COMPLETED &&
           !mStopDecodeThread)
    {
      if (mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING) {
        DecodeLoop();
      } else if (mState == DECODER_STATE_SEEKING) {
        DecodeSeek();
      }
    }

    mDecodeThreadIdle = true;
    LOG(PR_LOG_DEBUG, ("%p Decode thread finished", mDecoder.get()));
  }
  
  mReader->OnDecodeThreadFinish();
}

void MediaDecoderStateMachine::SendStreamAudio(AudioData* aAudio,
                                               DecodedStreamData* aStream,
                                               AudioSegment* aOutput)
{
  NS_ASSERTION(OnDecodeThread() ||
               OnStateMachineThread(), "Should be on decode thread or state machine thread");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (aAudio->mTime <= aStream->mLastAudioPacketTime) {
    
    return;
  }
  aStream->mLastAudioPacketTime = aAudio->mTime;
  aStream->mLastAudioPacketEndTime = aAudio->GetEnd();

  
  
  CheckedInt64 audioWrittenOffset = UsecsToFrames(mInfo.mAudioRate,
      aStream->mInitialTime + mStartTime) + aStream->mAudioFramesWritten;
  CheckedInt64 frameOffset = UsecsToFrames(mInfo.mAudioRate, aAudio->mTime);
  if (!audioWrittenOffset.isValid() || !frameOffset.isValid())
    return;
  if (audioWrittenOffset.value() < frameOffset.value()) {
    
    LOG(PR_LOG_DEBUG, ("%p Decoder writing %d frames of silence to MediaStream",
                       mDecoder.get(), int32_t(frameOffset.value() - audioWrittenOffset.value())));
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
  LOG(PR_LOG_DEBUG, ("%p Decoder writing %d frames of data to MediaStream for AudioData at %lld",
                     mDecoder.get(), aAudio->mFrames - int32_t(offset), aAudio->mTime));
  aStream->mAudioFramesWritten += aAudio->mFrames - int32_t(offset);
}

static void WriteVideoToMediaStream(layers::Image* aImage,
                                    int64_t aDuration, const gfxIntSize& aIntrinsicSize,
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  DecodedStreamData* stream = mDecoder->GetDecodedStream();
  if (!stream)
    return;

  if (mState == DECODER_STATE_DECODING_METADATA)
    return;

  
  
  
  
  if (mAudioThread)
    return;

  int64_t minLastAudioPacketTime = INT64_MAX;
  SourceMediaStream* mediaStream = stream->mStream;
  StreamTime endPosition = 0;

  if (!stream->mStreamInitialized) {
    if (mInfo.mHasAudio) {
      AudioSegment* audio = new AudioSegment();
      mediaStream->AddTrack(TRACK_AUDIO, mInfo.mAudioRate, 0, audio);
    }
    if (mInfo.mHasVideo) {
      VideoSegment* video = new VideoSegment();
      mediaStream->AddTrack(TRACK_VIDEO, RATE_VIDEO, 0, video);
    }
    stream->mStreamInitialized = true;
  }

  if (mInfo.mHasAudio) {
    nsAutoTArray<AudioData*,10> audio;
    
    
    mReader->AudioQueue().GetElementsAfter(stream->mLastAudioPacketTime, &audio);
    AudioSegment output;
    for (uint32_t i = 0; i < audio.Length(); ++i) {
      SendStreamAudio(audio[i], stream, &output);
    }
    if (output.GetDuration() > 0) {
      mediaStream->AppendToTrack(TRACK_AUDIO, &output);
    }
    if (mReader->AudioQueue().IsFinished() && !stream->mHaveSentFinishAudio) {
      mediaStream->EndTrack(TRACK_AUDIO);
      stream->mHaveSentFinishAudio = true;
    }
    minLastAudioPacketTime = std::min(minLastAudioPacketTime, stream->mLastAudioPacketTime);
    endPosition = std::max(endPosition,
        TicksToTimeRoundDown(mInfo.mAudioRate, stream->mAudioFramesWritten));
  }

  if (mInfo.mHasVideo) {
    nsAutoTArray<VideoData*,10> video;
    
    
    mReader->VideoQueue().GetElementsAfter(stream->mNextVideoTime + mStartTime, &video);
    VideoSegment output;
    for (uint32_t i = 0; i < video.Length(); ++i) {
      VideoData* v = video[i];
      if (stream->mNextVideoTime + mStartTime < v->mTime) {
        LOG(PR_LOG_DEBUG, ("%p Decoder writing last video to MediaStream %p for %lld ms",
                           mDecoder.get(), mediaStream,
                           v->mTime - (stream->mNextVideoTime + mStartTime)));
        
        
        WriteVideoToMediaStream(stream->mLastVideoImage,
          v->mTime - (stream->mNextVideoTime + mStartTime), stream->mLastVideoImageDisplaySize,
            &output);
        stream->mNextVideoTime = v->mTime - mStartTime;
      }
      if (stream->mNextVideoTime + mStartTime < v->mEndTime) {
        LOG(PR_LOG_DEBUG, ("%p Decoder writing video frame %lld to MediaStream %p for %lld ms",
                           mDecoder.get(), v->mTime, mediaStream,
                           v->mEndTime - (stream->mNextVideoTime + mStartTime)));
        WriteVideoToMediaStream(v->mImage,
            v->mEndTime - (stream->mNextVideoTime + mStartTime), v->mDisplay,
            &output);
        stream->mNextVideoTime = v->mEndTime - mStartTime;
        stream->mLastVideoImage = v->mImage;
        stream->mLastVideoImageDisplaySize = v->mDisplay;
      } else {
        LOG(PR_LOG_DEBUG, ("%p Decoder skipping writing video frame %lld to MediaStream",
                           mDecoder.get(), v->mTime));
      }
    }
    if (output.GetDuration() > 0) {
      mediaStream->AppendToTrack(TRACK_VIDEO, &output);
    }
    if (mReader->VideoQueue().IsFinished() && !stream->mHaveSentFinishVideo) {
      mediaStream->EndTrack(TRACK_VIDEO);
      stream->mHaveSentFinishVideo = true;
    }
    endPosition = std::max(endPosition,
        TicksToTimeRoundDown(RATE_VIDEO, stream->mNextVideoTime - stream->mInitialTime));
  }

  if (!stream->mHaveSentFinish) {
    stream->mStream->AdvanceKnownTracksTime(endPosition);
  }

  bool finished =
      (!mInfo.mHasAudio || mReader->AudioQueue().IsFinished()) &&
      (!mInfo.mHasVideo || mReader->VideoQueue().IsFinished());
  if (finished && !stream->mHaveSentFinish) {
    stream->mHaveSentFinish = true;
    stream->mStream->Finish();
  }

  if (mAudioCaptured) {
    
    int64_t audioPacketTimeToDiscard =
        std::min(minLastAudioPacketTime, mStartTime + mCurrentFrameTime);
    while (true) {
      nsAutoPtr<AudioData> a(mReader->AudioQueue().PopFront());
      if (!a)
        break;
      
      
      
      
      
      
      if (a->GetEnd() >= audioPacketTimeToDiscard) {
        mReader->AudioQueue().PushFront(a.forget());
        break;
      }
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (!mPendingWakeDecoder.get()) {
    mPendingWakeDecoder = new WakeDecoderRunnable(this);
  }
  return mPendingWakeDecoder.get();
}

bool MediaDecoderStateMachine::HaveEnoughDecodedAudio(int64_t aAmpleAudioUSecs)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (mReader->AudioQueue().GetSize() == 0 ||
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (static_cast<uint32_t>(mReader->VideoQueue().GetSize()) < GetAmpleVideoFrames() * mPlaybackRate) {
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

void MediaDecoderStateMachine::DecodeLoop()
{
  LOG(PR_LOG_DEBUG, ("%p Start DecodeLoop()", mDecoder.get()));

  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  
  bool audioPump = true;
  bool videoPump = true;

  
  
  
  
  bool skipToNextKeyframe = false;

  
  
  const unsigned videoPumpThreshold = mRealTime ? 0 : GetAmpleVideoFrames() / 2;

  
  
  
  const unsigned audioPumpThreshold = mRealTime ? 0 : LOW_AUDIO_USECS * 2;

  
  
  int64_t lowAudioThreshold = LOW_AUDIO_USECS;

  
  
  
  int64_t ampleAudioThreshold = AMPLE_AUDIO_USECS;

  
  bool videoPlaying = HasVideo();
  bool audioPlaying = HasAudio();
  while ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING) &&
         !mStopDecodeThread &&
         (videoPlaying || audioPlaying))
  {
#ifdef MOZ_DASH
    mReader->PrepareToDecode();
#endif

    
    
    
    if (videoPump &&
        (static_cast<uint32_t>(mReader->VideoQueue().GetSize())
         >= videoPumpThreshold * mPlaybackRate))
    {
      videoPump = false;
    }

    
    
    
    if (audioPump && GetDecodedAudioDuration() >= audioPumpThreshold * mPlaybackRate) {
      audioPump = false;
    }

    
    
    
    
    
    
    if (mState == DECODER_STATE_DECODING &&
        !skipToNextKeyframe &&
        videoPlaying &&
        ((!audioPump && audioPlaying && !mDidThrottleAudioDecoding &&
          GetDecodedAudioDuration() < lowAudioThreshold * mPlaybackRate) ||
         (!videoPump && videoPlaying && !mDidThrottleVideoDecoding &&
          (static_cast<uint32_t>(mReader->VideoQueue().GetSize())
           < LOW_VIDEO_FRAMES * mPlaybackRate))) &&
        !HasLowUndecodedData())
    {
      skipToNextKeyframe = true;
      LOG(PR_LOG_DEBUG, ("%p Skipping video decode to the next keyframe", mDecoder.get()));
    }

    
    bool throttleVideoDecoding = !videoPlaying || HaveEnoughDecodedVideo();
    if (mDidThrottleVideoDecoding && !throttleVideoDecoding) {
      videoPump = true;
    }
    mDidThrottleVideoDecoding = throttleVideoDecoding;
    if (!throttleVideoDecoding)
    {
      
      
      
      TimeDuration decodeTime;
      {
        int64_t currentTime = GetMediaTime();
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        TimeStamp start = TimeStamp::Now();
        videoPlaying = mReader->DecodeVideoFrame(skipToNextKeyframe, currentTime);
        decodeTime = TimeStamp::Now() - start;
      }
      if (THRESHOLD_FACTOR * DurationToUsecs(decodeTime) > lowAudioThreshold &&
          !HasLowUndecodedData())
      {
        lowAudioThreshold =
          std::min(THRESHOLD_FACTOR * DurationToUsecs(decodeTime), AMPLE_AUDIO_USECS);
        ampleAudioThreshold = std::max(THRESHOLD_FACTOR * lowAudioThreshold,
                                     ampleAudioThreshold);
        LOG(PR_LOG_DEBUG,
            ("Slow video decode, set lowAudioThreshold=%lld ampleAudioThreshold=%lld",
             lowAudioThreshold, ampleAudioThreshold));
      }
    }

    
    bool throttleAudioDecoding = !audioPlaying || HaveEnoughDecodedAudio(ampleAudioThreshold * mPlaybackRate);
    if (mDidThrottleAudioDecoding && !throttleAudioDecoding) {
      audioPump = true;
    }
    mDidThrottleAudioDecoding = throttleAudioDecoding;
    if (!mDidThrottleAudioDecoding) {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      audioPlaying = mReader->DecodeAudioData();
    }

    SendStreamData();

    
    
    mDecoder->GetReentrantMonitor().NotifyAll();

    
    
    UpdateReadyState();

    if ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING) &&
        !mStopDecodeThread &&
        (videoPlaying || audioPlaying) &&
        throttleAudioDecoding && throttleVideoDecoding)
    {
      
      
      
      
      
      
      
      
      
      mDecodeThreadWaiting = true;
      if (mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING) {
        
        
        
        
        ScheduleStateMachine();
      }
      mDecoder->GetReentrantMonitor().Wait();
      mDecodeThreadWaiting = false;
    }

  } 

  if (!mStopDecodeThread &&
      mState != DECODER_STATE_SHUTDOWN &&
      mState != DECODER_STATE_SEEKING)
  {
    mState = DECODER_STATE_COMPLETED;
    ScheduleStateMachine();
  }

  LOG(PR_LOG_DEBUG, ("%p Exiting DecodeLoop", mDecoder.get()));
}

bool MediaDecoderStateMachine::IsPlaying()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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
  LOG(PR_LOG_DEBUG, ("%p Begun audio thread/loop", mDecoder.get()));
  int64_t audioDuration = 0;
  int64_t audioStartTime = -1;
  uint32_t channels, rate;
  double volume = -1;
  bool setVolume;
  double playbackRate = -1;
  bool setPlaybackRate;
  bool preservesPitch;
  bool setPreservesPitch;
  AudioChannelType audioChannelType;

  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioCompleted = false;
    audioStartTime = mAudioStartTime;
    NS_ASSERTION(audioStartTime != -1, "Should have audio start time by now");
    channels = mInfo.mAudioChannels;
    rate = mInfo.mAudioRate;

    audioChannelType = mDecoder->GetAudioChannelType();
    volume = mVolume;
    preservesPitch = mPreservesPitch;
    playbackRate = mPlaybackRate;
  }

  {
    
    
    
    nsAutoPtr<AudioStream> audioStream(AudioStream::AllocateStream());
    audioStream->Init(channels, rate, audioChannelType);
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
      mAudioStream = audioStream;
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
              (mReader->AudioQueue().GetSize() == 0 &&
               !mReader->AudioQueue().AtEndOfStream())))
      {
        if (!IsPlaying() && !mAudioStream->IsPaused()) {
          mAudioStream->Pause();
        }
        mon.Wait();
      }

      
      
      if (mState == DECODER_STATE_SHUTDOWN ||
          mStopAudioThread ||
          mReader->AudioQueue().AtEndOfStream())
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
    NS_ASSERTION(mReader->AudioQueue().GetSize() > 0,
                 "Should have data to play");
    
    
    const AudioData* s = mReader->AudioQueue().PeekFront();

    
    
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
      LOG(PR_LOG_DEBUG, ("%p Decoder playing %d frames of silence",
                         mDecoder.get(), int32_t(missingFrames.value())));
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
    if (mReader->AudioQueue().AtEndOfStream() &&
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
        
        mEventManager.Drain(mAudioEndTime);
      }
    }
  }
  LOG(PR_LOG_DEBUG, ("%p Reached audio stream end.", mDecoder.get()));
  {
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioStream->Shutdown();
    mAudioStream = nullptr;
    mEventManager.Clear();
    if (!mAudioCaptured) {
      mAudioCompleted = true;
      UpdateReadyState();
      
      mDecoder->GetReentrantMonitor().NotifyAll();
    }
  }

  LOG(PR_LOG_DEBUG, ("%p Audio stream finished playing, audio thread exit", mDecoder.get()));
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
  
  mEventManager.QueueWrittenAudioData(nullptr, frames * aChannels,
                                      (aFrameOffset + frames) * aChannels);
  return frames;
}

uint32_t MediaDecoderStateMachine::PlayFromAudioQueue(uint64_t aFrameOffset,
                                                      uint32_t aChannels)
{
  NS_ASSERTION(OnAudioThread(), "Only call on audio thread.");
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  nsAutoPtr<AudioData> audio(mReader->AudioQueue().PopFront());
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    NS_WARN_IF_FALSE(IsPlaying(), "Should be playing");
    
    
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
  int64_t offset = -1;
  uint32_t frames = 0;
  if (!PR_GetEnv("MOZ_QUIET")) {
    LOG(PR_LOG_DEBUG, ("%p Decoder playing %d frames of data to stream for AudioData at %lld",
                       mDecoder.get(), audio->mFrames, audio->mTime));
  }
  mAudioStream->Write(audio->mAudioData,
                      audio->mFrames);

  StartAudioStreamPlaybackIfNeeded(mAudioStream);

  offset = audio->mOffset;
  frames = audio->mFrames;

  
  mEventManager.QueueWrittenAudioData(audio->mAudioData.get(),
                                      audio->mFrames * aChannels,
                                      (aFrameOffset + frames) * aChannels);
  if (offset != -1) {
    mDecoder->UpdatePlaybackOffset(offset);
  }
  return frames;
}

nsresult MediaDecoderStateMachine::Init(MediaDecoderStateMachine* aCloneDonor)
{
  MediaDecoderReader* cloneReader = nullptr;
  if (aCloneDonor) {
    cloneReader = static_cast<MediaDecoderStateMachine*>(aCloneDonor)->mReader;
  }
  return mReader->Init(cloneReader);
}

void MediaDecoderStateMachine::StopPlayback()
{
  LOG(PR_LOG_DEBUG, ("%p StopPlayback()", mDecoder.get()));

  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine thread or the decoder thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mDecoder->NotifyPlaybackStopped();

  if (IsPlaying()) {
    mPlayDuration += DurationToUsecs(TimeStamp::Now() - mPlayStartTime);
    mPlayStartTime = TimeStamp();
  }
  
  
  mDecoder->GetReentrantMonitor().NotifyAll();
  NS_ASSERTION(!IsPlaying(), "Should report not playing at end of StopPlayback()");
}

void MediaDecoderStateMachine::StartPlayback()
{
  LOG(PR_LOG_DEBUG, ("%p StartPlayback()", mDecoder.get()));

  NS_ASSERTION(!IsPlaying(), "Shouldn't be playing when StartPlayback() is called");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mDecoder->NotifyPlaybackStarted();
  mPlayStartTime = TimeStamp::Now();

  NS_ASSERTION(IsPlaying(), "Should report playing by end of StartPlayback()");
  if (NS_FAILED(StartAudioThread())) {
    NS_WARNING("Failed to create audio thread"); 
  }
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void MediaDecoderStateMachine::UpdatePlaybackPositionInternal(int64_t aTime)
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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

  
  mEventManager.DispatchPendingEvents(GetMediaTime());

  mMetadataManager.DispatchMetadataIfNeeded(mDecoder, aTime);

  if (fragmentEnded) {
    StopPlayback();
  }
}

void MediaDecoderStateMachine::ClearPositionChangeFlag()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

void MediaDecoderStateMachine::SetDuration(int64_t aDuration)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
               "Should be on main or decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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

void MediaDecoderStateMachine::SetMediaEndTime(int64_t aEndTime)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mEndTime = aEndTime;
}

void MediaDecoderStateMachine::SetFragmentEndTime(int64_t aEndTime)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mFragmentEndTime = aEndTime < 0 ? aEndTime : aEndTime + mStartTime;
}

void MediaDecoderStateMachine::SetTransportSeekable(bool aTransportSeekable)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
      "Should be on main thread or the decoder thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mTransportSeekable = aTransportSeekable;
}

void MediaDecoderStateMachine::SetMediaSeekable(bool aMediaSeekable)
{
  NS_ASSERTION(NS_IsMainThread() || OnDecodeThread(),
      "Should be on main thread or the decoder thread.");

  mMediaSeekable = aMediaSeekable;
}

void MediaDecoderStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  LOG(PR_LOG_DEBUG, ("%p Changed state to SHUTDOWN", mDecoder.get()));
  ScheduleStateMachine();
  mState = DECODER_STATE_SHUTDOWN;
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void MediaDecoderStateMachine::StartDecoding()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState != DECODER_STATE_DECODING) {
    mDecodeStartTime = TimeStamp::Now();
  }
  mState = DECODER_STATE_DECODING;
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder.get()));
    mState = DECODER_STATE_DECODING;
    mDecodeStartTime = TimeStamp::Now();
  }
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

  
  
  
  
  
  TimeRanges buffered;
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

void MediaDecoderStateMachine::Seek(double aTime)
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
  double t = aTime * static_cast<double>(USECS_PER_S);
  if (t > INT64_MAX) {
    
    return;
  }

  mSeekTime = static_cast<int64_t>(t) + mStartTime;
  NS_ASSERTION(mSeekTime >= mStartTime && mSeekTime <= mEndTime,
               "Can only seek in range [0,duration]");

  
  NS_ASSERTION(mStartTime != -1, "Should know start time by now");
  NS_ASSERTION(mEndTime != -1, "Should know end time by now");
  mSeekTime = std::min(mSeekTime, mEndTime);
  mSeekTime = std::max(mStartTime, mSeekTime);
  mBasePosition = mSeekTime - mStartTime;
  LOG(PR_LOG_DEBUG, ("%p Changed state to SEEKING (to %f)", mDecoder.get(), aTime));
  mState = DECODER_STATE_SEEKING;
  if (mDecoder->GetDecodedStream()) {
    mDecoder->RecreateDecodedStream(mSeekTime - mStartTime);
  }
  ScheduleStateMachine();
}

void MediaDecoderStateMachine::StopDecodeThread()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  if (mRequestedNewDecodeThread) {
    
    
    NS_ASSERTION(!mDecodeThread,
      "Shouldn't have a decode thread until after request processed");
    StateMachineTracker::Instance().CancelCreateDecodeThread(this);
    mRequestedNewDecodeThread = false;
  }
  mStopDecodeThread = true;
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mDecodeThread) {
    LOG(PR_LOG_DEBUG, ("%p Shutdown decode thread", mDecoder.get()));
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mDecodeThread->Shutdown();
      StateMachineTracker::Instance().NoteDecodeThreadDestroyed();
    }
    mDecodeThread = nullptr;
    mDecodeThreadIdle = false;
  }
  NS_ASSERTION(!mRequestedNewDecodeThread,
    "Any pending requests for decode threads must be canceled and unflagged");
  NS_ASSERTION(!StateMachineTracker::Instance().IsQueued(this),
    "Any pending requests for decode threads must be canceled");
}

void MediaDecoderStateMachine::StopAudioThread()
{
  NS_ASSERTION(OnDecodeThread() ||
               OnStateMachineThread(), "Should be on decode thread or state machine thread");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (mStopAudioThread) {
    
    return;
  }

  mStopAudioThread = true;
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mAudioThread) {
    LOG(PR_LOG_DEBUG, ("%p Shutdown audio thread", mDecoder.get()));
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mAudioThread->Shutdown();
    }
    mAudioThread = nullptr;
    
    
    SendStreamData();
  }
}

nsresult
MediaDecoderStateMachine::ScheduleDecodeThread()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
 
  mStopDecodeThread = false;
  if (mState >= DECODER_STATE_COMPLETED) {
    return NS_OK;
  }
  if (mDecodeThread) {
    NS_ASSERTION(!mRequestedNewDecodeThread,
      "Shouldn't have requested new decode thread when we have a decode thread");
    
    if (mDecodeThreadIdle) {
      
      nsCOMPtr<nsIRunnable> event =
        NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeThreadRun);
      mDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
      mDecodeThreadIdle = false;
    }
    return NS_OK;
  } else if (!mRequestedNewDecodeThread) {
  
    mRequestedNewDecodeThread = true;
    ReentrantMonitorAutoExit mon(mDecoder->GetReentrantMonitor());
    StateMachineTracker::Instance().RequestCreateDecodeThread(this);
  }
  return NS_OK;
}

nsresult
MediaDecoderStateMachine::StartDecodeThread()
{
  NS_ASSERTION(StateMachineTracker::Instance().GetDecodeThreadCount() <
               StateMachineTracker::MAX_DECODE_THREADS,
               "Should not have reached decode thread limit");

  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(!StateMachineTracker::Instance().IsQueued(this),
    "Should not already have a pending request for a new decode thread.");
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  NS_ASSERTION(!mDecodeThread, "Should not have decode thread yet");
  NS_ASSERTION(mRequestedNewDecodeThread, "Should have requested this...");

  mRequestedNewDecodeThread = false;

  nsresult rv = NS_NewNamedThread("Media Decode",
                                  getter_AddRefs(mDecodeThread),
                                  nullptr,
                                  MEDIA_THREAD_STACK_SIZE);
  if (NS_FAILED(rv)) {
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    return rv;
  }

  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &MediaDecoderStateMachine::DecodeThreadRun);
  mDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
  mDecodeThreadIdle = false;

  return NS_OK;
}

nsresult
MediaDecoderStateMachine::StartAudioThread()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
      LOG(PR_LOG_DEBUG, ("%p Changed state to SHUTDOWN because failed to create audio thread", mDecoder.get()));
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }

    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &MediaDecoderStateMachine::AudioLoop);
    mAudioThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}

int64_t MediaDecoderStateMachine::AudioDecodedUsecs() const
{
  NS_ASSERTION(HasAudio(),
               "Should only call AudioDecodedUsecs() when we have audio");
  
  
  
  int64_t pushed = (mAudioEndTime != -1) ? (mAudioEndTime - GetMediaTime()) : 0;
  return pushed + mReader->AudioQueue().Duration();
}

bool MediaDecoderStateMachine::HasLowDecodedData(int64_t aAudioUsecs) const
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  
  
  
  
  return ((HasAudio() &&
           !mReader->AudioQueue().IsFinished() &&
           AudioDecodedUsecs() < aAudioUsecs)
          ||
         (!HasAudio() &&
          HasVideo() &&
          !mReader->VideoQueue().IsFinished() &&
          static_cast<uint32_t>(mReader->VideoQueue().GetSize()) < LOW_VIDEO_FRAMES));
}

bool MediaDecoderStateMachine::HasLowUndecodedData() const
{
  return GetUndecodedData() < mLowDataThresholdUsecs;
}

int64_t MediaDecoderStateMachine::GetUndecodedData() const
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "Must have loaded metadata for GetBuffered() to work");
  TimeRanges buffered;
  
  nsresult res = mDecoder->GetBuffered(&buffered);
  NS_ENSURE_SUCCESS(res, 0);
  double currentTime = GetCurrentTime();

  nsIDOMTimeRanges* r = static_cast<nsIDOMTimeRanges*>(&buffered);
  uint32_t length = 0;
  res = r->GetLength(&length);
  NS_ENSURE_SUCCESS(res, 0);

  for (uint32_t index = 0; index < length; ++index) {
    double start, end;
    res = r->Start(index, &start);
    NS_ENSURE_SUCCESS(res, 0);

    res = r->End(index, &end);
    NS_ENSURE_SUCCESS(res, 0);

    if (start <= currentTime && end >= currentTime) {
      return static_cast<int64_t>((end - currentTime) * USECS_PER_S);
    }
  }
  return 0;
}

void MediaDecoderStateMachine::SetFrameBufferLength(uint32_t aLength)
{
  NS_ASSERTION(aLength >= 512 && aLength <= 16384,
               "The length must be between 512 and 16384");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEventManager.SetSignalBufferLength(aLength);
}

nsresult MediaDecoderStateMachine::DecodeMetadata()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState == DECODER_STATE_DECODING_METADATA,
               "Only call when in metadata decoding state");

  LOG(PR_LOG_DEBUG, ("%p Decoding Media Headers", mDecoder.get()));
  nsresult res;
  VideoInfo info;
  MetadataTags* tags;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    res = mReader->ReadMetadata(&info, &tags);
  }
  mInfo = info;

  if (NS_FAILED(res) || (!info.mHasVideo && !info.mHasAudio)) {
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &MediaDecoder::DecodeError);
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
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
  LOG(PR_LOG_DEBUG, ("%p Media goes from %lld to %lld (duration %lld)"
                     " transportSeekable=%d, mediaSeekable=%d",
                     mDecoder.get(), mStartTime, mEndTime, GetDuration(),
                     mTransportSeekable, mMediaSeekable));

  
  
  
  
  if (HasAudio()) {
    mEventManager.Init(mInfo.mAudioChannels, mInfo.mAudioRate);
    
    
    
    uint32_t frameBufferLength = mInfo.mAudioChannels * FRAMEBUFFER_LENGTH_PER_CHANNEL;
    mDecoder->RequestFrameBufferLength(frameBufferLength);
  }

  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new AudioMetadataEventRunner(mDecoder,
                                 mInfo.mAudioChannels,
                                 mInfo.mAudioRate,
                                 HasAudio(),
                                 HasVideo(),
                                 tags);
  NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

  if (mState == DECODER_STATE_DECODING_METADATA) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from DECODING_METADATA to DECODING", mDecoder.get()));
    StartDecoding();
  }

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
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState == DECODER_STATE_SEEKING,
               "Only call when in seeking state");

  mDidThrottleAudioDecoding = false;
  mDidThrottleVideoDecoding = false;

  
  
  
  
  
  
  
  
  int64_t seekTime = mSeekTime;
  mDecoder->StopProgressUpdates();

  bool currentTimeChanged = false;
  int64_t mediaTime = GetMediaTime();
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
    }
    if (NS_SUCCEEDED(res)) {
      AudioData* audio = HasAudio() ? mReader->AudioQueue().PeekFront() : nullptr;
      NS_ASSERTION(!audio || (audio->mTime <= seekTime &&
                              seekTime <= audio->mTime + audio->mDuration),
                    "Seek target should lie inside the first audio block after seek");
      int64_t startTime = (audio && audio->mTime < seekTime) ? audio->mTime : seekTime;
      mAudioStartTime = startTime;
      mPlayDuration = startTime - mStartTime;
      if (HasVideo()) {
        VideoData* video = mReader->VideoQueue().PeekFront();
        if (video) {
          NS_ASSERTION(video->mTime <= seekTime && seekTime <= video->mEndTime,
                        "Seek target should lie inside the first frame after seek");
          {
            ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
            RenderVideoFrame(video, TimeStamp::Now());
          }
          nsCOMPtr<nsIRunnable> event =
            NS_NewRunnableMethod(mDecoder, &MediaDecoder::Invalidate);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
        }
      }
    }
  }
  mDecoder->StartProgressUpdates();
  if (mState == DECODER_STATE_SHUTDOWN)
    return;

  
  LOG(PR_LOG_DEBUG, ("%p Seek completed, mCurrentFrameTime=%lld\n",
      mDecoder.get(), mCurrentFrameTime));

  
  
  

  nsCOMPtr<nsIRunnable> stopEvent;
  bool isLiveStream = mDecoder->GetResource()->GetLength() == -1;
  if (GetMediaTime() == mEndTime && !isLiveStream) {
    
    
    
    LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lld) to COMPLETED",
                        mDecoder.get(), seekTime));
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStoppedAtEnd);
    mState = DECODER_STATE_COMPLETED;
  } else {
    LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lld) to DECODING",
                        mDecoder.get(), seekTime));
    stopEvent = NS_NewRunnableMethod(mDecoder, &MediaDecoder::SeekingStopped);
    StartDecoding();
  }
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
  nsCOMPtr<MediaDecoderStateMachine> mStateMachine;
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
  nsCOMPtr<MediaDecoderStateMachine> mStateMachine;
};

nsresult MediaDecoderStateMachine::RunStateMachine()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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
      StopDecodeThread();
      
      
      mPendingWakeDecoder = nullptr;
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "How did we escape from the shutdown state?");
      
      
      
      
      
      
      
      
      
      
      
      
      
      NS_DispatchToCurrentThread(new nsDispatchDisposeEvent(mDecoder, this));
      return NS_OK;
    }

    case DECODER_STATE_DECODING_METADATA: {
      
      return ScheduleDecodeThread();
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

      if (IsPausedAndDecoderWaiting()) {
        
        
        StopDecodeThread();
        return NS_OK;
      }

      
      
      if (NS_FAILED(ScheduleDecodeThread())) {
        NS_WARNING("Failed to start media decode thread!");
        return NS_ERROR_FAILURE;
      }

      AdvanceFrame();
      NS_ASSERTION(mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING ||
                   IsStateMachineScheduled() ||
                   mPlaybackRate == 0.0, "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_BUFFERING: {
      if (IsPausedAndDecoderWaiting()) {
        
        
        StopDecodeThread();
        return NS_OK;
      }

      TimeStamp now = TimeStamp::Now();
      NS_ASSERTION(!mBufferingStart.IsNull(), "Must know buffering start time.");

      
      
      
      TimeDuration elapsed = now - mBufferingStart;
      bool isLiveStream = mDecoder->GetResource()->GetLength() == -1;
      if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
            elapsed < TimeDuration::FromSeconds(mBufferingWait * mPlaybackRate) &&
            (mQuickBuffering ? HasLowDecodedData(QUICK_BUFFERING_LOW_DATA_USECS)
                            : (GetUndecodedData() < mBufferingWait * mPlaybackRate * USECS_PER_S)) &&
            !resource->IsDataCachedToEndOfResource(mDecoder->mDecoderPosition) &&
            !resource->IsSuspended())
      {
        LOG(PR_LOG_DEBUG,
            ("%p Buffering: %.3lfs/%ds, timeout in %.3lfs %s",
              mDecoder.get(),
              GetUndecodedData() / static_cast<double>(USECS_PER_S),
              mBufferingWait,
              mBufferingWait - elapsed.ToSeconds(),
              (mQuickBuffering ? "(quick exit)" : "")));
        ScheduleStateMachine(USECS_PER_S);
        return NS_OK;
      } else {
        LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder.get()));
        LOG(PR_LOG_DEBUG, ("%p Buffered for %.3lfs",
                            mDecoder.get(),
                            (now - mBufferingStart).ToSeconds()));
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
      
     return ScheduleDecodeThread();
    }

    case DECODER_STATE_COMPLETED: {
      StopDecodeThread();

      if (mState != DECODER_STATE_COMPLETED) {
        
        
        
        
        NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
        return NS_OK;
      }

      
      
      
      if (mState == DECODER_STATE_COMPLETED &&
          (mReader->VideoQueue().GetSize() > 0 ||
          (HasAudio() && !mAudioCompleted)))
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
        int64_t videoTime = HasVideo() ? mVideoFrameEndTime : 0;
        int64_t clockTime = std::max(mEndTime, std::max(videoTime, GetAudioClock()));
        UpdatePlaybackPosition(clockTime);
        nsCOMPtr<nsIRunnable> event =
          NS_NewRunnableMethod(mDecoder, &MediaDecoder::PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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

  if (!PR_GetEnv("MOZ_QUIET")) {
    LOG(PR_LOG_DEBUG, ("%p Decoder playing video frame %lld",
                       mDecoder.get(), aData->mTime));
  }

  VideoFrameContainer* container = mDecoder->GetVideoFrameContainer();
  if (container) {
    container->SetCurrentFrame(aData->mDisplay, aData->mImage, aTarget);
  }
}

int64_t
MediaDecoderStateMachine::GetAudioClock()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  
  
  
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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

int64_t MediaDecoderStateMachine::GetClock() {
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  
  
  
  int64_t clock_time = -1;
  if (!IsPlaying()) {
    clock_time = mPlayDuration + mStartTime;
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
  if (mReader->VideoQueue().GetSize() > 0) {
    VideoData* frame = mReader->VideoQueue().PeekFront();
    while (mRealTime || clock_time >= frame->mTime) {
      mVideoFrameEndTime = frame->mEndTime;
      currentFrame = frame;
      LOG(PR_LOG_DEBUG, ("%p Decoder discarding video frame %lld", mDecoder.get(), frame->mTime));
#ifdef PR_LOGGING
      if (droppedFrames++) {
        LOG(PR_LOG_DEBUG, ("%p Decoder discarding video frame %lld (%d so far)",
              mDecoder.get(), frame->mTime, droppedFrames - 1));
      }
#endif
      mReader->VideoQueue().PopFront();
      
      
      mDecoder->GetReentrantMonitor().NotifyAll();
      mDecoder->UpdatePlaybackOffset(frame->mOffset);
      if (mReader->VideoQueue().GetSize() == 0)
        break;
      frame = mReader->VideoQueue().PeekFront();
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
      !resource->IsDataCachedToEndOfResource(mDecoder->mDecoderPosition) &&
      !resource->IsSuspended() &&
      (JustExitedQuickBuffering() || HasLowUndecodedData()))
  {
    if (currentFrame) {
      mReader->VideoQueue().PushFront(currentFrame.forget());
    }
    StartBuffering();
    ScheduleStateMachine();
    return;
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
    mDecoder->GetFrameStatistics().NotifyPresentedFrame();
    remainingTime = currentFrame->mEndTime - clock_time;
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
  LOG(PR_LOG_DEBUG, ("%p Media start time is %lld", mDecoder.get(), mStartTime));
  return v;
}

void MediaDecoderStateMachine::UpdateReadyState() {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  MediaDecoderOwner::NextFrameStatus nextFrameStatus = GetNextFrameStatus();
  if (nextFrameStatus == mLastFrameStatus) {
    return;
  }
  mLastFrameStatus = nextFrameStatus;

  nsCOMPtr<nsIRunnable> event;
  switch (nextFrameStatus) {
    case MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE_BUFFERING:
      event = NS_NewRunnableMethod(mDecoder, &MediaDecoder::NextFrameUnavailableBuffering);
      break;
    case MediaDecoderOwner::NEXT_FRAME_AVAILABLE:
      event = NS_NewRunnableMethod(mDecoder, &MediaDecoder::NextFrameAvailable);
      break;
    case MediaDecoderOwner::NEXT_FRAME_UNAVAILABLE:
      event = NS_NewRunnableMethod(mDecoder, &MediaDecoder::NextFrameUnavailable);
      break;
    default:
      PR_NOT_REACHED("unhandled frame state");
  }

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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

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
  LOG(PR_LOG_DEBUG, ("%p Changed state from DECODING to BUFFERING, decoded for %.3lfs",
                     mDecoder.get(), decodeDuration.ToSeconds()));
#ifdef PR_LOGGING
  MediaDecoder::Statistics stats = mDecoder->GetStatistics();
#endif
  LOG(PR_LOG_DEBUG, ("%p Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
    mDecoder.get(),
    stats.mPlaybackRate/1024, stats.mPlaybackRateReliable ? "" : " (unreliable)",
    stats.mDownloadRate/1024, stats.mDownloadRateReliable ? "" : " (unreliable)"));
}

nsresult MediaDecoderStateMachine::GetBuffered(TimeRanges* aBuffered) {
  MediaResource* resource = mDecoder->GetResource();
  NS_ENSURE_TRUE(resource, NS_ERROR_FAILURE);
  resource->Pin();
  nsresult res = mReader->GetBuffered(aBuffered, mStartTime);
  resource->Unpin();
  return res;
}

bool MediaDecoderStateMachine::IsPausedAndDecoderWaiting() {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  return
    mDecodeThreadWaiting &&
    mDecoder->GetState() != MediaDecoder::PLAY_STATE_PLAYING &&
    (mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING);
}

nsresult MediaDecoderStateMachine::Run()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  return CallRunStateMachine();
}

nsresult MediaDecoderStateMachine::CallRunStateMachine()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  
  
  mRunAgain = false;

  
  
  mDispatchedRunEvent = false;

  
  if (mAudioCaptured) {
    StopAudioThread();
  }

  mTimeout = TimeStamp();

  mIsRunning = true;
  nsresult res = RunStateMachine();
  mIsRunning = false;

  if (mRunAgain && !mDispatchedRunEvent) {
    mDispatchedRunEvent = true;
    return NS_DispatchToCurrentThread(this);
  }

  return res;
}

static void TimeoutExpired(nsITimer *aTimer, void *aClosure) {
  MediaDecoderStateMachine *machine =
    static_cast<MediaDecoderStateMachine*>(aClosure);
  NS_ASSERTION(machine, "Must have been passed state machine");
  machine->TimeoutExpired();
}

void MediaDecoderStateMachine::TimeoutExpired()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread(), "Must be on state machine thread");
  if (mIsRunning) {
    mRunAgain = true;
  } else if (!mDispatchedRunEvent) {
    
    
    CallRunStateMachine();
  }
  
  
  
}

void MediaDecoderStateMachine::ScheduleStateMachineWithLockAndWakeDecoder() {
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mon.NotifyAll();
  ScheduleStateMachine();
}

nsresult MediaDecoderStateMachine::ScheduleStateMachine(int64_t aUsecs) {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ABORT_IF_FALSE(GetStateMachineThread(),
    "Must have a state machine thread to schedule");

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }
  aUsecs = std::max<int64_t>(aUsecs, 0);

  TimeStamp timeout = TimeStamp::Now() + UsecsToDuration(aUsecs);
  if (!mTimeout.IsNull()) {
    if (timeout >= mTimeout) {
      
      
      return NS_OK;
    }
    if (mTimer) {
      
      
      mTimer->Cancel();
    }
  }

  uint32_t ms = static_cast<uint32_t>((aUsecs / USECS_PER_MS) & 0xFFFFFFFF);
  if (mRealTime && ms > 40)
    ms = 40;
  if (ms == 0) {
    if (mIsRunning) {
      
      
      mRunAgain = true;
      return NS_OK;
    } else if (!mDispatchedRunEvent) {
      
      
      mDispatchedRunEvent = true;
      return GetStateMachineThread()->Dispatch(this, NS_DISPATCH_NORMAL);
    }
    
    
    
    return NS_OK;
  }

  mTimeout = timeout;

  nsresult res;
  if (!mTimer) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &res);
    if (NS_FAILED(res)) return res;
    mTimer->SetTarget(GetStateMachineThread());
  }

  res = mTimer->InitWithFuncCallback(mozilla::TimeoutExpired,
                                     this,
                                     ms,
                                     nsITimer::TYPE_ONE_SHOT);
  return res;
}

bool MediaDecoderStateMachine::OnStateMachineThread() const
{
    return IsCurrentThread(GetStateMachineThread());
}

nsIThread* MediaDecoderStateMachine::GetStateMachineThread()
{
  return StateMachineTracker::Instance().GetGlobalStateMachineThread();
}

void MediaDecoderStateMachine::NotifyAudioAvailableListener()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEventManager.NotifyAudioAvailableListener();
}

void MediaDecoderStateMachine::SetPlaybackRate(double aPlaybackRate)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ASSERTION(aPlaybackRate != 0,
      "PlaybackRate == 0 should be handled before this function.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  if (mAudioStream && mAudioStream->GetChannels() > 2) {
    return;
  }

  if (mPlaybackRate == aPlaybackRate) {
    return;
  }

  
  if (!HasAudio()) {
    
    if (mState == DECODER_STATE_SEEKING) {
      mBasePosition = mSeekTime - mStartTime;
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

  return;
}

bool MediaDecoderStateMachine::IsShutdown()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
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
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  TimedMetadata* metadata = new TimedMetadata;
  metadata->mPublishTime = aPublishTime;
  metadata->mChannels = aChannels;
  metadata->mRate = aRate;
  metadata->mHasAudio = aHasAudio;
  metadata->mTags = aTags;
  mMetadataManager.QueueMetadata(metadata);
}

} 

