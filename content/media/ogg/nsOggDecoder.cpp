




































#include <limits>
#include "prlog.h"
#include "prmem.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsIDOMHTMLMediaElement.h"
#include "nsNetUtil.h"
#include "nsAudioStream.h"
#include "nsChannelReader.h"
#include "nsHTMLVideoElement.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsAutoLock.h"
#include "nsTArray.h"
#include "nsNetUtil.h"
#include "nsOggDecoder.h"

using mozilla::TimeDuration;
using mozilla::TimeStamp;





#define MAX_VIDEO_WIDTH  2000
#define MAX_VIDEO_HEIGHT 2000



#define OGGPLAY_BUFFER_SIZE 20



#define OGGPLAY_FRAMES_PER_CALLBACK 2048



#define OGGPLAY_AUDIO_OFFSET 250L




#define BUFFERING_WAIT 15





#define BUFFERING_MIN_RATE 50000
#define BUFFERING_RATE(x) ((x)< BUFFERING_MIN_RATE ? BUFFERING_MIN_RATE : (x))



#define BUFFERING_SECONDS_LOW_WATER_MARK 1





#define MIN_BOUNDED_SEEK_SIZE (64 * 1024)

class nsOggStepDecodeEvent;


































class nsOggDecodeStateMachine : public nsRunnable
{
  friend class nsOggStepDecodeEvent;
public:
  
  class FrameData {
  public:
    FrameData() :
      mVideoHeader(nsnull),
      mVideoWidth(0),
      mVideoHeight(0),
      mUVWidth(0),
      mUVHeight(0),
      mDecodedFrameTime(0.0),
      mTime(0.0)
    {
      MOZ_COUNT_CTOR(FrameData);
    }

    ~FrameData()
    {
      MOZ_COUNT_DTOR(FrameData);

      if (mVideoHeader) {
        oggplay_callback_info_unlock_item(mVideoHeader);
      }
    }

    
    void Write(nsAudioStream* aStream)
    {
      aStream->Write(mAudioData.Elements(), mAudioData.Length());
      mAudioData.Clear(); 
    }

    void SetVideoHeader(OggPlayDataHeader* aVideoHeader)
    {
      NS_ABORT_IF_FALSE(!mVideoHeader, "Frame already owns a video header");
      mVideoHeader = aVideoHeader;
      oggplay_callback_info_lock_item(mVideoHeader);
    }

    
    PRInt64 mEndStreamPosition;
    OggPlayDataHeader* mVideoHeader;
    nsTArray<float> mAudioData;
    int mVideoWidth;
    int mVideoHeight;
    int mUVWidth;
    int mUVHeight;
    float mDecodedFrameTime;
    float mTime;
    OggPlayStreamInfo mState;
  };

  
  class FrameQueue
  {
  public:
    FrameQueue() :
      mHead(0),
      mTail(0),
      mCount(0)
    {
    }

    void Push(FrameData* frame)
    {
      NS_ASSERTION(!IsFull(), "FrameQueue is full");
      mQueue[mTail] = frame;
      mTail = (mTail+1) % OGGPLAY_BUFFER_SIZE;
      ++mCount;
    }

    FrameData* Peek() const
    {
      NS_ASSERTION(mCount > 0, "FrameQueue is empty");

      return mQueue[mHead];
    }

    FrameData* Pop()
    {
      NS_ASSERTION(mCount, "FrameQueue is empty");

      FrameData* result = mQueue[mHead];
      mHead = (mHead + 1) % OGGPLAY_BUFFER_SIZE;
      --mCount;
      return result;
    }

    PRBool IsEmpty() const
    {
      return mCount == 0;
    }

    PRUint32 GetCount() const
    {
      return mCount;
    }

    PRBool IsFull() const
    {
      return mCount == OGGPLAY_BUFFER_SIZE;
    }

    PRUint32 ResetTimes(float aPeriod)
    {
      PRUint32 frames = 0;
      if (mCount > 0) {
        PRUint32 current = mHead;
        do {
          mQueue[current]->mTime = frames * aPeriod;
          frames += 1;
          current = (current + 1) % OGGPLAY_BUFFER_SIZE;
        } while (current != mTail);
      }
      return frames;
    }

  private:
    FrameData* mQueue[OGGPLAY_BUFFER_SIZE];
    PRUint32 mHead;
    PRUint32 mTail;
    
    
    PRUint32 mCount;
  };

  
  enum State {
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_DECODING,
    DECODER_STATE_SEEKING,
    DECODER_STATE_BUFFERING,
    DECODER_STATE_COMPLETED,
    DECODER_STATE_SHUTDOWN
  };

  nsOggDecodeStateMachine(nsOggDecoder* aDecoder);
  ~nsOggDecodeStateMachine();

  
  
  
  void Shutdown();
  void Decode();
  void Seek(float aTime);
  void StopStepDecodeThread(nsAutoMonitor* aMonitor);

  NS_IMETHOD Run();

  PRBool HasAudio()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "HasAudio() called during invalid state");
    
    return mAudioTrack != -1;
  }

  
  
  
  
  
  
  
  
  OggPlayErrorCode DecodeFrame();

  
  
  
  void HandleDecodeErrors(OggPlayErrorCode r);

  
  
  
  
  
  FrameData* NextFrame();

  
  
  void PlayFrame();

  
  
  void PlayVideo(FrameData* aFrame);

  
  
  
  
  
  void PlayAudio(FrameData* aFrame);

  
  
  float GetCurrentTime();

  
  
  PRInt64 GetDuration();

  
  
  
  void SetDuration(PRInt64 aDuration);

  
  
  void SetSeekable(PRBool aSeekable);

  
  
  void SetVolume(float aVolume);

  
  
  
  void ClearPositionChangeFlag();

  
  
  PRBool HaveNextFrameData() const {
    return !mDecodedFrames.IsEmpty() &&
      (mDecodedFrames.Peek()->mDecodedFrameTime > mCurrentFrameTime ||
       mDecodedFrames.GetCount() > 1);
  }

  
  
  PRBool IsBuffering() const {
    PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());
    return mState == nsOggDecodeStateMachine::DECODER_STATE_BUFFERING;
  }

  
  
  PRBool IsSeeking() const {
    PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());
    return mState == nsOggDecodeStateMachine::DECODER_STATE_SEEKING;
  }

protected:

  
  
  void DecodeToFrame(nsAutoMonitor& aMonitor,
                     float aSeekTime);

  
  
  
  
  void HandleVideoData(FrameData* aFrame, int aTrackNum, OggPlayDataHeader* aVideoHeader);
  void HandleAudioData(FrameData* aFrame, OggPlayAudioData* aAudioData, int aSize);

  
  void LoadOggHeaders(nsChannelReader* aReader);

  
  
  void OpenAudioStream();

  
  
  
  void CloseAudioStream();

  
  
  void StartAudio();

  
  
  void StopAudio();

  
  
  void StartPlayback();

  
  
  void StopPlayback();

  
  
  
  void PausePlayback();

  
  
  void ResumePlayback();

  
  
  
  
  
  void UpdatePlaybackPosition(float aTime);

  
  
  
  void QueueDecodedFrames();

  
  
  nsresult Seek(float aTime, nsChannelReader* aReader);

  
  
  void SetTracksActive();

