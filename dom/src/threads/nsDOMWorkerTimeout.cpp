





































#include "nsDOMWorkerTimeout.h"


#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsITimer.h"
#include "nsIXPConnect.h"


#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "pratom.h"


#include "nsDOMThreadService.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

#define CONSTRUCTOR_ENSURE_TRUE(_cond, _rv) \
  PR_BEGIN_MACRO \
    if (NS_UNLIKELY(!(_cond))) { \
      NS_WARNING("CONSTRUCTOR_ENSURE_TRUE(" #_cond ") failed"); \
      (_rv) = NS_ERROR_FAILURE; \
      return; \
    } \
  PR_END_MACRO

#define SUSPEND_SPINLOCK_COUNT 5000

static const char* kSetIntervalStr = "setInterval";
static const char* kSetTimeoutStr = "setTimeout";

nsDOMWorkerTimeout::FunctionCallback::FunctionCallback(PRUint32 aArgc,
                                                       jsval* aArgv,
                                                       nsresult* aRv)
: mCallback(nsnull),
  mCallbackArgs(nsnull),
  mCallbackArgsLength(0)
{
  MOZ_COUNT_CTOR(nsDOMWorkerTimeout::FunctionCallback);

  JSRuntime* rt;
  *aRv = nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt);
  NS_ENSURE_SUCCESS(*aRv,);

  PRBool success = JS_AddNamedRootRT(rt, &mCallback,
                                     "nsDOMWorkerTimeout Callback Object");
  CONSTRUCTOR_ENSURE_TRUE(success, *aRv);

  mCallback = aArgv[0];

  
  mCallbackArgsLength = aArgc > 2 ? aArgc - 1 : 1;

  mCallbackArgs = new jsval[mCallbackArgsLength];
  if (NS_UNLIKELY(!mCallbackArgs)) {
    
    mCallbackArgsLength = 0;

    NS_ERROR("Out of memory!");
    *aRv = NS_ERROR_OUT_OF_MEMORY;
    return;
  }

  for (PRUint32 i = 0; i < mCallbackArgsLength - 1; i++) {
    mCallbackArgs[i] = aArgv[i + 2];
    success = JS_AddNamedRootRT(rt, &mCallbackArgs[i],
                                "nsDOMWorkerTimeout Callback Arg");
    if (NS_UNLIKELY(!success)) {
      
      
      mCallbackArgsLength = i;

      NS_WARNING("Failed to add root!");
      *aRv = NS_ERROR_FAILURE;
      return;
    }
  }

  
  mCallbackArgs[mCallbackArgsLength - 1] = 0;
  success = JS_AddNamedRootRT(rt, &mCallbackArgs[mCallbackArgsLength - 1],
                              "nsDOMWorkerTimeout Callback Final Arg");
  if (NS_UNLIKELY(!success)) {
    
    
    mCallbackArgsLength -= 1;

    NS_WARNING("Failed to add root!");
    *aRv = NS_ERROR_FAILURE;
    return;
  }

  *aRv = NS_OK;
}

nsDOMWorkerTimeout::FunctionCallback::~FunctionCallback()
{
  MOZ_COUNT_DTOR(nsDOMWorkerTimeout::FunctionCallback);

  if (mCallback) {
    JSRuntime* rt;
    nsresult rv = nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt);

    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Can't unroot callback objects!");

    if (NS_SUCCEEDED(rv)) {
      for (PRUint32 i = 0; i < mCallbackArgsLength; i++) {
        JS_RemoveRootRT(rt, &mCallbackArgs[i]);
      }
      JS_RemoveRootRT(rt, &mCallback);
    }
  }

  delete [] mCallbackArgs;
}

nsresult
nsDOMWorkerTimeout::FunctionCallback::Run(nsDOMWorkerTimeout* aTimeout,
                                          JSContext* aCx)
{
  PRInt32 lateness = PR_MAX(0, PRInt32(PR_Now() - aTimeout->mTargetTime)) /
                     (PRTime)PR_USEC_PER_MSEC;
  mCallbackArgs[mCallbackArgsLength - 1] = INT_TO_JSVAL(lateness);

  JSObject* global = JS_GetGlobalObject(aCx);
  NS_ENSURE_TRUE(global, NS_ERROR_FAILURE);

  jsval rval;
  PRBool success =
    JS_CallFunctionValue(aCx, global, mCallback, mCallbackArgsLength,
                         mCallbackArgs, &rval);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

nsDOMWorkerTimeout::ExpressionCallback::ExpressionCallback(PRUint32 aArgc,
                                                           jsval* aArgv,
                                                           JSContext* aCx,
                                                           nsresult* aRv)
: mExpression(nsnull),
  mLineNumber(0)
{
  MOZ_COUNT_CTOR(nsDOMWorkerTimeout::ExpressionCallback);

  JSString* expr = JS_ValueToString(aCx, aArgv[0]);
  *aRv = expr ? NS_OK : NS_ERROR_FAILURE;
  NS_ENSURE_SUCCESS(*aRv,);

  JSRuntime* rt;
  *aRv = nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt);
  NS_ENSURE_SUCCESS(*aRv,);

  PRBool success = JS_AddNamedRootRT(rt, &mExpression,
                                     "nsDOMWorkerTimeout Expression");
  CONSTRUCTOR_ENSURE_TRUE(success, *aRv);

  mExpression = expr;

  
  const char* fileName;
  PRUint32 lineNumber;
  if (nsJSUtils::GetCallingLocation(aCx, &fileName, &lineNumber, nsnull)) {
    CopyUTF8toUTF16(nsDependentCString(fileName), mFileName);
    mLineNumber = lineNumber;
  }

  *aRv = NS_OK;
}

