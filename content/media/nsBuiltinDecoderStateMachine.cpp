





































#include <limits>
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"
#include "nsTimeRanges.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::layers;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif




static const PRUint32 BUFFERING_WAIT = 30000;





static const PRUint32 LOW_AUDIO_USECS = 300000;





const PRInt64 AMPLE_AUDIO_USECS = 1000000;






const PRUint32 SILENCE_BYTES_CHUNK = 32 * 1024;




static const PRUint32 LOW_VIDEO_FRAMES = 1;




static const PRUint32 AMPLE_VIDEO_FRAMES = 10;


static const int AUDIO_DURATION_USECS = 40000;





static const int THRESHOLD_FACTOR = 2;





static const PRInt64 LOW_DATA_THRESHOLD_USECS = 5000000;



PR_STATIC_ASSERT(LOW_DATA_THRESHOLD_USECS > AMPLE_AUDIO_USECS);


static const PRUint32 EXHAUSTED_DATA_MARGIN_USECS = 60000;










static const PRUint32 QUICK_BUFFER_THRESHOLD_USECS = 2000000;



static const PRUint32 QUICK_BUFFERING_LOW_DATA_USECS = 1000000;





PR_STATIC_ASSERT(QUICK_BUFFERING_LOW_DATA_USECS <= AMPLE_AUDIO_USECS);

static TimeDuration UsecsToDuration(PRInt64 aUsecs) {
  return TimeDuration::FromMilliseconds(static_cast<double>(aUsecs) / USECS_PER_MS);
}

static PRInt64 DurationToUsecs(TimeDuration aDuration) {
  return static_cast<PRInt64>(aDuration.ToSeconds() * USECS_PER_S);
}

class nsAudioMetadataEventRunner : public nsRunnable
{
private:
  nsCOMPtr<nsBuiltinDecoder> mDecoder;
public:
  nsAudioMetadataEventRunner(nsBuiltinDecoder* aDecoder, PRUint32 aChannels,
                             PRUint32 aRate) :
    mDecoder(aDecoder),
    mChannels(aChannels),
    mRate(aRate)
  {
  }

  NS_IMETHOD Run()
  {
    mDecoder->MetadataLoaded(mChannels, mRate);
    return NS_OK;
  }

  const PRUint32 mChannels;
  const PRUint32 mRate;
};

static PRUint32 gStateMachineCount = 0;
static nsIThread* gStateMachineThread = 0;

nsIThread* nsBuiltinDecoderStateMachine::GetStateMachineThread() {
  return gStateMachineThread;
}


class ShutdownThreadEvent : public nsRunnable 
{
public:
  ShutdownThreadEvent(nsIThread* aThread) : mThread(aThread) {}
  ~ShutdownThreadEvent() {}
  NS_IMETHOD Run() {
    mThread->Shutdown();
    mThread = nsnull;
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};

nsBuiltinDecoderStateMachine::nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder,
                                                           nsBuiltinDecoderReader* aReader,
                                                           bool aRealTime) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mCbCrSize(0),
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
  mSeekable(true),
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
  mEventManager(aDecoder)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoderStateMachine);
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  if (gStateMachineCount == 0) {
    NS_ASSERTION(!gStateMachineThread, "Should have null state machine thread!");
    nsresult res = NS_NewThread(&gStateMachineThread,
                                nsnull,
                                MEDIA_THREAD_STACK_SIZE);
    NS_ABORT_IF_FALSE(NS_SUCCEEDED(res), "Can't create media state machine thread");
  }
  gStateMachineCount++;

  
  if (Preferences::GetBool("media.realtime_decoder.enabled", false) == false)
    mRealTime = false;

  mBufferingWait = mRealTime ? 0 : BUFFERING_WAIT;
  mLowDataThresholdUsecs = mRealTime ? 0 : LOW_DATA_THRESHOLD_USECS;
}

nsBuiltinDecoderStateMachine::~nsBuiltinDecoderStateMachine()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MOZ_COUNT_DTOR(nsBuiltinDecoderStateMachine);
  if (mTimer)
    mTimer->Cancel();
  mTimer = nsnull;
  
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  NS_ABORT_IF_FALSE(gStateMachineCount > 0,
    "State machine ref count must be > 0");
  gStateMachineCount--;
  if (gStateMachineCount == 0) {
    LOG(PR_LOG_DEBUG, ("Destroying media state machine thread"));
    nsCOMPtr<nsIRunnable> event = new ShutdownThreadEvent(gStateMachineThread);
    NS_RELEASE(gStateMachineThread);
    gStateMachineThread = nsnull;
    NS_DispatchToMainThread(event);
  }
}

bool nsBuiltinDecoderStateMachine::HasFutureAudio() const {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedUsecs() > LOW_AUDIO_USECS || mReader->mAudioQueue.IsFinished());
}

bool nsBuiltinDecoderStateMachine::HaveNextFrameData() const {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  return (!HasAudio() || HasFutureAudio()) &&
         (!HasVideo() || mReader->mVideoQueue.GetSize() > 0);
}

PRInt64 nsBuiltinDecoderStateMachine::GetDecodedAudioDuration() {
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  PRInt64 audioDecoded = mReader->mAudioQueue.Duration();
  if (mAudioEndTime != -1) {
    audioDecoded += mAudioEndTime - GetMediaTime();
  }
  return audioDecoded;
}

void nsBuiltinDecoderStateMachine::DecodeThreadRun()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  if (mState == DECODER_STATE_DECODING_METADATA) {
    if (NS_FAILED(DecodeMetadata())) {
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "Should be in shutdown state if metadata loading fails.");
      LOG(PR_LOG_DEBUG, ("Decode metadata failed, shutting down decode thread"));
    }
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

