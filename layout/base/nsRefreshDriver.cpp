


















#ifdef XP_WIN
#include <windows.h>


#include <mmsystem.h>
#endif

#include "mozilla/Util.h"

#include "nsRefreshDriver.h"
#include "nsITimer.h"
#include "nsPresContext.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsCSSFrameConstructor.h"
#include "nsIDocument.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "jsapi.h"
#include "nsContentUtils.h"
#include "mozilla/Preferences.h"
#include "nsViewManager.h"
#include "sampler.h"
#include "nsNPAPIPluginInstance.h"

using mozilla::TimeStamp;
using mozilla::TimeDuration;

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *gLog = nullptr;
#define LOG(...) PR_LOG(gLog, PR_LOG_NOTICE, (__VA_ARGS__))
#else
#define LOG(...) do { } while(0)
#endif

#define DEFAULT_FRAME_RATE 60
#define DEFAULT_THROTTLED_FRAME_RATE 1

#define DEFAULT_INACTIVE_TIMER_DISABLE_SECONDS 600

namespace mozilla {











class RefreshDriverTimer {
public:
  


  RefreshDriverTimer(double aRate)
  {
    SetRate(aRate);
  }

  virtual ~RefreshDriverTimer()
  {
    NS_ASSERTION(mRefreshDrivers.Length() == 0, "Should have removed all refresh drivers from here by now!");
  }

  virtual void AddRefreshDriver(nsRefreshDriver* aDriver)
  {
    LOG("[%p] AddRefreshDriver %p", this, aDriver);

    NS_ASSERTION(!mRefreshDrivers.Contains(aDriver), "AddRefreshDriver for a refresh driver that's already in the list!");
    mRefreshDrivers.AppendElement(aDriver);

    if (mRefreshDrivers.Length() == 1) {
      StartTimer();
    }
  }

  virtual void RemoveRefreshDriver(nsRefreshDriver* aDriver)
  {
    LOG("[%p] RemoveRefreshDriver %p", this, aDriver);

    NS_ASSERTION(mRefreshDrivers.Contains(aDriver), "RemoveRefreshDriver for a refresh driver that's not in the list!");
    mRefreshDrivers.RemoveElement(aDriver);

    if (mRefreshDrivers.Length() == 0) {
      StopTimer();
    }
  }

  double GetRate() const
  {
    return mRateMilliseconds;
  }

  
  virtual void SetRate(double aNewRate)
  {
    mRateMilliseconds = aNewRate;
    mRateDuration = TimeDuration::FromMilliseconds(mRateMilliseconds);
  }

  TimeStamp MostRecentRefresh() const { return mLastFireTime; }
  int64_t MostRecentRefreshEpochTime() const { return mLastFireEpoch; }

protected:
  virtual void StartTimer() = 0;
  virtual void StopTimer() = 0;
  virtual void ScheduleNextTick(TimeStamp aNowTime) = 0;

  



  void Tick()
  {
    int64_t jsnow = JS_Now();
    TimeStamp now = TimeStamp::Now();

    ScheduleNextTick(now);

    mLastFireEpoch = jsnow;
    mLastFireTime = now;

    LOG("[%p] ticking drivers...", this);
    nsTArray<nsRefPtr<nsRefreshDriver> > drivers(mRefreshDrivers);
    for (size_t i = 0; i < drivers.Length(); ++i) {
      
      if (drivers[i]->IsTestControllingRefreshesEnabled()) {
        continue;
      }

      TickDriver(drivers[i], jsnow, now);
    }
    LOG("[%p] done.", this);
  }

  static void TickDriver(nsRefreshDriver* driver, int64_t jsnow, TimeStamp now)
  {
    LOG(">> TickDriver: %p (jsnow: %lld)", driver, jsnow);
    driver->Tick(jsnow, now);
  }

  double mRateMilliseconds;
  TimeDuration mRateDuration;

