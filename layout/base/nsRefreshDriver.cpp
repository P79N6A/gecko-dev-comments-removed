


















#ifdef XP_WIN
#include <windows.h>


#include <mmsystem.h>
#include "WinUtils.h"
#endif

#include "mozilla/ArrayUtils.h"
#include "mozilla/AutoRestore.h"
#include "nsHostObjectProtocolHandler.h"
#include "nsRefreshDriver.h"
#include "nsITimer.h"
#include "nsLayoutUtils.h"
#include "nsPresContext.h"
#include "nsComponentManagerUtils.h"
#include "prlog.h"
#include "nsAutoPtr.h"
#include "nsIDocument.h"
#include "jsapi.h"
#include "nsContentUtils.h"
#include "mozilla/PendingPlayerTracker.h"
#include "mozilla/Preferences.h"
#include "nsViewManager.h"
#include "GeckoProfiler.h"
#include "nsNPAPIPluginInstance.h"
#include "nsPerformance.h"
#include "mozilla/dom/WindowBinding.h"
#include "RestyleManager.h"
#include "Layers.h"
#include "imgIContainer.h"
#include "nsIFrameRequestCallback.h"
#include "mozilla/dom/ScriptSettings.h"
#include "nsDocShell.h"
#include "nsISimpleEnumerator.h"
#include "nsJSEnvironment.h"
#include "mozilla/Telemetry.h"
#include "gfxPrefs.h"
#include "BackgroundChild.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIIPCBackgroundChildCreateCallback.h"
#include "mozilla/layout/VsyncChild.h"
#include "VsyncSource.h"
#include "mozilla/VsyncDispatcher.h"
#include "nsThreadUtils.h"
#include "mozilla/unused.h"

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

using namespace mozilla;
using namespace mozilla::widget;
using namespace mozilla::ipc;
using namespace mozilla::layout;

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
  RefreshDriverTimer()
    : mLastFireEpoch(0)
  {
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

  TimeStamp MostRecentRefresh() const { return mLastFireTime; }
  int64_t MostRecentRefreshEpochTime() const { return mLastFireEpoch; }

  void SwapRefreshDrivers(RefreshDriverTimer* aNewTimer)
  {
    MOZ_ASSERT(NS_IsMainThread());

    for (size_t i = 0; i < mRefreshDrivers.Length(); i++) {
      aNewTimer->AddRefreshDriver(mRefreshDrivers[i]);
      mRefreshDrivers[i]->mActiveTimer = aNewTimer;
    }
    mRefreshDrivers.Clear();

    aNewTimer->mLastFireEpoch = mLastFireEpoch;
    aNewTimer->mLastFireTime = mLastFireTime;
  }

protected:
  virtual void StartTimer() = 0;
  virtual void StopTimer() = 0;
  virtual void ScheduleNextTick(TimeStamp aNowTime) = 0;

  



  void Tick()
  {
    int64_t jsnow = JS_Now();
    TimeStamp now = TimeStamp::Now();
    Tick(jsnow, now);
  }

  


  void Tick(int64_t jsnow, TimeStamp now)
  {
    ScheduleNextTick(now);

    mLastFireEpoch = jsnow;
    mLastFireTime = now;

    LOG("[%p] ticking drivers...", this);
    nsTArray<nsRefPtr<nsRefreshDriver> > drivers(mRefreshDrivers);
    
    profiler_tracing("Paint", "RD", TRACING_INTERVAL_START);
    for (size_t i = 0; i < drivers.Length(); ++i) {
      
      if (drivers[i]->IsTestControllingRefreshesEnabled()) {
        continue;
      }

      TickDriver(drivers[i], jsnow, now);
    }
    profiler_tracing("Paint", "RD", TRACING_INTERVAL_END);
    LOG("[%p] done.", this);
  }

  static void TickDriver(nsRefreshDriver* driver, int64_t jsnow, TimeStamp now)
  {
    LOG(">> TickDriver: %p (jsnow: %lld)", driver, jsnow);
    driver->Tick(jsnow, now);
  }

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
  


  explicit SimpleTimerBasedRefreshDriverTimer(double aRate)
  {
    SetRate(aRate);
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  }

  virtual ~SimpleTimerBasedRefreshDriverTimer()
  {
    StopTimer();
  }

  
  virtual void SetRate(double aNewRate)
  {
    mRateMilliseconds = aNewRate;
    mRateDuration = TimeDuration::FromMilliseconds(mRateMilliseconds);
  }

  double GetRate() const
  {
    return mRateMilliseconds;
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

  double mRateMilliseconds;
  TimeDuration mRateDuration;
  nsRefPtr<nsITimer> mTimer;
};






class VsyncRefreshDriverTimer : public RefreshDriverTimer
{
public:
  VsyncRefreshDriverTimer()
    : mVsyncChild(nullptr)
  {
    MOZ_ASSERT(XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    nsRefPtr<mozilla::gfx::VsyncSource> vsyncSource = gfxPlatform::GetPlatform()->GetHardwareVsync();
    MOZ_ALWAYS_TRUE(mVsyncDispatcher = vsyncSource->GetRefreshTimerVsyncDispatcher());
    mVsyncDispatcher->SetParentRefreshTimer(mVsyncObserver);
  }

  explicit VsyncRefreshDriverTimer(VsyncChild* aVsyncChild)
    : mVsyncChild(aVsyncChild)
  {
    MOZ_ASSERT(!XRE_IsParentProcess());
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mVsyncChild);
    mVsyncObserver = new RefreshDriverVsyncObserver(this);
    mVsyncChild->SetVsyncObserver(mVsyncObserver);
  }

private:
  
  
  
  
  
  class RefreshDriverVsyncObserver final : public VsyncObserver
  {
  public:
    explicit RefreshDriverVsyncObserver(VsyncRefreshDriverTimer* aVsyncRefreshDriverTimer)
      : mVsyncRefreshDriverTimer(aVsyncRefreshDriverTimer)
      , mRefreshTickLock("RefreshTickLock")
      , mProcessedVsync(true)
    {
      MOZ_ASSERT(NS_IsMainThread());
    }

    virtual bool NotifyVsync(TimeStamp aVsyncTimestamp) override
    {
      if (!NS_IsMainThread()) {
        MOZ_ASSERT(XRE_IsParentProcess());
        
        
        
        { 
          MonitorAutoLock lock(mRefreshTickLock);
          mRecentVsync = aVsyncTimestamp;
          if (!mProcessedVsync) {
            return true;
          }
          mProcessedVsync = false;
        }

        nsCOMPtr<nsIRunnable> vsyncEvent =
             NS_NewRunnableMethodWithArg<TimeStamp>(this,
                                                    &RefreshDriverVsyncObserver::TickRefreshDriver,
                                                    aVsyncTimestamp);
        NS_DispatchToMainThread(vsyncEvent);
      } else {
        TickRefreshDriver(aVsyncTimestamp);
      }

      return true;
    }

    void Shutdown()
    {
      MOZ_ASSERT(NS_IsMainThread());
      mVsyncRefreshDriverTimer = nullptr;
    }

  private:
    virtual ~RefreshDriverVsyncObserver() {}

    void TickRefreshDriver(TimeStamp aVsyncTimestamp)
    {
      MOZ_ASSERT(NS_IsMainThread());

      if (XRE_IsParentProcess()) {
        MonitorAutoLock lock(mRefreshTickLock);
        aVsyncTimestamp = mRecentVsync;
        mProcessedVsync = true;
      }

      
      
      
      if (mVsyncRefreshDriverTimer) {
        mVsyncRefreshDriverTimer->RunRefreshDrivers(aVsyncTimestamp);
      }
    }

    
    
    
    VsyncRefreshDriverTimer* mVsyncRefreshDriverTimer;
    Monitor mRefreshTickLock;
    TimeStamp mRecentVsync;
    bool mProcessedVsync;
  }; 

  virtual ~VsyncRefreshDriverTimer()
  {
    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(nullptr);
      mVsyncDispatcher = nullptr;
    } else {
      
      
      
      
      unused << mVsyncChild->SendUnobserve();
      mVsyncChild->SetVsyncObserver(nullptr);
      mVsyncChild = nullptr;
    }

    
    
    mVsyncObserver->Shutdown();
    mVsyncObserver = nullptr;
  }

  virtual void StartTimer() override
  {
    mLastFireEpoch = JS_Now();
    mLastFireTime = TimeStamp::Now();

    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(mVsyncObserver);
    } else {
      unused << mVsyncChild->SendObserve();
    }
  }

  virtual void StopTimer() override
  {
    if (XRE_IsParentProcess()) {
      mVsyncDispatcher->SetParentRefreshTimer(nullptr);
    } else {
      unused << mVsyncChild->SendUnobserve();
    }
  }

  virtual void ScheduleNextTick(TimeStamp aNowTime) override
  {
    
    
  }

  void RunRefreshDrivers(TimeStamp aTimeStamp)
  {
    int64_t jsnow = JS_Now();
    TimeDuration diff = TimeStamp::Now() - aTimeStamp;
    int64_t vsyncJsNow = jsnow - diff.ToMicroseconds();
    Tick(vsyncJsNow, aTimeStamp);
  }

  nsRefPtr<RefreshDriverVsyncObserver> mVsyncObserver;
  
  nsRefPtr<RefreshTimerVsyncDispatcher> mVsyncDispatcher;
  
  
  
  nsRefPtr<VsyncChild> mVsyncChild;
}; 










