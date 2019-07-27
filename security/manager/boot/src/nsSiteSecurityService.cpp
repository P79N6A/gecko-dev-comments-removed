



#include "nsSiteSecurityService.h"

#include "mozilla/LinkedList.h"
#include "mozilla/Preferences.h"
#include "mozilla/Base64.h"
#include "base64.h"
#include "nsCRTGlue.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsISocketProvider.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsSecurityHeaderParser.h"
#include "nsString.h"
#include "nsThreadUtils.h"
#include "pkix/pkixtypes.h"
#include "plstr.h"
#include "prlog.h"
#include "prnetdb.h"
#include "prprf.h"
#include "PublicKeyPinningService.h"
#include "ScopedNSSTypes.h"
#include "nsXULAppAPI.h"








#include "nsSTSPreloadList.inc"

using namespace mozilla;
using namespace mozilla::psm;

#if defined(PR_LOGGING)
static PRLogModuleInfo *
GetSSSLog()
{
  static PRLogModuleInfo *gSSSLog;
  if (!gSSSLog)
    gSSSLog = PR_NewLogModule("nsSSService");
  return gSSSLog;
}
#endif

#define SSSLOG(args) PR_LOG(GetSSSLog(), 4, args)



SiteHSTSState::SiteHSTSState(nsCString& aStateString)
  : mHSTSExpireTime(0)
  , mHSTSState(SecurityPropertyUnset)
  , mHSTSIncludeSubdomains(false)
{
  uint32_t hstsState = 0;
  uint32_t hstsIncludeSubdomains = 0; 
  int32_t matches = PR_sscanf(aStateString.get(), "%lld,%lu,%lu",
                              &mHSTSExpireTime, &hstsState,
                              &hstsIncludeSubdomains);
  bool valid = (matches == 3 &&
                (hstsIncludeSubdomains == 0 || hstsIncludeSubdomains == 1) &&
                ((SecurityPropertyState)hstsState == SecurityPropertyUnset ||
                 (SecurityPropertyState)hstsState == SecurityPropertySet ||
                 (SecurityPropertyState)hstsState == SecurityPropertyKnockout));
  if (valid) {
    mHSTSState = (SecurityPropertyState)hstsState;
    mHSTSIncludeSubdomains = (hstsIncludeSubdomains == 1);
  } else {
    SSSLOG(("%s is not a valid SiteHSTSState", aStateString.get()));
    mHSTSExpireTime = 0;
    mHSTSState = SecurityPropertyUnset;
    mHSTSIncludeSubdomains = false;
  }
}

SiteHSTSState::SiteHSTSState(PRTime aHSTSExpireTime,
                             SecurityPropertyState aHSTSState,
                             bool aHSTSIncludeSubdomains)

  : mHSTSExpireTime(aHSTSExpireTime)
  , mHSTSState(aHSTSState)
  , mHSTSIncludeSubdomains(aHSTSIncludeSubdomains)
{
}

void
SiteHSTSState::ToString(nsCString& aString)
{
  aString.Truncate();
  aString.AppendInt(mHSTSExpireTime);
  aString.Append(',');
  aString.AppendInt(mHSTSState);
  aString.Append(',');
  aString.AppendInt(static_cast<uint32_t>(mHSTSIncludeSubdomains));
}


static bool
stringIsBase64EncodingOf256bitValue(nsCString& encodedString) {
  nsAutoCString binaryValue;
  nsresult rv = mozilla::Base64Decode(encodedString, binaryValue);
  if (NS_FAILED(rv)) {
    return false;
  }
  if (binaryValue.Length() != SHA256_LENGTH) {
    return false;
  }
  return true;
}

SiteHPKPState::SiteHPKPState()
  : mExpireTime(0)
  , mState(SecurityPropertyUnset)
  , mIncludeSubdomains(false)
{
}