private:
  
  
  

  
  
  nsOggDecoder* mDecoder;

  
  
  
  OggPlay* mPlayer;

  
  
  
  
  FrameQueue mDecodedFrames;

  
  
  
  
  TimeStamp mPlayStartTime;

  
  
  
  TimeStamp mPauseStartTime;

  
  
  
  
  TimeDuration mPauseDuration;

  
  
  
  
  
  PRPackedBool mPlaying;

  
  
  double mCallbackPeriod;

  
  
  PRInt32 mVideoTrack;
  float   mFramerate;
  float   mAspectRatio;

  
  
  PRInt32 mAudioRate;
  PRInt32 mAudioChannels;
  PRInt32 mAudioTrack;

  
  
  TimeStamp mBufferingStart;

  
  
  PRInt64 mBufferingEndOffset;

  
  
  PRUint64 mLastFrame;

  
  
  PRInt64 mLastFramePosition;

  
  
  nsCOMPtr<nsIThread> mStepDecodeThread;

  
  
  
  

  
  
  
  State mState;

  
  
  
  float mSeekTime;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  
  
  
  float mCurrentFrameTime;

  
  
  
  
  
  float mPlaybackStartTime;

  
  
  
  float mVolume;

  
  
  
  PRInt64 mDuration;

  
  
  PRPackedBool mSeekable;

  
  
  
  
  PRPackedBool mPositionChangeQueued;

  
  
  
  
  
  PRPackedBool mDecodingCompleted;

  
  
  
  PRPackedBool mExitStepDecodeThread;

  
  
  
  PRPackedBool mBufferExhausted;

  
  
  
  PRPackedBool mGotDurationFromHeader;
};






class nsOggStepDecodeEvent : public nsRunnable {
private:
  
  
  
  
  nsOggDecodeStateMachine* mDecodeStateMachine;

  
  
  
  OggPlay* mPlayer;

public:
  nsOggStepDecodeEvent(nsOggDecodeStateMachine* machine, OggPlay* player) : 
    mDecodeStateMachine(machine), mPlayer(player) {}
  
  
  PRBool InStopDecodingState() {
    PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecodeStateMachine->mDecoder->GetMonitor());
    return 
      mDecodeStateMachine->mState != nsOggDecodeStateMachine::DECODER_STATE_DECODING &&
      mDecodeStateMachine->mState != nsOggDecodeStateMachine::DECODER_STATE_BUFFERING;
  }
  
  
  
  
  
  
  NS_IMETHOD Run() {
    OggPlayErrorCode r = E_OGGPLAY_TIMEOUT;
    nsAutoMonitor mon(mDecodeStateMachine->mDecoder->GetMonitor());
    nsOggDecoder* decoder = mDecodeStateMachine->mDecoder;
    NS_ASSERTION(!mDecodeStateMachine->mDecodingCompleted,
                 "State machine should have cleared this flag");

    while (!mDecodeStateMachine->mExitStepDecodeThread &&
           !InStopDecodingState() &&
           (r == E_OGGPLAY_TIMEOUT ||
            r == E_OGGPLAY_USER_INTERRUPT ||
            r == E_OGGPLAY_CONTINUE)) {
      if (mDecodeStateMachine->mBufferExhausted) {
        mon.Wait();
      } else {
        
        
        
        PRInt64 initialDownloadPosition =
          decoder->mReader->Stream()->GetCachedDataEnd(decoder->mDecoderPosition);

        mon.Exit();
        r = oggplay_step_decoding(mPlayer);
        mon.Enter();

        mDecodeStateMachine->HandleDecodeErrors(r);

        
        
        
        if (decoder->mDecoderPosition > initialDownloadPosition) {
          mDecodeStateMachine->mBufferExhausted = PR_TRUE;
        }

        
        
        
        
        mon.NotifyAll();
      }
    }

    mDecodeStateMachine->mDecodingCompleted = PR_TRUE;
    return NS_OK;
  }
};

nsOggDecodeStateMachine::nsOggDecodeStateMachine(nsOggDecoder* aDecoder) :
  mDecoder(aDecoder),
  mPlayer(0),
  mPlayStartTime(),
  mPauseStartTime(),
  mPauseDuration(0),
  mPlaying(PR_FALSE),
  mCallbackPeriod(1.0),
  mVideoTrack(-1),
  mFramerate(0.0),
  mAspectRatio(1.0),
  mAudioRate(0),
  mAudioChannels(0),
  mAudioTrack(-1),
  mBufferingStart(),
  mBufferingEndOffset(0),
  mLastFrame(0),
  mLastFramePosition(-1),
  mState(DECODER_STATE_DECODING_METADATA),
  mSeekTime(0.0),
  mCurrentFrameTime(0.0),
  mPlaybackStartTime(0.0), 
  mVolume(1.0),
  mDuration(-1),
  mSeekable(PR_TRUE),
  mPositionChangeQueued(PR_FALSE),
  mDecodingCompleted(PR_FALSE),
  mExitStepDecodeThread(PR_FALSE),
  mBufferExhausted(PR_FALSE),
  mGotDurationFromHeader(PR_FALSE)
{
}

nsOggDecodeStateMachine::~nsOggDecodeStateMachine()
{
  while (!mDecodedFrames.IsEmpty()) {
    delete mDecodedFrames.Pop();
  }
  oggplay_close(mPlayer);
}

OggPlayErrorCode nsOggDecodeStateMachine::DecodeFrame()
{
  OggPlayErrorCode r = oggplay_step_decoding(mPlayer);
  return r;
}

void nsOggDecodeStateMachine::HandleDecodeErrors(OggPlayErrorCode aErrorCode)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());

  if (aErrorCode != E_OGGPLAY_TIMEOUT &&
      aErrorCode != E_OGGPLAY_OK &&
      aErrorCode != E_OGGPLAY_USER_INTERRUPT &&
      aErrorCode != E_OGGPLAY_CONTINUE) {
    mState = DECODER_STATE_SHUTDOWN;
    nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, NetworkError);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

nsOggDecodeStateMachine::FrameData* nsOggDecodeStateMachine::NextFrame()
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());
  OggPlayCallbackInfo** info = oggplay_buffer_retrieve_next(mPlayer);
  if (!info)
    return nsnull;

  FrameData* frame = new FrameData();
  if (!frame) {
    return nsnull;
  }

  frame->mTime = mCallbackPeriod * mLastFrame;
  frame->mEndStreamPosition = mDecoder->mDecoderPosition;
  mLastFrame += 1;

  if (mLastFramePosition >= 0) {
    NS_ASSERTION(frame->mEndStreamPosition >= mLastFramePosition,
                 "Playback positions must not decrease without an intervening reset");
    TimeStamp base = mPlayStartTime;
    if (base.IsNull()) {
      
      
      base = TimeStamp::Now();
    }
    mDecoder->mPlaybackStatistics.Start(
        base + TimeDuration::FromMilliseconds(NS_round(frame->mTime*1000)));
    mDecoder->mPlaybackStatistics.AddBytes(frame->mEndStreamPosition - mLastFramePosition);
    mDecoder->mPlaybackStatistics.Stop(
        base + TimeDuration::FromMilliseconds(NS_round(mCallbackPeriod*mLastFrame*1000)));
    mDecoder->UpdatePlaybackRate();
  }
  mLastFramePosition = frame->mEndStreamPosition;

  int num_tracks = oggplay_get_num_tracks(mPlayer);
  float audioTime = -1.0;
  float videoTime = -1.0;

  if (mVideoTrack != -1 &&
      num_tracks > mVideoTrack &&
      oggplay_callback_info_get_type(info[mVideoTrack]) == OGGPLAY_YUV_VIDEO) {
    OggPlayDataHeader** headers = oggplay_callback_info_get_headers(info[mVideoTrack]);
    if (headers[0]) {
      videoTime = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;
      HandleVideoData(frame, mVideoTrack, headers[0]);
    }
  }

  
  
  
  
  PRBool needSilence = PR_FALSE;

  if (mAudioTrack != -1 && num_tracks > mAudioTrack) {
    OggPlayDataType type = oggplay_callback_info_get_type(info[mAudioTrack]);
    needSilence = (type == OGGPLAY_INACTIVE);
    if (type == OGGPLAY_FLOATS_AUDIO) {
      OggPlayDataHeader** headers = oggplay_callback_info_get_headers(info[mAudioTrack]);
      if (headers[0]) {
        audioTime = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;
        int required = oggplay_callback_info_get_required(info[mAudioTrack]);
        for (int j = 0; j < required; ++j) {
          int size = oggplay_callback_info_get_record_size(headers[j]);
          OggPlayAudioData* audio_data = oggplay_callback_info_get_audio_data(headers[j]);
          HandleAudioData(frame, audio_data, size);
        }
      }
    }
  }

  if (needSilence) {
    
    size_t count = mAudioChannels * mAudioRate * mCallbackPeriod;
    
    count = mAudioChannels * PRInt32(NS_ceil(mAudioRate*mCallbackPeriod));
    float* data = frame->mAudioData.AppendElements(count);
    if (data) {
      memset(data, 0, sizeof(float)*count);
    }
  }

  
  
  if (videoTime >= 0) {
    frame->mState = oggplay_callback_info_get_stream_info(info[mVideoTrack]);
    frame->mDecodedFrameTime = videoTime;
  } else if (audioTime >= 0) {
    frame->mState = oggplay_callback_info_get_stream_info(info[mAudioTrack]);
    frame->mDecodedFrameTime = audioTime;
  } else {
    NS_WARNING("Encountered frame with no audio or video data");
    frame->mState = OGGPLAY_STREAM_UNINITIALISED;
    frame->mDecodedFrameTime = 0.0;
  }

  oggplay_buffer_release(mPlayer, info);
  return frame;
}

