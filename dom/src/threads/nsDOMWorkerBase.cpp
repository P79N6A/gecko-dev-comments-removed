





































#include "nsDOMWorkerBase.h"

struct JSContext;


#include "nsIDOMThreads.h"
#include "nsIJSContextStack.h"
#include "nsIXPConnect.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsThreadUtils.h"
#include "prlog.h"

#include "nsDOMWorkerThread.h"
#include "nsDOMWorkerPool.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gDOMThreadsLog;
#endif
#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)





class nsDOMPostMessageRunnable : public nsRunnable
{
public:
  nsDOMPostMessageRunnable(const nsAString& aMessage,
                           nsDOMWorkerBase* aSource,
                           nsDOMWorkerBase* aTarget)
  : mMessage(aMessage), mSource(aSource), mTarget(aTarget) {
    NS_ASSERTION(aSource && aTarget, "Must specify both!");
  }

  NS_IMETHOD Run() {
#ifdef PR_LOGGING
    nsCAutoString utf8Message;
    utf8Message.AssignWithConversion(mMessage);

    static const char* poolStr = "pool";
    static const char* workerStr = "worker";

    nsCOMPtr<nsIDOMWorkerPool> sourceIsPool;
    mSource->QueryInterface(NS_GET_IID(nsIDOMWorkerPool),
                            getter_AddRefs(sourceIsPool));

    nsCOMPtr<nsIDOMWorkerPool> targetIsPool;
    mTarget->QueryInterface(NS_GET_IID(nsIDOMWorkerPool),
                            getter_AddRefs(targetIsPool));
#endif

    if (!(mTarget->IsCanceled() || mSource->IsCanceled())) {
      LOG(("Posting message '%s' from %s [0x%p] to %s [0x%p]",
           utf8Message.get(),
           sourceIsPool ? poolStr : workerStr,
           static_cast<void*>(mSource.get()),
           targetIsPool ? poolStr : workerStr,
           static_cast<void*>(mTarget.get())));

      mTarget->HandleMessage(mMessage, mSource);
    }

    return NS_OK;
  }

protected:
  nsString mMessage;
  nsRefPtr<nsDOMWorkerBase> mSource;
  nsRefPtr<nsDOMWorkerBase> mTarget;
};

nsresult
nsDOMWorkerBase::PostMessageInternal(const nsAString& aMessage,
                                     nsDOMWorkerBase* aSource)
{
  if (IsCanceled() || aSource->IsCanceled()) {
    return NS_OK;
  }

  nsRefPtr<nsDOMPostMessageRunnable> runnable =
    new nsDOMPostMessageRunnable(aMessage, aSource, this);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = DispatchMessage(runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDOMWorkerBase::PostMessageInternal(const nsAString& aMessage)
{
  nsRefPtr<nsDOMWorkerBase> source;
  JSContext *cx = nsContentUtils::GetCurrentJSContext();

  if (cx) {
    if (NS_IsMainThread()) {
      
      source = Pool();
    }
    else {
      
      nsRefPtr<nsDOMWorkerThread> worker =
        (nsDOMWorkerThread*)JS_GetContextPrivate(cx);

      
      nsRefPtr<nsDOMWorkerPool> sourcePool = worker->Pool();
      NS_ENSURE_TRUE(sourcePool == Pool(), NS_ERROR_NOT_AVAILABLE);

      source = worker;
    }
  }
  else {
    source = this;
  }

  nsresult rv = PostMessageInternal(aMessage, source);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsDOMWorkerBase::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOG(("Canceling %s [0x%p]", Pool() == this ? "pool" : "worker",
       static_cast<void*>(this)));

#ifdef DEBUG
  PRInt32 cancel =
#endif
  PR_AtomicSet(&mCanceled, 1);
  NS_ASSERTION(!cancel, "Canceled more than once?!");
}

void
nsDOMWorkerBase::Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOG(("Suspending %s [0x%p]", Pool() == this ? "pool" : "worker",
       static_cast<void*>(this)));

#ifdef DEBUG
  PRInt32 suspended =
#endif
  PR_AtomicSet(&mSuspended, 1);
  NS_ASSERTION(!suspended, "Suspended more than once?!");
}

void
nsDOMWorkerBase::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  LOG(("Resuming %s [0x%p]", Pool() == this ? "pool" : "worker",
       static_cast<void*>(this)));

#ifdef DEBUG
  PRInt32 suspended =
#endif
  PR_AtomicSet(&mSuspended, 0);
  NS_ASSERTION(suspended, "Not suspended!");
}
