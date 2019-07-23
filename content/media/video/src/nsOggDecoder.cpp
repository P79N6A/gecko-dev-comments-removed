




































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
#include "nsIRenderingContext.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsPresContext.h"
#include "nsOggDecoder.h"





#define MAX_VIDEO_WIDTH  2000
#define MAX_VIDEO_HEIGHT 2000



#define OGGPLAY_BUFFER_SIZE 20



#define OGGPLAY_FRAMES_PER_CALLBACK 2048



#define OGGPLAY_AUDIO_OFFSET 250L



#define BUFFERING_WAIT 15




#define BUFFERING_MIN_RATE 50000
#define BUFFERING_RATE(x) ((x)< BUFFERING_MIN_RATE ? BUFFERING_MIN_RATE : (x))



#define BUFFERING_SECONDS_WATERMARK 1











class nsOggDisplayStateMachine : public nsRunnable
{
public:
  nsOggDisplayStateMachine(nsOggDecoder* aDecoder);
  ~nsOggDisplayStateMachine();

  
  
  float GetCurrentTime();

  
  
  float GetVolume();
  void SetVolume(float aVolume);

  NS_IMETHOD Run();

  
  
  
  void UpdateFrameTime(float aTime);

protected:
  
  
  void OpenAudioStream();

  
  
  
  void CloseAudioStream();

  
  
  void StartAudio();

  
  
  void StopAudio();

private:
  
  
  nsOggDecoder* mDecoder;

  
  
  
  nsAutoPtr<nsAudioStream> mAudioStream;

  
  
  
  
  float mCurrentFrameTime;

  
  
  
  PRIntervalTime mLastFrameTime;

  
  
  
  float mVolume;
};



































class nsOggDecodeStateMachine : public nsRunnable
{
public:
  
  enum State {
    DECODER_STATE_DECODING_METADATA,
    DECODER_STATE_DECODING_FIRSTFRAME,
    DECODER_STATE_DECODING,
    DECODER_STATE_SEEKING,
    DECODER_STATE_BUFFERING,
    DECODER_STATE_COMPLETED,
    DECODER_STATE_SHUTDOWN
  };

  nsOggDecodeStateMachine(nsOggDecoder* aDecoder, nsChannelReader* aReader);
  ~nsOggDecodeStateMachine();

  
  
  
  void Shutdown();
  void Decode();
  void Seek(float aTime);

  NS_IMETHOD Run();

  
  
  State GetState()
  {
    return mState;
  }

  
  
  
  
  
  float GetFramerate()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "GetFramerate() called during invalid state");
    
    return mFramerate;
  }

  PRBool HasAudio()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "HasAudio() called during invalid state");
    
    return mAudioTrack != -1;
  }

  PRInt32 GetAudioRate()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "GetAudioRate() called during invalid state");
    
    return mAudioRate;
  }

  PRInt32 GetAudioChannels()
  {
    NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "GetAudioChannels() called during invalid state");
    
    return mAudioChannels;
  }
  
  
  
  PRBool IsCompleted()
  {
    
    return 
      mState == DECODER_STATE_COMPLETED ||
      mState == DECODER_STATE_SHUTDOWN;
  }

  
  
  
  
  
  
  
  
  OggPlayErrorCode DecodeFrame();

  
  
  
  
  
  
  OggPlayCallbackInfo** NextFrame();

  
  
  
  
  void ReleaseFrame(OggPlayCallbackInfo** aFrame);

  
  
  
  
  
  
  float DisplayFrame(OggPlayCallbackInfo** aFrame, nsAudioStream* aAudioStream);

  
  
  
  float DisplayInitialFrame();

protected:
  
  
  
  
  
  float DisplayTrack(int aTrackNumber, OggPlayCallbackInfo* aTrack, nsAudioStream* aAudioStream);
  void HandleVideoData(int aTrackNum, OggPlayVideoData* aVideoData);
  void HandleAudioData(nsAudioStream* aAudioStream, OggPlayAudioData* aAudioData, int aSize);
  void CopyVideoFrame(int aTrackNum, OggPlayVideoData* aVideoData, float aFramerate);

  
  void LoadOggHeaders();
  void LoadFirstFrame();

