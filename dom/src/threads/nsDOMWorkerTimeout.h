





































#ifndef __NSDOMWORKERTIMEOUT_H__
#define __NSDOMWORKERTIMEOUT_H__


#include "nsITimer.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "prclist.h"


#include "nsDOMWorkerThread.h"













class nsDOMWorkerTimeout : public PRCList,
                           public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  nsDOMWorkerTimeout(nsDOMWorkerThread* aWorker, PRUint32 aId);
  ~nsDOMWorkerTimeout();

  nsresult Init(JSContext* aCx, PRUint32 aArgc, jsval* aArgv,
                PRBool aIsInterval);

  nsresult Run();

  void Cancel();
  void Suspend(PRTime aNow);
  void Resume(PRTime aNow);

  PRIntervalTime GetInterval() {
    return mInterval;
  }

  nsDOMWorkerThread* GetWorker() {
    return mWorker;
  }

  PRUint32 GetId() {
    return mId;
  }

  PRBool IsSuspended() {
    AutoSpinlock lock(this);
    return IsSuspendedNoLock();
  }

private:
  void AcquireSpinlock();
  void ReleaseSpinlock();

  PRBool IsSuspendedNoLock() {
    return mIsSuspended;
  }

  class AutoSpinlock
  {
  public:
    AutoSpinlock(nsDOMWorkerTimeout* aTimeout)
    : mTimeout(aTimeout) {
      aTimeout->AcquireSpinlock();
    }

    ~AutoSpinlock() {
      mTimeout->ReleaseSpinlock();
    }
  private:
    nsDOMWorkerTimeout* mTimeout;
  };

  
  
  
  class CallbackBase
  {
  public:
    virtual ~CallbackBase() { }
    virtual nsresult Run(nsDOMWorkerTimeout* aTimeout,
                         JSContext* aCx) = 0;
  };

  class FunctionCallback : public CallbackBase
  {
  public:
    FunctionCallback(PRUint32 aArgc, jsval* aArgv, nsresult* aRv);
    virtual ~FunctionCallback();
    virtual nsresult Run(nsDOMWorkerTimeout* aTimeout,
                         JSContext* aCx);
  protected:
    jsval mCallback;
    jsval* mCallbackArgs;
    PRUint32 mCallbackArgsLength;
  };
  
  class ExpressionCallback : public CallbackBase
  {
  public:
    ExpressionCallback(PRUint32 aArgc, jsval* aArgv, JSContext* aCx,
                       nsresult* aRv);
    virtual ~ExpressionCallback();
    virtual nsresult Run(nsDOMWorkerTimeout* aTimeout,
                         JSContext* aCx);
  protected:
    JSString* mExpression;
    nsString mFileName;
    PRUint32 mLineNumber;
  };

  
  nsRefPtr<nsDOMWorkerThread> mWorker;

  
  nsCOMPtr<nsITimer> mTimer;

  PRUint32 mInterval;
  PRBool mIsInterval;

  PRTime mTargetTime;

  nsAutoPtr<CallbackBase> mCallback;

  PRUint32 mId;

  PRInt32 mSuspendSpinlock;
  PRBool mIsSuspended;
  PRUint32 mSuspendInterval;
  nsRefPtr<nsDOMWorkerTimeout> mSuspendedRef;

#ifdef DEBUG
  PRBool mFiredOrCanceled;
#endif
};

#endif 
