






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
#include <algorithm>

#ifdef MOZ_WIDGET_ANDROID
#include <android/log.h>
#endif

using namespace mozilla;


#define MIN_IDLE_POLL_INTERVAL_MSEC (5 * PR_MSEC_PER_SEC) /* 5 sec */




#define DAILY_SIGNIFICANT_IDLE_SERVICE_SEC (3 * 60)




#define DAILY_SHORTENED_IDLE_SERVICE_SEC 60


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
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("nsIdleServiceDaily: Observe '%s' (%d)",
          aTopic, mShutdownInProgress));

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

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("nsIdleServiceDaily: Notifying idle-daily observers"));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Notifying idle-daily observers");
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

  
  (void)mIdleService->RemoveIdleObserver(this, mIdleDailyTriggerWait);

  
  int32_t nowSec = static_cast<int32_t>(PR_Now() / PR_USEC_PER_SEC);
  Preferences::SetInt(PREF_LAST_DAILY, nowSec);

  
  
  nsIPrefService* prefs = Preferences::GetService();
  if (prefs) {
    prefs->SavePrefFile(nullptr);
  }

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("nsIdleServiceDaily: Storing last idle time as %d sec.", nowSec));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Storing last idle time as %d", nowSec);
#endif

  
  mExpectedTriggerTime  = PR_Now() + ((PRTime)SECONDS_PER_DAY *
                                      (PRTime)PR_USEC_PER_SEC);

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("nsIdleServiceDaily: Restarting daily timer"));

  
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
  , mExpectedTriggerTime(0)
  , mIdleDailyTriggerWait(DAILY_SIGNIFICANT_IDLE_SERVICE_SEC)
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
  int32_t secondsSinceLastDaily = nowSec - lastDaily;

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("nsIdleServiceDaily: Init: seconds since last daily: %d",
          secondsSinceLastDaily));

  
  
  if (secondsSinceLastDaily > SECONDS_PER_DAY) {
    
    bool hasBeenLongWait = (lastDaily &&
                            (secondsSinceLastDaily > (SECONDS_PER_DAY * 2)));

    PR_LOG(sLog, PR_LOG_DEBUG,
           ("nsIdleServiceDaily: has been long wait? %d",
            hasBeenLongWait));

    
    
    StageIdleDaily(hasBeenLongWait);
  } else {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("nsIdleServiceDaily: Setting timer a day from now"));
#ifdef MOZ_WIDGET_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Setting timer a day from now");
#endif

    
    
    int32_t milliSecLeftUntilDaily = (SECONDS_PER_DAY - secondsSinceLastDaily)
      * PR_MSEC_PER_SEC;

    PR_LOG(sLog, PR_LOG_DEBUG,
           ("nsIdleServiceDaily: Seconds till next timeout: %d",
            (SECONDS_PER_DAY - secondsSinceLastDaily)));

    
    
    
    mExpectedTriggerTime  = PR_Now() +
      (milliSecLeftUntilDaily * PR_USEC_PER_MSEC);

    (void)mTimer->InitWithFuncCallback(DailyCallback,
                                       this,
                                       milliSecLeftUntilDaily,
                                       nsITimer::TYPE_ONE_SHOT);
  }

  
  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (obs) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("nsIdleServiceDaily: Registering for system event observers."));
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
nsIdleServiceDaily::StageIdleDaily(bool aHasBeenLongWait)
{
  NS_ASSERTION(mIdleService, "No idle service available?");
  PR_LOG(sLog, PR_LOG_DEBUG,
          ("nsIdleServiceDaily: Registering Idle observer callback "
           "(short wait requested? %d)", aHasBeenLongWait));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Registering Idle observer callback");
#endif
  mIdleDailyTriggerWait = (aHasBeenLongWait ?
                             DAILY_SHORTENED_IDLE_SERVICE_SEC :
                             DAILY_SIGNIFICANT_IDLE_SERVICE_SEC);
  (void)mIdleService->AddIdleObserver(this, mIdleDailyTriggerWait);
}


void
nsIdleServiceDaily::DailyCallback(nsITimer* aTimer, void* aClosure)
{
  PR_LOG(sLog, PR_LOG_DEBUG,
          ("nsIdleServiceDaily: DailyCallback running"));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "DailyCallback running");
#endif

  nsIdleServiceDaily* self = static_cast<nsIdleServiceDaily*>(aClosure);

  
  
  PRTime now = PR_Now();
  if (self->mExpectedTriggerTime && now < self->mExpectedTriggerTime) {
    
    PRTime delayTime = self->mExpectedTriggerTime - now;

    
    delayTime += 10 * PR_USEC_PER_MSEC;

    PR_LOG(sLog, PR_LOG_DEBUG, ("nsIdleServiceDaily: DailyCallback resetting timer to %lld msec",
                        delayTime / PR_USEC_PER_MSEC));
#ifdef MOZ_WIDGET_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "DailyCallback resetting timer to %lld msec",
                        delayTime / PR_USEC_PER_MSEC);