  int64_t mLastFireEpoch;
  TimeStamp mLastFireTime;
  TimeStamp mTargetTime;

  nsTArray<nsRefPtr<nsRefreshDriver> > mRefreshDrivers;

  
  
  static void TimerTick(nsITimer* aTimer, void* aClosure)
  {
    RefreshDriverTimer *timer = static_cast<RefreshDriverTimer*>(aClosure);
    timer->Tick();
  }
};








class SimpleTimerBasedRefreshDriverTimer :
    public RefreshDriverTimer
{
public:
  SimpleTimerBasedRefreshDriverTimer(double aRate)
    : RefreshDriverTimer(aRate)
  {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  virtual ~SimpleTimerBasedRefreshDriverTimer()
  {
    StopTimer();
  }

protected:

  virtual void StartTimer()
  {
    
    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);
  }

  virtual void StopTimer()
  {
    mTimer->Cancel();
  }

  nsRefPtr<nsITimer> mTimer;
};










class PreciseRefreshDriverTimer :
    public SimpleTimerBasedRefreshDriverTimer
{
public:
  PreciseRefreshDriverTimer(double aRate)
    : SimpleTimerBasedRefreshDriverTimer(aRate)
  {
  }

protected:
  virtual void ScheduleNextTick(TimeStamp aNowTime)
  {
    
    
    
    int numElapsedIntervals = static_cast<int>((aNowTime - mTargetTime) / mRateDuration);

    if (numElapsedIntervals < 0) {
      
      
      
      numElapsedIntervals = 0;
    }

    
    
    
    
    
    
    
    TimeStamp newTarget = mTargetTime + mRateDuration * (numElapsedIntervals + 1);

    
    uint32_t delay = static_cast<uint32_t>((newTarget - aNowTime).ToMilliseconds());

    
    
    
#if 0
    if (numElapsedIntervals > 0) {
      
      newTarget = aNowTime;
      delay = 0;
    }
#endif

    
    LOG("[%p] precise timer last tick late by %f ms, next tick in %d ms",
        this,
        (aNowTime - mTargetTime).ToMilliseconds(),
        delay);

    
    LOG("[%p] scheduling callback for %d ms (2)", this, delay);
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);

    mTargetTime = newTarget;
  }
};














