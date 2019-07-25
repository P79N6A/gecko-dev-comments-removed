






































#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/PAudioChild.h"
#include "mozilla/dom/AudioChild.h"
#include "nsXULAppAPI.h"
using namespace mozilla::dom;

#include <stdio.h>
#include <math.h>
#include "prlog.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsAudioStream.h"
#include "nsAlgorithm.h"
#include "VideoUtils.h"
extern "C" {
#include "sydneyaudio/sydney_audio.h"
}
#include "mozilla/TimeStamp.h"
#include "nsThreadUtils.h"

#if defined(XP_MACOSX)
#define SA_PER_STREAM_VOLUME 1
#endif



#if defined(ANDROID)
#define REMOTE_AUDIO 1
#endif

using mozilla::TimeStamp;

#ifdef PR_LOGGING
PRLogModuleInfo* gAudioStreamLog = nsnull;
#endif

#define FAKE_BUFFER_SIZE 176400

class nsAudioStreamLocal : public nsAudioStream
{
 public:
  NS_DECL_ISUPPORTS

  ~nsAudioStreamLocal();
  nsAudioStreamLocal();

  nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat);
  void Shutdown();
  nsresult Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking);
  PRUint32 Available();
  void SetVolume(double aVolume);
  void Drain();
  void Pause();
  void Resume();
  PRInt64 GetPosition();
  PRInt64 GetSampleOffset();
  PRBool IsPaused();
  PRInt32 GetMinWriteSamples();

 private:

  double mVolume;
  void* mAudioHandle;
  int mRate;
  int mChannels;

  SampleFormat mFormat;

  
  
  
  
  nsTArray<short> mBufferOverflow;

  
  PRPackedBool mPaused;

  
  PRPackedBool mInError;

};

class nsAudioStreamRemote : public nsAudioStream
{
 public:
  NS_DECL_ISUPPORTS

  nsAudioStreamRemote();
  ~nsAudioStreamRemote();

  nsresult Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat);
  void Shutdown();
  nsresult Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking);
  PRUint32 Available();
  void SetVolume(double aVolume);
  void Drain();
  void Pause();
  void Resume();
  PRInt64 GetPosition();
  PRInt64 GetSampleOffset();
  PRBool IsPaused();
  PRInt32 GetMinWriteSamples();

  nsRefPtr<AudioChild> mAudioChild;

  SampleFormat mFormat;
  int mRate;
  int mChannels;

  PRInt32 mBytesPerSample;

  
  PRPackedBool mPaused;

  friend class AudioInitEvent;
};

class AudioInitEvent : public nsRunnable
{
 public:
  AudioInitEvent(nsAudioStreamRemote* owner)
  {
    mOwner = owner;
  }

  NS_IMETHOD Run()
  {
    ContentChild * cpc = ContentChild::GetSingleton();
    NS_ASSERTION(cpc, "Content Protocol is NULL!");
    mOwner->mAudioChild =  static_cast<AudioChild*> (cpc->SendPAudioConstructor(mOwner->mChannels,
                                                                                mOwner->mRate,
                                                                                mOwner->mFormat));
    return NS_OK;
  }
  
  nsRefPtr<nsAudioStreamRemote> mOwner;
};

class AudioWriteEvent : public nsRunnable
{
 public:
  AudioWriteEvent(AudioChild* aChild,
                  const void* aBuf,
                  PRUint32 aNumberOfSamples,
                  PRUint32 aBytesPerSample)
  {    
    mAudioChild = aChild;
    mBytesPerSample = aBytesPerSample;
    mBuffer.Assign((const char*)aBuf, aNumberOfSamples*aBytesPerSample);
  }

  NS_IMETHOD Run()
  {
    if (!mAudioChild->IsIPCOpen())
      return NS_OK;

    mAudioChild->SendWrite(mBuffer,
                           mBuffer.Length() / mBytesPerSample);
    return NS_OK;
  }

  nsRefPtr<AudioChild> mAudioChild;
  nsCString mBuffer;
  PRUint32 mBytesPerSample;
};

class AudioSetVolumeEvent : public nsRunnable
{
 public:
  AudioSetVolumeEvent(AudioChild* aChild, double aVolume)
  {
    mAudioChild = aChild;
    mVolume = aVolume;
  }

  NS_IMETHOD Run()
  {
    if (!mAudioChild->IsIPCOpen())
      return NS_OK;

    mAudioChild->SendSetVolume(mVolume);
    return NS_OK;
  }
  
  nsRefPtr<AudioChild> mAudioChild;
  double mVolume;
};

class AudioDrainEvent : public nsRunnable
{
 public:
  AudioDrainEvent(AudioChild* aChild)
  {
    mAudioChild = aChild;
  }

