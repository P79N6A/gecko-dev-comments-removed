




































#include <stdio.h>
#include <math.h>
#include "prlog.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsAudioStream.h"
#include "nsAlgorithm.h"
extern "C" {
#include "sydneyaudio/sydney_audio.h"
}
#include "mozilla/TimeStamp.h"

#if defined(XP_MACOSX)
#define SA_PER_STREAM_VOLUME 1
#endif

using mozilla::TimeStamp;

#ifdef PR_LOGGING
PRLogModuleInfo* gAudioStreamLog = nsnull;
#endif

#define FAKE_BUFFER_SIZE 176400
#define MILLISECONDS_PER_SECOND 1000

void nsAudioStream::InitLibrary()
{
#ifdef PR_LOGGING
  gAudioStreamLog = PR_NewLogModule("nsAudioStream");
#endif
}

void nsAudioStream::ShutdownLibrary()
{
}

nsAudioStream::nsAudioStream() :
  mVolume(1.0),
  mAudioHandle(0),
  mRate(0),
  mChannels(0),
  mFormat(FORMAT_S16_LE),
  mPaused(PR_FALSE),
  mInError(PR_FALSE)
{
}

nsAudioStream::~nsAudioStream()
{
  Shutdown();
}

nsresult nsAudioStream::Init(PRInt32 aNumChannels, PRInt32 aRate, SampleFormat aFormat)
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
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_create_pcm error"));
    return NS_ERROR_FAILURE;
  }
  
  if (sa_stream_open(static_cast<sa_stream_t*>(mAudioHandle)) != SA_SUCCESS) {
    sa_stream_destroy(static_cast<sa_stream_t*>(mAudioHandle));
    mAudioHandle = nsnull;
    mInError = PR_TRUE;
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_open error"));
    return NS_ERROR_FAILURE;
  }
  mInError = PR_FALSE;
  return NS_OK;
}

void nsAudioStream::Shutdown()
{
  if (!mAudioHandle) 
    return;

  sa_stream_destroy(static_cast<sa_stream_t*>(mAudioHandle));
  mAudioHandle = nsnull;
  mInError = PR_TRUE;
}

nsresult nsAudioStream::Write(const void* aBuf, PRUint32 aCount, PRBool aBlocking)
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
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_write error"));
      mInError = PR_TRUE;
      return NS_ERROR_FAILURE;
    }
  }
  return NS_OK;
}

PRUint32 nsAudioStream::Available()
{
  
  
  if (mInError)
    return FAKE_BUFFER_SIZE;

  size_t s = 0; 
  if (sa_stream_get_write_size(static_cast<sa_stream_t*>(mAudioHandle), &s) != SA_SUCCESS)
    return 0;

  return s / sizeof(short);
}

void nsAudioStream::SetVolume(float aVolume)
{
  NS_ASSERTION(aVolume >= 0.0 && aVolume <= 1.0, "Invalid volume");
#if defined(SA_PER_STREAM_VOLUME)
  if (sa_stream_set_volume_abs(static_cast<sa_stream_t*>(mAudioHandle), aVolume) != SA_SUCCESS) {
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_set_volume_abs error"));
    mInError = PR_TRUE;
  }
#else
  mVolume = aVolume;
#endif
}

void nsAudioStream::Drain()
{
  if (mInError)
    return;

  
  if (!mBufferOverflow.IsEmpty()) {
    if (sa_stream_write(static_cast<sa_stream_t*>(mAudioHandle),
                        mBufferOverflow.Elements(),
                        mBufferOverflow.Length() * sizeof(short)) != SA_SUCCESS)
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_write error"));
      mInError = PR_TRUE;
      return;
  }

  int r = sa_stream_drain(static_cast<sa_stream_t*>(mAudioHandle));
  if (r != SA_SUCCESS && r != SA_ERROR_INVALID) {
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_drain error"));
    mInError = PR_TRUE;
  }
}

void nsAudioStream::Pause()
{
  if (mInError)
    return;
  mPaused = PR_TRUE;
  sa_stream_pause(static_cast<sa_stream_t*>(mAudioHandle));
}

void nsAudioStream::Resume()
{
  if (mInError)
    return;
  mPaused = PR_FALSE;
  sa_stream_resume(static_cast<sa_stream_t*>(mAudioHandle));
}

PRInt64 nsAudioStream::GetPosition()
{
  PRInt64 sampleOffset = GetSampleOffset();
  if (sampleOffset >= 0) {
    return ((MILLISECONDS_PER_SECOND * sampleOffset) / mRate / mChannels);
  }

  return -1;
}

PRInt64 nsAudioStream::GetSampleOffset()
{
  if (mInError) {
    return -1;
  }
 
  sa_position_t positionType = SA_POSITION_WRITE_SOFTWARE;
#if defined(XP_WIN)
  positionType = SA_POSITION_WRITE_HARDWARE;
#endif
  PRInt64 position = 0;
  if (sa_stream_get_position(static_cast<sa_stream_t*>(mAudioHandle),
                             positionType, &position) == SA_SUCCESS) {
    return position / sizeof(short);
  }

  return -1;
}
