




#include "DataStore.h"

#include "mozilla/dom/DataStore.h"
#include "mozilla/dom/DataStoreBinding.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/dom/WorkerNavigatorBinding.h"

#include "Navigator.h"
#include "nsProxyRelease.h"
#include "RuntimeService.h"

#include "nsIDocument.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

BEGIN_WORKERS_NAMESPACE

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(WorkerNavigator)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(WorkerNavigator, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(WorkerNavigator, Release)

 already_AddRefed<WorkerNavigator>
WorkerNavigator::Create(bool aOnLine)
{
  RuntimeService* rts = RuntimeService::GetService();
  MOZ_ASSERT(rts);

  const RuntimeService::NavigatorProperties& properties =
    rts->GetNavigatorProperties();

  nsRefPtr<WorkerNavigator> navigator =
    new WorkerNavigator(properties, aOnLine);

  return navigator.forget();
}

JSObject*
WorkerNavigator::WrapObject(JSContext* aCx)
{
  return WorkerNavigatorBinding_workers::Wrap(aCx, this);
}




class DataStoreAddEventListenerRunnable : public WorkerMainThreadRunnable
{
  nsMainThreadPtrHandle<DataStore> mBackingStore;
  DataStoreChangeEventProxy* mEventProxy;

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    
    if (NS_FAILED(mBackingStore->AddEventListener(NS_LITERAL_STRING("change"),
                                                  mEventProxy,
                                                  false,
                                                  false,
                                                  2))) {
      MOZ_ASSERT(false, "failed to add event listener!");
      return false;
    }

    return true;
  }

public:
  DataStoreAddEventListenerRunnable(
    WorkerPrivate* aWorkerPrivate,
    const nsMainThreadPtrHandle<DataStore>& aBackingStore,
    DataStoreChangeEventProxy* aEventProxy)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mBackingStore(aBackingStore)
    , mEventProxy(aEventProxy)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }
};

#define WORKER_DATA_STORES_TAG JS_SCTAG_USER_MIN

static JSObject*
GetDataStoresStructuredCloneCallbacksRead(JSContext* aCx,
                                          JSStructuredCloneReader* aReader,
                                          uint32_t aTag,
                                          uint32_t aData,
                                          void* aClosure)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  if (aTag != WORKER_DATA_STORES_TAG) {
    MOZ_ASSERT(false, "aTag must be WORKER_DATA_STORES_TAG!");
    return nullptr;
  }

  NS_ASSERTION(!aData, "aData should be empty");

  
  nsMainThreadPtrHolder<DataStore>* dataStoreholder;
  if (!JS_ReadBytes(aReader, &dataStoreholder, sizeof(dataStoreholder))) {
    MOZ_ASSERT(false, "cannot read bytes for dataStoreholder!");
    return nullptr;
  }

  
  JS::Rooted<JSObject*> workerStoreObj(aCx, nullptr);
  {
    nsRefPtr<WorkerDataStore> workerStore =
      new WorkerDataStore(workerPrivate->GlobalScope());
    nsMainThreadPtrHandle<DataStore> backingStore(dataStoreholder);

    
    nsRefPtr<DataStoreChangeEventProxy> eventProxy =
      new DataStoreChangeEventProxy(workerPrivate, workerStore);

    
    nsRefPtr<DataStoreAddEventListenerRunnable> runnable =
      new DataStoreAddEventListenerRunnable(workerPrivate,
                                            backingStore,
                                            eventProxy);
    runnable->Dispatch(aCx);

    
    workerStore->SetBackingDataStore(backingStore);

    JS::Rooted<JSObject*> global(aCx, JS::CurrentGlobalOrNull(aCx));
    if (!global) {
      MOZ_ASSERT(false, "cannot get global!");
    } else {
      workerStoreObj = workerStore->WrapObject(aCx);
      if (!JS_WrapObject(aCx, &workerStoreObj)) {
        MOZ_ASSERT(false, "cannot wrap object for workerStoreObj!");
        workerStoreObj = nullptr;
      }
    }
  }

  return workerStoreObj;
}

static bool
GetDataStoresStructuredCloneCallbacksWrite(JSContext* aCx,
                                           JSStructuredCloneWriter* aWriter,
                                           JS::Handle<JSObject*> aObj,
                                           void* aClosure)
{
  AssertIsOnMainThread();

  PromiseWorkerProxy* proxy = static_cast<PromiseWorkerProxy*>(aClosure);
  NS_ASSERTION(proxy, "must have proxy!");

  if (!JS_WriteUint32Pair(aWriter, WORKER_DATA_STORES_TAG, 0)) {
    MOZ_ASSERT(false, "cannot write pair for WORKER_DATA_STORES_TAG!");
    return false;
  }

  JS::Rooted<JSObject*> storeObj(aCx, aObj);

  DataStore* store = nullptr;
  nsresult rv = UNWRAP_OBJECT(DataStore, storeObj, store);
  if (NS_FAILED(rv)) {
    MOZ_ASSERT(false, "cannot unwrap the DataStore object!");
    return false;
  }

  
  proxy->StoreISupports(store);

  
  nsMainThreadPtrHolder<DataStore>* dataStoreholder =
    new nsMainThreadPtrHolder<DataStore>(store);

  
  if (!JS_WriteBytes(aWriter, &dataStoreholder, sizeof(dataStoreholder))) {
    MOZ_ASSERT(false, "cannot write bytes for dataStoreholder!");
    return false;
  }

  return true;
}