#endif

    (void)self->mTimer->InitWithFuncCallback(DailyCallback,
                                             self,
                                             delayTime / PR_USEC_PER_MSEC,
                                             nsITimer::TYPE_ONE_SHOT);
    return;
  }

  
  
  self->StageIdleDaily(false);
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

nsIdleService::nsIdleService() : mCurrentlySetToTimeoutAt(TimeStamp()),
                                 mIdleObserverCount(0),
                                 mDeltaToNextIdleSwitchInS(UINT32_MAX),
                                 mLastUserInteraction(TimeStamp::Now())
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
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Register idle observer %x for %d seconds",
                      aObserver, aIdleTimeInS);
#endif

  NS_ENSURE_ARG_POINTER(aObserver);
  
  
  NS_ENSURE_ARG_RANGE(aIdleTimeInS, 1, (UINT32_MAX / 10) - 1);

  
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
#ifdef MOZ_WIDGET_ANDROID
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
  nsTArray<IdleListener>::index_type listenerIndex = mArrayListeners.IndexOf(listener, 0, c);
  if (listenerIndex != mArrayListeners.NoIndex) {
    if (mArrayListeners.ElementAt(listenerIndex).isIdle)
      mIdleObserverCount--;
    mArrayListeners.RemoveElementAt(listenerIndex);
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Remove observer %x (%d seconds), %d remain idle",
            aObserver, aTimeInS, mIdleObserverCount));
#ifdef MOZ_WIDGET_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Remove observer %x (%d seconds), %d remain idle",
                        aObserver, aTimeInS, mIdleObserverCount);
#endif
    return NS_OK;
  }

  
  PR_LOG(sLog, PR_LOG_WARNING, 
         ("idleService: Failed to remove idle observer %x (%d seconds)",
          aObserver, aTimeInS));
#ifdef MOZ_WIDGET_ANDROID
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

  
  mLastUserInteraction = TimeStamp::Now() -
                         TimeDuration::FromMilliseconds(idleDeltaInMS);

  
  if (mIdleObserverCount == 0) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: Reset idle timeout: no idle observers"));
    return NS_OK;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_BACK_MS> timer;
  nsCOMArray<nsIObserver> notifyList;
  mDeltaToNextIdleSwitchInS = UINT32_MAX;

  
  for (uint32_t i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (curListener.isIdle) {
      notifyList.AppendObject(curListener.observer);
      curListener.isIdle = false;
    }

    
    mDeltaToNextIdleSwitchInS = std::min(mDeltaToNextIdleSwitchInS,
                                       curListener.reqIdleTime);
  }

  
  mIdleObserverCount = 0;

  
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
#ifdef MOZ_WIDGET_ANDROID
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
  
  
  TimeDuration timeSinceReset = TimeStamp::Now() - mLastUserInteraction;
  uint32_t timeSinceResetInMS = timeSinceReset.ToMilliseconds();

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Get idle time: time since reset %u msec",
          timeSinceResetInMS));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Get idle time: time since reset %u msec",
                      timeSinceResetInMS);
#endif

  
  if (!polledIdleTimeIsValid) {
    
    *idleTime = timeSinceResetInMS;
    return NS_OK;
  }

  
  *idleTime = std::min(timeSinceResetInMS, polledIdleTimeMS);

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
  
  mCurrentlySetToTimeoutAt = TimeStamp();

  
  uint32_t lastIdleTimeInMS = static_cast<uint32_t>((TimeStamp::Now() -
                              mLastUserInteraction).ToMilliseconds());
  
  uint32_t currentIdleTimeInMS;

  if (NS_FAILED(GetIdleTime(&currentIdleTimeInMS))) {
    PR_LOG(sLog, PR_LOG_ALWAYS,
           ("idleService: Idle timer callback: failed to get idle time"));
#ifdef MOZ_WIDGET_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "Idle timer callback: failed to get idle time");
#endif
    return;
  }

  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: Idle timer callback: current idle time %u msec",
          currentIdleTimeInMS));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "Idle timer callback: current idle time %u msec",
                      currentIdleTimeInMS);