void nsOggDecodeStateMachine::HandleVideoData(FrameData* aFrame, int aTrackNum, OggPlayDataHeader* aVideoHeader) {
  if (!aVideoHeader)
    return;

  int y_width;
  int y_height;
  oggplay_get_video_y_size(mPlayer, aTrackNum, &y_width, &y_height);
  int uv_width;
  int uv_height;
  oggplay_get_video_uv_size(mPlayer, aTrackNum, &uv_width, &uv_height);

  if (y_width >= MAX_VIDEO_WIDTH || y_height >= MAX_VIDEO_HEIGHT) {
    return;
  }

  aFrame->mVideoWidth = y_width;
  aFrame->mVideoHeight = y_height;
  aFrame->mUVWidth = uv_width;
  aFrame->mUVHeight = uv_height;
  aFrame->SetVideoHeader(aVideoHeader);
}

void nsOggDecodeStateMachine::HandleAudioData(FrameData* aFrame, OggPlayAudioData* aAudioData, int aSize) {
  
  
  int size = aSize * mAudioChannels;

  aFrame->mAudioData.AppendElements(reinterpret_cast<float*>(aAudioData), size);
}

void nsOggDecodeStateMachine::PlayFrame() {
  
  
  
  
  
  
  
  
  
  
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());

  if (mDecoder->GetState() == nsOggDecoder::PLAY_STATE_PLAYING) {
    if (!mPlaying) {
      ResumePlayback();
    }

    if (!mDecodedFrames.IsEmpty()) {
      FrameData* frame = mDecodedFrames.Peek();
      if (frame->mState == OGGPLAY_STREAM_JUST_SEEKED) {
        
        
        
        mPlayStartTime = TimeStamp::Now();
        mPauseDuration = TimeDuration(0);
        frame->mState = OGGPLAY_STREAM_INITIALISED;
      }

      double time;
      PRUint32 hasAudio = frame->mAudioData.Length();
      for (;;) {
        
        
        
        PlayAudio(frame);
        double hwtime = mAudioStream && hasAudio ? mAudioStream->GetPosition() : -1.0;
        time = hwtime < 0.0 ?
          (TimeStamp::Now() - mPlayStartTime - mPauseDuration).ToSeconds() :
          hwtime;
        
        
        PRInt64 wait = PRInt64((frame->mTime - time)*1000);
        if (wait <= 0)
          break;
        mon.Wait(PR_MillisecondsToInterval(wait));
        if (mState == DECODER_STATE_SHUTDOWN)
          return;
      }

      mDecodedFrames.Pop();
      QueueDecodedFrames();

      
      while (!mDecodedFrames.IsEmpty() && time >= mDecodedFrames.Peek()->mTime) {
        LOG(PR_LOG_DEBUG, ("Skipping frame time %f with audio at time %f", mDecodedFrames.Peek()->mTime, time));
        PlayAudio(frame);
        delete frame;
        frame = mDecodedFrames.Peek();
        mDecodedFrames.Pop();
      }
      if (time < frame->mTime + mCallbackPeriod) {
        PlayAudio(frame);
        PlayVideo(frame);
        mDecoder->mPlaybackPosition = frame->mEndStreamPosition;
        UpdatePlaybackPosition(frame->mDecodedFrameTime);
        delete frame;
      }
      else {
        PlayAudio(frame);
        delete frame;
        frame = 0;
      }
    }
  }
  else {
    if (mPlaying) {
      PausePlayback();
    }

    if (mState == DECODER_STATE_DECODING) {
      mon.Wait();
      if (mState == DECODER_STATE_SHUTDOWN) {
        return;
      }
    }
  }
}

void nsOggDecodeStateMachine::PlayVideo(FrameData* aFrame)
{
  
  if (aFrame && aFrame->mVideoHeader) {
    OggPlayVideoData* videoData = oggplay_callback_info_get_video_data(aFrame->mVideoHeader);

    OggPlayYUVChannels yuv;
    yuv.ptry = videoData->y;
    yuv.ptru = videoData->u;
    yuv.ptrv = videoData->v;
    yuv.uv_width = aFrame->mUVWidth;
    yuv.uv_height = aFrame->mUVHeight;
    yuv.y_width = aFrame->mVideoWidth;
    yuv.y_height = aFrame->mVideoHeight;

    size_t size = aFrame->mVideoWidth * aFrame->mVideoHeight * 4;
    nsAutoArrayPtr<unsigned char> buffer(new unsigned char[size]);
    if (!buffer)
      return;

    OggPlayRGBChannels rgb;
    rgb.ptro = buffer;
    rgb.rgb_width = aFrame->mVideoWidth;
    rgb.rgb_height = aFrame->mVideoHeight;

    oggplay_yuv2bgra(&yuv, &rgb);

    mDecoder->SetRGBData(aFrame->mVideoWidth, aFrame->mVideoHeight,
                         mFramerate, mAspectRatio, buffer.forget());
  }
}

void nsOggDecodeStateMachine::PlayAudio(FrameData* aFrame)
{
  
  if (!mAudioStream)
    return;

  aFrame->Write(mAudioStream);
}

void nsOggDecodeStateMachine::OpenAudioStream()
{
  
  mAudioStream = new nsAudioStream();
  if (!mAudioStream) {
    LOG(PR_LOG_ERROR, ("Could not create audio stream"));
  }
  else {
    mAudioStream->Init(mAudioChannels, mAudioRate, nsAudioStream::FORMAT_FLOAT32);
    mAudioStream->SetVolume(mVolume);
  }
}

void nsOggDecodeStateMachine::CloseAudioStream()
{
  
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }
}

void nsOggDecodeStateMachine::StartAudio()
{
  
  if (HasAudio()) {
    OpenAudioStream();
  }
}

void nsOggDecodeStateMachine::StopAudio()
{
  
  if (HasAudio()) {
    CloseAudioStream();
  }
}

void nsOggDecodeStateMachine::StartPlayback()
{
  
  StartAudio();
  mPlaying = PR_TRUE;

  
  if (mPlayStartTime.IsNull()) {
    mPlayStartTime = TimeStamp::Now();
  }

  
  if (!mPauseStartTime.IsNull()) {
    mPauseDuration += TimeStamp::Now() - mPauseStartTime;
    
    mPauseStartTime = TimeStamp();
  }
  mPlayStartTime = TimeStamp::Now();
  mPauseDuration = 0;

}

void nsOggDecodeStateMachine::StopPlayback()
{
  
  mLastFrame = mDecodedFrames.ResetTimes(mCallbackPeriod);
  StopAudio();
  mPlaying = PR_FALSE;
  mPauseStartTime = TimeStamp::Now();
}

void nsOggDecodeStateMachine::PausePlayback()
{
  if (!mAudioStream) {
    StopPlayback();
    return;
  }
  mAudioStream->Pause();
  mPlaying = PR_FALSE;
  mPauseStartTime = TimeStamp::Now();
  if (mAudioStream->GetPosition() < 0) {
    mLastFrame = mDecodedFrames.ResetTimes(mCallbackPeriod);
  }
}

