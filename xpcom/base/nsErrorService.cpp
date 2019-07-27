





#include "nsErrorService.h"
#include "nsCRTGlue.h"
#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS(nsErrorService, nsIErrorService)

nsresult
nsErrorService::Create(nsISupports* aOuter, const nsIID& aIID,
                       void** aInstancePtr)
{
  if (NS_WARN_IF(aOuter)) {
    return NS_ERROR_NO_AGGREGATION;
  }
  nsRefPtr<nsErrorService> serv = new nsErrorService();
  return serv->QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP
nsErrorService::RegisterErrorStringBundle(int16_t aErrorModule,
                                          const char* aStringBundleURL)
{
  mErrorStringBundleURLMap.Put(aErrorModule, new nsCString(aStringBundleURL));
  return NS_OK;
}

NS_IMETHODIMP
nsErrorService::UnregisterErrorStringBundle(int16_t aErrorModule)
{
  mErrorStringBundleURLMap.Remove(aErrorModule);
  return NS_OK;
}

NS_IMETHODIMP
nsErrorService::GetErrorStringBundle(int16_t aErrorModule, char** aResult)
{
  nsCString* bundleURL = mErrorStringBundleURLMap.Get(aErrorModule);
  if (!bundleURL) {
    return NS_ERROR_FAILURE;
  }
  *aResult = ToNewCString(*bundleURL);
  return NS_OK;
}


