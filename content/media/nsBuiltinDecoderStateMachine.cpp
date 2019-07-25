





































#include <limits>
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"
#include "nsTimeRanges.h"

using namespace mozilla;
using namespace mozilla::layers;

#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif




#define BUFFERING_WAIT 30





#define BUFFERING_MIN_RATE 50000
#define BUFFERING_RATE(x) ((x)< BUFFERING_MIN_RATE ? BUFFERING_MIN_RATE : (x))





static const PRUint32 LOW_AUDIO_MS = 300;





const PRInt64 AMPLE_AUDIO_MS = 1000;






const PRUint32 SILENCE_BYTES_CHUNK = 32 * 1024;




static const PRUint32 LOW_VIDEO_FRAMES = 1;




static const PRUint32 AMPLE_VIDEO_FRAMES = 10;


static const int AUDIO_DURATION_MS = 40;





static const int THRESHOLD_FACTOR = 2;





static const PRInt64 LOW_DATA_THRESHOLD_MS = 5000;



PR_STATIC_ASSERT(LOW_DATA_THRESHOLD_MS > AMPLE_AUDIO_MS);


static const PRUint32 EXHAUSTED_DATA_MARGIN_MS = 60;










static const PRUint32 QUICK_BUFFER_THRESHOLD_MS = 2000;



static const PRUint32 QUICK_BUFFERING_LOW_DATA_MS = 1000;





PR_STATIC_ASSERT(QUICK_BUFFERING_LOW_DATA_MS <= AMPLE_AUDIO_MS);

static TimeDuration MsToDuration(PRInt64 aMs) {
  return TimeDuration::FromMilliseconds(static_cast<double>(aMs));
}

static PRInt64 DurationToMs(TimeDuration aDuration) {
  return static_cast<PRInt64>(aDuration.ToSeconds() * 1000);
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

nsBuiltinDecoderStateMachine::nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder,
                                                           nsBuiltinDecoderReader* aReader) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mAudioMonitor("media.audiostream"),
  mCbCrSize(0),
  mPlayDuration(0),
  mStartTime(-1),
  mEndTime(-1),
  mSeekTime(0),
  mReader(aReader),
  mCurrentFrameTime(0),
  mAudioStartTime(-1),
  mAudioEndTime(-1),
  mVideoFrameEndTime(-1),
  mVolume(1.0),
  mSeekable(PR_TRUE),
  mPositionChangeQueued(PR_FALSE),
  mAudioCompleted(PR_FALSE),
  mGotDurationFromMetaData(PR_FALSE),
  mStopDecodeThreads(PR_TRUE),
  mQuickBuffering(PR_FALSE),
  mEventManager(aDecoder)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoderStateMachine);
}

nsBuiltinDecoderStateMachine::~nsBuiltinDecoderStateMachine()
{
  MOZ_COUNT_DTOR(nsBuiltinDecoderStateMachine);
}

PRBool nsBuiltinDecoderStateMachine::HasFutureAudio() const {
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  NS_ASSERTION(HasAudio(), "Should only call HasFutureAudio() when we have audio");
  
  
  
  
  
  return !mAudioCompleted &&
         (AudioDecodedMs() > LOW_AUDIO_MS || mReader->mAudioQueue.IsFinished());
}

PRBool nsBuiltinDecoderStateMachine::HaveNextFrameData() const {
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  return (!HasAudio() || HasFutureAudio()) &&
         (!HasVideo() || mReader->mVideoQueue.GetSize() > 0);
}

PRInt64 nsBuiltinDecoderStateMachine::GetDecodedAudioDuration() {
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  PRInt64 audioDecoded = mReader->mAudioQueue.Duration();
  if (mAudioEndTime != -1) {
    audioDecoded += mAudioEndTime - GetMediaTime();
  }
  return audioDecoded;
}

