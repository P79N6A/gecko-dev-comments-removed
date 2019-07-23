




































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





#define MAX_VIDEO_WIDTH  2000
#define MAX_VIDEO_HEIGHT 2000



#define OGGPLAY_BUFFER_SIZE 20



#define OGGPLAY_FRAMES_PER_CALLBACK 2048



#define OGGPLAY_AUDIO_OFFSET 250L




#define BUFFERING_WAIT 15





#define BUFFERING_MIN_RATE 50000
#define BUFFERING_RATE(x) ((x)< BUFFERING_MIN_RATE ? BUFFERING_MIN_RATE : (x))



#define BUFFERING_SECONDS_LOW_WATER_MARK 1


































class nsOggDecodeStateMachine : public nsRunnable
{
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
      PRUint32 length = mAudioData.Length();
      if (length == 0)
        return;

      aStream->Write(mAudioData.Elements(), length);
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
      mEmpty(PR_TRUE)
    {
    }

    void Push(FrameData* frame)
    {
      NS_ASSERTION(!IsFull(), "FrameQueue is full");
      mQueue[mTail] = frame;
      mTail = (mTail+1) % OGGPLAY_BUFFER_SIZE;
      mEmpty = PR_FALSE;
    }

    FrameData* Peek()
    {
      NS_ASSERTION(!mEmpty, "FrameQueue is empty");

      return mQueue[mHead];
    }

    FrameData* Pop()
    {
      NS_ASSERTION(!mEmpty, "FrameQueue is empty");

      FrameData* result = mQueue[mHead];
      mHead = (mHead + 1) % OGGPLAY_BUFFER_SIZE;
      mEmpty = mHead == mTail;
      return result;
    }

    PRBool IsEmpty() const
    {
      return mEmpty;
    }

    PRBool IsFull() const
    {
      return !mEmpty && mHead == mTail;
    }

  private:
    FrameData* mQueue[OGGPLAY_BUFFER_SIZE];
    PRInt32 mHead;
    PRInt32 mTail;
    PRPackedBool mEmpty;
  };

  
  enum State {
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_DECODING_FIRSTFRAME,
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

  NS_IMETHOD Run();

  PRBool HasAudio()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "HasAudio() called during invalid state");
    
    return mAudioTrack != -1;
  }

  
  
  
  
  
  
  
  
  OggPlayErrorCode DecodeFrame();

  
  
  
  
  
  FrameData* NextFrame();

  
  
  void PlayFrame();

  
  
  void PlayVideo(FrameData* aFrame);

  
  
  void PlayAudio(FrameData* aFrame);

  
  
  float GetCurrentTime();

  
  
  PRInt64 GetDuration();

  
  
  void SetContentLength(PRInt64 aLength);

  
  
  
  void SetDuration(PRInt64 aDuration);

  
  
  void SetSeekable(PRBool aSeekable);

  
  
  void SetVolume(float aVolume);

  
  
  
  void ClearPositionChangeFlag();

  
  
  PRBool HaveNextFrameData() const {
    return !mDecodedFrames.IsEmpty() &&
      (mState == DECODER_STATE_DECODING ||
       mState == DECODER_STATE_COMPLETED);
  }

  
  
  PRBool IsBuffering() const {
    return mState == nsOggDecodeStateMachine::DECODER_STATE_BUFFERING;
  }

protected:
  
  
  
  
  void HandleVideoData(FrameData* aFrame, int aTrackNum, OggPlayDataHeader* aVideoHeader);
  void HandleAudioData(FrameData* aFrame, OggPlayAudioData* aAudioData, int aSize);

  
  void LoadOggHeaders(nsChannelReader* aReader);

  
  
  void OpenAudioStream();

  
  
  
  void CloseAudioStream();

  
  
  void StartAudio();

  
  
  void StopAudio();

  
  void StartPlayback();

  
  void StopPlayback();

  
  
  
  
  
  void UpdatePlaybackPosition(float aTime);

