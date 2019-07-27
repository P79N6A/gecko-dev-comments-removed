




#include "nsPermission.h"
#include "nsContentUtils.h"
#include "nsIClassInfoImpl.h"
#include "nsIEffectiveTLDService.h"
#include "nsIScriptSecurityManager.h"
#include "mozilla/BasePrincipal.h"



NS_IMPL_CLASSINFO(nsPermission, nullptr, 0, {0})
NS_IMPL_ISUPPORTS_CI(nsPermission, nsIPermission)

nsPermission::nsPermission(nsIPrincipal*    aPrincipal,
                           const nsACString &aType,
                           uint32_t         aCapability,
                           uint32_t         aExpireType,
                           int64_t          aExpireTime)
 : mPrincipal(aPrincipal)
 , mType(aType)
 , mCapability(aCapability)
 , mExpireType(aExpireType)
 , mExpireTime(aExpireTime)
{
}

NS_IMETHODIMP
nsPermission::GetPrincipal(nsIPrincipal** aPrincipal)
{
  nsCOMPtr<nsIPrincipal> copy = mPrincipal;
  copy.forget(aPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetType(nsACString &aType)
{
  aType = mType;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetCapability(uint32_t *aCapability)
{
  *aCapability = mCapability;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetExpireType(uint32_t *aExpireType)
{
  *aExpireType = mExpireType;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::GetExpireTime(int64_t *aExpireTime)
{
  *aExpireTime = mExpireTime;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::Matches(nsIPrincipal* aPrincipal, bool aExactHost, bool* aMatches)
{
  NS_ENSURE_ARG_POINTER(aPrincipal);
  NS_ENSURE_ARG_POINTER(aMatches);

  *aMatches = false;

  
  if (mPrincipal->Equals(aPrincipal)) {
    *aMatches = true;
    return NS_OK;
  }

  
  
  if (aExactHost) {
      return NS_OK;
  }

  
  const mozilla::OriginAttributes& theirAttrs = mozilla::BasePrincipal::Cast(aPrincipal)->OriginAttributesRef();
  const mozilla::OriginAttributes& ourAttrs = mozilla::BasePrincipal::Cast(mPrincipal)->OriginAttributesRef();

  if (theirAttrs != ourAttrs) {
      return NS_OK;
  }

  nsCOMPtr<nsIURI> theirURI;
  nsresult rv = aPrincipal->GetURI(getter_AddRefs(theirURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> ourURI;
  rv = mPrincipal->GetURI(getter_AddRefs(ourURI));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString theirScheme;
  rv = theirURI->GetScheme(theirScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString ourScheme;
  rv = ourURI->GetScheme(ourScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if (theirScheme != ourScheme) {
    return NS_OK;
  }

  
  int32_t theirPort;
  rv = theirURI->GetPort(&theirPort);
  NS_ENSURE_SUCCESS(rv, rv);

  int32_t ourPort;
  rv = ourURI->GetPort(&ourPort);
  NS_ENSURE_SUCCESS(rv, rv);

  if (theirPort != ourPort) {
    return NS_OK;
  }

  
  nsAutoCString theirHost;
  rv = theirURI->GetHost(theirHost);
  if (NS_FAILED(rv) || theirHost.IsEmpty()) {
    return NS_OK;
  }

  nsAutoCString ourHost;
  rv = ourURI->GetHost(ourHost);
  if (NS_FAILED(rv) || ourHost.IsEmpty()) {
    return NS_OK;
  }

  nsCOMPtr<nsIEffectiveTLDService> tldService =
    do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);
  if (!tldService) {
    NS_ERROR("Should have a tld service!");
    return NS_ERROR_FAILURE;
  }

  
  
  while (theirHost != ourHost) {
    rv = tldService->GetNextSubDomain(theirHost, theirHost);
    if (NS_FAILED(rv)) {
      if (rv == NS_ERROR_INSUFFICIENT_DOMAIN_LEVELS) {
        return NS_OK;
      } else {
        return rv;
      }
    }
  }

  *aMatches = true;
  return NS_OK;
}

NS_IMETHODIMP
nsPermission::MatchesURI(nsIURI* aURI, bool aExactHost, bool* aMatches)
{
  NS_ENSURE_ARG_POINTER(aURI);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

  nsCOMPtr<nsIPrincipal> principal;
  nsresult rv = secMan->GetNoAppCodebasePrincipal(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  return Matches(principal, aExactHost, aMatches);
}