void nsBuiltinDecoderStateMachine::DecodeLoop()
{
  LOG(PR_LOG_DEBUG, ("%p Start DecodeLoop()", mDecoder.get()));

  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  
  bool audioPump = true;
  bool videoPump = true;

  
  
  
  
  bool skipToNextKeyframe = false;

  
  
  const unsigned videoPumpThreshold = mRealTime ? 0 : AMPLE_VIDEO_FRAMES / 2;

  
  
  
  const unsigned audioPumpThreshold = mRealTime ? 0 : LOW_AUDIO_USECS * 2;

  
  
  PRInt64 lowAudioThreshold = LOW_AUDIO_USECS;

  
  
  
  PRInt64 ampleAudioThreshold = AMPLE_AUDIO_USECS;

  MediaQueue<VideoData>& videoQueue = mReader->mVideoQueue;
  MediaQueue<AudioData>& audioQueue = mReader->mAudioQueue;

  
  bool videoPlaying = HasVideo();
  bool audioPlaying = HasAudio();
  while ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING) &&
         !mStopDecodeThread &&
         (videoPlaying || audioPlaying))
  {
    
    
    
    if (videoPump &&
        static_cast<PRUint32>(videoQueue.GetSize()) >= videoPumpThreshold)
    {
      videoPump = false;
    }

    
    
    
    if (audioPump && GetDecodedAudioDuration() >= audioPumpThreshold) {
      audioPump = false;
    }

    
    
    
    
    
    
    if (mState == DECODER_STATE_DECODING &&
        !skipToNextKeyframe &&
        videoPlaying &&
        ((!audioPump && audioPlaying && GetDecodedAudioDuration() < lowAudioThreshold) ||
         (!videoPump &&
           videoPlaying &&
           static_cast<PRUint32>(videoQueue.GetSize()) < LOW_VIDEO_FRAMES)) &&
        !HasLowUndecodedData())

    {
      skipToNextKeyframe = true;
      LOG(PR_LOG_DEBUG, ("%p Skipping video decode to the next keyframe", mDecoder.get()));
    }

    
    if (videoPlaying &&
        static_cast<PRUint32>(videoQueue.GetSize()) < AMPLE_VIDEO_FRAMES)
    {
      
      
      
      TimeDuration decodeTime;
      {
        PRInt64 currentTime = GetMediaTime();
        ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
        TimeStamp start = TimeStamp::Now();
        videoPlaying = mReader->DecodeVideoFrame(skipToNextKeyframe, currentTime);
        decodeTime = TimeStamp::Now() - start;
      }
      if (THRESHOLD_FACTOR * DurationToUsecs(decodeTime) > lowAudioThreshold &&
          !HasLowUndecodedData())
      {
        lowAudioThreshold =
          NS_MIN(THRESHOLD_FACTOR * DurationToUsecs(decodeTime), AMPLE_AUDIO_USECS);
        ampleAudioThreshold = NS_MAX(THRESHOLD_FACTOR * lowAudioThreshold,
                                     ampleAudioThreshold);
        LOG(PR_LOG_DEBUG,
            ("Slow video decode, set lowAudioThreshold=%lld ampleAudioThreshold=%lld",
             lowAudioThreshold, ampleAudioThreshold));
      }
    }

    
    if (audioPlaying &&
        (GetDecodedAudioDuration() < ampleAudioThreshold || audioQueue.GetSize() == 0))
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      audioPlaying = mReader->DecodeAudioData();
    }

    
    
    mDecoder->GetReentrantMonitor().NotifyAll();

    
    
    UpdateReadyState();

    if ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING) &&
        !mStopDecodeThread &&
        (videoPlaying || audioPlaying) &&
        (!audioPlaying || (GetDecodedAudioDuration() >= ampleAudioThreshold &&
                           audioQueue.GetSize() > 0))
        &&
        (!videoPlaying ||
          static_cast<PRUint32>(videoQueue.GetSize()) >= AMPLE_VIDEO_FRAMES))
    {
      
      
      
      
      
      
      
      
      
      mDecodeThreadWaiting = true;
      if (mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING) {
        
        
        
        
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

bool nsBuiltinDecoderStateMachine::IsPlaying()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  return !mPlayStartTime.IsNull();
}

void nsBuiltinDecoderStateMachine::AudioLoop()
{
  NS_ASSERTION(OnAudioThread(), "Should be on audio thread.");
  LOG(PR_LOG_DEBUG, ("%p Begun audio thread/loop", mDecoder.get()));
  PRInt64 audioDuration = 0;
  PRInt64 audioStartTime = -1;
  PRInt64 framesWritten = 0;
  PRUint32 channels, rate;
  double volume = -1;
  bool setVolume;
  PRInt32 minWriteFrames = -1;
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioCompleted = false;
    audioStartTime = mAudioStartTime;
    channels = mInfo.mAudioChannels;
    rate = mInfo.mAudioRate;
    NS_ASSERTION(audioStartTime != -1, "Should have audio start time by now");
  }

  
  
  
  
  
  
  
  
  nsRefPtr<nsAudioStream> audioStream = nsAudioStream::AllocateStream();
  audioStream->Init(channels, rate, MOZ_AUDIO_DATA_FORMAT);

  {
    
    
    
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioStream = audioStream;
    volume = mVolume;
    mAudioStream->SetVolume(volume);
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
              (mReader->mAudioQueue.GetSize() == 0 &&
               !mReader->mAudioQueue.AtEndOfStream())))
      {
        if (!IsPlaying() && !mAudioStream->IsPaused()) {
          mAudioStream->Pause();
        }
        mon.Wait();
      }

      
      if (mState == DECODER_STATE_SHUTDOWN ||
          mStopAudioThread ||
          mReader->mAudioQueue.AtEndOfStream())
      {
        break;
      }

      
      
      setVolume = volume != mVolume;
      volume = mVolume;

      
      
      
      if (IsPlaying() && mAudioStream->IsPaused()) {
        mAudioStream->Resume();
      }
    }

    if (setVolume) {
      mAudioStream->SetVolume(volume);
    }
    if (minWriteFrames == -1) {
      minWriteFrames = mAudioStream->GetMinWriteSize();
    }
    NS_ASSERTION(mReader->mAudioQueue.GetSize() > 0,
                 "Should have data to play");
    
    
    const AudioData* s = mReader->mAudioQueue.PeekFront();

    
    
    PRInt64 playedFrames = 0;
    if (!UsecsToFrames(audioStartTime, rate, playedFrames)) {
      NS_WARNING("Int overflow converting playedFrames");
      break;
    }
    if (!AddOverflow(playedFrames, audioDuration, playedFrames)) {
      NS_WARNING("Int overflow adding playedFrames");
      break;
    }

    
    
    PRInt64 sampleTime = 0;
    if (!UsecsToFrames(s->mTime, rate, sampleTime)) {
      NS_WARNING("Int overflow converting sampleTime");
      break;
    }
    PRInt64 missingFrames = 0;
    if (!AddOverflow(sampleTime, -playedFrames, missingFrames)) {
      NS_WARNING("Int overflow adding missingFrames");
      break;
    }

    if (missingFrames > 0) {
      
      
      
      
      missingFrames = NS_MIN(static_cast<PRInt64>(PR_UINT32_MAX), missingFrames);
      framesWritten = PlaySilence(static_cast<PRUint32>(missingFrames),
                                  channels, playedFrames);
    } else {
      framesWritten = PlayFromAudioQueue(sampleTime, channels);
    }
    audioDuration += framesWritten;
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      PRInt64 playedUsecs;
      if (!FramesToUsecs(audioDuration, rate, playedUsecs)) {
        NS_WARNING("Int overflow calculating playedUsecs");
        break;
      }
      if (!AddOverflow(audioStartTime, playedUsecs, mAudioEndTime)) {
        NS_WARNING("Int overflow calculating audio end time");
        break;
      }

      PRInt64 audioAhead = mAudioEndTime - GetMediaTime();
      if (audioAhead > AMPLE_AUDIO_USECS &&
          framesWritten > minWriteFrames)
      {
        
        
        
        
        Wait(AMPLE_AUDIO_USECS / 2);
        
        
        
        
        
        
        mon.NotifyAll();
      }
    }
  }
  if (mReader->mAudioQueue.AtEndOfStream() &&
      mState != DECODER_STATE_SHUTDOWN &&
      !mStopAudioThread)
  {
    
    
    bool seeking = false;
    {
      ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
      if (framesWritten < minWriteFrames) {
        
        
        
        PRInt64 minToWrite = minWriteFrames - framesWritten;
        if (minToWrite < PR_UINT32_MAX / channels) {
          
          
          PRUint32 numSamples = minToWrite * channels;
          nsAutoArrayPtr<AudioDataValue> buf(new AudioDataValue[numSamples]);
          memset(buf.get(), 0, numSamples * sizeof(AudioDataValue));
          mAudioStream->Write(buf, minToWrite);
        }
      }

      PRInt64 oldPosition = -1;
      PRInt64 position = GetMediaTime();
      while (oldPosition != position &&
             mAudioEndTime - position > 0 &&
             mState != DECODER_STATE_SEEKING &&
             mState != DECODER_STATE_SHUTDOWN)
      {
        const PRInt64 DRAIN_BLOCK_USECS = 100000;
        Wait(NS_MIN(mAudioEndTime - position, DRAIN_BLOCK_USECS));
        oldPosition = position;
        position = GetMediaTime();
      }
      seeking = mState == DECODER_STATE_SEEKING;
    }

    if (!seeking && !mAudioStream->IsPaused()) {
      mAudioStream->Drain();
      
      mEventManager.Drain(mAudioEndTime);
    }
  }
  LOG(PR_LOG_DEBUG, ("%p Reached audio stream end.", mDecoder.get()));
  {
    
    
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    mAudioStream = nsnull;
    mEventManager.Clear();
    mAudioCompleted = true;
    UpdateReadyState();
    
    mDecoder->GetReentrantMonitor().NotifyAll();
  }

  
  
  audioStream->Shutdown();
  audioStream = nsnull;

  LOG(PR_LOG_DEBUG, ("%p Audio stream finished playing, audio thread exit", mDecoder.get()));
}