  NS_IMETHOD Run()
  {
    if (!mAudioChild->IsIPCOpen())
      return NS_OK;

    mAudioChild->SendDrain();
    return NS_OK;
  }
  
  nsRefPtr<AudioChild> mAudioChild;
};


class AudioPauseEvent : public nsRunnable
{
 public:
  AudioPauseEvent(AudioChild* aChild, PRBool pause)
  {
    mAudioChild = aChild;
    mPause = pause;
  }

  NS_IMETHOD Run()
  {
    if (!mAudioChild->IsIPCOpen())
      return NS_OK;

    if (mPause)
      mAudioChild->SendPause();
    else
      mAudioChild->SendResume();

    return NS_OK;
  }
  
  nsRefPtr<AudioChild> mAudioChild;
  PRBool mPause;
};


class AudioShutdownEvent : public nsRunnable
{
 public:
  AudioShutdownEvent(AudioChild* aChild)
  {
    mAudioChild = aChild;
  }

  NS_IMETHOD Run()
  {
    if (mAudioChild->IsIPCOpen())
      mAudioChild->SendShutdown();
    return NS_OK;
  }
  
  nsRefPtr<AudioChild> mAudioChild;
};


void nsAudioStream::InitLibrary()
{
#ifdef PR_LOGGING
  gAudioStreamLog = PR_NewLogModule("nsAudioStream");
#endif
}

void nsAudioStream::ShutdownLibrary()
{
}

nsIThread *
nsAudioStream::GetThread()
{
  if (!mAudioPlaybackThread) {
    NS_NewThread(getter_AddRefs(mAudioPlaybackThread));
  }
  return mAudioPlaybackThread;
}

nsAudioStream* nsAudioStream::AllocateStream()
{
#if defined(REMOTE_AUDIO)
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    return new nsAudioStreamRemote();
  }
#endif
  return new nsAudioStreamLocal();
}

class AsyncShutdownPlaybackThread : public nsRunnable
{
public:
  AsyncShutdownPlaybackThread(nsIThread* aThread) : mThread(aThread) {}
  NS_IMETHODIMP Run() { return mThread->Shutdown(); }
private:
  nsCOMPtr<nsIThread> mThread;
};

nsAudioStream::~nsAudioStream()
{
  if (mAudioPlaybackThread) {
    nsCOMPtr<nsIRunnable> event = new AsyncShutdownPlaybackThread(mAudioPlaybackThread);
    NS_DispatchToMainThread(event);
  }
}

nsAudioStreamLocal::nsAudioStreamLocal() :
  mVolume(1.0),
  mAudioHandle(0),
  mRate(0),
  mChannels(0),
  mFormat(FORMAT_S16_LE),
  mPaused(PR_FALSE),
  mInError(PR_FALSE)
{
}

nsAudioStreamLocal::~nsAudioStreamLocal()
{
  Shutdown();
}

NS_IMPL_THREADSAFE_ISUPPORTS0(nsAudioStreamLocal)

nsresult nsAudioStreamLocal::Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat)
{
  mRate = aRate;
  mChannels = aNumChannels;
  mFormat = aFormat;

  if (sa_stream_create_pcm(reinterpret_cast<sa_stream_t**>(&mAudioHandle),
                           NULL, 
                           SA_MODE_WRONLY, 
                           SA_PCM_FORMAT_S16_NE,
                           aRate,
                           aNumChannels) != SA_SUCCESS) {
    mAudioHandle = nsnull;
    mInError = PR_TRUE;
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_create_pcm error"));
    return NS_ERROR_FAILURE;
  }
  
  if (sa_stream_open(static_cast<sa_stream_t*>(mAudioHandle)) != SA_SUCCESS) {
    sa_stream_destroy(static_cast<sa_stream_t*>(mAudioHandle));
    mAudioHandle = nsnull;
    mInError = PR_TRUE;
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_open error"));
    return NS_ERROR_FAILURE;
  }
  mInError = PR_FALSE;

  return NS_OK;
}

void nsAudioStreamLocal::Shutdown()
{
  if (!mAudioHandle)
    return;

  sa_stream_destroy(static_cast<sa_stream_t*>(mAudioHandle));
  mAudioHandle = nsnull;
  mInError = PR_TRUE;
}