private:
  
  
  

  
  
  nsOggDecoder* mDecoder;

  
  
  
  OggPlay* mPlayer;

  
  
  
  
  FrameQueue mDecodedFrames;

  
  
  
  
  PRIntervalTime mPlayStartTime;

  
  
  
  PRIntervalTime mPauseStartTime;

  
  
  
  
  PRIntervalTime mPauseDuration;

  
  
  
  
  
  PRPackedBool mPlaying;

  
  
  float mCallbackPeriod;

  
  
  PRInt32 mVideoTrack;
  float   mFramerate;

  
  
  PRInt32 mAudioRate;
  PRInt32 mAudioChannels;
  PRInt32 mAudioTrack;

  
  
  PRIntervalTime mBufferingStart;

  
  
  PRInt64 mBufferingEndOffset;

  
  
  
  float mLastFrameTime;

  
  
  PRInt64 mLastFramePosition;

  
  
  
  

  
  
  
  State mState;

  
  
  
  float mSeekTime;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  
  
  
  float mCurrentFrameTime;

  
  
  
  float mVolume;

  
  
  
  PRInt64 mDuration;

  
  
  
  PRInt64 mContentLength;

  
  
  PRPackedBool mSeekable;

  
  
  
  
  PRPackedBool mPositionChangeQueued;
};

nsOggDecodeStateMachine::nsOggDecodeStateMachine(nsOggDecoder* aDecoder) :
  mDecoder(aDecoder),
  mPlayer(0),
  mPlayStartTime(0),
  mPauseStartTime(0),
  mPauseDuration(0),
  mPlaying(PR_FALSE),
  mCallbackPeriod(1.0),
  mVideoTrack(-1),
  mFramerate(0.0),
  mAudioRate(0),
  mAudioChannels(0),
  mAudioTrack(-1),
  mBufferingStart(0),
  mBufferingEndOffset(0),
  mLastFrameTime(0),
  mLastFramePosition(-1),
  mState(DECODER_STATE_DECODING_METADATA),
  mSeekTime(0.0),
  mCurrentFrameTime(0.0),
  mVolume(1.0),
  mDuration(-1),
  mContentLength(-1),
  mSeekable(PR_TRUE),
  mPositionChangeQueued(PR_FALSE)
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
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "DecodeFrame() called during invalid state");
  return oggplay_step_decoding(mPlayer);
}

