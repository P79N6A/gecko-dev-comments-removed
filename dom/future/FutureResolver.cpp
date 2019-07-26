





#include "mozilla/dom/FutureResolver.h"
#include "mozilla/dom/FutureBinding.h"
#include "mozilla/dom/Future.h"
#include "FutureCallback.h"

namespace mozilla {
namespace dom {




class FutureResolverTask MOZ_FINAL : public nsRunnable
{
public:
  FutureResolverTask(FutureResolver* aResolver,
                     const JS::Handle<JS::Value> aValue,
                     Future::FutureState aState)
    : mResolver(aResolver)
    , mValue(aValue)
    , mState(aState)
  {
    MOZ_ASSERT(aResolver);
    MOZ_ASSERT(mState != Future::Pending);
    MOZ_COUNT_CTOR(FutureResolverTask);

    JSContext* cx = nsContentUtils::GetSafeJSContext();
    JS_AddNamedValueRootRT(JS_GetRuntime(cx), &mValue,
                           "FutureResolverTask.mValue");
  }

  ~FutureResolverTask()
  {
    MOZ_COUNT_DTOR(FutureResolverTask);

    JSContext* cx = nsContentUtils::GetSafeJSContext();
    JS_RemoveValueRootRT(JS_GetRuntime(cx), &mValue);
  }

  NS_IMETHOD Run()
  {
    mResolver->RunTask(JS::Handle<JS::Value>::fromMarkedLocation(&mValue),
                       mState, FutureResolver::SyncTask);
    return NS_OK;
  }

private:
  nsRefPtr<FutureResolver> mResolver;
  JS::Value mValue;
  Future::FutureState mState;
};



NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(FutureResolver, mFuture)

NS_IMPL_CYCLE_COLLECTING_ADDREF(FutureResolver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(FutureResolver)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(FutureResolver)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

FutureResolver::FutureResolver(Future* aFuture)
  : mFuture(aFuture)
  , mResolvePending(false)
{
  SetIsDOMBinding();
}

JSObject*
FutureResolver::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return FutureResolverBinding::Wrap(aCx, aScope, this);
}

void
FutureResolver::Resolve(JSContext* aCx,
                        const Optional<JS::Handle<JS::Value> >& aValue,
                        FutureTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  ResolveInternal(aCx, aValue, aAsynchronous);
}

void
FutureResolver::ResolveInternal(JSContext* aCx,
                                const Optional<JS::Handle<JS::Value> >& aValue,
                                FutureTaskSync aAsynchronous)
{
  mResolvePending = true;

  
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> valueObj(aCx, &aValue.Value().toObject());
    Future* nextFuture;
    nsresult rv = UnwrapObject<Future>(aCx, valueObj, nextFuture);

    if (NS_SUCCEEDED(rv)) {
      nsRefPtr<FutureCallback> resolveCb = new ResolveFutureCallback(this);
      nsRefPtr<FutureCallback> rejectCb = new RejectFutureCallback(this);
      nextFuture->AppendCallbacks(resolveCb, rejectCb);
      return;
    }
  }

  
  
  
  
  RunTask(aValue.WasPassed() ? aValue.Value() : JS::UndefinedHandleValue,
          Future::Resolved, aAsynchronous);
}

void
FutureResolver::Reject(JSContext* aCx,
                       const Optional<JS::Handle<JS::Value> >& aValue,
                       FutureTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  RejectInternal(aCx, aValue, aAsynchronous);
}

void
FutureResolver::RejectInternal(JSContext* aCx,
                               const Optional<JS::Handle<JS::Value> >& aValue,
                               FutureTaskSync aAsynchronous)
{
  mResolvePending = true;

  
  
  
  RunTask(aValue.WasPassed() ? aValue.Value() : JS::UndefinedHandleValue,
          Future::Rejected, aAsynchronous);
}

void
FutureResolver::RunTask(JS::Handle<JS::Value> aValue,
                        Future::FutureState aState,
                        FutureTaskSync aAsynchronous)
{
  
  
  if (aAsynchronous == AsyncTask) {
    nsRefPtr<FutureResolverTask> task =
      new FutureResolverTask(this, aValue, aState);
    NS_DispatchToCurrentThread(task);
    return;
  }

  mFuture->SetResult(aValue);
  mFuture->SetState(aState);
  mFuture->RunTask();
  mFuture = nullptr;
}

} 
} 
