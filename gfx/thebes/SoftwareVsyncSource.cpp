





#include "SoftwareVsyncSource.h"
#include "base/task.h"
#include "nsThreadUtils.h"

SoftwareVsyncSource::SoftwareVsyncSource()
{
  MOZ_ASSERT(NS_IsMainThread());
  mGlobalDisplay = new SoftwareDisplay();
}

SoftwareVsyncSource::~SoftwareVsyncSource()
{
  MOZ_ASSERT(NS_IsMainThread());
  
  mGlobalDisplay->DisableVsync();
  mGlobalDisplay = nullptr;
}

SoftwareDisplay::SoftwareDisplay()
  : mVsyncEnabled(false)
  , mCurrentTaskMonitor("SoftwareVsyncCurrentTaskMonitor")
{
  
  MOZ_ASSERT(NS_IsMainThread());
  const double rate = 1000 / 60.0;
  mVsyncRate = mozilla::TimeDuration::FromMilliseconds(rate);
  mVsyncThread = new base::Thread("SoftwareVsyncThread");
  MOZ_RELEASE_ASSERT(mVsyncThread->Start(), "Could not start software vsync thread");
}

void
SoftwareDisplay::EnableVsync()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (IsVsyncEnabled()) {
    return;
  }

  { 
    mozilla::MonitorAutoLock lock(mCurrentTaskMonitor);
    mVsyncEnabled = true;
    MOZ_ASSERT(mVsyncThread->IsRunning());
    mCurrentVsyncTask = NewRunnableMethod(this,
        &SoftwareDisplay::NotifyVsync,
        mozilla::TimeStamp::Now());
    mVsyncThread->message_loop()->PostTask(FROM_HERE, mCurrentVsyncTask);
  }
}

void
SoftwareDisplay::DisableVsync()
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!IsVsyncEnabled()) {
    return;
  }

  MOZ_ASSERT(mVsyncThread->IsRunning());
  { 
    mozilla::MonitorAutoLock lock(mCurrentTaskMonitor);
    mVsyncEnabled = false;
    if (mCurrentVsyncTask) {
      mCurrentVsyncTask->Cancel();
      mCurrentVsyncTask = nullptr;
    }
  }
}

bool
SoftwareDisplay::IsVsyncEnabled()
{
  MOZ_ASSERT(NS_IsMainThread());
  mozilla::MonitorAutoLock lock(mCurrentTaskMonitor);
  return mVsyncEnabled;
}

bool
SoftwareDisplay::IsInSoftwareVsyncThread()
{
  return mVsyncThread->thread_id() == PlatformThread::CurrentId();
}

void
SoftwareDisplay::NotifyVsync(mozilla::TimeStamp aVsyncTimestamp)
{
  MOZ_ASSERT(IsInSoftwareVsyncThread());

  mozilla::TimeStamp displayVsyncTime = aVsyncTimestamp;
  mozilla::TimeStamp now = mozilla::TimeStamp::Now();
  
  
  
  
  if (aVsyncTimestamp > now) {
    displayVsyncTime = now;
  }

  Display::NotifyVsync(displayVsyncTime);

  
  
  ScheduleNextVsync(aVsyncTimestamp);
}

void
SoftwareDisplay::ScheduleNextVsync(mozilla::TimeStamp aVsyncTimestamp)
{
  MOZ_ASSERT(IsInSoftwareVsyncThread());
  mozilla::TimeStamp nextVsync = aVsyncTimestamp + mVsyncRate;
  mozilla::TimeDuration delay = nextVsync - mozilla::TimeStamp::Now();
  if (delay.ToMilliseconds() < 0) {
    delay = mozilla::TimeDuration::FromMilliseconds(0);
  }

  mozilla::MonitorAutoLock lock(mCurrentTaskMonitor);
  
  
  if (!mVsyncEnabled) {
    return;
  }
  mCurrentVsyncTask = NewRunnableMethod(this,
      &SoftwareDisplay::NotifyVsync,
      nextVsync);

  mVsyncThread->message_loop()->PostDelayedTask(FROM_HERE,
      mCurrentVsyncTask,
      delay.ToMilliseconds());
}

SoftwareDisplay::~SoftwareDisplay()
{
  MOZ_ASSERT(NS_IsMainThread());
  mVsyncThread->Stop();
  delete mVsyncThread;
}