nsDOMWorkerTimeout::ExpressionCallback::~ExpressionCallback()
{
  MOZ_COUNT_DTOR(nsDOMWorkerTimeout::ExpressionCallback);

  if (mExpression) {
    JSRuntime* rt;
    nsresult rv = nsDOMThreadService::JSRuntimeService()->GetRuntime(&rt);

    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Can't unroot callback objects!");

    if (NS_SUCCEEDED(rv)) {
      JS_RemoveRootRT(rt, &mExpression);
    }
  }
}

nsresult
nsDOMWorkerTimeout::ExpressionCallback::Run(nsDOMWorkerTimeout* aTimeout,
                                            JSContext* aCx)
{
  NS_ERROR("Not yet implemented!");
  return NS_ERROR_NOT_IMPLEMENTED;
}

nsDOMWorkerTimeout::nsDOMWorkerTimeout(nsDOMWorkerThread* aWorker,
                                       PRUint32 aId)
: mWorker(aWorker),
  mInterval(0),
  mIsInterval(PR_FALSE),
  mId(aId),
  mSuspendSpinlock(0),
  mIsSuspended(PR_FALSE),
  mSuspendInterval(0)
#ifdef DEBUG
, mFiredOrCanceled(PR_FALSE)
#endif
{
  MOZ_COUNT_CTOR(nsDOMWorkerTimeout);
  NS_ASSERTION(mWorker, "Need a worker here!");
}

