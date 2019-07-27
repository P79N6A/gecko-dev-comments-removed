





#include "nsTimerImpl.h"
#include "TimerThread.h"
#include "nsAutoPtr.h"
#include "nsThreadManager.h"
#include "nsThreadUtils.h"
#include "plarena.h"
#include "pratom.h"
#include "GeckoProfiler.h"
#include "mozilla/Atomics.h"
#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

using mozilla::Atomic;
using mozilla::TimeDuration;
using mozilla::TimeStamp;

static Atomic<int32_t>  gGenerator;
static TimerThread*     gThread = nullptr;

#ifdef DEBUG_TIMERS

PRLogModuleInfo*
GetTimerLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsTimerImpl");
  }
  return sLog;
}

#include <math.h>

double nsTimerImpl::sDeltaSumSquared = 0;
double nsTimerImpl::sDeltaSum = 0;
double nsTimerImpl::sDeltaNum = 0;

static void
myNS_MeanAndStdDev(double n, double sumOfValues, double sumOfSquaredValues,
                   double* meanResult, double* stdDevResult)
{
  double mean = 0.0, var = 0.0, stdDev = 0.0;
  if (n > 0.0 && sumOfValues >= 0) {
    mean = sumOfValues / n;
    double temp = (n * sumOfSquaredValues) - (sumOfValues * sumOfValues);
    if (temp < 0.0 || n <= 1) {
      var = 0.0;
    } else {
      var = temp / (n * (n - 1));
    }
    
    stdDev = var != 0.0 ? sqrt(var) : 0.0;
  }
  *meanResult = mean;
  *stdDevResult = stdDev;
}
#endif

namespace {















class TimerEventAllocator
{
private:
  struct FreeEntry
  {
    FreeEntry* mNext;
  };

  PLArenaPool mPool;
  FreeEntry* mFirstFree;
  mozilla::Monitor mMonitor;

public:
  TimerEventAllocator()
    : mFirstFree(nullptr)
    , mMonitor("TimerEventAllocator")
  {
    PL_InitArenaPool(&mPool, "TimerEventPool", 4096,  0);
  }

  ~TimerEventAllocator()
  {
    PL_FinishArenaPool(&mPool);
  }

  void* Alloc(size_t aSize);
  void Free(void* aPtr);
};

} 

class nsTimerEvent : public nsRunnable
{
public:
  NS_IMETHOD Run();

  nsTimerEvent()
    : mTimer()
    , mGeneration(0)
  {
    MOZ_COUNT_CTOR(nsTimerEvent);

    MOZ_ASSERT(gThread->IsOnTimerThread(),
               "nsTimer must always be allocated on the timer thread");

    sAllocatorUsers++;
  }

#ifdef DEBUG_TIMERS
  TimeStamp mInitTime;
#endif

  static void Init();
  static void Shutdown();
  static void DeleteAllocatorIfNeeded();

  static void* operator new(size_t aSize) CPP_THROW_NEW
  {
    return sAllocator->Alloc(aSize);
  }
  void operator delete(void* aPtr)
  {
    sAllocator->Free(aPtr);
    DeleteAllocatorIfNeeded();
  }

  already_AddRefed<nsTimerImpl> ForgetTimer()
  {
    return mTimer.forget();
  }

  void SetTimer(already_AddRefed<nsTimerImpl> aTimer)
  {
    mTimer = aTimer;
    mGeneration = mTimer->GetGeneration();
  }

private:
  ~nsTimerEvent()
  {
    MOZ_COUNT_DTOR(nsTimerEvent);

    MOZ_ASSERT(!sCanDeleteAllocator || sAllocatorUsers > 0,
               "This will result in us attempting to deallocate the nsTimerEvent allocator twice");
    sAllocatorUsers--;
  }

  nsRefPtr<nsTimerImpl> mTimer;
  int32_t      mGeneration;

  static TimerEventAllocator* sAllocator;
  static Atomic<int32_t> sAllocatorUsers;
  static bool sCanDeleteAllocator;
};

TimerEventAllocator* nsTimerEvent::sAllocator = nullptr;
Atomic<int32_t> nsTimerEvent::sAllocatorUsers;
bool nsTimerEvent::sCanDeleteAllocator = false;

namespace {

void*
TimerEventAllocator::Alloc(size_t aSize)
{
  MOZ_ASSERT(aSize == sizeof(nsTimerEvent));

  mozilla::MonitorAutoLock lock(mMonitor);

  void* p;
  if (mFirstFree) {
    p = mFirstFree;
    mFirstFree = mFirstFree->mNext;
  } else {
    PL_ARENA_ALLOCATE(p, &mPool, aSize);
    if (!p) {
      return nullptr;
    }
  }

  return p;
}

void
TimerEventAllocator::Free(void* aPtr)
{
  mozilla::MonitorAutoLock lock(mMonitor);

  FreeEntry* entry = reinterpret_cast<FreeEntry*>(aPtr);

  entry->mNext = mFirstFree;
  mFirstFree = entry;
}

} 

