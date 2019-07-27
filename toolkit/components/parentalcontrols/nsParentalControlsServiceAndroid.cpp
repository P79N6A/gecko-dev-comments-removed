




#include "nsParentalControlsService.h"
#include "AndroidBridge.h"
#include "nsString.h"
#include "nsIFile.h"

NS_IMPL_ISUPPORTS(nsParentalControlsService, nsIParentalControlsService)

nsParentalControlsService::nsParentalControlsService() :
  mEnabled(false)
{
  if (mozilla::AndroidBridge::HasEnv()) {
    mEnabled = mozilla::widget::RestrictedProfiles::IsUserRestricted();
  }
}

nsParentalControlsService::~nsParentalControlsService()
{
}

NS_IMETHODIMP
nsParentalControlsService::GetParentalControlsEnabled(bool *aResult)
{
  *aResult = mEnabled;
  return NS_OK;
}

NS_IMETHODIMP
nsParentalControlsService::GetBlockFileDownloadsEnabled(bool *aResult)
{
  
  bool res;
  IsAllowed(nsIParentalControlsService::DOWNLOAD, NULL, &res);
  *aResult = !res;

  return NS_OK;
}

NS_IMETHODIMP
nsParentalControlsService::GetLoggingEnabled(bool *aResult)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsParentalControlsService::Log(int16_t aEntryType,
                               bool aBlocked,
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
  nsresult rv = NS_OK;
  *_retval = true;

  if (!mEnabled) {
    return rv;
  }

  if (mozilla::AndroidBridge::HasEnv()) {
    nsAutoCString url;
    if (aUri) {
      rv = aUri->GetSpec(url);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    *_retval = mozilla::widget::RestrictedProfiles::IsAllowed(aAction,
                                                    NS_ConvertUTF8toUTF16(url));
    return rv;
  }

  return NS_ERROR_NOT_AVAILABLE;
}
