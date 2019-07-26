






#include "nsIdleService.h"
#include "nsString.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsDebug.h"
#include "nsCOMArray.h"
#include "prinrval.h"
#include "prlog.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#ifdef ANDROID
#include <android/log.h>
#endif

using namespace mozilla;


#define MIN_IDLE_POLL_INTERVAL_MSEC (5 * PR_MSEC_PER_SEC) /* 5 sec */


#define DAILY_SIGNIFICANT_IDLE_SERVICE_SEC 300 /* 5 min */

#define PREF_LAST_DAILY "idle.lastDailyNotification"

#define SECONDS_PER_DAY 86400

#ifdef PR_LOGGING
static PRLogModuleInfo *sLog = NULL;
#endif


class IdleListenerComparator
{
public:
  bool Equals(IdleListener a, IdleListener b) const
  {
    return (a.observer == b.observer) &&
           (a.reqIdleTime == b.reqIdleTime);
  }
};




NS_IMPL_ISUPPORTS2(nsIdleServiceDaily, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
nsIdleServiceDaily::Observe(nsISupports *,
                            const char *aTopic,
                            const PRUnichar *)
{
  if (strcmp(aTopic, "profile-after-change") == 0) {
    
    mShutdownInProgress = false;
    return NS_OK;
  }

  if (strcmp(aTopic, "xpcom-will-shutdown") == 0 ||
      strcmp(aTopic, "profile-change-teardown") == 0) {
    mShutdownInProgress = true;
  }

  if (mShutdownInProgress || strcmp(aTopic, OBSERVER_TOPIC_ACTIVE) == 0) {
    return NS_OK;
  }
  MOZ_ASSERT(strcmp(aTopic, OBSERVER_TOPIC_IDLE) == 0);

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService", "Notifying idle-daily observers");
#endif

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  NS_ENSURE_STATE(observerService);
  (void)observerService->NotifyObservers(nullptr,
                                         OBSERVER_TOPIC_IDLE_DAILY,
                                         nullptr);

  
  const nsCOMArray<nsIObserver> &entries = mCategoryObservers.GetEntries();
  for (int32_t i = 0; i < entries.Count(); ++i) {
    (void)entries[i]->Observe(nullptr, OBSERVER_TOPIC_IDLE_DAILY, nullptr);
  }

  
  (void)mIdleService->RemoveIdleObserver(this,
                                         DAILY_SIGNIFICANT_IDLE_SERVICE_SEC);

  
  int32_t nowSec = static_cast<int32_t>(PR_Now() / PR_USEC_PER_SEC);
  Preferences::SetInt(PREF_LAST_DAILY, nowSec);

  
  
  nsIPrefService* prefs = Preferences::GetService();
  if (prefs) {
    prefs->SavePrefFile(nullptr);
  }

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService", "Storing last idle time as %d",
                      nowSec);
#endif

  
  mDailyTimerStart  = PR_Now();

  
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
  , mShutdownInProgress(false)
{
}

void
nsIdleServiceDaily::Init()
{
  
  
  int32_t nowSec = static_cast<int32_t>(PR_Now() / PR_USEC_PER_SEC);
  int32_t lastDaily = Preferences::GetInt(PREF_LAST_DAILY, 0);
  if (lastDaily < 0 || lastDaily > nowSec) {
    
    lastDaily = 0;
  }

  
  if (nowSec - lastDaily > SECONDS_PER_DAY) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService", "DailyCallback started");
#endif
    
    DailyCallback(nullptr, this);
  }
  else {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService", "Setting timer a day from now");
#endif

    
    mDailyTimerStart  = PR_Now();

    
    (void)mTimer->InitWithFuncCallback(DailyCallback,
                                       this,
                                       SECONDS_PER_DAY * PR_MSEC_PER_SEC,
                                       nsITimer::TYPE_ONE_SHOT);
  }

  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    obs->AddObserver(this, "xpcom-will-shutdown", true);
    obs->AddObserver(this, "profile-change-teardown", true);
    obs->AddObserver(this, "profile-after-change", true);
  }
}

nsIdleServiceDaily::~nsIdleServiceDaily()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
}


