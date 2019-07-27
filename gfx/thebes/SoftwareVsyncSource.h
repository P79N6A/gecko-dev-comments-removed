





#ifndef GFX_SOFTWARE_VSYNC_SOURCE_H
#define GFX_SOFTWARE_VSYNC_SOURCE_H

#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "base/thread.h"
#include "nsISupportsImpl.h"
#include "VsyncSource.h"

class CancelableTask;

class SoftwareDisplay MOZ_FINAL : public mozilla::gfx::VsyncSource::Display
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SoftwareDisplay)

public:
  SoftwareDisplay();
  virtual void EnableVsync() MOZ_OVERRIDE;
  virtual void DisableVsync() MOZ_OVERRIDE;
  virtual bool IsVsyncEnabled() MOZ_OVERRIDE;
  bool IsInSoftwareVsyncThread();
  virtual void NotifyVsync(mozilla::TimeStamp aVsyncTimestamp) MOZ_OVERRIDE;
  void ScheduleNextVsync(mozilla::TimeStamp aVsyncTimestamp);

protected:
  ~SoftwareDisplay();

private:
  mozilla::TimeDuration mVsyncRate;
  
  base::Thread* mVsyncThread;
  bool mVsyncEnabled;
  CancelableTask* mCurrentVsyncTask;
  
  mozilla::Monitor mCurrentTaskMonitor;
}; 




class SoftwareVsyncSource : public mozilla::gfx::VsyncSource
{
public:
  SoftwareVsyncSource();
  ~SoftwareVsyncSource();

  virtual Display& GetGlobalDisplay() MOZ_OVERRIDE
  {
    MOZ_ASSERT(mGlobalDisplay != nullptr);
    return *mGlobalDisplay;
  }

private:
  nsRefPtr<SoftwareDisplay> mGlobalDisplay;
};

#endif 