class InactiveRefreshDriverTimer :
    public RefreshDriverTimer
{
public:
  InactiveRefreshDriverTimer(double aRate)
    : RefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(-1.0),
      mNextDriverIndex(0)
  {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  InactiveRefreshDriverTimer(double aRate, double aDisableAfterMilliseconds)
    : RefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(aDisableAfterMilliseconds),
      mNextDriverIndex(0)
  {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  virtual void AddRefreshDriver(nsRefreshDriver* aDriver)
  {
    RefreshDriverTimer::AddRefreshDriver(aDriver);

    LOG("[%p] inactive timer got new refresh driver %p, resetting rate",
        this, aDriver);

    
    mNextTickDuration = mRateMilliseconds;

    
    
    mNextDriverIndex = mRefreshDrivers.Length() - 1;

    StopTimer();
    StartTimer();
  }

protected:
  virtual void StartTimer()
  {
    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    mTargetTime = mLastFireTime + mRateDuration;

    uint32_t delay = static_cast<uint32_t>(mRateMilliseconds);
    mTimer->InitWithFuncCallback(TimerTickOne, this, delay, nsITimer::TYPE_ONE_SHOT);
  }

  virtual void StopTimer()
  {
    mTimer->Cancel();
  }

  virtual void ScheduleNextTick(TimeStamp aNowTime)
  {
    if (mDisableAfterMilliseconds > 0.0 &&
        mNextTickDuration > mDisableAfterMilliseconds)
    {
      
      
      
      return;
    }

    
    if (mNextDriverIndex >= mRefreshDrivers.Length()) {
      mNextTickDuration *= 2.0;
      mNextDriverIndex = 0;
    }

    
    uint32_t delay = static_cast<uint32_t>(mNextTickDuration);
    mTimer->InitWithFuncCallback(TimerTickOne, this, delay, nsITimer::TYPE_ONE_SHOT);

    LOG("[%p] inactive timer next tick in %f ms [index %d/%d]", this, mNextTickDuration,
        mNextDriverIndex, mRefreshDrivers.Length());
  }

  
  void TickOne()
  {
    int64_t jsnow = JS_Now();
    TimeStamp now = TimeStamp::Now();

    ScheduleNextTick(now);

    mLastFireEpoch = jsnow;
    mLastFireTime = now;

    nsTArray<nsRefPtr<nsRefreshDriver> > drivers(mRefreshDrivers);
    if (mNextDriverIndex < drivers.Length() &&
        !drivers[mNextDriverIndex]->IsTestControllingRefreshesEnabled())
    {
      TickDriver(drivers[mNextDriverIndex], jsnow, now);
    }

    mNextDriverIndex++;
  }

  static void TimerTickOne(nsITimer* aTimer, void* aClosure)
  {
    InactiveRefreshDriverTimer *timer = static_cast<InactiveRefreshDriverTimer*>(aClosure);
    timer->TickOne();
  }

  nsRefPtr<nsITimer> mTimer;
  double mNextTickDuration;
  double mDisableAfterMilliseconds;
  uint32_t mNextDriverIndex;
};

} 

static PreciseRefreshDriverTimer *sRegularRateTimer = nullptr;
static InactiveRefreshDriverTimer *sThrottledRateTimer = nullptr;

#ifdef XP_WIN
static int32_t sHighPrecisionTimerRequests = 0;

static nsITimer *sDisableHighPrecisionTimersTimer = nullptr;
#endif

 void
nsRefreshDriver::InitializeStatics()
{
#ifdef PR_LOGGING
  if (!gLog) {
    gLog = PR_NewLogModule("nsRefreshDriver");
  }
#endif
}

 void
nsRefreshDriver::Shutdown()
{
  
  delete sRegularRateTimer;
  delete sThrottledRateTimer;

  sRegularRateTimer = nullptr;
  sThrottledRateTimer = nullptr;

#ifdef XP_WIN
  if (sDisableHighPrecisionTimersTimer) {
    sDisableHighPrecisionTimersTimer->Cancel();
    NS_RELEASE(sDisableHighPrecisionTimersTimer);
    timeEndPeriod(1);
  } else if (sHighPrecisionTimerRequests) {
    timeEndPeriod(1);
  }
#endif
}

 int32_t
nsRefreshDriver::DefaultInterval()
{
  return NSToIntRound(1000.0 / DEFAULT_FRAME_RATE);
}



double
nsRefreshDriver::GetRegularTimerInterval() const
{
  int32_t rate = Preferences::GetInt("layout.frame_rate", -1);
  if (rate <= 0) {
    
    rate = DEFAULT_FRAME_RATE;
  }
  return 1000.0 / rate;
}

double
nsRefreshDriver::GetThrottledTimerInterval() const
{
  int32_t rate = Preferences::GetInt("layout.throttled_frame_rate", -1);
  if (rate <= 0) {
    rate = DEFAULT_THROTTLED_FRAME_RATE;
  }
  return 1000.0 / rate;
}

double
nsRefreshDriver::GetRefreshTimerInterval() const
{
  return mThrottled ? GetThrottledTimerInterval() : GetRegularTimerInterval();
}

RefreshDriverTimer*
nsRefreshDriver::ChooseTimer() const
{
  if (mThrottled) {
    if (!sThrottledRateTimer) 
      sThrottledRateTimer = new InactiveRefreshDriverTimer(GetThrottledTimerInterval(),
                                                           DEFAULT_INACTIVE_TIMER_DISABLE_SECONDS * 1000.0);
    return sThrottledRateTimer;
  }

  if (!sRegularRateTimer)
    sRegularRateTimer = new PreciseRefreshDriverTimer(GetRegularTimerInterval());
  return sRegularRateTimer;
}

nsRefreshDriver::nsRefreshDriver(nsPresContext* aPresContext)
  : mActiveTimer(nullptr),
    mPresContext(aPresContext),
    mFrozen(false),
    mThrottled(false),
    mTestControllingRefreshes(false),
    mViewManagerFlushIsPending(false),
    mRequestedHighPrecision(false)
{
  mMostRecentRefreshEpochTime = JS_Now();
  mMostRecentRefresh = TimeStamp::Now();

  mRequests.Init();
}

nsRefreshDriver::~nsRefreshDriver()
{
  NS_ABORT_IF_FALSE(ObserverCount() == 0,
                    "observers should have unregistered");
  NS_ABORT_IF_FALSE(!mActiveTimer, "timer should be gone");
  
  for (uint32_t i = 0; i < mPresShellsToInvalidateIfHidden.Length(); i++) {
    mPresShellsToInvalidateIfHidden[i]->InvalidatePresShellIfHidden();
  }
  mPresShellsToInvalidateIfHidden.Clear();
}



void
nsRefreshDriver::AdvanceTimeAndRefresh(int64_t aMilliseconds)
{
  
  StopTimer();

  if (!mTestControllingRefreshes) {
    mMostRecentRefreshEpochTime = JS_Now();
    mMostRecentRefresh = TimeStamp::Now();

    mTestControllingRefreshes = true;
  }

  mMostRecentRefreshEpochTime += aMilliseconds * 1000;
  mMostRecentRefresh += TimeDuration::FromMilliseconds((double) aMilliseconds);

  nsCxPusher pusher;
  pusher.PushNull();
  DoTick();
}

void
nsRefreshDriver::RestoreNormalRefresh()
{
  mTestControllingRefreshes = false;
  EnsureTimerStarted(false);
}

TimeStamp
nsRefreshDriver::MostRecentRefresh() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted(false);

  return mMostRecentRefresh;
}