private:
  
  
  nsOggDecoder* mDecoder;

  
  
  
  OggPlay* mPlayer;

  
  
  
  
  
  nsChannelReader* mReader;

  
  
  
  PRInt32 mVideoTrack;
  float   mFramerate;

  
  
  
  PRInt32 mAudioRate;
  PRInt32 mAudioChannels;
  PRInt32 mAudioTrack;

  
  
  
  State mState;

  
  
  PRIntervalTime mBufferingStart;

  
  
  PRUint32 mBufferingBytes;

  
  
  
  float mSeekTime;

  
  
  
  PRPackedBool mBufferFull;
};

nsOggDecodeStateMachine::nsOggDecodeStateMachine(nsOggDecoder* aDecoder, nsChannelReader* aReader) :
  mDecoder(aDecoder),
  mPlayer(0),
  mReader(aReader),
  mVideoTrack(-1),
  mFramerate(0.0),
  mAudioRate(0),
  mAudioChannels(0),
  mAudioTrack(-1),
  mState(DECODER_STATE_DECODING_METADATA),
  mBufferingStart(0),
  mBufferingBytes(0),
  mSeekTime(0.0),
  mBufferFull(PR_FALSE)
{
}

nsOggDecodeStateMachine::~nsOggDecodeStateMachine()
{
  oggplay_close(mPlayer);
}


OggPlayErrorCode nsOggDecodeStateMachine::DecodeFrame()
{
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "DecodeFrame() called during invalid state");
  return oggplay_step_decoding(mPlayer);
}

OggPlayCallbackInfo** nsOggDecodeStateMachine::NextFrame()
{
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "NextFrame() called during invalid state");
  return oggplay_buffer_retrieve_next(mPlayer);
}

void nsOggDecodeStateMachine::ReleaseFrame(OggPlayCallbackInfo** aFrame)
{
  
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "ReleaseFrame() called during invalid state");
  oggplay_buffer_release(mPlayer, aFrame);
  mBufferFull = PR_FALSE;
}

float nsOggDecodeStateMachine::DisplayFrame(OggPlayCallbackInfo** aFrame, nsAudioStream* aAudioStream)
{
  NS_ASSERTION(mState > DECODER_STATE_DECODING_METADATA, "DisplayFrame() called during invalid state");
  
  int num_tracks = oggplay_get_num_tracks(mPlayer);
  float audioTime = 0.0;
  float videoTime = 0.0;

  if (mVideoTrack != -1 && num_tracks > mVideoTrack) {
    videoTime = DisplayTrack(mVideoTrack, aFrame[mVideoTrack], aAudioStream);
  }

  if (aAudioStream && mAudioTrack != -1 && num_tracks > mAudioTrack) {
    audioTime = DisplayTrack(mAudioTrack, aFrame[mAudioTrack], aAudioStream);
  }

  return videoTime > audioTime ? videoTime : audioTime;
}

float nsOggDecodeStateMachine::DisplayInitialFrame()
{
  
  float time = 0.0;
  OggPlayCallbackInfo **frame = NextFrame();
  while (!frame) {
    OggPlayErrorCode r = DecodeFrame();
    if (r != E_OGGPLAY_CONTINUE &&
        r != E_OGGPLAY_USER_INTERRUPT &&
        r != E_OGGPLAY_TIMEOUT) {
      break;
    }

    mBufferFull = (r == E_OGGPLAY_USER_INTERRUPT);
    frame = NextFrame();
  }

  if (frame) {
    time = DisplayFrame(frame, nsnull);
    ReleaseFrame(frame);
    mBufferFull = PR_FALSE;
  }
  return time;
}

float nsOggDecodeStateMachine::DisplayTrack(int aTrackNumber, OggPlayCallbackInfo* aTrack, nsAudioStream* aAudioStream)
{
  OggPlayDataType type = oggplay_callback_info_get_type(aTrack);
  OggPlayDataHeader ** headers = oggplay_callback_info_get_headers(aTrack);
  float time = 0.0;

  switch(type) {
  case OGGPLAY_INACTIVE:
    {
      break;
    }
    
  case OGGPLAY_YUV_VIDEO:
    {
      time = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;          
      OggPlayVideoData* video_data = oggplay_callback_info_get_video_data(headers[0]);
      HandleVideoData(aTrackNumber, video_data);
    }
    break;
  case OGGPLAY_FLOATS_AUDIO:
    {
      time = ((float)oggplay_callback_info_get_presentation_time(headers[0]))/1000.0;
      int required = oggplay_callback_info_get_required(aTrack);
      for (int j = 0; j < required; ++j) {
        int size = oggplay_callback_info_get_record_size(headers[j]);
        OggPlayAudioData* audio_data = oggplay_callback_info_get_audio_data(headers[j]);
        HandleAudioData(aAudioStream, audio_data, size);
      }
      break;
    }
  case OGGPLAY_CMML:
    {
      if (oggplay_callback_info_get_required(aTrack) > 0) {
        LOG(PR_LOG_DEBUG, ("CMML: %s", oggplay_callback_info_get_text_data(headers[0])));
      }
      break;
    }
  default:
    break;
  }
  return time;
}