class PreciseRefreshDriverTimer :
    public SimpleTimerBasedRefreshDriverTimer
{
public:
  explicit PreciseRefreshDriverTimer(double aRate)
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
#ifndef ANDROID  
    Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_FRAME_DELAY_MS, (aNowTime - mTargetTime).ToMilliseconds());
#endif

    
    LOG("[%p] scheduling callback for %d ms (2)", this, delay);
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);

    mTargetTime = newTarget;
  }
};

#ifdef XP_WIN



class PreciseRefreshDriverTimerWindowsDwmVsync :
  public PreciseRefreshDriverTimer
{
public:
  
  static bool IsSupported()
  {
    return WinUtils::dwmGetCompositionTimingInfoPtr != nullptr;
  }

  PreciseRefreshDriverTimerWindowsDwmVsync(double aRate, bool aPreferHwTiming = false)
    : PreciseRefreshDriverTimer(aRate)
    , mPreferHwTiming(aPreferHwTiming)
  {
  }

protected:
  
  
  bool mPreferHwTiming;

  nsresult GetVBlankInfo(mozilla::TimeStamp &aLastVBlank, mozilla::TimeDuration &aInterval)
  {
    MOZ_ASSERT(WinUtils::dwmGetCompositionTimingInfoPtr,
               "DwmGetCompositionTimingInfoPtr is unavailable (windows vsync)");

    DWM_TIMING_INFO timingInfo;
    timingInfo.cbSize = sizeof(DWM_TIMING_INFO);
    HRESULT hr = WinUtils::dwmGetCompositionTimingInfoPtr(0, &timingInfo); 

    if (FAILED(hr)) {
      
      return NS_ERROR_NOT_INITIALIZED;
    }

    LARGE_INTEGER time, freq;
    ::QueryPerformanceCounter(&time);
    ::QueryPerformanceFrequency(&freq);
    aLastVBlank = TimeStamp::Now();
    double secondsPassed = double(time.QuadPart - timingInfo.qpcVBlank) / double(freq.QuadPart);

    aLastVBlank -= TimeDuration::FromSeconds(secondsPassed);
    aInterval = TimeDuration::FromSeconds(double(timingInfo.qpcRefreshPeriod) / double(freq.QuadPart));

    return NS_OK;
  }

  virtual void ScheduleNextTick(TimeStamp aNowTime)
  {
    static const TimeDuration kMinSaneInterval = TimeDuration::FromMilliseconds(3); 
    static const TimeDuration kMaxSaneInterval = TimeDuration::FromMilliseconds(44); 
    static const TimeDuration kNegativeMaxSaneInterval = TimeDuration::FromMilliseconds(-44); 
    TimeStamp lastVblank;
    TimeDuration vblankInterval;

    if (!mPreferHwTiming ||
        NS_OK != GetVBlankInfo(lastVblank, vblankInterval) ||
        vblankInterval > kMaxSaneInterval ||
        vblankInterval < kMinSaneInterval ||
        (aNowTime - lastVblank) > kMaxSaneInterval ||
        (aNowTime - lastVblank) < kNegativeMaxSaneInterval) {
      
      PreciseRefreshDriverTimer::ScheduleNextTick(aNowTime);
      return;
    }

    TimeStamp newTarget = lastVblank + vblankInterval; 

    
    
    
    
    
    
    

    const double kSameVsyncThreshold = 0.1;
    while (newTarget <= mTargetTime + vblankInterval.MultDouble(kSameVsyncThreshold)) {
      newTarget += vblankInterval;
    }

    
    
    
    static const double kDefaultPhaseShiftPercent = 10;
    static const double phaseShiftFactor = 0.01 *
      (Preferences::GetInt("layout.frame_rate.vsync.phasePercentage", kDefaultPhaseShiftPercent) % 100);

    double phaseDelay = 1.0 + vblankInterval.ToMilliseconds() * phaseShiftFactor;

    
    double delayMs = (newTarget - aNowTime).ToMilliseconds() + phaseDelay;

    
    uint32_t delay = static_cast<uint32_t>(delayMs < 0 ? 0 : delayMs);

    
    LOG("[%p] precise dwm-vsync timer last tick late by %f ms, next tick in %d ms",
        this,
        (aNowTime - mTargetTime).ToMilliseconds(),
        delay);
#ifndef ANDROID  
    Telemetry::Accumulate(Telemetry::FX_REFRESH_DRIVER_FRAME_DELAY_MS, (aNowTime - mTargetTime).ToMilliseconds());
#endif

    
    LOG("[%p] scheduling callback for %d ms (2)", this, delay);
    mTimer->InitWithFuncCallback(TimerTick, this, delay, nsITimer::TYPE_ONE_SHOT);

    mTargetTime = newTarget;
  }
};
#endif














