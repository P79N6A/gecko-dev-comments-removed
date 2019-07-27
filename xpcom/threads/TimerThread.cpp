





#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsThreadUtils.h"
#include "pratom.h"

#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "mozilla/Services.h"
#include "mozilla/ChaosMode.h"
#include "mozilla/ArrayUtils.h"
#include "mozilla/BinarySearch.h"

#include <math.h>

using namespace mozilla;

NS_IMPL_ISUPPORTS(TimerThread, nsIRunnable, nsIObserver)

TimerThread::TimerThread() :
  mInitInProgress(false),
  mInitialized(false),
  mMonitor("TimerThread.mMonitor"),
  mShutdown(false),
  mWaiting(false),
  mNotified(false),
  mSleeping(false)
{
}

TimerThread::~TimerThread()
{
  mThread = nullptr;

  NS_ASSERTION(mTimers.IsEmpty(), "Timers remain in TimerThread::~TimerThread");
}

nsresult
TimerThread::InitLocks()
{
  return NS_OK;
}

namespace {

class TimerObserverRunnable : public nsRunnable
{
public:
  explicit TimerObserverRunnable(nsIObserver* aObserver)
    : mObserver(aObserver)
  {
  }

  NS_DECL_NSIRUNNABLE

private:
  nsCOMPtr<nsIObserver> mObserver;
};

NS_IMETHODIMP
TimerObserverRunnable::Run()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(mObserver, "sleep_notification", false);
    observerService->AddObserver(mObserver, "wake_notification", false);
    observerService->AddObserver(mObserver, "suspend_process_notification", false);
    observerService->AddObserver(mObserver, "resume_process_notification", false);
  }
  return NS_OK;
}

} 

nsresult
TimerThread::Init()
{
  PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
         ("TimerThread::Init [%d]\n", mInitialized));

  if (mInitialized) {
    if (!mThread) {
      return NS_ERROR_FAILURE;
    }

    return NS_OK;
  }

  if (mInitInProgress.exchange(true) == false) {
    
    nsresult rv = NS_NewThread(getter_AddRefs(mThread), this);
    if (NS_FAILED(rv)) {
      mThread = nullptr;
    } else {
      nsRefPtr<TimerObserverRunnable> r = new TimerObserverRunnable(this);
      if (NS_IsMainThread()) {
        r->Run();
      } else {
        NS_DispatchToMainThread(r);
      }
    }

    {
      MonitorAutoLock lock(mMonitor);
      mInitialized = true;
      mMonitor.NotifyAll();
    }
  } else {
    MonitorAutoLock lock(mMonitor);
    while (!mInitialized) {
      mMonitor.Wait();
    }
  }

  if (!mThread) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

nsresult
TimerThread::Shutdown()
{
  PR_LOG(GetTimerLog(), PR_LOG_DEBUG, ("TimerThread::Shutdown begin\n"));

  if (!mThread) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsTArray<nsTimerImpl*> timers;
  {
    
    MonitorAutoLock lock(mMonitor);

    mShutdown = true;

    
    if (mWaiting) {
      mNotified = true;
      mMonitor.Notify();
    }

    
    
    
    
    
    
    timers.AppendElements(mTimers);
    mTimers.Clear();
  }

  uint32_t timersCount = timers.Length();
  for (uint32_t i = 0; i < timersCount; i++) {
    nsTimerImpl* timer = timers[i];
    timer->ReleaseCallback();
    ReleaseTimerInternal(timer);
  }

  mThread->Shutdown();    

  PR_LOG(GetTimerLog(), PR_LOG_DEBUG, ("TimerThread::Shutdown end\n"));
  return NS_OK;
}

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

namespace {

struct MicrosecondsToInterval
{
  PRIntervalTime operator[](size_t aMs) const {
    return PR_MicrosecondsToInterval(aMs);
  }
};

struct IntervalComparator
{
  int operator()(PRIntervalTime aInterval) const {
    return (0 < aInterval) ? -1 : 1;
  }
};

} 


NS_IMETHODIMP
TimerThread::Run()
{
  PR_SetCurrentThreadName("Timer");

#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess()) {
    NS_ASSERTION(NuwaMarkCurrentThread,
                 "NuwaMarkCurrentThread is undefined!");
    NuwaMarkCurrentThread(nullptr, nullptr);
  }
