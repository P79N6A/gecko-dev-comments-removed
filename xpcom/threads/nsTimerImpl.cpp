







































#include "nsTimerImpl.h"
#include "TimerThread.h"
#include "nsAutoPtr.h"
#include "nsThreadManager.h"
#include "nsThreadUtils.h"
#include "prmem.h"

using mozilla::TimeDuration;
using mozilla::TimeStamp;

static PRInt32          gGenerator = 0;
static TimerThread*     gThread = nsnull;

#ifdef DEBUG_TIMERS
#include <math.h>

double nsTimerImpl::sDeltaSumSquared = 0;
double nsTimerImpl::sDeltaSum = 0;
double nsTimerImpl::sDeltaNum = 0;

static void
myNS_MeanAndStdDev(double n, double sumOfValues, double sumOfSquaredValues,
                   double *meanResult, double *stdDevResult)
{
  double mean = 0.0, var = 0.0, stdDev = 0.0;
  if (n > 0.0 && sumOfValues >= 0) {
    mean = sumOfValues / n;
    double temp = (n * sumOfSquaredValues) - (sumOfValues * sumOfValues);
    if (temp < 0.0 || n <= 1)
      var = 0.0;
    else
      var = temp / (n * (n - 1));
    
    stdDev = var != 0.0 ? sqrt(var) : 0.0;
  }
  *meanResult = mean;
  *stdDevResult = stdDev;
}
#endif

NS_IMPL_THREADSAFE_QUERY_INTERFACE1(nsTimerImpl, nsITimer)
NS_IMPL_THREADSAFE_ADDREF(nsTimerImpl)

NS_IMETHODIMP_(nsrefcnt) nsTimerImpl::Release(void)
{
  nsrefcnt count;

  NS_PRECONDITION(0 != mRefCnt, "dup release");
  count = NS_AtomicDecrementRefcnt(mRefCnt);
  NS_LOG_RELEASE(this, count, "nsTimerImpl");
  if (count == 0) {
    mRefCnt = 1; 

    
    
    delete this;
    return 0;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (count == 1 && mArmed) {
    mCanceled = PR_TRUE;

    NS_ASSERTION(gThread, "An armed timer exists after the thread timer stopped.");
    if (NS_SUCCEEDED(gThread->RemoveTimer(this)))
      return 0;
  }

  return count;
}

nsTimerImpl::nsTimerImpl() :
  mClosure(nsnull),
  mCallbackType(CALLBACK_TYPE_UNKNOWN),
  mFiring(PR_FALSE),
  mArmed(PR_FALSE),
  mCanceled(PR_FALSE),
  mGeneration(0),
  mDelay(0)
{
  
  mEventTarget = static_cast<nsIEventTarget*>(NS_GetCurrentThread());

  mCallback.c = nsnull;
}

nsTimerImpl::~nsTimerImpl()
{
  ReleaseCallback();
}


nsresult
nsTimerImpl::Startup()
{
  nsresult rv;

  gThread = new TimerThread();
  if (!gThread) return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(gThread);
  rv = gThread->InitLocks();

  if (NS_FAILED(rv)) {
    NS_RELEASE(gThread);
  }

  return rv;
}

void nsTimerImpl::Shutdown()
{
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    double mean = 0, stddev = 0;
    myNS_MeanAndStdDev(sDeltaNum, sDeltaSum, sDeltaSumSquared, &mean, &stddev);

    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("sDeltaNum = %f, sDeltaSum = %f, sDeltaSumSquared = %f\n", sDeltaNum, sDeltaSum, sDeltaSumSquared));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("mean: %fms, stddev: %fms\n", mean, stddev));
  }
#endif

  if (!gThread)
    return;

  gThread->Shutdown();
  NS_RELEASE(gThread);
}


nsresult nsTimerImpl::InitCommon(PRUint32 aType, PRUint32 aDelay)
{
  nsresult rv;

  NS_ENSURE_TRUE(gThread, NS_ERROR_NOT_INITIALIZED);

  rv = gThread->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  













  if (mArmed)
    gThread->RemoveTimer(this);
  mCanceled = PR_FALSE;
  mGeneration = PR_ATOMIC_INCREMENT(&gGenerator);

  mType = (PRUint8)aType;
  SetDelayInternal(aDelay);

  return gThread->AddTimer(this);
}