class InactiveRefreshDriverTimer final :
    public SimpleTimerBasedRefreshDriverTimer
{
public:
  explicit InactiveRefreshDriverTimer(double aRate)
    : SimpleTimerBasedRefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(-1.0),
      mNextDriverIndex(0)
  {
  }

  InactiveRefreshDriverTimer(double aRate, double aDisableAfterMilliseconds)
    : SimpleTimerBasedRefreshDriverTimer(aRate),
      mNextTickDuration(aRate),
      mDisableAfterMilliseconds(aDisableAfterMilliseconds),
      mNextDriverIndex(0)
  {
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

  double mNextTickDuration;
  double mDisableAfterMilliseconds;
  uint32_t mNextDriverIndex;
};




class VsyncChildCreateCallback final : public nsIIPCBackgroundChildCreateCallback
{
  NS_DECL_ISUPPORTS

public:
  VsyncChildCreateCallback()
  {
    MOZ_ASSERT(NS_IsMainThread());
  }

  static void CreateVsyncActor(PBackgroundChild* aPBackgroundChild)
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aPBackgroundChild);

    layout::PVsyncChild* actor = aPBackgroundChild->SendPVsyncConstructor();
    layout::VsyncChild* child = static_cast<layout::VsyncChild*>(actor);
    nsRefreshDriver::PVsyncActorCreated(child);
  }

private:
  virtual ~VsyncChildCreateCallback() {}

  virtual void ActorCreated(PBackgroundChild* aPBackgroundChild) override
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(aPBackgroundChild);
    CreateVsyncActor(aPBackgroundChild);
  }

  virtual void ActorFailed() override
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_CRASH("Failed To Create VsyncChild Actor");
  }
}; 
NS_IMPL_ISUPPORTS(VsyncChildCreateCallback, nsIIPCBackgroundChildCreateCallback)

} 

static RefreshDriverTimer* sRegularRateTimer;
static InactiveRefreshDriverTimer* sThrottledRateTimer;

#ifdef XP_WIN
static int32_t sHighPrecisionTimerRequests = 0;

static nsITimer *sDisableHighPrecisionTimersTimer = nullptr;
#endif

static void
CreateContentVsyncRefreshTimer(void*)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!XRE_IsParentProcess());

  
  
  
  
  
  
  
  PBackgroundChild* backgroundChild = BackgroundChild::GetForCurrentThread();
  if (backgroundChild) {
    
    
    VsyncChildCreateCallback::CreateVsyncActor(backgroundChild);
    return;
  }
  
  nsRefPtr<nsIIPCBackgroundChildCreateCallback> callback = new VsyncChildCreateCallback();
  if (NS_WARN_IF(!BackgroundChild::GetOrCreateForCurrentThread(callback))) {
    MOZ_CRASH("PVsync actor create failed!");
  }
}

