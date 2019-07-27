





#include "mozilla/BasePrincipal.h"
#include "nsScriptSecurityManager.h"

namespace mozilla {

NS_IMETHODIMP
BasePrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  NS_IF_ADDREF(*aCsp = mCSP);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  
  if (mCSP)
    return NS_ERROR_ALREADY_INITIALIZED;

  mCSP = aCsp;
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetIsNullPrincipal(bool* aIsNullPrincipal)
{
  *aIsNullPrincipal = false;
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetJarPrefix(nsACString& aJarPrefix)
{
  MOZ_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  mozilla::GetJarPrefix(mAppId, mIsInBrowserElement, aJarPrefix);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  if (mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    NS_WARNING("Asking for app status on a principal with an unknown app id");
    *aAppStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
    return NS_OK;
  }

  *aAppStatus = nsScriptSecurityManager::AppStatusForPrincipal(this);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetAppId(uint32_t* aAppId)
{
  if (mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    MOZ_ASSERT(false);
    *aAppId = nsIScriptSecurityManager::NO_APP_ID;
    return NS_OK;
  }

  *aAppId = mAppId;
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = mIsInBrowserElement;
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID;
  return NS_OK;
}

} 