void nsOggDecodeStateMachine::ResumePlayback()
{
 if (!mAudioStream) {
    StartPlayback();
    return;
 }
 
 mAudioStream->Resume();
 mPlaying = PR_TRUE;

 
 if (!mPauseStartTime.IsNull()) {
   mPauseDuration += TimeStamp::Now() - mPauseStartTime;
   
   mPauseStartTime = TimeStamp();
 }
 mPlayStartTime = TimeStamp::Now();
 mPauseDuration = 0;
}

void nsOggDecodeStateMachine::UpdatePlaybackPosition(float aTime)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());
  mCurrentFrameTime = aTime - mPlaybackStartTime;
  if (!mPositionChangeQueued) {
    mPositionChangeQueued = PR_TRUE;
    nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, PlaybackPositionChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  }
}

void nsOggDecodeStateMachine::QueueDecodedFrames()
{
  
  FrameData* frame;
  while (!mDecodedFrames.IsFull() && (frame = NextFrame())) {
    if (mDecodedFrames.GetCount() < 2) {
      
      
      
      
      
      nsCOMPtr<nsIRunnable> event = 
        NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, UpdateReadyStateForData);
      NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
    }
    mDecodedFrames.Push(frame);
  }
}

void nsOggDecodeStateMachine::ClearPositionChangeFlag()
{
  
  mPositionChangeQueued = PR_FALSE;
}

void nsOggDecodeStateMachine::SetVolume(float volume)
{
  
  if (mAudioStream) {
    mAudioStream->SetVolume(volume);
  }

  mVolume = volume;
}

float nsOggDecodeStateMachine::GetCurrentTime()
{
  
  return mCurrentFrameTime;
}

PRInt64 nsOggDecodeStateMachine::GetDuration()
{
  
  return mDuration;
}

void nsOggDecodeStateMachine::SetDuration(PRInt64 aDuration)
{
   
  mDuration = aDuration;
}

void nsOggDecodeStateMachine::SetSeekable(PRBool aSeekable)
{
   
  mSeekable = aSeekable;
}

void nsOggDecodeStateMachine::Shutdown()
{
  
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());

  
  
  LOG(PR_LOG_DEBUG, ("Changed state to SHUTDOWN"));
  mState = DECODER_STATE_SHUTDOWN;
  mon.NotifyAll();

  if (mPlayer) {
    
    
    
    oggplay_prepare_for_close(mPlayer);
  }
}

void nsOggDecodeStateMachine::Decode()
{
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("Changed state from BUFFERING to DECODING"));
    mState = DECODER_STATE_DECODING;
    mon.NotifyAll();
  }
}

void nsOggDecodeStateMachine::Seek(float aTime)
{
  nsAutoMonitor mon(mDecoder->GetMonitor());
  
  
  NS_ASSERTION(mState != DECODER_STATE_SEEKING,
               "We shouldn't already be seeking");
  mSeekTime = aTime + mPlaybackStartTime;
  float duration = static_cast<float>(mDuration) / 1000.0;
  NS_ASSERTION(mSeekTime >= 0 && mSeekTime <= duration,
               "Can only seek in range [0,duration]");
  LOG(PR_LOG_DEBUG, ("Changed state to SEEKING (to %f)", aTime));
  mState = DECODER_STATE_SEEKING;
}

class ByteRange {
public:
  ByteRange() : mStart(-1), mEnd(-1) {}
  ByteRange(PRInt64 aStart, PRInt64 aEnd) : mStart(aStart), mEnd(aEnd) {}
  PRInt64 mStart, mEnd;
};

static void GetBufferedBytes(nsMediaStream* aStream, nsTArray<ByteRange>& aRanges)
{
  PRInt64 startOffset = 0;
  while (PR_TRUE) {
    PRInt64 endOffset = aStream->GetCachedDataEnd(startOffset);
    if (endOffset == startOffset) {
      
      endOffset = aStream->GetNextCachedData(startOffset);
      if (endOffset == -1) {
        
        
        break;
      }
    } else {
      
      PRInt64 cachedLength = endOffset - startOffset;
      
      
      
      if (cachedLength > MIN_BOUNDED_SEEK_SIZE) {
        aRanges.AppendElement(ByteRange(startOffset, endOffset));
      }
    }
    startOffset = endOffset;
  }
}

nsresult nsOggDecodeStateMachine::Seek(float aTime, nsChannelReader* aReader)
{
  LOG(PR_LOG_DEBUG, ("About to seek OggPlay to %fms", aTime));
  nsMediaStream* stream = aReader->Stream(); 
  nsAutoTArray<ByteRange, 16> ranges;
  stream->Pin();
  GetBufferedBytes(stream, ranges);
  PRInt64 rv = -1;
  for (PRUint32 i = 0; rv < 0 && i < ranges.Length(); i++) {
    rv = oggplay_seek_to_keyframe(mPlayer,
                                  ogg_int64_t(aTime * 1000),
                                  ranges[i].mStart,
                                  ranges[i].mEnd);
  }
  stream->Unpin();

  if (rv < 0) {
    
    
    rv = oggplay_seek_to_keyframe(mPlayer,
                                  ogg_int64_t(aTime * 1000),
                                  0,
                                  stream->GetLength());
  }

  LOG(PR_LOG_DEBUG, ("Finished seeking OggPlay"));

  return (rv < 0) ? NS_ERROR_FAILURE : NS_OK;
}

void nsOggDecodeStateMachine::DecodeToFrame(nsAutoMonitor& aMonitor,
                                            float aTime)
{
  
  float target = aTime - mCallbackPeriod / 2.0;
  FrameData* frame = nsnull;
  OggPlayErrorCode r;
  mLastFrame = 0;

  
  
  
  float audioTime = 0;
  nsTArray<float> audioData;
  do {
    if (frame) {
      audioData.AppendElements(frame->mAudioData);
      audioTime += frame->mAudioData.Length() /
        (float)mAudioRate / (float)mAudioChannels;
    }
    do {
      aMonitor.Exit();
      r = DecodeFrame();
      aMonitor.Enter();
    } while (mState != DECODER_STATE_SHUTDOWN && r == E_OGGPLAY_TIMEOUT);

    HandleDecodeErrors(r);

    if (mState == DECODER_STATE_SHUTDOWN)
      break;

    FrameData* nextFrame = NextFrame();
    if (!nextFrame)
      break;

    delete frame;
    frame = nextFrame;
  } while (frame->mDecodedFrameTime < target);

  if (mState == DECODER_STATE_SHUTDOWN) {
    delete frame;
    return;
  }

  NS_ASSERTION(frame != nsnull, "No frame after decode!");
  if (frame) {
    if (audioTime > frame->mTime) {
      
      
      audioTime -= frame->mTime;
      
      size_t numExtraSamples = mAudioChannels *
        PRInt32(NS_ceil(mAudioRate*audioTime));
      float* data = audioData.Elements() + audioData.Length() - numExtraSamples;
      float* dst = frame->mAudioData.InsertElementsAt(0, numExtraSamples);
      memcpy(dst, data, numExtraSamples * sizeof(float));
    }

    mLastFrame = 0;
    frame->mTime = 0;
    frame->mState = OGGPLAY_STREAM_JUST_SEEKED;
    mDecodedFrames.Push(frame);
    UpdatePlaybackPosition(frame->mDecodedFrameTime);
    PlayVideo(frame);
  }
}

void nsOggDecodeStateMachine::StopStepDecodeThread(nsAutoMonitor* aMonitor)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mDecoder->GetMonitor());

  if (!mStepDecodeThread)
    return;

  if (!mDecodingCompleted) {
    
    
    mExitStepDecodeThread = PR_TRUE;
    
    
    delete NextFrame();
    
    aMonitor->NotifyAll();
  }

  aMonitor->Exit();
  mStepDecodeThread->Shutdown();
  aMonitor->Enter();
  mStepDecodeThread = nsnull;
}