nsOggDecodeStateMachine::FrameData* nsOggDecodeStateMachine::NextFrame()
{
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "NextFrame() called during invalid state");
  OggPlayCallbackInfo** info = oggplay_buffer_retrieve_next(mPlayer);
  if (!info)
    return nsnull;

  FrameData* frame = new FrameData();
  if (!frame) {
    return nsnull;
  }

  frame->mTime = mLastFrameTime;
  frame->mEndStreamPosition = mDecoder->mDecoderPosition;
  mLastFrameTime += mCallbackPeriod;

  if (mLastFramePosition >= 0) {
    NS_ASSERTION(frame->mEndStreamPosition >= mLastFramePosition,
                 "Playback positions must not decrease without an intervening reset");
    mDecoder->mPlaybackStatistics.Start(frame->mTime*PR_TicksPerSecond());
    mDecoder->mPlaybackStatistics.AddBytes(frame->mEndStreamPosition - mLastFramePosition);
    mDecoder->mPlaybackStatistics.Stop(mLastFrameTime*PR_TicksPerSecond());
  }
  mLastFramePosition = frame->mEndStreamPosition;

  int num_tracks = oggplay_get_num_tracks(mPlayer);
  float audioTime = 0.0;
  float videoTime = 0.0;

  if (mVideoTrack != -1 &&
      num_tracks > mVideoTrack &&
      oggplay_callback_info_get_type(info[mVideoTrack]) == OGGPLAY_YUV_VIDEO) {
    OggPlayDataHeader** headers = oggplay_callback_info_get_headers(info[mVideoTrack]);
    videoTime = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;
    HandleVideoData(frame, mVideoTrack, headers[0]);
  }

  if (mAudioTrack != -1 &&
      num_tracks > mAudioTrack &&
      oggplay_callback_info_get_type(info[mAudioTrack]) == OGGPLAY_FLOATS_AUDIO) {
    OggPlayDataHeader** headers = oggplay_callback_info_get_headers(info[mAudioTrack]);
    audioTime = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;
    int required = oggplay_callback_info_get_required(info[mAudioTrack]);
    for (int j = 0; j < required; ++j) {
      int size = oggplay_callback_info_get_record_size(headers[j]);
      OggPlayAudioData* audio_data = oggplay_callback_info_get_audio_data(headers[j]);
      HandleAudioData(frame, audio_data, size);
    }
  }

  
  
  if (mVideoTrack >= 0 )
    frame->mState = oggplay_callback_info_get_stream_info(info[mVideoTrack]);
  else if (mAudioTrack >= 0)
    frame->mState = oggplay_callback_info_get_stream_info(info[mAudioTrack]);
  else
    frame->mState = OGGPLAY_STREAM_UNINITIALISED;

  frame->mDecodedFrameTime = mVideoTrack == -1 ? audioTime : videoTime;

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
      StartPlayback();
    }

    if (!mDecodedFrames.IsEmpty()) {
      FrameData* frame = mDecodedFrames.Peek();
      if (frame->mState == OGGPLAY_STREAM_JUST_SEEKED) {
        
        
        
        mPlayStartTime = PR_IntervalNow();
        mPauseDuration = 0;
        frame->mState = OGGPLAY_STREAM_INITIALISED;
      }

      double time = (PR_IntervalToMilliseconds(PR_IntervalNow()-mPlayStartTime-mPauseDuration)/1000.0);
      if (time >= frame->mTime) {
        
        
        
        
        PlayAudio(frame);
        mDecodedFrames.Pop();
        PlayVideo(mDecodedFrames.IsEmpty() ? frame : mDecodedFrames.Peek());
        mDecoder->mPlaybackPosition = frame->mEndStreamPosition;
        UpdatePlaybackPosition(frame->mDecodedFrameTime);
        delete frame;
      }
      else {
        
        
        if (mDecodedFrames.IsFull()) {
          mon.Wait(PR_MillisecondsToInterval(PRInt64((frame->mTime - time)*1000)));
          if (mState == DECODER_STATE_SHUTDOWN) {
            return;
          }
        }
      }
    }
  }
  else {
    if (mPlaying) {
      StopPlayback();
    }

    if (mDecodedFrames.IsFull() && mState == DECODER_STATE_DECODING) {
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

#ifdef IS_BIG_ENDIAN
    oggplay_yuv2argb(&yuv, &rgb);
#else
    oggplay_yuv2bgr(&yuv, &rgb);
#endif

    mDecoder->SetRGBData(aFrame->mVideoWidth, aFrame->mVideoHeight, mFramerate, buffer.forget());
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

  
  if (mPlayStartTime == 0) {
    mPlayStartTime = PR_IntervalNow();
  }

  
  if (mPauseStartTime != 0) {
    mPauseDuration += PR_IntervalNow() - mPauseStartTime;
  }
}

void nsOggDecodeStateMachine::StopPlayback()
{
  
  StopAudio();
  mPlaying = PR_FALSE;
  mPauseStartTime = PR_IntervalNow();
}

void nsOggDecodeStateMachine::UpdatePlaybackPosition(float aTime)
{
  
  mCurrentFrameTime = aTime;
  if (!mPositionChangeQueued) {
    mPositionChangeQueued = PR_TRUE;
    nsCOMPtr<nsIRunnable> event =
      NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, PlaybackPositionChanged);
    NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
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

void nsOggDecodeStateMachine::SetContentLength(PRInt64 aLength)
{
  
  mContentLength = aLength;
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
  if (mPlayer) {
    oggplay_prepare_for_close(mPlayer);
  }
  LOG(PR_LOG_DEBUG, ("Changed state to SHUTDOWN"));
  mState = DECODER_STATE_SHUTDOWN;
  mon.NotifyAll();
}

void nsOggDecodeStateMachine::Decode()
{
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    LOG(PR_LOG_DEBUG, ("Changed state from BUFFERING to DECODING"));
    mState = DECODER_STATE_DECODING;
  }
}

void nsOggDecodeStateMachine::Seek(float aTime)
{
  nsAutoMonitor mon(mDecoder->GetMonitor());
  mSeekTime = aTime;
  LOG(PR_LOG_DEBUG, ("Changed state to SEEKING (to %f)", aTime));
  mState = DECODER_STATE_SEEKING;
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
      return NS_OK;

    case DECODER_STATE_DECODING_METADATA:
      mon.Exit();
      LoadOggHeaders(reader);
      mon.Enter();
      
      if (mState == DECODER_STATE_DECODING_METADATA) {
        LOG(PR_LOG_DEBUG, ("Changed state from DECODING_METADATA to DECODING_FIRSTFRAME"));
        mState = DECODER_STATE_DECODING_FIRSTFRAME;
      }
      break;

    case DECODER_STATE_DECODING_FIRSTFRAME:
      {
        OggPlayErrorCode r;
        do {
          mon.Exit();
          r = DecodeFrame();
          mon.Enter();
        } while (mState != DECODER_STATE_SHUTDOWN && r == E_OGGPLAY_TIMEOUT);

        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        mLastFrameTime = 0;
        FrameData* frame = NextFrame();
        if (frame) {
          mDecodedFrames.Push(frame);
          mDecoder->mPlaybackPosition = frame->mEndStreamPosition;
          UpdatePlaybackPosition(frame->mDecodedFrameTime);
          PlayVideo(frame);
        }

        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, FirstFrameLoaded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

        if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
          LOG(PR_LOG_DEBUG, ("Changed state from DECODING_FIRSTFRAME to DECODING"));
          mState = DECODER_STATE_DECODING;
        }
      }
      break;

    case DECODER_STATE_DECODING:
      {
        PRBool bufferExhausted = PR_FALSE;

        if (!mDecodedFrames.IsFull()) {
          PRInt64 initialDownloadPosition = mDecoder->mDownloadPosition;

          mon.Exit();
          OggPlayErrorCode r = DecodeFrame();
          mon.Enter();

          
          
          
          bufferExhausted =
            mDecoder->mDecoderPosition > initialDownloadPosition;

          if (mState != DECODER_STATE_DECODING)
            continue;

          
          FrameData* frame = NextFrame();
          if (frame) {
            mDecodedFrames.Push(frame);
          }

          if (r != E_OGGPLAY_CONTINUE &&
              r != E_OGGPLAY_USER_INTERRUPT &&
              r != E_OGGPLAY_TIMEOUT)  {
            LOG(PR_LOG_DEBUG, ("Changed state from DECODING to COMPLETED"));
            mState = DECODER_STATE_COMPLETED;
          }
        }

        
        
        if (!mPlaying && !mDecodedFrames.IsEmpty()) {
          PlayVideo(mDecodedFrames.Peek());
        }

        if (bufferExhausted && mState == DECODER_STATE_DECODING &&
            mDecoder->GetState() == nsOggDecoder::PLAY_STATE_PLAYING &&
            (mDecoder->mTotalBytes < 0 ||
             mDecoder->mDownloadPosition < mDecoder->mTotalBytes)) {
          
          
          
          if (mPlaying) {
            StopPlayback();
          }

          
          
          
          
          
          
          
          
          
          nsCOMPtr<nsIRunnable> event = 
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, UpdateReadyStateForData);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

          mBufferingStart = PR_IntervalNow();
          double playbackRate = mDecoder->GetStatistics().mPlaybackRate;
          mBufferingEndOffset = mDecoder->mDownloadPosition +
              BUFFERING_RATE(playbackRate) * BUFFERING_WAIT;
          mState = DECODER_STATE_BUFFERING;
          LOG(PR_LOG_DEBUG, ("Changed state from DECODING to BUFFERING"));
        } else {
          PlayFrame();
        }
      }
      break;

    case DECODER_STATE_SEEKING:
      {
        
        
        
        
        
        
        
        
        float seekTime = mSeekTime;
        mDecoder->StopProgressUpdates();
        mon.Exit();
        nsCOMPtr<nsIRunnable> startEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStarted);
        NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        
        LOG(PR_LOG_DEBUG, ("Entering oggplay_seek(%f)", seekTime));
        oggplay_seek(mPlayer, ogg_int64_t(seekTime * 1000));
        LOG(PR_LOG_DEBUG, ("Leaving oggplay_seek"));

        
        
        
        for (int i = 0; i < oggplay_get_num_tracks(mPlayer); ++i) {
         if (oggplay_set_track_active(mPlayer, i) < 0)  {
            LOG(PR_LOG_ERROR, ("Could not set track %d active", i));
          }
        }

        mon.Enter();
        mLastFramePosition = mDecoder->mDecoderPosition;
        mDecoder->StartProgressUpdates();
        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        
        while (!mDecodedFrames.IsEmpty()) {
          delete mDecodedFrames.Pop();
        }

        OggPlayErrorCode r;
        do {
          mon.Exit();
          r = DecodeFrame();
          mon.Enter();
        } while (mState != DECODER_STATE_SHUTDOWN && r == E_OGGPLAY_TIMEOUT);

        if (mState == DECODER_STATE_SHUTDOWN)
          continue;

        mLastFrameTime = 0;
        FrameData* frame = NextFrame();
        NS_ASSERTION(frame != nsnull, "No frame after seek!");
        if (frame) {
          mDecodedFrames.Push(frame);
          UpdatePlaybackPosition(frame->mDecodedFrameTime);
          PlayVideo(frame);
        }
        mon.Exit();
        nsCOMPtr<nsIRunnable> stopEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStopped);
        NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);        
        mon.Enter();

        if (mState == DECODER_STATE_SEEKING && mSeekTime == seekTime) {
          LOG(PR_LOG_DEBUG, ("Changed state from SEEKING (to %f) to DECODING", seekTime));
          mState = DECODER_STATE_DECODING;
        }
      }
      break;

    case DECODER_STATE_BUFFERING:
      {
        PRIntervalTime now = PR_IntervalNow();
        if ((PR_IntervalToMilliseconds(now - mBufferingStart) < BUFFERING_WAIT*1000) &&
            mDecoder->mDownloadPosition < mBufferingEndOffset &&
            (mDecoder->mTotalBytes < 0 || mDecoder->mDownloadPosition < mDecoder->mTotalBytes)) {
          LOG(PR_LOG_DEBUG, 
              ("In buffering: buffering data until %d bytes available or %d milliseconds", 
               PRUint32(mBufferingEndOffset - mDecoder->mDownloadPosition),
               BUFFERING_WAIT*1000 - (PR_IntervalToMilliseconds(now - mBufferingStart))));
          mon.Wait(PR_MillisecondsToInterval(1000));
          if (mState == DECODER_STATE_SHUTDOWN)
            continue;
        } else {
          LOG(PR_LOG_DEBUG, ("Changed state from BUFFERING to DECODING"));
          mState = DECODER_STATE_DECODING;
        }

        if (mState != DECODER_STATE_BUFFERING) {
          nsCOMPtr<nsIRunnable> event = 
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, UpdateReadyStateForData);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
          if (mDecoder->GetState() == nsOggDecoder::PLAY_STATE_PLAYING) {
            if (!mPlaying) {
              StartPlayback();
            }
          }
        }

        break;
      }

    case DECODER_STATE_COMPLETED:
      {
        while (mState == DECODER_STATE_COMPLETED &&
               !mDecodedFrames.IsEmpty()) {
          PlayFrame();
          if (mState != DECODER_STATE_SHUTDOWN) {
            
            
            
            mon.Wait(PR_MillisecondsToInterval(PRInt64(mCallbackPeriod*1000)));
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
          if (mState != DECODER_STATE_COMPLETED)
            continue;
        }

        nsCOMPtr<nsIRunnable> event =
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, PlaybackEnded);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
        do {
          mon.Wait();
        } while (mState == DECODER_STATE_COMPLETED);
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

        int y_width;
        int y_height;
        oggplay_get_video_y_size(mPlayer, i, &y_width, &y_height);
        mDecoder->SetRGBData(y_width, y_height, mFramerate, nsnull);
      }
      else if (mAudioTrack == -1 && oggplay_get_track_type(mPlayer, i) == OGGZ_CONTENT_VORBIS) {
        mAudioTrack = i;
        oggplay_set_offset(mPlayer, i, OGGPLAY_AUDIO_OFFSET);
        oggplay_get_audio_samplerate(mPlayer, i, &mAudioRate);
        oggplay_get_audio_channels(mPlayer, i, &mAudioChannels);
        LOG(PR_LOG_DEBUG, ("samplerate: %d, channels: %d", mAudioRate, mAudioChannels));
      }
 
      if (oggplay_set_track_active(mPlayer, i) < 0)  {
        LOG(PR_LOG_ERROR, ("Could not set track %d active", i));
      }
    }

    if (mVideoTrack == -1) {
      oggplay_set_callback_num_frames(mPlayer, mAudioTrack, OGGPLAY_FRAMES_PER_CALLBACK);
      mCallbackPeriod = 1.0 / (float(mAudioRate) / OGGPLAY_FRAMES_PER_CALLBACK);
    }
    LOG(PR_LOG_DEBUG, ("Callback Period: %f", mCallbackPeriod));

    oggplay_use_buffer(mPlayer, OGGPLAY_BUFFER_SIZE);

    
    
    
    
    
    {
      nsAutoMonitor mon(mDecoder->GetMonitor());
      if (mState != DECODER_STATE_SHUTDOWN &&
          mContentLength >= 0 && 
          mSeekable &&
          mDuration == -1) {
        mDecoder->StopProgressUpdates();
        
        
        
        mon.Exit();
        PRInt64 d = oggplay_get_duration(mPlayer);
        oggplay_seek(mPlayer, 0);
        mon.Enter();
        mDuration = d;
        mDecoder->StartProgressUpdates();
      }
      if (mState == DECODER_STATE_SHUTDOWN)
        return;
    }

    
    nsCOMPtr<nsIRunnable> metadataLoadedEvent = 
      NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, MetadataLoaded); 
    
    NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsOggDecoder, nsIObserver)

