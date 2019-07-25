






































#include <limits>
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsBuiltinDecoderReader.h"
#include "nsBuiltinDecoderStateMachine.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"

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



const unsigned AMPLE_AUDIO_MS = 2000;









static const PRUint32 LOW_VIDEO_FRAMES = 1;


static const int AUDIO_DURATION_MS = 40;

nsBuiltinDecoderStateMachine::nsBuiltinDecoderStateMachine(nsBuiltinDecoder* aDecoder,
                                                           nsBuiltinDecoderReader* aReader) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mAudioMonitor("media.audiostream"),
  mCbCrSize(0),
  mPlayDuration(0),
  mBufferingEndOffset(0),
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
  mBufferExhausted(PR_FALSE),
  mGotDurationFromHeader(PR_FALSE),
  mStopDecodeThreads(PR_TRUE)
{
  MOZ_COUNT_CTOR(nsBuiltinDecoderStateMachine);
}

nsBuiltinDecoderStateMachine::~nsBuiltinDecoderStateMachine()
{
  MOZ_COUNT_DTOR(nsBuiltinDecoderStateMachine);
}

PRBool nsBuiltinDecoderStateMachine::HasFutureAudio() const {
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  PRBool aboveLowAudioThreshold = PR_FALSE;
  if (mAudioEndTime != -1) {
    aboveLowAudioThreshold = mAudioEndTime - mCurrentFrameTime + mStartTime > LOW_AUDIO_MS;
  }
  return HasAudio() &&
    !mAudioCompleted &&
    (mReader->mAudioQueue.GetSize() > 0 || aboveLowAudioThreshold);
}

PRBool nsBuiltinDecoderStateMachine::HaveNextFrameData() const {
    return ((!HasAudio() || mReader->mAudioQueue.AtEndOfStream()) && 
             mReader->mVideoQueue.GetSize() > 0) ||
            HasFutureAudio();
}