static JSStructuredCloneCallbacks kGetDataStoresStructuredCloneCallbacks = {
  GetDataStoresStructuredCloneCallbacksRead,
  GetDataStoresStructuredCloneCallbacksWrite,
  nullptr
};



class NavigatorGetDataStoresRunnable MOZ_FINAL : public WorkerMainThreadRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseWorkerProxy;
  const nsString mName;
  const nsString mOwner;
  ErrorResult& mRv;

public:
  NavigatorGetDataStoresRunnable(WorkerPrivate* aWorkerPrivate,
                                 Promise* aWorkerPromise,
                                 const nsAString& aName,
                                 const nsAString& aOwner,
                                 ErrorResult& aRv)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mName(aName)
    , mOwner(aOwner)
    , mRv(aRv)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    mPromiseWorkerProxy =
      new PromiseWorkerProxy(aWorkerPrivate,
                             aWorkerPromise,
                             &kGetDataStoresStructuredCloneCallbacks);
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    
    WorkerPrivate* wp = mWorkerPrivate;
    while (wp->GetParent()) {
      wp = wp->GetParent();
    }
    nsPIDOMWindow* window = wp->GetWindow();

    
    
    if (!window) {
      mRv.Throw(NS_ERROR_FAILURE);
      return false;
    }

    nsRefPtr<Promise> promise =
      Navigator::GetDataStores(window, mName, mOwner, mRv);
    promise->AppendNativeHandler(mPromiseWorkerProxy);
    return true;
  }
};

already_AddRefed<Promise>
WorkerNavigator::GetDataStores(JSContext* aCx,
                               const nsAString& aName,
                               const nsAString& aOwner,
                               ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<Promise> promise = Promise::Create(workerPrivate->GlobalScope(), aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<NavigatorGetDataStoresRunnable> runnable =
    new NavigatorGetDataStoresRunnable(workerPrivate, promise, aName, aOwner, aRv);
  runnable->Dispatch(aCx);

  return promise.forget();
}

void
WorkerNavigator::SetLanguages(const nsTArray<nsString>& aLanguages)
{
  WorkerNavigatorBinding_workers::ClearCachedLanguagesValue(this);
  mProperties.mLanguages = aLanguages;
}

void
WorkerNavigator::GetAppName(nsString& aAppName) const
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  if (!mProperties.mAppNameOverridden.IsEmpty() &&
      !workerPrivate->UsesSystemPrincipal()) {
    aAppName = mProperties.mAppNameOverridden;
  } else {
    aAppName = mProperties.mAppName;
  }
}

void
WorkerNavigator::GetAppVersion(nsString& aAppVersion) const
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  if (!mProperties.mAppVersionOverridden.IsEmpty() &&
      !workerPrivate->UsesSystemPrincipal()) {
    aAppVersion = mProperties.mAppVersionOverridden;
  } else {
    aAppVersion = mProperties.mAppVersion;
  }
}

void
WorkerNavigator::GetPlatform(nsString& aPlatform) const
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  if (!mProperties.mPlatformOverridden.IsEmpty() &&
      !workerPrivate->UsesSystemPrincipal()) {
    aPlatform = mProperties.mPlatformOverridden;
  } else {
    aPlatform = mProperties.mPlatform;
  }
}

namespace {

class GetUserAgentRunnable MOZ_FINAL : public WorkerMainThreadRunnable
{
  nsString& mUA;

public:
  GetUserAgentRunnable(WorkerPrivate* aWorkerPrivate, nsString& aUA)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mUA(aUA)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  virtual bool MainThreadRun() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    nsCOMPtr<nsPIDOMWindow> window = mWorkerPrivate->GetWindow();
    nsCOMPtr<nsIURI> uri;
    if (window && window->GetDocShell()) {
      nsIDocument* doc = window->GetExtantDoc();
      if (doc) {
        doc->NodePrincipal()->GetURI(getter_AddRefs(uri));
      }
    }

    bool isCallerChrome = mWorkerPrivate->UsesSystemPrincipal();
    nsresult rv = dom::Navigator::GetUserAgent(window, uri,
                                               isCallerChrome, mUA);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to retrieve user-agent from the worker thread.");
    }

    return true;
  }
};

} 

void
WorkerNavigator::GetUserAgent(nsString& aUserAgent) const
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  nsRefPtr<GetUserAgentRunnable> runnable =
    new GetUserAgentRunnable(workerPrivate, aUserAgent);

  if (!runnable->Dispatch(workerPrivate->GetJSContext())) {
    JS_ReportPendingException(workerPrivate->GetJSContext());
  }
}

END_WORKERS_NAMESPACE
