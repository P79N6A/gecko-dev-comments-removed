









































#include "nsIdleService.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "nsCOMArray.h"
#include "prinrval.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

using namespace mozilla;


#define OBSERVER_TOPIC_IDLE "idle"
#define OBSERVER_TOPIC_BACK "back"
#define OBSERVER_TOPIC_IDLE_DAILY "idle-daily"

#define MIN_IDLE_POLL_INTERVAL_MSEC (5 * PR_MSEC_PER_SEC) /* 5 sec */


#define DAILY_SIGNIFICANT_IDLE_SERVICE_SEC 300 /* 5 min */

#define PREF_LAST_DAILY "idle.lastDailyNotification"

#define SECONDS_PER_DAY 86400


class IdleListenerComparator
{
public:
  bool Equals(IdleListener a, IdleListener b) const
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

  
  (void)mIdleService->RemoveIdleObserver(this,
                                         DAILY_SIGNIFICANT_IDLE_SERVICE_SEC);

  
  PRInt32 nowSec = static_cast<PRInt32>(PR_Now() / PR_USEC_PER_SEC);
  Preferences::SetInt(PREF_LAST_DAILY, nowSec);

  
  (void)mTimer->InitWithFuncCallback(DailyCallback,
                                     this,
                                     SECONDS_PER_DAY * PR_MSEC_PER_SEC,
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
    
    (void)mTimer->InitWithFuncCallback(DailyCallback,
                                       this,
                                       SECONDS_PER_DAY * PR_MSEC_PER_SEC,
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

  
  
  (void)me->mIdleService->AddIdleObserver(me,
                                          DAILY_SIGNIFICANT_IDLE_SERVICE_SEC);
}











































































nsIdleService::nsIdleService() : mCurrentlySetToTimeoutAtInPR(0),
                                 mAnyObserverIdle(false),
                                 mDeltaToNextIdleSwitchInS(PR_UINT32_MAX),
                                 mLastUserInteractionInPR(0)
{
  mDailyIdle = new nsIdleServiceDaily(this);
  mDailyIdle->Init();
}

nsIdleService::~nsIdleService()
{
  if(mTimer) {
    mTimer->Cancel();
  }
}

NS_IMETHODIMP
nsIdleService::AddIdleObserver(nsIObserver* aObserver, PRUint32 aIdleTimeInS)
{
  NS_ENSURE_ARG_POINTER(aObserver);
  
  
  NS_ENSURE_ARG_RANGE(aIdleTimeInS, 1, (PR_UINT32_MAX / 10) - 1);

  
  IdleListener listener(aObserver, aIdleTimeInS);

  if (!mArrayListeners.AppendElement(listener)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (!mTimer) {
    nsresult rv;
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (mDeltaToNextIdleSwitchInS > aIdleTimeInS) {
    
    
    mDeltaToNextIdleSwitchInS = aIdleTimeInS;
  }

  
  ReconfigureTimer();

  return NS_OK;
}

NS_IMETHODIMP
nsIdleService::RemoveIdleObserver(nsIObserver* aObserver, PRUint32 aTimeInS)
{

  NS_ENSURE_ARG_POINTER(aObserver);
  NS_ENSURE_ARG(aTimeInS);
  IdleListener listener(aObserver, aTimeInS);

  
  
  
  IdleListenerComparator c;
  if (mArrayListeners.RemoveElement(listener, c)) {
    return NS_OK;
  }

  
  return NS_ERROR_FAILURE;
}

void
nsIdleService::ResetIdleTimeOut(PRUint32 idleDeltaInMS)
{
  
  mLastUserInteractionInPR = PR_Now() - (idleDeltaInMS * PR_USEC_PER_MSEC);

  
  if (!mAnyObserverIdle) {
    return;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_BACK_MS> timer;
  nsCOMArray<nsIObserver> notifyList;
  mDeltaToNextIdleSwitchInS = PR_UINT32_MAX;

  
  for (PRUint32 i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (curListener.isIdle) {
      notifyList.AppendObject(curListener.observer);
      curListener.isIdle = false;
    }

    
    mDeltaToNextIdleSwitchInS = PR_MIN(mDeltaToNextIdleSwitchInS,
                                       curListener.reqIdleTime);
  }

  
  mAnyObserverIdle = false;

  
  ReconfigureTimer();

  PRInt32 numberOfPendingNotifications = notifyList.Count();
  Telemetry::Accumulate(Telemetry::IDLE_NOTIFY_BACK_LISTENERS,
                        numberOfPendingNotifications);

  
  if (!numberOfPendingNotifications) {
    return;
  }

  
  

  
  nsAutoString timeStr;

  timeStr.AppendInt((PRInt32)(idleDeltaInMS / PR_MSEC_PER_SEC));

  
  while (numberOfPendingNotifications--) {
    notifyList[numberOfPendingNotifications]->Observe(this,
                                                      OBSERVER_TOPIC_BACK,
                                                      timeStr.get());
  }

}

NS_IMETHODIMP
nsIdleService::GetIdleTime(PRUint32* idleTime)
{
  
  if (!idleTime) {
    return NS_ERROR_NULL_POINTER;
  }

  
  PRUint32 polledIdleTimeMS;

  bool polledIdleTimeIsValid = PollIdleTime(&polledIdleTimeMS);

  
  if (!polledIdleTimeIsValid && 0 == mLastUserInteractionInPR) {
    *idleTime = 0;
    return NS_OK;
  }

  
  if (0 == mLastUserInteractionInPR) {
    *idleTime = polledIdleTimeMS;
    return NS_OK;
  }

  
  PRUint32 timeSinceResetInMS = (PR_Now() - mLastUserInteractionInPR) /
                                PR_USEC_PER_MSEC;

  
  if (!polledIdleTimeIsValid) {
    
    *idleTime = timeSinceResetInMS;
    return NS_OK;
  }

  
  *idleTime = NS_MIN(timeSinceResetInMS, polledIdleTimeMS);

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
nsIdleService::StaticIdleTimerCallback(nsITimer* aTimer, void* aClosure)
{
  static_cast<nsIdleService*>(aClosure)->IdleTimerCallback();
}

void
nsIdleService::IdleTimerCallback(void)
{
  
  mCurrentlySetToTimeoutAtInPR = 0;

  
  PRUint32 currentIdleTimeInMS;

  if (NS_FAILED(GetIdleTime(&currentIdleTimeInMS))) {
    return;
  }

  
  
  
  
  if (((PR_Now() - mLastUserInteractionInPR) / PR_USEC_PER_MSEC) >
      currentIdleTimeInMS)
  {
    
    
    ResetIdleTimeOut(currentIdleTimeInMS);

    
  }

  
  PRUint32 currentIdleTimeInS = currentIdleTimeInMS / PR_MSEC_PER_SEC;

  
  if (mDeltaToNextIdleSwitchInS > currentIdleTimeInS) {
    
    ReconfigureTimer();
    return;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_IDLE_MS> timer;

  
  mDeltaToNextIdleSwitchInS = PR_UINT32_MAX;

  
  nsCOMArray<nsIObserver> notifyList;

  for (PRUint32 i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (!curListener.isIdle) {
      
      if (curListener.reqIdleTime <= currentIdleTimeInS) {
        
        
        notifyList.AppendObject(curListener.observer);
        
        curListener.isIdle = true;
      } else {
        
        mDeltaToNextIdleSwitchInS = PR_MIN(mDeltaToNextIdleSwitchInS,
                                           curListener.reqIdleTime);
      }
    }
  }

  
  
  ReconfigureTimer();

  PRInt32 numberOfPendingNotifications = notifyList.Count();
  Telemetry::Accumulate(Telemetry::IDLE_NOTIFY_IDLE_LISTENERS,
                        numberOfPendingNotifications);

  
  if (!numberOfPendingNotifications) {
    return;
  }

  
  mAnyObserverIdle = true;

  
  nsAutoString timeStr;
  timeStr.AppendInt(currentIdleTimeInS);

  
  while (numberOfPendingNotifications--) {
    notifyList[numberOfPendingNotifications]->Observe(this,
                                                      OBSERVER_TOPIC_IDLE,
                                                      timeStr.get());
  }
}

void
nsIdleService::SetTimerExpiryIfBefore(PRTime aNextTimeoutInPR)
{
  
  if (!mTimer) {
    return;
  }

  
  
  if (mCurrentlySetToTimeoutAtInPR > aNextTimeoutInPR ||
      !mCurrentlySetToTimeoutAtInPR) {

    mCurrentlySetToTimeoutAtInPR = aNextTimeoutInPR ;

    
    mTimer->Cancel();

    
    PRTime currentTimeInPR = PR_Now();
    if (currentTimeInPR > mCurrentlySetToTimeoutAtInPR) {
      mCurrentlySetToTimeoutAtInPR = currentTimeInPR;
    }

    
    mCurrentlySetToTimeoutAtInPR += 10 * PR_USEC_PER_MSEC;

    
    mTimer->InitWithFuncCallback(StaticIdleTimerCallback,
                                 this,
                                 (mCurrentlySetToTimeoutAtInPR -
                                  currentTimeInPR) / PR_USEC_PER_MSEC,
                                 nsITimer::TYPE_ONE_SHOT);

  }
}


void
nsIdleService::ReconfigureTimer(void)
{
  
  if (!mAnyObserverIdle && PR_UINT32_MAX == mDeltaToNextIdleSwitchInS) {
    
    
    return;
  }

  

  
  
  PRTime curTimeInPR = PR_Now();

  PRTime nextTimeoutAtInPR = mLastUserInteractionInPR +
                             (((PRTime)mDeltaToNextIdleSwitchInS) *
                              PR_USEC_PER_SEC);

  
  if (mAnyObserverIdle && UsePollMode()) {
    PRTime pollTimeout = curTimeInPR +
                         MIN_IDLE_POLL_INTERVAL_MSEC * PR_USEC_PER_SEC;

    if (nextTimeoutAtInPR > pollTimeout) {
      nextTimeoutAtInPR = pollTimeout;
    }
  }

  SetTimerExpiryIfBefore(nextTimeoutAtInPR);
}