void
nsIdleServiceDaily::DailyCallback(nsITimer* aTimer, void* aClosure)
{
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService", "DailyCallback running");
#endif

  nsIdleServiceDaily* me = static_cast<nsIdleServiceDaily*>(aClosure);

  PRTime now = PR_Now();
  PRTime launchTime = me->mDailyTimerStart + ((PRTime)SECONDS_PER_DAY * PR_USEC_PER_SEC);

  
  if (now < launchTime) {
      
      PRTime newTime = launchTime;

      
      newTime += 10 * PR_USEC_PER_MSEC;

#ifdef ANDROID
      __android_log_print(ANDROID_LOG_INFO, "IdleService",
                          "DailyCallback resetting timer to %lld msec",
                          (newTime - now) / PR_USEC_PER_MSEC);
#endif

      
      (void)me->mTimer->InitWithFuncCallback(DailyCallback,
                                             me,
                                             (newTime - now) / PR_USEC_PER_MSEC,
                                             nsITimer::TYPE_ONE_SHOT);
      return;
  }

#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService", "DailyCallback registering Idle observer");
#endif

  
  
  (void)me->mIdleService->AddIdleObserver(me,
                                          DAILY_SIGNIFICANT_IDLE_SERVICE_SEC);
}











































































namespace { 
nsIdleService* gIdleService;
}

already_AddRefed<nsIdleService>
nsIdleService::GetInstance()
{
  nsRefPtr<nsIdleService> instance(gIdleService);
  return instance.forget();
}

nsIdleService::nsIdleService() : mCurrentlySetToTimeoutAtInPR(0),
                                 mAnyObserverIdle(false),
                                 mDeltaToNextIdleSwitchInS(PR_UINT32_MAX),
                                 mLastUserInteractionInPR(PR_Now())
{
#ifdef PR_LOGGING
  if (sLog == NULL)
    sLog = PR_NewLogModule("idleService");
#endif
  MOZ_ASSERT(!gIdleService);
  gIdleService = this;
  mDailyIdle = new nsIdleServiceDaily(this);
  mDailyIdle->Init();
}

nsIdleService::~nsIdleService()
{
  if(mTimer) {
    mTimer->Cancel();
  }


  MOZ_ASSERT(gIdleService == this);
  gIdleService = nullptr;
}

NS_IMPL_ISUPPORTS2(nsIdleService, nsIIdleService, nsIIdleServiceInternal)

NS_IMETHODIMP
nsIdleService::AddIdleObserver(nsIObserver* aObserver, uint32_t aIdleTimeInS)
{
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Register idle observer %x for %d seconds",
          aObserver, aIdleTimeInS));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService", "Register idle observer %x for %d seconds",
                      aObserver, aIdleTimeInS);
#endif

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
    
    
    PR_LOG(sLog, PR_LOG_DEBUG,
          ("idleService: Register: adjusting next switch from %d to %d seconds",
           mDeltaToNextIdleSwitchInS, aIdleTimeInS));
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Register: adjusting next switch from %d to %d seconds",
                        mDeltaToNextIdleSwitchInS, aIdleTimeInS);
#endif

    mDeltaToNextIdleSwitchInS = aIdleTimeInS;
  }

  
  ReconfigureTimer();

  return NS_OK;
}

NS_IMETHODIMP
nsIdleService::RemoveIdleObserver(nsIObserver* aObserver, uint32_t aTimeInS)
{

  NS_ENSURE_ARG_POINTER(aObserver);
  NS_ENSURE_ARG(aTimeInS);
  IdleListener listener(aObserver, aTimeInS);

  
  
  
  IdleListenerComparator c;
  if (mArrayListeners.RemoveElement(listener, c)) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Remove idle observer %x (%d seconds)",
            aObserver, aTimeInS));
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Remove idle observer %x (%d seconds)",
                        aObserver, aTimeInS);
#endif
    return NS_OK;
  }

  
  PR_LOG(sLog, PR_LOG_WARNING, 
         ("idleService: Failed to remove idle observer %x (%d seconds)",
          aObserver, aTimeInS));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Failed to remove idle observer %x (%d seconds)",
                      aObserver, aTimeInS);