void nsBuiltinDecoderStateMachine::DecodeLoop()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");

  
  
  PRBool audioPump = PR_TRUE;
  PRBool videoPump = PR_TRUE;

  
  
  
  
  PRBool skipToNextKeyframe = PR_FALSE;

  
  
  const unsigned videoPumpThreshold = AMPLE_VIDEO_FRAMES / 2;

  
  
  
  const unsigned audioPumpThresholdMs = LOW_AUDIO_MS * 2;

  
  
  PRInt64 lowAudioThreshold = LOW_AUDIO_MS;

  
  
  
  PRInt64 ampleAudioThreshold = AMPLE_AUDIO_MS;

  MediaQueue<VideoData>& videoQueue = mReader->mVideoQueue;
  MediaQueue<SoundData>& audioQueue = mReader->mAudioQueue;

  MonitorAutoEnter mon(mDecoder->GetMonitor());

  PRBool videoPlaying = HasVideo();
  PRBool audioPlaying = HasAudio();

  
  while (mState != DECODER_STATE_SHUTDOWN &&
         !mStopDecodeThreads &&
         (videoPlaying || audioPlaying))
  {
    
    
    
    if (videoPump &&
        static_cast<PRUint32>(videoQueue.GetSize()) >= videoPumpThreshold)
    {
      videoPump = PR_FALSE;
    }

    
    
    
    if (audioPump && GetDecodedAudioDuration() >= audioPumpThresholdMs) {
      audioPump = PR_FALSE;
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
      skipToNextKeyframe = PR_TRUE;
      LOG(PR_LOG_DEBUG, ("Skipping video decode to the next keyframe"));
    }

    
    if (videoPlaying &&
        static_cast<PRUint32>(videoQueue.GetSize()) < AMPLE_VIDEO_FRAMES)
    {
      
      
      
      TimeDuration decodeTime;
      {
        PRInt64 currentTime = GetMediaTime();
        MonitorAutoExit exitMon(mDecoder->GetMonitor());
        TimeStamp start = TimeStamp::Now();
        videoPlaying = mReader->DecodeVideoFrame(skipToNextKeyframe, currentTime);
        decodeTime = TimeStamp::Now() - start;
      }
      if (THRESHOLD_FACTOR * DurationToMs(decodeTime) > lowAudioThreshold &&
          !HasLowUndecodedData())
      {
        lowAudioThreshold =
          NS_MIN(THRESHOLD_FACTOR * DurationToMs(decodeTime), AMPLE_AUDIO_MS);
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
      MonitorAutoExit exitMon(mDecoder->GetMonitor());
      audioPlaying = mReader->DecodeAudioData();
    }
    
    
    
    mDecoder->GetMonitor().NotifyAll();

    if (!IsPlaying()) {
      
      
      
      UpdateReadyState();
    }

    if (mState != DECODER_STATE_SHUTDOWN &&
        !mStopDecodeThreads &&
        (!audioPlaying || (GetDecodedAudioDuration() >= ampleAudioThreshold &&
                           audioQueue.GetSize() > 0))
        &&
        (!videoPlaying ||
          static_cast<PRUint32>(videoQueue.GetSize()) >= AMPLE_VIDEO_FRAMES))
    {
      
      
      
      
      
      
      
      
      
      mon.Wait();
    }

  } 

  if (!mStopDecodeThreads &&
      mState != DECODER_STATE_SHUTDOWN &&
      mState != DECODER_STATE_SEEKING)
  {
    mState = DECODER_STATE_COMPLETED;
    mDecoder->GetMonitor().NotifyAll();
  }

  LOG(PR_LOG_DEBUG, ("Shutting down DecodeLoop this=%p", this));
}

PRBool nsBuiltinDecoderStateMachine::IsPlaying()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  return !mPlayStartTime.IsNull();
}

void nsBuiltinDecoderStateMachine::AudioLoop()
{
  NS_ASSERTION(OnAudioThread(), "Should be on audio thread.");
  LOG(PR_LOG_DEBUG, ("Begun audio thread/loop"));
  PRInt64 audioDuration = 0;
  PRInt64 audioStartTime = -1;
  PRUint32 channels, rate;
  double volume = -1;
  PRBool setVolume;
  PRInt32 minWriteSamples = -1;
  PRInt64 samplesAtLastSleep = 0;
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    mAudioCompleted = PR_FALSE;
    audioStartTime = mAudioStartTime;
    channels = mInfo.mAudioChannels;
    rate = mInfo.mAudioRate;
    NS_ASSERTION(audioStartTime != -1, "Should have audio start time by now");
  }
  while (1) {

    
    
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      NS_ASSERTION(mState != DECODER_STATE_DECODING_METADATA,
                   "Should have meta data before audio started playing.");
      while (mState != DECODER_STATE_SHUTDOWN &&
             !mStopDecodeThreads &&
             (!IsPlaying() ||
              mState == DECODER_STATE_BUFFERING ||
              (mReader->mAudioQueue.GetSize() == 0 &&
               !mReader->mAudioQueue.AtEndOfStream())))
      {
        samplesAtLastSleep = audioDuration;
        mon.Wait();
      }

      
      if (mState == DECODER_STATE_SHUTDOWN ||
          mStopDecodeThreads ||
          mReader->mAudioQueue.AtEndOfStream())
      {
        break;
      }

      
      
      
      
      setVolume = volume != mVolume;
      volume = mVolume;
    }

    if (setVolume || minWriteSamples == -1) {
      MonitorAutoEnter audioMon(mAudioMonitor);
      if (mAudioStream) {
        if (setVolume) {
          mAudioStream->SetVolume(volume);
        }
        if (minWriteSamples == -1) {
          minWriteSamples = mAudioStream->GetMinWriteSamples();
        }
      }
    }
    NS_ASSERTION(mReader->mAudioQueue.GetSize() > 0,
                 "Should have data to play");
    
    
    const SoundData* s = mReader->mAudioQueue.PeekFront();

    
    
    PRInt64 playedSamples = 0;
    if (!MsToSamples(audioStartTime, rate, playedSamples)) {
      NS_WARNING("Int overflow converting playedSamples");
      break;
    }
    if (!AddOverflow(playedSamples, audioDuration, playedSamples)) {
      NS_WARNING("Int overflow adding playedSamples");
      break;
    }

    
    
    PRInt64 sampleTime = 0;
    if (!MsToSamples(s->mTime, rate, sampleTime)) {
      NS_WARNING("Int overflow converting sampleTime");
      break;
    }
    PRInt64 missingSamples = 0;
    if (!AddOverflow(sampleTime, -playedSamples, missingSamples)) {
      NS_WARNING("Int overflow adding missingSamples");
      break;
    }

    if (missingSamples > 0) {
      
      
      
      
      missingSamples = NS_MIN(static_cast<PRInt64>(PR_UINT32_MAX), missingSamples);
      audioDuration += PlaySilence(static_cast<PRUint32>(missingSamples),
                                   channels, playedSamples);
    } else {
      audioDuration += PlayFromAudioQueue(sampleTime, channels);
    }
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      PRInt64 playedMs;
      if (!SamplesToMs(audioDuration, rate, playedMs)) {
        NS_WARNING("Int overflow calculating playedMs");
        break;
      }
      if (!AddOverflow(audioStartTime, playedMs, mAudioEndTime)) {
        NS_WARNING("Int overflow calculating audio end time");
        break;
      }

      PRInt64 audioAhead = mAudioEndTime - GetMediaTime();
      if (audioAhead > AMPLE_AUDIO_MS &&
          audioDuration - samplesAtLastSleep > minWriteSamples)
      {
        samplesAtLastSleep = audioDuration;
        
        
        
        
        Wait(AMPLE_AUDIO_MS / 2);
        
        
        
        
        
        
        mon.NotifyAll();
      }
    }
  }
  if (mReader->mAudioQueue.AtEndOfStream() &&
      mState != DECODER_STATE_SHUTDOWN &&
      !mStopDecodeThreads)
  {
    
    
    MonitorAutoEnter audioMon(mAudioMonitor);
    if (mAudioStream) {
      PRBool seeking = PR_FALSE;
      PRInt64 oldPosition = -1;

      {
        MonitorAutoExit audioExit(mAudioMonitor);
        MonitorAutoEnter mon(mDecoder->GetMonitor());
        PRInt64 position = GetMediaTime();
        while (oldPosition != position &&
               mAudioEndTime - position > 0 &&
               mState != DECODER_STATE_SEEKING &&
               mState != DECODER_STATE_SHUTDOWN)
        {
          const PRInt64 DRAIN_BLOCK_MS = 100;
          Wait(NS_MIN(mAudioEndTime - position, DRAIN_BLOCK_MS));
          oldPosition = position;
          position = GetMediaTime();
        }
        if (mState == DECODER_STATE_SEEKING) {
          seeking = PR_TRUE;
        }
      }

      if (!seeking && mAudioStream && !mAudioStream->IsPaused()) {
        mAudioStream->Drain();

        
        mEventManager.Drain(mAudioEndTime);
      }
    }
    LOG(PR_LOG_DEBUG, ("%p Reached audio stream end.", mDecoder));
  }
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    mAudioCompleted = PR_TRUE;
    UpdateReadyState();
    
    
    mDecoder->GetMonitor().NotifyAll();
  }
  LOG(PR_LOG_DEBUG, ("Audio stream finished playing, audio thread exit"));
}