nsresult nsOggDecodeStateMachine::Run()
{
  nsChannelReader* reader = mDecoder->GetReader();
  NS_ENSURE_TRUE(reader, NS_ERROR_NULL_POINTER);
  while (PR_TRUE) {
   nsAutoMonitor mon(mDecoder->GetMonitor());
   switch(mState) {
    case DECODER_STATE_SHUTDOWN:
      if (mPlaying) {
        StopPlayback();
      }
      StopStepDecodeThread(&mon);
      NS_ASSERTION(mState == DECODER_STATE_SHUTDOWN,
                   "How did we escape from the shutdown state???");
      return NS_OK;

    case DECODER_STATE_DECODING_METADATA:
      {
        mon.Exit();
        LoadOggHeaders(reader);
        mon.Enter();
      
        OggPlayErrorCode r = E_OGGPLAY_TIMEOUT;
        while (mState != DECODER_STATE_SHUTDOWN && r == E_OGGPLAY_TIMEOUT) {
          mon.Exit();
          r = DecodeFrame();
          mon.Enter();
        }

        HandleDecodeErrors(r);

        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        mLastFrame = 0;
        FrameData* frame = NextFrame();
        if (frame) {
          mDecodedFrames.Push(frame);
          mDecoder->mPlaybackPosition = frame->mEndStreamPosition;
          mPlaybackStartTime = frame->mDecodedFrameTime;
          UpdatePlaybackPosition(frame->mDecodedFrameTime);
          
          
          if (mGotDurationFromHeader) {
            
            
            reader->SetLastFrameTime((PRInt64)(mPlaybackStartTime * 1000) + mDuration);
          }
          else if (mDuration != -1) {
            
            
            reader->SetLastFrameTime(mDuration);
            
            
            
            mDuration -= (PRInt64)(mPlaybackStartTime * 1000);
          }
          PlayVideo(frame);
        }

        
        
        nsCOMPtr<nsIRunnable> metadataLoadedEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, MetadataLoaded); 
        NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);

        if (mState == DECODER_STATE_DECODING_METADATA) {
          LOG(PR_LOG_DEBUG, ("Changed state from DECODING_METADATA to DECODING"));
          mState = DECODER_STATE_DECODING;
        }
      }
      break;

    case DECODER_STATE_DECODING:
      {
        
        
        
        if (!mStepDecodeThread) {
          nsresult rv = NS_NewThread(getter_AddRefs(mStepDecodeThread));
          if (NS_FAILED(rv)) {
            mState = DECODER_STATE_SHUTDOWN;
            continue;
          }

          mBufferExhausted = PR_FALSE;
          mDecodingCompleted = PR_FALSE;
          mExitStepDecodeThread = PR_FALSE;
          nsCOMPtr<nsIRunnable> event = new nsOggStepDecodeEvent(this, mPlayer);
          mStepDecodeThread->Dispatch(event, NS_DISPATCH_NORMAL);
        }

        
        QueueDecodedFrames();
        while (mDecodedFrames.IsEmpty() && !mDecodingCompleted &&
               !mBufferExhausted) {
          mon.Wait(PR_MillisecondsToInterval(PRInt64(mCallbackPeriod*500)));
          if (mState != DECODER_STATE_DECODING)
            break;
          QueueDecodedFrames();
        }

        if (mState != DECODER_STATE_DECODING)
          continue;

        if (mDecodingCompleted) {
          LOG(PR_LOG_DEBUG, ("Changed state from DECODING to COMPLETED"));
          mState = DECODER_STATE_COMPLETED;
          StopStepDecodeThread(&mon);
          continue;
        }

        
        
        if (!mPlaying && !mDecodedFrames.IsEmpty()) {
          PlayVideo(mDecodedFrames.Peek());
        }

        if (mBufferExhausted &&
            mDecoder->GetState() == nsOggDecoder::PLAY_STATE_PLAYING &&
            !mDecoder->mReader->Stream()->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !mDecoder->mReader->Stream()->IsSuspendedByCache()) {
          
          
          
          if (mPlaying) {
            PausePlayback();
          }

          
          
          
          
          
          
          
          
          
          nsCOMPtr<nsIRunnable> event = 
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, UpdateReadyStateForData);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

          mBufferingStart = TimeStamp::Now();
          PRPackedBool reliable;
          double playbackRate = mDecoder->ComputePlaybackRate(&reliable);
          mBufferingEndOffset = mDecoder->mDecoderPosition +
              BUFFERING_RATE(playbackRate) * BUFFERING_WAIT;
          mState = DECODER_STATE_BUFFERING;
          if (mPlaying) {
            PausePlayback();
          }
          LOG(PR_LOG_DEBUG, ("Changed state from DECODING to BUFFERING"));
        } else {
          if (mBufferExhausted) {
            
            
            
            mBufferExhausted = PR_FALSE;
            mon.NotifyAll();
          }
          PlayFrame();
        }
      }
      break;

    case DECODER_STATE_SEEKING:
      {
        
        
        
        
        
        
        
        
        StopStepDecodeThread(&mon);
        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        float seekTime = mSeekTime;
        mDecoder->StopProgressUpdates();

        StopPlayback();

        
        while (!mDecodedFrames.IsEmpty()) {
          delete mDecodedFrames.Pop();
        }
        
        
        

        mon.Exit();
        nsCOMPtr<nsIRunnable> startEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStarted);
        NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        
        nsresult res = Seek(seekTime, reader);

        
        
        
        SetTracksActive();

        mon.Enter();
        mDecoder->StartProgressUpdates();
        mLastFramePosition = mDecoder->mPlaybackPosition;
        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        if (NS_SUCCEEDED(res)) {
          DecodeToFrame(mon, seekTime);
          
          
          
          NS_ASSERTION(seekTime == mSeekTime, "No-one should have changed mSeekTime");
          if (mState == DECODER_STATE_SHUTDOWN) {
            continue;
          }

          OggPlayErrorCode r;
          
          do {
            mon.Exit();
            r = DecodeFrame();
            mon.Enter();
          } while (mState != DECODER_STATE_SHUTDOWN && r == E_OGGPLAY_TIMEOUT);
          HandleDecodeErrors(r);
          if (mState == DECODER_STATE_SHUTDOWN)
            continue;
          QueueDecodedFrames();
        }

        
        
        
        LOG(PR_LOG_DEBUG, ("Changed state from SEEKING (to %f) to DECODING", seekTime));
        mState = DECODER_STATE_DECODING;
        nsCOMPtr<nsIRunnable> stopEvent;
        if (mDecodedFrames.GetCount() > 1) {
          stopEvent = NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStopped);
          mState = DECODER_STATE_DECODING;
        } else {
          stopEvent = NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStoppedAtEnd);
          mState = DECODER_STATE_COMPLETED;
        }
        mon.NotifyAll();

        mon.Exit();
        NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);        
        mon.Enter();
      }
      break;

    case DECODER_STATE_BUFFERING:
      {
        TimeStamp now = TimeStamp::Now();
        if (now - mBufferingStart < TimeDuration::FromSeconds(BUFFERING_WAIT) &&
            mDecoder->mReader->Stream()->GetCachedDataEnd(mDecoder->mDecoderPosition) < mBufferingEndOffset &&
            !mDecoder->mReader->Stream()->IsDataCachedToEndOfStream(mDecoder->mDecoderPosition) &&
            !mDecoder->mReader->Stream()->IsSuspendedByCache()) {
          LOG(PR_LOG_DEBUG, 
              ("In buffering: buffering data until %d bytes available or %f seconds", 
               PRUint32(mBufferingEndOffset - mDecoder->mReader->Stream()->GetCachedDataEnd(mDecoder->mDecoderPosition)),
               BUFFERING_WAIT - (now - mBufferingStart).ToSeconds()));
          mon.Wait(PR_MillisecondsToInterval(1000));
          if (mState == DECODER_STATE_SHUTDOWN)
            continue;
        } else {
          LOG(PR_LOG_DEBUG, ("Changed state from BUFFERING to DECODING"));
          mState = DECODER_STATE_DECODING;
        }

        if (mState != DECODER_STATE_BUFFERING) {
          mBufferExhausted = PR_FALSE;
          
          mon.NotifyAll();
          nsCOMPtr<nsIRunnable> event = 
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, UpdateReadyStateForData);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          if (mDecoder->GetState() == nsOggDecoder::PLAY_STATE_PLAYING) {
            if (!mPlaying) {
              ResumePlayback();
            }
          }
        }

        break;
      }

    case DECODER_STATE_COMPLETED:
      {
        
        
        QueueDecodedFrames();

        
        while (mState == DECODER_STATE_COMPLETED &&
               !mDecodedFrames.IsEmpty()) {
          PlayFrame();
          if (mState == DECODER_STATE_COMPLETED) {
            
            
            
            mon.Wait(PR_MillisecondsToInterval(PRInt64(mCallbackPeriod*1000)));
            QueueDecodedFrames();
          }
        }

        if (mState != DECODER_STATE_COMPLETED)
          continue;

        if (mAudioStream) {
          mon.Exit();
          LOG(PR_LOG_DEBUG, ("Begin nsAudioStream::Drain"));
          mAudioStream->Drain();
          LOG(PR_LOG_DEBUG, ("End nsAudioStream::Drain"));
          mon.Enter();

          
          
          
          StopPlayback();

          if (mState != DECODER_STATE_COMPLETED)
            continue;
        }

        
        mCurrentFrameTime += mCallbackPeriod;
        if (mDuration >= 0) {
          mCurrentFrameTime = PR_MAX(mCurrentFrameTime, mDuration / 1000.0);
        }

        mon.Exit();
        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
        mon.Enter();

        while (mState == DECODER_STATE_COMPLETED) {
          mon.Wait();
        }
      }
      break;
    }
  }

  return NS_OK;
}

