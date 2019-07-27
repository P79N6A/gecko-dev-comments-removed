





#include "nsPrincipal.h"

#include "mozIThirdPartyUtil.h"
#include "nscore.h"
#include "nsScriptSecurityManager.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "pratom.h"
#include "nsIURI.h"
#include "nsJSPrincipals.h"
#include "nsIEffectiveTLDService.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIClassInfoImpl.h"
#include "nsIProtocolHandler.h"
#include "nsError.h"
#include "nsIContentSecurityPolicy.h"
#include "nsNetCID.h"
#include "jswrapper.h"

#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/Preferences.h"
#include "mozilla/HashFunctions.h"

#include "nsIAppsService.h"
#include "mozIApplication.h"

using namespace mozilla;

static bool gIsWhitelistingTestDomains = false;
static bool gCodeBasePrincipalSupport = false;

static bool URIIsImmutable(nsIURI* aURI)
{
  nsCOMPtr<nsIMutable> mutableObj(do_QueryInterface(aURI));
  bool isMutable;
  return
    mutableObj &&
    NS_SUCCEEDED(mutableObj->GetMutable(&isMutable)) &&
    !isMutable;
}


const char nsBasePrincipal::sInvalid[] = "Invalid";

NS_IMETHODIMP_(MozExternalRefCountType)
nsBasePrincipal::AddRef()
{
  NS_PRECONDITION(int32_t(refcount) >= 0, "illegal refcnt");
  
  nsrefcnt count = ++refcount;
  NS_LOG_ADDREF(this, count, "nsBasePrincipal", sizeof(*this));
  return count;
}

NS_IMETHODIMP_(MozExternalRefCountType)
nsBasePrincipal::Release()
{
  NS_PRECONDITION(0 != refcount, "dup release");
  nsrefcnt count = --refcount;
  NS_LOG_RELEASE(this, count, "nsBasePrincipal");
  if (count == 0) {
    delete this;
  }

  return count;
}

nsBasePrincipal::nsBasePrincipal()
{
}

nsBasePrincipal::~nsBasePrincipal(void)
{
}

NS_IMETHODIMP
nsBasePrincipal::GetCsp(nsIContentSecurityPolicy** aCsp)
{
  NS_IF_ADDREF(*aCsp = mCSP);
  return NS_OK;
}

NS_IMETHODIMP
nsBasePrincipal::SetCsp(nsIContentSecurityPolicy* aCsp)
{
  
  
  if (mCSP)
    return NS_ERROR_ALREADY_INITIALIZED;

  mCSP = aCsp;
  return NS_OK;
}

#ifdef DEBUG
void nsPrincipal::dumpImpl()
{
  nsAutoCString str;
  GetScriptLocation(str);
  fprintf(stderr, "nsPrincipal (%p) = %s\n", static_cast<void*>(this), str.get());
}
#endif 

NS_IMPL_CLASSINFO(nsPrincipal, nullptr, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_PRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsPrincipal,
                           nsIPrincipal,
                           nsISerializable)
NS_IMPL_CI_INTERFACE_GETTER(nsPrincipal,
                            nsIPrincipal,
                            nsISerializable)
NS_IMPL_ADDREF_INHERITED(nsPrincipal, nsBasePrincipal)
NS_IMPL_RELEASE_INHERITED(nsPrincipal, nsBasePrincipal)


 void
nsPrincipal::InitializeStatics()
{
  Preferences::AddBoolVarCache(
    &gIsWhitelistingTestDomains,
    "layout.css.unprefixing-service.include-test-domains");

  Preferences::AddBoolVarCache(&gCodeBasePrincipalSupport,
                               "signed.applets.codebase_principal_support",
                               false);
}

nsPrincipal::nsPrincipal()
  : mAppId(nsIScriptSecurityManager::UNKNOWN_APP_ID)
  , mInMozBrowser(false)
  , mCodebaseImmutable(false)
  , mDomainImmutable(false)
  , mInitialized(false)
{ }

nsPrincipal::~nsPrincipal()
{ }