void nsOggDecoder::Pause() 
{
  nsAutoMonitor mon(mMonitor);
  if (mPlayState == PLAY_STATE_SEEKING) {
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
  mTotalBytes(-1),
  mDownloadPosition(0),
  mProgressPosition(0),
  mDecoderPosition(0),
  mPlaybackPosition(0),
  mCurrentTime(0.0),
  mInitialVolume(0.0),
  mRequestedSeekTime(-1.0),
  mDuration(-1),
  mNotifyOnShutdown(PR_FALSE),
  mSeekable(PR_TRUE),
  mReader(0),
  mMonitor(0),
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

void nsOggDecoder::Shutdown()
{
  mShuttingDown = PR_TRUE;

  ChangeState(PLAY_STATE_SHUTDOWN);
  nsMediaDecoder::Shutdown();

  Stop();
}

nsOggDecoder::~nsOggDecoder()
{
  MOZ_COUNT_DTOR(nsOggDecoder);
  nsAutoMonitor::DestroyMonitor(mMonitor);
}

nsresult nsOggDecoder::Load(nsIURI* aURI, nsIChannel* aChannel,
                            nsIStreamListener** aStreamListener)
{
  
  
  mStopping = PR_FALSE;

  
  mDownloadPosition = 0;
  mProgressPosition = 0;
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
  mDownloadStatistics.Reset();
  mDownloadStatistics.Start(PR_IntervalNow());

  nsresult rv = mReader->Init(this, mURI, aChannel, aStreamListener);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mDecodeThread));
  NS_ENSURE_SUCCESS(rv, rv);

  mDecodeStateMachine = new nsOggDecodeStateMachine(this);
  {
    nsAutoMonitor mon(mMonitor);
    mDecodeStateMachine->SetContentLength(mTotalBytes);
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
    mNextState = mPlayState;
    ChangeState(PLAY_STATE_SEEKING);
  }

  return NS_OK;
}

