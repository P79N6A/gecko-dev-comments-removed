





#include "mozilla/dom/MMICall.h"

#include "mozilla/dom/MMICallBinding.h"
#include "nsIGlobalObject.h"

using namespace mozilla::dom;
using mozilla::ErrorResult;

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MMICall, mWindow)
NS_IMPL_CYCLE_COLLECTING_ADDREF(MMICall)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MMICall)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MMICall)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MMICall::MMICall(nsPIDOMWindow* aWindow, const nsAString& aServiceCode)
  : mWindow(aWindow), mServiceCode(aServiceCode)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mWindow);
  if (!global) {
    return;
  }

  ErrorResult rv;
  nsRefPtr<Promise> promise = Promise::Create(global, rv);
  if (rv.Failed()) {
    return;
  }

  mPromise = promise;
}

MMICall::~MMICall()
{
}

nsPIDOMWindow*
MMICall::GetParentObject() const
{
  return mWindow;
}

JSObject*
MMICall::WrapObject(JSContext* aCx)
{
  return MMICallBinding::Wrap(aCx, this);
}

void
MMICall::NotifyResult(JS::Handle<JS::Value> aResult)
{
  if (!mPromise) {
    return;
  }

  mPromise->MaybeResolve(aResult);
}



already_AddRefed<Promise>
MMICall::GetResult(ErrorResult& aRv)
{
  if (!mPromise) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsRefPtr<Promise> promise = mPromise;
  return promise.forget();
}
