





#include "nsInterfaceRequestorAgg.h"
#include "nsIInterfaceRequestor.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"
#include "nsThreadUtils.h"
#include "nsProxyRelease.h"

class nsInterfaceRequestorAgg final : public nsIInterfaceRequestor
{
public:
  
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  nsInterfaceRequestorAgg(nsIInterfaceRequestor* aFirst,
                          nsIInterfaceRequestor* aSecond,
                          nsIEventTarget* aConsumerTarget = nullptr)
    : mFirst(aFirst)
    , mSecond(aSecond)
    , mConsumerTarget(aConsumerTarget)
  {
    if (!mConsumerTarget) {
      mConsumerTarget = NS_GetCurrentThread();
    }
  }

private:
  ~nsInterfaceRequestorAgg();

  nsCOMPtr<nsIInterfaceRequestor> mFirst, mSecond;
  nsCOMPtr<nsIEventTarget> mConsumerTarget;
};

NS_IMPL_ISUPPORTS(nsInterfaceRequestorAgg, nsIInterfaceRequestor)

NS_IMETHODIMP
nsInterfaceRequestorAgg::GetInterface(const nsIID& aIID, void** aResult)
{
  nsresult rv = NS_ERROR_NO_INTERFACE;
  if (mFirst) {
    rv = mFirst->GetInterface(aIID, aResult);
  }
  if (mSecond && NS_FAILED(rv)) {
    rv = mSecond->GetInterface(aIID, aResult);
  }
  return rv;
}

nsInterfaceRequestorAgg::~nsInterfaceRequestorAgg()
{
  nsIInterfaceRequestor* iir = nullptr;
  mFirst.swap(iir);
  if (iir) {
    NS_ProxyRelease(mConsumerTarget, iir);
  }
  iir = nullptr;
  mSecond.swap(iir);
  if (iir) {
    NS_ProxyRelease(mConsumerTarget, iir);
  }
}

nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor* aFirst,
                                    nsIInterfaceRequestor* aSecond,
                                    nsIInterfaceRequestor** aResult)
{
  *aResult = new nsInterfaceRequestorAgg(aFirst, aSecond);
  if (!*aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aResult);
  return NS_OK;
}

nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor* aFirst,
                                    nsIInterfaceRequestor* aSecond,
                                    nsIEventTarget* aTarget,
                                    nsIInterfaceRequestor** aResult)
{
  *aResult = new nsInterfaceRequestorAgg(aFirst, aSecond, aTarget);
  if (!*aResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  NS_ADDREF(*aResult);
  return NS_OK;
}