NS_IMPL_QUERY_INTERFACE(nsTimerImpl, nsITimer)
NS_IMPL_ADDREF(nsTimerImpl)

NS_IMETHODIMP_(MozExternalRefCountType)
nsTimerImpl::Release(void)
{
  nsrefcnt count;

  MOZ_ASSERT(int32_t(mRefCnt) > 0, "dup release");
  count = --mRefCnt;
  NS_LOG_RELEASE(this, count, "nsTimerImpl");
  if (count == 0) {
    mRefCnt = 1; 

    
    
    delete this;
    return 0;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (count == 1 && mArmed) {
    mCanceled = true;

    MOZ_ASSERT(gThread, "Armed timer exists after the thread timer stopped.");
    if (NS_SUCCEEDED(gThread->RemoveTimer(this))) {
      return 0;
    }
  }

  return count;
}

nsTimerImpl::nsTimerImpl() :
  mClosure(nullptr),
  mCallbackType(CALLBACK_TYPE_UNKNOWN),
  mFiring(false),
  mArmed(false),
  mCanceled(false),
  mGeneration(0),
  mDelay(0)
{
  
  mEventTarget = static_cast<nsIEventTarget*>(NS_GetCurrentThread());

  mCallback.c = nullptr;
}

nsTimerImpl::~nsTimerImpl()
{
  ReleaseCallback();
}


nsresult
nsTimerImpl::Startup()
{
  nsresult rv;

  nsTimerEvent::Init();

  gThread = new TimerThread();
  if (!gThread) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_ADDREF(gThread);
  rv = gThread->InitLocks();

  if (NS_FAILED(rv)) {
    NS_RELEASE(gThread);
  }

  return rv;
}

void
nsTimerImpl::Shutdown()
{
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    double mean = 0, stddev = 0;
    myNS_MeanAndStdDev(sDeltaNum, sDeltaSum, sDeltaSumSquared, &mean, &stddev);

    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("sDeltaNum = %f, sDeltaSum = %f, sDeltaSumSquared = %f\n",
            sDeltaNum, sDeltaSum, sDeltaSumSquared));
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("mean: %fms, stddev: %fms\n", mean, stddev));
  }
#endif

  if (!gThread) {
    return;
  }

  gThread->Shutdown();
  NS_RELEASE(gThread);

  nsTimerEvent::Shutdown();
}


nsresult
nsTimerImpl::InitCommon(uint32_t aType, uint32_t aDelay)
{
  nsresult rv;

  if (NS_WARN_IF(!gThread)) {
    return NS_ERROR_NOT_INITIALIZED;
  }
  if (!mEventTarget) {
    NS_ERROR("mEventTarget is NULL");
    return NS_ERROR_NOT_INITIALIZED;
  }

  rv = gThread->Init();
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  













  if (mArmed) {
    gThread->RemoveTimer(this);
  }
  mCanceled = false;
  mTimeout = TimeStamp();
  mGeneration = gGenerator++;

  mType = (uint8_t)aType;
  SetDelayInternal(aDelay);

  return gThread->AddTimer(this);
}

