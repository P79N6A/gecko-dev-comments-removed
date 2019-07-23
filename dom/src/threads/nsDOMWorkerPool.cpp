






































#include "nsDOMWorkerPool.h"


#include "nsIDocument.h"
#include "nsIDOMClassInfo.h"
#include "nsIJSContextStack.h"
#include "nsIScriptGlobalObject.h"
#include "nsIServiceManager.h"
#include "nsIThreadManager.h"
#include "nsIXPConnect.h"
#include "nsPIDOMWindow.h"


#include "nsAutoLock.h"
#include "nsContentUtils.h"
#include "nsDOMJSUtils.h"
#include "nsProxyRelease.h"
#include "nsThreadUtils.h"


#include "nsDOMThreadService.h"
#include "nsDOMWorker.h"

#define LOG(_args) PR_LOG(gDOMThreadsLog, PR_LOG_DEBUG, _args)

nsDOMWorkerPool::nsDOMWorkerPool(nsIScriptGlobalObject* aGlobalObject,
                                 nsIDocument* aDocument)
: mParentGlobal(aGlobalObject),
  mParentDocument(aDocument),
  mMonitor(nsnull),
  mCanceled(PR_FALSE),
  mSuspended(PR_FALSE)
{
  NS_ASSERTION(aGlobalObject, "Must have a global object!");
  NS_ASSERTION(aDocument, "Must have a document!");
}

nsDOMWorkerPool::~nsDOMWorkerPool()
{
  nsCOMPtr<nsIThread> mainThread;
  NS_GetMainThread(getter_AddRefs(mainThread));

  nsIScriptGlobalObject* global;
  mParentGlobal.forget(&global);
  if (global) {
    NS_ProxyRelease(mainThread, global, PR_FALSE);
  }

  nsIDocument* document;
  mParentDocument.forget(&document);
  if (document) {
    NS_ProxyRelease(mainThread, document, PR_FALSE);
  }

  if (mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

NS_IMPL_THREADSAFE_ADDREF(nsDOMWorkerPool)
NS_IMPL_THREADSAFE_RELEASE(nsDOMWorkerPool)

nsresult
nsDOMWorkerPool::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  mMonitor = nsAutoMonitor::NewMonitor("nsDOMWorkerPool::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
nsDOMWorkerPool::NoteWorker(nsDOMWorker* aWorker)
{
  NS_ASSERTION(aWorker, "Null pointer!");

  PRBool suspendWorker;

  {
    nsAutoMonitor mon(mMonitor);

    if (mCanceled) {
      return NS_ERROR_ABORT;
    }

    nsDOMWorker** newWorker = mWorkers.AppendElement(aWorker);
    NS_ENSURE_TRUE(newWorker, NS_ERROR_OUT_OF_MEMORY);

    suspendWorker = mSuspended;
  }

  if (suspendWorker) {
    aWorker->Suspend();
  }

  return NS_OK;
}

void
nsDOMWorkerPool::NoteDyingWorker(nsDOMWorker* aWorker)
{
  NS_ASSERTION(aWorker, "Null pointer!");

  PRBool removeFromThreadService = PR_FALSE;

  {
    nsAutoMonitor mon(mMonitor);

    NS_ASSERTION(mWorkers.Contains(aWorker), "Worker from a different pool?!");
    mWorkers.RemoveElement(aWorker);

    if (!mCanceled && !mWorkers.Length()) {
      removeFromThreadService = PR_TRUE;
    }
  }

  if (removeFromThreadService) {
    nsRefPtr<nsDOMWorkerPool> kungFuDeathGrip(this);
    nsDOMThreadService::get()->NoteEmptyPool(this);
  }
}

void
nsDOMWorkerPool::GetWorkers(nsTArray<nsDOMWorker*>& aArray)
{
  PR_ASSERT_CURRENT_THREAD_IN_MONITOR(mMonitor);
  NS_ASSERTION(!aArray.Length(), "Should be empty!");

#ifdef DEBUG
  nsDOMWorker** newWorkers =
#endif
  aArray.AppendElements(mWorkers);
  NS_WARN_IF_FALSE(newWorkers, "Out of memory!");
}

void
nsDOMWorkerPool::Cancel()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ASSERTION(!mCanceled, "Canceled more than once!");

  nsAutoTArray<nsDOMWorker*, 10> workers;
  {
    nsAutoMonitor mon(mMonitor);

    mCanceled = PR_TRUE;

    GetWorkers(workers);
  }

  PRUint32 count = workers.Length();
  if (count) {
    for (PRUint32 index = 0; index < count; index++) {
      workers[index]->Cancel();
    }
    nsAutoMonitor mon(mMonitor);
    mon.NotifyAll();
  }
}

void
nsDOMWorkerPool::Suspend()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsAutoTArray<nsDOMWorker*, 10> workers;
  {
    nsAutoMonitor mon(mMonitor);

    NS_ASSERTION(!mSuspended, "Suspended more than once!");
    mSuspended = PR_TRUE;

    GetWorkers(workers);
  }

  PRUint32 count = workers.Length();
  for (PRUint32 index = 0; index < count; index++) {
    workers[index]->Suspend();
  }
}

void
nsDOMWorkerPool::Resume()
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsAutoTArray<nsDOMWorker*, 10> workers;
  {
    nsAutoMonitor mon(mMonitor);

    NS_ASSERTION(mSuspended, "Not suspended!");
    mSuspended = PR_FALSE;

    GetWorkers(workers);
  }

  PRUint32 count = workers.Length();
  if (count) {
    for (PRUint32 index = 0; index < count; index++) {
      workers[index]->Resume();
    }
    nsAutoMonitor mon(mMonitor);
    mon.NotifyAll();
  }
}