static void
CreateVsyncRefreshTimer()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  gfxPrefs::GetSingleton();

  if (!gfxPrefs::VsyncAlignedRefreshDriver()
        || !gfxPrefs::HardwareVsyncEnabled()
        || gfxPlatform::IsInLayoutAsapMode()) {
    return;
  }

  NS_WARNING("Enabling vsync refresh driver\n");

  if (XRE_IsParentProcess()) {
    
    gfxPlatform::GetPlatform();
    
    
    sRegularRateTimer = new VsyncRefreshDriverTimer();
    return;
  }

#ifdef MOZ_NUWA_PROCESS
  
  
  
  if (IsNuwaProcess()) {
    NuwaAddFinalConstructor(&CreateContentVsyncRefreshTimer, nullptr);
    return;
  }
#endif
  
  CreateContentVsyncRefreshTimer(nullptr);
}

static uint32_t
GetFirstFrameDelay(imgIRequest* req)
{
  nsCOMPtr<imgIContainer> container;
  if (NS_FAILED(req->GetImage(getter_AddRefs(container))) || !container) {
    return 0;
  }

  
  int32_t delay = container->GetFirstFrameDelay();
  if (delay < 0)
    return 0;

  return static_cast<uint32_t>(delay);
}

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
nsRefreshDriver::GetRegularTimerInterval(bool *outIsDefault) const
{
  int32_t rate = Preferences::GetInt("layout.frame_rate", -1);
  if (rate < 0) {
    rate = DEFAULT_FRAME_RATE;
    if (outIsDefault) {
      *outIsDefault = true;
    }
  } else {
    if (outIsDefault) {
      *outIsDefault = false;
    }
  }

  if (rate == 0) {
    rate = 10000;
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

  if (!sRegularRateTimer) {
    bool isDefault = true;
    double rate = GetRegularTimerInterval(&isDefault);

    
    CreateVsyncRefreshTimer();

#ifdef XP_WIN
    if (!sRegularRateTimer && PreciseRefreshDriverTimerWindowsDwmVsync::IsSupported()) {
      sRegularRateTimer = new PreciseRefreshDriverTimerWindowsDwmVsync(rate, isDefault);
    }
#endif
    if (!sRegularRateTimer) {
      sRegularRateTimer = new PreciseRefreshDriverTimer(rate);
    }
  }
  return sRegularRateTimer;
}

nsRefreshDriver::nsRefreshDriver(nsPresContext* aPresContext)
  : mActiveTimer(nullptr),
    mReflowCause(nullptr),
    mStyleCause(nullptr),
    mPresContext(aPresContext),
    mRootRefresh(nullptr),
    mPendingTransaction(0),
    mCompletedTransaction(0),
    mFreezeCount(0),
    mThrottled(false),
    mTestControllingRefreshes(false),
    mViewManagerFlushIsPending(false),
    mRequestedHighPrecision(false),
    mInRefresh(false),
    mWaitingForTransaction(false),
    mSkippedPaints(false)
{
  mMostRecentRefreshEpochTime = JS_Now();
  mMostRecentRefresh = TimeStamp::Now();
  mMostRecentTick = mMostRecentRefresh;
}

nsRefreshDriver::~nsRefreshDriver()
{
  MOZ_ASSERT(ObserverCount() == 0,
             "observers should have unregistered");
  MOZ_ASSERT(!mActiveTimer, "timer should be gone");
  
  if (mRootRefresh) {
    mRootRefresh->RemoveRefreshObserver(this, Flush_Style);
    mRootRefresh = nullptr;
  }
  for (uint32_t i = 0; i < mPresShellsToInvalidateIfHidden.Length(); i++) {
    mPresShellsToInvalidateIfHidden[i]->InvalidatePresShellIfHidden();
  }
  mPresShellsToInvalidateIfHidden.Clear();

  profiler_free_backtrace(mStyleCause);
  profiler_free_backtrace(mReflowCause);
}



void
nsRefreshDriver::AdvanceTimeAndRefresh(int64_t aMilliseconds)
{
  
  StopTimer();

  if (!mTestControllingRefreshes) {
    mMostRecentRefreshEpochTime = JS_Now();
    mMostRecentRefresh = TimeStamp::Now();

    mTestControllingRefreshes = true;
    if (mWaitingForTransaction) {
      
      mWaitingForTransaction = false;
      mSkippedPaints = false;
    }
  }

  mMostRecentRefreshEpochTime += aMilliseconds * 1000;
  mMostRecentRefresh += TimeDuration::FromMilliseconds((double) aMilliseconds);

  mozilla::dom::AutoNoJSAPI nojsapi;
  DoTick();
}

void
nsRefreshDriver::RestoreNormalRefresh()
{
  mTestControllingRefreshes = false;
  EnsureTimerStarted(eAllowTimeToGoBackwards);
  mCompletedTransaction = mPendingTransaction;
}

TimeStamp
nsRefreshDriver::MostRecentRefresh() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted();

  return mMostRecentRefresh;
}

int64_t
nsRefreshDriver::MostRecentRefreshEpochTime() const
{
  const_cast<nsRefreshDriver*>(this)->EnsureTimerStarted();

  return mMostRecentRefreshEpochTime;
}

bool
nsRefreshDriver::AddRefreshObserver(nsARefreshObserver* aObserver,
                                    mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  bool success = array.AppendElement(aObserver) != nullptr;
  EnsureTimerStarted();
  return success;
}

bool
nsRefreshDriver::RemoveRefreshObserver(nsARefreshObserver* aObserver,
                                       mozFlushType aFlushType)
{
  ObserverArray& array = ArrayFor(aFlushType);
  return array.RemoveElement(aObserver);
}

void
nsRefreshDriver::AddPostRefreshObserver(nsAPostRefreshObserver* aObserver)
{
  mPostRefreshObservers.AppendElement(aObserver);
}