SiteHPKPState::SiteHPKPState(nsCString& aStateString)
  : mExpireTime(0)
  , mState(SecurityPropertyUnset)
  , mIncludeSubdomains(false)
{
  uint32_t hpkpState = 0;
  uint32_t hpkpIncludeSubdomains = 0; 
  const uint32_t MaxMergedHPKPPinSize = 1024;
  char mergedHPKPins[MaxMergedHPKPPinSize];
  memset(mergedHPKPins, 0, MaxMergedHPKPPinSize);

  if (aStateString.Length() >= MaxMergedHPKPPinSize) {
    SSSLOG(("SSS: Cannot parse PKPState string, too large\n"));
    return;
  }

  int32_t matches = PR_sscanf(aStateString.get(), "%lld,%lu,%lu,%s",
                              &mExpireTime, &hpkpState,
                              &hpkpIncludeSubdomains, mergedHPKPins);
  bool valid = (matches == 4 &&
                (hpkpIncludeSubdomains == 0 || hpkpIncludeSubdomains == 1) &&
                ((SecurityPropertyState)hpkpState == SecurityPropertyUnset ||
                 (SecurityPropertyState)hpkpState == SecurityPropertySet ||
                 (SecurityPropertyState)hpkpState == SecurityPropertyKnockout));

  SSSLOG(("SSS: loading SiteHPKPState matches=%d\n", matches));
  const uint32_t SHA256Base64Len = 44;

  if (valid && (SecurityPropertyState)hpkpState == SecurityPropertySet) {
    
    const char* cur = mergedHPKPins;
    nsAutoCString pin;
    uint32_t collectedLen = 0;
    mergedHPKPins[MaxMergedHPKPPinSize - 1] = 0;
    size_t totalLen = strlen(mergedHPKPins);
    while (collectedLen + SHA256Base64Len <= totalLen) {
      pin.Assign(cur, SHA256Base64Len);
      if (stringIsBase64EncodingOf256bitValue(pin)) {
        mSHA256keys.AppendElement(pin);
      }
      cur += SHA256Base64Len;
      collectedLen += SHA256Base64Len;
    }
    if (mSHA256keys.IsEmpty()) {
      valid = false;
    }
  }
  if (valid) {
    mState = (SecurityPropertyState)hpkpState;
    mIncludeSubdomains = (hpkpIncludeSubdomains == 1);
  } else {
    SSSLOG(("%s is not a valid SiteHPKPState", aStateString.get()));
    mExpireTime = 0;
    mState = SecurityPropertyUnset;
    mIncludeSubdomains = false;
    if (!mSHA256keys.IsEmpty()) {
      mSHA256keys.Clear();
    }
  }
}

SiteHPKPState::SiteHPKPState(PRTime aExpireTime,
                             SecurityPropertyState aState,
                             bool aIncludeSubdomains,
                             nsTArray<nsCString>& aSHA256keys)
  : mExpireTime(aExpireTime)
  , mState(aState)
  , mIncludeSubdomains(aIncludeSubdomains)
  , mSHA256keys(aSHA256keys)
{
}

void
SiteHPKPState::ToString(nsCString& aString)
{
  aString.Truncate();
  aString.AppendInt(mExpireTime);
  aString.Append(',');
  aString.AppendInt(mState);
  aString.Append(',');
  aString.AppendInt(static_cast<uint32_t>(mIncludeSubdomains));
  aString.Append(',');
  for (unsigned int i = 0; i < mSHA256keys.Length(); i++) {
    aString.Append(mSHA256keys[i]);
  }
}



nsSiteSecurityService::nsSiteSecurityService()
  : mUsePreloadList(true)
  , mPreloadListTimeOffset(0)
{
}

nsSiteSecurityService::~nsSiteSecurityService()
{
}

NS_IMPL_ISUPPORTS(nsSiteSecurityService,
                  nsIObserver,
                  nsISiteSecurityService)