int64_t
nsRefreshDriver::MostRecentRefreshEpochTime() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted(false);

  return mMostRecentRefreshEpochTime;
}

bool
nsRefreshDriver::AddRefreshObserver(nsARefreshObserver* aObserver,
                                    mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  bool success = array.AppendElement(aObserver) != nullptr;

  EnsureTimerStarted(false);

  return success;
}

bool
nsRefreshDriver::RemoveRefreshObserver(nsARefreshObserver* aObserver,
                                       mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.RemoveElement(aObserver);
}

bool
nsRefreshDriver::AddImageRequest(imgIRequest* aRequest)
{
  if (!mRequests.PutEntry(aRequest)) {
    return false;
  }

  EnsureTimerStarted(false);

  return true;
}

void
nsRefreshDriver::RemoveImageRequest(imgIRequest* aRequest)
{
  mRequests.RemoveEntry(aRequest);
}

void nsRefreshDriver::ClearAllImageRequests()
{
  mRequests.Clear();
}

void
nsRefreshDriver::EnsureTimerStarted(bool aAdjustingTimer)
{
  if (mTestControllingRefreshes)
    return;

  
  if (mActiveTimer && !aAdjustingTimer)
    return;

  if (mFrozen || !mPresContext) {
    
    StopTimer();
    return;
  }

  
  
  
  RefreshDriverTimer *newTimer = ChooseTimer();
  if (newTimer != mActiveTimer) {
    if (mActiveTimer)
      mActiveTimer->RemoveRefreshDriver(this);
    mActiveTimer = newTimer;
    mActiveTimer->AddRefreshDriver(this);
  }

  mMostRecentRefresh = mActiveTimer->MostRecentRefresh();
  mMostRecentRefreshEpochTime = mActiveTimer->MostRecentRefreshEpochTime();
}