void
nsRefreshDriver::RemovePostRefreshObserver(nsAPostRefreshObserver* aObserver)
{
  mPostRefreshObservers.RemoveElement(aObserver);
}

bool
nsRefreshDriver::AddImageRequest(imgIRequest* aRequest)
{
  uint32_t delay = GetFirstFrameDelay(aRequest);
  if (delay == 0) {
    if (!mRequests.PutEntry(aRequest)) {
      return false;
    }
  } else {
    ImageStartData* start = mStartTable.Get(delay);
    if (!start) {
      start = new ImageStartData();
      mStartTable.Put(delay, start);
    }
    start->mEntries.PutEntry(aRequest);
  }

  EnsureTimerStarted();

  return true;
}

void
nsRefreshDriver::RemoveImageRequest(imgIRequest* aRequest)
{
  
  
  mRequests.RemoveEntry(aRequest);
  uint32_t delay = GetFirstFrameDelay(aRequest);
  if (delay != 0) {
    ImageStartData* start = mStartTable.Get(delay);
    if (start) {
      start->mEntries.RemoveEntry(aRequest);
    }
  }
}

void
nsRefreshDriver::EnsureTimerStarted(EnsureTimerStartedFlags aFlags)
{
  if (mTestControllingRefreshes)
    return;

  
  if (mActiveTimer && !(aFlags & eAdjustingTimer))
    return;

  if (IsFrozen() || !mPresContext) {
    
    StopTimer();
    return;
  }

  if (mPresContext->Document()->IsBeingUsedAsImage()) {
    
    
    
    nsIURI* uri = mPresContext->Document()->GetDocumentURI();
    if (!uri || !IsFontTableURI(uri)) {
      MOZ_ASSERT(!mActiveTimer,
                 "image doc refresh driver should never have its own timer");
      return;
    }
  }

  
  
  
  RefreshDriverTimer *newTimer = ChooseTimer();
  if (newTimer != mActiveTimer) {
    if (mActiveTimer)
      mActiveTimer->RemoveRefreshDriver(this);
    mActiveTimer = newTimer;
    mActiveTimer->AddRefreshDriver(this);
  }

  
  
  
  
  
  
  
  
  mMostRecentRefresh =
    aFlags & eAllowTimeToGoBackwards
    ? mActiveTimer->MostRecentRefresh()
    : std::max(mActiveTimer->MostRecentRefresh(), mMostRecentRefresh);
  mMostRecentRefreshEpochTime =
    aFlags & eAllowTimeToGoBackwards
    ? mActiveTimer->MostRecentRefreshEpochTime()
    : std::max(mActiveTimer->MostRecentRefreshEpochTime(),
               mMostRecentRefreshEpochTime);
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

 PLDHashOperator
nsRefreshDriver::StartTableRequestCounter(const uint32_t& aKey,
                                          ImageStartData* aEntry,
                                          void* aUserArg)
{
  uint32_t *count = static_cast<uint32_t*>(aUserArg);
  *count += aEntry->mEntries.Count();

  return PL_DHASH_NEXT;
}

uint32_t
nsRefreshDriver::ImageRequestCount() const
{
  uint32_t count = 0;
  mStartTable.EnumerateRead(nsRefreshDriver::StartTableRequestCounter, &count);
  return count + mRequests.Count();
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
      MOZ_ASSERT(false, "bad flush type");
      return *static_cast<ObserverArray*>(nullptr);
  }
}





void
nsRefreshDriver::DoTick()
{
  NS_PRECONDITION(!IsFrozen(), "Why are we notified while frozen?");
  NS_PRECONDITION(mPresContext, "Why are we notified after disconnection?");
  NS_PRECONDITION(!nsContentUtils::GetCurrentJSContext(),
                  "Shouldn't have a JSContext on the stack");

  if (mTestControllingRefreshes) {
    Tick(mMostRecentRefreshEpochTime, mMostRecentRefresh);
  } else {
    Tick(JS_Now(), TimeStamp::Now());
  }
}

struct DocumentFrameCallbacks {
  explicit DocumentFrameCallbacks(nsIDocument* aDocument) :
    mDocument(aDocument)
  {}

  nsCOMPtr<nsIDocument> mDocument;
  nsIDocument::FrameRequestCallbackList mCallbacks;
};

static nsDocShell* GetDocShell(nsPresContext* aPresContext)
{
  return static_cast<nsDocShell*>(aPresContext->GetDocShell());
}

static bool
HasPendingAnimations(nsIPresShell* aShell)
{
  nsIDocument* doc = aShell->GetDocument();
  if (!doc) {
    return false;
  }

  PendingPlayerTracker* tracker = doc->GetPendingPlayerTracker();
  return tracker && tracker->HasPendingPlayers();
}