void nsBuiltinDecoderStateMachine::DecodeLoop()
{
  NS_ASSERTION(OnDecodeThread(), "Should be on decode thread.");
  PRBool videoPlaying = PR_FALSE;
  PRBool audioPlaying = PR_FALSE;
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    videoPlaying = HasVideo();
    audioPlaying = HasAudio();
  }

  
  
  PRBool audioPump = PR_TRUE;
  PRBool videoPump = PR_TRUE;

  
  
  
  
  PRBool skipToNextKeyframe = PR_FALSE;

  
  
  const unsigned videoPumpThreshold = 5;

  
  
  
  const unsigned videoWaitThreshold = 10;

  
  
  
  const unsigned audioPumpThresholdMs = LOW_AUDIO_MS * 2;

  
  while (videoPlaying || audioPlaying) {
    PRBool audioWait = !audioPlaying;
    PRBool videoWait = !videoPlaying;
    {
      
      
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      while (!mStopDecodeThreads &&
             mBufferExhausted &&
             mState != DECODER_STATE_SHUTDOWN)
      {
        mon.Wait();
      }
      if (mState == DECODER_STATE_SHUTDOWN || mStopDecodeThreads)
        break;
    }

    PRUint32 videoQueueSize = mReader->mVideoQueue.GetSize();
    
    
    if (videoQueueSize > videoWaitThreshold) {
      videoWait = PR_TRUE;
    }

    
    
    
    if (videoPump && videoQueueSize >= videoPumpThreshold) {
      videoPump = PR_FALSE;
    }
    if (!videoPump &&
        videoPlaying &&
        videoQueueSize < LOW_VIDEO_FRAMES)
    {
      skipToNextKeyframe = PR_TRUE;
    }

    
    
    int audioQueueSize = mReader->mAudioQueue.GetSize();
    PRInt64 initialDownloadPosition = 0;
    PRInt64 currentTime = 0;
    PRInt64 audioDecoded = 0;
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      currentTime = mCurrentFrameTime + mStartTime;
      audioDecoded = mReader->mAudioQueue.Duration();
      if (mAudioEndTime != -1) {
        audioDecoded += mAudioEndTime - currentTime;
      }
      initialDownloadPosition =
        mDecoder->GetCurrentStream()->GetCachedDataEnd(mDecoder->mDecoderPosition);
    }

    
    if (audioDecoded > AMPLE_AUDIO_MS) {
      audioWait = PR_TRUE;
    }
    if (audioPump && audioDecoded > audioPumpThresholdMs) {
      audioPump = PR_FALSE;
    }
    if (!audioPump && audioPlaying && audioDecoded < LOW_AUDIO_MS) {
      skipToNextKeyframe = PR_TRUE;
    }

    if (videoPlaying && !videoWait) {
      videoPlaying = mReader->DecodeVideoFrame(skipToNextKeyframe, currentTime);
      {
        MonitorAutoEnter mon(mDecoder->GetMonitor());
        if (mDecoder->mDecoderPosition > initialDownloadPosition) {
          mBufferExhausted = PR_TRUE;
        }
      }
    }
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      initialDownloadPosition =
        mDecoder->GetCurrentStream()->GetCachedDataEnd(mDecoder->mDecoderPosition);
      mDecoder->GetMonitor().NotifyAll();
    }

    if (audioPlaying && !audioWait) {
      audioPlaying = mReader->DecodeAudioData();
      {
        MonitorAutoEnter mon(mDecoder->GetMonitor());
        if (mDecoder->mDecoderPosition > initialDownloadPosition) {
          mBufferExhausted = PR_TRUE;
        }
      }
    }

    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());

      if (!IsPlaying() &&
          (!audioWait || !videoWait) &&
          (videoQueueSize < 2 || audioQueueSize < 2))
      {
        
        
        
        
        
        
        
        UpdateReadyState();
      }

      if (mState == DECODER_STATE_SHUTDOWN || mStopDecodeThreads) {
        break;
      }
      if ((!HasAudio() || (audioWait && audioPlaying)) &&
          (!HasVideo() || (videoWait && videoPlaying)))
      {
        
        
        mon.Wait();
      }
    }
  }

  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    if (!mStopDecodeThreads &&
        mState != DECODER_STATE_SHUTDOWN &&
        mState != DECODER_STATE_SEEKING)
    {
      mState = DECODER_STATE_COMPLETED;
      mDecoder->GetMonitor().NotifyAll();
    }
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
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    mAudioCompleted = PR_FALSE;
  }
  PRInt64 audioStartTime = -1;
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
        mon.Wait();
      }

      
      if (mState == DECODER_STATE_SHUTDOWN ||
          mStopDecodeThreads ||
          mReader->mAudioQueue.AtEndOfStream())
      {
        break;
      }
    }

    NS_ASSERTION(mReader->mAudioQueue.GetSize() > 0,
                 "Should have data to play");
    nsAutoPtr<SoundData> sound(mReader->mAudioQueue.PopFront());
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      NS_WARN_IF_FALSE(IsPlaying(), "Should be playing");
      
      
      mDecoder->GetMonitor().NotifyAll();
    }

    if (audioStartTime == -1) {
      
      
      
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      mAudioStartTime = audioStartTime = sound->mTime;
      LOG(PR_LOG_DEBUG, ("First audio sample has timestamp %lldms", mAudioStartTime));
    }

    PRInt64 audioEndTime = -1;
    {
      MonitorAutoEnter audioMon(mAudioMonitor);
      if (mAudioStream) {
        
        
        
        
        
        
        
        if (!mAudioStream->IsPaused()) {
          mAudioStream->Write(sound->mAudioData,
                              sound->AudioDataLength(),
                              PR_TRUE);
          audioEndTime = sound->mTime + sound->mDuration;
          mDecoder->UpdatePlaybackOffset(sound->mOffset);
        } else {
          mReader->mAudioQueue.PushFront(sound);
          sound.forget();
        }
      }
    }
    sound = nsnull;

    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      if (audioEndTime != -1) {
        mAudioEndTime = audioEndTime;
      }
      PRInt64 audioAhead = mAudioEndTime - mCurrentFrameTime - mStartTime;
      if (audioAhead > AMPLE_AUDIO_MS) {
        
        
        
        
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
      mAudioStream->Drain();
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

nsresult nsBuiltinDecoderStateMachine::Init()
{
  return mReader->Init();
}

void nsBuiltinDecoderStateMachine::StopPlayback(eStopMode aMode)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  
  
  
  
  
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
  if (HasAudio()) {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    MonitorAutoEnter audioMon(mAudioMonitor);
    if (mAudioStream) {
      
      
      mAudioStream->Resume();
    } else {
      
      const nsVideoInfo& info = mReader->GetInfo();
      mAudioStream = new nsAudioStream();
      mAudioStream->Init(info.mAudioChannels,
                         info.mAudioRate,
                         nsAudioStream::FORMAT_FLOAT32);
      mAudioStream->SetVolume(mVolume);
    }
  }
  mPlayStartTime = TimeStamp::Now();
  mDecoder->GetMonitor().NotifyAll();
}