NS_IMETHODIMP
nsTimerImpl::InitWithFuncCallback(nsTimerCallbackFunc aFunc,
                                  void* aClosure,
                                  uint32_t aDelay,
                                  uint32_t aType)
{
  if (NS_WARN_IF(!aFunc)) {
    return NS_ERROR_INVALID_ARG;
  }

  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_FUNC;
  mCallback.c = aFunc;
  mClosure = aClosure;

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP
nsTimerImpl::InitWithCallback(nsITimerCallback* aCallback,
                              uint32_t aDelay,
                              uint32_t aType)
{
  if (NS_WARN_IF(!aCallback)) {
    return NS_ERROR_INVALID_ARG;
  }

  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_INTERFACE;
  mCallback.i = aCallback;
  NS_ADDREF(mCallback.i);

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP
nsTimerImpl::Init(nsIObserver* aObserver, uint32_t aDelay, uint32_t aType)
{
  if (NS_WARN_IF(!aObserver)) {
    return NS_ERROR_INVALID_ARG;
  }

  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_OBSERVER;
  mCallback.o = aObserver;
  NS_ADDREF(mCallback.o);

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP
nsTimerImpl::Cancel()
{
  mCanceled = true;

  if (gThread) {
    gThread->RemoveTimer(this);
  }

  ReleaseCallback();

  return NS_OK;
}

NS_IMETHODIMP
nsTimerImpl::SetDelay(uint32_t aDelay)
{
  if (mCallbackType == CALLBACK_TYPE_UNKNOWN && mType == TYPE_ONE_SHOT) {
    
    
    NS_ERROR("nsITimer->SetDelay() called when the "
             "one-shot timer is not set up.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  if (!mTimeout.IsNull() && mType == TYPE_REPEATING_PRECISE) {
    mTimeout = TimeStamp::Now();
  }

  SetDelayInternal(aDelay);

  if (!mFiring && gThread) {
    gThread->TimerDelayChanged(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsTimerImpl::GetDelay(uint32_t* aDelay)
{
  *aDelay = mDelay;
  return NS_OK;
}

NS_IMETHODIMP
nsTimerImpl::SetType(uint32_t aType)
{
  mType = (uint8_t)aType;
  
  
  
  return NS_OK;
}

NS_IMETHODIMP
nsTimerImpl::GetType(uint32_t* aType)
{
  *aType = mType;
  return NS_OK;
}


NS_IMETHODIMP
nsTimerImpl::GetClosure(void** aClosure)
{
  *aClosure = mClosure;
  return NS_OK;
}


NS_IMETHODIMP
nsTimerImpl::GetCallback(nsITimerCallback** aCallback)
{
  if (mCallbackType == CALLBACK_TYPE_INTERFACE) {
    NS_IF_ADDREF(*aCallback = mCallback.i);
  } else if (mTimerCallbackWhileFiring) {
    NS_ADDREF(*aCallback = mTimerCallbackWhileFiring);
  } else {
    *aCallback = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsTimerImpl::GetTarget(nsIEventTarget** aTarget)
{
  NS_IF_ADDREF(*aTarget = mEventTarget);
  return NS_OK;
}


NS_IMETHODIMP
nsTimerImpl::SetTarget(nsIEventTarget* aTarget)
{
  if (NS_WARN_IF(mCallbackType != CALLBACK_TYPE_UNKNOWN)) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  if (aTarget) {
    mEventTarget = aTarget;
  } else {
    mEventTarget = static_cast<nsIEventTarget*>(NS_GetCurrentThread());
  }
  return NS_OK;
}


void
nsTimerImpl::Fire()
{
  if (mCanceled) {
    return;
  }

#ifdef MOZ_NUWA_PROCESS
  if (IsNuwaProcess() && IsNuwaReady()) {
    
    return;
  }
#endif

  PROFILER_LABEL("Timer", "Fire",
                 js::ProfileEntry::Category::OTHER);

#ifdef MOZ_TASK_TRACER
  
  
  
  mozilla::tasktracer::AutoRunFakeTracedTask runTracedTask(mTracedTask);
#endif

#ifdef DEBUG_TIMERS
  TimeStamp now = TimeStamp::Now();
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    TimeDuration   a = now - mStart; 
    TimeDuration   b = TimeDuration::FromMilliseconds(mDelay); 
    TimeDuration   delta = (a > b) ? a - b : b - a;
    uint32_t       d = delta.ToMilliseconds(); 
    sDeltaSum += d;
    sDeltaSumSquared += double(d) * double(d);
    sDeltaNum++;

    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p] expected delay time %4ums\n", this, mDelay));
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p] actual delay time   %fms\n", this,
            a.ToMilliseconds()));
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p] (mType is %d)       -------\n", this, mType));
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p]     delta           %4dms\n",
            this, (a > b) ? (int32_t)d : -(int32_t)d));

    mStart = mStart2;
    mStart2 = TimeStamp();
  }
#endif

  TimeStamp timeout = mTimeout;
  if (IsRepeatingPrecisely()) {
    
    
    timeout -= TimeDuration::FromMilliseconds(mDelay);
  }

  if (mCallbackType == CALLBACK_TYPE_INTERFACE) {
    mTimerCallbackWhileFiring = mCallback.i;
  }
  mFiring = true;

  
  
  CallbackUnion callback = mCallback;
  unsigned callbackType = mCallbackType;
  if (callbackType == CALLBACK_TYPE_INTERFACE) {
    NS_ADDREF(callback.i);
  } else if (callbackType == CALLBACK_TYPE_OBSERVER) {
    NS_ADDREF(callback.o);
  }
  ReleaseCallback();

  switch (callbackType) {
    case CALLBACK_TYPE_FUNC:
      callback.c(this, mClosure);
      break;
    case CALLBACK_TYPE_INTERFACE:
      callback.i->Notify(this);
      break;
    case CALLBACK_TYPE_OBSERVER:
      callback.o->Observe(static_cast<nsITimer*>(this),
                          NS_TIMER_CALLBACK_TOPIC,
                          nullptr);
      break;
    default:
      ;
  }

  
  
  if (mCallbackType == CALLBACK_TYPE_UNKNOWN &&
      mType != TYPE_ONE_SHOT && !mCanceled) {
    mCallback = callback;
    mCallbackType = callbackType;
  } else {
    
    if (callbackType == CALLBACK_TYPE_INTERFACE) {
      NS_RELEASE(callback.i);
    } else if (callbackType == CALLBACK_TYPE_OBSERVER) {
      NS_RELEASE(callback.o);
    }
  }

  mFiring = false;
  mTimerCallbackWhileFiring = nullptr;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p] Took %fms to fire timer callback\n",
            this, (TimeStamp::Now() - now).ToMilliseconds()));
  }