nsresult
nsPrincipal::Init(nsIURI *aCodebase,
                  uint32_t aAppId,
                  bool aInMozBrowser)
{
  NS_ENSURE_STATE(!mInitialized);
  NS_ENSURE_ARG(aCodebase);

  mInitialized = true;

  mCodebase = NS_TryToMakeImmutable(aCodebase);
  mCodebaseImmutable = URIIsImmutable(mCodebase);

  mAppId = aAppId;
  mInMozBrowser = aInMozBrowser;

  return NS_OK;
}

void
nsPrincipal::GetScriptLocation(nsACString &aStr)
{
  mCodebase->GetSpec(aStr);
}

 nsresult
nsPrincipal::GetOriginForURI(nsIURI* aURI, char **aOrigin)
{
  if (!aURI) {
    return NS_ERROR_FAILURE;
  }

  *aOrigin = nullptr;

  nsCOMPtr<nsIURI> origin = NS_GetInnermostURI(aURI);
  if (!origin) {
    return NS_ERROR_FAILURE;
  }

  nsAutoCString hostPort;

  
  
  
  
  bool isChrome;
  nsresult rv = origin->SchemeIs("chrome", &isChrome);
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetAsciiHost(hostPort);
    
    
    if (hostPort.IsEmpty()) {
      rv = NS_ERROR_FAILURE;
    }
  }

  int32_t port;
  if (NS_SUCCEEDED(rv) && !isChrome) {
    rv = origin->GetPort(&port);
  }

  if (NS_SUCCEEDED(rv) && !isChrome) {
    if (port != -1) {
      hostPort.Append(':');
      hostPort.AppendInt(port, 10);
    }

    nsAutoCString scheme;
    rv = origin->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOrigin = ToNewCString(scheme + NS_LITERAL_CSTRING("://") + hostPort);
  }
  else {
    
    
    nsAutoCString spec;
    
    
    rv = origin->GetAsciiSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOrigin = ToNewCString(spec);
  }

  return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsPrincipal::GetOrigin(char **aOrigin)
{
  return GetOriginForURI(mCodebase, aOrigin);
}