#endif
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsIdleService::ResetIdleTimeOut(uint32_t idleDeltaInMS)
{
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Reset idle timeout (last interaction %u msec)",
          idleDeltaInMS));

  
  mLastUserInteractionInPR = PR_Now() - (idleDeltaInMS * PR_USEC_PER_MSEC);

  
  if (!mAnyObserverIdle) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Reset idle timeout: no idle observers"));
    return NS_OK;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_BACK_MS> timer;
  nsCOMArray<nsIObserver> notifyList;
  mDeltaToNextIdleSwitchInS = PR_UINT32_MAX;

  
  for (uint32_t i = 0; i < mArrayListeners.Length(); i++) {
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

  int32_t numberOfPendingNotifications = notifyList.Count();
  Telemetry::Accumulate(Telemetry::IDLE_NOTIFY_BACK_LISTENERS,
                        numberOfPendingNotifications);

  
  if (!numberOfPendingNotifications) {
    return NS_OK;
  }

  
  

  
  nsAutoString timeStr;

  timeStr.AppendInt((int32_t)(idleDeltaInMS / PR_MSEC_PER_SEC));

  
  while (numberOfPendingNotifications--) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Reset idle timeout: tell observer %x user is back",
            notifyList[numberOfPendingNotifications]));
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Reset idle timeout: tell observer %x user is back",
                        notifyList[numberOfPendingNotifications]);
#endif
    notifyList[numberOfPendingNotifications]->Observe(this,
                                                      OBSERVER_TOPIC_ACTIVE,
                                                      timeStr.get());
  }
  return NS_OK;
}

NS_IMETHODIMP
nsIdleService::GetIdleTime(uint32_t* idleTime)
{
  
  if (!idleTime) {
    return NS_ERROR_NULL_POINTER;
  }

  
  uint32_t polledIdleTimeMS;

  bool polledIdleTimeIsValid = PollIdleTime(&polledIdleTimeMS);

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Get idle time: polled %u msec, valid = %d",
          polledIdleTimeMS, polledIdleTimeIsValid));
  
  
  uint32_t timeSinceResetInMS = (PR_Now() - mLastUserInteractionInPR) /
                                PR_USEC_PER_MSEC;

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Get idle time: time since reset %u msec",
          timeSinceResetInMS));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Get idle time: time since reset %u msec",
                      timeSinceResetInMS);
#endif

  
  if (!polledIdleTimeIsValid) {
    
    *idleTime = timeSinceResetInMS;
    return NS_OK;
  }

  
  *idleTime = NS_MIN(timeSinceResetInMS, polledIdleTimeMS);

  return NS_OK;
}


bool
nsIdleService::PollIdleTime(uint32_t* )
{
  
  return false;
}

bool
nsIdleService::UsePollMode()
{
  uint32_t dummy;
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

  
  uint32_t currentIdleTimeInMS;

  if (NS_FAILED(GetIdleTime(&currentIdleTimeInMS))) {
    PR_LOG(sLog, PR_LOG_ALWAYS,
           ("idleService: Idle timer callback: failed to get idle time"));
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Idle timer callback: failed to get idle time");
#endif
    return;
  }

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Idle timer callback: current idle time %u msec",
          currentIdleTimeInMS));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Idle timer callback: current idle time %u msec",
                      currentIdleTimeInMS);
#endif

  
  
  
  
  if (((PR_Now() - mLastUserInteractionInPR) / PR_USEC_PER_MSEC) >
      currentIdleTimeInMS)
  {
    
    
    ResetIdleTimeOut(currentIdleTimeInMS);

    
  }

  
  uint32_t currentIdleTimeInS = currentIdleTimeInMS / PR_MSEC_PER_SEC;

  
  if (mDeltaToNextIdleSwitchInS > currentIdleTimeInS) {
    
    ReconfigureTimer();
    return;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_IDLE_MS> timer;

  
  mDeltaToNextIdleSwitchInS = PR_UINT32_MAX;

  
  nsCOMArray<nsIObserver> notifyList;

  for (uint32_t i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (!curListener.isIdle) {
      
      if (curListener.reqIdleTime <= currentIdleTimeInS) {
        
        
        notifyList.AppendObject(curListener.observer);
        
        curListener.isIdle = true;
        
        mAnyObserverIdle = true;
      } else {
        
        mDeltaToNextIdleSwitchInS = PR_MIN(mDeltaToNextIdleSwitchInS,
                                           curListener.reqIdleTime);
      }
    }
  }

  
  
  ReconfigureTimer();

  int32_t numberOfPendingNotifications = notifyList.Count();
  Telemetry::Accumulate(Telemetry::IDLE_NOTIFY_IDLE_LISTENERS,
                        numberOfPendingNotifications);

  
  if (!numberOfPendingNotifications) {
    return;
  }

  
  nsAutoString timeStr;
  timeStr.AppendInt(currentIdleTimeInS);

  
  while (numberOfPendingNotifications--) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Idle timer callback: tell observer %x user is idle",
            notifyList[numberOfPendingNotifications]));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Idle timer callback: tell observer %x user is idle",
                      notifyList[numberOfPendingNotifications]);