PRUint32 nsBuiltinDecoderStateMachine::PlaySilence(PRUint32 aFrames,
                                                   PRUint32 aChannels,
                                                   PRUint64 aFrameOffset)

{
  NS_ASSERTION(OnAudioThread(), "Only call on audio thread.");
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  PRUint32 maxFrames = SILENCE_BYTES_CHUNK / aChannels / sizeof(AudioDataValue);
  PRUint32 frames = NS_MIN(aFrames, maxFrames);
  PRUint32 numSamples = frames * aChannels;
  nsAutoArrayPtr<AudioDataValue> buf(new AudioDataValue[numSamples]);
  memset(buf.get(), 0, numSamples * sizeof(AudioDataValue));
  mAudioStream->Write(buf, frames);
  
  mEventManager.QueueWrittenAudioData(buf.get(), frames * aChannels,
                                      (aFrameOffset + frames) * aChannels);
  return frames;
}

PRUint32 nsBuiltinDecoderStateMachine::PlayFromAudioQueue(PRUint64 aFrameOffset,
                                                          PRUint32 aChannels)
{
  NS_ASSERTION(OnAudioThread(), "Only call on audio thread.");
  NS_ASSERTION(!mAudioStream->IsPaused(), "Don't play when paused");
  nsAutoPtr<AudioData> audio(mReader->mAudioQueue.PopFront());
  {
    ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
    NS_WARN_IF_FALSE(IsPlaying(), "Should be playing");
    
    
    mDecoder->GetReentrantMonitor().NotifyAll();
  }
  PRInt64 offset = -1;
  PRUint32 frames = 0;
  
  
  
  
  
  
  
  if (!mAudioStream->IsPaused()) {
    mAudioStream->Write(audio->mAudioData,
                        audio->mFrames);

    offset = audio->mOffset;
    frames = audio->mFrames;

    
    mEventManager.QueueWrittenAudioData(audio->mAudioData.get(),
                                        audio->mFrames * aChannels,
                                        (aFrameOffset + frames) * aChannels);
  } else {
    mReader->mAudioQueue.PushFront(audio);
    audio.forget();
  }
  if (offset != -1) {
    mDecoder->UpdatePlaybackOffset(offset);
  }
  return frames;
}

