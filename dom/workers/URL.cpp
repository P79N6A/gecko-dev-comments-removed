




#include "URL.h"
#include "File.h"

#include "nsTraceRefcnt.h"

#include "WorkerPrivate.h"
#include "nsThreadUtils.h"

#include "nsPIDOMWindow.h"
#include "nsGlobalWindow.h"
#include "nsHostObjectProtocolHandler.h"

#include "nsIDOMFile.h"

USING_WORKERS_NAMESPACE
using mozilla::dom::WorkerGlobalObject;


class URLRunnable : public nsRunnable
{
protected:
  WorkerPrivate* mWorkerPrivate;
  uint32_t mSyncQueueKey;

private:
  class ResponseRunnable : public WorkerSyncRunnable
  {
    uint32_t mSyncQueueKey;

  public:
    ResponseRunnable(WorkerPrivate* aWorkerPrivate,
                     uint32_t aSyncQueueKey)
    : WorkerSyncRunnable(aWorkerPrivate, aSyncQueueKey, false),
      mSyncQueueKey(aSyncQueueKey)
    {
      NS_ASSERTION(aWorkerPrivate, "Don't hand me a null WorkerPrivate!");
    }

    bool
    WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      aWorkerPrivate->StopSyncLoop(mSyncQueueKey, true);
      return true;
    }

    bool
    PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
    {
      AssertIsOnMainThread();
      return true;
    }

    void
    PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
                 bool aDispatchResult)
    {
      AssertIsOnMainThread();
    }
  };

protected:
  URLRunnable(WorkerPrivate* aWorkerPrivate)
  : mWorkerPrivate(aWorkerPrivate)
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
  }

public:
  bool
  Dispatch(JSContext* aCx)
  {
    mWorkerPrivate->AssertIsOnWorkerThread();
    mSyncQueueKey = mWorkerPrivate->CreateNewSyncLoop();

    if (NS_FAILED(NS_DispatchToMainThread(this, NS_DISPATCH_NORMAL))) {
      JS_ReportError(aCx, "Failed to dispatch to main thread!");
      mWorkerPrivate->StopSyncLoop(mSyncQueueKey, false);
      return false;
    }

    return mWorkerPrivate->RunSyncLoop(aCx, mSyncQueueKey);
  }

private:
  NS_IMETHOD Run()
  {
    AssertIsOnMainThread();

    MainThreadRun();

    nsRefPtr<ResponseRunnable> response =
      new ResponseRunnable(mWorkerPrivate, mSyncQueueKey);
    if (!response->Dispatch(nullptr)) {
      NS_WARNING("Failed to dispatch response!");
    }

    return NS_OK;
  }

protected:
  virtual void
  MainThreadRun() = 0;
};


class CreateURLRunnable : public URLRunnable
{
private:
  nsIDOMBlob* mBlob;
  nsString& mURL;

public:
  CreateURLRunnable(WorkerPrivate* aWorkerPrivate, nsIDOMBlob* aBlob,
                    const mozilla::dom::objectURLOptionsWorkers& aOptions,
                    nsString& aURL)
  : URLRunnable(aWorkerPrivate),
    mBlob(aBlob),
    mURL(aURL)
  {
    MOZ_ASSERT(aBlob);
  }

  void
  MainThreadRun()
  {
    AssertIsOnMainThread();

    nsCOMPtr<nsPIDOMWindow> window = mWorkerPrivate->GetWindow();
    nsIDocument* doc = window->GetExtantDoc();
    if (!doc) {
      SetDOMStringToNull(mURL);
      return;
    }

    nsCString url;
    nsresult rv = nsHostObjectProtocolHandler::AddDataEntry(
        NS_LITERAL_CSTRING(BLOBURI_SCHEME),
        mBlob, doc->NodePrincipal(), url);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to add data entry for the blob!");
      SetDOMStringToNull(mURL);
      return;
    }

    doc->RegisterHostObjectUri(url);
    mURL = NS_ConvertUTF8toUTF16(url);
  }
};


class RevokeURLRunnable : public URLRunnable
{
private:
  const nsString mURL;

public:
  RevokeURLRunnable(WorkerPrivate* aWorkerPrivate,
                    const nsAString& aURL)
  : URLRunnable(aWorkerPrivate),
    mURL(aURL)
  {}

  void
  MainThreadRun()
  {
    AssertIsOnMainThread();

    nsCOMPtr<nsPIDOMWindow> window = mWorkerPrivate->GetWindow();
    nsIDocument* doc = window->GetExtantDoc();
    if (!doc) {
      return;
    }

    NS_ConvertUTF16toUTF8 url(mURL);

    nsIPrincipal* principal =
      nsHostObjectProtocolHandler::GetDataEntryPrincipal(url);

    bool subsumes;
    if (principal &&
        NS_SUCCEEDED(doc->NodePrincipal()->Subsumes(principal, &subsumes)) &&
        subsumes) {
      doc->UnregisterHostObjectUri(url);
      nsHostObjectProtocolHandler::RemoveDataEntry(url);
    }
  }
};


void
URL::CreateObjectURL(const WorkerGlobalObject& aGlobal, JSObject* aBlob,
                     const mozilla::dom::objectURLOptionsWorkers& aOptions,
                     nsString& aResult, mozilla::ErrorResult& aRv)
{
  JSContext* cx = aGlobal.GetContext();
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(cx);

  nsCOMPtr<nsIDOMBlob> blob = file::GetDOMBlobFromJSObject(aBlob);
  if (!blob) {
    SetDOMStringToNull(aResult);
    aRv.Throw(NS_ERROR_TYPE_ERR);
    return;
  }

  nsRefPtr<CreateURLRunnable> runnable =
    new CreateURLRunnable(workerPrivate, blob, aOptions, aResult);

  if (!runnable->Dispatch(cx)) {
    JS_ReportPendingException(cx);
  }
}


void
URL::RevokeObjectURL(const WorkerGlobalObject& aGlobal, const nsAString& aUrl)
{
  JSContext* cx = aGlobal.GetContext();
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(cx);

  nsRefPtr<RevokeURLRunnable> runnable =
    new RevokeURLRunnable(workerPrivate, aUrl);

  if (!runnable->Dispatch(cx)) {
    JS_ReportPendingException(cx);
  }
}

