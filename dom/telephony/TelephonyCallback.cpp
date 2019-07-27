





#include "TelephonyCallback.h"

#include "mozilla/dom/Promise.h"
#include "nsJSUtils.h"

using namespace mozilla::dom;
using namespace mozilla::dom::telephony;

NS_IMPL_ISUPPORTS(TelephonyCallback, nsITelephonyCallback)

TelephonyCallback::TelephonyCallback(Promise* aPromise)
  : mPromise(aPromise)
{
}



NS_IMETHODIMP
TelephonyCallback::NotifySuccess()
{
  mPromise->MaybeResolve(JS::UndefinedHandleValue);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCallback::NotifyError(const nsAString& aError)
{
  mPromise->MaybeRejectBrokenly(aError);
  return NS_OK;
}