NS_IMETHODIMP
nsPrincipal::EqualsConsideringDomain(nsIPrincipal *aOther, bool *aResult)
{
  *aResult = false;

  if (!aOther) {
    NS_WARNING("Need a principal to compare this to!");
    return NS_OK;
  }

  if (aOther == this) {
    *aResult = true;
    return NS_OK;
  }

  if (!nsScriptSecurityManager::AppAttributesEqual(this, aOther)) {
      return NS_OK;
  }

  
  
  
  

  nsCOMPtr<nsIURI> thisURI;
  this->GetDomain(getter_AddRefs(thisURI));
  bool thisSetDomain = !!thisURI;
  if (!thisURI) {
      this->GetURI(getter_AddRefs(thisURI));
  }

  nsCOMPtr<nsIURI> otherURI;
  aOther->GetDomain(getter_AddRefs(otherURI));
  bool otherSetDomain = !!otherURI;
  if (!otherURI) {
      aOther->GetURI(getter_AddRefs(otherURI));
  }

  *aResult = thisSetDomain == otherSetDomain &&
             nsScriptSecurityManager::SecurityCompareURIs(thisURI, otherURI);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::Equals(nsIPrincipal *aOther, bool *aResult)
{
  *aResult = false;

  if (!aOther) {
    NS_WARNING("Need a principal to compare this to!");
    return NS_OK;
  }

  if (aOther == this) {
    *aResult = true;
    return NS_OK;
  }

  if (!nsScriptSecurityManager::AppAttributesEqual(this, aOther)) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> otherURI;
  nsresult rv = aOther->GetURI(getter_AddRefs(otherURI));
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(mCodebase,
               "shouldn't be calling this on principals from preferences");

  
  *aResult = nsScriptSecurityManager::SecurityCompareURIs(mCodebase,
                                                          otherURI);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::Subsumes(nsIPrincipal *aOther, bool *aResult)
{
  return Equals(aOther, aResult);
}

NS_IMETHODIMP
nsPrincipal::SubsumesConsideringDomain(nsIPrincipal *aOther, bool *aResult)
{
  return EqualsConsideringDomain(aOther, aResult);
}

NS_IMETHODIMP
nsPrincipal::GetURI(nsIURI** aURI)
{
  if (mCodebaseImmutable) {
    NS_ADDREF(*aURI = mCodebase);
    return NS_OK;
  }

  if (!mCodebase) {
    *aURI = nullptr;
    return NS_OK;
  }

  return NS_EnsureSafeToReturn(mCodebase, aURI);
}

NS_IMETHODIMP
nsPrincipal::CheckMayLoad(nsIURI* aURI, bool aReport, bool aAllowIfInheritsPrincipal)
{
   if (aAllowIfInheritsPrincipal) {
    
    
    if (nsPrincipal::IsPrincipalInherited(aURI)) {
      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIURIWithPrincipal> uriWithPrin = do_QueryInterface(aURI);
  nsCOMPtr<nsIPrincipal> uriPrin;
  if (uriWithPrin) {
    uriWithPrin->GetPrincipal(getter_AddRefs(uriPrin));
  }
  if (uriPrin && nsIPrincipal::Subsumes(uriPrin)) {
      return NS_OK;
  }

  if (nsScriptSecurityManager::SecurityCompareURIs(mCodebase, aURI)) {
    return NS_OK;
  }

  
  
  
  if (nsScriptSecurityManager::GetStrictFileOriginPolicy() &&
      NS_URIIsLocalFile(aURI) &&
      NS_RelaxStrictFileOriginPolicy(aURI, mCodebase)) {
    return NS_OK;
  }

  if (aReport) {
    nsScriptSecurityManager::ReportError(nullptr, NS_LITERAL_STRING("CheckSameOriginError"), mCodebase, aURI);
  }
  return NS_ERROR_DOM_BAD_URI;
}

void
nsPrincipal::SetURI(nsIURI* aURI)
{
  mCodebase = NS_TryToMakeImmutable(aURI);
  mCodebaseImmutable = URIIsImmutable(mCodebase);
}

NS_IMETHODIMP
nsPrincipal::GetHashValue(uint32_t* aValue)
{
  NS_PRECONDITION(mCodebase, "Need a codebase");

  *aValue = nsScriptSecurityManager::HashPrincipalByOrigin(this);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetDomain(nsIURI** aDomain)
{
  if (!mDomain) {
    *aDomain = nullptr;
    return NS_OK;
  }

  if (mDomainImmutable) {
    NS_ADDREF(*aDomain = mDomain);
    return NS_OK;
  }

  return NS_EnsureSafeToReturn(mDomain, aDomain);
}

NS_IMETHODIMP
nsPrincipal::SetDomain(nsIURI* aDomain)
{
  mDomain = NS_TryToMakeImmutable(aDomain);
  mDomainImmutable = URIIsImmutable(mDomain);

  
  
  AutoSafeJSContext cx;
  JSPrincipals *principals = nsJSPrincipals::get(static_cast<nsIPrincipal*>(this));
  bool success = js::RecomputeWrappers(cx, js::ContentCompartmentsOnly(),
                                       js::CompartmentsWithPrincipals(principals));
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  success = js::RecomputeWrappers(cx, js::CompartmentsWithPrincipals(principals),
                                  js::ContentCompartmentsOnly());
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetJarPrefix(nsACString& aJarPrefix)
{
  MOZ_ASSERT(mAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID);

  mozilla::GetJarPrefix(mAppId, mInMozBrowser, aJarPrefix);
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  *aAppStatus = GetAppStatus();
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetAppId(uint32_t* aAppId)
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
nsPrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = mInMozBrowser;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetIsNullPrincipal(bool* aIsNullPrincipal)
{
  *aIsNullPrincipal = false;
  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::GetBaseDomain(nsACString& aBaseDomain)
{
  
  if (NS_URIIsLocalFile(mCodebase)) {
    nsCOMPtr<nsIURL> url = do_QueryInterface(mCodebase);

    if (url) {
      return url->GetFilePath(aBaseDomain);
    }
  }

  bool hasNoRelativeFlag;
  nsresult rv = NS_URIChainHasFlags(mCodebase,
                                    nsIProtocolHandler::URI_NORELATIVE,
                                    &hasNoRelativeFlag);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  if (hasNoRelativeFlag) {
    return mCodebase->GetSpec(aBaseDomain);
  }

  
  
  nsCOMPtr<mozIThirdPartyUtil> thirdPartyUtil =
    do_GetService(THIRDPARTYUTIL_CONTRACTID);
  if (thirdPartyUtil) {
    return thirdPartyUtil->GetBaseDomain(mCodebase, aBaseDomain);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::Read(nsIObjectInputStream* aStream)
{
  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIURI> codebase;
  nsresult rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(supports));
  if (NS_FAILED(rv)) {
    return rv;
  }

  codebase = do_QueryInterface(supports);

  nsCOMPtr<nsIURI> domain;
  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(supports));
  if (NS_FAILED(rv)) {
    return rv;
  }

  domain = do_QueryInterface(supports);

  uint32_t appId;
  rv = aStream->Read32(&appId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool inMozBrowser;
  rv = aStream->ReadBoolean(&inMozBrowser);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIContentSecurityPolicy> csp = do_QueryInterface(supports, &rv);

  rv = Init(codebase, appId, inMozBrowser);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetCsp(csp);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (csp) {
    csp->SetRequestContext(codebase, nullptr, nullptr);
  }

  SetDomain(domain);

  return NS_OK;
}

NS_IMETHODIMP
nsPrincipal::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mCodebase);

  nsresult rv = NS_WriteOptionalCompoundObject(aStream, mCodebase, NS_GET_IID(nsIURI),
                                               true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = NS_WriteOptionalCompoundObject(aStream, mDomain, NS_GET_IID(nsIURI),
                                      true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  aStream->Write32(mAppId);
  aStream->WriteBoolean(mInMozBrowser);

  rv = NS_WriteOptionalCompoundObject(aStream, mCSP,
                                      NS_GET_IID(nsIContentSecurityPolicy),
                                      true);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  

  return NS_OK;
}

uint16_t
nsPrincipal::GetAppStatus()
{
  if (mAppId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    NS_WARNING("Asking for app status on a principal with an unknown app id");
    return nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  }
  return nsScriptSecurityManager::AppStatusForPrincipal(this);
}




static inline bool
IsWhitelistingTestDomains()
{
  return gIsWhitelistingTestDomains;
}



static bool
IsOnFullDomainWhitelist(nsIURI* aURI)
{
  nsAutoCString hostStr;
  nsresult rv = aURI->GetHost(hostStr);
  NS_ENSURE_SUCCESS(rv, false);

  
  
  static const nsLiteralCString sFullDomainsOnWhitelist[] = {
    
    NS_LITERAL_CSTRING("test1.example.org"),
    NS_LITERAL_CSTRING("map.baidu.com"),
    NS_LITERAL_CSTRING("music.baidu.com"),
    NS_LITERAL_CSTRING("3g.163.com"),
    NS_LITERAL_CSTRING("3glogo.gtimg.com"), 
    NS_LITERAL_CSTRING("info.3g.qq.com"), 
    NS_LITERAL_CSTRING("3gimg.qq.com"), 
    NS_LITERAL_CSTRING("img.m.baidu.com"), 
    NS_LITERAL_CSTRING("m.mogujie.com"),
    NS_LITERAL_CSTRING("touch.qunar.com"),
  };
  static const size_t sNumFullDomainsOnWhitelist =
    MOZ_ARRAY_LENGTH(sFullDomainsOnWhitelist);

  
  const size_t firstWhitelistIdx = IsWhitelistingTestDomains() ? 0 : 1;

  for (size_t i = firstWhitelistIdx; i < sNumFullDomainsOnWhitelist; ++i) {
    if (hostStr == sFullDomainsOnWhitelist[i]) {
      return true;
    }
  }
  return false;
}




static bool
IsOnBaseDomainWhitelist(nsIURI* aURI)
{
  static const nsLiteralCString sBaseDomainsOnWhitelist[] = {
    
    NS_LITERAL_CSTRING("test2.example.org"),
    NS_LITERAL_CSTRING("tbcdn.cn"), 
    NS_LITERAL_CSTRING("dpfile.com"), 
    NS_LITERAL_CSTRING("hao123img.com"), 
  };
  static const size_t sNumBaseDomainsOnWhitelist =
    MOZ_ARRAY_LENGTH(sBaseDomainsOnWhitelist);

  nsCOMPtr<nsIEffectiveTLDService> tldService =
    do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);

  if (tldService) {
    
    const size_t firstWhitelistIdx = IsWhitelistingTestDomains() ? 0 : 1;

    
    
    
    
    
    
    const uint32_t maxSubdomainDepth = IsWhitelistingTestDomains() ? 1 : 0;

    for (uint32_t subdomainDepth = 0;
         subdomainDepth <= maxSubdomainDepth; ++subdomainDepth) {

      
      nsAutoCString baseDomainStr;
      nsresult rv = tldService->GetBaseDomain(aURI, subdomainDepth,
                                              baseDomainStr);
      if (NS_FAILED(rv)) {
        
        
        return false;
      }

      
      for (size_t i = firstWhitelistIdx; i < sNumBaseDomainsOnWhitelist; ++i) {
        if (baseDomainStr == sBaseDomainsOnWhitelist[i]) {
          return true;
        }
      }
    }
  }

  return false;
}


static bool
IsOnCSSUnprefixingWhitelistImpl(nsIURI* aURI)
{
  
  nsAutoCString schemeStr;
  nsresult rv = aURI->GetScheme(schemeStr);
  NS_ENSURE_SUCCESS(rv, false);

  
  if (!(StringBeginsWith(schemeStr, NS_LITERAL_CSTRING("http")) &&
        (schemeStr.Length() == 4 ||
         (schemeStr.Length() == 5 && schemeStr[4] == 's')))) {
    return false;
  }

  return (IsOnFullDomainWhitelist(aURI) ||
          IsOnBaseDomainWhitelist(aURI));
}


bool
nsPrincipal::IsOnCSSUnprefixingWhitelist()
{
  if (mIsOnCSSUnprefixingWhitelist.isNothing()) {
    
    
    
    mIsOnCSSUnprefixingWhitelist.emplace(
      mCodebaseImmutable &&
      IsOnCSSUnprefixingWhitelistImpl(mCodebase));
  }

  return *mIsOnCSSUnprefixingWhitelist;
}



static const char EXPANDED_PRINCIPAL_SPEC[] = "[Expanded Principal]";

NS_IMPL_CLASSINFO(nsExpandedPrincipal, nullptr, nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_EXPANDEDPRINCIPAL_CID)
NS_IMPL_QUERY_INTERFACE_CI(nsExpandedPrincipal,
                           nsIPrincipal,
                           nsIExpandedPrincipal)
NS_IMPL_CI_INTERFACE_GETTER(nsExpandedPrincipal,
                             nsIPrincipal,
                             nsIExpandedPrincipal)
NS_IMPL_ADDREF_INHERITED(nsExpandedPrincipal, nsBasePrincipal)
NS_IMPL_RELEASE_INHERITED(nsExpandedPrincipal, nsBasePrincipal)

nsExpandedPrincipal::nsExpandedPrincipal(nsTArray<nsCOMPtr <nsIPrincipal> > &aWhiteList)
{
  mPrincipals.AppendElements(aWhiteList);
}

nsExpandedPrincipal::~nsExpandedPrincipal()
{ }

NS_IMETHODIMP
nsExpandedPrincipal::GetDomain(nsIURI** aDomain)
{
  *aDomain = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::SetDomain(nsIURI* aDomain)
{
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetOrigin(char** aOrigin)
{
  *aOrigin = ToNewCString(NS_LITERAL_CSTRING(EXPANDED_PRINCIPAL_SPEC));
  return *aOrigin ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

typedef nsresult (NS_STDCALL nsIPrincipal::*nsIPrincipalMemFn)(nsIPrincipal* aOther,
                                                               bool* aResult);
#define CALL_MEMBER_FUNCTION(THIS,MEM_FN)  ((THIS)->*(MEM_FN))





static nsresult
Equals(nsExpandedPrincipal* aThis, nsIPrincipalMemFn aFn, nsIPrincipal* aOther,
       bool* aResult)
{
  
  
  *aResult = false;
  
  nsresult rv = CALL_MEMBER_FUNCTION(aThis, aFn)(aOther, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!*aResult)
    return NS_OK;

  
  rv = CALL_MEMBER_FUNCTION(aOther, aFn)(aThis, aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::Equals(nsIPrincipal* aOther, bool* aResult)
{
  return ::Equals(this, &nsIPrincipal::Subsumes, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::EqualsConsideringDomain(nsIPrincipal* aOther, bool* aResult)
{
  return ::Equals(this, &nsIPrincipal::SubsumesConsideringDomain, aOther, aResult);
}




static nsresult
Subsumes(nsExpandedPrincipal* aThis, nsIPrincipalMemFn aFn, nsIPrincipal* aOther,
         bool* aResult)
{
  nsresult rv;
  nsCOMPtr<nsIExpandedPrincipal> expanded = do_QueryInterface(aOther);
  if (expanded) {
    
    
    nsTArray< nsCOMPtr<nsIPrincipal> >* otherList;
    expanded->GetWhiteList(&otherList);
    for (uint32_t i = 0; i < otherList->Length(); ++i){
      rv = CALL_MEMBER_FUNCTION(aThis, aFn)((*otherList)[i], aResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!*aResult) {
        
        return NS_OK;
      }
    }
  } else {
    
    nsTArray< nsCOMPtr<nsIPrincipal> >* list;
    aThis->GetWhiteList(&list);
    for (uint32_t i = 0; i < list->Length(); ++i){
      rv = CALL_MEMBER_FUNCTION((*list)[i], aFn)(aOther, aResult);
      NS_ENSURE_SUCCESS(rv, rv);
      if (*aResult) {
        
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

#undef CALL_MEMBER_FUNCTION

NS_IMETHODIMP
nsExpandedPrincipal::Subsumes(nsIPrincipal* aOther, bool* aResult)
{
  return ::Subsumes(this, &nsIPrincipal::Subsumes, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::SubsumesConsideringDomain(nsIPrincipal* aOther, bool* aResult)
{
  return ::Subsumes(this, &nsIPrincipal::SubsumesConsideringDomain, aOther, aResult);
}

NS_IMETHODIMP
nsExpandedPrincipal::CheckMayLoad(nsIURI* uri, bool aReport, bool aAllowIfInheritsPrincipal)
{
  nsresult rv;
  for (uint32_t i = 0; i < mPrincipals.Length(); ++i){
    rv = mPrincipals[i]->CheckMayLoad(uri, aReport, aAllowIfInheritsPrincipal);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  return NS_ERROR_DOM_BAD_URI;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetHashValue(uint32_t* result)
{
  MOZ_CRASH("extended principal should never be used as key in a hash map");
}

NS_IMETHODIMP
nsExpandedPrincipal::GetURI(nsIURI** aURI)
{
  *aURI = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetWhiteList(nsTArray<nsCOMPtr<nsIPrincipal> >** aWhiteList)
{
  *aWhiteList = &mPrincipals;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetJarPrefix(nsACString& aJarPrefix)
{
  aJarPrefix.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetAppStatus(uint16_t* aAppStatus)
{
  *aAppStatus = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetAppId(uint32_t* aAppId)
{
  *aAppId = nsIScriptSecurityManager::NO_APP_ID;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
  *aIsInBrowserElement = false;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetUnknownAppId(bool* aUnknownAppId)
{
  *aUnknownAppId = false;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetIsNullPrincipal(bool* aIsNullPrincipal)
{
  *aIsNullPrincipal = false;
  return NS_OK;
}

NS_IMETHODIMP
nsExpandedPrincipal::GetBaseDomain(nsACString& aBaseDomain)
{
  return NS_ERROR_NOT_AVAILABLE;
}

bool
nsExpandedPrincipal::IsOnCSSUnprefixingWhitelist()
{
  
  
  return false;
}


void
nsExpandedPrincipal::GetScriptLocation(nsACString& aStr)
{
  aStr.Assign(EXPANDED_PRINCIPAL_SPEC);
  aStr.AppendLiteral(" (");

  for (size_t i = 0; i < mPrincipals.Length(); ++i) {
    if (i != 0) {
      aStr.AppendLiteral(", ");
    }

    nsAutoCString spec;
    nsJSPrincipals::get(mPrincipals.ElementAt(i))->GetScriptLocation(spec);

    aStr.Append(spec);

  }
  aStr.Append(")");
}

#ifdef DEBUG
void nsExpandedPrincipal::dumpImpl()
{
  fprintf(stderr, "nsExpandedPrincipal (%p)\n", static_cast<void*>(this));
}
#endif 





NS_IMETHODIMP
nsExpandedPrincipal::Read(nsIObjectInputStream* aStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsExpandedPrincipal::Write(nsIObjectOutputStream* aStream)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
