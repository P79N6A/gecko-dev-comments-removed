






































#include "mozilla/dom/AudioChild.h"

namespace mozilla {
namespace dom {
NS_IMPL_THREADSAFE_ADDREF(AudioChild);
NS_IMPL_THREADSAFE_RELEASE(AudioChild);

AudioChild::AudioChild()
  : mLastPosition(-1),
    mLastPositionTimestamp(0),
    mMinWriteSize(-2),
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
AudioChild::RecvPositionInFramesUpdate(const PRInt64& position,
                                       const PRInt64& time)
{
  mLastPosition = position;
  mLastPositionTimestamp = time;
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

PRInt32
AudioChild::WaitForMinWriteSize()
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  
  while (mMinWriteSize == -2 && mIPCOpen) {
    mAudioReentrantMonitor.Wait();
  }
  return mMinWriteSize;
}

bool
AudioChild::RecvMinWriteSizeDone(const PRInt32& minFrames)
{
  ReentrantMonitorAutoEnter mon(mAudioReentrantMonitor);
  mMinWriteSize = minFrames;
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
AudioChild::GetLastKnownPosition()
{
  return mLastPosition;
}

PRInt64
AudioChild::GetLastKnownPositionTimestamp()
{
  return mLastPositionTimestamp;
}

} 
} 