nsresult nsBuiltinDecoderStateMachine::Init(nsDecoderStateMachine* aCloneDonor)
{
  nsBuiltinDecoderReader* cloneReader = nsnull;
  if (aCloneDonor) {
    cloneReader = static_cast<nsBuiltinDecoderStateMachine*>(aCloneDonor)->mReader;
  }
  return mReader->Init(cloneReader);
}

void nsBuiltinDecoderStateMachine::StopPlayback()
{
  LOG(PR_LOG_DEBUG, ("%p StopPlayback()", mDecoder.get()));

  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mDecoder->mPlaybackStatistics.Stop(TimeStamp::Now());

  
  
  
  
  
  if (IsPlaying()) {
    mPlayDuration += DurationToUsecs(TimeStamp::Now() - mPlayStartTime);
    mPlayStartTime = TimeStamp();
  }
  
  
  mDecoder->GetReentrantMonitor().NotifyAll();
  NS_ASSERTION(!IsPlaying(), "Should report not playing at end of StopPlayback()");
}

void nsBuiltinDecoderStateMachine::StartPlayback()
{
  LOG(PR_LOG_DEBUG, ("%p StartPlayback()", mDecoder.get()));

  NS_ASSERTION(!IsPlaying(), "Shouldn't be playing when StartPlayback() is called");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  LOG(PR_LOG_DEBUG, ("%p StartPlayback", mDecoder.get()));
  mDecoder->mPlaybackStatistics.Start(TimeStamp::Now());
  mPlayStartTime = TimeStamp::Now();

  NS_ASSERTION(IsPlaying(), "Should report playing by end of StartPlayback()");
  if (NS_FAILED(StartAudioThread())) {
    NS_WARNING("Failed to create audio thread"); 
  }
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void nsBuiltinDecoderStateMachine::UpdatePlaybackPositionInternal(PRInt64 aTime)
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
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::DurationChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void nsBuiltinDecoderStateMachine::UpdatePlaybackPosition(PRInt64 aTime)
{
  UpdatePlaybackPositionInternal(aTime);

  bool fragmentEnded = mFragmentEndTime >= 0 && GetMediaTime() >= mFragmentEndTime;
  if (!mPositionChangeQueued || fragmentEnded) {
    mPositionChangeQueued = true;
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::PlaybackPositionChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }

  
  mEventManager.DispatchPendingEvents(GetMediaTime());

  if (fragmentEnded) {
    StopPlayback();
  }
}

void nsBuiltinDecoderStateMachine::ClearPositionChangeFlag()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mPositionChangeQueued = false;
}

nsHTMLMediaElement::NextFrameStatus nsBuiltinDecoderStateMachine::GetNextFrameStatus()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  if (IsBuffering() || IsSeeking()) {
    return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING;
  } else if (HaveNextFrameData()) {
    return nsHTMLMediaElement::NEXT_FRAME_AVAILABLE;
  }
  return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE;
}

void nsBuiltinDecoderStateMachine::SetVolume(double volume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  mVolume = volume;
}

double nsBuiltinDecoderStateMachine::GetCurrentTime() const
{
  NS_ASSERTION(NS_IsMainThread() ||
               OnStateMachineThread() ||
               OnDecodeThread(),
               "Should be on main, decode, or state machine thread.");

  return static_cast<double>(mCurrentFrameTime) / static_cast<double>(USECS_PER_S);
}

PRInt64 nsBuiltinDecoderStateMachine::GetDuration()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

void nsBuiltinDecoderStateMachine::SetDuration(PRInt64 aDuration)
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

void nsBuiltinDecoderStateMachine::SetEndTime(PRInt64 aEndTime)
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mEndTime = aEndTime;
}

void nsBuiltinDecoderStateMachine::SetFragmentEndTime(PRInt64 aEndTime)
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mFragmentEndTime = aEndTime < 0 ? aEndTime : aEndTime + mStartTime;
}

void nsBuiltinDecoderStateMachine::SetSeekable(bool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  mSeekable = aSeekable;
}

void nsBuiltinDecoderStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());

  
  
  LOG(PR_LOG_DEBUG, ("%p Changed state to SHUTDOWN", mDecoder.get()));
  ScheduleStateMachine();
  mState = DECODER_STATE_SHUTDOWN;
  mDecoder->GetReentrantMonitor().NotifyAll();
}

void nsBuiltinDecoderStateMachine::StartDecoding()
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

void nsBuiltinDecoderStateMachine::Play()
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

void nsBuiltinDecoderStateMachine::ResetPlayback()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mVideoFrameEndTime = -1;
  mAudioStartTime = -1;
  mAudioEndTime = -1;
  mAudioCompleted = false;
}