PRUint32 nsBuiltinDecoderStateMachine::PlaySilence(PRUint32 aSamples,
                                                   PRUint32 aChannels,
                                                   PRUint64 aSampleOffset)

{
  MonitorAutoEnter audioMon(mAudioMonitor);
  if (!mAudioStream || mAudioStream->IsPaused()) {
    
    
    return 0;
  }
  PRUint32 maxSamples = SILENCE_BYTES_CHUNK / aChannels;
  PRUint32 samples = NS_MIN(aSamples, maxSamples);
  PRUint32 numValues = samples * aChannels;
  nsAutoArrayPtr<SoundDataValue> buf(new SoundDataValue[numValues]);
  memset(buf.get(), 0, sizeof(SoundDataValue) * numValues);
  mAudioStream->Write(buf, numValues, PR_TRUE);
  
  mEventManager.QueueWrittenAudioData(buf.get(), numValues,
                                      (aSampleOffset + samples) * aChannels);
  return samples;
}

PRUint32 nsBuiltinDecoderStateMachine::PlayFromAudioQueue(PRUint64 aSampleOffset,
                                                          PRUint32 aChannels)
{
  nsAutoPtr<SoundData> sound(mReader->mAudioQueue.PopFront());
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    NS_WARN_IF_FALSE(IsPlaying(), "Should be playing");
    
    
    mDecoder->GetMonitor().NotifyAll();
  }
  PRInt64 offset = -1;
  PRUint32 samples = 0;
  {
    MonitorAutoEnter audioMon(mAudioMonitor);
    if (!mAudioStream) {
      return 0;
    }
    
    
    
    
    
    
    
    if (!mAudioStream->IsPaused()) {
      mAudioStream->Write(sound->mAudioData,
                          sound->AudioDataLength(),
                          PR_TRUE);

      offset = sound->mOffset;
      samples = sound->mSamples;

      
      mEventManager.QueueWrittenAudioData(sound->mAudioData.get(),
                                          sound->AudioDataLength(),
                                          (aSampleOffset + samples) * aChannels);
    } else {
      mReader->mAudioQueue.PushFront(sound);
      sound.forget();
    }
  }
  if (offset != -1) {
    mDecoder->UpdatePlaybackOffset(offset);
  }
  return samples;
}

