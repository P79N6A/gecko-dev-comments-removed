




































#include "nsString.h"
#include "nsIServiceManager.h"
#include "nsISocketProvider.h"
#include "nsSocketProviderService.h"
#include "nsNetError.h"



NS_METHOD
nsSocketProviderService::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
  nsresult rv;
  nsCOMPtr<nsISocketProviderService> inst = new nsSocketProviderService();
  if (!inst)
    rv = NS_ERROR_OUT_OF_MEMORY;
  else
    rv = inst->QueryInterface(aIID, aResult);
  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsSocketProviderService, nsISocketProviderService)



NS_IMETHODIMP
nsSocketProviderService::GetSocketProvider(const char         *type,
                                           nsISocketProvider **result)
{
  nsresult rv;
  nsCAutoString contractID(
          NS_LITERAL_CSTRING(NS_NETWORK_SOCKET_CONTRACTID_PREFIX) +
          nsDependentCString(type));

  rv = CallGetService(contractID.get(), result);
  if (NS_FAILED(rv)) 
      rv = NS_ERROR_UNKNOWN_SOCKET_TYPE;
  return rv;
}