NS_IMETHODIMP nsTimerImpl::InitWithFuncCallback(nsTimerCallbackFunc aFunc,
                                                void *aClosure,
                                                PRUint32 aDelay,
                                                PRUint32 aType)
{
  NS_ENSURE_ARG_POINTER(aFunc);
  
  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_FUNC;
  mCallback.c = aFunc;
  mClosure = aClosure;

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP nsTimerImpl::InitWithCallback(nsITimerCallback *aCallback,
                                            PRUint32 aDelay,
                                            PRUint32 aType)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_INTERFACE;
  mCallback.i = aCallback;
  NS_ADDREF(mCallback.i);

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP nsTimerImpl::Init(nsIObserver *aObserver,
                                PRUint32 aDelay,
                                PRUint32 aType)
{
  NS_ENSURE_ARG_POINTER(aObserver);

  ReleaseCallback();
  mCallbackType = CALLBACK_TYPE_OBSERVER;
  mCallback.o = aObserver;
  NS_ADDREF(mCallback.o);

  return InitCommon(aType, aDelay);
}

NS_IMETHODIMP nsTimerImpl::Cancel()
{
  mCanceled = PR_TRUE;

  if (gThread)
    gThread->RemoveTimer(this);

  ReleaseCallback();

  return NS_OK;
}

NS_IMETHODIMP nsTimerImpl::SetDelay(PRUint32 aDelay)
{
  if (mCallbackType == CALLBACK_TYPE_UNKNOWN && mType == TYPE_ONE_SHOT) {
    
    
    NS_ERROR("nsITimer->SetDelay() called when the "
             "one-shot timer is not set up.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  
  if (!mTimeout.IsNull() && mType == TYPE_REPEATING_PRECISE)
    mTimeout = TimeStamp::Now();

  SetDelayInternal(aDelay);

  if (!mFiring && gThread)
    gThread->TimerDelayChanged(this);

  return NS_OK;
}

NS_IMETHODIMP nsTimerImpl::GetDelay(PRUint32* aDelay)
{
  *aDelay = mDelay;
  return NS_OK;
}

NS_IMETHODIMP nsTimerImpl::SetType(PRUint32 aType)
{
  mType = (PRUint8)aType;
  
  
  
  return NS_OK;
}

NS_IMETHODIMP nsTimerImpl::GetType(PRUint32* aType)
{
  *aType = mType;
  return NS_OK;
}


NS_IMETHODIMP nsTimerImpl::GetClosure(void** aClosure)
{
  *aClosure = mClosure;
  return NS_OK;
}


NS_IMETHODIMP nsTimerImpl::GetCallback(nsITimerCallback **aCallback)
{
  if (mCallbackType == CALLBACK_TYPE_INTERFACE)
    NS_IF_ADDREF(*aCallback = mCallback.i);
  else if (mTimerCallbackWhileFiring)
    NS_ADDREF(*aCallback = mTimerCallbackWhileFiring);
  else
    *aCallback = nsnull;

  return NS_OK;
}


NS_IMETHODIMP nsTimerImpl::GetTarget(nsIEventTarget** aTarget)
{
  NS_IF_ADDREF(*aTarget = mEventTarget);
  return NS_OK;
}


NS_IMETHODIMP nsTimerImpl::SetTarget(nsIEventTarget* aTarget)
{
  NS_ENSURE_TRUE(mCallbackType == CALLBACK_TYPE_UNKNOWN,
                 NS_ERROR_ALREADY_INITIALIZED);

  if (aTarget)
    mEventTarget = aTarget;
  else
    mEventTarget = static_cast<nsIEventTarget*>(NS_GetCurrentThread());
  return NS_OK;
}


void nsTimerImpl::Fire()
{
  if (mCanceled)
    return;

  TimeStamp now = TimeStamp::Now();
#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    TimeDuration   a = now - mStart; 
    TimeDuration   b = TimeDuration::FromMilliseconds(mDelay); 
    TimeDuration   delta = (a > b) ? a - b : b - a;
    PRUint32       d = delta.ToMilliseconds(); 
    sDeltaSum += d;
    sDeltaSumSquared += double(d) * double(d);
    sDeltaNum++;

    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] expected delay time %4ums\n", this, mDelay));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] actual delay time   %fms\n", this, a.ToMilliseconds()));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p] (mType is %d)       -------\n", this, mType));
    PR_LOG(gTimerLog, PR_LOG_DEBUG, ("[this=%p]     delta           %4dms\n", this, (a > b) ? (PRInt32)d : -(PRInt32)d));

    mStart = mStart2;
    mStart2 = TimeStamp();
  }
