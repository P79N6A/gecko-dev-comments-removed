






































#include "mozilla/dom/AudioChild.h"

namespace mozilla {
namespace dom {
NS_IMPL_THREADSAFE_ADDREF(AudioChild);
NS_IMPL_THREADSAFE_RELEASE(AudioChild);

AudioChild::AudioChild()
  : mLastSampleOffset(-1),
    mLastSampleOffsetTime(0),
    mMinWriteSample(-2),
    mAudioReentrantMonitor("AudioChild.mReentrantMonitor"),
    mIPCOpen(PR_TRUE),
    mDrained(PR_FALSE)
{
  MOZ_COUNT_CTOR(AudioChild);
}

AudioChild::~AudioChild()
{
  MOZ_COUNT_DTOR(AudioChild);
}

void
AudioChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mIPCOpen = PR_FALSE;
}

bool
AudioChild::RecvSampleOffsetUpdate(const PRInt64& offset,
                                   const PRInt64& time)
{
  mLastSampleOffset = offset;
  mLastSampleOffsetTime = time;
  return true;
}

bool
AudioChild::RecvDrainDone(const nsresult& status)
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);

  if (status == NS_OK)
    mDrained = PR_TRUE;

  mAudioReentrantMonitor.NotifyAll();
  return true;
}

PRInt32
AudioChild::WaitForMinWriteSample()
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  
  while (mMinWriteSample == -2 && mIPCOpen)
    mAudioReentrantMonitor.Wait();
  return mMinWriteSample;
}

bool
AudioChild::RecvMinWriteSampleDone(const PRInt32& minSamples)
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  mMinWriteSample = minSamples;
  mAudioReentrantMonitor.NotifyAll();
  return true;
}

nsresult
AudioChild::WaitForDrain()
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  while (!mDrained && mIPCOpen) {
    mAudioReentrantMonitor.Wait();
  }
  if (mDrained)
    return NS_OK;

  return NS_ERROR_FAILURE;
}

PRInt64
AudioChild::GetLastKnownSampleOffset()
{
  return mLastSampleOffset;
}

PRInt64
AudioChild::GetLastKnownSampleOffsetTime()
{
  return mLastSampleOffsetTime;
}

} 
} 