void nsOggDecodeStateMachine::HandleVideoData(int aTrackNum, OggPlayVideoData* aVideoData) {
  CopyVideoFrame(aTrackNum, aVideoData, mFramerate);

  nsCOMPtr<nsIRunnable> event = 
    NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, Invalidate); 
  
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

void nsOggDecodeStateMachine::HandleAudioData(nsAudioStream* aAudioStream, OggPlayAudioData* aAudioData, int aSize) {
  if (aAudioStream) {
    
    
    nsresult rv = aAudioStream->Write(reinterpret_cast<float*>(aAudioData), aSize * mAudioChannels);
    if (!NS_SUCCEEDED(rv)) {
      LOG(PR_LOG_ERROR, ("Could not write audio data to pipe"));
    }
  }
}

void nsOggDecodeStateMachine::CopyVideoFrame(int aTrackNum, OggPlayVideoData* aVideoData, float aFramerate) 
{
  int y_width;
  int y_height;
  oggplay_get_video_y_size(mPlayer, aTrackNum, &y_width, &y_height);
  int uv_width;
  int uv_height;
  oggplay_get_video_uv_size(mPlayer, aTrackNum, &uv_width, &uv_height);

  if (y_width >= MAX_VIDEO_WIDTH || y_height >= MAX_VIDEO_HEIGHT) {
    return;
  }

  {
    nsAutoLock lock(mDecoder->mVideoUpdateLock);

    mDecoder->SetRGBData(y_width, y_height, aFramerate, nsnull);

    
    
    
    if (mDecoder->mRGB) {
      OggPlayYUVChannels yuv;
      OggPlayRGBChannels rgb;
      
      yuv.ptry = aVideoData->y;
      yuv.ptru = aVideoData->u;
      yuv.ptrv = aVideoData->v;
      yuv.uv_width = uv_width;
      yuv.uv_height = uv_height;
      yuv.y_width = y_width;
      yuv.y_height = y_height;
      
      rgb.ptro = mDecoder->mRGB.get();
      rgb.rgb_width = mDecoder->mRGBWidth;
      rgb.rgb_height = mDecoder->mRGBHeight;

      oggplay_yuv2bgr(&yuv, &rgb);
    }
  }
}


void nsOggDecodeStateMachine::Shutdown()
{
  
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());
  oggplay_prepare_for_close(mPlayer);
  mState = DECODER_STATE_SHUTDOWN;
  mon.NotifyAll();
}

void nsOggDecodeStateMachine::Decode()
{
  
  
  nsAutoMonitor mon(mDecoder->GetMonitor());
  if (mState == DECODER_STATE_BUFFERING) {
    mState = DECODER_STATE_DECODING;
    mon.NotifyAll();
  }
}

void nsOggDecodeStateMachine::Seek(float aTime)
{
  nsAutoMonitor mon(mDecoder->GetMonitor());
  mSeekTime = aTime;
  mState = DECODER_STATE_SEEKING;
  mon.NotifyAll();
}