#endif

  MonitorAutoLock lock(mMonitor);

  
  
  
  
  uint32_t usForPosInterval = 1;
  while (PR_MicrosecondsToInterval(usForPosInterval) == 0) {
    usForPosInterval <<= 1;
  }

  size_t usIntervalResolution;
  BinarySearchIf(MicrosecondsToInterval(), 0, usForPosInterval, IntervalComparator(), &usIntervalResolution);
  MOZ_ASSERT(PR_MicrosecondsToInterval(usIntervalResolution - 1) == 0);
  MOZ_ASSERT(PR_MicrosecondsToInterval(usIntervalResolution) == 1);

  
  
  int32_t halfMicrosecondsIntervalResolution = usIntervalResolution / 2;
  bool forceRunNextTimer = false;

  while (!mShutdown) {
    
    PRIntervalTime waitFor;
    bool forceRunThisTimer = forceRunNextTimer;
    forceRunNextTimer = false;

    if (mSleeping) {
      
      uint32_t milliseconds = 100;
      if (ChaosMode::isActive()) {
        milliseconds = ChaosMode::randomUint32LessThan(200);
      }
      waitFor = PR_MillisecondsToInterval(milliseconds);
    } else {
      waitFor = PR_INTERVAL_NO_TIMEOUT;
      TimeStamp now = TimeStamp::Now();
      nsTimerImpl* timer = nullptr;

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        if (now >= timer->mTimeout || forceRunThisTimer) {
    next:
          
          
          
          
          

          nsRefPtr<nsTimerImpl> timerRef(timer);
          RemoveTimerInternal(timer);
          timer = nullptr;

          {
            
            MonitorAutoUnlock unlock(mMonitor);

#ifdef DEBUG_TIMERS
            if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
              PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
                     ("Timer thread woke up %fms from when it was supposed to\n",
                      fabs((now - timerRef->mTimeout).ToMilliseconds())));
            }
#endif

            
            
            
            timerRef = nsTimerImpl::PostTimerEvent(timerRef.forget());

            if (timerRef) {
              
              
              
              nsrefcnt rc = timerRef.forget().take()->Release();
              (void)rc;

              
              
              
              
              
              
              
              
              
              
              
              MOZ_ASSERT(rc != 0, "destroyed timer off its target thread!");
            }
          }

          if (mShutdown) {
            break;
          }

          
          
          now = TimeStamp::Now();
        }
      }

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        TimeStamp timeout = timer->mTimeout;

        
        
        
        
        
        
        
        double microseconds = (timeout - now).ToMilliseconds() * 1000;

        if (ChaosMode::isActive()) {
          
          
          
          static const float sFractions[] = {
            0.0f, 0.25f, 0.5f, 0.75f, 1.0f, 1.75f, 2.75f
          };
          microseconds *=
            sFractions[ChaosMode::randomUint32LessThan(ArrayLength(sFractions))];
          forceRunNextTimer = true;
        }

        if (microseconds < halfMicrosecondsIntervalResolution) {
          forceRunNextTimer = false;
          goto next; 
        }
        waitFor = PR_MicrosecondsToInterval(
          static_cast<uint32_t>(microseconds)); 
        if (waitFor == 0) {
          waitFor = 1;  
        }
      }

#ifdef DEBUG_TIMERS
      if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
        if (waitFor == PR_INTERVAL_NO_TIMEOUT)
          PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
                 ("waiting for PR_INTERVAL_NO_TIMEOUT\n"));
        else
          PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
                 ("waiting for %u\n", PR_IntervalToMilliseconds(waitFor)));
      }
#endif
    }

    mWaiting = true;
    mNotified = false;
    mMonitor.Wait(waitFor);
    if (mNotified) {
      forceRunNextTimer = false;
    }
    mWaiting = false;
  }

  return NS_OK;
}

nsresult
TimerThread::AddTimer(nsTimerImpl* aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  int32_t i = AddTimerInternal(aTimer);
  if (i < 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (mWaiting && i == 0) {
    mNotified = true;
    mMonitor.Notify();
  }

  return NS_OK;
}

nsresult
TimerThread::TimerDelayChanged(nsTimerImpl* aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  
  RemoveTimerInternal(aTimer);

  int32_t i = AddTimerInternal(aTimer);
  if (i < 0) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (mWaiting && i == 0) {
    mNotified = true;
    mMonitor.Notify();
  }

  return NS_OK;
}

nsresult
TimerThread::RemoveTimer(nsTimerImpl* aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  
  
  
  
  

  if (!RemoveTimerInternal(aTimer)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  if (mWaiting) {
    mNotified = true;
    mMonitor.Notify();
  }

  return NS_OK;
}


int32_t
TimerThread::AddTimerInternal(nsTimerImpl* aTimer)
{
  if (mShutdown) {
    return -1;
  }

  TimeStamp now = TimeStamp::Now();

  TimerAdditionComparator c(now, aTimer);
  nsTimerImpl** insertSlot = mTimers.InsertElementSorted(aTimer, c);

  if (!insertSlot) {
    return -1;
  }

  aTimer->mArmed = true;
  NS_ADDREF(aTimer);

#ifdef MOZ_TASK_TRACER
  aTimer->DispatchTracedTask();
#endif

  return insertSlot - mTimers.Elements();
}

bool
TimerThread::RemoveTimerInternal(nsTimerImpl* aTimer)
{
  if (!mTimers.RemoveElement(aTimer)) {
    return false;
  }

  ReleaseTimerInternal(aTimer);
  return true;
}

void
TimerThread::ReleaseTimerInternal(nsTimerImpl* aTimer)
{
  
  aTimer->mArmed = false;
  NS_RELEASE(aTimer);
}

void
TimerThread::DoBeforeSleep()
{
  mSleeping = true;
}

void
TimerThread::DoAfterSleep()
{
  mSleeping = true; 
  for (uint32_t i = 0; i < mTimers.Length(); i ++) {
    nsTimerImpl* timer = mTimers[i];
    
    uint32_t delay;
    timer->GetDelay(&delay);
    timer->SetDelay(delay);
  }

  mSleeping = false;
}



NS_IMETHODIMP
TimerThread::Observe(nsISupports* , const char* aTopic,
                     const char16_t* )
{
  if (strcmp(aTopic, "sleep_notification") == 0 ||
      strcmp(aTopic, "suspend_process_notification") == 0) {
    DoBeforeSleep();
  } else if (strcmp(aTopic, "wake_notification") == 0 ||
             strcmp(aTopic, "resume_process_notification") == 0) {
    DoAfterSleep();
  }

  return NS_OK;
}
