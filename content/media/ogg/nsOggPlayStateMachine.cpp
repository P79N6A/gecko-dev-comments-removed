






































#include <limits>
#include "nsAudioStream.h"
#include "nsTArray.h"
#include "nsBuiltinDecoder.h"
#include "nsOggReader.h"
#include "nsOggPlayStateMachine.h"
#include "oggplay/oggplay.h"
#include "mozilla/mozalloc.h"
#include "VideoUtils.h"

using namespace mozilla::layers;
using mozilla::MonitorAutoExit;



static PRBool AddOverflow(PRUint32 a, PRUint32 b, PRUint32& aResult);

#ifdef PR_LOGGING
extern PRLogModuleInfo* gBuiltinDecoderLog;
#define LOG(type, msg) PR_LOG(gBuiltinDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif




#define BUFFERING_WAIT 30





#define BUFFERING_MIN_RATE 50000
#define BUFFERING_RATE(x) ((x)< BUFFERING_MIN_RATE ? BUFFERING_MIN_RATE : (x))



#define AUDIO_FRAME_RATE 25.0

nsOggPlayStateMachine::nsOggPlayStateMachine(nsBuiltinDecoder* aDecoder) :
  mDecoder(aDecoder),
  mState(DECODER_STATE_DECODING_METADATA),
  mAudioMonitor("media.audiostream"),
  mCbCrSize(0),
  mPlayDuration(0),
  mBufferingEndOffset(0),
  mStartTime(-1),
  mEndTime(-1),
  mSeekTime(0),
  mCurrentFrameTime(0),
  mAudioStartTime(-1),
  mAudioEndTime(-1),
  mVideoFrameTime(-1),
  mVolume(1.0),
  mSeekable(PR_TRUE),
  mPositionChangeQueued(PR_FALSE),
  mAudioCompleted(PR_FALSE),
  mBufferExhausted(PR_FALSE),
  mGotDurationFromHeader(PR_FALSE),
  mStopDecodeThreads(PR_TRUE)
{
  MOZ_COUNT_CTOR(nsOggPlayStateMachine);
}

nsOggPlayStateMachine::~nsOggPlayStateMachine()
{
  MOZ_COUNT_DTOR(nsOggPlayStateMachine);
}

void nsOggPlayStateMachine::DecodeLoop()
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

  
  
  
  const unsigned videoKeyframeSkipThreshold = 1;

  
  
  const unsigned videoPumpThreshold = 5;

  
  
  
  const unsigned videoWaitThreshold = 10;

  
  
  
  const unsigned audioPumpThresholdMs = 250;

  
  
  
  const unsigned lowAudioThresholdMs = 100;

  
  
  const unsigned audioWaitThresholdMs = 2000;

  
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
        videoQueueSize < videoKeyframeSkipThreshold)
    {
      skipToNextKeyframe = PR_TRUE;
    }

    PRInt64 initialDownloadPosition = 0;
    PRInt64 currentTime = 0;
    {
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      initialDownloadPosition =
        mDecoder->GetCurrentStream()->GetCachedDataEnd(mDecoder->mDecoderPosition);
      currentTime = mCurrentFrameTime + mStartTime;
    }

    
    
    int audioQueueSize = mReader->mAudioQueue.GetSize();
    PRInt64 audioDecoded = mReader->mAudioQueue.Duration();

    
    
    if (audioDecoded > audioWaitThresholdMs ||
        (skipToNextKeyframe && audioDecoded > audioPumpThresholdMs)) {
      audioWait = PR_TRUE;
    }
    if (audioPump && audioDecoded > audioPumpThresholdMs) {
      audioPump = PR_FALSE;
    }
    if (!audioPump && audioPlaying && audioDecoded < lowAudioThresholdMs) {
      skipToNextKeyframe = PR_TRUE;
    }

    if (videoPlaying && !videoWait) {
      videoPlaying = mReader->DecodeVideoPage(skipToNextKeyframe, currentTime);
      {
        MonitorAutoEnter mon(mDecoder->GetMonitor());
        if (mDecoder->mDecoderPosition >= initialDownloadPosition) {
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
      audioPlaying = mReader->DecodeAudioPage();
      {
        MonitorAutoEnter mon(mDecoder->GetMonitor());
        if (mDecoder->mDecoderPosition >= initialDownloadPosition) {
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

PRBool nsOggPlayStateMachine::IsPlaying()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  return !mPlayStartTime.IsNull();
}

void nsOggPlayStateMachine::AudioLoop()
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
      NS_ASSERTION(IsPlaying(), "Should be playing");
      
      
      mDecoder->GetMonitor().NotifyAll();
    }

    if (audioStartTime == -1) {
      
      
      
      MonitorAutoEnter mon(mDecoder->GetMonitor());
      mAudioStartTime = audioStartTime = sound->mTime;
      LOG(PR_LOG_DEBUG, ("First audio sample has timestamp %lldms", mAudioStartTime));
    }

    {
      MonitorAutoEnter audioMon(mAudioMonitor);
      if (mAudioStream) {
        
        
        
        
        
        
        
        if (!mAudioStream->IsPaused()) {
          mAudioStream->Write(sound->mAudioData,
                              sound->AudioDataLength(),
                              PR_TRUE);
          mAudioEndTime = sound->mTime + sound->mDuration;
        } else {
          mReader->mAudioQueue.PushFront(sound);
          sound.forget();
        }
      }
    }
    sound = nsnull;

    if (mReader->mAudioQueue.AtEndOfStream()) {
      
      
      MonitorAutoEnter audioMon(mAudioMonitor);
      if (mAudioStream) {
        mAudioStream->Drain();
      }
      LOG(PR_LOG_DEBUG, ("%p Reached audio stream end.", mDecoder));
    }
  }
  {
    MonitorAutoEnter mon(mDecoder->GetMonitor());
    mAudioCompleted = PR_TRUE;
  }
  LOG(PR_LOG_DEBUG, ("Audio stream finished playing, audio thread exit"));
}

nsresult nsOggPlayStateMachine::Init()
{
  mReader = new nsOggReader(this);
  return mReader->Init();
}

void nsOggPlayStateMachine::StopPlayback(eStopMode aMode)
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

void nsOggPlayStateMachine::StartPlayback()
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
      
      mAudioStream = new nsAudioStream();
      mAudioStream->Init(mInfo.mAudioChannels,
                         mInfo.mAudioRate,
                         nsAudioStream::FORMAT_FLOAT32);
      mAudioStream->SetVolume(mVolume);
    }
  }
  mPlayStartTime = TimeStamp::Now();
  mDecoder->GetMonitor().NotifyAll();
}