nsDOMWorkerTimeout::~nsDOMWorkerTimeout()
{
  MOZ_COUNT_DTOR(nsDOMWorkerTimeout);

  
  if (mTimer) {
    NS_ASSERTION(mFiredOrCanceled || mWorker->IsCanceled(),
                 "Timeout should have fired or been canceled!");

    mWorker->RemoveTimeout(this);
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDOMWorkerTimeout, nsITimerCallback)

nsresult
nsDOMWorkerTimeout::Init(JSContext* aCx, PRUint32 aArgc, jsval* aArgv,
                         PRBool aIsInterval)
{
  NS_ASSERTION(aCx, "Null pointer!");
  NS_ASSERTION(aArgv, "Null pointer!");

  JSAutoRequest ar(aCx);

  if (!aArgc) {
    JS_ReportError(aCx, "Function %s requires at least 1 parameter",
                   aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
    return NS_ERROR_INVALID_ARG;
  }

  PRUint32 interval;
  if (aArgc > 1) {
    if (!JS_ValueToECMAUint32(aCx, aArgv[1], (uint32*)&interval)) {
      JS_ReportError(aCx, "Second argument to %s must be a millisecond value",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
      return NS_ERROR_INVALID_ARG;
    }
  }
  else {
    
    
    aIsInterval = PR_FALSE;
  }

  mInterval = interval;

  mTargetTime = PR_Now() + interval * (PRTime)PR_USEC_PER_MSEC;

  nsresult rv;
  switch (JS_TypeOfValue(aCx, aArgv[0])) {
    case JSTYPE_FUNCTION:
      mCallback = new FunctionCallback(aArgc, aArgv, &rv);
      NS_ENSURE_TRUE(mCallback, NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_SUCCESS(rv, rv);

      break;

    case JSTYPE_STRING:
    case JSTYPE_OBJECT:
      mCallback = new ExpressionCallback(aArgc, aArgv, aCx, &rv);
      NS_ENSURE_TRUE(mCallback, NS_ERROR_OUT_OF_MEMORY);
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    default:
      JS_ReportError(aCx, "useless %s call (missing quotes around argument?)",
                     aIsInterval ? kSetIntervalStr : kSetTimeoutStr);
  
      
      return NS_ERROR_INVALID_ARG;
  }

  PRInt32 type;
  if (aIsInterval) {
    type = nsITimer::TYPE_REPEATING_SLACK;
  }
  else {
    type = nsITimer::TYPE_ONE_SHOT;
  }
  mIsInterval = aIsInterval;

  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIEventTarget* target =
    static_cast<nsIEventTarget*>(nsDOMThreadService::get());

  rv = timer->SetTarget(target);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = timer->InitWithCallback(this, interval, type);
  NS_ENSURE_SUCCESS(rv, rv);

  mTimer.swap(timer);

  if (!mWorker->AddTimeout(this)) {
    
    mTimer->Cancel();
    mTimer = nsnull;
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

nsresult
nsDOMWorkerTimeout::Run()
{
  NS_ENSURE_TRUE(mCallback, NS_ERROR_NOT_INITIALIZED);
  LOG(("Worker [0x%p] running timeout [0x%p] with id %u",
       static_cast<void*>(mWorker.get()), static_cast<void*>(this), mId));

#ifdef DEBUG
  mFiredOrCanceled = PR_TRUE;
#endif

  JSContext* cx;
  nsresult rv =
    nsDOMThreadService::ThreadJSContextStack()->GetSafeJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  JSAutoRequest ar(cx);

  rv = mCallback->Run(this, cx);

  
  JS_ReportPendingException(cx);

  if (mIsInterval) {
    mTargetTime = PR_Now() + mInterval * (PRTime)PR_USEC_PER_MSEC;
  }

  return rv;
}

void
nsDOMWorkerTimeout::Cancel()
{
  NS_ASSERTION(mTimer, "Impossible to get here without a timer!");

  LOG(("Worker [0x%p] canceling timeout [0x%p] with id %u",
       static_cast<void*>(mWorker.get()), static_cast<void*>(this), mId));

#ifdef DEBUG
  mFiredOrCanceled = PR_TRUE;
#endif

  {
    AutoSpinlock lock(this);

    if (IsSuspendedNoLock()) {
      mIsSuspended = PR_FALSE;
      
      mSuspendedRef = nsnull;
    }
  }

  
  mTimer->Cancel();
}

void
nsDOMWorkerTimeout::Suspend(PRTime aNow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mTimer, "Impossible to get here without a timer!");

  AutoSpinlock lock(this);

  if (!mIsSuspended) {
    mIsSuspended = PR_TRUE;
    mSuspendedRef = this;
  }

  mTimer->Cancel();

  mSuspendInterval = PR_MAX(0, PRInt32(mTargetTime - aNow)) /
                     (PRTime)PR_USEC_PER_MSEC;

  LOG(("Worker [0x%p] suspending timeout [0x%p] with id %u (interval = %u)",
       static_cast<void*>(mWorker.get()), static_cast<void*>(this), mId,
       mSuspendInterval));
}

void
nsDOMWorkerTimeout::Resume(PRTime aNow)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(mTimer, "Impossible to get here without a timer!");

  LOG(("Worker [0x%p] resuming timeout [0x%p] with id %u",
       static_cast<void*>(mWorker.get()), static_cast<void*>(this), mId));

  AutoSpinlock lock(this);

  NS_ASSERTION(IsSuspendedNoLock(), "Should be suspended!");

  mTargetTime = aNow + mSuspendInterval * (PRTime)PR_USEC_PER_MSEC;

#ifdef DEBUG
  nsresult rv =
#endif
  mTimer->InitWithCallback(this, mSuspendInterval, nsITimer::TYPE_ONE_SHOT);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to init timer!");
}

void
nsDOMWorkerTimeout::AcquireSpinlock()
{
  PRUint32 loopCount = 0;
  while (PR_AtomicSet(&mSuspendSpinlock, 1) == 1) {
    if (++loopCount > SUSPEND_SPINLOCK_COUNT) {
      LOG(("AcquireSpinlock taking too long (looped %u times), yielding.",
           loopCount));
      loopCount = 0;
      PR_Sleep(PR_INTERVAL_NO_WAIT);
    }
  }
#ifdef PR_LOGGING
  if (loopCount) {
    LOG(("AcquireSpinlock needed %u loops", loopCount));
  }
#endif
}

void
nsDOMWorkerTimeout::ReleaseSpinlock()
{
#ifdef DEBUG
  PRInt32 suspended =
#endif
  PR_AtomicSet(&mSuspendSpinlock, 0);
  NS_ASSERTION(suspended == 1, "Huh?!");
}

NS_IMETHODIMP
nsDOMWorkerTimeout::Notify(nsITimer* aTimer)
{
  
  NS_ASSERTION(!NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(aTimer == mTimer, "Wrong timer?!");

  PRUint32 type;
  nsresult rv = aTimer->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (type == nsITimer::TYPE_ONE_SHOT) {
    AutoSpinlock lock(this);
    if (mIsSuspended) {
      if (mIsInterval) {
        
             

        
        
        mTargetTime = PR_Now() + mInterval * (PRTime)PR_USEC_PER_MSEC;

        rv = aTimer->InitWithCallback(this, mInterval,
                                      nsITimer::TYPE_REPEATING_SLACK);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      mIsSuspended = PR_FALSE;
      mSuspendedRef = nsnull;
    }
  }

  nsDOMThreadService::get()->TimeoutReady(this);
  return NS_OK;
}
