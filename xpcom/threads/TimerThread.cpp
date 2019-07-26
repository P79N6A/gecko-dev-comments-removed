




#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsThreadUtils.h"
#include "pratom.h"

#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "mozilla/Services.h"

#include <math.h>

using namespace mozilla;

NS_IMPL_THREADSAFE_ISUPPORTS2(TimerThread, nsIRunnable, nsIObserver)

TimerThread::TimerThread() :
  mInitInProgress(0),
  mInitialized(false),
  mMonitor("TimerThread.mMonitor"),
  mShutdown(false),
  mWaiting(false),
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
  TimerObserverRunnable(nsIObserver* observer)
    : mObserver(observer)
  { }

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

nsresult TimerThread::Init()
{
  PR_LOG(GetTimerLog(), PR_LOG_DEBUG, ("TimerThread::Init [%d]\n", mInitialized));

  if (mInitialized) {
    if (!mThread)
      return NS_ERROR_FAILURE;

    return NS_OK;
  }

  if (PR_ATOMIC_SET(&mInitInProgress, 1) == 0) {
    
    nsresult rv = NS_NewThread(getter_AddRefs(mThread), this);
    if (NS_FAILED(rv)) {
      mThread = nullptr;
    }
    else {
      nsRefPtr<TimerObserverRunnable> r = new TimerObserverRunnable(this);
      if (NS_IsMainThread()) {
        r->Run();
      }
      else {
        NS_DispatchToMainThread(r);
      }
    }

    {
      MonitorAutoLock lock(mMonitor);
      mInitialized = true;
      mMonitor.NotifyAll();
    }
  }
  else {
    MonitorAutoLock lock(mMonitor);
    while (!mInitialized) {
      mMonitor.Wait();
    }
  }

  if (!mThread)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult TimerThread::Shutdown()
{
  PR_LOG(GetTimerLog(), PR_LOG_DEBUG, ("TimerThread::Shutdown begin\n"));

  if (!mThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsTArray<nsTimerImpl*> timers;
  {   
    MonitorAutoLock lock(mMonitor);

    mShutdown = true;

    
    if (mWaiting)
      mMonitor.Notify();

    
    
    
    
    
    
    timers.AppendElements(mTimers);
    mTimers.Clear();
  }

  uint32_t timersCount = timers.Length();
  for (uint32_t i = 0; i < timersCount; i++) {
    nsTimerImpl *timer = timers[i];
    timer->ReleaseCallback();
    ReleaseTimerInternal(timer);
  }

  mThread->Shutdown();    

  PR_LOG(GetTimerLog(), PR_LOG_DEBUG, ("TimerThread::Shutdown end\n"));
  return NS_OK;
}


NS_IMETHODIMP TimerThread::Run()
{
  PR_SetCurrentThreadName("Timer");

  MonitorAutoLock lock(mMonitor);

  
  
  
  int32_t low = 0, high = 1;
  while (PR_MicrosecondsToInterval(high) == 0)
    high <<= 1;
  
  
  
  
  while (high-low > 1) {
    int32_t mid = (high+low) >> 1;
    if (PR_MicrosecondsToInterval(mid) == 0)
      low = mid;
    else
      high = mid;
  }

  
  
  int32_t halfMicrosecondsIntervalResolution = high >> 1;

  while (!mShutdown) {
    
    PRIntervalTime waitFor;

    if (mSleeping) {
      
      waitFor = PR_MillisecondsToInterval(100);
    } else {
      waitFor = PR_INTERVAL_NO_TIMEOUT;
      TimeStamp now = TimeStamp::Now();
      nsTimerImpl *timer = nullptr;

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        if (now >= timer->mTimeout) {
    next:
          
          
          
          
          

          NS_ADDREF(timer);
          RemoveTimerInternal(timer);

          {
            
            MonitorAutoUnlock unlock(mMonitor);

#ifdef DEBUG_TIMERS
            if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
              PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
                     ("Timer thread woke up %fms from when it was supposed to\n",
                      fabs((now - timer->mTimeout).ToMilliseconds())));
            }
#endif

            
            
            
            if (NS_FAILED(timer->PostTimerEvent())) {
              nsrefcnt rc;
              NS_RELEASE2(timer, rc);
            
              
              
              
              
              
              
              
              
              
              
              
              MOZ_ASSERT(rc != 0, "destroyed timer off its target thread!");
            }
            timer = nullptr;
          }

          if (mShutdown)
            break;

          
          
          now = TimeStamp::Now();
        }
      }

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        TimeStamp timeout = timer->mTimeout;

        
        
        
        
        
        
        
        double microseconds = (timeout - now).ToMilliseconds()*1000;
        if (microseconds < halfMicrosecondsIntervalResolution)
          goto next; 
        waitFor = PR_MicrosecondsToInterval(static_cast<PRUint32>(microseconds)); 
        if (waitFor == 0)
          waitFor = 1; 
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
    mMonitor.Wait(waitFor);
    mWaiting = false;
  }

  return NS_OK;
}

nsresult TimerThread::AddTimer(nsTimerImpl *aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  int32_t i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (mWaiting && i == 0)
    mMonitor.Notify();

  return NS_OK;
}

nsresult TimerThread::TimerDelayChanged(nsTimerImpl *aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  
  RemoveTimerInternal(aTimer);

  int32_t i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (mWaiting && i == 0)
    mMonitor.Notify();

  return NS_OK;
}

nsresult TimerThread::RemoveTimer(nsTimerImpl *aTimer)
{
  MonitorAutoLock lock(mMonitor);

  
  
  
  
  
  

  if (!RemoveTimerInternal(aTimer))
    return NS_ERROR_NOT_AVAILABLE;

  
  if (mWaiting)
    mMonitor.Notify();

  return NS_OK;
}


int32_t TimerThread::AddTimerInternal(nsTimerImpl *aTimer)
{
  if (mShutdown)
    return -1;

  TimeStamp now = TimeStamp::Now();

  TimerAdditionComparator c(now, aTimer);
  nsTimerImpl** insertSlot = mTimers.InsertElementSorted(aTimer, c);

  if (!insertSlot)
    return -1;

  aTimer->mArmed = true;
  NS_ADDREF(aTimer);
  return insertSlot - mTimers.Elements();
}

bool TimerThread::RemoveTimerInternal(nsTimerImpl *aTimer)
{
  if (!mTimers.RemoveElement(aTimer))
    return false;

  ReleaseTimerInternal(aTimer);
  return true;
}

void TimerThread::ReleaseTimerInternal(nsTimerImpl *aTimer)
{
  
  aTimer->mArmed = false;
  NS_RELEASE(aTimer);
}

void TimerThread::DoBeforeSleep()
{
  mSleeping = true;
}

void TimerThread::DoAfterSleep()
{
  mSleeping = true; 
  for (uint32_t i = 0; i < mTimers.Length(); i ++) {
    nsTimerImpl *timer = mTimers[i];
    
    uint32_t delay;
    timer->GetDelay(&delay);
    timer->SetDelay(delay);
  }

  mSleeping = false;
}



NS_IMETHODIMP
TimerThread::Observe(nsISupports* , const char *aTopic, const PRUnichar* )
{
  if (strcmp(aTopic, "sleep_notification") == 0 ||
      strcmp(aTopic, "suspend_process_notification") == 0)
    DoBeforeSleep();
  else if (strcmp(aTopic, "wake_notification") == 0 ||
           strcmp(aTopic, "resume_process_notification") == 0)
    DoAfterSleep();

  return NS_OK;
}