void nsBuiltinDecoderStateMachine::Seek(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  
  
  NS_ASSERTION(mState != DECODER_STATE_SEEKING,
               "We shouldn't already be seeking");
  NS_ASSERTION(mState >= DECODER_STATE_DECODING,
               "We should have loaded metadata");
  double t = aTime * static_cast<double>(USECS_PER_S);
  if (t > PR_INT64_MAX) {
    
    return;
  }

  mSeekTime = static_cast<PRInt64>(t) + mStartTime;
  NS_ASSERTION(mSeekTime >= mStartTime && mSeekTime <= mEndTime,
               "Can only seek in range [0,duration]");

  
  NS_ASSERTION(mStartTime != -1, "Should know start time by now");
  NS_ASSERTION(mEndTime != -1, "Should know end time by now");
  mSeekTime = NS_MIN(mSeekTime, mEndTime);
  mSeekTime = NS_MAX(mStartTime, mSeekTime);
  LOG(PR_LOG_DEBUG, ("%p Changed state to SEEKING (to %f)", mDecoder.get(), aTime));
  mState = DECODER_STATE_SEEKING;
  ScheduleStateMachine();
}

void nsBuiltinDecoderStateMachine::StopDecodeThread()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mStopDecodeThread = true;
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mDecodeThread) {
    LOG(PR_LOG_DEBUG, ("%p Shutdown decode thread", mDecoder.get()));
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mDecodeThread->Shutdown();
    }
    mDecodeThread = nsnull;
    mDecodeThreadIdle = false;
  }
}

void nsBuiltinDecoderStateMachine::StopAudioThread()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mStopAudioThread = true;
  mDecoder->GetReentrantMonitor().NotifyAll();
  if (mAudioThread) {
    LOG(PR_LOG_DEBUG, ("%p Shutdown audio thread", mDecoder.get()));
    {
      ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
      mAudioThread->Shutdown();
    }
    mAudioThread = nsnull;
  }
}

nsresult
nsBuiltinDecoderStateMachine::StartDecodeThread()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mStopDecodeThread = false;
  if ((mDecodeThread && !mDecodeThreadIdle) || mState >= DECODER_STATE_COMPLETED)
    return NS_OK;

  if (!mDecodeThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mDecodeThread),
                               nsnull,
                               MEDIA_THREAD_STACK_SIZE);
    if (NS_FAILED(rv)) {
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }
  }
  nsCOMPtr<nsIRunnable> event =
    NS_NewRunnableMethod(this, &nsBuiltinDecoderStateMachine::DecodeThreadRun);
  mDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
  mDecodeThreadIdle = false;
  return NS_OK;
}

nsresult
nsBuiltinDecoderStateMachine::StartAudioThread()
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mStopAudioThread = false;
  if (HasAudio() && !mAudioThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mAudioThread),
                               nsnull,
                               MEDIA_THREAD_STACK_SIZE);
    if (NS_FAILED(rv)) {
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsBuiltinDecoderStateMachine::AudioLoop);
    mAudioThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}

PRInt64 nsBuiltinDecoderStateMachine::AudioDecodedUsecs() const
{
  NS_ASSERTION(HasAudio(),
               "Should only call AudioDecodedUsecs() when we have audio");
  
  
  
  PRInt64 pushed = (mAudioEndTime != -1) ? (mAudioEndTime - GetMediaTime()) : 0;
  return pushed + mReader->mAudioQueue.Duration();
}

bool nsBuiltinDecoderStateMachine::HasLowDecodedData(PRInt64 aAudioUsecs) const
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  
  
  
  
  return ((HasAudio() &&
           !mReader->mAudioQueue.IsFinished() &&
           AudioDecodedUsecs() < aAudioUsecs)
          ||
         (!HasAudio() &&
          HasVideo() &&
          !mReader->mVideoQueue.IsFinished() &&
          static_cast<PRUint32>(mReader->mVideoQueue.GetSize()) < LOW_VIDEO_FRAMES));
}

bool nsBuiltinDecoderStateMachine::HasLowUndecodedData() const
{
  return GetUndecodedData() < mLowDataThresholdUsecs;
}

PRInt64 nsBuiltinDecoderStateMachine::GetUndecodedData() const
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA,
               "Must have loaded metadata for GetBuffered() to work");
  nsTimeRanges buffered;
  
  nsresult res = mDecoder->GetBuffered(&buffered);
  NS_ENSURE_SUCCESS(res, 0);
  double currentTime = GetCurrentTime();

  nsIDOMTimeRanges* r = static_cast<nsIDOMTimeRanges*>(&buffered);
  PRUint32 length = 0;
  res = r->GetLength(&length);
  NS_ENSURE_SUCCESS(res, 0);

  for (PRUint32 index = 0; index < length; ++index) {
    double start, end;
    res = r->Start(index, &start);
    NS_ENSURE_SUCCESS(res, 0);

    res = r->End(index, &end);
    NS_ENSURE_SUCCESS(res, 0);

    if (start <= currentTime && end >= currentTime) {
      return static_cast<PRInt64>((end - currentTime) * USECS_PER_S);
    }
  }
  return 0;
}

void nsBuiltinDecoderStateMachine::SetFrameBufferLength(PRUint32 aLength)
{
  NS_ASSERTION(aLength >= 512 && aLength <= 16384,
               "The length must be between 512 and 16384");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  mEventManager.SetSignalBufferLength(aLength);
}

