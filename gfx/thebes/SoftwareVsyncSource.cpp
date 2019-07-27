





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
  mGlobalDisplay->Shutdown();
  mGlobalDisplay = nullptr;
}

SoftwareDisplay::SoftwareDisplay()
  : mCurrentVsyncTask(nullptr)
  , mVsyncEnabled(false)
{
  
  MOZ_ASSERT(NS_IsMainThread());
  const double rate = 1000 / 60.0;
  mVsyncRate = mozilla::TimeDuration::FromMilliseconds(rate);
  mVsyncThread = new base::Thread("SoftwareVsyncThread");
  MOZ_RELEASE_ASSERT(mVsyncThread->Start(), "Could not start software vsync thread");
}

SoftwareDisplay::~SoftwareDisplay() {}

void
SoftwareDisplay::EnableVsync()
{
  MOZ_ASSERT(mVsyncThread->IsRunning());
  if (NS_IsMainThread()) {
    if (mVsyncEnabled) {
      return;
    }
    mVsyncEnabled = true;

    mVsyncThread->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(this, &SoftwareDisplay::EnableVsync));
    return;
  }

  MOZ_ASSERT(IsInSoftwareVsyncThread());
  NotifyVsync(mozilla::TimeStamp::Now());
}

void
SoftwareDisplay::DisableVsync()
{
  MOZ_ASSERT(mVsyncThread->IsRunning());
  if (NS_IsMainThread()) {
    if (!mVsyncEnabled) {
      return;
    }
    mVsyncEnabled = false;

    mVsyncThread->message_loop()->PostTask(FROM_HERE,
      NewRunnableMethod(this, &SoftwareDisplay::DisableVsync));
    return;
  }

  MOZ_ASSERT(IsInSoftwareVsyncThread());
  if (mCurrentVsyncTask) {
    mCurrentVsyncTask->Cancel();
    mCurrentVsyncTask = nullptr;
  }
}

bool
SoftwareDisplay::IsVsyncEnabled()
{
  MOZ_ASSERT(NS_IsMainThread());
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
    nextVsync = mozilla::TimeStamp::Now();
  }

  mCurrentVsyncTask = NewRunnableMethod(this,
      &SoftwareDisplay::NotifyVsync,
      nextVsync);

  mVsyncThread->message_loop()->PostDelayedTask(FROM_HERE,
      mCurrentVsyncTask,
      delay.ToMilliseconds());
}

void
SoftwareDisplay::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  DisableVsync();
  mVsyncThread->Stop();
  delete mVsyncThread;
}