nsresult nsOggDecodeStateMachine::Run()
{
  nsAutoMonitor mon(mDecoder->GetMonitor());

  while (PR_TRUE) {
    switch(mState) {
    case DECODER_STATE_DECODING_METADATA:
      mon.Exit();
      LoadOggHeaders();
      mon.Enter();
      
      if (mState == DECODER_STATE_DECODING_METADATA) {
        mState = DECODER_STATE_DECODING_FIRSTFRAME;
      }

      mon.NotifyAll();
      break;

    case DECODER_STATE_DECODING_FIRSTFRAME:
      {
        mon.Exit();
        LoadFirstFrame();
        mon.Enter();

        if (mState == DECODER_STATE_DECODING_FIRSTFRAME) {
          mState = DECODER_STATE_DECODING;
        }

        mon.NotifyAll();
      }
      break;

    case DECODER_STATE_DECODING:
      {
        if (mReader->DownloadRate() >= 0 &&
            mReader->Available() < mReader->PlaybackRate() * BUFFERING_SECONDS_WATERMARK) {
          mBufferingStart = PR_IntervalNow();
          mBufferingBytes = PRUint32(BUFFERING_RATE(mReader->PlaybackRate()) * BUFFERING_WAIT);
          mState = DECODER_STATE_BUFFERING;
          mon.NotifyAll();

          nsCOMPtr<nsIRunnable> event =
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, BufferingStarted);
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
        }
        else {
          
          
          while (mBufferFull && mState == DECODER_STATE_DECODING) {
            mon.Wait();
          }

          
          if (mState != DECODER_STATE_DECODING) {
            continue;
          }

          mon.Exit();
          OggPlayErrorCode r = DecodeFrame();
          mon.Enter();

          mBufferFull = (r == E_OGGPLAY_USER_INTERRUPT);

          if (mState == DECODER_STATE_SHUTDOWN) {
            continue;
          }

          if (r != E_OGGPLAY_CONTINUE && 
              r != E_OGGPLAY_USER_INTERRUPT &&
              r != E_OGGPLAY_TIMEOUT)  {
            mState = DECODER_STATE_COMPLETED;
          }
          
          mon.NotifyAll();
        }
      }
      break;

    case DECODER_STATE_SEEKING:
      {
        
        
        
        
        
        
        
        
        float seekTime = mSeekTime;
        mon.Exit();
        nsCOMPtr<nsIRunnable> startEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStarted);
        NS_DispatchToMainThread(startEvent, NS_DISPATCH_SYNC);
        
        oggplay_seek(mPlayer, ogg_int64_t(seekTime * 1000));

        mon.Enter();
        if (mState == DECODER_STATE_SHUTDOWN) {
          continue;
        }

        mDecoder->mDisplayStateMachine->UpdateFrameTime(DisplayInitialFrame());
        mon.Exit();
        
        nsCOMPtr<nsIRunnable> stopEvent = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, SeekingStopped);
        NS_DispatchToMainThread(stopEvent, NS_DISPATCH_SYNC);        
        mon.Enter();

        if (mState == DECODER_STATE_SEEKING && mSeekTime == seekTime) {
          mState = DECODER_STATE_DECODING;
        }
        
        mon.NotifyAll();
      }
      break;

    case DECODER_STATE_BUFFERING:
      if ((PR_IntervalToMilliseconds(PR_IntervalNow() - mBufferingStart) < BUFFERING_WAIT*1000) &&
          mReader->DownloadRate() >= 0 &&            
          mReader->Available() < mBufferingBytes) {
        LOG(PR_LOG_DEBUG, 
            ("Buffering data until %d bytes available or %d milliseconds", 
             (long)(mBufferingBytes - mReader->Available()),
             BUFFERING_WAIT*1000 - (PR_IntervalToMilliseconds(PR_IntervalNow() - mBufferingStart))));
        
        mon.Wait(PR_MillisecondsToInterval(1000));
      }
      else
        mState = DECODER_STATE_DECODING;

      if (mState == DECODER_STATE_SHUTDOWN) {
        continue;
      }

      if (mState != DECODER_STATE_BUFFERING) {
        nsCOMPtr<nsIRunnable> event = 
          NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, BufferingStopped);
        NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
        mon.NotifyAll();
      }

      break;

    case DECODER_STATE_COMPLETED:
      mon.Wait();
      break;

    case DECODER_STATE_SHUTDOWN:
      return NS_OK;
    }
  }

  return NS_OK;
}