nsresult nsBuiltinDecoderStateMachine::DecodeMetadata()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState == DECODER_STATE_DECODING_METADATA,
               "Only call when in metadata decoding state");

  LOG(PR_LOG_DEBUG, ("%p Decoding Media Headers", mDecoder.get()));
  nsresult res;
  nsVideoInfo info;
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    res = mReader->ReadMetadata(&info);
  }
  mInfo = info;

  if (NS_FAILED(res) || (!info.mHasVideo && !info.mHasAudio)) {
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::DecodeError);
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
  NS_ASSERTION((!HasVideo() && !HasAudio()) ||
                !mSeekable || mEndTime != -1,
                "Active seekable media should have end time");
  NS_ASSERTION(!mSeekable || GetDuration() != -1, "Seekable media should have duration");
  LOG(PR_LOG_DEBUG, ("%p Media goes from %lld to %lld (duration %lld) seekable=%d",
                      mDecoder.get(), mStartTime, mEndTime, GetDuration(), mSeekable));

  
  
  
  
  if (HasAudio()) {
    mEventManager.Init(mInfo.mAudioChannels, mInfo.mAudioRate);
    
    
    
    PRUint32 frameBufferLength = mInfo.mAudioChannels * FRAMEBUFFER_LENGTH_PER_CHANNEL;
    mDecoder->RequestFrameBufferLength(frameBufferLength);
  }
  nsCOMPtr<nsIRunnable> metadataLoadedEvent =
    new nsAudioMetadataEventRunner(mDecoder, mInfo.mAudioChannels, mInfo.mAudioRate);
  NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

  if (mState == DECODER_STATE_DECODING_METADATA) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from DECODING_METADATA to DECODING", mDecoder.get()));
    StartDecoding();
  }

  if ((mState == DECODER_STATE_DECODING || mState == DECODER_STATE_COMPLETED) &&
      mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING &&
      !IsPlaying())
  {
    StartPlayback();
  }

  return NS_OK;
}

void nsBuiltinDecoderStateMachine::DecodeSeek()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(mState == DECODER_STATE_SEEKING,
               "Only call when in seeking state");

  
  
  
  
  
  
  
  
  PRInt64 seekTime = mSeekTime;
  mDecoder->StopProgressUpdates();

  bool currentTimeChanged = false;
  PRInt64 mediaTime = GetMediaTime();
  if (mediaTime != seekTime) {
    currentTimeChanged = true;
    UpdatePlaybackPositionInternal(seekTime);
  }

  
  
  
  {
    ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
    nsCOMPtr<nsIRunnable> startEvent =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStarted);
    NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
  }

  if (currentTimeChanged) {
    
    
    
    StopPlayback();
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
      AudioData* audio = HasAudio() ? mReader->mAudioQueue.PeekFront() : nsnull;
      NS_ASSERTION(!audio || (audio->mTime <= seekTime &&
                              seekTime <= audio->mTime + audio->mDuration),
                    "Seek target should lie inside the first audio block after seek");
      PRInt64 startTime = (audio && audio->mTime < seekTime) ? audio->mTime : seekTime;
      mAudioStartTime = startTime;
      mPlayDuration = startTime - mStartTime;
      if (HasVideo()) {
        nsAutoPtr<VideoData> video(mReader->mVideoQueue.PeekFront());
        if (video) {
          NS_ASSERTION(video->mTime <= seekTime && seekTime <= video->mEndTime,
                        "Seek target should lie inside the first frame after seek");
          {
            ReentrantMonitorAutoExit exitMon(mDecoder->GetReentrantMonitor());
            RenderVideoFrame(video, TimeStamp::Now());
          }
          mReader->mVideoQueue.PopFront();
          nsCOMPtr<nsIRunnable> event =
            NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::Invalidate);
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
  if (GetMediaTime() == mEndTime) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lld) to COMPLETED",
                        mDecoder.get(), seekTime));
    stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStoppedAtEnd);
    mState = DECODER_STATE_COMPLETED;
  } else {
    LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lld) to DECODING",
                        mDecoder.get(), seekTime));
    stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStopped);
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
  nsDecoderDisposeEvent(already_AddRefed<nsBuiltinDecoder> aDecoder,
                        already_AddRefed<nsBuiltinDecoderStateMachine> aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}
  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Must be on main thread.");
    mStateMachine->ReleaseDecoder();
    mDecoder->ReleaseStateMachine();
    mStateMachine = nsnull;
    mDecoder = nsnull;
    return NS_OK;
  }
private:
  nsRefPtr<nsBuiltinDecoder> mDecoder;
  nsCOMPtr<nsBuiltinDecoderStateMachine> mStateMachine;
};





class nsDispatchDisposeEvent : public nsRunnable {
public:
  nsDispatchDisposeEvent(nsBuiltinDecoder* aDecoder,
                         nsBuiltinDecoderStateMachine* aStateMachine)
    : mDecoder(aDecoder), mStateMachine(aStateMachine) {}
  NS_IMETHOD Run() {
    NS_DispatchToMainThread(new nsDecoderDisposeEvent(mDecoder.forget(),
                                                      mStateMachine.forget()));
    return NS_OK;
  }
private:
  nsRefPtr<nsBuiltinDecoder> mDecoder;
  nsCOMPtr<nsBuiltinDecoderStateMachine> mStateMachine;
};