nsresult nsAudioStreamLocal::Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking)
{
  NS_ABORT_IF_FALSE(aCount % mChannels == 0,
                    "Buffer size must be divisible by channel count");
  NS_ASSERTION(!mPaused, "Don't write audio when paused, you'll block");

  if (mInError)
    return NS_ERROR_FAILURE;

  PRUint32 offset = mBufferOverflow.Length();
  PRUint32 count = aCount + offset;

  nsAutoArrayPtr<short> s_data(new short[count]);

  if (s_data) {
    for (PRUint32 i=0; i < offset; ++i) {
      s_data[i] = mBufferOverflow.ElementAt(i);
    }
    mBufferOverflow.Clear();

    switch (mFormat) {
      case FORMAT_U8: {
        const PRUint8* buf = static_cast<const PRUint8*>(aBuf);
        PRInt32 volume = PRInt32((1 << 16) * mVolume);
        for (PRUint32 i = 0; i < aCount; ++i) {
          s_data[i + offset] = short(((PRInt32(buf[i]) - 128) * volume) >> 8);
        }
        break;
      }
      case FORMAT_S16_LE: {
        const short* buf = static_cast<const short*>(aBuf);
        PRInt32 volume = PRInt32((1 << 16) * mVolume);
        for (PRUint32 i = 0; i < aCount; ++i) {
          short s = buf[i];
#if defined(IS_BIG_ENDIAN)
          s = ((s & 0x00ff) << 8) | ((s & 0xff00) >> 8);
#endif
          s_data[i + offset] = short((PRInt32(s) * volume) >> 16);
        }
        break;
      }
      case FORMAT_FLOAT32: {
        const float* buf = static_cast<const float*>(aBuf);
        for (PRUint32 i = 0; i <  aCount; ++i) {
          float scaled_value = floorf(0.5 + 32768 * buf[i] * mVolume);
          if (buf[i] < 0.0) {
            s_data[i + offset] = (scaled_value < -32768.0) ?
              -32768 :
              short(scaled_value);
          } else {
            s_data[i+offset] = (scaled_value > 32767.0) ?
              32767 :
              short(scaled_value);
          }
        }
        break;
      }
    }

    if (!aBlocking) {
      
      
      
      PRUint32 available = Available();
      if (available < count) {
        mBufferOverflow.AppendElements(s_data.get() + available, (count - available));
        count = available;
      }
    }

    if (sa_stream_write(static_cast<sa_stream_t*>(mAudioHandle),
                        s_data.get(),
                        count * sizeof(short)) != SA_SUCCESS)
    {
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_write error"));
      mInError = PR_TRUE;
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

PRUint32 nsAudioStreamLocal::Available()
{
  
  
  if (mInError)
    return FAKE_BUFFER_SIZE;

  size_t s = 0; 
  if (sa_stream_get_write_size(static_cast<sa_stream_t*>(mAudioHandle), &s) != SA_SUCCESS)
    return 0;

  return s / sizeof(short);
}

void nsAudioStreamLocal::SetVolume(double aVolume)
{
  NS_ASSERTION(aVolume >= 0.0 && aVolume <= 1.0, "Invalid volume");
#if defined(SA_PER_STREAM_VOLUME)
  if (sa_stream_set_volume_abs(static_cast<sa_stream_t*>(mAudioHandle), aVolume) != SA_SUCCESS) {
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_set_volume_abs error"));
    mInError = PR_TRUE;
  }
#else
  mVolume = aVolume;
#endif
}

void nsAudioStreamLocal::Drain()
{
  NS_ASSERTION(!mPaused, "Don't drain audio when paused, it won't finish!");

  if (mInError)
    return;

  
  if (!mBufferOverflow.IsEmpty()) {
    if (sa_stream_write(static_cast<sa_stream_t*>(mAudioHandle),
                        mBufferOverflow.Elements(),
                        mBufferOverflow.Length() * sizeof(short)) != SA_SUCCESS)
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_write error"));
      mInError = PR_TRUE;
      return;
  }

  int r = sa_stream_drain(static_cast<sa_stream_t*>(mAudioHandle));
  if (r != SA_SUCCESS && r != SA_ERROR_INVALID) {
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStreamLocal: sa_stream_drain error"));
    mInError = PR_TRUE;
  }
}

void nsAudioStreamLocal::Pause()
{
  if (mInError)
    return;
  mPaused = PR_TRUE;
  sa_stream_pause(static_cast<sa_stream_t*>(mAudioHandle));
}

void nsAudioStreamLocal::Resume()
{
  if (mInError)
    return;
  mPaused = PR_FALSE;
  sa_stream_resume(static_cast<sa_stream_t*>(mAudioHandle));
}

PRInt64 nsAudioStreamLocal::GetPosition()
{
  PRInt64 sampleOffset = GetSampleOffset();
  if (sampleOffset >= 0) {
    return ((USECS_PER_S * sampleOffset) / mRate / mChannels);
  }
  return -1;
}

PRInt64 nsAudioStreamLocal::GetSampleOffset()
{
  if (mInError) {
    return -1;
  }
 
  sa_position_t positionType = SA_POSITION_WRITE_SOFTWARE;
#if defined(XP_WIN)
  positionType = SA_POSITION_WRITE_HARDWARE;
#endif
  int64_t position = 0;
  if (sa_stream_get_position(static_cast<sa_stream_t*>(mAudioHandle),
                             positionType, &position) == SA_SUCCESS) {
    return position / sizeof(short);
  }

  return -1;
}

PRBool nsAudioStreamLocal::IsPaused()
{
  return mPaused;
}

PRInt32 nsAudioStreamLocal::GetMinWriteSamples()
{
  size_t samples;
  int r = sa_stream_get_min_write(static_cast<sa_stream_t*>(mAudioHandle),
                                  &samples);
  if (r == SA_ERROR_NOT_SUPPORTED)
    return 1;
  else if (r != SA_SUCCESS || samples > PR_INT32_MAX)
    return -1;

  return static_cast<PRInt32>(samples);
}

nsAudioStreamRemote::nsAudioStreamRemote()
 : mAudioChild(NULL),
   mFormat(FORMAT_S16_LE),
   mRate(0),
   mChannels(0),
   mBytesPerSample(1),
   mPaused(PR_FALSE)
{}

nsAudioStreamRemote::~nsAudioStreamRemote()
{
  Shutdown();
}

NS_IMPL_THREADSAFE_ISUPPORTS0(nsAudioStreamRemote)

nsresult 
nsAudioStreamRemote::Init(PRInt32 aNumChannels,
                          PRInt32 aRate,
                          SampleFormat aFormat)
{
  mRate = aRate;
  mChannels = aNumChannels;
  mFormat = aFormat;

  switch (mFormat) {
    case FORMAT_U8: {
      mBytesPerSample = sizeof(PRUint8);
      break;
    }
    case FORMAT_S16_LE: {
      mBytesPerSample = sizeof(short);
      break;
    }
    case FORMAT_FLOAT32: {
      mBytesPerSample = sizeof(float);
    }
  }

  nsCOMPtr<nsIRunnable> event = new AudioInitEvent(this);
  NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
  return NS_OK;
}

void
nsAudioStreamRemote::Shutdown()
{
  if (!mAudioChild)
    return;
  nsCOMPtr<nsIRunnable> event = new AudioShutdownEvent(mAudioChild);
  NS_DispatchToMainThread(event);
  mAudioChild = nsnull;
}

nsresult
nsAudioStreamRemote::Write(const void* aBuf,
                           PRUint32 aCount,
                           PRBool aBlocking)
{
  if (!mAudioChild)
    return NS_ERROR_FAILURE;
  nsCOMPtr<nsIRunnable> event = new AudioWriteEvent(mAudioChild,
                                                    aBuf,
                                                    aCount,
                                                    mBytesPerSample);
  NS_DispatchToMainThread(event);
  return NS_OK;
}

PRUint32
nsAudioStreamRemote::Available()
{
  return FAKE_BUFFER_SIZE;
}

PRInt32 nsAudioStreamRemote::GetMinWriteSamples()
{
  

  return 1;
}

void
nsAudioStreamRemote::SetVolume(double aVolume)
{
  if (!mAudioChild)
    return;
  nsCOMPtr<nsIRunnable> event = new AudioSetVolumeEvent(mAudioChild, aVolume);
  NS_DispatchToMainThread(event);
}

void
nsAudioStreamRemote::Drain()
{
  if (!mAudioChild)
    return;
  nsCOMPtr<nsIRunnable> event = new AudioDrainEvent(mAudioChild);
  NS_DispatchToMainThread(event);
  mAudioChild->WaitForDrain();
}
 
void
nsAudioStreamRemote::Pause()
{
  mPaused = PR_TRUE;
  if (!mAudioChild)
    return;
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mAudioChild, PR_TRUE);
  NS_DispatchToMainThread(event);
}

void
nsAudioStreamRemote::Resume()
{
  mPaused = PR_FALSE;
  if (!mAudioChild)
    return;
  nsCOMPtr<nsIRunnable> event = new AudioPauseEvent(mAudioChild, PR_FALSE);
  NS_DispatchToMainThread(event);
}

PRInt64 nsAudioStreamRemote::GetPosition()
{
  PRInt64 sampleOffset = GetSampleOffset();
  if (sampleOffset >= 0) {
    return ((USECS_PER_S * sampleOffset) / mRate / mChannels);
  }
  return 0;
}

PRInt64
nsAudioStreamRemote::GetSampleOffset()
{
  if(!mAudioChild)
    return 0;

  PRInt64 offset = mAudioChild->GetLastKnownSampleOffset();
  if (offset == -1)
    return 0;

  PRInt64 time   = mAudioChild->GetLastKnownSampleOffsetTime();
  PRInt64 result = offset + (mRate * mChannels * (PR_IntervalNow() - time) / USECS_PER_S);

  return result;
}

PRBool
nsAudioStreamRemote::IsPaused()
{
  return mPaused;
}