void nsOggDecodeStateMachine::LoadOggHeaders() 
{
  LOG(PR_LOG_DEBUG, ("Loading Ogg Headers"));

  mPlayer = oggplay_open_with_reader(mReader);
  if (mPlayer) {
    LOG(PR_LOG_DEBUG, ("There are %d tracks", oggplay_get_num_tracks(mPlayer)));

    for (int i = 0; i < oggplay_get_num_tracks(mPlayer); ++i) {
      LOG(PR_LOG_DEBUG, ("Tracks %d: %s", i, oggplay_get_track_typename(mPlayer, i)));
      if (mVideoTrack == -1 && oggplay_get_track_type(mPlayer, i) == OGGZ_CONTENT_THEORA) {
        oggplay_set_callback_num_frames(mPlayer, i, 1);
        mVideoTrack = i;
        int fpsd, fpsn;
        oggplay_get_video_fps(mPlayer, i, &fpsd, &fpsn);
        mFramerate = fpsd == 0 ? 0.0 : double(fpsn)/double(fpsd);
        LOG(PR_LOG_DEBUG, ("Frame rate: %f", mFramerate));
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
    }

    oggplay_use_buffer(mPlayer, OGGPLAY_BUFFER_SIZE);

    
    nsCOMPtr<nsIRunnable> metadataLoadedEvent = 
      NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, MetadataLoaded); 
    
    NS_DispatchToMainThread(metadataLoadedEvent, NS_DISPATCH_NORMAL);
  }
}

void nsOggDecodeStateMachine::LoadFirstFrame()
{
  NS_ASSERTION(!mBufferFull, "Buffer before reading first frame");
  if(DecodeFrame() == E_OGGPLAY_USER_INTERRUPT) {
    nsAutoMonitor mon(mDecoder->GetMonitor());
    mBufferFull = PR_TRUE;
  }

  nsCOMPtr<nsIRunnable> event = 
    NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, FirstFrameLoaded); 
    
  NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
}

nsOggDisplayStateMachine::nsOggDisplayStateMachine(nsOggDecoder* aDecoder) :
  mDecoder(aDecoder),
  mCurrentFrameTime(0.0),
  mLastFrameTime(0),
  mVolume(1.0)
{
}

nsOggDisplayStateMachine::~nsOggDisplayStateMachine()
{
}

float nsOggDisplayStateMachine::GetCurrentTime()
{
  
  return mCurrentFrameTime;
}


nsresult nsOggDisplayStateMachine::Run()
{
  nsOggDecodeStateMachine* sm = mDecoder->mDecodeStateMachine;
  PRBool playing = PR_FALSE;
  {
    nsAutoMonitor mon(mDecoder->GetMonitor());

    
    
    
    
    
    while (sm->GetState() <= nsOggDecodeStateMachine::DECODER_STATE_DECODING_FIRSTFRAME)
      mon.Wait();

    OggPlayCallbackInfo **frame = sm->NextFrame();
    while (!frame) {
      frame = sm->NextFrame();

      
      if (!frame) {
        mon.Wait();
      }

      if (sm->IsCompleted()) {
        sm->ReleaseFrame(frame);
        mon.NotifyAll();
        return NS_OK;
      }
    }
    mCurrentFrameTime = sm->DisplayFrame(frame, nsnull);
    sm->ReleaseFrame(frame);
    mon.NotifyAll();
  }

  while (PR_TRUE) {
    nsAutoMonitor mon(mDecoder->GetMonitor());
    nsOggDecoder::PlayState state = mDecoder->GetState();

    if (state == nsOggDecoder::PLAY_STATE_PLAYING) {
      if (!playing) {
        
        StartAudio();
        playing = PR_TRUE;
        mLastFrameTime = PR_IntervalNow();
      }
      
      
      PRIntervalTime target = PR_MillisecondsToInterval(PRInt64(1000.0 / sm->GetFramerate()));
      PRIntervalTime diff = PR_IntervalNow() - mLastFrameTime;
      while (diff < target) {
        mon.Wait(target-diff);
        if (mDecoder->GetState() >= nsOggDecoder::PLAY_STATE_ENDED) {
          return NS_OK;
        }
        diff = PR_IntervalNow() - mLastFrameTime;
      }
      mLastFrameTime = PR_IntervalNow();
      
      OggPlayCallbackInfo **frame = sm->NextFrame();
      if (frame) {
        mCurrentFrameTime = sm->DisplayFrame(frame, mAudioStream);
        sm->ReleaseFrame(frame);
        mon.NotifyAll();
      }
      else {
        if (sm->IsCompleted()) {
          nsCOMPtr<nsIRunnable> event = 
            NS_NEW_RUNNABLE_METHOD(nsOggDecoder, mDecoder, PlaybackEnded);
            
          NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);        
          return NS_OK;
        }

        
        
        mon.NotifyAll();

        mon.Wait();
      }
    }
    else {
      
      if (playing) {
        
        StopAudio();
        playing = PR_FALSE;
      }
      mon.Wait();
    }
    if (mDecoder->GetState() >= nsOggDecoder::PLAY_STATE_ENDED) {
      return NS_OK;
    }
  }

  return NS_OK;
}