nsresult nsOggDecoder::PlaybackRateChanged()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}



class nsDestroyStateMachine : public nsRunnable {
public:
  nsDestroyStateMachine(nsOggDecoder *aDecoder,
                        nsOggDecodeStateMachine *aMachine,
                        nsChannelReader *aReader,
                        nsIThread *aThread)
  : mDecoder(aDecoder),
    mDecodeStateMachine(aMachine),
    mReader(aReader),
    mDecodeThread(aThread)
  {
  }

  NS_IMETHOD Run() {
    NS_ASSERTION(NS_IsMainThread(), "Should be called on main thread");
    
    
    
    if (mDecodeThread)
      mDecodeThread->Shutdown();
    mDecodeThread = nsnull;
    mDecodeStateMachine = nsnull;
    mReader = nsnull;
    mDecoder = nsnull;
    return NS_OK;
  }

private:
  nsRefPtr<nsOggDecoder> mDecoder;
  nsCOMPtr<nsOggDecodeStateMachine> mDecodeStateMachine;
  nsAutoPtr<nsChannelReader> mReader;
  nsCOMPtr<nsIThread> mDecodeThread;
};

void nsOggDecoder::Stop()
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsOggDecoder::Stop called on non-main thread");  
  
  if (mStopping)
    return;

  mStopping = PR_TRUE;

  ChangeState(PLAY_STATE_ENDED);

  StopProgress();
  mDownloadStatistics.Stop(PR_IntervalNow());

  
  
  if (mReader) {
    mReader->Cancel();
  }

  
  
  
  if (mDecodeStateMachine) {
    mDecodeStateMachine->Shutdown();
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsCOMPtr<nsIRunnable> event = new nsDestroyStateMachine(this,
                                                          mDecodeStateMachine,
                                                          mReader.forget(),
                                                          mDecodeThread);
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);

  
  mDecodeThread = nsnull;
  mDecodeStateMachine = nsnull;
  UnregisterShutdownObserver();
}

