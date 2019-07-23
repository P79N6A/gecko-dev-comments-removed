





































#ifndef __NSDOMWORKER_H__
#define __NSDOMWORKER_H__

#include "nsIDOMEventTarget.h"
#include "nsIDOMWorkers.h"
#include "nsIJSNativeInitializer.h"
#include "nsIXPCScriptable.h"

#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTPtrArray.h"
#include "prlock.h"

#include "nsDOMWorkerMessageHandler.h"

class nsDOMWorkerMessageHandler;
class nsDOMWorkerPool;
class nsDOMWorkerScope;
class nsDOMWorkerTimeout;
class nsICancelable;
class nsIDOMEventListener;
class nsIEventTarget;
class nsIScriptGlobalObject;
class nsIXPConnectWrappedNative;

class nsDOMWorkerFeature;

class nsDOMWorker : public nsIWorker,
                    public nsIJSNativeInitializer,
                    public nsIXPCScriptable
{
  friend class nsDOMWorkerFeature;
  friend class nsDOMWorkerFunctions;
  friend class nsDOMWorkerRefPtr;
  friend class nsDOMWorkerScope;
  friend class nsDOMWorkerScriptLoader;
  friend class nsDOMWorkerTimeout;
  friend class nsDOMWorkerXHR;

  friend JSBool DOMWorkerOperationCallback(JSContext* aCx);
  friend void DOMWorkerErrorReporter(JSContext* aCx,
                                     const char* aMessage,
                                     JSErrorReport* aReport);

#ifdef DEBUG
  
  friend class nsDOMFireEventRunnable;
#endif

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIABSTRACTWORKER
  NS_DECL_NSIWORKER
  NS_FORWARD_SAFE_NSIDOMEVENTTARGET(mOuterHandler)
  NS_DECL_NSIXPCSCRIPTABLE

  static nsresult NewWorker(nsISupports** aNewObject);

  nsDOMWorker(nsDOMWorker* aParent,
              nsIXPConnectWrappedNative* aParentWN);

  NS_IMETHOD Initialize(nsISupports* aOwner,
                        JSContext* aCx,
                        JSObject* aObj,
                        PRUint32 aArgc,
                        jsval* aArgv);

  nsresult InitializeInternal(nsIScriptGlobalObject* aOwner,
                              JSContext* aCx,
                              JSObject* aObj,
                              PRUint32 aArgc,
                              jsval* aArgv);

  void Cancel();
  void Suspend();
  void Resume();

  PRBool IsCanceled() {
    return mCanceled;
  }

  PRBool IsSuspended() {
    return mSuspended;
  }

  PRBool SetGlobalForContext(JSContext* aCx);

  void SetPool(nsDOMWorkerPool* aPool);

  nsDOMWorkerPool* Pool() {
    return mPool;
  }

  PRLock* Lock() {
    return mLock;
  }

  already_AddRefed<nsIXPConnectWrappedNative> GetWrappedNative();
  already_AddRefed<nsDOMWorker> GetParent();

  nsDOMWorkerScope* GetInnerScope() {
    return mInnerScope;
  }

private:
  ~nsDOMWorker();

  nsresult PostMessageInternal(const nsAString& aMessage,
                               PRBool aToInner);

  PRBool CompileGlobalObject(JSContext* aCx);

  PRUint32 NextTimeoutId() {
    return mNextTimeoutId++;
  }

  nsresult AddFeature(nsDOMWorkerFeature* aFeature,
                      JSContext* aCx);
  void RemoveFeature(nsDOMWorkerFeature* aFeature,
                     JSContext* aCx);
  void CancelTimeoutWithId(PRUint32 aId);
  void SuspendFeatures();
  void ResumeFeatures();
  void CancelFeatures();

private:

  
  
  nsDOMWorker* mParent;
  nsCOMPtr<nsIXPConnectWrappedNative> mParentWN;

  PRUint32 mCallbackCount;

  PRLock* mLock;

  nsRefPtr<nsDOMWorkerMessageHandler> mInnerHandler;
  nsRefPtr<nsDOMWorkerMessageHandler> mOuterHandler;

  nsRefPtr<nsDOMWorkerPool> mPool;

  nsDOMWorkerScope* mInnerScope;
  JSObject* mGlobal;

  PRUint32 mNextTimeoutId;

  nsTArray<nsDOMWorkerFeature*> mFeatures;
  PRUint32 mFeatureSuspendDepth;

  nsString mScriptURL;

  nsIXPConnectWrappedNative* mWrappedNative;

  PRPackedBool mCanceled;
  PRPackedBool mSuspended;
  PRPackedBool mCompileAttempted;
  PRPackedBool mTerminated;
};












class nsDOMWorkerFeature : public nsISupports
{
  friend class nsDOMWorker;

public:
  NS_DECL_ISUPPORTS

  nsDOMWorkerFeature(nsDOMWorker* aWorker)
  : mWorker(aWorker), mWorkerWN(aWorker->GetWrappedNative()), mId(0),
    mHasId(PR_FALSE), mFreeToDie(PR_TRUE) { }

  nsDOMWorkerFeature(nsDOMWorker* aWorker, PRUint32 aId)
  : mWorker(aWorker), mWorkerWN(aWorker->GetWrappedNative()), mId(aId),
    mHasId(PR_TRUE), mFreeToDie(PR_TRUE) { }

  virtual void Cancel() = 0;
  virtual void Suspend() { }
  virtual void Resume() { }

  PRUint32 GetId() {
    return mId;
  }

  PRBool HasId() {
    return mHasId;
  }

protected:
  virtual ~nsDOMWorkerFeature() { }

private:
  void FreeToDie(PRBool aFreeToDie) {
    mFreeToDie = aFreeToDie;
  }

protected:
  nsRefPtr<nsDOMWorker> mWorker;
  nsCOMPtr<nsIXPConnectWrappedNative> mWorkerWN;
  PRUint32 mId;

private:
  PRPackedBool mHasId;
  PRPackedBool mFreeToDie;
};

#endif 
