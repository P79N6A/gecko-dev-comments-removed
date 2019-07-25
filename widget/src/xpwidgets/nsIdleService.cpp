









































#include "nsIdleService.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "nsCOMArray.h"
#include "prinrval.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"

using namespace mozilla;


#define OBSERVER_TOPIC_IDLE "idle"
#define OBSERVER_TOPIC_BACK "back"
#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"

#define MIN_IDLE_POLL_INTERVAL 5
#define MAX_IDLE_POLL_INTERVAL 300 /* 5 min */

#define PREF_LAST_DAILY "idle.lastDailyNotification"

#define SECONDS_PER_DAY 86400


class IdleListenerComparator
{
public:
  PRBool Equals(IdleListener a, IdleListener b) const
  {
    return (a.observer == b.observer) &&
           (a.reqIdleTime == b.reqIdleTime);
  }
};




NS_IMPL_ISUPPORTS1(nsIdleServiceDaily, nsIObserver)

NS_IMETHODIMP
nsIdleServiceDaily::Observe(nsISupports *,
                            const char *,
                            const PRUnichar *)
{
  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  NS_ENSURE_STATE(observerService);
  (void)observerService->NotifyObservers(nsnull,
                                         OBSERVER_TOPIC_IDLE_DAILY,
                                         nsnull);

  
  const nsCOMArray<nsIObserver> &entries = mCategoryObservers.GetEntries();
  for (PRInt32 i = 0; i < entries.Count(); ++i) {
    (void)entries[i]->Observe(nsnull, OBSERVER_TOPIC_IDLE_DAILY, nsnull);
  }

  
  (void)mIdleService->RemoveIdleObserver(this, MAX_IDLE_POLL_INTERVAL);

  
  PRInt32 nowSec = static_cast<PRInt32>(PR_Now() / PR_USEC_PER_SEC);
  Preferences::SetInt(PREF_LAST_DAILY, nowSec);

  
  (void)mTimer->InitWithFuncCallback(DailyCallback, this, SECONDS_PER_DAY * 1000,
                                     nsITimer::TYPE_ONE_SHOT);

  return NS_OK;
}

nsIdleServiceDaily::nsIdleServiceDaily(nsIIdleService* aIdleService)
  : mIdleService(aIdleService)
  , mTimer(do_CreateInstance(NS_TIMER_CONTRACTID))
  , mCategoryObservers(OBSERVER_TOPIC_IDLE_DAILY)
{
}

void
nsIdleServiceDaily::Init()
{
  
  
  PRInt32 nowSec = static_cast<PRInt32>(PR_Now() / PR_USEC_PER_SEC);
  PRInt32 lastDaily = Preferences::GetInt(PREF_LAST_DAILY, 0);
  if (lastDaily < 0 || lastDaily > nowSec) {
    
    lastDaily = 0;
  }

  
  if (nowSec - lastDaily > SECONDS_PER_DAY) {
    
    DailyCallback(nsnull, this);
  }
  else {
    
    (void)mTimer->InitWithFuncCallback(DailyCallback, this, SECONDS_PER_DAY * 1000,
                                       nsITimer::TYPE_ONE_SHOT);
  }
}

nsIdleServiceDaily::~nsIdleServiceDaily()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
}


void
nsIdleServiceDaily::DailyCallback(nsITimer* aTimer, void* aClosure)
{
  nsIdleServiceDaily* me = static_cast<nsIdleServiceDaily*>(aClosure);

  
  
  (void)me->mIdleService->AddIdleObserver(me, MAX_IDLE_POLL_INTERVAL);
}




nsIdleService::nsIdleService() : mLastIdleReset(0)
                               , mLastHandledActivity(0)
                               , mPolledIdleTimeIsValid(false)
{
  mDailyIdle = new nsIdleServiceDaily(this);
  mDailyIdle->Init();
}

nsIdleService::~nsIdleService()
{
  StopTimer();
}