void nsOggDecodeStateMachine::LoadOggHeaders(nsChannelReader* aReader) 
{
  LOG(PR_LOG_DEBUG, ("Loading Ogg Headers"));
  mPlayer = oggplay_open_with_reader(aReader);
  if (mPlayer) {
    LOG(PR_LOG_DEBUG, ("There are %d tracks", oggplay_get_num_tracks(mPlayer)));

    for (int i = 0; i < oggplay_get_num_tracks(mPlayer); ++i) {
      LOG(PR_LOG_DEBUG, ("Tracks %d: %s", i, oggplay_get_track_typename(mPlayer, i)));
      if (mVideoTrack == -1 && oggplay_get_track_type(mPlayer, i) == OGGZ_CONTENT_THEORA) {
        oggplay_set_callback_num_frames(mPlayer, i, 1);
        mVideoTrack = i;

        int fpsd, fpsn;
        oggplay_get_video_fps(mPlayer, i, &fpsd, &fpsn);
        mFramerate = fpsd == 0 ? 0.0 : float(fpsn)/float(fpsd);
        mCallbackPeriod = 1.0 / mFramerate;
        LOG(PR_LOG_DEBUG, ("Frame rate: %f", mFramerate));

        int aspectd, aspectn;
        
        
        OggPlayErrorCode r =
          oggplay_get_video_aspect_ratio(mPlayer, i, &aspectd, &aspectn);
        mAspectRatio = r == E_OGGPLAY_OK && aspectd > 0 ?
            float(aspectn)/float(aspectd) : 1.0;

        int y_width;
        int y_height;
        oggplay_get_video_y_size(mPlayer, i, &y_width, &y_height);
        mDecoder->SetRGBData(y_width, y_height, mFramerate, mAspectRatio, nsnull);
      }
      else if (mAudioTrack == -1 && oggplay_get_track_type(mPlayer, i) == OGGZ_CONTENT_VORBIS) {
        mAudioTrack = i;
        oggplay_set_offset(mPlayer, i, OGGPLAY_AUDIO_OFFSET);
        oggplay_get_audio_samplerate(mPlayer, i, &mAudioRate);
        oggplay_get_audio_channels(mPlayer, i, &mAudioChannels);
        LOG(PR_LOG_DEBUG, ("samplerate: %d, channels: %d", mAudioRate, mAudioChannels));
      }
    }

    SetTracksActive();

    if (mVideoTrack == -1) {
      oggplay_set_callback_num_frames(mPlayer, mAudioTrack, OGGPLAY_FRAMES_PER_CALLBACK);
      mCallbackPeriod = 1.0 / (float(mAudioRate) / OGGPLAY_FRAMES_PER_CALLBACK);
    }
    LOG(PR_LOG_DEBUG, ("Callback Period: %f", mCallbackPeriod));

    oggplay_use_buffer(mPlayer, OGGPLAY_BUFFER_SIZE);

    
    
    
    
    
    {
      nsAutoMonitor mon(mDecoder->GetMonitor());
      mGotDurationFromHeader = (mDuration != -1);
      if (mState != DECODER_STATE_SHUTDOWN &&
          aReader->Stream()->GetLength() >= 0 &&
          mSeekable &&
          mDuration == -1) {
        mDecoder->StopProgressUpdates();
        
        
        
        mon.Exit();
        PRInt64 d = oggplay_get_duration(mPlayer);
        oggplay_seek(mPlayer, 0);
        mon.Enter();
        mDuration = d;
        mDecoder->StartProgressUpdates();
        mDecoder->UpdatePlaybackRate();
      }
      if (mState == DECODER_STATE_SHUTDOWN)
        return;
    }
  }
}

void nsOggDecodeStateMachine::SetTracksActive()
{
  if (mVideoTrack != -1 && 
      oggplay_set_track_active(mPlayer, mVideoTrack) < 0)  {
    LOG(PR_LOG_ERROR, ("Could not set track %d active", mVideoTrack));
  }

  if (mAudioTrack != -1 && 
      oggplay_set_track_active(mPlayer, mAudioTrack) < 0)  {
    LOG(PR_LOG_ERROR, ("Could not set track %d active", mAudioTrack));
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsOggDecoder, nsIObserver)

void nsOggDecoder::Pause() 
{
  nsAutoMonitor mon(mMonitor);
  if (mPlayState == PLAY_STATE_SEEKING || mPlayState == PLAY_STATE_ENDED) {
    mNextState = PLAY_STATE_PAUSED;
    return;
  }

  ChangeState(PLAY_STATE_PAUSED);
}

void nsOggDecoder::SetVolume(float volume)
{
  nsAutoMonitor mon(mMonitor);
  mInitialVolume = volume;

  if (mDecodeStateMachine) {
    mDecodeStateMachine->SetVolume(volume);
  }
}

float nsOggDecoder::GetDuration()
{
  if (mDuration >= 0) {
     return static_cast<float>(mDuration) / 1000.0;
  }

  return std::numeric_limits<float>::quiet_NaN();
}

nsOggDecoder::nsOggDecoder() :
  nsMediaDecoder(),
  mDecoderPosition(0),
  mPlaybackPosition(0),
  mCurrentTime(0.0),
  mInitialVolume(0.0),
  mRequestedSeekTime(-1.0),
  mDuration(-1),
  mNotifyOnShutdown(PR_FALSE),
  mSeekable(PR_TRUE),
  mReader(nsnull),
  mMonitor(nsnull),
  mPlayState(PLAY_STATE_PAUSED),
  mNextState(PLAY_STATE_PAUSED),
  mResourceLoaded(PR_FALSE),
  mIgnoreProgressData(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsOggDecoder);
}

PRBool nsOggDecoder::Init(nsHTMLMediaElement* aElement)
{
  mMonitor = nsAutoMonitor::NewMonitor("media.decoder");
  return mMonitor && nsMediaDecoder::Init(aElement);
}

void nsOggDecoder::Stop()
{
  NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");

  
  
  
  if (mDecodeThread)
    mDecodeThread->Shutdown();

  mDecodeThread = nsnull;
  mDecodeStateMachine = nsnull;
  mReader = nsnull;
}

