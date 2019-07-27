





#if !defined(MediaTimer_h_)
#define MediaTimer_h_

#include "MediaPromise.h"

#include <queue>

#include "nsITimer.h"
#include "nsRefPtr.h"

#include "mozilla/Monitor.h"
#include "mozilla/TimeStamp.h"

namespace mozilla {



typedef MediaPromise<bool, bool,  true> MediaTimerPromise;





class MediaTimer
{
public:
  MediaTimer();

  
  NS_IMETHOD_(MozExternalRefCountType) AddRef(void);
  NS_IMETHOD_(MozExternalRefCountType) Release(void);

  nsRefPtr<MediaTimerPromise> WaitUntil(const TimeStamp& aTimeStamp, const char* aCallSite);

private:
  virtual ~MediaTimer() { MOZ_ASSERT(OnMediaTimerThread()); }

  void DispatchDestroy(); 
  void Destroy(); 

  bool OnMediaTimerThread();
  void ScheduleUpdate();
  void Update();
  void UpdateLocked();

  static void TimerCallback(nsITimer* aTimer, void* aClosure);
  void TimerFired();
  void ArmTimer(const TimeStamp& aTarget, const TimeStamp& aNow);

  bool TimerIsArmed()
  {
    return !mCurrentTimerTarget.IsNull();
  }

  void CancelTimerIfArmed()
  {
    MOZ_ASSERT(OnMediaTimerThread());
    if (TimerIsArmed()) {
      mTimer->Cancel();
      mCurrentTimerTarget = TimeStamp();
    }
  }


  struct Entry
  {
    TimeStamp mTimeStamp;
    nsRefPtr<MediaTimerPromise::Private> mPromise;

    explicit Entry(const TimeStamp& aTimeStamp, const char* aCallSite)
      : mTimeStamp(aTimeStamp)
      , mPromise(new MediaTimerPromise::Private(aCallSite))
    {}

    
    
    
    bool operator<(const Entry& aOther) const
    {
      return mTimeStamp > aOther.mTimeStamp;
    }
  };

  ThreadSafeAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD
  nsCOMPtr<nsIEventTarget> mThread;
  std::priority_queue<Entry> mEntries;
  Monitor mMonitor;
  nsCOMPtr<nsITimer> mTimer;
  TimeStamp mCurrentTimerTarget;
  bool mUpdateScheduled;
};

} 

#endif