NS_IMETHODIMP
nsIdleService::AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTime)
{
  NS_ENSURE_ARG_POINTER(aObserver);
  NS_ENSURE_ARG(aIdleTime);

  
  IdleListener listener(aObserver, aIdleTime);

  if (!mArrayListeners.AppendElement(listener)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (!mTimer) {
    nsresult rv;
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  CheckAwayState(false);

  return NS_OK;
}

NS_IMETHODIMP
nsIdleService::RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aTime)
{
  NS_ENSURE_ARG_POINTER(aObserver);
  NS_ENSURE_ARG(aTime);
  IdleListener listener(aObserver, aTime);

  
  IdleListenerComparator c;
  if (mArrayListeners.RemoveElement(listener, c)) {
    if (mArrayListeners.IsEmpty()) {
      StopTimer();
    }
    return NS_OK;
  }

  
  return NS_ERROR_FAILURE;
}

void
nsIdleService::ResetIdleTimeOut()
{
  
  
  bool calledBefore = mLastIdleReset != 0;
  mLastIdleReset = PR_IntervalToSeconds(PR_IntervalNow());
  if (!mLastIdleReset) mLastIdleReset = 1;

  
  
  
  
  
  CheckAwayState(calledBefore);
}

NS_IMETHODIMP
nsIdleService::GetIdleTime(PRUint32* idleTime)
{
  
  if (!idleTime) {
    return NS_ERROR_NULL_POINTER;
  }

  
  PRUint32 polledIdleTimeMS;

  mPolledIdleTimeIsValid = PollIdleTime(&polledIdleTimeMS);

  
  if (!mPolledIdleTimeIsValid && 0 == mLastIdleReset) {
    *idleTime = 0;
    return NS_OK;
  }

  
  if (0 == mLastIdleReset) {
    *idleTime = polledIdleTimeMS;
    return NS_OK;
  }

  
  PRUint32 timeSinceReset =
    PR_IntervalToSeconds(PR_IntervalNow()) - mLastIdleReset;

  
  if (!mPolledIdleTimeIsValid) {
    
    *idleTime = timeSinceReset * 1000;
    return NS_OK;
  }

  
  *idleTime = NS_MIN(timeSinceReset * 1000, polledIdleTimeMS);

  return NS_OK;
}


bool
nsIdleService::PollIdleTime(PRUint32* )
{
  
  return false;
}

bool
nsIdleService::UsePollMode()
{
  PRUint32 dummy;
  return PollIdleTime(&dummy);
}

void
nsIdleService::IdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
  static_cast<nsIdleService*>(aClosure)->CheckAwayState(false);
}

void
nsIdleService::CheckAwayState(bool aNoTimeReset)
{
  




  PRUint32 curTime = static_cast<PRUint32>(PR_Now() / PR_USEC_PER_SEC);
  PRUint32 lastTime = curTime - mLastHandledActivity;

  
  PRUint32 idleTime;
  if (NS_FAILED(GetIdleTime(&idleTime))) {
    return;
  }

  
  if (!mPolledIdleTimeIsValid && 0 == mLastIdleReset) {
    return;
  }

  
  idleTime /= 1000;

  
  nsAutoString timeStr;
  timeStr.AppendInt(idleTime);

  
  mLastHandledActivity = curTime - idleTime;

  



  nsCOMArray<nsIObserver> notifyList;

  if (lastTime > idleTime) {
    
    for (PRUint32 i = 0; i < mArrayListeners.Length(); i++) {
      IdleListener& curListener = mArrayListeners.ElementAt(i);

      if (curListener.isIdle) {
        notifyList.AppendObject(curListener.observer);
        curListener.isIdle = false;
      }
    }

    
    for (PRInt32 i = 0; i < notifyList.Count(); i++) {
      notifyList[i]->Observe(this, OBSERVER_TOPIC_BACK, timeStr.get());
    }
  }

  





  
  notifyList.Clear();

  
  if (aNoTimeReset) {
    return;
  }
  




  PRUint32 nextWaitTime = PR_UINT32_MAX;

  




  bool anyOneIdle = false;

  for (PRUint32 i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (!curListener.isIdle) {
      
      if (curListener.reqIdleTime <= idleTime) {
        
        
        notifyList.AppendObject(curListener.observer);
        
        curListener.isIdle = true;
      } else {
        
        
        nextWaitTime = NS_MIN(nextWaitTime, curListener.reqIdleTime);
      }
    }

    
    
    anyOneIdle |= curListener.isIdle;
  }

  
  
  nextWaitTime -= idleTime;

  
  for (PRInt32 i = 0; i < notifyList.Count(); i++) {
    notifyList[i]->Observe(this, OBSERVER_TOPIC_IDLE, timeStr.get());
  }

  
  
  if (UsePollMode() &&
      anyOneIdle &&
      nextWaitTime > MIN_IDLE_POLL_INTERVAL) {
    nextWaitTime = MIN_IDLE_POLL_INTERVAL;
  }

  
  if (PR_UINT32_MAX != nextWaitTime) {
    StartTimer(nextWaitTime);
  }
}

void
nsIdleService::StartTimer(PRUint32 aDelay)
{
  if (mTimer) {
    StopTimer();
    if (aDelay) {
      mTimer->InitWithFuncCallback(IdleTimerCallback, this, aDelay*1000,
                                   nsITimer::TYPE_ONE_SHOT);
    }
  }
}

void
nsIdleService::StopTimer()
{
  if (mTimer) {
    mTimer->Cancel();
  }
}

