




#include "mozilla/dom/Promise.h"

#include "ServiceWorkerClient.h"
#include "ServiceWorkerClients.h"
#include "ServiceWorkerManager.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::workers;

NS_IMPL_CYCLE_COLLECTING_ADDREF(ServiceWorkerClients)
NS_IMPL_CYCLE_COLLECTING_RELEASE(ServiceWorkerClients)
NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(ServiceWorkerClients, mWorkerScope)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(ServiceWorkerClients)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

ServiceWorkerClients::ServiceWorkerClients(ServiceWorkerGlobalScope* aWorkerScope)
  : mWorkerScope(aWorkerScope)
{
  MOZ_ASSERT(mWorkerScope);
}

JSObject*
ServiceWorkerClients::WrapObject(JSContext* aCx)
{
  return ClientsBinding::Wrap(aCx, this);
}

namespace {



class PromiseHolder MOZ_FINAL : public WorkerFeature
{
  friend class MatchAllRunnable;

  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PromiseHolder)

public:
  PromiseHolder(WorkerPrivate* aWorkerPrivate,
                Promise* aPromise)
    : mWorkerPrivate(aWorkerPrivate),
      mPromise(aPromise),
      mCleanUpLock("promiseHolderCleanUpLock"),
      mClean(false)
  {
    MOZ_ASSERT(mWorkerPrivate);
    mWorkerPrivate->AssertIsOnWorkerThread();
    MOZ_ASSERT(mPromise);

    if (NS_WARN_IF(!mWorkerPrivate->AddFeature(mWorkerPrivate->GetJSContext(), this))) {
      
      
      
      mPromise = nullptr;
      mClean = true;
    }
  }

  Promise*
  GetPromise() const
  {
    return mPromise;
  }

  void
  Clean()
  {
    mWorkerPrivate->AssertIsOnWorkerThread();

    MutexAutoLock lock(mCleanUpLock);
    if (mClean) {
      return;
    }

    mPromise = nullptr;
    mWorkerPrivate->RemoveFeature(mWorkerPrivate->GetJSContext(), this);
    mClean = true;
  }

  bool
  Notify(JSContext* aCx, Status aStatus)
  {
    mWorkerPrivate->AssertIsOnWorkerThread();

    if (aStatus > Running) {
      Clean();
    }

    return true;
  }

private:
  ~PromiseHolder()
  {
    MOZ_ASSERT(mClean);
  }

  WorkerPrivate* mWorkerPrivate;
  nsRefPtr<Promise> mPromise;

  
  
  
  Mutex mCleanUpLock;

  bool mClean;
};

class ResolvePromiseWorkerRunnable MOZ_FINAL : public WorkerRunnable
{
  nsRefPtr<PromiseHolder> mPromiseHolder;
  nsAutoPtr<nsTArray<uint64_t>> mValue;

public:
  ResolvePromiseWorkerRunnable(WorkerPrivate* aWorkerPrivate,
                               PromiseHolder* aPromiseHolder,
                               nsAutoPtr<nsTArray<uint64_t>>& aValue)
    : WorkerRunnable(aWorkerPrivate, WorkerThreadModifyBusyCount),
      mPromiseHolder(aPromiseHolder),
      mValue(aValue)
  {
    AssertIsOnMainThread();
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    Promise* promise = mPromiseHolder->GetPromise();
    MOZ_ASSERT(promise);

    nsTArray<nsRefPtr<ServiceWorkerClient>> ret;
    for (size_t i = 0; i < mValue->Length(); i++) {
      ret.AppendElement(nsRefPtr<ServiceWorkerClient>(
            new ServiceWorkerClient(promise->GetParentObject(),
                                    mValue->ElementAt(i))));
    }
    promise->MaybeResolve(ret);

    
    mPromiseHolder->Clean();

    return true;
  }
};

class ReleasePromiseRunnable MOZ_FINAL : public MainThreadWorkerControlRunnable
{
  nsRefPtr<PromiseHolder> mPromiseHolder;

public:
  ReleasePromiseRunnable(WorkerPrivate* aWorkerPrivate,
                         PromiseHolder* aPromiseHolder)
    : MainThreadWorkerControlRunnable(aWorkerPrivate),
      mPromiseHolder(aPromiseHolder)
  { }

private:
  ~ReleasePromiseRunnable()
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();

    mPromiseHolder->Clean();

    return true;
  }

};

class MatchAllRunnable MOZ_FINAL : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;
  nsRefPtr<PromiseHolder> mPromiseHolder;
  nsCString mScope;
public:
  MatchAllRunnable(WorkerPrivate* aWorkerPrivate,
                   PromiseHolder* aPromiseHolder,
                   const nsCString& aScope)
    : mWorkerPrivate(aWorkerPrivate),
      mPromiseHolder(aPromiseHolder),
      mScope(aScope)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    MutexAutoLock lock(mPromiseHolder->mCleanUpLock);
    if (mPromiseHolder->mClean) {
      
      return NS_OK;
    }

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    nsAutoPtr<nsTArray<uint64_t>> result(new nsTArray<uint64_t>());

    swm->GetAllClients(mScope, result);
    nsRefPtr<ResolvePromiseWorkerRunnable> r =
      new ResolvePromiseWorkerRunnable(mWorkerPrivate, mPromiseHolder, result);

    AutoSafeJSContext cx;
    if (r->Dispatch(cx)) {
      return NS_OK;
    }

    
    
    nsRefPtr<ReleasePromiseRunnable> releaseRunnable =
      new ReleasePromiseRunnable(mWorkerPrivate, mPromiseHolder);

    if (!releaseRunnable->Dispatch(cx)) {
      NS_RUNTIMEABORT("Failed to dispatch PromiseHolder control runnable.");
    }

    return NS_OK;
  }
};

} 

already_AddRefed<Promise>
ServiceWorkerClients::MatchAll(const ClientQueryOptions& aOptions,
                               ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  nsString scope;
  mWorkerScope->GetScope(scope);

  if (aOptions.mIncludeUncontrolled || aOptions.mType != ClientType::Window) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(mWorkerScope, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<PromiseHolder> promiseHolder = new PromiseHolder(workerPrivate,
                                                            promise);
  if (!promiseHolder->GetPromise()) {
    
    return promise.forget();
  }

  nsRefPtr<MatchAllRunnable> r =
    new MatchAllRunnable(workerPrivate,
                         promiseHolder,
                         NS_ConvertUTF16toUTF8(scope));
  nsresult rv = NS_DispatchToMainThread(r);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_NOT_AVAILABLE);
  }

  return promise.forget();
}