#endif

  
  
  
  if (lastIdleTimeInMS > currentIdleTimeInMS)
  {
    
    
    ResetIdleTimeOut(currentIdleTimeInMS);

    
  }

  
  uint32_t currentIdleTimeInS = currentIdleTimeInMS / PR_MSEC_PER_SEC;

  
  if (mDeltaToNextIdleSwitchInS > currentIdleTimeInS) {
    
    ReconfigureTimer();
    return;
  }

  
  Telemetry::AutoTimer<Telemetry::IDLE_NOTIFY_IDLE_MS> timer;

  
  mDeltaToNextIdleSwitchInS = UINT32_MAX;

  
  nsCOMArray<nsIObserver> notifyList;

  for (uint32_t i = 0; i < mArrayListeners.Length(); i++) {
    IdleListener& curListener = mArrayListeners.ElementAt(i);

    
    if (!curListener.isIdle) {
      
      if (curListener.reqIdleTime <= currentIdleTimeInS) {
        
        
        notifyList.AppendObject(curListener.observer);
        
        curListener.isIdle = true;
        
        mIdleObserverCount++;
      } else {
        
        mDeltaToNextIdleSwitchInS = std::min(mDeltaToNextIdleSwitchInS,
                                           curListener.reqIdleTime);
      }
    }
  }

  
  
  ReconfigureTimer();

  int32_t numberOfPendingNotifications = notifyList.Count();
  Telemetry::Accumulate(Telemetry::IDLE_NOTIFY_IDLE_LISTENERS,
                        numberOfPendingNotifications);

  
  if (!numberOfPendingNotifications) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: **** Idle timer callback: no observers to message."));
    return;
  }

  
  nsAutoString timeStr;
  timeStr.AppendInt(currentIdleTimeInS);

  
  while (numberOfPendingNotifications--) {
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: **** Idle timer callback: tell observer %x user is idle",
            notifyList[numberOfPendingNotifications]));
#ifdef MOZ_WIDGET_ANDROID
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
nsIdleService::SetTimerExpiryIfBefore(TimeStamp aNextTimeout)
{
  TimeDuration nextTimeoutDuration = aNextTimeout - TimeStamp::Now();
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: SetTimerExpiryIfBefore: next timeout %0.f msec from now",
          nextTimeoutDuration.ToMilliseconds()));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "SetTimerExpiryIfBefore: next timeout %0.f msec from now",
                      nextTimeoutDuration.ToMilliseconds());
#endif

  
  if (!mTimer) {
    return;
  }

  
  
  if (mCurrentlySetToTimeoutAt.IsNull() ||
      mCurrentlySetToTimeoutAt > aNextTimeout) {

    mCurrentlySetToTimeoutAt = aNextTimeout;

    
    mTimer->Cancel();

    
    TimeStamp currentTime = TimeStamp::Now();
    if (currentTime > mCurrentlySetToTimeoutAt) {
      mCurrentlySetToTimeoutAt = currentTime;
    }

    
    mCurrentlySetToTimeoutAt += TimeDuration::FromMilliseconds(10);

    TimeDuration deltaTime = mCurrentlySetToTimeoutAt - currentTime;
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: IdleService reset timer expiry to %0.f msec from now",
            deltaTime.ToMilliseconds()));
#ifdef MOZ_WIDGET_ANDROID
    __android_log_print(ANDROID_LOG_INFO, "IdleService",
                        "reset timer expiry to %0.f msec from now",
                        deltaTime.ToMilliseconds());
#endif

    
    mTimer->InitWithFuncCallback(StaticIdleTimerCallback,
                                 this,
                                 deltaTime.ToMilliseconds(),
                                 nsITimer::TYPE_ONE_SHOT);

  }
}


void
nsIdleService::ReconfigureTimer(void)
{
  
  if ((mIdleObserverCount == 0) && UINT32_MAX == mDeltaToNextIdleSwitchInS) {
    
    
    PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: ReconfigureTimer: no idle or waiting observers"));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "ReconfigureTimer: no idle or waiting observers");
#endif
    return;
  }

  

  
  
  TimeStamp curTime = TimeStamp::Now();

  TimeStamp nextTimeoutAt = mLastUserInteraction +
                            TimeDuration::FromSeconds(mDeltaToNextIdleSwitchInS);

  TimeDuration nextTimeoutDuration = nextTimeoutAt - curTime;
  PR_LOG(sLog, PR_LOG_DEBUG,
         ("idleService: next timeout %0.f msec from now",
          nextTimeoutDuration.ToMilliseconds()));
#ifdef MOZ_WIDGET_ANDROID
  __android_log_print(ANDROID_LOG_INFO, "IdleService",
                      "next timeout %0.f msec from now",
                      nextTimeoutDuration.ToMilliseconds());
#endif
  
  if ((mIdleObserverCount > 0) && UsePollMode()) {
    TimeStamp pollTimeout =
        curTime + TimeDuration::FromMilliseconds(MIN_IDLE_POLL_INTERVAL_MSEC);

    if (nextTimeoutAt > pollTimeout) {
      PR_LOG(sLog, PR_LOG_DEBUG,
           ("idleService: idle observers, reducing timeout to %u msec from now",
            MIN_IDLE_POLL_INTERVAL_MSEC));
#ifdef MOZ_WIDGET_ANDROID
      __android_log_print(ANDROID_LOG_INFO, "IdleService",
                          "idle observers, reducing timeout to %u msec from now",
                          MIN_IDLE_POLL_INTERVAL_MSEC);
#endif
      nextTimeoutAt = pollTimeout;
    }
  }

  SetTimerExpiryIfBefore(nextTimeoutAt);
}

