






































#ifndef __NSDOMTHREADSERVICE_H__
#define __NSDOMTHREADSERVICE_H__


#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "nsIThreadPool.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsRefPtrHashtable.h"
#include "nsStringGlue.h"
#include "nsTPtrArray.h"
#include "prmon.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gDOMThreadsLog;
#endif

class nsDOMWorker;
class nsDOMWorkerPool;
class nsDOMWorkerRunnable;
class nsDOMWorkerTimeout;
class nsIJSRuntimeService;
class nsIScriptGlobalObject;
class nsIThreadJSContextStack;
class nsIXPConnect;
class nsIXPCSecurityManager;

class nsDOMThreadService : public nsIEventTarget,
                           public nsIObserver,
                           public nsIThreadPoolListener
{
  friend class nsDOMWorker;
  friend class nsDOMWorkerNavigator;
  friend class nsDOMWorkerPool;
  friend class nsDOMWorkerRunnable;
  friend class nsDOMWorkerThread;
  friend class nsDOMWorkerTimeout;
  friend class nsDOMWorkerXHR;
  friend class nsDOMWorkerXHRProxy;
  friend class nsLayoutStatics;
  friend class nsReportErrorRunnable;

  friend void DOMWorkerErrorReporter(JSContext* aCx,
                                     const char* aMessage,
                                     JSErrorReport* aReport);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITHREADPOOLLISTENER

  
  static already_AddRefed<nsDOMThreadService> GetOrInitService();

  
  
  static nsDOMThreadService* get();

  static JSContext* GetCurrentContext();

  
  static nsIJSRuntimeService* JSRuntimeService();
  static nsIThreadJSContextStack* ThreadJSContextStack();
  static nsIXPCSecurityManager* WorkerSecurityManager();

  void CancelWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);
  void SuspendWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);
  void ResumeWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);

  nsresult ChangeThreadPoolMaxThreads(PRInt16 aDelta);

private:
  nsDOMThreadService();
  ~nsDOMThreadService();

  nsresult Init();
  void Cleanup();

  static void Shutdown();

  nsresult Dispatch(nsDOMWorker* aWorker,
                    nsIRunnable* aRunnable,
                    PRIntervalTime aTimeoutInterval = 0,
                    PRBool aClearQueue = PR_FALSE);

  void SetWorkerTimeout(nsDOMWorker* aWorker,
                        PRIntervalTime aTimeoutInterval);

  void WorkerComplete(nsDOMWorkerRunnable* aRunnable);

  static JSContext* CreateJSContext();

  already_AddRefed<nsDOMWorkerPool>
    GetPoolForGlobal(nsIScriptGlobalObject* aGlobalObject,
                     PRBool aRemove);

  void TriggerOperationCallbackForPool(nsDOMWorkerPool* aPool);
  void RescheduleSuspendedWorkerForPool(nsDOMWorkerPool* aPool);

  void NoteEmptyPool(nsDOMWorkerPool* aPool);

  void TimeoutReady(nsDOMWorkerTimeout* aTimeout);

  nsresult RegisterWorker(nsDOMWorker* aWorker,
                          nsIScriptGlobalObject* aGlobalObject);

  void GetAppName(nsAString& aAppName);
  void GetAppVersion(nsAString& aAppVersion);
  void GetPlatform(nsAString& aPlatform);
  void GetUserAgent(nsAString& aUserAgent);

  void RegisterPrefCallbacks();
  void UnregisterPrefCallbacks();

  static int PrefCallback(const char* aPrefName,
                          void* aClosure);

  static PRUint32 GetWorkerCloseHandlerTimeoutMS();

  PRBool QueueSuspendedWorker(nsDOMWorkerRunnable* aRunnable);

  
  nsCOMPtr<nsIThreadPool> mThreadPool;

  
  nsRefPtrHashtable<nsISupportsHashKey, nsDOMWorkerPool> mPools;

  
  
  PRMonitor* mMonitor;

  
  nsRefPtrHashtable<nsVoidPtrHashKey, nsDOMWorkerRunnable> mWorkersInProgress;

  
  
  nsTArray<JSContext*> mJSContexts;

  
  
  nsTArray<nsDOMWorkerRunnable*> mSuspendedWorkers;

  nsString mAppName;
  nsString mAppVersion;
  nsString mPlatform;
  nsString mUserAgent;

  PRBool mNavigatorStringsLoaded;
};

#endif 