static void GetProfileTimelineSubDocShells(nsDocShell* aRootDocShell,
                                           nsTArray<nsDocShell*>& aShells)
{
  if (!aRootDocShell || nsDocShell::gProfileTimelineRecordingsCount == 0) {
    return;
  }

  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = aRootDocShell->GetDocShellEnumerator(nsIDocShellTreeItem::typeAll,
    nsIDocShell::ENUMERATE_BACKWARDS, getter_AddRefs(enumerator));

  if (NS_FAILED(rv)) {
    return;
  }

  nsCOMPtr<nsIDocShell> curItem;
  bool hasMore = false;
  while (NS_SUCCEEDED(enumerator->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> curSupports;
    enumerator->GetNext(getter_AddRefs(curSupports));
    curItem = do_QueryInterface(curSupports);

    if (!curItem || !curItem->GetRecordProfileTimelineMarkers()) {
      continue;
    }

    nsDocShell* shell = static_cast<nsDocShell*>(curItem.get());
    bool isVisible = false;
    shell->GetVisibility(&isVisible);
    if (!isVisible) {
      continue;
    }

    aShells.AppendElement(shell);
  };
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

  PROFILER_LABEL("nsRefreshDriver", "Tick",
    js::ProfileEntry::Category::GRAPHICS);

  
  
  
  if (IsFrozen() || !mPresContext) {
    return;
  }

  
  
  
  
  
  
  if ((aNowTime <= mMostRecentRefresh) && !mTestControllingRefreshes) {
    return;
  }

  TimeStamp previousRefresh = mMostRecentRefresh;

  mMostRecentRefresh = aNowTime;
  mMostRecentRefreshEpochTime = aNowEpoch;

  if (IsWaitingForPaint(aNowTime)) {
    
    
    
    return;
  }
  mMostRecentTick = aNowTime;
  if (mRootRefresh) {
    mRootRefresh->RemoveRefreshObserver(this, Flush_Style);
    mRootRefresh = nullptr;
  }
  mSkippedPaints = false;

  nsCOMPtr<nsIPresShell> presShell = mPresContext->GetPresShell();
  if (!presShell || (ObserverCount() == 0 && ImageRequestCount() == 0)) {
    
    
    
    
    
    
    
    StopTimer();
    return;
  }

  AutoRestore<bool> restoreInRefresh(mInRefresh);
  mInRefresh = true;

  AutoRestore<TimeStamp> restoreTickStart(mTickStart);
  mTickStart = TimeStamp::Now();

  





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
      

      
      nsTArray<DocumentFrameCallbacks>
        frameRequestCallbacks(mFrameRequestCallbackDocs.Length());
      for (uint32_t i = 0; i < mFrameRequestCallbackDocs.Length(); ++i) {
        frameRequestCallbacks.AppendElement(mFrameRequestCallbackDocs[i]);
        mFrameRequestCallbackDocs[i]->
          TakeFrameRequestCallbacks(frameRequestCallbacks.LastElement().mCallbacks);
      }
      
      
      mFrameRequestCallbackDocs.Clear();

      profiler_tracing("Paint", "Scripts", TRACING_INTERVAL_START);
      int64_t eventTime = aNowEpoch / PR_USEC_PER_MSEC;
      for (uint32_t i = 0; i < frameRequestCallbacks.Length(); ++i) {
        const DocumentFrameCallbacks& docCallbacks = frameRequestCallbacks[i];
        
        
        nsPIDOMWindow* innerWindow = docCallbacks.mDocument->GetInnerWindow();
        DOMHighResTimeStamp timeStamp = 0;
        if (innerWindow && innerWindow->IsInnerWindow()) {
          nsPerformance* perf = innerWindow->GetPerformance();
          if (perf) {
            timeStamp = perf->GetDOMTiming()->TimeStampToDOMHighRes(aNowTime);
          }
          
        }
        for (uint32_t j = 0; j < docCallbacks.mCallbacks.Length(); ++j) {
          const nsIDocument::FrameRequestCallbackHolder& holder =
            docCallbacks.mCallbacks[j];
          nsAutoMicroTask mt;
          if (holder.HasWebIDLCallback()) {
            ErrorResult ignored;
            holder.GetWebIDLCallback()->Call(timeStamp, ignored);
          } else {
            holder.GetXPCOMCallback()->Sample(eventTime);
          }
        }
      }
      profiler_tracing("Paint", "Scripts", TRACING_INTERVAL_END);

      if (mPresContext && mPresContext->GetPresShell()) {
        bool tracingStyleFlush = false;
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mStyleFlushObservers);
        for (uint32_t j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mStyleFlushObservers.Contains(shell))
            continue;

          nsRefPtr<nsDocShell> docShell = GetDocShell(shell->GetPresContext());
          if (docShell) {
            docShell->AddProfileTimelineMarker("Styles", TRACING_INTERVAL_START);
          }

          if (!tracingStyleFlush) {
            tracingStyleFlush = true;
            profiler_tracing("Paint", "Styles", mStyleCause, TRACING_INTERVAL_START);
            mStyleCause = nullptr;
          }

          NS_ADDREF(shell);
          mStyleFlushObservers.RemoveElement(shell);
          shell->GetPresContext()->RestyleManager()->mObservingRefreshDriver = false;
          shell->FlushPendingNotifications(ChangesToFlush(Flush_Style, false));
          
          
          
          nsPresContext* presContext = shell->GetPresContext();
          if (presContext) {
            presContext->NotifyFontFaceSetOnRefresh();
          }
          NS_RELEASE(shell);

          if (docShell) {
            docShell->AddProfileTimelineMarker("Styles", TRACING_INTERVAL_END);
          }
        }

        if (tracingStyleFlush) {
          profiler_tracing("Paint", "Styles", TRACING_INTERVAL_END);
        }
      }

      if (!nsLayoutUtils::AreAsyncAnimationsEnabled()) {
        mPresContext->TickLastStyleUpdateForAllAnimations();
      }
    } else if  (i == 1) {
      
      if (mPresContext && mPresContext->GetPresShell()) {
        bool tracingLayoutFlush = false;
        nsAutoTArray<nsIPresShell*, 16> observers;
        observers.AppendElements(mLayoutFlushObservers);
        for (uint32_t j = observers.Length();
             j && mPresContext && mPresContext->GetPresShell(); --j) {
          
          
          nsIPresShell* shell = observers[j - 1];
          if (!mLayoutFlushObservers.Contains(shell))
            continue;

          if (!tracingLayoutFlush) {
            tracingLayoutFlush = true;
            profiler_tracing("Paint", "Reflow", mReflowCause, TRACING_INTERVAL_START);
            mReflowCause = nullptr;
          }

          NS_ADDREF(shell);
          mLayoutFlushObservers.RemoveElement(shell);
          shell->mReflowScheduled = false;
          shell->mSuppressInterruptibleReflows = false;
          mozFlushType flushType = HasPendingAnimations(shell)
                                 ? Flush_Layout
                                 : Flush_InterruptibleLayout;
          shell->FlushPendingNotifications(ChangesToFlush(flushType, false));
          
          
          nsPresContext* presContext = shell->GetPresContext();
          if (presContext) {
            presContext->NotifyFontFaceSetOnRefresh();
          }
          NS_RELEASE(shell);
        }

        if (tracingLayoutFlush) {
          profiler_tracing("Paint", "Reflow", TRACING_INTERVAL_END);
        }
      }
    }
  }

  




  ImageRequestParameters parms = {aNowTime, previousRefresh, &mRequests};

  mStartTable.EnumerateRead(nsRefreshDriver::StartTableRefresh, &parms);

  if (mRequests.Count()) {
    
    
    
    
    nsCOMArray<imgIContainer> imagesToRefresh(mRequests.Count());
    mRequests.EnumerateEntries(nsRefreshDriver::ImageRequestEnumerator,
                               &imagesToRefresh);

    for (uint32_t i = 0; i < imagesToRefresh.Length(); i++) {
      imagesToRefresh[i]->RequestRefresh(aNowTime);
    }
  }

  for (uint32_t i = 0; i < mPresShellsToInvalidateIfHidden.Length(); i++) {
    mPresShellsToInvalidateIfHidden[i]->InvalidatePresShellIfHidden();
  }
  mPresShellsToInvalidateIfHidden.Clear();

  if (mViewManagerFlushIsPending) {
    nsTArray<nsDocShell*> profilingDocShells;
    GetProfileTimelineSubDocShells(GetDocShell(mPresContext), profilingDocShells);
    for (uint32_t i = 0; i < profilingDocShells.Length(); i ++) {
      
      
      profilingDocShells[i]->AddProfileTimelineMarker("Paint",
                                                      TRACING_INTERVAL_START);
    }
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Starting ProcessPendingUpdates\n");
    }
