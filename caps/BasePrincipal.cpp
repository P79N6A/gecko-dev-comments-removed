





#include "mozilla/BasePrincipal.h"

#include "nsIAddonPolicyService.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

#include "nsPrincipal.h"
#include "nsNetUtil.h"
#include "nsIURIWithPrincipal.h"
#include "nsNullPrincipal.h"
#include "nsScriptSecurityManager.h"

#include "mozilla/dom/CSPDictionariesBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/dom/URLSearchParams.h"

namespace mozilla {

using dom::URLParams;

void
OriginAttributes::CreateSuffix(nsACString& aStr) const
{
  MOZ_RELEASE_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  UniquePtr<URLParams> params(new URLParams());
  nsAutoString value;

  if (mAppId != nsIScriptSecurityManager::NO_APP_ID) {
    value.AppendInt(mAppId);
    params->Set(NS_LITERAL_STRING("appId"), value);
  }

  if (mInBrowser) {
    params->Set(NS_LITERAL_STRING("inBrowser"), NS_LITERAL_STRING("1"));
  }

  if (!mAddonId.IsEmpty()) {
    params->Set(NS_LITERAL_STRING("addonId"), mAddonId);
  }

  aStr.Truncate();

  params->Serialize(value);
  if (!value.IsEmpty()) {
    aStr.AppendLiteral("!");
    aStr.Append(NS_ConvertUTF16toUTF8(value));
  }
}

namespace {

class MOZ_STACK_CLASS PopulateFromSuffixIterator final
  : public URLParams::ForEachIterator
{
public:
  explicit PopulateFromSuffixIterator(OriginAttributes* aOriginAttributes)
    : mOriginAttributes(aOriginAttributes)
  {
    MOZ_ASSERT(aOriginAttributes);
  }

  bool URLParamsIterator(const nsString& aName,
                         const nsString& aValue) override
  {
    if (aName.EqualsLiteral("appId")) {
      nsresult rv;
      mOriginAttributes->mAppId = aValue.ToInteger(&rv);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return false;
      }

      if (mOriginAttributes->mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
        return false;
      }

      return true;
    }

    if (aName.EqualsLiteral("inBrowser")) {
      if (!aValue.EqualsLiteral("1")) {
        return false;
      }

      mOriginAttributes->mInBrowser = true;
      return true;
    }

    if (aName.EqualsLiteral("addonId")) {
      MOZ_RELEASE_ASSERT(mOriginAttributes->mAddonId.IsEmpty());
      mOriginAttributes->mAddonId.Assign(aValue);
      return true;
    }

    
    return false;
  }

private:
  OriginAttributes* mOriginAttributes;
};

} 

bool
OriginAttributes::PopulateFromSuffix(const nsACString& aStr)
{
  if (aStr.IsEmpty()) {
    return true;
  }

  if (aStr[0] != '!') {
    return false;
  }

  UniquePtr<URLParams> params(new URLParams());
  params->ParseInput(Substring(aStr, 1, aStr.Length() - 1));

  PopulateFromSuffixIterator iterator(this);
  return params->ForEach(iterator);
}

bool
OriginAttributes::PopulateFromOrigin(const nsACString& aOrigin,
                                     nsACString& aOriginNoSuffix)
{
  
  nsCString origin(aOrigin);
  int32_t pos = origin.RFindChar('!');

  if (pos == kNotFound) {
    aOriginNoSuffix = origin;
    return true;
  }

  aOriginNoSuffix = Substring(origin, 0, pos);
  return PopulateFromSuffix(Substring(origin, pos));
}

void
OriginAttributes::CookieJar(nsACString& aStr)
{
  mozilla::GetJarPrefix(mAppId, mInBrowser, aStr);
}

BasePrincipal::BasePrincipal()
{}

BasePrincipal::~BasePrincipal()
{}

