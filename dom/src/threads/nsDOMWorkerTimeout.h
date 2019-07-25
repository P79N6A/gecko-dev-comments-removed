





































#ifndef __NSDOMWORKERTIMEOUT_H__
#define __NSDOMWORKERTIMEOUT_H__


#include "nsITimer.h"


#include "jsapi.h"
#include "nsAutoJSValHolder.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"


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
                bool aIsInterval);

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

  bool IsSuspended() {
    AutoSpinlock lock(this);
    return IsSuspendedNoLock();
  }

private:
  ~nsDOMWorkerTimeout() { }

  void AcquireSpinlock();
  void ReleaseSpinlock();

  bool IsSuspendedNoLock() {
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
    nsAutoJSValHolder mCallback;
    nsTArray<nsAutoJSValHolder> mCallbackArgs;
    PRUint32 mCallbackArgsLength;
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
    nsAutoJSValHolder mExpression;
    nsCString mFileName;
    PRUint32 mLineNumber;
  };

  
  nsCOMPtr<nsITimer> mTimer;

  PRUint32 mInterval;

  PRTime mTargetTime;

  nsAutoPtr<CallbackBase> mCallback;

  PRInt32 mSuspendSpinlock;
  PRUint32 mSuspendInterval;
  nsRefPtr<nsDOMWorkerTimeout> mSuspendedRef;

  bool mIsInterval;
  bool mIsSuspended;
  bool mSuspendedBeforeStart;
  bool mStarted;
};

#endif 
