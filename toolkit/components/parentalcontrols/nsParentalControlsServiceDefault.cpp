




#include "nsParentalControlsService.h"
#include "nsString.h"
#include "nsIFile.h"
#include "mozilla/unused.h"

NS_IMPL_ISUPPORTS(nsParentalControlsService, nsIParentalControlsService)

nsParentalControlsService::nsParentalControlsService() :
  mEnabled(false)
{
  mozilla::unused << mEnabled;
}

nsParentalControlsService::~nsParentalControlsService()
{
}

NS_IMETHODIMP
nsParentalControlsService::GetParentalControlsEnabled(bool *aResult)
{
  *aResult = false;
  return NS_OK;
}

NS_IMETHODIMP
nsParentalControlsService::GetBlockFileDownloadsEnabled(bool *aResult)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::GetLoggingEnabled(bool *aResult)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::Log(int16_t aEntryType,
                               bool blocked,
                               nsIURI *aSource,
                               nsIFile *aTarget)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::RequestURIOverride(nsIURI *aTarget,
                                              nsIInterfaceRequestor *aWindowContext,
                                              bool *_retval)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::RequestURIOverrides(nsIArray *aTargets,
                                               nsIInterfaceRequestor *aWindowContext,
                                               bool *_retval)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::IsAllowed(int16_t aAction,
                                     nsIURI *aUri,
                                     bool *_retval)
{
  return NS_ERROR_NOT_AVAILABLE;
}
