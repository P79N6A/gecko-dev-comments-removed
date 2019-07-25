






































#ifndef __NSDOMWORKERPOOL_H__
#define __NSDOMWORKERPOOL_H__


#include "jsapi.h"
#include "mozilla/Monitor.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsTArray.h"

class nsDOMWorker;
class nsIDocument;
class nsIScriptContext;
class nsIScriptError;
class nsIScriptGlobalObject;

class nsDOMWorkerPool
{
  typedef mozilla::Monitor Monitor;

public:
  nsDOMWorkerPool(nsIScriptGlobalObject* aGlobalObject,
                  nsIDocument* aDocument);

  NS_IMETHOD_(nsrefcnt) AddRef();
  NS_IMETHOD_(nsrefcnt) Release();

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

  Monitor& GetMonitor() {
    return mMonitor;
  }

  const PRUint64 WindowID() const {
    return mWindowID;
  }

private:
  virtual ~nsDOMWorkerPool();

  void GetWorkers(nsTArray<nsDOMWorker*>& aArray);

  nsAutoRefCnt mRefCnt;

  
  nsCOMPtr<nsIScriptGlobalObject> mParentGlobal;

  
  nsCOMPtr<nsIDocument> mParentDocument;

  
  
  nsTArray<nsDOMWorker*> mWorkers;

  
  Monitor mMonitor;

  PRPackedBool mCanceled;
  PRPackedBool mSuspended;

  const PRUint64 mWindowID;
};

#endif 
