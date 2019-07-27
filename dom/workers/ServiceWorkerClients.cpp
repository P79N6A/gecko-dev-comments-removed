




#include "ServiceWorkerClient.h"
#include "ServiceWorkerClients.h"
#include "ServiceWorkerManager.h"

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerClientsBinding.h"

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
  return ServiceWorkerClientsBinding::Wrap(aCx, this);
}

namespace {



class PromiseHolder MOZ_FINAL : public WorkerFeature
{
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PromiseHolder)

public:
  PromiseHolder(WorkerPrivate* aWorkerPrivate,
                Promise* aPromise)
    : mWorkerPrivate(aWorkerPrivate),
      mPromise(aPromise),
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
  Get() const
  {
    return mPromise;
  }

  void
  Clean()
  {
    mWorkerPrivate->AssertIsOnWorkerThread();

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

    Promise* promise = mPromiseHolder->Get();
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

class GetServicedRunnable MOZ_FINAL : public nsRunnable
{
  WorkerPrivate* mWorkerPrivate;
  nsCString mScope;
  nsRefPtr<PromiseHolder> mPromiseHolder;
public:
  GetServicedRunnable(WorkerPrivate* aWorkerPrivate,
                      Promise* aPromise,
                      const nsCString& aScope)
    : mWorkerPrivate(aWorkerPrivate),
      mScope(aScope)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
    mPromiseHolder = new PromiseHolder(aWorkerPrivate, aPromise);
  }

  NS_IMETHOD
  Run() MOZ_OVERRIDE
  {
    AssertIsOnMainThread();

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    nsAutoPtr<nsTArray<uint64_t>> result(new nsTArray<uint64_t>());

    swm->GetServicedClients(mScope, result);
    nsRefPtr<ResolvePromiseWorkerRunnable> r =
      new ResolvePromiseWorkerRunnable(mWorkerPrivate, mPromiseHolder, result);

    AutoSafeJSContext cx;
    r->Dispatch(cx);
    return NS_OK;
  }
};

} 

already_AddRefed<Promise>
ServiceWorkerClients::GetServiced(ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
  MOZ_ASSERT(workerPrivate);
  workerPrivate->AssertIsOnWorkerThread();

  DOMString scope;
  mWorkerScope->GetScope(scope);

  nsRefPtr<Promise> promise = Promise::Create(mWorkerScope, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<GetServicedRunnable> r =
    new GetServicedRunnable(workerPrivate, promise, NS_ConvertUTF16toUTF8(scope));
  nsresult rv = NS_DispatchToMainThread(r);

  if (NS_WARN_IF(NS_FAILED(rv))) {
    promise->MaybeReject(NS_ERROR_NOT_AVAILABLE);
  }

  return promise.forget();
}


already_AddRefed<Promise>
ServiceWorkerClients::ReloadAll(ErrorResult& aRv)
{
  nsRefPtr<Promise> promise = Promise::Create(mWorkerScope, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }
  promise->MaybeReject(NS_ERROR_NOT_AVAILABLE);
  return promise.forget();
}