void
nsRefreshDriver::StopTimer()
{
  if (!mActiveTimer)
    return;

  mActiveTimer->RemoveRefreshDriver(this);
  mActiveTimer = nullptr;

  if (mRequestedHighPrecision) {
    SetHighPrecisionTimersEnabled(false);
  }
}

#ifdef XP_WIN
static void
DisableHighPrecisionTimersCallback(nsITimer *aTimer, void *aClosure)
{
  timeEndPeriod(1);
  NS_RELEASE(sDisableHighPrecisionTimersTimer);
}
#endif

void
nsRefreshDriver::ConfigureHighPrecision()
{
  bool haveFrameRequestCallbacks = mFrameRequestCallbackDocs.Length() > 0;

  
  
  if (!mThrottled && !mRequestedHighPrecision && haveFrameRequestCallbacks) {
    SetHighPrecisionTimersEnabled(true);
  } else if (mRequestedHighPrecision && !haveFrameRequestCallbacks) {
    SetHighPrecisionTimersEnabled(false);
  }
}

void
nsRefreshDriver::SetHighPrecisionTimersEnabled(bool aEnable)
{
  LOG("[%p] SetHighPrecisionTimersEnabled (%s)", this, aEnable ? "true" : "false");

  if (aEnable) {
    NS_ASSERTION(!mRequestedHighPrecision, "SetHighPrecisionTimersEnabled(true) called when already requested!");
#ifdef XP_WIN
    if (++sHighPrecisionTimerRequests == 1) {
      
      
      if (sDisableHighPrecisionTimersTimer) {
        sDisableHighPrecisionTimersTimer->Cancel();
        NS_RELEASE(sDisableHighPrecisionTimersTimer);
      } else {
        timeBeginPeriod(1);
      }
    }
#endif
    mRequestedHighPrecision = true;
  } else {
    NS_ASSERTION(mRequestedHighPrecision, "SetHighPrecisionTimersEnabled(false) called when not requested!");
#ifdef XP_WIN
    if (--sHighPrecisionTimerRequests == 0) {
      
      
      
      
      NS_ASSERTION(!sDisableHighPrecisionTimersTimer, "We shouldn't have an outstanding disable-high-precision timer !");

      nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);
      if (timer) {
        timer.forget(&sDisableHighPrecisionTimersTimer);
        sDisableHighPrecisionTimersTimer->InitWithFuncCallback(DisableHighPrecisionTimersCallback,
                                                               nullptr,
                                                               90 * 1000,
                                                               nsITimer::TYPE_ONE_SHOT);
      } else {
        
        
        timeEndPeriod(1);
      }
    }
#endif
    mRequestedHighPrecision = false;
  }
}

uint32_t
nsRefreshDriver::ObserverCount() const
{
  uint32_t sum = 0;
  for (uint32_t i = 0; i < ArrayLength(mObservers); ++i) {
    sum += mObservers[i].Length();
  }

  
  
  
  
  sum += mStyleFlushObservers.Length();
  sum += mLayoutFlushObservers.Length();
  sum += mFrameRequestCallbackDocs.Length();
  sum += mViewManagerFlushIsPending;
  return sum;
}

uint32_t
nsRefreshDriver::ImageRequestCount() const
{
  return mRequests.Count();
}

nsRefreshDriver::ObserverArray&
nsRefreshDriver::ArrayFor(mozFlushType aFlushType)
{
  switch (aFlushType) {
    case Flush_Style:
      return mObservers[0];
    case Flush_Layout:
      return mObservers[1];
    case Flush_Display:
      return mObservers[2];
    default:
      NS_ABORT_IF_FALSE(false, "bad flush type");
      return *static_cast<ObserverArray*>(nullptr);
  }
}





NS_IMPL_ISUPPORTS1(nsRefreshDriver, nsISupports)





