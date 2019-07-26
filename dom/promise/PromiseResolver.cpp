





#include "mozilla/dom/PromiseResolver.h"
#include "mozilla/dom/PromiseBinding.h"
#include "mozilla/dom/Promise.h"
#include "PromiseCallback.h"

namespace mozilla {
namespace dom {




class PromiseResolverTask MOZ_FINAL : public nsRunnable
{
public:
  PromiseResolverTask(PromiseResolver* aResolver,
                      const JS::Handle<JS::Value> aValue,
                      Promise::PromiseState aState)
    : mResolver(aResolver)
    , mValue(aValue)
    , mState(aState)
  {
    MOZ_ASSERT(aResolver);
    MOZ_ASSERT(mState != Promise::Pending);
    MOZ_COUNT_CTOR(PromiseResolverTask);

    JSContext* cx = nsContentUtils::GetSafeJSContext();
    JS_AddNamedValueRootRT(JS_GetRuntime(cx), &mValue,
                           "PromiseResolverTask.mValue");
  }

  ~PromiseResolverTask()
  {
    MOZ_COUNT_DTOR(PromiseResolverTask);

    JSContext* cx = nsContentUtils::GetSafeJSContext();
    JS_RemoveValueRootRT(JS_GetRuntime(cx), &mValue);
  }

  NS_IMETHOD Run()
  {
    mResolver->RunTask(JS::Handle<JS::Value>::fromMarkedLocation(&mValue),
                       mState, PromiseResolver::SyncTask);
    return NS_OK;
  }

private:
  nsRefPtr<PromiseResolver> mResolver;
  JS::Value mValue;
  Promise::PromiseState mState;
};



NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_1(PromiseResolver, mPromise)

NS_IMPL_CYCLE_COLLECTING_ADDREF(PromiseResolver)
NS_IMPL_CYCLE_COLLECTING_RELEASE(PromiseResolver)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(PromiseResolver)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

PromiseResolver::PromiseResolver(Promise* aPromise)
  : mPromise(aPromise)
  , mResolvePending(false)
{
  SetIsDOMBinding();
}

JSObject*
PromiseResolver::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return PromiseResolverBinding::Wrap(aCx, aScope, this);
}

void
PromiseResolver::Resolve(JSContext* aCx,
                         const Optional<JS::Handle<JS::Value> >& aValue,
                         PromiseTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  ResolveInternal(aCx, aValue, aAsynchronous);
}

void
PromiseResolver::ResolveInternal(JSContext* aCx,
                                 const Optional<JS::Handle<JS::Value> >& aValue,
                                 PromiseTaskSync aAsynchronous)
{
  mResolvePending = true;

  
  if (aValue.WasPassed() && aValue.Value().isObject()) {
    JS::Rooted<JSObject*> valueObj(aCx, &aValue.Value().toObject());
    Promise* nextPromise;
    nsresult rv = UnwrapObject<Promise>(aCx, valueObj, nextPromise);

    if (NS_SUCCEEDED(rv)) {
      nsRefPtr<PromiseCallback> resolveCb = new ResolvePromiseCallback(this);
      nsRefPtr<PromiseCallback> rejectCb = new RejectPromiseCallback(this);
      nextPromise->AppendCallbacks(resolveCb, rejectCb);
      return;
    }
  }

  
  
  
  
  RunTask(aValue.WasPassed() ? aValue.Value() : JS::UndefinedHandleValue,
          Promise::Resolved, aAsynchronous);
}

void
PromiseResolver::Reject(JSContext* aCx,
                        const Optional<JS::Handle<JS::Value> >& aValue,
                        PromiseTaskSync aAsynchronous)
{
  if (mResolvePending) {
    return;
  }

  RejectInternal(aCx, aValue, aAsynchronous);
}

void
PromiseResolver::RejectInternal(JSContext* aCx,
                                const Optional<JS::Handle<JS::Value> >& aValue,
                                PromiseTaskSync aAsynchronous)
{
  mResolvePending = true;

  
  
  
  RunTask(aValue.WasPassed() ? aValue.Value() : JS::UndefinedHandleValue,
          Promise::Rejected, aAsynchronous);
}

void
PromiseResolver::RunTask(JS::Handle<JS::Value> aValue,
                         Promise::PromiseState aState,
                         PromiseTaskSync aAsynchronous)
{
  
  
  if (aAsynchronous == AsyncTask) {
    nsRefPtr<PromiseResolverTask> task =
      new PromiseResolverTask(this, aValue, aState);
    NS_DispatchToCurrentThread(task);
    return;
  }

  mPromise->SetResult(aValue);
  mPromise->SetState(aState);
  mPromise->RunTask();
  mPromise = nullptr;
}

} 
} 
