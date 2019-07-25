







































#include "nsTimerImpl.h"
#include "TimerThread.h"

#include "nsAutoLock.h"
#include "nsThreadUtils.h"
#include "pratom.h"

#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsIProxyObjectManager.h"
#include "mozilla/Services.h"

#include <math.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(TimerThread, nsIRunnable, nsIObserver)

TimerThread::TimerThread() :
  mInitInProgress(0),
  mInitialized(PR_FALSE),
  mLock(nsnull),
  mCondVar(nsnull),
  mShutdown(PR_FALSE),
  mWaiting(PR_FALSE),
  mSleeping(PR_FALSE)
{
}

TimerThread::~TimerThread()
{
  if (mCondVar)
    PR_DestroyCondVar(mCondVar);
  if (mLock)
    PR_DestroyLock(mLock);

  mThread = nsnull;

  NS_ASSERTION(mTimers.IsEmpty(), "Timers remain in TimerThread::~TimerThread");
}

nsresult
TimerThread::InitLocks()
{
  NS_ASSERTION(!mLock, "InitLocks called twice?");
  mLock = PR_NewLock();
  if (!mLock)
    return NS_ERROR_OUT_OF_MEMORY;

  mCondVar = PR_NewCondVar(mLock);
  if (!mCondVar)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

nsresult TimerThread::Init()
{
  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Init [%d]\n", mInitialized));

  if (mInitialized) {
    if (!mThread)
      return NS_ERROR_FAILURE;

    return NS_OK;
  }

  if (PR_AtomicSet(&mInitInProgress, 1) == 0) {
    
    nsresult rv = NS_NewThread(getter_AddRefs(mThread), this);
    if (NS_FAILED(rv)) {
      mThread = nsnull;
    }
    else {
      nsCOMPtr<nsIObserverService> observerService =
          mozilla::services::GetObserverService();
      
      if (observerService && !NS_IsMainThread()) {
        nsCOMPtr<nsIObserverService> result = nsnull;
        NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                             NS_GET_IID(nsIObserverService),
                             observerService, NS_PROXY_ASYNC,
                             getter_AddRefs(result));
        observerService.swap(result);
      }
      
      if (observerService) {
        observerService->AddObserver(this, "sleep_notification", PR_FALSE);
        observerService->AddObserver(this, "wake_notification", PR_FALSE);
      }
    }

    PR_Lock(mLock);
    mInitialized = PR_TRUE;
    PR_NotifyAllCondVar(mCondVar);
    PR_Unlock(mLock);
  }
  else {
    PR_Lock(mLock);
    while (!mInitialized) {
      PR_WaitCondVar(mCondVar, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_Unlock(mLock);
  }

  if (!mThread)
    return NS_ERROR_FAILURE;

  return NS_OK;
}

nsresult TimerThread::Shutdown()
{
  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Shutdown begin\n"));

  if (!mThread)
    return NS_ERROR_NOT_INITIALIZED;

  nsTArray<nsTimerImpl*> timers;
  {   
    nsAutoLock lock(mLock);

    mShutdown = PR_TRUE;

    
    if (mCondVar && mWaiting)
      PR_NotifyCondVar(mCondVar);

    
    
    
    
    
    
    timers.AppendElements(mTimers);
    mTimers.Clear();
  }

  PRUint32 timersCount = timers.Length();
  for (PRUint32 i = 0; i < timersCount; i++) {
    nsTimerImpl *timer = timers[i];
    timer->ReleaseCallback();
    ReleaseTimerInternal(timer);
  }

  mThread->Shutdown();    

  PR_LOG(gTimerLog, PR_LOG_DEBUG, ("TimerThread::Shutdown end\n"));
  return NS_OK;
}


NS_IMETHODIMP TimerThread::Run()
{
  nsAutoLock lock(mLock);

  while (!mShutdown) {
    
    PRIntervalTime waitFor;

    if (mSleeping) {
      
      waitFor = PR_MillisecondsToInterval(100);
    } else {
      waitFor = PR_INTERVAL_NO_TIMEOUT;
      TimeStamp now = TimeStamp::Now();
      nsTimerImpl *timer = nsnull;

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        if (now >= timer->mTimeout) {
    next:
          
          
          
          
          

          NS_ADDREF(timer);
          RemoveTimerInternal(timer);

          
          lock.unlock();

#ifdef DEBUG_TIMERS
          if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
            PR_LOG(gTimerLog, PR_LOG_DEBUG,
                   ("Timer thread woke up %fms from when it was supposed to\n",
                    fabs((now - timer->mTimeout).ToMilliseconds())));
          }
#endif

          
          
          
          if (NS_FAILED(timer->PostTimerEvent())) {
            nsrefcnt rc;
            NS_RELEASE2(timer, rc);
            
            
            
            
            
            
            
            
            
            
            
            
            NS_ASSERTION(rc != 0, "destroyed timer off its target thread!");
          }
          timer = nsnull;

          lock.lock();
          if (mShutdown)
            break;

          
          
          now = TimeStamp::Now();
        }
      }

      if (!mTimers.IsEmpty()) {
        timer = mTimers[0];

        TimeStamp timeout = timer->mTimeout;

        
        
        if (now >= timeout)
          goto next;
        waitFor = PR_MillisecondsToInterval((timeout - now).ToMilliseconds());
      }

#ifdef DEBUG_TIMERS
      if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
        if (waitFor == PR_INTERVAL_NO_TIMEOUT)
          PR_LOG(gTimerLog, PR_LOG_DEBUG,
                 ("waiting for PR_INTERVAL_NO_TIMEOUT\n"));
        else
          PR_LOG(gTimerLog, PR_LOG_DEBUG,
                 ("waiting for %u\n", PR_IntervalToMilliseconds(waitFor)));
      }
#endif
    }

    mWaiting = PR_TRUE;
    PR_WaitCondVar(mCondVar, waitFor);
    mWaiting = PR_FALSE;
  }

  return NS_OK;
}