#endif

    mViewManagerFlushIsPending = false;
    nsRefPtr<nsViewManager> vm = mPresContext->GetPresShell()->GetViewManager();
    vm->ProcessPendingUpdates();
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Ending ProcessPendingUpdates\n");
    }
#endif
    for (uint32_t i = 0; i < profilingDocShells.Length(); i ++) {
      profilingDocShells[i]->AddProfileTimelineMarker("Paint",
                                                      TRACING_INTERVAL_END);
    }

    if (nsContentUtils::XPConnect()) {
      nsContentUtils::XPConnect()->NotifyDidPaint();
      nsJSContext::NotifyDidPaint();
    }
  }

#ifndef ANDROID  
  mozilla::Telemetry::AccumulateTimeDelta(mozilla::Telemetry::REFRESH_DRIVER_TICK, mTickStart);
#endif

  for (uint32_t i = 0; i < mPostRefreshObservers.Length(); ++i) {
    mPostRefreshObservers[i]->DidRefresh();
  }

  NS_ASSERTION(mInRefresh, "Still in refresh");
}

 PLDHashOperator
nsRefreshDriver::ImageRequestEnumerator(nsISupportsHashKey* aEntry,
                                        void* aUserArg)
{
  nsCOMArray<imgIContainer>* imagesToRefresh =
    static_cast<nsCOMArray<imgIContainer>*> (aUserArg);
  imgIRequest* req = static_cast<imgIRequest*>(aEntry->GetKey());
  MOZ_ASSERT(req, "Unable to retrieve the image request");
  nsCOMPtr<imgIContainer> image;
  if (NS_SUCCEEDED(req->GetImage(getter_AddRefs(image)))) {
    imagesToRefresh->AppendElement(image);
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
nsRefreshDriver::BeginRefreshingImages(nsISupportsHashKey* aEntry,
                                       void* aUserArg)
{
  ImageRequestParameters* parms =
    static_cast<ImageRequestParameters*> (aUserArg);

  imgIRequest* req = static_cast<imgIRequest*>(aEntry->GetKey());
  MOZ_ASSERT(req, "Unable to retrieve the image request");

  parms->mRequests->PutEntry(req);

  nsCOMPtr<imgIContainer> image;
  if (NS_SUCCEEDED(req->GetImage(getter_AddRefs(image)))) {
    image->SetAnimationStartTime(parms->mDesired);
  }

  return PL_DHASH_REMOVE;
}

 PLDHashOperator
nsRefreshDriver::StartTableRefresh(const uint32_t& aDelay,
                                   ImageStartData* aData,
                                   void* aUserArg)
{
  ImageRequestParameters* parms =
    static_cast<ImageRequestParameters*> (aUserArg);

  if (aData->mStartTime) {
    TimeStamp& start = *aData->mStartTime;
    TimeDuration prev = parms->mPrevious - start;
    TimeDuration curr = parms->mCurrent - start;
    uint32_t prevMultiple = static_cast<uint32_t>(prev.ToMilliseconds()) / aDelay;

    
    
    
    
    if (prevMultiple != static_cast<uint32_t>(curr.ToMilliseconds()) / aDelay) {
      parms->mDesired = start + TimeDuration::FromMilliseconds(prevMultiple * aDelay);
      aData->mEntries.EnumerateEntries(nsRefreshDriver::BeginRefreshingImages, parms);
    }
  } else {
    
    
    
    parms->mDesired = parms->mCurrent;
    aData->mEntries.EnumerateEntries(nsRefreshDriver::BeginRefreshingImages, parms);
    aData->mStartTime.emplace(parms->mCurrent);
  }

  return PL_DHASH_NEXT;
}

void
nsRefreshDriver::Freeze()
{
  StopTimer();
  mFreezeCount++;
}

void
nsRefreshDriver::Thaw()
{
  NS_ASSERTION(mFreezeCount > 0, "Thaw() called on an unfrozen refresh driver");

  if (mFreezeCount > 0) {
    mFreezeCount--;
  }

  if (mFreezeCount == 0) {
    if (ObserverCount() || ImageRequestCount()) {
      
      
      
      
      NS_DispatchToCurrentThread(NS_NewRunnableMethod(this, &nsRefreshDriver::DoRefresh));
      EnsureTimerStarted();
    }
  }
}

void
nsRefreshDriver::FinishedWaitingForTransaction()
{
  mWaitingForTransaction = false;
  if (mSkippedPaints &&
      !IsInRefresh() &&
      (ObserverCount() || ImageRequestCount())) {
    profiler_tracing("Paint", "RD", TRACING_INTERVAL_START);
    DoRefresh();
    profiler_tracing("Paint", "RD", TRACING_INTERVAL_END);
  }
  mSkippedPaints = false;
}

uint64_t
nsRefreshDriver::GetTransactionId()
{
  ++mPendingTransaction;

  if (mPendingTransaction >= mCompletedTransaction + 2 &&
      !mWaitingForTransaction &&
      !mTestControllingRefreshes) {
    mWaitingForTransaction = true;
    mSkippedPaints = false;
  }

  return mPendingTransaction;
}

void
nsRefreshDriver::RevokeTransactionId(uint64_t aTransactionId)
{
  MOZ_ASSERT(aTransactionId == mPendingTransaction);
  if (mPendingTransaction == mCompletedTransaction + 2 &&
      mWaitingForTransaction) {
    MOZ_ASSERT(!mSkippedPaints, "How did we skip a paint when we're in the middle of one?");
    FinishedWaitingForTransaction();
  }
  mPendingTransaction--;
}

mozilla::TimeStamp
nsRefreshDriver::GetTransactionStart()
{
  return mTickStart;
}

void
nsRefreshDriver::NotifyTransactionCompleted(uint64_t aTransactionId)
{
  if (aTransactionId > mCompletedTransaction) {
    if (mPendingTransaction > mCompletedTransaction + 1 &&
        mWaitingForTransaction) {
      mCompletedTransaction = aTransactionId;
      FinishedWaitingForTransaction();
    } else {
      mCompletedTransaction = aTransactionId;
    }
  }
}

void
nsRefreshDriver::WillRefresh(mozilla::TimeStamp aTime)
{
  mRootRefresh->RemoveRefreshObserver(this, Flush_Style);
  mRootRefresh = nullptr;
  if (mSkippedPaints) {
    DoRefresh();
  }
}

bool
nsRefreshDriver::IsWaitingForPaint(mozilla::TimeStamp aTime)
{
  if (mTestControllingRefreshes) {
    return false;
  }
  
  
  
  if (aTime > (mMostRecentTick + TimeDuration::FromMilliseconds(200))) {
    mSkippedPaints = false;
    mWaitingForTransaction = false;
    if (mRootRefresh) {
      mRootRefresh->RemoveRefreshObserver(this, Flush_Style);
    }
    return false;
  }
  if (mWaitingForTransaction) {
    mSkippedPaints = true;
    return true;
  }

  
  
  nsPresContext *displayRoot = PresContext()->GetDisplayRootPresContext();
  if (displayRoot) {
    nsRefreshDriver *rootRefresh = displayRoot->GetRootPresContext()->RefreshDriver();
    if (rootRefresh && rootRefresh != this) {
      if (rootRefresh->IsWaitingForPaint(aTime)) {
        if (mRootRefresh != rootRefresh) {
          if (mRootRefresh) {
            mRootRefresh->RemoveRefreshObserver(this, Flush_Style);
          }
          rootRefresh->AddRefreshObserver(this, Flush_Style);
          mRootRefresh = rootRefresh;
        }
        mSkippedPaints = true;
        return true;
      }
    }
  }
  return false;
}

void
nsRefreshDriver::SetThrottled(bool aThrottled)
{
  if (aThrottled != mThrottled) {
    mThrottled = aThrottled;
    if (mActiveTimer) {
      
      
      EnsureTimerStarted(eAdjustingTimer);
    }
  }
}

 void
nsRefreshDriver::PVsyncActorCreated(VsyncChild* aVsyncChild)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!XRE_IsParentProcess());
  VsyncRefreshDriverTimer* vsyncRefreshDriverTimer =
                           new VsyncRefreshDriverTimer(aVsyncChild);

  
  
  if (sRegularRateTimer) {
    sRegularRateTimer->SwapRefreshDrivers(vsyncRefreshDriverTimer);
    delete sRegularRateTimer;
  }
  sRegularRateTimer = vsyncRefreshDriverTimer;
}

void
nsRefreshDriver::DoRefresh()
{
  
  if (!IsFrozen() && mPresContext && mActiveTimer) {
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
  EnsureTimerStarted();
}

void
nsRefreshDriver::ScheduleFrameRequestCallbacks(nsIDocument* aDocument)
{
  NS_ASSERTION(mFrameRequestCallbackDocs.IndexOf(aDocument) ==
               mFrameRequestCallbackDocs.NoIndex,
               "Don't schedule the same document multiple times");
  mFrameRequestCallbackDocs.AppendElement(aDocument);

  
  ConfigureHighPrecision();
  EnsureTimerStarted();
}

void
nsRefreshDriver::RevokeFrameRequestCallbacks(nsIDocument* aDocument)
{
  mFrameRequestCallbackDocs.RemoveElement(aDocument);
  ConfigureHighPrecision();
  
  
}

#undef LOG