#endif

  TimeStamp timeout = mTimeout;
  if (IsRepeatingPrecisely()) {
    
    
    timeout -= TimeDuration::FromMilliseconds(mDelay);
  }
  if (gThread)
    gThread->UpdateFilter(mDelay, timeout, now);

  if (mCallbackType == CALLBACK_TYPE_INTERFACE)
    mTimerCallbackWhileFiring = mCallback.i;
  mFiring = PR_TRUE;
  
  
  
  CallbackUnion callback = mCallback;
  PRUintn callbackType = mCallbackType;
  if (callbackType == CALLBACK_TYPE_INTERFACE)
    NS_ADDREF(callback.i);
  else if (callbackType == CALLBACK_TYPE_OBSERVER)
    NS_ADDREF(callback.o);
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
                          nsnull);
      break;
    default:;
  }

  
  
  if (mCallbackType == CALLBACK_TYPE_UNKNOWN &&
      mType != TYPE_ONE_SHOT && !mCanceled) {
    mCallback = callback;
    mCallbackType = callbackType;
  } else {
    
    if (callbackType == CALLBACK_TYPE_INTERFACE)
      NS_RELEASE(callback.i);
    else if (callbackType == CALLBACK_TYPE_OBSERVER)
      NS_RELEASE(callback.o);
  }

  mFiring = PR_FALSE;
  mTimerCallbackWhileFiring = nsnull;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    PR_LOG(gTimerLog, PR_LOG_DEBUG,
           ("[this=%p] Took %fms to fire timer callback\n",
            this, (TimeStamp::Now() - now).ToMilliseconds()));
  }
#endif

  
  
  
  if (IsRepeating() && mType != TYPE_REPEATING_PRECISE && !mArmed) {
    if (mType == TYPE_REPEATING_SLACK)
      SetDelayInternal(mDelay); 
                                
                                
    if (gThread)
      gThread->AddTimer(this);
  }
}


class nsTimerEvent : public nsRunnable {
public:
  NS_IMETHOD Run();

  nsTimerEvent(nsTimerImpl *timer, PRInt32 generation)
    : mTimer(timer), mGeneration(generation) {
    
    MOZ_COUNT_CTOR(nsTimerEvent);
  }

#ifdef DEBUG_TIMERS
  TimeStamp mInitTime;
#endif

private:
  ~nsTimerEvent() { 
#ifdef DEBUG
    if (mTimer)
      NS_WARNING("leaking reference to nsTimerImpl");
#endif
    MOZ_COUNT_DTOR(nsTimerEvent);
  }

  nsTimerImpl *mTimer;
  PRInt32      mGeneration;
};

NS_IMETHODIMP nsTimerEvent::Run()
{
  nsRefPtr<nsTimerImpl> timer;
  timer.swap(mTimer);

  if (mGeneration != timer->GetGeneration())
    return NS_OK;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    TimeStamp now = TimeStamp::Now();
    PR_LOG(gTimerLog, PR_LOG_DEBUG,
           ("[this=%p] time between PostTimerEvent() and Fire(): %fms\n",
            this, (now - mInitTime).ToMilliseconds()));
  }
#endif

  timer->Fire();

  return NS_OK;
}

nsresult nsTimerImpl::PostTimerEvent()
{
  

  
  
  
  

  nsRefPtr<nsTimerEvent> event = new nsTimerEvent(this, mGeneration);
  if (!event)
    return NS_ERROR_OUT_OF_MEMORY;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    event->mInitTime = TimeStamp::Now();
  }
#endif

  
  
  if (IsRepeatingPrecisely()) {
    SetDelayInternal(mDelay);

    
    if (gThread && mType == TYPE_REPEATING_PRECISE) {
      nsresult rv = gThread->AddTimer(this);
      if (NS_FAILED(rv))
        return rv;
    }
  }

  nsresult rv = mEventTarget->Dispatch(event, NS_DISPATCH_NORMAL);
  if (NS_FAILED(rv) && gThread)
    gThread->RemoveTimer(this);
  return rv;
}

void nsTimerImpl::SetDelayInternal(PRUint32 aDelay)
{
  TimeDuration delayInterval = TimeDuration::FromMilliseconds(aDelay);

  mDelay = aDelay;

  TimeStamp now = TimeStamp::Now();
  if (mTimeout.IsNull() || mType != TYPE_REPEATING_PRECISE)
    mTimeout = now;

  mTimeout += delayInterval;

#ifdef DEBUG_TIMERS
  if (PR_LOG_TEST(gTimerLog, PR_LOG_DEBUG)) {
    if (mStart.IsNull())
      mStart = now;
    else
      mStart2 = now;
  }
#endif
}


nsresult
NS_NewTimer(nsITimer* *aResult, nsTimerCallbackFunc aCallback, void *aClosure,
            PRUint32 aDelay, PRUint32 aType)
{
    nsTimerImpl* timer = new nsTimerImpl();
    if (timer == nsnull)
        return NS_ERROR_OUT_OF_MEMORY;
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