nsresult nsBuiltinDecoderStateMachine::Init(nsDecoderStateMachine* aCloneDonor)
{
  nsBuiltinDecoderReader* cloneReader = nsnull;
  if (aCloneDonor) {
    cloneReader = static_cast<nsBuiltinDecoderStateMachine*>(aCloneDonor)->mReader;
  }
  return mReader->Init(cloneReader);
}

void nsBuiltinDecoderStateMachine::StopPlayback(eStopMode aMode)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  mDecoder->mPlaybackStatistics.Stop(TimeStamp::Now());

  
  
  
  
  
  if (IsPlaying()) {
    mPlayDuration += TimeStamp::Now() - mPlayStartTime;
    mPlayStartTime = TimeStamp();
  }
  if (HasAudio()) {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    MonitorAutoEnter audioMon(mAudioMonitor);
    if (mAudioStream) {
      if (aMode == AUDIO_PAUSE) {
        mAudioStream->Pause();
      } else if (aMode == AUDIO_SHUTDOWN) {
        mAudioStream->Shutdown();
        mAudioStream = nsnull;
        mEventManager.Clear();
      }
    }
  }
}

void nsBuiltinDecoderStateMachine::StartPlayback()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  NS_ASSERTION(!IsPlaying(), "Shouldn't be playing when StartPlayback() is called");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  LOG(PR_LOG_DEBUG, ("%p StartPlayback", mDecoder));
  mDecoder->mPlaybackStatistics.Start(TimeStamp::Now());
  if (HasAudio()) {
    PRInt32 rate = mInfo.mAudioRate;
    PRInt32 channels = mInfo.mAudioChannels;

    {
      MonitorAutoExit exitMon(mDecoder->GetMonitor());
      MonitorAutoEnter audioMon(mAudioMonitor);
      if (mAudioStream) {
        
        
        mAudioStream->Resume();
      } else {
        
        mAudioStream = nsAudioStream::AllocateStream();
        mAudioStream->Init(channels, rate, MOZ_SOUND_DATA_FORMAT);
        mAudioStream->SetVolume(mVolume);
      }
    }
  }
  mPlayStartTime = TimeStamp::Now();
  mDecoder->GetMonitor().NotifyAll();
}

void nsBuiltinDecoderStateMachine::UpdatePlaybackPositionInternal(PRInt64 aTime)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

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

  if (!mPositionChangeQueued) {
    mPositionChangeQueued = PR_TRUE;
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::PlaybackPositionChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }

  
  mEventManager.DispatchPendingEvents(GetMediaTime());
}

void nsBuiltinDecoderStateMachine::ClearPositionChangeFlag()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  mPositionChangeQueued = PR_FALSE;
}

nsHTMLMediaElement::NextFrameStatus nsBuiltinDecoderStateMachine::GetNextFrameStatus()
{
  MonitorAutoEnter mon(mDecoder->GetMonitor());
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
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  mVolume = volume;
}

double nsBuiltinDecoderStateMachine::GetCurrentTime() const
{
  NS_ASSERTION(NS_IsMainThread() ||
               mDecoder->OnStateMachineThread() ||
               OnDecodeThread(),
               "Should be on main, decode, or state machine thread.");

  return static_cast<double>(mCurrentFrameTime) / 1000.0;
}

PRInt64 nsBuiltinDecoderStateMachine::GetDuration()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

void nsBuiltinDecoderStateMachine::SetDuration(PRInt64 aDuration)
{
  NS_ASSERTION(NS_IsMainThread() || mDecoder->OnStateMachineThread(),
    "Should be on main or state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  if (mStartTime != -1) {
    mEndTime = mStartTime + aDuration;
  } else {
    mStartTime = 0;
    mEndTime = aDuration;
  }
}

void nsBuiltinDecoderStateMachine::SetSeekable(PRBool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  mSeekable = aSeekable;
}

void nsBuiltinDecoderStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  MonitorAutoEnter mon(mDecoder->GetMonitor());

  
  
  LOG(PR_LOG_DEBUG, ("%p Changed state to SHUTDOWN", mDecoder));
  mState = DECODER_STATE_SHUTDOWN;
  mDecoder->GetMonitor().NotifyAll();
}

void nsBuiltinDecoderStateMachine::StartDecoding()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  if (mState != DECODER_STATE_DECODING) {
    mDecodeStartTime = TimeStamp::Now();
  }
  mState = DECODER_STATE_DECODING;
}

void nsBuiltinDecoderStateMachine::Play()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder));
    mState = DECODER_STATE_DECODING;
    mDecodeStartTime = TimeStamp::Now();
    mDecoder->GetMonitor().NotifyAll();
  }
}

void nsBuiltinDecoderStateMachine::ResetPlayback()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mVideoFrameEndTime = -1;
  mAudioStartTime = -1;
  mAudioEndTime = -1;
  mAudioCompleted = PR_FALSE;
}

void nsBuiltinDecoderStateMachine::Seek(double aTime)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  
  
  NS_ASSERTION(mState != DECODER_STATE_SEEKING,
               "We shouldn't already be seeking");
  NS_ASSERTION(mState >= DECODER_STATE_DECODING,
               "We should have loaded metadata");
  double t = aTime * 1000.0;
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
  LOG(PR_LOG_DEBUG, ("%p Changed state to SEEKING (to %f)", mDecoder, aTime));
  mState = DECODER_STATE_SEEKING;
}

