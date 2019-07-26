





#include "mozilla/net/DNSListenerProxy.h"
#include "nsICancelable.h"
#include "nsIEventTarget.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(DNSListenerProxy, nsIDNSListener)

NS_IMETHODIMP
DNSListenerProxy::OnLookupComplete(nsICancelable* aRequest,
                                   nsIDNSRecord* aRecord,
                                   nsresult aStatus)
{
  nsRefPtr<OnLookupCompleteRunnable> r =
    new OnLookupCompleteRunnable(mListener, aRequest, aRecord, aStatus);
  return mTargetThread->Dispatch(r, NS_DISPATCH_NORMAL);
}

NS_IMETHODIMP
DNSListenerProxy::OnLookupCompleteRunnable::Run()
{
  mListener->OnLookupComplete(mRequest, mRecord, mStatus);
  return NS_OK;
}



} 
} 
