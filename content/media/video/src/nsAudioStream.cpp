




































#include <stdio.h>
#include <math.h>
#include "prlog.h"
#include "prmem.h"
#include "nsAutoPtr.h"
#include "nsAudioStream.h"
extern "C" {
#include "sydneyaudio/sydney_audio.h"
}

#ifdef PR_LOGGING
PRLogModuleInfo* gAudioStreamLog = nsnull;
#endif

#define FAKE_BUFFER_SIZE 176400

static float CurrentTimeInSeconds()
{
  return PR_IntervalToMilliseconds(PR_IntervalNow()) / 1000.0;
}

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
  mSavedPauseBytes(0),
  mPauseBytes(0),
  mPauseTime(0.0),
  mSamplesBuffered(0),
  mPaused(PR_FALSE)
{
}

void nsAudioStream::Init(PRInt32 aNumChannels, PRInt32 aRate)
{
  mRate = aRate;
  mChannels = aNumChannels;
  mStartTime = CurrentTimeInSeconds();
  if (sa_stream_create_pcm(reinterpret_cast<sa_stream_t**>(&mAudioHandle),
                           NULL, 
                           SA_MODE_WRONLY, 
                           SA_PCM_FORMAT_S16_LE,
                           aRate,
                           aNumChannels) != SA_SUCCESS) {
    mAudioHandle = nsnull;
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_create_pcm error"));
    return;
  }
  
  if (sa_stream_open(reinterpret_cast<sa_stream_t*>(mAudioHandle)) != SA_SUCCESS) {
    sa_stream_destroy((sa_stream_t*)mAudioHandle);
    mAudioHandle = nsnull;
    PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_open error"));
    return;
  }
}

void nsAudioStream::Shutdown()
{
  if (!mAudioHandle) 
    return;

  sa_stream_destroy(reinterpret_cast<sa_stream_t*>(mAudioHandle));
  mAudioHandle = nsnull;
}

void nsAudioStream::Write(const float* aBuf, PRUint32 aCount)
{
  mSamplesBuffered += aCount;

  if (!mAudioHandle)
    return;

  
  nsAutoArrayPtr<short> s_data(new short[aCount]);

  if (s_data) {
    for (PRUint32 i=0; i <  aCount; ++i) {
      float scaled_value = floorf(0.5 + 32768 * aBuf[i] * mVolume);
      if (aBuf[i] < 0.0) {
        s_data[i] = (scaled_value < -32768.0) ? 
          -32768 : 
          short(scaled_value);
      }
      else {
        s_data[i] = (scaled_value > 32767.0) ? 
          32767 : 
          short(scaled_value);
      }
    }
    
    if (sa_stream_write(reinterpret_cast<sa_stream_t*>(mAudioHandle), s_data.get(), aCount * sizeof(short)) != SA_SUCCESS) {
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_write error"));
      Shutdown();
    }
  }
}

void nsAudioStream::Write(const short* aBuf, PRUint32 aCount)
{
  mSamplesBuffered += aCount;

  if (!mAudioHandle)
    return;

  nsAutoArrayPtr<short> s_data(new short[aCount]);

  if (s_data) {
    for (PRUint32 i = 0; i < aCount; ++i) {
      s_data[i] = aBuf[i] * mVolume;
    }

    if (sa_stream_write(reinterpret_cast<sa_stream_t*>(mAudioHandle), s_data.get(), aCount * sizeof(short)) != SA_SUCCESS) {
      PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_write error"));
      Shutdown();
    }
  }
}

PRInt32 nsAudioStream::Available()
{
  
  
  if (!mAudioHandle)
    return FAKE_BUFFER_SIZE;

  size_t s = 0; 
  sa_stream_get_write_size(reinterpret_cast<sa_stream_t*>(mAudioHandle), &s);
  return s / sizeof(short);
}

float nsAudioStream::GetVolume()
{
  return mVolume;
}

void nsAudioStream::SetVolume(float aVolume)
{
  mVolume = aVolume;
}

void nsAudioStream::Drain()
{
  if (!mAudioHandle) {
    PRUint32 drainTime = (float(mSamplesBuffered) / mRate / mChannels - GetTime()) * 1000.0;
    PR_Sleep(PR_MillisecondsToInterval(drainTime));
    return;
  }

  if (sa_stream_drain(reinterpret_cast<sa_stream_t*>(mAudioHandle)) != SA_SUCCESS) {
        PR_LOG(gAudioStreamLog, PR_LOG_ERROR, ("nsAudioStream: sa_stream_drain error"));
        Shutdown();
  }
}

void nsAudioStream::Pause()
{
  if (mPaused)
    return;

  
  
  mPauseTime = CurrentTimeInSeconds() - mStartTime;

  mPaused = PR_TRUE;

  if (!mAudioHandle)
    return;

  int64_t bytes = 0;
#if !defined(WIN32)
  sa_stream_get_position(reinterpret_cast<sa_stream_t*>(mAudioHandle), SA_POSITION_WRITE_SOFTWARE, &bytes);
#endif
  mSavedPauseBytes = bytes;

  sa_stream_pause(reinterpret_cast<sa_stream_t*>(mAudioHandle));
}

void nsAudioStream::Resume()
{
  if (!mPaused)
    return;

  
  
  mStartTime = CurrentTimeInSeconds() - mPauseTime;

  mPaused = PR_FALSE;

  if (!mAudioHandle)
    return;

  sa_stream_resume(reinterpret_cast<sa_stream_t*>(mAudioHandle));

#if !defined(WIN32)
  mPauseBytes += mSavedPauseBytes;
#endif
}

double nsAudioStream::GetTime()
{
  
  
  if (!mAudioHandle)
    return mPaused ? mPauseTime : CurrentTimeInSeconds() - mStartTime;

  int64_t bytes = 0;
#if defined(WIN32)
  sa_stream_get_position(reinterpret_cast<sa_stream_t*>(mAudioHandle), SA_POSITION_WRITE_HARDWARE, &bytes);
#else
  sa_stream_get_position(reinterpret_cast<sa_stream_t*>(mAudioHandle), SA_POSITION_WRITE_SOFTWARE, &bytes);
#endif
  return double(bytes + mPauseBytes) / (sizeof(short) * mChannels * mRate);
}