#endif

  
  
  
  if (IsRepeating() && mType != TYPE_REPEATING_PRECISE && !mArmed) {
    if (mType == TYPE_REPEATING_SLACK) {
      SetDelayInternal(mDelay);  
    }
    
    
    if (gThread) {
      gThread->AddTimer(this);
    }
  }
}

void
nsTimerEvent::Init()
{
  sAllocator = new TimerEventAllocator();
}

void
nsTimerEvent::Shutdown()
{
  sCanDeleteAllocator = true;
  DeleteAllocatorIfNeeded();
}

void
nsTimerEvent::DeleteAllocatorIfNeeded()
{
  if (sCanDeleteAllocator && sAllocatorUsers == 0) {
    delete sAllocator;
    sAllocator = nullptr;
  }
}

NS_IMETHODIMP
nsTimerEvent::Run()
{
  if (mGeneration != mTimer->GetGeneration()) {
    return NS_OK;
  }

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    TimeStamp now = TimeStamp::Now();
    PR_LOG(GetTimerLog(), PR_LOG_DEBUG,
           ("[this=%p] time between PostTimerEvent() and Fire(): %fms\n",
            this, (now - mInitTime).ToMilliseconds()));
  }
#endif

  mTimer->Fire();
  
  
  
  
  mTimer = nullptr;

  return NS_OK;
}

already_AddRefed<nsTimerImpl>
nsTimerImpl::PostTimerEvent(already_AddRefed<nsTimerImpl> aTimerRef)
{
  nsRefPtr<nsTimerImpl> timer(aTimerRef);
  if (!timer->mEventTarget) {
    NS_ERROR("Attempt to post timer event to NULL event target");
    return timer.forget();
  }

  

  
  
  
  
  

  
  
  nsRefPtr<nsTimerEvent> event = new nsTimerEvent;
  if (!event) {
    return timer.forget();
  }

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    event->mInitTime = TimeStamp::Now();
  }
#endif

  
  
  if (timer->IsRepeatingPrecisely()) {
    timer->SetDelayInternal(timer->mDelay);

    
    if (gThread && timer->mType == TYPE_REPEATING_PRECISE) {
      nsresult rv = gThread->AddTimer(timer);
      if (NS_FAILED(rv)) {
        return timer.forget();
      }
    }
  }

  nsIEventTarget* target = timer->mEventTarget;
  event->SetTimer(timer.forget());

  nsresult rv = target->Dispatch(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    timer = event->ForgetTimer();
    if (gThread) {
      gThread->RemoveTimer(timer);
    }
    return timer.forget();
  }

  return nullptr;
}

void
nsTimerImpl::SetDelayInternal(uint32_t aDelay)
{
  TimeDuration delayInterval = TimeDuration::FromMilliseconds(aDelay);

  mDelay = aDelay;

  TimeStamp now = TimeStamp::Now();
  if (mTimeout.IsNull() || mType != TYPE_REPEATING_PRECISE) {
    mTimeout = now;
  }

  mTimeout += delayInterval;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(GetTimerLog(), PR_LOG_DEBUG)) {
    if (mStart.IsNull()) {
      mStart = now;
    } else {
      mStart2 = now;
    }
  }
#endif
}

size_t
nsTimerImpl::SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
  return aMallocSizeOf(this);
}


nsresult
NS_NewTimer(nsITimer** aResult, nsTimerCallbackFunc aCallback, void* aClosure,
            uint32_t aDelay, uint32_t aType)
{
  nsTimerImpl* timer = new nsTimerImpl();
  if (!timer) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(timer);

  nsresult rv = timer->InitWithFuncCallback(aCallback, aClosure,
                                            aDelay, aType);
  if (NS_FAILED(rv)) {
    NS_RELEASE(timer);
    return rv;
  }

  *aResult = timer;
  return NS_OK;
}