void nsBuiltinDecoderStateMachine::UpdatePlaybackPosition(PRInt64 aTime)
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
  if (!mPositionChangeQueued) {
    mPositionChangeQueued = PR_TRUE;
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::PlaybackPositionChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
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

void nsBuiltinDecoderStateMachine::SetVolume(float volume)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  {
    MonitorAutoEnter audioMon(mAudioMonitor);
    if (mAudioStream) {
      mAudioStream->SetVolume(volume);
    }
  }
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    mVolume = volume;
  }
}

float nsBuiltinDecoderStateMachine::GetCurrentTime()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  return (float)mCurrentFrameTime / 1000.0;
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

void nsBuiltinDecoderStateMachine::Decode()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder));
    mState = DECODER_STATE_DECODING;
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

void nsBuiltinDecoderStateMachine::Seek(float aTime)
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
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          RenderVideoFrame(videoData);
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

        
        
        nsCOMPtr<nsIRunnable> metadataLoadedEvent =
          NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::MetadataLoaded);
        NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

        if (mState == DECODER_STATE_DECODING_METADATA) {
          LOG(PR_LOG_DEBUG, ("%p Changed state from DECODING_METADATA to DECODING", mDecoder));
          mState = DECODER_STATE_DECODING;
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

        if (mBufferExhausted &&
            mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING &&
            !mDecoder->GetCurrentStream()->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !mDecoder->GetCurrentStream()->IsSuspendedByCache() &&
            ((HasAudio() && mReader->mAudioQueue.Duration() < LOW_AUDIO_MS) ||
             (HasVideo() && (PRUint32)mReader->mVideoQueue.GetSize() < LOW_VIDEO_FRAMES)))
        {
          
          
          
          StartBuffering();
        } else {
          if (mBufferExhausted) {
            
            
            mBufferExhausted = PR_FALSE;
            mDecoder->GetMonitor().NotifyAll();
          }
        }

      }
      break;

    case DECODER_STATE_SEEKING:
      {
        
        
        
        
        
        
        
        
        PRInt64 seekTime = mSeekTime;
        mDecoder->StopProgressUpdates();

        StopPlayback(AUDIO_SHUTDOWN);
        StopDecodeThreads();
        ResetPlayback();

        
        
        
        {
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          nsCOMPtr<nsIRunnable> startEvent =
            NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStarted);
          NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        }
        if (mCurrentFrameTime != mSeekTime - mStartTime) {
          nsresult res;
          {
            MonitorAutoExit exitMon(mDecoder->GetMonitor());
            
            
            res = mReader->Seek(seekTime, mStartTime, mEndTime);
          }
          if (NS_SUCCEEDED(res)){
            SoundData* audio = HasAudio() ? mReader->mAudioQueue.PeekFront() : nsnull;
            if (audio) {
              mPlayDuration = TimeDuration::FromMilliseconds(audio->mTime);
            }
            if (HasVideo()) {
              nsAutoPtr<VideoData> video(mReader->mVideoQueue.PeekFront());
              if (video) {
                RenderVideoFrame(video);
                if (!audio) {
                  NS_ASSERTION(video->mTime <= seekTime &&
                               seekTime <= video->mEndTime,
                               "Seek target should lie inside the first frame after seek");
                  mPlayDuration = TimeDuration::FromMilliseconds(seekTime);
                }
              }
              mReader->mVideoQueue.PopFront();
            }
            UpdatePlaybackPosition(seekTime);
          }
        }
        mDecoder->StartProgressUpdates();
        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        
        LOG(PR_LOG_DEBUG, ("Seek completed, mCurrentFrameTime=%lld\n", mCurrentFrameTime));

        
        
        
        
        nsCOMPtr<nsIRunnable> stopEvent;
        if (mCurrentFrameTime == mEndTime) {
          LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lldms) to COMPLETED",
                             mDecoder, seekTime));
          stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStoppedAtEnd);
          mState = DECODER_STATE_COMPLETED;
        } else {
          LOG(PR_LOG_DEBUG, ("%p Changed state from SEEKING (to %lldms) to DECODING",
                             mDecoder, seekTime));
          stopEvent = NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::SeekingStopped);
          mState = DECODER_STATE_DECODING;
        }
        mBufferExhausted = PR_FALSE;
        mDecoder->GetMonitor().NotifyAll();

        {
          MonitorAutoExit exitMon(mDecoder->GetMonitor());
          NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);
        }
      }
      break;

    case DECODER_STATE_BUFFERING:
      {
        TimeStamp now = TimeStamp::Now();
        nsMediaStream* stream = mDecoder->GetCurrentStream();
        if (!mDecoder->CanPlayThrough() &&
            now - mBufferingStart < TimeDuration::FromSeconds(BUFFERING_WAIT) &&
            stream->GetCachedDataEnd(mDecoder->mDecoderPosition) < mBufferingEndOffset &&
            !stream->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !stream->IsSuspendedByCache()) {
          LOG(PR_LOG_DEBUG,
              ("In buffering: buffering data until %d bytes available or %f seconds",
               PRUint32(mBufferingEndOffset - mDecoder->GetCurrentStream()->GetCachedDataEnd(mDecoder->mDecoderPosition)),
               BUFFERING_WAIT - (now - mBufferingStart).ToSeconds()));
          Wait(1000);
          if (mState == DECODER_STATE_SHUTDOWN)
            continue;
        } else {
          LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder));
          LOG(PR_LOG_DEBUG, ("%p Buffered for %lf seconds",
                             mDecoder,
                             (TimeStamp::Now() - mBufferingStart).ToSeconds()));
          mState = DECODER_STATE_DECODING;
        }

        if (mState != DECODER_STATE_BUFFERING) {
          mBufferExhausted = PR_FALSE;
          
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

        while (mState == DECODER_STATE_COMPLETED) {
          mDecoder->GetMonitor().Wait();
        }
      }
      break;
    }
  }

  return NS_OK;
}