void
nsRefreshDriver::DoTick()
{
  NS_PRECONDITION(!mFrozen, "Why are we notified while frozen?");
  NS_PRECONDITION(mPresContext, "Why are we notified after disconnection?");
  NS_PRECONDITION(!nsContentUtils::GetCurrentJSContext(),
                  "Shouldn't have a JSContext on the stack");

  if (mTestControllingRefreshes) {
    Tick(mMostRecentRefreshEpochTime, mMostRecentRefresh);
  } else {
    Tick(JS_Now(), TimeStamp::Now());
  }
}

void
nsRefreshDriver::Tick(int64_t aNowEpoch, TimeStamp aNowTime)
{
  NS_PRECONDITION(!nsContentUtils::GetCurrentJSContext(),
                  "Shouldn't have a JSContext on the stack");

  if (nsNPAPIPluginInstance::InPluginCallUnsafeForReentry()) {
    NS_ERROR("Refresh driver should not run during plugin call!");
    
    return;
  }

  SAMPLE_LABEL("nsRefreshDriver", "Tick");

  
  
  
  if (mFrozen || !mPresContext) {
    return;
  }

  mMostRecentRefresh = aNowTime;
  mMostRecentRefreshEpochTime = aNowEpoch;

  nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
  if (!presShell || (ObserverCount() == 0 && ImageRequestCount() == 0)) {
    
    
    
    
    
    
    
    StopTimer();
    return;
  }

  





  for (uint32_t i = 0; i < ArrayLength(mObservers); ++i) {
    ObserverArray::EndLimitedIterator etor(mObservers[i]);
    while (etor.HasMore()) {
      nsRefPtr<nsARefreshObserver> obs = etor.GetNext();
      obs->WillRefresh(aNowTime);
      
      if (!mPresContext || !mPresContext->GetPresShell()) {
        StopTimer();
        return;
      }
    }

    if (i == 0) {
      
      nsIDocument::FrameRequestCallbackList frameRequestCallbacks;
      for (uint32_t i = 0; i < mFrameRequestCallbackDocs.Length(); ++i) {
        mFrameRequestCallbackDocs[i]->
          TakeFrameRequestCallbacks(frameRequestCallbacks);
      }
      
      
      mFrameRequestCallbackDocs.Clear();

      int64_t eventTime = aNowEpoch / PR_USEC_PER_MSEC;
      for (uint32_t i = 0; i < frameRequestCallbacks.Length(); ++i) {
        nsAutoMicroTask mt;
        frameRequestCallbacks[i]->Sample(eventTime);
      }

      
      if (mPresContext && mPresContext->GetPresShell()) {
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mStyleFlushObservers);
        for (uint32_t j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mStyleFlushObservers.Contains(shell))
            continue;
          NS_ADDREF(shell);
          mStyleFlushObservers.RemoveElement(shell);
          shell->FrameConstructor()->mObservingRefreshDriver = false;
          shell->FlushPendingNotifications(ChangesToFlush(Flush_Style, false));
          NS_RELEASE(shell);
        }
      }
    } else if  (i == 1) {
      
      if (mPresContext && mPresContext->GetPresShell()) {
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mLayoutFlushObservers);
        for (uint32_t j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mLayoutFlushObservers.Contains(shell))
            continue;
          NS_ADDREF(shell);
          mLayoutFlushObservers.RemoveElement(shell);
          shell->mReflowScheduled = false;
          shell->mSuppressInterruptibleReflows = false;
          shell->FlushPendingNotifications(ChangesToFlush(Flush_InterruptibleLayout,
                                                          false));
          NS_RELEASE(shell);
        }
      }
    }
  }

  




  ImageRequestParameters parms = {aNowTime};
  if (mRequests.Count()) {
    mRequests.EnumerateEntries(nsRefreshDriver::ImageRequestEnumerator, &parms);
  }
    
  for (uint32_t i = 0; i < mPresShellsToInvalidateIfHidden.Length(); i++) {
    mPresShellsToInvalidateIfHidden[i]->InvalidatePresShellIfHidden();
  }
  mPresShellsToInvalidateIfHidden.Clear();

  if (mViewManagerFlushIsPending) {
#ifdef DEBUG_INVALIDATIONS
    printf("Starting ProcessPendingUpdates\n");
#endif
#ifndef MOZ_WIDGET_GONK
    
    nsRefPtr<layers::LayerManager> mgr = mPresContext->GetPresShell()->GetLayerManager();
    if (mgr) {
      mgr->SetPaintStartTime(mMostRecentRefresh);
    }
#endif

    mViewManagerFlushIsPending = false;
    nsRefPtr<nsViewManager> vm = mPresContext->GetPresShell()->GetViewManager();
    vm->ProcessPendingUpdates();
#ifdef DEBUG_INVALIDATIONS
    printf("Ending ProcessPendingUpdates\n");
#endif
  }
}