void nsBuiltinDecoderStateMachine::StopDecodeThreads()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  mStopDecodeThreads = PR_TRUE;
  mDecoder->GetMonitor().NotifyAll();
  if (mDecodeThread) {
    {
      MonitorAutoExit exitMon(mDecoder->GetMonitor());
      mDecodeThread->Shutdown();
    }
    mDecodeThread = nsnull;
  }
  if (mAudioThread) {
    {
      MonitorAutoExit exitMon(mDecoder->GetMonitor());
      mAudioThread->Shutdown();
    }
    mAudioThread = nsnull;
  }
}

nsresult
nsBuiltinDecoderStateMachine::StartDecodeThreads()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  mStopDecodeThreads = PR_FALSE;
  if (!mDecodeThread && mState < DECODER_STATE_COMPLETED) {
    nsresult rv = NS_NewThread(getter_AddRefs(mDecodeThread));
    if (NS_FAILED(rv)) {
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsBuiltinDecoderStateMachine::DecodeLoop);
    mDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }
  if (HasAudio() && !mAudioThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mAudioThread));
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

PRInt64 nsBuiltinDecoderStateMachine::AudioDecodedMs() const
{
  NS_ASSERTION(HasAudio(),
               "Should only call AudioDecodedMs() when we have audio");
  
  
  
  PRInt64 pushed = (mAudioEndTime != -1) ? (mAudioEndTime - GetMediaTime()) : 0;
  return pushed + mReader->mAudioQueue.Duration();
}

PRBool nsBuiltinDecoderStateMachine::HasLowDecodedData(PRInt64 aAudioMs) const
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  
  
  
  
  return ((HasAudio() &&
           !mReader->mAudioQueue.IsFinished() &&
           AudioDecodedMs() < aAudioMs)
          ||
         (!HasAudio() &&
          HasVideo() &&
          !mReader->mVideoQueue.IsFinished() &&
          static_cast<PRUint32>(mReader->mVideoQueue.GetSize()) < LOW_VIDEO_FRAMES));
}

PRBool nsBuiltinDecoderStateMachine::HasLowUndecodedData() const
{
  return GetUndecodedData() < LOW_DATA_THRESHOLD_MS;
}

PRInt64 nsBuiltinDecoderStateMachine::GetUndecodedData() const
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();
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
      return static_cast<PRInt64>((end - currentTime) * 1000);
    }
  }
  return 0;
}

void nsBuiltinDecoderStateMachine::SetFrameBufferLength(PRUint32 aLength)
{
  NS_ASSERTION(aLength >= 512 && aLength <= 16384,
               "The length must be between 512 and 16384");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  mEventManager.SetSignalBufferLength(aLength);
}