void nsOggPlayStateMachine::UpdatePlaybackPosition(PRInt64 aTime)
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

void nsOggPlayStateMachine::ClearPositionChangeFlag()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  mPositionChangeQueued = PR_FALSE;
}

nsHTMLMediaElement::NextFrameStatus nsOggPlayStateMachine::GetNextFrameStatus()
{
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  if (IsBuffering() || IsSeeking()) {
    return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING;
  } else if (HaveNextFrameData()) {
    return nsHTMLMediaElement::NEXT_FRAME_AVAILABLE;
  }
  return nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE;
}

void nsOggPlayStateMachine::SetVolume(float volume)
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

float nsOggPlayStateMachine::GetCurrentTime()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  return (float)mCurrentFrameTime / 1000.0;
}

PRInt64 nsOggPlayStateMachine::GetDuration()
{
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  if (mEndTime == -1 || mStartTime == -1)
    return -1;
  return mEndTime - mStartTime;
}

void nsOggPlayStateMachine::SetDuration(PRInt64 aDuration)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  if (mStartTime != -1) {
    mEndTime = mStartTime + aDuration;
  } else {
    mStartTime = 0;
    mEndTime = aDuration;
  }
}

void nsOggPlayStateMachine::SetSeekable(PRBool aSeekable)
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  mSeekable = aSeekable;
}

void nsOggPlayStateMachine::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");

  
  MonitorAutoEnter mon(mDecoder->GetMonitor());

  
  
  LOG(PR_LOG_DEBUG, ("%p Changed state to SHUTDOWN", mDecoder));
  mState = DECODER_STATE_SHUTDOWN;
  mDecoder->GetMonitor().NotifyAll();
}

void nsOggPlayStateMachine::Decode()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be on main thread.");
  
  
  MonitorAutoEnter mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("%p Changed state from BUFFERING to DECODING", mDecoder));
    mState = DECODER_STATE_DECODING;
    mDecoder->GetMonitor().NotifyAll();
  }
}

void nsOggPlayStateMachine::ResetPlayback()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mVideoFrameTime = -1;
  mAudioStartTime = -1;
  mAudioEndTime = -1;
  mAudioCompleted = PR_FALSE;
}

void nsOggPlayStateMachine::Seek(float aTime)
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

void nsOggPlayStateMachine::StopDecodeThreads()
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
nsOggPlayStateMachine::StartDecodeThreads()
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
      NS_NewRunnableMethod(this, &nsOggPlayStateMachine::DecodeLoop);
    mDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }
  if (HasAudio() && !mAudioThread) {
    nsresult rv = NS_NewThread(getter_AddRefs(mAudioThread));
    if (NS_FAILED(rv)) {
      mState = DECODER_STATE_SHUTDOWN;
      return rv;
    }
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsOggPlayStateMachine::AudioLoop);
    mAudioThread->Dispatch(event, NS_DISPATCH_NORMAL);
  }
  return NS_OK;
}