void nsBuiltinDecoderStateMachine::RenderVideoFrame(VideoData* aData)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");

  if (aData->mDuplicate) {
    return;
  }

  nsRefPtr<Image> image = aData->mImage;
  if (image) {
    const nsVideoInfo& info = mReader->GetInfo();
    mDecoder->SetVideoData(gfxIntSize(info.mPicture.width, info.mPicture.height), info.mPixelAspectRatio, image);
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
    if (!IsPlaying()) {
      StartPlayback();
      mDecoder->GetMonitor().NotifyAll();
    }

    if (HasAudio() && mAudioStartTime == -1 && !mAudioCompleted) {
      
      
      
      
      
      Wait(AUDIO_DURATION_MS);
      return;
    }

    
    
    
    PRInt64 clock_time = -1;
    PRInt64 audio_time = GetAudioClock();
    if (HasAudio() && !mAudioCompleted && audio_time != -1) {
      clock_time = audio_time;
      
      
      mPlayStartTime = TimeStamp::Now() - TimeDuration::FromMilliseconds(clock_time);
    } else {
      
      TimeDuration t = TimeStamp::Now() - mPlayStartTime + mPlayDuration;
      clock_time = (PRInt64)(1000 * t.ToSeconds());
      
      NS_ASSERTION(mCurrentFrameTime <= clock_time, "Clock should go forwards");
      clock_time = NS_MAX(mCurrentFrameTime, clock_time) + mStartTime;
    }

    NS_ASSERTION(clock_time >= mStartTime, "Should have positive clock time.");
    nsAutoPtr<VideoData> videoData;
    if (mReader->mVideoQueue.GetSize() > 0) {
      VideoData* data = mReader->mVideoQueue.PeekFront();
      while (clock_time >= data->mTime) {
        mVideoFrameEndTime = data->mEndTime;
        videoData = data;
        mReader->mVideoQueue.PopFront();
        mDecoder->UpdatePlaybackOffset(data->mOffset);
        if (mReader->mVideoQueue.GetSize() == 0)
          break;
        data = mReader->mVideoQueue.PeekFront();
      }
    }

    PRInt64 frameDuration = AUDIO_DURATION_MS;
    if (videoData) {
      
      NS_ASSERTION(videoData->mTime >= mStartTime, "Should have positive frame time");
      {
        MonitorAutoExit exitMon(mDecoder->GetMonitor());
        
        
        RenderVideoFrame(videoData);
      }
      mDecoder->GetMonitor().NotifyAll();
      frameDuration = videoData->mEndTime - videoData->mTime;
      videoData = nsnull;
    }

    
    
    
    if (mVideoFrameEndTime != -1 || mAudioEndTime != -1) {
      
      clock_time = NS_MIN(clock_time, NS_MAX(mVideoFrameEndTime, mAudioEndTime));
      if (clock_time - mStartTime > mCurrentFrameTime) {
        
        
        
        
        UpdatePlaybackPosition(clock_time);
      }
    }

    
    
    
    
    UpdateReadyState();

    NS_ASSERTION(frameDuration >= 0, "Frame duration must be positive.");
    Wait(frameDuration);
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

void nsBuiltinDecoderStateMachine::Wait(PRUint32 aMs) {
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  TimeStamp end = TimeStamp::Now() + TimeDuration::FromMilliseconds(aMs);
  TimeStamp now;
  while ((now = TimeStamp::Now()) < end &&
         mState != DECODER_STATE_SHUTDOWN &&
         mState != DECODER_STATE_SEEKING)
  {
    TimeDuration d = end - now;
    PRInt64 ms = d.ToSeconds() * 1000;
    if (ms == 0) {
      break;
    }
    NS_ASSERTION(ms <= aMs && ms > 0,
                 "nsBuiltinDecoderStateMachine::Wait interval very wrong!");
    mDecoder->GetMonitor().Wait(PR_MillisecondsToInterval(ms));
  }
}

VideoData* nsBuiltinDecoderStateMachine::FindStartTime()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  PRInt64 startTime = 0;
  mStartTime = 0;
  VideoData* v = nsnull;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    v = mReader->FindStartTime(mReader->GetInfo().mDataOffset, startTime);
  }
  if (startTime != 0) {
    mStartTime = startTime;
    if (mGotDurationFromHeader) {
      NS_ASSERTION(mEndTime != -1,
                   "We should have mEndTime as supplied duration here");
      
      
      
      mEndTime = mStartTime + mEndTime;
    }
  }
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

  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    mReader->ReadMetadata();
  }
  mDecoder->StartProgressUpdates();
  const nsVideoInfo& info = mReader->GetInfo();

  if (!info.mHasVideo && !info.mHasAudio) {
    mState = DECODER_STATE_SHUTDOWN;      
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::DecodeError);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    return;
  }
}

void nsBuiltinDecoderStateMachine::StartBuffering()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  mBufferExhausted = PR_TRUE;
  if (IsPlaying()) {
    StopPlayback(AUDIO_PAUSE);
    mDecoder->GetMonitor().NotifyAll();
  }

  
  
  
  
  
  
  
  
  
  UpdateReadyState();

  mBufferingStart = TimeStamp::Now();
  PRPackedBool reliable;
  double playbackRate = mDecoder->ComputePlaybackRate(&reliable);
  mBufferingEndOffset = mDecoder->mDecoderPosition +
    BUFFERING_RATE(playbackRate) * BUFFERING_WAIT;
  mState = DECODER_STATE_BUFFERING;
  LOG(PR_LOG_DEBUG, ("Changed state from DECODING to BUFFERING"));
}
