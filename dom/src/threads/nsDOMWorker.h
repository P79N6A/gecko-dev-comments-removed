





































#ifndef __NSDOMWORKER_H__
#define __NSDOMWORKER_H__

#include "nsIDOMEventTarget.h"
#include "nsIDOMWorkers.h"
#include "nsIJSNativeInitializer.h"
#include "nsIPrincipal.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsIXPCScriptable.h"

#include "jsapi.h"
#include "mozilla/Mutex.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsTPtrArray.h"

#include "nsDOMWorkerMessageHandler.h"


#define NS_WORKERFACTORY_CID \
 {0x1295efb5, 0x8644, 0x42b2, \
  {0x8b, 0x8e, 0x80, 0xee, 0xf5, 0x6e, 0x42, 0x84} }

class nsDOMWorker;
class nsDOMWorkerFeature;
class nsDOMWorkerMessageHandler;
class nsDOMWorkerNavigator;
class nsDOMWorkerPool;
class nsDOMWorkerTimeout;
class nsICancelable;
class nsIDOMEventListener;
class nsIEventTarget;
class nsIRunnable;
class nsIScriptGlobalObject;
class nsIXPConnectWrappedNative;

class nsDOMWorkerScope : public nsDOMWorkerMessageHandler,
                         public nsIWorkerScope,
                         public nsIXPCScriptable
{
  friend class nsDOMWorker;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMEVENTTARGET
  
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture,
                              PRBool aWantsUntrusted,
                              PRUint8 optional_argc);
  NS_DECL_NSIWORKERGLOBALSCOPE
  NS_DECL_NSIWORKERSCOPE
  NS_DECL_NSIXPCSCRIPTABLE
  NS_DECL_NSICLASSINFO

  typedef NS_STDCALL_FUNCPROTO(nsresult, SetListenerFunc, nsDOMWorkerScope,
                               SetOnmessage, (nsIDOMEventListener*));

  nsDOMWorkerScope(nsDOMWorker* aWorker);

protected:
  already_AddRefed<nsIXPConnectWrappedNative> GetWrappedNative();

private:
  nsDOMWorker* mWorker;
  nsIXPConnectWrappedNative* mWrappedNative;

  nsRefPtr<nsDOMWorkerNavigator> mNavigator;

  PRPackedBool mHasOnerror;
};

class nsLazyAutoRequest
{
public:
  nsLazyAutoRequest() : mCx(nsnull) {}

  ~nsLazyAutoRequest() {
    if (mCx)
      JS_EndRequest(mCx);
  }

  void enter(JSContext *aCx) {
    JS_BeginRequest(aCx);
    mCx = aCx;
  }

  bool entered() const { return mCx != nsnull; }

  void swap(nsLazyAutoRequest &other) {
    JSContext *tmp = mCx;
    mCx = other.mCx;
    other.mCx = tmp;
  }

private:
  JSContext *mCx;
};