void nsOggDecoder::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsOggDecoder::Shutdown called on non-main thread");  
  
  if (mShuttingDown)
    return;

  mShuttingDown = PR_TRUE;

  
  
  
  if (mDecodeStateMachine) {
    mDecodeStateMachine->Shutdown();
  }

  
  
  mReader->Stream()->Close();

  ChangeState(PLAY_STATE_SHUTDOWN);
  nsMediaDecoder::Shutdown();

  
  
  
  
  
  
  
  nsCOMPtr<nsIRunnable> event =
    NS_NEW_RUNNABLE_METHOD(nsOggDecoder, this, Stop);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

  UnregisterShutdownObserver();
}

nsOggDecoder::~nsOggDecoder()
{
  MOZ_COUNT_DTOR(nsOggDecoder);
  nsAutoMonitor::DestroyMonitor(mMonitor);
}

nsresult nsOggDecoder::Load(nsIURI* aURI, nsIChannel* aChannel,
                            nsIStreamListener** aStreamListener)
{
  
  mDecoderPosition = 0;
  mPlaybackPosition = 0;
  mResourceLoaded = PR_FALSE;

  NS_ASSERTION(!mReader, "Didn't shutdown properly!");
  NS_ASSERTION(!mDecodeStateMachine, "Didn't shutdown properly!");
  NS_ASSERTION(!mDecodeThread, "Didn't shutdown properly!");

  if (aStreamListener) {
    *aStreamListener = nsnull;
  }

  if (aURI) {
    NS_ASSERTION(!aStreamListener, "No listener should be requested here");
    mURI = aURI;
  } else {
    NS_ASSERTION(aChannel, "Either a URI or a channel is required");
    NS_ASSERTION(aStreamListener, "A listener should be requested here");

    
    
    
    nsresult rv = NS_GetFinalChannelURI(aChannel, getter_AddRefs(mURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  RegisterShutdownObserver();

  mReader = new nsChannelReader();
  NS_ENSURE_TRUE(mReader, NS_ERROR_OUT_OF_MEMORY);

  {
    nsAutoMonitor mon(mMonitor);
    
    
    
    nsresult rv = mReader->Init(this, mURI, aChannel, aStreamListener);
    if (NS_FAILED(rv)) {
      
      mReader = nsnull;
      return rv;
    }
  }

  nsresult rv = NS_NewThread(getter_AddRefs(mDecodeThread));
  NS_ENSURE_SUCCESS(rv, rv);

  mDecodeStateMachine = new nsOggDecodeStateMachine(this);
  {
    nsAutoMonitor mon(mMonitor);
    mDecodeStateMachine->SetSeekable(mSeekable);
  }

  ChangeState(PLAY_STATE_LOADING);

  return mDecodeThread->Dispatch(mDecodeStateMachine, NS_DISPATCH_NORMAL);
}

nsresult nsOggDecoder::Play()
{
  nsAutoMonitor mon(mMonitor);
  if (mPlayState == PLAY_STATE_SEEKING) {
    mNextState = PLAY_STATE_PLAYING;
    return NS_OK;
  }
  if (mPlayState == PLAY_STATE_ENDED)
    return Seek(0);

  ChangeState(PLAY_STATE_PLAYING);

  return NS_OK;
}

nsresult nsOggDecoder::Seek(float aTime)
{
  nsAutoMonitor mon(mMonitor);

  if (aTime < 0.0)
    return NS_ERROR_FAILURE;

  mRequestedSeekTime = aTime;

  
  
  
  if (mPlayState != PLAY_STATE_SEEKING) {
    if (mPlayState == PLAY_STATE_ENDED) {
      mNextState = PLAY_STATE_PLAYING;
    } else {
      mNextState = mPlayState;
    }
    ChangeState(PLAY_STATE_SEEKING);
  }

  return NS_OK;
}

nsresult nsOggDecoder::PlaybackRateChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

float nsOggDecoder::GetCurrentTime()
{
  return mCurrentTime;
}

void nsOggDecoder::GetCurrentURI(nsIURI** aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
}

already_AddRefed<nsIPrincipal> nsOggDecoder::GetCurrentPrincipal()
{
  if (!mReader)
    return nsnull;
  return mReader->Stream()->GetCurrentPrincipal();
}

void nsOggDecoder::MetadataLoaded()
{
  if (mShuttingDown)
    return;

  
  
  PRBool notifyElement = PR_TRUE;
  {
    nsAutoMonitor mon(mMonitor);
    mDuration = mDecodeStateMachine ? mDecodeStateMachine->GetDuration() : -1;
    notifyElement = mNextState != PLAY_STATE_SEEKING;
  }

  if (mElement && notifyElement) {
    
    
    Invalidate();
    mElement->MetadataLoaded();
  }

  if (!mResourceLoaded) {
    StartProgress();
  }
  else if (mElement)
  {
    
    
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
  }
 
  
  
  PRBool resourceIsLoaded = !mResourceLoaded && mReader &&
    mReader->Stream()->IsDataCachedToEndOfStream(mDecoderPosition);
  if (mElement && notifyElement) {
    mElement->FirstFrameLoaded(resourceIsLoaded);
  }

  
  
  
  
  nsAutoMonitor mon(mMonitor);
  if (mPlayState == PLAY_STATE_LOADING) {
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
    }
    else {
      ChangeState(mNextState);
    }
  }

  if (resourceIsLoaded) {
    ResourceLoaded();
  }
}

void nsOggDecoder::ResourceLoaded()
{
  
  
  
  
  if (mShuttingDown)
    return;

  {
    
    
    nsAutoMonitor mon(mMonitor);
    if (mIgnoreProgressData || mResourceLoaded || mPlayState == PLAY_STATE_LOADING)
      return;

    Progress(PR_FALSE);

    mResourceLoaded = PR_TRUE;
    StopProgress();
  }

  
  if (mElement) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mElement->ResourceLoaded();
  }
}

void nsOggDecoder::NetworkError()
{
  if (mShuttingDown)
    return;

  if (mElement)
    mElement->NetworkError();

  Shutdown();
}

PRBool nsOggDecoder::IsSeeking() const
{
  return mPlayState == PLAY_STATE_SEEKING || mNextState == PLAY_STATE_SEEKING;
}

PRBool nsOggDecoder::IsEnded() const
{
  return mPlayState == PLAY_STATE_ENDED || mPlayState == PLAY_STATE_SHUTDOWN;
}

void nsOggDecoder::PlaybackEnded()
{
  if (mShuttingDown || mPlayState == nsOggDecoder::PLAY_STATE_SEEKING)
    return;

  PlaybackPositionChanged();
  ChangeState(PLAY_STATE_ENDED);

  if (mElement)  {
    UpdateReadyStateForData();
    mElement->PlaybackEnded();
  }
}

NS_IMETHODIMP nsOggDecoder::Observe(nsISupports *aSubjet,
                                      const char *aTopic,
                                      const PRUnichar *someData)
{
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    Shutdown();
  }

  return NS_OK;
}

nsMediaDecoder::Statistics
nsOggDecoder::GetStatistics()
{
  Statistics result;

  nsAutoMonitor mon(mMonitor);
  if (mReader) {
    result.mDownloadRate = 
      mReader->Stream()->GetDownloadRate(&result.mDownloadRateReliable);
    result.mDownloadPosition =
      mReader->Stream()->GetCachedDataEnd(mDecoderPosition);
    result.mTotalBytes = mReader->Stream()->GetLength();
    result.mPlaybackRate = ComputePlaybackRate(&result.mPlaybackRateReliable);
    result.mDecoderPosition = mDecoderPosition;
    result.mPlaybackPosition = mPlaybackPosition;
  } else {
    result.mDownloadRate = 0;
    result.mDownloadRateReliable = PR_TRUE;
    result.mPlaybackRate = 0;
    result.mPlaybackRateReliable = PR_TRUE;
    result.mDecoderPosition = 0;
    result.mPlaybackPosition = 0;
    result.mDownloadPosition = 0;
    result.mTotalBytes = 0;
  }

  return result;
}

