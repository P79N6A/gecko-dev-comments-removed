




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
  mSleeping(false),
  mDelayLineCounter(0),
  mMinTimerPeriod(0)
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




void TimerThread::UpdateFilter(uint32_t aDelay, TimeStamp aTimeout,
                               TimeStamp aNow)
{
  TimeDuration slack = aTimeout - aNow;
  double smoothSlack = 0;
  uint32_t i, filterLength;
  static TimeDuration kFilterFeedbackMaxTicks =
    TimeDuration::FromMilliseconds(FILTER_FEEDBACK_MAX);
  static TimeDuration kFilterFeedbackMinTicks =
    TimeDuration::FromMilliseconds(-FILTER_FEEDBACK_MAX);

  if (slack > kFilterFeedbackMaxTicks)
    slack = kFilterFeedbackMaxTicks;
  else if (slack < kFilterFeedbackMinTicks)
    slack = kFilterFeedbackMinTicks;

  mDelayLine[mDelayLineCounter & DELAY_LINE_LENGTH_MASK] =
    slack.ToMilliseconds();
  if (++mDelayLineCounter < DELAY_LINE_LENGTH) {
    
    PR_ASSERT(mTimeoutAdjustment.ToSeconds() == 0);
    filterLength = 0;
  } else {
    
    if (mMinTimerPeriod == 0) {
      mMinTimerPeriod = (aDelay != 0) ? aDelay : 1;
    } else if (aDelay != 0 && aDelay < mMinTimerPeriod) {
      mMinTimerPeriod = aDelay;
    }

    filterLength = (uint32_t) (FILTER_DURATION / mMinTimerPeriod);
    if (filterLength > DELAY_LINE_LENGTH)
      filterLength = DELAY_LINE_LENGTH;
    else if (filterLength < 4)
      filterLength = 4;

    for (i = 1; i <= filterLength; i++)
      smoothSlack += mDelayLine[(mDelayLineCounter-i) & DELAY_LINE_LENGTH_MASK];
    smoothSlack /= filterLength;

    
    mTimeoutAdjustment = TimeDuration::FromMilliseconds(smoothSlack * 1.5);
  }

#ifdef DEBUG_TIMERS
  PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
         ("UpdateFilter: smoothSlack = %g, filterLength = %u\n",
          smoothSlack, filterLength));
#endif
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

        if (now >= timer->mTimeout + mTimeoutAdjustment) {
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

        TimeStamp timeout = timer->mTimeout + mTimeoutAdjustment;

        
        
        
        
        
        
        
        double microseconds = (timeout - now).ToMilliseconds()*1000;
        if (microseconds < halfMicrosecondsIntervalResolution)
          goto next; 
        waitFor = PR_MicrosecondsToInterval(microseconds);
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

  TimerAdditionComparator c(now, mTimeoutAdjustment, aTimer);
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

  
  mTimeoutAdjustment = TimeDuration(0);
  mDelayLineCounter = 0;
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
