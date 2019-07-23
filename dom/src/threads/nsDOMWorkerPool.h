






































#ifndef __NSDOMWORKERPOOL_H__
#define __NSDOMWORKERPOOL_H__


#include "nsDOMWorkerBase.h"
#include "nsIClassInfo.h"
#include "nsIDOMThreads.h"


#include "jsapi.h"
#include "nsStringGlue.h"
#include "nsTPtrArray.h"
#include "prmon.h"

class nsDOMWorkerThread;
class nsIDocument;
class nsIScriptContext;
class nsIScriptError;
class nsIScriptGlobalObject;




class nsDOMWorkerPool : public nsDOMWorkerBase,
                        public nsIDOMWorkerPool,
                        public nsIClassInfo
{
  friend class nsDOMThreadService;
  friend class nsDOMWorkerFunctions;
  friend class nsDOMWorkerPoolWeakRef;
  friend class nsDOMWorkerScriptLoader;
  friend class nsDOMWorkerStreamObserver;
  friend class nsDOMWorkerThread;
  friend class nsReportErrorRunnable;
  friend JSBool DOMWorkerOperationCallback(JSContext* aCx);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMWORKERPOOL
  NS_DECL_NSICLASSINFO

  nsDOMWorkerPool(nsIDocument* aDocument);

  
  virtual nsDOMWorkerPool* Pool() {
    return this;
  }

  nsIDocument* ParentDocument();
  nsIScriptContext* ScriptContext();

private:
  virtual ~nsDOMWorkerPool();

  nsresult Init();

  
  virtual nsresult HandleMessage(const nsAString& aMessage,
                                 nsDOMWorkerBase* aSourceThread);

  
  virtual nsresult DispatchMessage(nsIRunnable* aRunnable);

  void HandleError(nsIScriptError* aError,
                   nsDOMWorkerThread* aSource);

  void NoteDyingWorker(nsDOMWorkerThread* aWorker);

  void CancelWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);
  void SuspendWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);
  void ResumeWorkersForGlobal(nsIScriptGlobalObject* aGlobalObject);

  PRMonitor* Monitor() {
    return mMonitor;
  }

  
  nsISupports* mParentGlobal;

  
  nsIDocument* mParentDocument;

  
  
  nsTPtrArray<nsDOMWorkerThread> mWorkers;

  
  nsCOMPtr<nsIDOMWorkerErrorListener> mErrorListener;

  
  PRMonitor* mMonitor;
};

#endif 
