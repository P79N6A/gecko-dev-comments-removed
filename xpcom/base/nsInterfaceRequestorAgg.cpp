



































#include "nsInterfaceRequestorAgg.h"
#include "nsCOMPtr.h"

class nsInterfaceRequestorAgg : public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  nsInterfaceRequestorAgg(nsIInterfaceRequestor *aFirst,
                          nsIInterfaceRequestor *aSecond)
    : mFirst(aFirst)
    , mSecond(aSecond) {}

  nsCOMPtr<nsIInterfaceRequestor> mFirst, mSecond;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsInterfaceRequestorAgg, nsIInterfaceRequestor)

NS_IMETHODIMP
nsInterfaceRequestorAgg::GetInterface(const nsIID &aIID, void **aResult)
{
  nsresult rv = NS_ERROR_NO_INTERFACE;
  if (mFirst)
    rv = mFirst->GetInterface(aIID, aResult);
  if (mSecond && NS_FAILED(rv))
    rv = mSecond->GetInterface(aIID, aResult);
  return rv;
}

nsresult
NS_NewInterfaceRequestorAggregation(nsIInterfaceRequestor *aFirst,
                                    nsIInterfaceRequestor *aSecond,
                                    nsIInterfaceRequestor **aResult)
{
  *aResult = new nsInterfaceRequestorAgg(aFirst, aSecond);
  if (!*aResult)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(*aResult);
  return NS_OK;
}