void nsOggDisplayStateMachine::UpdateFrameTime(float aTime)
{
  mCurrentFrameTime = aTime;
}

void nsOggDisplayStateMachine::OpenAudioStream()
{
  
  nsOggDecodeStateMachine* sm = mDecoder->mDecodeStateMachine;
  mAudioStream = new nsAudioStream();
  if (!mAudioStream) {
    LOG(PR_LOG_ERROR, ("Could not create audio stream"));
  }
  else {
    mAudioStream->Init(sm->GetAudioChannels(), sm->GetAudioRate());
    mAudioStream->SetVolume(mVolume);
  }
}

void nsOggDisplayStateMachine::CloseAudioStream()
{
  
  if (mAudioStream) {
    mAudioStream->Shutdown();
    mAudioStream = nsnull;
  }
}

void nsOggDisplayStateMachine::StartAudio()
{
  
  nsOggDecodeStateMachine* sm = mDecoder->mDecodeStateMachine;
  if (sm->HasAudio()) {
    if (!mAudioStream) {
      OpenAudioStream();
    }
    else if(mAudioStream) {
      mAudioStream->Resume();
    }
  }
}

void nsOggDisplayStateMachine::StopAudio()
{
  
  nsOggDecodeStateMachine* sm = mDecoder->mDecodeStateMachine;
  if (sm->HasAudio()) {
    if (mDecoder->GetState() == nsOggDecoder::PLAY_STATE_SEEKING) {
      
      
      
      CloseAudioStream();
    }
    else if (mAudioStream) {
      mAudioStream->Pause();
    }
  }
}

float nsOggDisplayStateMachine::GetVolume()
{
  
  return mVolume;
}