nsresult nsBuiltinDecoderStateMachine::RunStateMachine()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream, NS_ERROR_NULL_POINTER);

  switch (mState) {
    case DECODER_STATE_SHUTDOWN: {
      if (IsPlaying()) {
        StopPlayback();
      }
      StopAudioThread();
      StopDecodeThread();
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "How did we escape from the shutdown state?");
      
      
      
      
      
      
      
      
      
      
      
      
      
      NS_DispatchToCurrentThread(new nsDispatchDisposeEvent(mDecoder, this));
      return NS_OK;
    }

    case DECODER_STATE_DECODING_METADATA: {
      
      return StartDecodeThread();
    }
  
    case DECODER_STATE_DECODING: {
      if (mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING &&
          IsPlaying())
      {
        
        
        
        
        
        StopPlayback();
      }

      if (IsPausedAndDecoderWaiting()) {
        
        
        StopDecodeThread();
        return NS_OK;
      }

      
      
      if (NS_FAILED(StartDecodeThread())) {
        NS_WARNING("Failed to start media decode thread!");
        return NS_ERROR_FAILURE;
      }

      AdvanceFrame();
      NS_ASSERTION(mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING ||
                   IsStateMachineScheduled(), "Must have timer scheduled");
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
      bool isLiveStream = mDecoder->GetCurrentStream()->GetLength() == -1;
      if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
            elapsed < TimeDuration::FromSeconds(mBufferingWait) &&
            (mQuickBuffering ? HasLowDecodedData(QUICK_BUFFERING_LOW_DATA_USECS)
                            : (GetUndecodedData() < mBufferingWait * USECS_PER_S / 1000)) &&
            !stream->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !stream->IsSuspended())
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
      if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING &&
          !IsPlaying())
      {
        StartPlayback();
      }
      NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
      return NS_OK;
    }

    case DECODER_STATE_SEEKING: {
      
     return StartDecodeThread();
    }

    case DECODER_STATE_COMPLETED: {
      StopDecodeThread();

      if (mState != DECODER_STATE_COMPLETED) {
        
        
        
        
        NS_ASSERTION(IsStateMachineScheduled(), "Must have timer scheduled");
        return NS_OK;
      }

      
      
      
      if (mState == DECODER_STATE_COMPLETED &&
          (mReader->mVideoQueue.GetSize() > 0 ||
          (HasAudio() && !mAudioCompleted)))
      {
        AdvanceFrame();
        NS_ASSERTION(mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING ||
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
      if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
        PRInt64 videoTime = HasVideo() ? mVideoFrameEndTime : 0;
        PRInt64 clockTime = NS_MAX(mEndTime, NS_MAX(videoTime, GetAudioClock()));
        UpdatePlaybackPosition(clockTime);
        nsCOMPtr<nsIRunnable> event =
          NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
      }
      return NS_OK;
    }
  }

  return NS_OK;
}

void nsBuiltinDecoderStateMachine::RenderVideoFrame(VideoData* aData,
                                                    TimeStamp aTarget)
{
  NS_ASSERTION(OnStateMachineThread() || OnDecodeThread(),
               "Should be on state machine or decode thread.");
  mDecoder->GetReentrantMonitor().AssertNotCurrentThreadIn();

  if (aData->mDuplicate) {
    return;
  }

  nsRefPtr<Image> image = aData->mImage;
  if (image) {
    mDecoder->SetVideoData(aData->mDisplay, image, aTarget);
  }
}

PRInt64
nsBuiltinDecoderStateMachine::GetAudioClock()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  if (!HasAudio())
    return -1;
  
  
  
  if (!mAudioStream) {
    
    return mAudioStartTime;
  }
  
  
  
  PRInt64 t = mAudioStream->GetPosition();
  return (t == -1) ? -1 : t + mAudioStartTime;
}

void nsBuiltinDecoderStateMachine::AdvanceFrame()
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(!HasAudio() || mAudioStartTime != -1,
               "Should know audio start time if we have audio.");

  if (mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING) {
    return;
  }

  
  
  
  PRInt64 clock_time = -1;
  if (!IsPlaying()) {
    clock_time = mPlayDuration + mStartTime;
  } else {
    PRInt64 audio_time = GetAudioClock();
    if (HasAudio() && !mAudioCompleted && audio_time != -1) {
      clock_time = audio_time;
      
      
      mPlayDuration = clock_time - mStartTime;
      mPlayStartTime = TimeStamp::Now();
    } else {
      
      clock_time = DurationToUsecs(TimeStamp::Now() - mPlayStartTime) + mPlayDuration;
      
      NS_ASSERTION(mCurrentFrameTime <= clock_time, "Clock should go forwards");
      clock_time = NS_MAX(mCurrentFrameTime, clock_time) + mStartTime;
    }
  }

  
  
  PRInt64 remainingTime = AUDIO_DURATION_USECS;
  NS_ASSERTION(clock_time >= mStartTime, "Should have positive clock time.");
  nsAutoPtr<VideoData> currentFrame;
  if (mReader->mVideoQueue.GetSize() > 0) {
    VideoData* frame = mReader->mVideoQueue.PeekFront();
    while (mRealTime || clock_time >= frame->mTime) {
      mVideoFrameEndTime = frame->mEndTime;
      currentFrame = frame;
      mReader->mVideoQueue.PopFront();
      
      
      mDecoder->GetReentrantMonitor().NotifyAll();
      mDecoder->UpdatePlaybackOffset(frame->mOffset);
      if (mReader->mVideoQueue.GetSize() == 0)
        break;
      frame = mReader->mVideoQueue.PeekFront();
    }
    
    
    if (frame && !currentFrame) {
      PRInt64 now = IsPlaying()
        ? (DurationToUsecs(TimeStamp::Now() - mPlayStartTime) + mPlayDuration)
        : mPlayDuration;
      remainingTime = frame->mTime - mStartTime - now;
    }
  }

  
  
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  if (mState == DECODER_STATE_DECODING &&
      mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING &&
      HasLowDecodedData(remainingTime + EXHAUSTED_DATA_MARGIN_USECS) &&
      !stream->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
      !stream->IsSuspended() &&
      (JustExitedQuickBuffering() || HasLowUndecodedData()))
  {
    if (currentFrame) {
      mReader->mVideoQueue.PushFront(currentFrame.forget());
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
    mDecoder->GetFrameStatistics().NotifyPresentedFrame();
    PRInt64 now = DurationToUsecs(TimeStamp::Now() - mPlayStartTime) + mPlayDuration;
    remainingTime = currentFrame->mEndTime - mStartTime - now;
    currentFrame = nsnull;
  }

  
  
  
  if (mVideoFrameEndTime != -1 || mAudioEndTime != -1) {
    
    clock_time = NS_MIN(clock_time, NS_MAX(mVideoFrameEndTime, mAudioEndTime));
    if (clock_time > GetMediaTime()) {
      
      
      
      
      UpdatePlaybackPosition(clock_time);
    }
  }

  
  
  
  
  UpdateReadyState();

  ScheduleStateMachine(remainingTime);
}

