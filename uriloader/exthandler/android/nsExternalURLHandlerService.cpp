




#include "nsExternalURLHandlerService.h"
#include "nsMIMEInfoAndroid.h"

NS_IMPL_ISUPPORTS(nsExternalURLHandlerService, nsIExternalURLHandlerService)

nsExternalURLHandlerService::nsExternalURLHandlerService()
{
}

nsExternalURLHandlerService::~nsExternalURLHandlerService()
{
}

NS_IMETHODIMP
nsExternalURLHandlerService::GetURLHandlerInfoFromOS(nsIURI *aURL,
                                                     bool *found,
                                                     nsIHandlerInfo **info)
{
  nsCString uriSpec;
  aURL->GetSpec(uriSpec);
  return nsMIMEInfoAndroid::GetMimeInfoForURL(uriSpec, found, info);
}