#endif
    notifyList[numberOfPendingNotifications]->Observe(this,
                                                      OBSERVER_TOPIC_IDLE,
                                                      timeStr.get());
  }
}

void
nsIdleService::SetTimerExpiryIfBefore(PRTime aNextTimeoutInPR)
{
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: SetTimerExpiryIfBefore: next timeout %lld usec",
          aNextTimeoutInPR));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "SetTimerExpiryIfBefore: next timeout %lld usec",
                      aNextTimeoutInPR);
#endif

  
  if (!mTimer) {
    return;
  }

  
  
  if (mCurrentlySetToTimeoutAtInPR > aNextTimeoutInPR ||
      !mCurrentlySetToTimeoutAtInPR) {

#if defined(PR_LOGGING) || defined(ANDROID)
    PRTime oldTimeout = mCurrentlySetToTimeoutAtInPR;
#endif

    mCurrentlySetToTimeoutAtInPR = aNextTimeoutInPR ;

    
    mTimer->Cancel();

    
    PRTime currentTimeInPR = PR_Now();
    if (currentTimeInPR > mCurrentlySetToTimeoutAtInPR) {
      mCurrentlySetToTimeoutAtInPR = currentTimeInPR;
    }

    
    mCurrentlySetToTimeoutAtInPR += 10 * PR_USEC_PER_MSEC;

    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: reset timer expiry from %lld usec to %lld usec",
            oldTimeout, mCurrentlySetToTimeoutAtInPR));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "reset timer expiry from %lld usec to %lld usec",
                      oldTimeout, mCurrentlySetToTimeoutAtInPR);
#endif

    
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
    
    
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: ReconfigureTimer: no idle or waiting observers"));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "ReconfigureTimer: no idle or waiting observers");
#endif
    return;
  }

  

  
  
  PRTime curTimeInPR = PR_Now();

  PRTime nextTimeoutAtInPR = mLastUserInteractionInPR +
                             (((PRTime)mDeltaToNextIdleSwitchInS) *
                              PR_USEC_PER_SEC);

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: next timeout %lld usec (%u msec from now)",
          nextTimeoutAtInPR,
          (uint32_t)((nextTimeoutAtInPR - curTimeInPR) / PR_USEC_PER_MSEC)));
#ifdef ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "next timeout %lld usec (%lld msec from now)",
                      nextTimeoutAtInPR,
                      ((nextTimeoutAtInPR - curTimeInPR) / PR_USEC_PER_MSEC));
#endif
  
  if (mAnyObserverIdle && UsePollMode()) {
    PRTime pollTimeout = curTimeInPR +
                         MIN_IDLE_POLL_INTERVAL_MSEC * PR_USEC_PER_MSEC;

    if (nextTimeoutAtInPR > pollTimeout) {
      PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: idle observers, reducing timeout to %u msec from now",
            MIN_IDLE_POLL_INTERVAL_MSEC));
#ifdef ANDROID
      __android_log_print(ANDROID_LOG_INFO, "IdleService",
                          "idle observers, reducing timeout to %u msec from now",
                          MIN_IDLE_POLL_INTERVAL_MSEC);
#endif
      nextTimeoutAtInPR = pollTimeout;
    }
  }

  SetTimerExpiryIfBefore(nextTimeoutAtInPR);
}