nsresult nsOggPlayStateMachine::Run()
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
        LoadOggHeaders();
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
            !mDecoder->GetCurrentStream()->IsSuspendedByCache()) {
          
          
          
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
                               seekTime <= video->mTime + mInfo.mCallbackPeriod,
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
        if (now - mBufferingStart < TimeDuration::FromSeconds(BUFFERING_WAIT) &&
            mDecoder->GetCurrentStream()->GetCachedDataEnd(mDecoder->mDecoderPosition) < mBufferingEndOffset &&
            !mDecoder->GetCurrentStream()->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !mDecoder->GetCurrentStream()->IsSuspendedByCache()) {
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

        
        while (mState == DECODER_STATE_COMPLETED &&
               (mReader->mVideoQueue.GetSize() > 0 ||
                (HasAudio() && !mAudioCompleted)))
        {
          AdvanceFrame();
        }

        if (mAudioStream) {
          
          
          
          StopPlayback(AUDIO_SHUTDOWN);
        }

        if (mState != DECODER_STATE_COMPLETED)
          continue;

        LOG(PR_LOG_DEBUG, ("Shutting down the state machine thread"));
        StopDecodeThreads();

        if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
          PRInt64 videoTime = HasVideo() ? (mVideoFrameTime + mInfo.mCallbackPeriod) : 0;
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

void nsOggPlayStateMachine::RenderVideoFrame(VideoData* aData)
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");

  if (aData->mDuplicate) {
    return;
  }

  NS_ASSERTION(mInfo.mPicture.width != 0 && mInfo.mPicture.height != 0,
               "We can only render non-zero-sized video");
  NS_ASSERTION(aData->mBuffer[0].stride >= 0 && aData->mBuffer[0].height >= 0 &&
               aData->mBuffer[1].stride >= 0 && aData->mBuffer[1].height >= 0 &&
               aData->mBuffer[2].stride >= 0 && aData->mBuffer[2].height >= 0,
               "YCbCr stride and height must be non-negative");

  
  
  PRUint32 picXLimit;
  PRUint32 picYLimit;
  if (!AddOverflow(mInfo.mPicture.x, mInfo.mPicture.width, picXLimit) ||
      picXLimit > PRUint32(PR_ABS(aData->mBuffer[0].stride)) ||
      !AddOverflow(mInfo.mPicture.y, mInfo.mPicture.height, picYLimit) ||
      picYLimit > PRUint32(PR_ABS(aData->mBuffer[0].height)))
  {
    
    
    return;
  }

  unsigned ySize = aData->mBuffer[0].stride * aData->mBuffer[0].height;
  unsigned cbSize = aData->mBuffer[1].stride * aData->mBuffer[1].height;
  unsigned crSize = aData->mBuffer[2].stride * aData->mBuffer[2].height;
  unsigned cbCrSize = ySize + cbSize + crSize;

  if (cbCrSize != mCbCrSize) {
    mCbCrSize = cbCrSize;
    mCbCrBuffer = static_cast<unsigned char*>(moz_xmalloc(cbCrSize));
    if (!mCbCrBuffer) {
      
      NS_WARNING("Malloc failure allocating YCbCr->RGB buffer");
      return;
    }
  }

  unsigned char* data = mCbCrBuffer.get();

  unsigned char* y = data;
  unsigned char* cb = y + ySize;
  unsigned char* cr = cb + cbSize;
  
  memcpy(y, aData->mBuffer[0].data, ySize);
  memcpy(cb, aData->mBuffer[1].data, cbSize);
  memcpy(cr, aData->mBuffer[2].data, crSize);
 
  ImageContainer* container = mDecoder->GetImageContainer();
  
  
  Image::Format format = Image::PLANAR_YCBCR;
  nsRefPtr<Image> image;
  if (container) {
    image = container->CreateImage(&format, 1);
  }
  if (image) {
    NS_ASSERTION(image->GetFormat() == Image::PLANAR_YCBCR,
                 "Wrong format?");
    PlanarYCbCrImage* videoImage = static_cast<PlanarYCbCrImage*>(image.get());
    PlanarYCbCrImage::Data data;
    data.mYChannel = y;
    data.mYSize = gfxIntSize(mInfo.mFrame.width, mInfo.mFrame.height);
    data.mYStride = aData->mBuffer[0].stride;
    data.mCbChannel = cb;
    data.mCrChannel = cr;
    data.mCbCrSize = gfxIntSize(aData->mBuffer[1].width, aData->mBuffer[1].height);
    data.mCbCrStride = aData->mBuffer[1].stride;
    data.mPicX = mInfo.mPicture.x;
    data.mPicY = mInfo.mPicture.y;
    data.mPicSize = gfxIntSize(mInfo.mPicture.width, mInfo.mPicture.height);
    videoImage->SetData(data);
    mDecoder->SetVideoData(data.mPicSize, mInfo.mAspectRatio, image);
  }
}

PRInt64
nsOggPlayStateMachine::GetAudioClock()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  if (!mAudioStream || !HasAudio())
    return -1;
  PRInt64 t = mAudioStream->GetPosition();
  return (t == -1) ? -1 : t + mAudioStartTime;
}

