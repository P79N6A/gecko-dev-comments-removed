






































#include "mozilla/dom/AudioChild.h"

namespace mozilla {
namespace dom {

NS_IMPL_THREADSAFE_ADDREF(AudioChild);
NS_IMPL_THREADSAFE_RELEASE(AudioChild);

AudioChild::AudioChild()
  : mLastSampleOffset(-1),
    mLastSampleOffsetTime(0),
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
AudioChild::RecvDrainDone()
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  mDrained = PR_TRUE;
  mAudioReentrantMonitor.NotifyAll();
  return true;
}

void
AudioChild::WaitForDrain()
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  while (!mDrained && mIPCOpen) {
    mAudioReentrantMonitor.Wait();
  }
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