nsresult TimerThread::AddTimer(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  
  PRInt32 i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (mCondVar && mWaiting && i == 0)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}

nsresult TimerThread::TimerDelayChanged(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  
  
  RemoveTimerInternal(aTimer);

  PRInt32 i = AddTimerInternal(aTimer);
  if (i < 0)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (mCondVar && mWaiting && i == 0)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}

nsresult TimerThread::RemoveTimer(nsTimerImpl *aTimer)
{
  nsAutoLock lock(mLock);

  
  
  
  
  
  

  if (!RemoveTimerInternal(aTimer))
    return NS_ERROR_NOT_AVAILABLE;

  
  if (mCondVar && mWaiting)
    PR_NotifyCondVar(mCondVar);

  return NS_OK;
}


PRInt32 TimerThread::AddTimerInternal(nsTimerImpl *aTimer)
{
  if (mShutdown)
    return -1;

  TimeStamp now = TimeStamp::Now();
  PRUint32 count = mTimers.Length();
  PRUint32 i = 0;
  for (; i < count; i++) {
    nsTimerImpl *timer = mTimers[i];

    
    if (now < timer->mTimeout && aTimer->mTimeout < timer->mTimeout) {
      break;
    }
  }

  if (!mTimers.InsertElementAt(i, aTimer))
    return -1;

  aTimer->mArmed = PR_TRUE;
  NS_ADDREF(aTimer);
  return i;
}

PRBool TimerThread::RemoveTimerInternal(nsTimerImpl *aTimer)
{
  if (!mTimers.RemoveElement(aTimer))
    return PR_FALSE;

  ReleaseTimerInternal(aTimer);
  return PR_TRUE;
}

void TimerThread::ReleaseTimerInternal(nsTimerImpl *aTimer)
{
  
  aTimer->mArmed = PR_FALSE;
  NS_RELEASE(aTimer);
}

void TimerThread::DoBeforeSleep()
{
  mSleeping = PR_TRUE;
}

void TimerThread::DoAfterSleep()
{
  mSleeping = PR_TRUE; 
  for (PRUint32 i = 0; i < mTimers.Length(); i ++) {
    nsTimerImpl *timer = mTimers[i];
    
    PRUint32 delay;
    timer->GetDelay(&delay);
    timer->SetDelay(delay);
  }

  mSleeping = PR_FALSE;
}



NS_IMETHODIMP
TimerThread::Observe(nsISupports* , const char *aTopic, const PRUnichar* )
{
  if (strcmp(aTopic, "sleep_notification") == 0)
    DoBeforeSleep();
  else if (strcmp(aTopic, "wake_notification") == 0)
    DoAfterSleep();

  return NS_OK;
}
