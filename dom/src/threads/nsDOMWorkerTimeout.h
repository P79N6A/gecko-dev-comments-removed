





































#ifndef __NSDOMWORKERTIMEOUT_H__
#define __NSDOMWORKERTIMEOUT_H__


#include "nsITimer.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"


#include "nsDOMWorker.h"













class nsDOMWorkerTimeout : public nsDOMWorkerFeature,
                           public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSITIMERCALLBACK

  nsDOMWorkerTimeout(nsDOMWorker* aWorker,
                     PRUint32 aId);

  nsresult Init(JSContext* aCx,
                PRUint32 aArgc,
                jsval* aArgv,
                PRBool aIsInterval);

  nsresult Start();

  nsresult Run();

  virtual void Cancel();
  virtual void Suspend();
  virtual void Resume();

  PRIntervalTime GetInterval() {
    return mInterval;
  }

  nsDOMWorker* GetWorker() {
    return mWorker;
  }

  PRBool IsSuspended() {
    AutoSpinlock lock(this);
    return IsSuspendedNoLock();
  }

private:
  ~nsDOMWorkerTimeout() { }

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
    FunctionCallback(PRUint32 aArgc,
                     jsval* aArgv,
                     nsresult* aRv);
    virtual ~FunctionCallback();
    virtual nsresult Run(nsDOMWorkerTimeout* aTimeout,
                         JSContext* aCx);
  protected:
    jsval mCallback;
    jsval* mCallbackArgs;
    PRUint32 mCallbackArgsLength;
    JSRuntime* mRuntime;
  };

  class ExpressionCallback : public CallbackBase
  {
  public:
    ExpressionCallback(PRUint32 aArgc,
                       jsval* aArgv,
                       JSContext* aCx,
                       nsresult* aRv);
    virtual ~ExpressionCallback();
    virtual nsresult Run(nsDOMWorkerTimeout* aTimeout,
                         JSContext* aCx);
  protected:
    JSString* mExpression;
    nsString mFileName;
    PRUint32 mLineNumber;
    JSRuntime* mRuntime;
  };

  
  nsCOMPtr<nsITimer> mTimer;

  PRUint32 mInterval;

  PRTime mTargetTime;

  nsAutoPtr<CallbackBase> mCallback;

  PRInt32 mSuspendSpinlock;
  PRUint32 mSuspendInterval;
  nsRefPtr<nsDOMWorkerTimeout> mSuspendedRef;

  PRPackedBool mIsInterval;
  PRPackedBool mIsSuspended;
  PRPackedBool mSuspendedBeforeStart;
  PRPackedBool mStarted;
};

#endif 