void nsOggDisplayStateMachine::SetVolume(float volume)
{
  
  if (mAudioStream) {
    mAudioStream->SetVolume(volume);
  }

  mVolume = volume;
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

float nsOggDecoder::GetVolume()
{
  nsAutoMonitor mon(mMonitor);
  return mDisplayStateMachine ? mDisplayStateMachine->GetVolume() : mInitialVolume;
}

void nsOggDecoder::SetVolume(float volume)
{
  nsAutoMonitor mon(mMonitor);
  mInitialVolume = volume;

  if (mDisplayStateMachine) {
    mDisplayStateMachine->SetVolume(volume);
  }
}

float nsOggDecoder::GetDuration()
{
  
  
  
  return 0.0;
}

nsOggDecoder::nsOggDecoder() :
  nsMediaDecoder(),
  mBytesDownloaded(0),
  mInitialVolume(0.0),
  mSeekTime(-1.0),
  mNotifyOnShutdown(PR_FALSE),
  mReader(0),
  mMonitor(0),
  mPlayState(PLAY_STATE_PAUSED),
  mNextState(PLAY_STATE_PAUSED)
{
  MOZ_COUNT_CTOR(nsOggDecoder);
}

PRBool nsOggDecoder::Init() 
{
  mMonitor = nsAutoMonitor::NewMonitor("media.decoder");
  return mMonitor && nsMediaDecoder::Init();
}

void nsOggDecoder::Shutdown() 
{
  ChangeState(PLAY_STATE_SHUTDOWN);

  Stop();
  nsMediaDecoder::Shutdown();
}

nsOggDecoder::~nsOggDecoder()
{
  MOZ_COUNT_DTOR(nsOggDecoder);
  Shutdown();
  nsAutoMonitor::DestroyMonitor(mMonitor);
}

nsresult nsOggDecoder::Load(nsIURI* aURI) 
{
  nsresult rv;
  mURI = aURI;

  StartProgress();

  RegisterShutdownObserver();

  mReader = new nsChannelReader();
  NS_ENSURE_TRUE(mReader, NS_ERROR_OUT_OF_MEMORY);

  rv = mReader->Init(this, aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mDecodeThread));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_NewThread(getter_AddRefs(mDisplayThread));
  NS_ENSURE_SUCCESS(rv, rv);

  mDecodeStateMachine = new nsOggDecodeStateMachine(this, mReader);

  rv = mDecodeThread->Dispatch(mDecodeStateMachine, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  ChangeState(PLAY_STATE_LOADING);

  return NS_OK;
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

  if (mPlayState == PLAY_STATE_LOADING && aTime == 0.0) {
    return NS_OK;
  }

  mSeekTime = aTime;

  
  
  
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

void nsOggDecoder::Stop()
{
  ChangeState(PLAY_STATE_ENDED);

  StopProgress();

  
  
  if (mReader) {
    mReader->Cancel();
    mReader = nsnull;
  }

  
  
  
  
  if (mDecodeStateMachine) {
    mDecodeStateMachine->Shutdown();
  }

  
  
  
  if (mDecodeThread) {
    mDecodeThread->Shutdown();
    mDecodeThread = nsnull;
  }

  if (mDisplayThread) {
    mDisplayThread->Shutdown();
    mDisplayThread = nsnull;
  }
  
  mDecodeStateMachine = nsnull;
  mDisplayStateMachine = nsnull;

  UnregisterShutdownObserver();
}



float nsOggDecoder::GetCurrentTime()
{
  nsAutoMonitor mon(mMonitor);

  if (!mDisplayStateMachine) {
    return 0.0;
  }

  return mDisplayStateMachine->GetCurrentTime();
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
  if (mElement) {
    mElement->MetadataLoaded();
  }

  mDisplayStateMachine = new nsOggDisplayStateMachine(this);
  mDisplayThread->Dispatch(mDisplayStateMachine, NS_DISPATCH_NORMAL);
}

void nsOggDecoder::FirstFrameLoaded()
{
  if (mElement) {
    mElement->FirstFrameLoaded();
  }

  
  
  
  
  nsAutoMonitor mon(mMonitor);
  if (mPlayState == PLAY_STATE_LOADING) {
    if (mSeekTime >= 0.0)
      ChangeState(PLAY_STATE_SEEKING);
    else
      ChangeState(mNextState);
  }
}

void nsOggDecoder::ResourceLoaded()
{
  if (mElement) {
    mElement->ResourceLoaded();
  }
  StopProgress();
}

PRBool nsOggDecoder::IsSeeking() const
{
  return mPlayState == PLAY_STATE_SEEKING;
}

void nsOggDecoder::PlaybackEnded()
{
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

PRUint32 nsOggDecoder::GetBytesLoaded()
{
  return mBytesDownloaded;
}

PRUint32 nsOggDecoder::GetTotalBytes()
{
  
  return 0;
}

void nsOggDecoder::UpdateBytesDownloaded(PRUint32 aBytes)
{
  mBytesDownloaded = aBytes;
}

void nsOggDecoder::BufferingStopped()
{
  if (mElement) {
    mElement->ChangeReadyState(nsIDOMHTMLMediaElement::CAN_SHOW_CURRENT_FRAME);
  }
}

void nsOggDecoder::BufferingStarted()
{
  if (mElement) {
    mElement->ChangeReadyState(nsIDOMHTMLMediaElement::DATA_UNAVAILABLE);
  }
}

void nsOggDecoder::SeekingStopped()
{
  {
    nsAutoMonitor mon(mMonitor);
    if (mPlayState == PLAY_STATE_SHUTDOWN)
      return;

    
    
    if (mSeekTime >= 0.0)
      ChangeState(PLAY_STATE_SEEKING);
    else
      ChangeState(mNextState);
  }

  if (mElement) {
    mElement->SeekCompleted();
  }
}

void nsOggDecoder::SeekingStarted()
{
  {
    nsAutoMonitor mon(mMonitor);
    if (mPlayState == PLAY_STATE_SHUTDOWN)
      return;
  }

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
  nsAutoMonitor mon(mMonitor);

  if (mNextState == aState) {
    mNextState = PLAY_STATE_PAUSED;
  }

  if (mPlayState == PLAY_STATE_SHUTDOWN) {
    return;
  }

  if (mPlayState == PLAY_STATE_ENDED &&
      aState != PLAY_STATE_SHUTDOWN) {
    
    
    
    mNextState = aState;
    mPlayState = PLAY_STATE_LOADING;
    Load(mURI);
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
    mDecodeStateMachine->Seek(mSeekTime);
    mSeekTime = -1.0;
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