float nsOggDecoder::GetCurrentTime()
{
  return mCurrentTime;
}

void nsOggDecoder::GetCurrentURI(nsIURI** aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
}

nsIPrincipal* nsOggDecoder::GetCurrentPrincipal()
{
  if (!mReader) {
    return nsnull;
  }

  return mReader->GetCurrentPrincipal();
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
}

void nsOggDecoder::FirstFrameLoaded()
{
  if (mShuttingDown)
    return;
 
  
  
  PRBool notifyElement = PR_TRUE;
  {
    nsAutoMonitor mon(mMonitor);
    notifyElement = mNextState != PLAY_STATE_SEEKING;
  }  

  if (mElement && notifyElement) {
    mElement->FirstFrameLoaded();
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

  if (!mResourceLoaded && mDownloadPosition == mTotalBytes) {
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

    
    
    NS_ASSERTION(mDownloadPosition == mTotalBytes, "Wrong byte count");

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
  if (mStopping || mShuttingDown)
    return;

  if (mElement)
    mElement->NetworkError();
  Stop();
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

  Stop();
  if (mElement)  {
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
  result.mDownloadRate =
    mDownloadStatistics.GetRate(PR_IntervalNow(), &result.mDownloadRateReliable);
  if (mDuration >= 0 && mTotalBytes >= 0) {
    result.mPlaybackRate = double(mTotalBytes)*1000.0/mDuration;
    result.mPlaybackRateReliable = PR_TRUE;
  } else {
    result.mPlaybackRate =
      mPlaybackStatistics.GetRateAtLastStop(&result.mPlaybackRateReliable);
  }
  result.mTotalBytes = mTotalBytes;
  
  
  
  result.mDownloadPosition = mProgressPosition;
  result.mDecoderPosition = mDecoderPosition;
  result.mPlaybackPosition = mPlaybackPosition;
  return result;
}

void nsOggDecoder::SetTotalBytes(PRInt64 aBytes)
{
  nsAutoMonitor mon(mMonitor);

  
  
  
  mTotalBytes = PR_MAX(mDownloadPosition, aBytes);
  if (mDecodeStateMachine) {
    mDecodeStateMachine->SetContentLength(mTotalBytes);
  }
}

void nsOggDecoder::NotifyBytesDownloaded(PRInt64 aBytes)
{
  NS_ASSERTION(NS_IsMainThread(), 
               "nsOggDecoder::NotifyBytesDownloaded called on non-main thread");   
  {
    nsAutoMonitor mon(mMonitor);

    mDownloadPosition += aBytes;
    if (mTotalBytes >= 0) {
      
      mTotalBytes = PR_MAX(mTotalBytes, mDownloadPosition);
    }
    if (!mIgnoreProgressData) {
      mDownloadStatistics.AddBytes(aBytes);
      mProgressPosition = mDownloadPosition;
    }
  }

  UpdateReadyStateForData();
}

void nsOggDecoder::NotifyDownloadSeeked(PRInt64 aOffsetBytes)
{
  nsAutoMonitor mon(mMonitor);
  
  mDownloadPosition = mDecoderPosition = mPlaybackPosition = aOffsetBytes;
  if (!mIgnoreProgressData) {
    mProgressPosition = mDownloadPosition;
  }
  if (mTotalBytes >= 0) {
    
    mTotalBytes = PR_MAX(mTotalBytes, mDownloadPosition);
  }
}

void nsOggDecoder::NotifyDownloadEnded(nsresult aStatus)
{
  if (aStatus == NS_BINDING_ABORTED)
    return;

  {
    nsAutoMonitor mon(mMonitor);
    mDownloadStatistics.Stop(PR_IntervalNow());
    if (NS_SUCCEEDED(aStatus)) {
      
      mTotalBytes = mDownloadPosition;
    }
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
    if (mDecodeStateMachine->HaveNextFrameData()) {
      frameStatus = nsHTMLMediaElement::NEXT_FRAME_AVAILABLE;
    } else if (mDecodeStateMachine->IsBuffering()) {
      frameStatus = nsHTMLMediaElement::NEXT_FRAME_UNAVAILABLE_BUFFERING;
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
    mElement->SeekCompleted();
    UpdateReadyStateForData();
  }
}

void nsOggDecoder::SeekingStarted()
{
  if (mShuttingDown)
    return;

  if (mElement) {
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

  if (mPlayState == PLAY_STATE_ENDED &&
      aState != PLAY_STATE_SHUTDOWN) {
    
    
    
    
    
    
    mNextState = aState;
    mPlayState = PLAY_STATE_LOADING;
    Load(mURI, nsnull, nsnull);
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

    if (mReader) {
      mReader->SetDuration(mDuration);
    }
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
  mDownloadStatistics.Stop(PR_IntervalNow());
  if (mReader) {
    mReader->Suspend();
  }
}

void nsOggDecoder::Resume()
{
  if (mReader) {
    mReader->Resume();
  }
  mDownloadStatistics.Start(PR_IntervalNow());
}

void nsOggDecoder::StopProgressUpdates()
{
  mIgnoreProgressData = PR_TRUE;
  mDownloadStatistics.Stop(PR_IntervalNow());
}

void nsOggDecoder::StartProgressUpdates()
{
  mIgnoreProgressData = PR_FALSE;
  
  mProgressPosition = mDownloadPosition;
  mDownloadStatistics.Start(PR_IntervalNow());
}