NS_IMETHODIMP
BasePrincipal::GetOrigin(nsACString& aOrigin)
{
  nsresult rv = GetOriginInternal(aOrigin);
  NS_ENSURE_SUCCESS(rv, rv);
  nsAutoCString suffix;
  mOriginAttributes.CreateSuffix(suffix);
  aOrigin.Append(suffix);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetOriginNoSuffix(nsACString& aOrigin)
{
  return GetOriginInternal(aOrigin);
}

bool
BasePrincipal::Subsumes(nsIPrincipal* aOther, DocumentDomainConsideration aConsideration)
{
  MOZ_ASSERT(aOther);
  return SubsumesInternal(aOther, aConsideration);
}

NS_IMETHODIMP
BasePrincipal::Equals(nsIPrincipal *aOther, bool *aResult)
{
  NS_ENSURE_TRUE(aOther, NS_ERROR_INVALID_ARG);
  *aResult = Subsumes(aOther, DontConsiderDocumentDomain) &&
             Cast(aOther)->Subsumes(this, DontConsiderDocumentDomain);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::EqualsConsideringDomain(nsIPrincipal *aOther, bool *aResult)
{
  NS_ENSURE_TRUE(aOther, NS_ERROR_INVALID_ARG);
  *aResult = Subsumes(aOther, ConsiderDocumentDomain) &&
             Cast(aOther)->Subsumes(this, ConsiderDocumentDomain);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::Subsumes(nsIPrincipal *aOther, bool *aResult)
{
  NS_ENSURE_TRUE(aOther, NS_ERROR_INVALID_ARG);
  *aResult = Subsumes(aOther, DontConsiderDocumentDomain);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::SubsumesConsideringDomain(nsIPrincipal *aOther, bool *aResult)
{
  NS_ENSURE_TRUE(aOther, NS_ERROR_INVALID_ARG);
  *aResult = Subsumes(aOther, ConsiderDocumentDomain);
  return NS_OK;
}

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
BasePrincipal::GetCspJSON(nsAString& outCSPinJSON)
{
  outCSPinJSON.Truncate();
  dom::CSPPolicies jsonPolicies;

  if (!mCSP) {
    jsonPolicies.ToJSON(outCSPinJSON);
    return NS_OK;
  }
  return mCSP->ToJSON(outCSPinJSON);
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
  MOZ_ASSERT(AppId() != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  mOriginAttributes.CookieJar(aJarPrefix);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetOriginAttributes(JSContext* aCx, JS::MutableHandle<JS::Value> aVal)
{
  if (NS_WARN_IF(!ToJSValue(aCx, mOriginAttributes, aVal))) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetOriginSuffix(nsACString& aOriginAttributes)
{
  mOriginAttributes.CreateSuffix(aOriginAttributes);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetCookieJar(nsACString& aCookieJar)
{
  mOriginAttributes.CookieJar(aCookieJar);
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  if (AppId() == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
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
  if (AppId() == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    MOZ_ASSERT(false);
    *aAppId = nsIScriptSecurityManager::NO_APP_ID;
    return NS_OK;
  }

  *aAppId = AppId();
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = IsInBrowserElement();
  return NS_OK;
}

NS_IMETHODIMP
BasePrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = AppId() == nsIScriptSecurityManager::UNKNOWN_APP_ID;
  return NS_OK;
}

already_AddRefed<BasePrincipal>
BasePrincipal::CreateCodebasePrincipal(nsIURI* aURI, OriginAttributes& aAttrs)
{
  
  
  bool inheritsPrincipal;
  nsresult rv = NS_URIChainHasFlags(aURI, nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                                    &inheritsPrincipal);
  nsCOMPtr<nsIPrincipal> principal;
  if (NS_FAILED(rv) || inheritsPrincipal) {
    return nsNullPrincipal::Create();
  }

  
  nsCOMPtr<nsIURIWithPrincipal> uriPrinc = do_QueryInterface(aURI);
  if (uriPrinc) {
    nsCOMPtr<nsIPrincipal> principal;
    uriPrinc->GetPrincipal(getter_AddRefs(principal));
    if (!principal) {
      return nsNullPrincipal::Create();
    }
    nsRefPtr<BasePrincipal> concrete = Cast(principal);
    return concrete.forget();
  }

  
  nsRefPtr<nsPrincipal> codebase = new nsPrincipal();
  rv = codebase->Init(aURI, aAttrs);
  NS_ENSURE_SUCCESS(rv, nullptr);
  return codebase.forget();
}

bool
BasePrincipal::AddonAllowsLoad(nsIURI* aURI)
{
  if (mOriginAttributes.mAddonId.IsEmpty()) {
    return false;
  }

  nsCOMPtr<nsIAddonPolicyService> aps = do_GetService("@mozilla.org/addons/policy-service;1");
  NS_ENSURE_TRUE(aps, false);

  bool allowed = false;
  nsresult rv = aps->AddonMayLoadURI(mOriginAttributes.mAddonId, aURI, &allowed);
  return NS_SUCCEEDED(rv) && allowed;
}

} 