PLDHashOperator
nsRefreshDriver::ImageRequestEnumerator(nsISupportsHashKey* aEntry,
                                        void* aUserArg)
{
  ImageRequestParameters* parms =
    static_cast<ImageRequestParameters*> (aUserArg);
  mozilla::TimeStamp mostRecentRefresh = parms->ts;
  imgIRequest* req = static_cast<imgIRequest*>(aEntry->GetKey());
  NS_ABORT_IF_FALSE(req, "Unable to retrieve the image request");
  nsCOMPtr<imgIContainer> image;
  if (NS_SUCCEEDED(req->GetImage(getter_AddRefs(image)))) {
    image->RequestRefresh(mostRecentRefresh);
  }

  return PL_DHASH_NEXT;
}

void
nsRefreshDriver::Freeze()
{
  NS_ASSERTION(!mFrozen, "Freeze called on already-frozen refresh driver");
  StopTimer();
  mFrozen = true;
}

void
nsRefreshDriver::Thaw()
{
  NS_ASSERTION(mFrozen, "Thaw called on an unfrozen refresh driver");
  mFrozen = false;
  if (ObserverCount() || ImageRequestCount()) {
    
    
    
    
    NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsRefreshDriver::DoRefresh));
    EnsureTimerStarted(false);
  }
}

void
nsRefreshDriver::SetThrottled(bool aThrottled)
{
  if (aThrottled != mThrottled) {
    mThrottled = aThrottled;
    if (mActiveTimer) {
      
      
      EnsureTimerStarted(true);
    }
  }
}

void
nsRefreshDriver::DoRefresh()
{
  
  if (!mFrozen && mPresContext && mActiveTimer) {
    DoTick();
  }
}

#ifdef DEBUG
bool
nsRefreshDriver::IsRefreshObserver(nsARefreshObserver* aObserver,
                                   mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.Contains(aObserver);
}
#endif

void
nsRefreshDriver::ScheduleViewManagerFlush()
{
  NS_ASSERTION(mPresContext->IsRoot(),
               "Should only schedule view manager flush on root prescontexts");
  mViewManagerFlushIsPending = true;
  EnsureTimerStarted(false);
}

void
nsRefreshDriver::ScheduleFrameRequestCallbacks(nsIDocument* aDocument)
{
  NS_ASSERTION(mFrameRequestCallbackDocs.IndexOf(aDocument) ==
               mFrameRequestCallbackDocs.NoIndex,
               "Don't schedule the same document multiple times");
  mFrameRequestCallbackDocs.AppendElement(aDocument);

  
  ConfigureHighPrecision();
  EnsureTimerStarted(false);
}

void
nsRefreshDriver::RevokeFrameRequestCallbacks(nsIDocument* aDocument)
{
  mFrameRequestCallbackDocs.RemoveElement(aDocument);
  ConfigureHighPrecision();
  
  
}