nsresult nsBuiltinDecoderStateMachine::Run()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  nsMediaStream* stream = mDecoder->GetCurrentStream();
  NS_ENSURE_TRUE(stream, NS_ERROR_NULL_POINTER);

  while (PR_TRUE) {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    switch (mState) {
    case DECODER_STATE_SHUTDOWN:
      if (IsPlaying()) {
        StopPlayback(AUDIO_SHUTDOWN);
      }
      StopDecodeThreads();
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "How did we escape from the shutdown state???");
      return NS_OK;

    case DECODER_STATE_DECODING_METADATA:
      {
        LoadMetadata();
        if (mState == DECODER_STATE_SHUTDOWN) {
          continue;
        }

        VideoData* videoData = FindStartTime();
        if (videoData) {
          nsIntSize display = mInfo.mDisplay;
          float aspect = mInfo.mPixelAspectRatio;
          {
            MonitorAutoExit exitMon(mDecoder->GetMonitor());
            RenderVideoFrame(videoData, TimeStamp::Now(), display, aspect);
          }
        }

        
        
        if (NS_FAILED(StartDecodeThreads())) {
          continue;
        }

        NS_ASSERTION(mStartTime != -1, "Must have start time");
        NS_ASSERTION((!HasVideo() && !HasAudio()) ||
                     !mSeekable || mEndTime != -1,
                     "Active seekable media should have end time");
        NS_ASSERTION(!mSeekable || GetDuration() != -1, "Seekable media should have duration");
        LOG(PR_LOG_DEBUG, ("%p Media goes from %lldms to %lldms (duration %lldms) seekable=%d",
                           mDecoder, mStartTime, mEndTime, GetDuration(), mSeekable));

        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        
        
        
        
        if (HasAudio()) {
          mEventManager.Init(mInfo.mAudioChannels, mInfo.mAudioRate);
          
          
          
          PRUint32 frameBufferLength = mInfo.mAudioChannels * FRAMEBUFFER_LENGTH_PER_CHANNEL;
          mDecoder->RequestFrameBufferLength(frameBufferLength);
        }
        nsCOMPtr<nsIRunnable> metadataLoadedEvent =
          new nsAudioMetadataEventRunner(mDecoder, mInfo.mAudioChannels, mInfo.mAudioRate);
        NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

        if (mState == DECODER_STATE_DECODING_METADATA) {
          LOG(PR_LOG_DEBUG, ("%p Changed state from DECODING_METADATA to DECODING", mDecoder));
          StartDecoding();
        }

        
        if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
          if (!IsPlaying()) {
            StartPlayback();
          }
        }
      }
      break;

    case DECODER_STATE_DECODING:
      {
        if (NS_FAILED(StartDecodeThreads())) {
          continue;
        }

        AdvanceFrame();

        if (mState != DECODER_STATE_DECODING)
          continue;
      }
      break;

    case DECODER_STATE_SEEKING:
      {
        
        
        
        
        
        
        
        
        PRInt64 seekTime = mSeekTime;
        mDecoder->StopProgressUpdates();

        PRBool currentTimeChanged = false;
        PRInt64 mediaTime = GetMediaTime();
        if (mediaTime != seekTime) {
          currentTimeChanged = true;
          UpdatePlaybackPositionInternal(seekTime);
        }

        
        
        
        {
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          nsCOMPtr<nsIRunnable> startEvent =
            NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStarted);
          NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        }

        if (currentTimeChanged) {
          
          
          
          StopPlayback(AUDIO_SHUTDOWN);
          StopDecodeThreads();
          ResetPlayback();
          nsresult res;
          {
            MonitorAutoExit exitMon(mDecoder->GetMonitor());
            
            
            res = mReader->Seek(seekTime,
                                mStartTime,
                                mEndTime,
                                mediaTime);
          }
          if (NS_SUCCEEDED(res)){
            SoundData* audio = HasAudio() ? mReader->mAudioQueue.PeekFront() : nsnull;
            NS_ASSERTION(!audio || (audio->mTime <= seekTime &&
                                    seekTime <= audio->mTime + audio->mDuration),
                         "Seek target should lie inside the first audio block after seek");
            PRInt64 startTime = (audio && audio->mTime < seekTime) ? audio->mTime : seekTime;
            mAudioStartTime = startTime;
            mPlayDuration = MsToDuration(startTime - mStartTime);
            if (HasVideo()) {
              nsAutoPtr<VideoData> video(mReader->mVideoQueue.PeekFront());
              if (video) {
                NS_ASSERTION(video->mTime <= seekTime && seekTime <= video->mEndTime,
                             "Seek target should lie inside the first frame after seek");
                nsIntSize display = mInfo.mDisplay;
                float aspect = mInfo.mPixelAspectRatio;
                {
                  MonitorAutoExit exitMon(mDecoder->GetMonitor());
                  RenderVideoFrame(video, TimeStamp::Now(), display, aspect);
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
          continue;

        
        LOG(PR_LOG_DEBUG, ("Seek completed, mCurrentFrameTime=%lld\n", mCurrentFrameTime));

        
        
        
        
        nsCOMPtr<nsIRunnable> stopEvent;
        if (GetMediaTime() == mEndTime) {
          LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lldms) to COMPLETED",
                             mDecoder, seekTime));
          stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStoppedAtEnd);
          mState = DECODER_STATE_COMPLETED;
        } else {
          LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lldms) to DECODING",
                             mDecoder, seekTime));
          stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStopped);
          StartDecoding();
        }
        mDecoder->GetMonitor().NotifyAll();

        {
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
        }

        
        
        
        mQuickBuffering = PR_FALSE;
      }
      break;

    case DECODER_STATE_BUFFERING:
      {
        if (IsPlaying()) {
          StopPlayback(AUDIO_PAUSE);
          mDecoder->GetMonitor().NotifyAll();
        }

        TimeStamp now = TimeStamp::Now();
        NS_ASSERTION(!mBufferingStart.IsNull(), "Must know buffering start time.");

        
        
        
        TimeDuration elapsed = now - mBufferingStart;
        PRBool isLiveStream = mDecoder->GetCurrentStream()->GetLength() == -1;
        if ((isLiveStream || !mDecoder->CanPlayThrough()) &&
             elapsed < TimeDuration::FromSeconds(BUFFERING_WAIT) &&
             (mQuickBuffering ? HasLowDecodedData(QUICK_BUFFERING_LOW_DATA_MS)
                              : (GetUndecodedData() < BUFFERING_WAIT * 1000)) &&
             !stream->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
             !stream->IsSuspended())
        {
          LOG(PR_LOG_DEBUG,
              ("Buffering: %.3lfs/%ds, timeout in %.3lfs %s",
               GetUndecodedData() / 1000.0,
               BUFFERING_WAIT,
               BUFFERING_WAIT - elapsed.ToSeconds(),
               (mQuickBuffering ? "(quick exit)" : "")));
          Wait(1000);
          if (mState == DECODER_STATE_SHUTDOWN)
            continue;
        } else {
          LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder));
          LOG(PR_LOG_DEBUG, ("%p Buffered for %.3lfs",
                             mDecoder,
                             (now - mBufferingStart).ToSeconds()));
          StartDecoding();
        }

        if (mState != DECODER_STATE_BUFFERING) {
          
          mDecoder->GetMonitor().NotifyAll();
          UpdateReadyState();
          if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
            if (!IsPlaying()) {
              StartPlayback();
            }
          }
        }
        break;
      }

    case DECODER_STATE_COMPLETED:
      {
        if (NS_FAILED(StartDecodeThreads())) {
          continue;
        }

        
        
        
        do {
          AdvanceFrame();
        } while (mState == DECODER_STATE_COMPLETED &&
                 (mReader->mVideoQueue.GetSize() > 0 ||
                 (HasAudio() && !mAudioCompleted)));

        if (mAudioStream) {
          
          
          
          StopPlayback(AUDIO_SHUTDOWN);
        }

        if (mState != DECODER_STATE_COMPLETED)
          continue;

        LOG(PR_LOG_DEBUG, ("Shutting down the state machine thread"));
        StopDecodeThreads();

        if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
          PRInt64 videoTime = HasVideo() ? mVideoFrameEndTime : 0;
          PRInt64 clockTime = NS_MAX(mEndTime, NS_MAX(videoTime, GetAudioClock()));
          UpdatePlaybackPosition(clockTime);
          {
            MonitorAutoExit exitMon(mDecoder->GetMonitor());
            nsCOMPtr<nsIRunnable> event =
              NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::PlaybackEnded);
            NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
          }
        }

        if (mState == DECODER_STATE_COMPLETED) {
          
          
          nsCOMPtr<nsIRunnable> event =
            new ShutdownThreadEvent(mDecoder->mStateMachineThread);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          mDecoder->mStateMachineThread = nsnull;
          return NS_OK;
        }
      }
      break;
    }
  }

  return NS_OK;
}

