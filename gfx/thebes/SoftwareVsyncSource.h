





#ifndef GFX_SOFTWARE_VSYNC_SOURCE_H
#define GFX_SOFTWARE_VSYNC_SOURCE_H

#include "mozilla/Monitor.h"
#include "mozilla/RefPtr.h"
#include "mozilla/TimeStamp.h"
#include "base/thread.h"
#include "nsISupportsImpl.h"
#include "VsyncSource.h"

class CancelableTask;

class SoftwareDisplay final : public mozilla::gfx::VsyncSource::Display
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(SoftwareDisplay)

public:
  SoftwareDisplay();
  virtual void EnableVsync() override;
  virtual void DisableVsync() override;
  virtual bool IsVsyncEnabled() override;
  bool IsInSoftwareVsyncThread();
  virtual void NotifyVsync(mozilla::TimeStamp aVsyncTimestamp) override;
  void ScheduleNextVsync(mozilla::TimeStamp aVsyncTimestamp);
  void Shutdown();

protected:
  ~SoftwareDisplay();

private:
  mozilla::TimeDuration mVsyncRate;
  
  base::Thread* mVsyncThread;
  CancelableTask* mCurrentVsyncTask; 
  bool mVsyncEnabled; 
}; 




class SoftwareVsyncSource : public mozilla::gfx::VsyncSource
{
public:
  SoftwareVsyncSource();
  ~SoftwareVsyncSource();

  virtual Display& GetGlobalDisplay() override
  {
    MOZ_ASSERT(mGlobalDisplay != nullptr);
    return *mGlobalDisplay;
  }

private:
  nsRefPtr<SoftwareDisplay> mGlobalDisplay;
};

#endif 
