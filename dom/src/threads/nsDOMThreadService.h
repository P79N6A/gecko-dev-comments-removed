






































#ifndef __NSDOMTHREADSERVICE_H__
#define __NSDOMTHREADSERVICE_H__


#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "nsIThreadPool.h"
#include "nsIDOMThreads.h"


#include "jsapi.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsRefPtrHashtable.h"
#include "nsTPtrArray.h"
#include "prmon.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gDOMThreadsLog;
#endif

class nsDOMWorkerPool;
class nsDOMWorkerRunnable;
class nsDOMWorkerThread;
class nsDOMWorkerTimeout;
class nsIJSRuntimeService;
class nsIScriptGlobalObject;
class nsIThreadJSContextStack;
class nsIXPConnect;
class nsIXPCSecurityManager;

class nsDOMThreadService : public nsIEventTarget,
                           public nsIObserver,
                           public nsIThreadPoolListener,
                           public nsIDOMThreadService
{
  friend class nsDOMWorkerPool;
  friend class nsDOMWorkerRunnable;
  friend class nsDOMWorkerThread;
  friend class nsDOMWorkerTimeout;
  friend class nsDOMWorkerXHR;
  friend class nsDOMWorkerXHRProxy;
  friend class nsLayoutStatics;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIEVENTTARGET
  NS_DECL_NSIOBSERVER
  NS_DECL_NSITHREADPOOLLISTENER
  NS_DECL_NSIDOMTHREADSERVICE

  
  static already_AddRefed<nsIDOMThreadService> GetOrInitService();

  
  
  static nsDOMThreadService* get();

  
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

  nsresult Dispatch(nsDOMWorkerThread* aWorker,
                    nsIRunnable* aRunnable);

  void WorkerComplete(nsDOMWorkerRunnable* aRunnable);

  void WaitForCanceledWorker(nsDOMWorkerThread* aWorker);

  static JSContext* CreateJSContext();

  void NoteDyingPool(nsDOMWorkerPool* aPool);

  void TimeoutReady(nsDOMWorkerTimeout* aTimeout);

  
  nsCOMPtr<nsIThreadPool> mThreadPool;

  
  nsTPtrArray<nsDOMWorkerPool> mPools;

  
  
  PRMonitor* mMonitor;

  
  nsRefPtrHashtable<nsVoidPtrHashKey, nsDOMWorkerRunnable> mWorkersInProgress;
};

#endif 