void nsBuiltinDecoderStateMachine::RenderVideoFrame(VideoData* aData,
                                                    TimeStamp aTarget,
                                                    nsIntSize aDisplaySize,
                                                    float aAspectRatio)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertNotCurrentThreadIn();

  if (aData->mDuplicate) {
    return;
  }

  nsRefPtr<Image> image = aData->mImage;
  if (image) {
    mDecoder->SetVideoData(gfxIntSize(aDisplaySize.width, aDisplaySize.height),
                           aAspectRatio, image, aTarget);
  }
}

PRInt64
nsBuiltinDecoderStateMachine::GetAudioClock()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  if (!mAudioStream || !HasAudio())
    return -1;
  PRInt64 t = mAudioStream->GetPosition();
  return (t == -1) ? -1 : t + mAudioStartTime;
}

void nsBuiltinDecoderStateMachine::AdvanceFrame()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  
  if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
    if (HasAudio() && mAudioStartTime == -1 && !mAudioCompleted) {
      
      
      
      
      
      Wait(AUDIO_DURATION_MS);
      return;
    }

    
    
    
    PRInt64 clock_time = -1;
    if (!IsPlaying()) {
      clock_time = DurationToMs(mPlayDuration) + mStartTime;
    } else {
      PRInt64 audio_time = GetAudioClock();
      if (HasAudio() && !mAudioCompleted && audio_time != -1) {
        clock_time = audio_time;
        
        
        mPlayDuration = MsToDuration(clock_time - mStartTime);
        mPlayStartTime = TimeStamp::Now();
      } else {
        
        clock_time = DurationToMs(TimeStamp::Now() - mPlayStartTime + mPlayDuration);
        
        NS_ASSERTION(mCurrentFrameTime <= clock_time, "Clock should go forwards");
        clock_time = NS_MAX(mCurrentFrameTime, clock_time) + mStartTime;
      }
    }

    
    
    PRInt64 remainingTime = AUDIO_DURATION_MS;
    NS_ASSERTION(clock_time >= mStartTime, "Should have positive clock time.");
    nsAutoPtr<VideoData> currentFrame;
    if (mReader->mVideoQueue.GetSize() > 0) {
      VideoData* frame = mReader->mVideoQueue.PeekFront();
      while (clock_time >= frame->mTime) {
        mVideoFrameEndTime = frame->mEndTime;
        currentFrame = frame;
        mReader->mVideoQueue.PopFront();
        mDecoder->UpdatePlaybackOffset(frame->mOffset);
        if (mReader->mVideoQueue.GetSize() == 0)
          break;
        frame = mReader->mVideoQueue.PeekFront();
      }
      
      
      if (frame && !currentFrame) {
        PRInt64 now = IsPlaying()
          ? DurationToMs(TimeStamp::Now() - mPlayStartTime + mPlayDuration)
          : DurationToMs(mPlayDuration);
        remainingTime = frame->mTime - mStartTime - now;
      }
    }

    
    
    nsMediaStream* stream = mDecoder->GetCurrentStream();
    if (mState == DECODER_STATE_DECODING &&
        mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING &&
        HasLowDecodedData(remainingTime + EXHAUSTED_DATA_MARGIN_MS) &&
        !stream->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
        !stream->IsSuspended() &&
        (JustExitedQuickBuffering() || HasLowUndecodedData()))
    {
      if (currentFrame) {
        mReader->mVideoQueue.PushFront(currentFrame.forget());
      }
      StartBuffering();
      return;
    }

    
    
    if (!IsPlaying()) {
      StartPlayback();
      mDecoder->GetMonitor().NotifyAll();
    }

    if (currentFrame) {
      
      TimeStamp presTime = mPlayStartTime - mPlayDuration +
                           MsToDuration(currentFrame->mTime - mStartTime);
      NS_ASSERTION(currentFrame->mTime >= mStartTime, "Should have positive frame time");
      {
        nsIntSize display = mInfo.mDisplay;
        float aspect = mInfo.mPixelAspectRatio;
        {
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          
          
          RenderVideoFrame(currentFrame, presTime, display, aspect);
        }
      }
      mDecoder->GetFrameStatistics().NotifyPresentedFrame();
      PRInt64 now = DurationToMs(TimeStamp::Now() - mPlayStartTime + mPlayDuration);
      remainingTime = currentFrame->mEndTime - mStartTime - now;
      currentFrame = nsnull;
    }

    
    
    mDecoder->GetMonitor().NotifyAll();

    
    
    
    if (mVideoFrameEndTime != -1 || mAudioEndTime != -1) {
      
      clock_time = NS_MIN(clock_time, NS_MAX(mVideoFrameEndTime, mAudioEndTime));
      if (clock_time > GetMediaTime()) {
        
        
        
        
        UpdatePlaybackPosition(clock_time);
      }
    }

    
    
    
    
    UpdateReadyState();

    if (remainingTime > 0) {
      Wait(remainingTime);
    }
  } else {
    if (IsPlaying()) {
      StopPlayback(AUDIO_PAUSE);
      mDecoder->GetMonitor().NotifyAll();
    }

    if (mState == DECODER_STATE_DECODING ||
        mState == DECODER_STATE_COMPLETED) {
      mDecoder->GetMonitor().Wait();
    }
  }
}