class nsDOMWorker : public nsDOMWorkerMessageHandler,
                    public nsIWorker,
                    public nsITimerCallback,
                    public nsIJSNativeInitializer,
                    public nsIXPCScriptable
{
  typedef mozilla::Mutex Mutex;

  friend class nsDOMWorkerFeature;
  friend class nsDOMWorkerFunctions;
  friend class nsDOMWorkerScope;
  friend class nsDOMWorkerScriptLoader;
  friend class nsDOMWorkerTimeout;
  friend class nsDOMWorkerXHR;
  friend class nsDOMWorkerXHRProxy;
  friend class nsReportErrorRunnable;
  friend class nsDOMFireEventRunnable;

  friend JSBool DOMWorkerOperationCallback(JSContext* aCx);
  friend void DOMWorkerErrorReporter(JSContext* aCx,
                                     const char* aMessage,
                                     JSErrorReport* aReport);

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMEVENTTARGET
  
  NS_IMETHOD AddEventListener(const nsAString& aType,
                              nsIDOMEventListener* aListener,
                              PRBool aUseCapture,
                              PRBool aWantsUntrusted,
                              PRUint8 optional_argc);
  NS_DECL_NSIABSTRACTWORKER
  NS_DECL_NSIWORKER
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIXPCSCRIPTABLE

  static nsresult NewWorker(nsISupports** aNewObject);
  static nsresult NewChromeWorker(nsISupports** aNewObject);
  static nsresult NewChromeDOMWorker(nsDOMWorker** aNewObject);

  enum WorkerPrivilegeModel { CONTENT, CHROME };

  nsDOMWorker(nsDOMWorker* aParent,
              nsIXPConnectWrappedNative* aParentWN,
              WorkerPrivilegeModel aModel);

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
  void Kill();
  void Suspend();
  void Resume();

  
  PRBool IsCanceled();

  PRBool IsClosing();
  PRBool IsSuspended();

  PRBool SetGlobalForContext(JSContext* aCx, nsLazyAutoRequest *aRequest, JSAutoEnterCompartment *aComp);

  void SetPool(nsDOMWorkerPool* aPool);

  nsDOMWorkerPool* Pool() {
    return mPool;
  }

  Mutex& GetLock() {
    return mLock;
  }

  already_AddRefed<nsIXPConnectWrappedNative> GetWrappedNative();
  already_AddRefed<nsDOMWorker> GetParent();

  nsDOMWorkerScope* GetInnerScope() {
    return mInnerScope;
  }

  void SetExpirationTime(PRIntervalTime aExpirationTime);
#ifdef DEBUG
  PRIntervalTime GetExpirationTime();
#endif

  PRBool IsPrivileged() {
    return mPrivilegeModel == CHROME;
  }

  static JSObject* ReadStructuredClone(JSContext* aCx,
                                       JSStructuredCloneReader* aReader,
                                       uint32 aTag,
                                       uint32 aData,
                                       void* aClosure);

  















  enum DOMWorkerStatus {
    
    eRunning = 0,

    
    
    
    
    
    
    
    
    eClosed,

    
    
    
    
    
    
    
    eTerminated,

    
    
    
    
    
    
    eCanceled,

    
    eKilled
  };

private:
  ~nsDOMWorker();

  nsresult PostMessageInternal(PRBool aToInner);

  PRBool CompileGlobalObject(JSContext* aCx, nsLazyAutoRequest *aRequest, JSAutoEnterCompartment *aComp);

  PRUint32 NextTimeoutId() {
    return ++mNextTimeoutId;
  }

  nsresult AddFeature(nsDOMWorkerFeature* aFeature,
                      JSContext* aCx);
  void RemoveFeature(nsDOMWorkerFeature* aFeature,
                     JSContext* aCx);
  void CancelTimeoutWithId(PRUint32 aId);
  void SuspendFeatures();
  void ResumeFeatures();

  nsIPrincipal* GetPrincipal() {
    return mPrincipal;
  }

  void SetPrincipal(nsIPrincipal* aPrincipal);

  nsIURI* GetBaseURI() {
    return mBaseURI;
  }

  nsresult SetBaseURI(nsIURI* aURI);

  void ClearBaseURI();

  nsresult FireCloseRunnable(PRIntervalTime aTimeoutInterval,
                             PRBool aClearQueue,
                             PRBool aFromFinalize);
  nsresult Close();

  nsresult TerminateInternal(PRBool aFromFinalize);

  nsIWorkerLocation* GetLocation() {
    return mLocation;
  }

  PRBool QueueSuspendedRunnable(nsIRunnable* aRunnable);

  
  
  PRBool IsCanceledNoLock();

private:

  
  
  nsDOMWorker* mParent;
  nsCOMPtr<nsIXPConnectWrappedNative> mParentWN;

  
  
  WorkerPrivilegeModel mPrivilegeModel;

  Mutex mLock;

  nsRefPtr<nsDOMWorkerPool> mPool;

  nsDOMWorkerScope* mInnerScope;
  nsCOMPtr<nsIXPConnectWrappedNative> mScopeWN;
  JSObject* mGlobal;

  PRUint32 mNextTimeoutId;

  nsTArray<nsDOMWorkerFeature*> mFeatures;
  PRUint32 mFeatureSuspendDepth;

  nsString mScriptURL;

  nsIXPConnectWrappedNative* mWrappedNative;

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsIURI> mBaseURI;

  PRInt32 mErrorHandlerRecursionCount;

  
  DOMWorkerStatus mStatus;

  
  PRIntervalTime mExpirationTime;

  nsCOMPtr<nsITimer> mKillTimer;

  nsCOMPtr<nsIWorkerLocation> mLocation;

  nsTArray<nsCOMPtr<nsIRunnable> > mQueuedRunnables;

  PRPackedBool mSuspended;
  PRPackedBool mCompileAttempted;
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

class nsWorkerFactory : public nsIWorkerFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIWORKERFACTORY
};

#endif
