





#include "mozilla/BasePrincipal.h"

#include "nsIContentSecurityPolicy.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"

#include "nsPrincipal.h"
#include "nsNetUtil.h"
#include "nsNullPrincipal.h"
#include "nsScriptSecurityManager.h"

#include "mozilla/dom/CSPDictionariesBinding.h"
#include "mozilla/dom/ToJSValue.h"

namespace mozilla {

void
OriginAttributes::CreateSuffix(nsACString& aStr) const
{
  aStr.Truncate();
  MOZ_RELEASE_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);
  int attrCount = 0;

  if (mAppId != nsIScriptSecurityManager::NO_APP_ID) {
    aStr.Append(attrCount++ ? "&appId=" : "!appId=");
    aStr.AppendInt(mAppId);
  }

  if (mInBrowser) {
    aStr.Append(attrCount++ ? "&inBrowser=1" : "!inBrowser=1");
  }
}

void
OriginAttributes::CookieJar(nsACString& aStr)
{
  mozilla::GetJarPrefix(mAppId, mInBrowser, aStr);
}

void
OriginAttributes::Serialize(nsIObjectOutputStream* aStream) const
{
  aStream->Write32(mAppId);
  aStream->WriteBoolean(mInBrowser);
}

nsresult
OriginAttributes::Deserialize(nsIObjectInputStream* aStream)
{
  nsresult rv = aStream->Read32(&mAppId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->ReadBoolean(&mInBrowser);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
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

} 