nsresult
nsSiteSecurityService::Init()
{
   
   if (XRE_GetProcessType() != GeckoProcessType_Default) {
     MOZ_CRASH("Child process: no direct access to nsSiteSecurityService");
   }

  
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("nsSiteSecurityService initialized off main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  mUsePreloadList = mozilla::Preferences::GetBool(
    "network.stricttransportsecurity.preloadlist", true);
  mozilla::Preferences::AddStrongObserver(this,
    "network.stricttransportsecurity.preloadlist");
  mPreloadListTimeOffset = mozilla::Preferences::GetInt(
    "test.currentTimeOffsetSeconds", 0);
  mozilla::Preferences::AddStrongObserver(this,
    "test.currentTimeOffsetSeconds");
  mSiteStateStorage =
    new mozilla::DataStorage(NS_LITERAL_STRING("SiteSecurityServiceState.txt"));
  bool storageWillPersist = false;
  nsresult rv = mSiteStateStorage->Init(storageWillPersist);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }
  
  
  
  if (!storageWillPersist) {
    NS_WARNING("site security information will not be persisted");
  }

  return NS_OK;
}

nsresult
nsSiteSecurityService::GetHost(nsIURI *aURI, nsACString &aResult)
{
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  if (!innerURI) return NS_ERROR_FAILURE;

  nsresult rv = innerURI->GetAsciiHost(aResult);

  if (NS_FAILED(rv) || aResult.IsEmpty())
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

static void
SetStorageKey(nsAutoCString& storageKey, nsCString& hostname, uint32_t aType)
{
  storageKey = hostname;
  switch (aType) {
    case nsISiteSecurityService::HEADER_HSTS:
      storageKey.AppendLiteral(":HSTS");
      break;
    case nsISiteSecurityService::HEADER_HPKP:
      storageKey.AppendLiteral(":HPKP");
      break;
    default:
      NS_ASSERTION(false, "SSS:SetStorageKey got invalid type");
  }
}

nsresult
nsSiteSecurityService::SetHSTSState(uint32_t aType,
                                    nsIURI* aSourceURI,
                                    int64_t maxage,
                                    bool includeSubdomains,
                                    uint32_t flags)
{
  
  
  if (!maxage) {
    return RemoveState(aType, aSourceURI, flags);
  }

  
  
  int64_t expiretime = (PR_Now() / PR_USEC_PER_MSEC) +
                       (maxage * PR_MSEC_PER_SEC);

  SiteHSTSState siteState(expiretime, SecurityPropertySet, includeSubdomains);
  nsAutoCString stateString;
  siteState.ToString(stateString);
  nsAutoCString hostname;
  nsresult rv = GetHost(aSourceURI, hostname);
  NS_ENSURE_SUCCESS(rv, rv);
  SSSLOG(("SSS: setting state for %s", hostname.get()));
  bool isPrivate = flags & nsISocketProvider::NO_PERMANENT_STORAGE;
  mozilla::DataStorageType storageType = isPrivate
                                         ? mozilla::DataStorage_Private
                                         : mozilla::DataStorage_Persistent;
  nsAutoCString storageKey;
  SetStorageKey(storageKey, hostname, aType);
  rv = mSiteStateStorage->Put(storageKey, stateString, storageType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsSiteSecurityService::RemoveState(uint32_t aType, nsIURI* aURI,
                                   uint32_t aFlags)
{
  
  NS_ENSURE_TRUE(aType == nsISiteSecurityService::HEADER_HSTS,
                 NS_ERROR_NOT_IMPLEMENTED);

  nsAutoCString hostname;
  nsresult rv = GetHost(aURI, hostname);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isPrivate = aFlags & nsISocketProvider::NO_PERMANENT_STORAGE;
  mozilla::DataStorageType storageType = isPrivate
                                         ? mozilla::DataStorage_Private
                                         : mozilla::DataStorage_Persistent;
  
  if (GetPreloadListEntry(hostname.get())) {
    SSSLOG(("SSS: storing knockout entry for %s", hostname.get()));
    SiteHSTSState siteState(0, SecurityPropertyKnockout, false);
    nsAutoCString stateString;
    siteState.ToString(stateString);
    nsAutoCString storageKey;
    SetStorageKey(storageKey, hostname, aType);
    rv = mSiteStateStorage->Put(storageKey, stateString, storageType);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    SSSLOG(("SSS: removing entry for %s", hostname.get()));
    nsAutoCString storageKey;
    SetStorageKey(storageKey, hostname, aType);
    mSiteStateStorage->Remove(storageKey, storageType);
  }

  return NS_OK;
}

static bool
HostIsIPAddress(const char *hostname)
{
  PRNetAddr hostAddr;
  return (PR_StringToNetAddr(hostname, &hostAddr) == PR_SUCCESS);
}

NS_IMETHODIMP
nsSiteSecurityService::ProcessHeader(uint32_t aType,
                                     nsIURI* aSourceURI,
                                     const char* aHeader,
                                     uint32_t aFlags,
                                     uint64_t *aMaxAge,
                                     bool *aIncludeSubdomains)
{
  
  NS_ENSURE_TRUE(aType == nsISiteSecurityService::HEADER_HSTS,
                 NS_ERROR_NOT_IMPLEMENTED);

  if (aMaxAge != nullptr) {
    *aMaxAge = 0;
  }

  if (aIncludeSubdomains != nullptr) {
    *aIncludeSubdomains = false;
  }

  nsAutoCString host;
  nsresult rv = GetHost(aSourceURI, host);
  NS_ENSURE_SUCCESS(rv, rv);
  if (HostIsIPAddress(host.get())) {
    
    return NS_OK;
  }

  char * header = NS_strdup(aHeader);
  if (!header) return NS_ERROR_OUT_OF_MEMORY;
  rv = ProcessHeaderMutating(aType, aSourceURI, header, aFlags,
                             aMaxAge, aIncludeSubdomains);
  NS_Free(header);
  return rv;
}

nsresult
nsSiteSecurityService::ProcessHeaderMutating(uint32_t aType,
                                             nsIURI* aSourceURI,
                                             char* aHeader,
                                             uint32_t aFlags,
                                             uint64_t *aMaxAge,
                                             bool *aIncludeSubdomains)
{
  SSSLOG(("SSS: processing header '%s'", aHeader));

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  bool foundMaxAge = false;
  bool foundIncludeSubdomains = false;
  bool foundUnrecognizedDirective = false;
  int64_t maxAge = 0;

  NS_NAMED_LITERAL_CSTRING(max_age_var, "max-age");
  NS_NAMED_LITERAL_CSTRING(include_subd_var, "includesubdomains");


  nsSecurityHeaderParser parser(aHeader);
  nsresult rv = parser.Parse();
  if (NS_FAILED(rv)) {
    SSSLOG(("SSS: could not parse header"));
    return rv;
  }
  mozilla::LinkedList<nsSecurityHeaderDirective> *directives = parser.GetDirectives();

  for (nsSecurityHeaderDirective *directive = directives->getFirst();
       directive != nullptr; directive = directive->getNext()) {
    if (directive->mName.Length() == max_age_var.Length() &&
        directive->mName.EqualsIgnoreCase(max_age_var.get(),
                                          max_age_var.Length())) {
      if (foundMaxAge) {
        SSSLOG(("SSS: found two max-age directives"));
        return NS_ERROR_FAILURE;
      }

      SSSLOG(("SSS: found max-age directive"));
      foundMaxAge = true;

      size_t len = directive->mValue.Length();
      for (size_t i = 0; i < len; i++) {
        char chr = directive->mValue.CharAt(i);
        if (chr < '0' || chr > '9') {
          SSSLOG(("SSS: invalid value for max-age directive"));
          return NS_ERROR_FAILURE;
        }
      }

      if (PR_sscanf(directive->mValue.get(), "%lld", &maxAge) != 1) {
        SSSLOG(("SSS: could not parse delta-seconds"));
        return NS_ERROR_FAILURE;
      }

      SSSLOG(("SSS: parsed delta-seconds: %lld", maxAge));
    } else if (directive->mName.Length() == include_subd_var.Length() &&
               directive->mName.EqualsIgnoreCase(include_subd_var.get(),
                                                 include_subd_var.Length())) {
      if (foundIncludeSubdomains) {
        SSSLOG(("SSS: found two includeSubdomains directives"));
        return NS_ERROR_FAILURE;
      }

      SSSLOG(("SSS: found includeSubdomains directive"));
      foundIncludeSubdomains = true;

      if (directive->mValue.Length() != 0) {
        SSSLOG(("SSS: includeSubdomains directive unexpectedly had value '%s'",
                directive->mValue.get()));
        return NS_ERROR_FAILURE;
      }
    } else {
      SSSLOG(("SSS: ignoring unrecognized directive '%s'",
              directive->mName.get()));
      foundUnrecognizedDirective = true;
    }
  }

  
  
  if (!foundMaxAge) {
    SSSLOG(("SSS: did not encounter required max-age directive"));
    return NS_ERROR_FAILURE;
  }

  
  SetHSTSState(aType, aSourceURI, maxAge, foundIncludeSubdomains, aFlags);

  if (aMaxAge != nullptr) {
    *aMaxAge = (uint64_t)maxAge;
  }

  if (aIncludeSubdomains != nullptr) {
    *aIncludeSubdomains = foundIncludeSubdomains;
  }

  return foundUnrecognizedDirective ? NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA
                                    : NS_OK;
}

NS_IMETHODIMP
nsSiteSecurityService::IsSecureURI(uint32_t aType, nsIURI* aURI,
                                   uint32_t aFlags, bool* aResult)
{
  
  NS_ENSURE_TRUE(aType == nsISiteSecurityService::HEADER_HSTS,
                 NS_ERROR_NOT_IMPLEMENTED);

  nsAutoCString hostname;
  nsresult rv = GetHost(aURI, hostname);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (HostIsIPAddress(hostname.get())) {
    *aResult = false;
    return NS_OK;
  }

  return IsSecureHost(aType, hostname.get(), aFlags, aResult);
}

int STSPreloadCompare(const void *key, const void *entry)
{
  const char *keyStr = (const char *)key;
  const nsSTSPreload *preloadEntry = (const nsSTSPreload *)entry;
  return strcmp(keyStr, preloadEntry->mHost);
}




const nsSTSPreload *
nsSiteSecurityService::GetPreloadListEntry(const char *aHost)
{
  PRTime currentTime = PR_Now() + (mPreloadListTimeOffset * PR_USEC_PER_SEC);
  if (mUsePreloadList && currentTime < gPreloadListExpirationTime) {
    return (const nsSTSPreload *) bsearch(aHost,
                                          kSTSPreloadList,
                                          mozilla::ArrayLength(kSTSPreloadList),
                                          sizeof(nsSTSPreload),
                                          STSPreloadCompare);
  }

  return nullptr;
}

NS_IMETHODIMP
nsSiteSecurityService::IsSecureHost(uint32_t aType, const char* aHost,
                                    uint32_t aFlags, bool* aResult)
{
  
  NS_ENSURE_TRUE(aType == nsISiteSecurityService::HEADER_HSTS ||
                 aType == nsISiteSecurityService::HEADER_HPKP,
                 NS_ERROR_NOT_IMPLEMENTED);

  
  *aResult = false;

  
  if (HostIsIPAddress(aHost)) {
    return NS_OK;
  }

  if (aType == nsISiteSecurityService::HEADER_HPKP) {
    ScopedCERTCertList certList(CERT_NewCertList());
    if (!certList) {
      return NS_ERROR_FAILURE;
    }
    
    
    *aResult = !PublicKeyPinningService::ChainHasValidPins(certList,
                                                           aHost,
                                                           mozilla::pkix::Now(),
                                                           false);
    return NS_OK;
  }

  
  nsAutoCString host(aHost);
  ToLowerCase(host);
  if (host.EqualsLiteral("chart.apis.google.com") ||
      StringEndsWith(host, NS_LITERAL_CSTRING(".chart.apis.google.com"))) {
    return NS_OK;
  }

  const nsSTSPreload *preload = nullptr;

  
  
  
  
  
  
  
  bool isPrivate = aFlags & nsISocketProvider::NO_PERMANENT_STORAGE;
  mozilla::DataStorageType storageType = isPrivate
                                         ? mozilla::DataStorage_Private
                                         : mozilla::DataStorage_Persistent;
  nsAutoCString storageKey;
  SetStorageKey(storageKey, host, aType);
  nsCString value = mSiteStateStorage->Get(storageKey, storageType);
  SiteHSTSState siteState(value);
  if (siteState.mHSTSState != SecurityPropertyUnset) {
    SSSLOG(("Found entry for %s", host.get()));
    bool expired = siteState.IsExpired(aType);
    if (!expired && siteState.mHSTSState == SecurityPropertySet) {
      *aResult = true;
      return NS_OK;
    }

    
    if (expired && !GetPreloadListEntry(host.get())) {
      mSiteStateStorage->Remove(storageKey, storageType);
    }
  }
  
  
  else if (GetPreloadListEntry(host.get())) {
    SSSLOG(("%s is a preloaded STS host", host.get()));
    *aResult = true;
    return NS_OK;
  }

  SSSLOG(("no HSTS data for %s found, walking up domain", host.get()));
  const char *subdomain;

  uint32_t offset = 0;
  for (offset = host.FindChar('.', offset) + 1;
       offset > 0;
       offset = host.FindChar('.', offset) + 1) {

    subdomain = host.get() + offset;

    
    if (strlen(subdomain) < 1) {
      break;
    }

    
    
    
    
    
    
    nsCString subdomainString(subdomain);
    nsAutoCString storageKey;
    SetStorageKey(storageKey, subdomainString, aType);
    value = mSiteStateStorage->Get(storageKey, storageType);
    SiteHSTSState siteState(value);
    if (siteState.mHSTSState != SecurityPropertyUnset) {
      SSSLOG(("Found entry for %s", subdomain));
      bool expired = siteState.IsExpired(aType);
      if (!expired && siteState.mHSTSState == SecurityPropertySet) {
        *aResult = siteState.mHSTSIncludeSubdomains;
        break;
      }

      
      if (expired && !GetPreloadListEntry(subdomain)) {
        mSiteStateStorage->Remove(storageKey, storageType);
      }
    }
    
    
    else if ((preload = GetPreloadListEntry(subdomain)) != nullptr) {
      if (preload->mIncludeSubdomains) {
        SSSLOG(("%s is a preloaded STS host", subdomain));
        *aResult = true;
        break;
      }
    }

    SSSLOG(("no HSTS data for %s found, walking up domain", subdomain));
  }

  
  return NS_OK;
}



NS_IMETHODIMP
nsSiteSecurityService::ShouldIgnoreHeaders(nsISupports* aSecurityInfo,
                                           bool* aResult)
{
  nsresult rv;
  bool tlsIsBroken = false;
  nsCOMPtr<nsISSLStatusProvider> sslprov = do_QueryInterface(aSecurityInfo);
  NS_ENSURE_TRUE(sslprov, NS_ERROR_FAILURE);

  nsCOMPtr<nsISSLStatus> sslstat;
  rv = sslprov->GetSSLStatus(getter_AddRefs(sslstat));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(sslstat, NS_ERROR_FAILURE);

  bool trustcheck;
  rv = sslstat->GetIsDomainMismatch(&trustcheck);
  NS_ENSURE_SUCCESS(rv, rv);
  tlsIsBroken = tlsIsBroken || trustcheck;

  rv = sslstat->GetIsNotValidAtThisTime(&trustcheck);
  NS_ENSURE_SUCCESS(rv, rv);
  tlsIsBroken = tlsIsBroken || trustcheck;

  rv = sslstat->GetIsUntrusted(&trustcheck);
  NS_ENSURE_SUCCESS(rv, rv);
  tlsIsBroken = tlsIsBroken || trustcheck;

  *aResult = tlsIsBroken;
  return NS_OK;
}

NS_IMETHODIMP
nsSiteSecurityService::ClearAll()
{
  return mSiteStateStorage->Clear();
}

NS_IMETHODIMP
nsSiteSecurityService::GetKeyPinsForHostname(const char* aHostname,
                                             mozilla::pkix::Time& aEvalTime,
                                              nsTArray<nsCString>& pinArray,
                                              bool* aIncludeSubdomains,
                                              bool* afound) {
  NS_ENSURE_ARG(afound);
  NS_ENSURE_ARG(aHostname);

  SSSLOG(("Top of GetKeyPinsForHostname"));
  *afound = false;
  *aIncludeSubdomains = false;
  pinArray.Clear();

  nsAutoCString host(aHostname);
  nsAutoCString storageKey;
  SetStorageKey(storageKey, host, nsISiteSecurityService::HEADER_HPKP);

  SSSLOG(("storagekey '%s'\n", storageKey.get()));
  mozilla::DataStorageType storageType = mozilla::DataStorage_Persistent;
  nsCString value = mSiteStateStorage->Get(storageKey, storageType);
  
  SiteHPKPState foundEntry(value);
  if (foundEntry.mState != SecurityPropertySet ||
      foundEntry.IsExpired(aEvalTime) ||
      foundEntry.mSHA256keys.Length() < 1 ) {
    return NS_OK;
  }
  pinArray = foundEntry.mSHA256keys;
  *aIncludeSubdomains = foundEntry.mIncludeSubdomains;
  *afound = true;
  return NS_OK;
}

NS_IMETHODIMP
nsSiteSecurityService::SetKeyPins(const char* aHost, bool aIncludeSubdomains,
                                  uint32_t aMaxAge, uint32_t aPinCount,
                                  const char** aSha256Pins,
                                  bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aHost);
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aSha256Pins);

  SSSLOG(("Top of SetPins"));

  
  
  int64_t expireTime = (PR_Now() / PR_USEC_PER_MSEC) +
                       (aMaxAge * PR_MSEC_PER_SEC);
  nsTArray<nsCString> sha256keys;
  for (unsigned int i = 0; i < aPinCount; i++) {
    nsAutoCString pin(aSha256Pins[i]);
    SSSLOG(("SetPins pin=%s\n", pin.get()));
    if (!stringIsBase64EncodingOf256bitValue(pin)) {
      return NS_ERROR_INVALID_ARG;
    }
    sha256keys.AppendElement(pin);
  }
  SiteHPKPState dynamicEntry(expireTime, SecurityPropertySet,
                             aIncludeSubdomains, sha256keys);

  nsAutoCString host(aHost);
  nsAutoCString storageKey;
  SetStorageKey(storageKey, host, nsISiteSecurityService::HEADER_HPKP);
  
  mozilla::DataStorageType storageType = mozilla::DataStorage_Persistent;
  nsAutoCString stateString;
  dynamicEntry.ToString(stateString);
  nsresult rv = mSiteStateStorage->Put(storageKey, stateString, storageType);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}





NS_IMETHODIMP
nsSiteSecurityService::Observe(nsISupports *subject,
                               const char *topic,
                               const char16_t *data)
{
  
  if (!NS_IsMainThread()) {
    NS_NOTREACHED("Preferences accessed off main thread");
    return NS_ERROR_NOT_SAME_THREAD;
  }

  if (strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0) {
    mUsePreloadList = mozilla::Preferences::GetBool(
      "network.stricttransportsecurity.preloadlist", true);
    mPreloadListTimeOffset =
      mozilla::Preferences::GetInt("test.currentTimeOffsetSeconds", 0);
  }

  return NS_OK;
}