double nsOggDecoder::ComputePlaybackRate(PRPackedBool* aReliable)
{
  PRInt64 length = mReader ? mReader->Stream()->GetLength() : -1;
  if (mDuration >= 0 && length >= 0) {
    *aReliable = PR_TRUE;
    return double(length)*1000.0/mDuration;
  }
  return mPlaybackStatistics.GetRateAtLastStop(aReliable);
}

void nsOggDecoder::UpdatePlaybackRate()
{
  if (!mReader)
    return;
  PRPackedBool reliable;
  PRUint32 rate = PRUint32(ComputePlaybackRate(&reliable));
  if (reliable) {
    
    rate = PR_MAX(rate, 1);
  } else {
    
    
    rate = PR_MAX(rate, 10000);
  }
  mReader->Stream()->SetPlaybackRate(rate);
}

void nsOggDecoder::NotifySuspendedStatusChanged()
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsOggDecoder::NotifyDownloadSuspended called on non-main thread");
  if (!mReader)
    return;
  if (mReader->Stream()->IsSuspendedByCache() && mElement) {
    
    
    mElement->NotifyAutoplayDataReady();
  }
}

void nsOggDecoder::NotifyBytesDownloaded()
{
  NS_ASSERTION(NS_IsMainThread(),
               "nsOggDecoder::NotifyBytesDownloaded called on non-main thread");   
  UpdateReadyStateForData();
}

void nsOggDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  if (aStatus == NS_BINDING_ABORTED)
    return;

  {
    nsAutoMonitor mon(mMonitor);
    UpdatePlaybackRate();
  }

  if (NS_SUCCEEDED(aStatus)) {
    ResourceLoaded();
  } else if (aStatus != NS_BASE_STREAM_CLOSED) {
    NetworkError();
  }
  UpdateReadyStateForData();
}

void nsOggDecoder::NotifyBytesConsumed(PRInt64 aBytes)
{
  nsAutoMonitor mon(mMonitor);
  if (!mIgnoreProgressData) {
    mDecoderPosition += aBytes;
  }
}

void nsOggDecoder::UpdateReadyStateForData()
{
  if (!mElement || mShuttingDown || !mDecodeStateMachine)
    return;

  nsHTMLMediaElement::NextFrameStatus frameStatus;
  {
    nsAutoMonitor mon(mMonitor);
    if (mDecodeStateMachine->IsBuffering() ||
        mDecodeStateMachine->IsSeeking()) {
      frameStatus = nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING;
    } else if (mDecodeStateMachine->HaveNextFrameData()) {
      frameStatus = nsHTMLMediaElement::NEXT_FRAME_AVAILABLE;
    } else {
      frameStatus = nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE;
    }
  }
  mElement->UpdateReadyStateForData(frameStatus);
}

void nsOggDecoder::SeekingStopped()
{
  if (mShuttingDown)
    return;

  {
    nsAutoMonitor mon(mMonitor);

    
    
    if (mRequestedSeekTime >= 0.0)
      ChangeState(PLAY_STATE_SEEKING);
    else
      ChangeState(mNextState);
  }

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekCompleted();
  }
}



void nsOggDecoder::SeekingStoppedAtEnd()
{
  if (mShuttingDown)
    return;

  PRBool fireEnded = PR_FALSE;
  {
    nsAutoMonitor mon(mMonitor);

    
    
    if (mRequestedSeekTime >= 0.0) {
      ChangeState(PLAY_STATE_SEEKING);
    } else {
      fireEnded = mNextState != PLAY_STATE_PLAYING;
      ChangeState(fireEnded ? PLAY_STATE_ENDED : mNextState);
    }
  }

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekCompleted();
    if (fireEnded) {
      mElement->PlaybackEnded();
    }
  }
}

void nsOggDecoder::SeekingStarted()
{
  if (mShuttingDown)
    return;

  if (mElement) {
    UpdateReadyStateForData();
    mElement->SeekStarted();
  }
}

void nsOggDecoder::RegisterShutdownObserver()
{
  if (!mNotifyOnShutdown) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      mNotifyOnShutdown = 
        NS_SUCCEEDED(observerService->AddObserver(this, 
                                                  NS_XPCOM_SHUTDOWN_OBSERVER_ID, 
                                                  PR_FALSE));
    }
    else {
      NS_WARNING("Could not get an observer service. Video decoding events may not shutdown cleanly.");
    }
  }
}

void nsOggDecoder::UnregisterShutdownObserver()
{
  if (mNotifyOnShutdown) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1");
    if (observerService) {
      mNotifyOnShutdown = PR_FALSE;
      observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    }
  }
}

void nsOggDecoder::ChangeState(PlayState aState)
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsOggDecoder::ChangeState called on non-main thread");   
  nsAutoMonitor mon(mMonitor);

  if (mNextState == aState) {
    mNextState = PLAY_STATE_PAUSED;
  }

  if (mPlayState == PLAY_STATE_SHUTDOWN) {
    mon.NotifyAll();
    return;
  }

  mPlayState = aState;
  switch (aState) {
  case PLAY_STATE_PAUSED:
    
    break;
  case PLAY_STATE_PLAYING:
    mDecodeStateMachine->Decode();
    break;
  case PLAY_STATE_SEEKING:
    mDecodeStateMachine->Seek(mRequestedSeekTime);
    mRequestedSeekTime = -1.0;
    break;
  case PLAY_STATE_LOADING:
    
    break;
  case PLAY_STATE_START:
    
    break;
  case PLAY_STATE_ENDED:
    
    break;
  case PLAY_STATE_SHUTDOWN:
    
    break;
  }
  mon.NotifyAll();
}

void nsOggDecoder::PlaybackPositionChanged()
{
  if (mShuttingDown)
    return;

  float lastTime = mCurrentTime;

  
  
  {
    nsAutoMonitor mon(mMonitor);

    if (mDecodeStateMachine) {
      mCurrentTime = mDecodeStateMachine->GetCurrentTime();
      mDecodeStateMachine->ClearPositionChangeFlag();
    }
  }

  
  
  
  
  Invalidate();

  if (mElement && lastTime != mCurrentTime) {
    mElement->DispatchSimpleEvent(NS_LITERAL_STRING("timeupdate"));
  }
}

void nsOggDecoder::SetDuration(PRInt64 aDuration)
{
  mDuration = aDuration;
  if (mDecodeStateMachine) {
    nsAutoMonitor mon(mMonitor);
    mDecodeStateMachine->SetDuration(mDuration);
    UpdatePlaybackRate();
  }
}

void nsOggDecoder::SetSeekable(PRBool aSeekable)
{
  mSeekable = aSeekable;
  if (mDecodeStateMachine) {
    nsAutoMonitor mon(mMonitor);
    mDecodeStateMachine->SetSeekable(aSeekable);
  }
}

PRBool nsOggDecoder::GetSeekable()
{
  return mSeekable;
}

void nsOggDecoder::Suspend()
{
  if (mReader) {
    mReader->Stream()->Suspend(PR_TRUE);
  }
}

void nsOggDecoder::Resume()
{
  if (mReader) {
    mReader->Stream()->Resume();
  }
}

void nsOggDecoder::StopProgressUpdates()
{
  mIgnoreProgressData = PR_TRUE;
  if (mReader) {
    mReader->Stream()->SetReadMode(nsMediaCacheStream::MODE_METADATA);
  }
}

void nsOggDecoder::StartProgressUpdates()
{
  mIgnoreProgressData = PR_FALSE;
  if (mReader) {
    mReader->Stream()->SetReadMode(nsMediaCacheStream::MODE_PLAYBACK);
    mDecoderPosition = mPlaybackPosition = mReader->Stream()->Tell();
  }
}

void nsOggDecoder::MoveLoadsToBackground()
{
  if (mReader && mReader->Stream()) {
    mReader->Stream()->MoveLoadsToBackground();
  }
}

