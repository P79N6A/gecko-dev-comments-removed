






































#ifndef __NSDOMWORKERPOOL_H__
#define __NSDOMWORKERPOOL_H__


#include "jsapi.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"
#include "prmon.h"

class nsDOMWorker;
class nsIDocument;
class nsIScriptContext;
class nsIScriptError;
class nsIScriptGlobalObject;

class nsDOMWorkerPool
{
public:
  nsDOMWorkerPool(nsIScriptGlobalObject* aGlobalObject,
                  nsIDocument* aDocument);

  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

  nsIScriptContext* ScriptContext();

  nsIScriptGlobalObject* ScriptGlobalObject() {
    return mParentGlobal;
  }

  nsIDocument* ParentDocument() {
    return mParentDocument;
  }

  nsresult Init();

  void Cancel();
  void Suspend();
  void Resume();

  nsresult NoteWorker(nsDOMWorker* aWorker);
  void NoteDyingWorker(nsDOMWorker* aWorker);

  PRMonitor* Monitor() {
    return mMonitor;
  }

private:
  virtual ~nsDOMWorkerPool();

  void GetWorkers(nsTArray<nsDOMWorker*>& aArray);

  nsAutoRefCnt mRefCnt;

  
  nsCOMPtr<nsIScriptGlobalObject> mParentGlobal;

  
  nsCOMPtr<nsIDocument> mParentDocument;

  
  
  nsTArray<nsDOMWorker*> mWorkers;

  
  PRMonitor* mMonitor;

  PRPackedBool mCanceled;
  PRPackedBool mSuspended;
};

#endif 