void nsOggPlayStateMachine::AdvanceFrame()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  
  if (mDecoder->GetState() == nsBuiltinDecoder::PLAY_STATE_PLAYING) {
    if (!IsPlaying()) {
      StartPlayback();
      mDecoder->GetMonitor().NotifyAll();
    }

    if (HasAudio() && mAudioStartTime == -1 && !mAudioCompleted) {
      
      
      
      
      Wait(mInfo.mCallbackPeriod);
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
        mVideoFrameTime = data->mTime;
        videoData = data;
        mReader->mVideoQueue.PopFront();
        if (mReader->mVideoQueue.GetSize() == 0)
          break;
        data = mReader->mVideoQueue.PeekFront();
      }
    }

    if (videoData) {
      
      NS_ASSERTION(videoData->mTime >= mStartTime, "Should have positive frame time");
      {
        MonitorAutoExit exitMon(mDecoder->GetMonitor());
        
        
        RenderVideoFrame(videoData);
      }
      mDecoder->GetMonitor().NotifyAll();
      videoData = nsnull;
    }

    
    
    
    if (mVideoFrameTime != -1 || mAudioEndTime != -1) {
      
      clock_time = NS_MIN(clock_time, NS_MAX(mVideoFrameTime, mAudioEndTime));
      if (clock_time - mStartTime > mCurrentFrameTime) {
        
        
        
        
        UpdatePlaybackPosition(clock_time);
      }
    }

    
    
    
    
    UpdateReadyState();

    Wait(mInfo.mCallbackPeriod);
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

void nsOggPlayStateMachine::Wait(PRUint32 aMs) {
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
                 "nsOggPlayStateMachine::Wait interval very wrong!");
    mDecoder->GetMonitor().Wait(PR_MillisecondsToInterval(ms));
  }
}

void nsOggPlayStateMachine::LoadOggHeaders()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread),
               "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();

  LOG(PR_LOG_DEBUG, ("Loading Ogg Headers"));

  nsMediaStream* stream = mDecoder->mStream;

  nsOggInfo info;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    mReader->ReadOggHeaders(info);
  }
  mInfo = info;
  mDecoder->StartProgressUpdates();

  if (!mInfo.mHasVideo && !mInfo.mHasAudio) {
    mState = DECODER_STATE_SHUTDOWN;      
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(mDecoder, &nsBuiltinDecoder::DecodeError);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    return;
  }

  if (!mInfo.mHasVideo) {
    mInfo.mCallbackPeriod = 1000 / AUDIO_FRAME_RATE;
  }
  LOG(PR_LOG_DEBUG, ("%p Callback Period: %u", mDecoder, mInfo.mCallbackPeriod));

  

  
  
  
  
  
  mGotDurationFromHeader = (GetDuration() != -1);
  if (mState != DECODER_STATE_SHUTDOWN &&
      stream->GetLength() >= 0 &&
      mSeekable &&
      mEndTime == -1)
  {
    mDecoder->StopProgressUpdates();
    FindEndTime();
    mDecoder->StartProgressUpdates();
    mDecoder->UpdatePlaybackRate();
  }
}

VideoData* nsOggPlayStateMachine::FindStartTime()
{
  NS_ASSERTION(IsCurrentThread(mDecoder->mStateMachineThread), "Should be on state machine thread.");
  mDecoder->GetMonitor().AssertCurrentThreadIn();
  PRInt64 startTime = 0;
  mStartTime = 0;
  VideoData* v = nsnull;
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    v = mReader->FindStartTime(mInfo.mDataOffset, startTime);
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

void nsOggPlayStateMachine::FindEndTime() 
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

  NS_ASSERTION(mInfo.mDataOffset > 0,
               "Should have offset of first non-header page");
  {
    MonitorAutoExit exitMon(mDecoder->GetMonitor());
    stream->Seek(nsISeekableStream::NS_SEEK_SET, mInfo.mDataOffset);
  }
  LOG(PR_LOG_DEBUG, ("%p Media end time is %lldms", mDecoder, mEndTime));   
}

void nsOggPlayStateMachine::UpdateReadyState() {
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


static PRBool AddOverflow(PRUint32 a, PRUint32 b, PRUint32& aResult) {
  PRUint64 rl = static_cast<PRUint64>(a) + static_cast<PRUint64>(b);
  if (rl > PR_UINT32_MAX) {
    return PR_FALSE;
  }
  aResult = static_cast<PRUint32>(rl);
  return true;
}