void nsBuiltinDecoderStateMachine::Wait(PRInt64 aUsecs) {
  NS_ASSERTION(OnAudioThread(), "Only call on the audio thread");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  TimeStamp end = TimeStamp::Now() + UsecsToDuration(NS_MAX<PRInt64>(USECS_PER_MS, aUsecs));
  TimeStamp now;
  while ((now = TimeStamp::Now()) < end &&
         mState != DECODER_STATE_SHUTDOWN &&
         mState != DECODER_STATE_SEEKING &&
         !mStopAudioThread &&
         IsPlaying())
  {
    PRInt64 ms = static_cast<PRInt64>(NS_round((end - now).ToSeconds() * 1000));
    if (ms == 0 || ms > PR_UINT32_MAX) {
      break;
    }
    mDecoder->GetReentrantMonitor().Wait(PR_MillisecondsToInterval(static_cast<PRUint32>(ms)));
  }
}

VideoData* nsBuiltinDecoderStateMachine::FindStartTime()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  PRInt64 startTime = 0;
  mStartTime = 0;
  VideoData* v = nsnull;
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

void nsBuiltinDecoderStateMachine::UpdateReadyState() {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();

  nsCOMPtr<nsIRunnable> event;
  switch (GetNextFrameStatus()) {
    case nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING:
      event = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::NextFrameUnavailableBuffering);
      break;
    case nsHTMLMediaElement::NEXT_FRAME_AVAILABLE:
      event = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::NextFrameAvailable);
      break;
    case nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE:
      event = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::NextFrameUnavailable);
      break;
    default:
      PR_NOT_REACHED("unhandled frame state");
  }

  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

bool nsBuiltinDecoderStateMachine::JustExitedQuickBuffering()
{
  return !mDecodeStartTime.IsNull() &&
    mQuickBuffering &&
    (TimeStamp::Now() - mDecodeStartTime) < TimeDuration::FromSeconds(QUICK_BUFFER_THRESHOLD_USECS);
}

void nsBuiltinDecoderStateMachine::StartBuffering()
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
  nsMediaDecoder::Statistics stats = mDecoder->GetStatistics();
  LOG(PR_LOG_DEBUG, ("%p Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
    mDecoder.get(),
    stats.mPlaybackRate/1024, stats.mPlaybackRateReliable ? "" : " (unreliable)",
    stats.mDownloadRate/1024, stats.mDownloadRateReliable ? "" : " (unreliable)"));
}

nsresult nsBuiltinDecoderStateMachine::GetBuffered(nsTimeRanges* aBuffered) {
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream, NS_ERROR_FAILURE);
  stream->Pin();
  nsresult res = mReader->GetBuffered(aBuffered, mStartTime);
  stream->Unpin();
  return res;
}

bool nsBuiltinDecoderStateMachine::IsPausedAndDecoderWaiting() {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  return
    mDecodeThreadWaiting &&
    mDecoder->GetState() != nsBuiltinDecoder::PLAY_STATE_PLAYING &&
    (mState == DECODER_STATE_DECODING || mState == DECODER_STATE_BUFFERING);
}

nsresult nsBuiltinDecoderStateMachine::Run()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");

  return CallRunStateMachine();
}

nsresult nsBuiltinDecoderStateMachine::CallRunStateMachine()
{
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  
  
  mRunAgain = false;

  
  
  mDispatchedRunEvent = false;

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
  nsBuiltinDecoderStateMachine *machine =
    static_cast<nsBuiltinDecoderStateMachine*>(aClosure);
  NS_ASSERTION(machine, "Must have been passed state machine");
  machine->TimeoutExpired();
}

void nsBuiltinDecoderStateMachine::TimeoutExpired()
{
  ReentrantMonitorAutoEnter mon(mDecoder->GetReentrantMonitor());
  NS_ASSERTION(OnStateMachineThread(), "Must be on state machine thread");
  if (mIsRunning) {
    mRunAgain = true;
  } else if (!mDispatchedRunEvent) {
    
    
    CallRunStateMachine();
  }
  
  
  
}

nsresult nsBuiltinDecoderStateMachine::ScheduleStateMachine() {
  return ScheduleStateMachine(0);
}

nsresult nsBuiltinDecoderStateMachine::ScheduleStateMachine(PRInt64 aUsecs) {
  mDecoder->GetReentrantMonitor().AssertCurrentThreadIn();
  NS_ABORT_IF_FALSE(gStateMachineThread,
    "Must have a state machine thread to schedule");

  if (mState == DECODER_STATE_SHUTDOWN) {
    return NS_ERROR_FAILURE;
  }
  aUsecs = PR_MAX(aUsecs, 0);

  TimeStamp timeout = TimeStamp::Now() + UsecsToDuration(aUsecs);
  if (!mTimeout.IsNull()) {
    if (timeout >= mTimeout) {
      
      
      return NS_OK;
    }
    if (mTimer) {
      
      
      mTimer->Cancel();
    }
  }

  PRUint32 ms = static_cast<PRUint32>((aUsecs / USECS_PER_MS) & 0xFFFFFFFF);
  if (mRealTime && ms > 40)
    ms = 40;
  if (ms == 0) {
    if (mIsRunning) {
      
      
      mRunAgain = true;
      return NS_OK;
    } else if (!mDispatchedRunEvent) {
      
      
      mDispatchedRunEvent = true;
      return gStateMachineThread->Dispatch(this, NS_DISPATCH_NORMAL);
    }
    
    
    
    return NS_OK;
  }

  mTimeout = timeout;

  nsresult res;
  if (!mTimer) {
    mTimer = do_CreateInstance("@mozilla.org/timer;1", &res);
    if (NS_FAILED(res)) return res;
    mTimer->SetTarget(gStateMachineThread);
  }

  res = mTimer->InitWithFuncCallback(::TimeoutExpired,
                                     this,
                                     ms,
                                     nsITimer::TYPE_ONE_SHOT);
  return res;
}