void nsBuiltinDecoderStateMachine::Wait(PRInt64 aMs) {
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  TimeStamp end = TimeStamp::Now() + MsToDuration(aMs);
  TimeStamp now;
  while ((now = TimeStamp::Now()) < end &&
         mState != DECODER_STATE_SHUTDOWN &&
         mState != DECODER_STATE_SEEKING)
  {
    PRInt64 ms = static_cast<PRInt64>(NS_round((end - now).ToSeconds() * 1000));
    if (ms == 0 || ms > PR_UINT32_MAX) {
      break;
    }
    NS_ASSERTION(ms <= aMs && ms > 0,
                 "nsBuiltinDecoderStateMachine::Wait interval very wrong!");
    mDecoder->GetMonitor().Wait(PR_MillisecondsToInterval(static_cast<PRUint32>(ms)));
  }
}

VideoData* nsBuiltinDecoderStateMachine::FindStartTime()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  PRInt64 startTime = 0;
  mStartTime = 0;
  VideoData* v = nsnull;
  PRInt64 dataOffset = mInfo.mDataOffset;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    v = mReader->FindStartTime(dataOffset, startTime);
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
  LOG(PR_LOG_DEBUG, ("%p Media start time is %lldms", mDecoder, mStartTime));
  return v;
}

void nsBuiltinDecoderStateMachine::FindEndTime() 
{
  NS_ASSERTION(OnStateMachineThread(), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  nsMediaStream* stream = mDecoder->GetCurrentStream();

  
  PRInt64 length = stream->GetLength();
  NS_ASSERTION(length > 0, "Must have a content length to get end time");

  mEndTime = 0;
  PRInt64 endTime = 0;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    endTime = mReader->FindEndTime(length);
  }
  if (endTime != -1) {
    mEndTime = endTime;
  }

  LOG(PR_LOG_DEBUG, ("%p Media end time is %lldms", mDecoder, mEndTime));   
}

void nsBuiltinDecoderStateMachine::UpdateReadyState() {
  mDecoder->GetMonitor().AssertCurrentThreadIn();

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

void nsBuiltinDecoderStateMachine::LoadMetadata()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  LOG(PR_LOG_DEBUG, ("Loading Media Headers"));
  nsresult res;
  nsVideoInfo info;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    res = mReader->ReadMetadata(&info);
  }
  mInfo = info;

  if (NS_FAILED(res) || (!info.mHasVideo && !info.mHasAudio)) {
    mState = DECODER_STATE_SHUTDOWN;      
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::DecodeError);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    return;
  }
  mDecoder->StartProgressUpdates();
  mGotDurationFromMetaData = (GetDuration() != -1);
}

PRBool nsBuiltinDecoderStateMachine::JustExitedQuickBuffering()
{
  return !mDecodeStartTime.IsNull() &&
    mQuickBuffering &&
    (TimeStamp::Now() - mDecodeStartTime) < TimeDuration::FromSeconds(QUICK_BUFFER_THRESHOLD_MS);
}

void nsBuiltinDecoderStateMachine::StartBuffering()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  TimeDuration decodeDuration = TimeStamp::Now() - mDecodeStartTime;
  
  
  
  mQuickBuffering =
    !JustExitedQuickBuffering() &&
    decodeDuration < TimeDuration::FromMilliseconds(QUICK_BUFFER_THRESHOLD_MS);
  mBufferingStart = TimeStamp::Now();

  
  
  
  
  
  
  
  
  
  UpdateReadyState();
  mState = DECODER_STATE_BUFFERING;
  LOG(PR_LOG_DEBUG, ("Changed state from DECODING to BUFFERING, decoded for %.3lfs",
                     decodeDuration.ToSeconds()));
  nsMediaDecoder::Statistics stats = mDecoder->GetStatistics();
  LOG(PR_LOG_DEBUG, ("Playback rate: %.1lfKB/s%s download rate: %.1lfKB/s%s",
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
