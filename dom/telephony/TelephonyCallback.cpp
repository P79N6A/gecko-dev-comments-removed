



#include "TelephonyCallback.h"

#include "mozilla/dom/Promise.h"
#include "Telephony.h"

using namespace mozilla::dom;
using namespace mozilla::dom::telephony;

NS_IMPL_ISUPPORTS(TelephonyCallback, nsITelephonyCallback)

TelephonyCallback::TelephonyCallback(Telephony* aTelephony,
                                     Promise* aPromise,
                                     uint32_t aServiceId)
  : mTelephony(aTelephony), mPromise(aPromise), mServiceId(aServiceId)
{
  MOZ_ASSERT(mTelephony);
}



NS_IMETHODIMP
TelephonyCallback::NotifyDialError(const nsAString& aError)
{
  mPromise->MaybeRejectBrokenly(aError);
  return NS_OK;
}

NS_IMETHODIMP
TelephonyCallback::NotifyDialCallSuccess(uint32_t aCallIndex,
                                         const nsAString& aNumber)
{
  nsRefPtr<TelephonyCallId> id = mTelephony->CreateCallId(aNumber);
  nsRefPtr<TelephonyCall> call =
      mTelephony->CreateCall(id, mServiceId, aCallIndex,
                             nsITelephonyService::CALL_STATE_DIALING);

  mPromise->MaybeResolve(call);
  return NS_OK;
}
