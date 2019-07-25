



#include "plstr.h"
#include "prlog.h"
#include "prprf.h"
#include "nsCRTGlue.h"
#include "nsIPermissionManager.h"
#include "nsIPrivateBrowsingService.h"
#include "nsISSLStatus.h"
#include "nsISSLStatusProvider.h"
#include "nsStrictTransportSecurityService.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsStringGlue.h"
#include "nsIScriptSecurityManager.h"









#include "nsSTSPreloadList.inc"

#define STS_SET (nsIPermissionManager::ALLOW_ACTION)
#define STS_UNSET (nsIPermissionManager::UNKNOWN_ACTION)
#define STS_KNOCKOUT (nsIPermissionManager::DENY_ACTION)

#if defined(PR_LOGGING)
PRLogModuleInfo *gSTSLog = PR_NewLogModule("nsSTSService");
#endif

#define STSLOG(args) PR_LOG(gSTSLog, 4, args)

#define STS_PARSER_FAIL_IF(test,args) \
  if (test) { \
    STSLOG(args); \
    return NS_ERROR_FAILURE; \
  }



nsSTSHostEntry::nsSTSHostEntry(const char* aHost)
  : mHost(aHost)
  , mExpireTime(0)
  , mExpired(false)
  , mStsPermission(STS_UNSET)
  , mIncludeSubdomains(false)
{
}

nsSTSHostEntry::nsSTSHostEntry(const nsSTSHostEntry& toCopy)
  : mHost(toCopy.mHost)
  , mExpireTime(toCopy.mExpireTime)
  , mExpired(toCopy.mExpired)
  , mStsPermission(toCopy.mStsPermission)
  , mIncludeSubdomains(toCopy.mIncludeSubdomains)
{
}




nsStrictTransportSecurityService::nsStrictTransportSecurityService()
  : mInPrivateMode(false)
{
}

nsStrictTransportSecurityService::~nsStrictTransportSecurityService()
{
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsStrictTransportSecurityService,
                              nsIObserver,
                              nsIStrictTransportSecurityService)

nsresult
nsStrictTransportSecurityService::Init()
{
   nsresult rv;

   mPermMgr = do_GetService(NS_PERMISSIONMANAGER_CONTRACTID, &rv);
   NS_ENSURE_SUCCESS(rv, rv);

   
   nsCOMPtr<nsIPrivateBrowsingService> pbs =
     do_GetService(NS_PRIVATE_BROWSING_SERVICE_CONTRACTID);
   if (pbs)
     pbs->GetPrivateBrowsingEnabled(&mInPrivateMode);

   mObserverService = mozilla::services::GetObserverService();
   if (mObserverService)
     mObserverService->AddObserver(this, NS_PRIVATE_BROWSING_SWITCH_TOPIC, false);

   if (mInPrivateMode)
     mPrivateModeHostTable.Init();

   return NS_OK;
}

nsresult
nsStrictTransportSecurityService::GetHost(nsIURI *aURI, nsACString &aResult)
{
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(aURI);
  if (!innerURI) return NS_ERROR_FAILURE;

  nsresult rv = innerURI->GetAsciiHost(aResult);

  if (NS_FAILED(rv) || aResult.IsEmpty())
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

nsresult
nsStrictTransportSecurityService::GetPrincipalForURI(nsIURI* aURI,
                                                     nsIPrincipal** aPrincipal)
{
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> securityManager =
     do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCAutoString host;
  rv = GetHost(aURI, host);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("https://") + host);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  return securityManager->GetNoAppCodebasePrincipal(uri, aPrincipal);
}

nsresult
nsStrictTransportSecurityService::SetStsState(nsIURI* aSourceURI,
                                              int64_t maxage,
                                              bool includeSubdomains)
{
  
  
  if (!maxage)
    return RemoveStsState(aSourceURI);

  
  
  int64_t expiretime = (PR_Now() / PR_USEC_PER_MSEC) +
                       (maxage * PR_MSEC_PER_SEC);

  
  STSLOG(("STS: maxage permission SET, adding permission\n"));
  nsresult rv = AddPermission(aSourceURI,
                              STS_PERMISSION,
                              (uint32_t) STS_SET,
                              (uint32_t) nsIPermissionManager::EXPIRE_TIME,
                              expiretime);
  NS_ENSURE_SUCCESS(rv, rv);

  if (includeSubdomains) {
    
    STSLOG(("STS: subdomains permission SET, adding permission\n"));
    rv = AddPermission(aSourceURI,
                       STS_SUBDOMAIN_PERMISSION,
                       (uint32_t) STS_SET,
                       (uint32_t) nsIPermissionManager::EXPIRE_TIME,
                       expiretime);
    NS_ENSURE_SUCCESS(rv, rv);
  } else { 
    nsCAutoString hostname;
    rv = GetHost(aSourceURI, hostname);
    NS_ENSURE_SUCCESS(rv, rv);

    STSLOG(("STS: subdomains permission UNSET, removing any existing ones\n"));
    rv = RemovePermission(hostname, STS_SUBDOMAIN_PERMISSION);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsStrictTransportSecurityService::RemoveStsState(nsIURI* aURI)
{
  
  
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);

  nsCAutoString hostname;
  nsresult rv = GetHost(aURI, hostname);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemovePermission(hostname, STS_PERMISSION);
  NS_ENSURE_SUCCESS(rv, rv);
  STSLOG(("STS: deleted maxage permission\n"));

  rv = RemovePermission(hostname, STS_SUBDOMAIN_PERMISSION);
  NS_ENSURE_SUCCESS(rv, rv);
  STSLOG(("STS: deleted subdomains permission\n"));

  return NS_OK;
}

NS_IMETHODIMP
nsStrictTransportSecurityService::ProcessStsHeader(nsIURI* aSourceURI,
                                                   const char* aHeader)
{
  
  
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);

  char * header = NS_strdup(aHeader);
  if (!header) return NS_ERROR_OUT_OF_MEMORY;
  nsresult rv = ProcessStsHeaderMutating(aSourceURI, header);
  NS_Free(header);
  return rv;
}

nsresult
nsStrictTransportSecurityService::ProcessStsHeaderMutating(nsIURI* aSourceURI,
                                                           char* aHeader)
{
  STSLOG(("STS: ProcessStrictTransportHeader(%s)\n", aHeader));

  
  
  
  
  
  
  
  
  

  const char* directive;

  bool foundMaxAge = false;
  bool foundUnrecognizedTokens = false;
  bool includeSubdomains = false;
  int64_t maxAge = 0;

  NS_NAMED_LITERAL_CSTRING(max_age_var, "max-age");
  NS_NAMED_LITERAL_CSTRING(include_subd_var, "includesubdomains");

  while ((directive = NS_strtok(";", &aHeader))) {
    
    directive = NS_strspnp(" \t", directive);
    STS_PARSER_FAIL_IF(!(*directive), ("error removing initial whitespace\n."));

    if (!PL_strncasecmp(directive, max_age_var.get(), max_age_var.Length())) {
      
      directive += max_age_var.Length();
      
      directive = NS_strspnp(" \t", directive);
      STS_PARSER_FAIL_IF(*directive != '=',
                  ("No equal sign found in max-age directive\n"));

      
      STS_PARSER_FAIL_IF(*(++directive) == '\0',
                  ("No delta-seconds present\n"));

      
      STS_PARSER_FAIL_IF(PR_sscanf(directive, "%lld", &maxAge) != 1,
                  ("Could not convert delta-seconds\n"));
      STSLOG(("STS: ProcessStrictTransportHeader() STS found maxage %lld\n", maxAge));
      foundMaxAge = true;

      
      directive = NS_strspnp("0123456789 \t", directive);

      
      if (*directive != '\0') {
        foundUnrecognizedTokens = true;
        STSLOG(("Extra stuff in max-age after delta-seconds: %s \n", directive));
      }
    }
    else if (!PL_strncasecmp(directive, include_subd_var.get(), include_subd_var.Length())) {
      directive += include_subd_var.Length();

      
      
      
      if (*directive == '\0' || *directive =='\t' || *directive == ' ') {
        includeSubdomains = true;
        STSLOG(("STS: ProcessStrictTransportHeader: obtained subdomains status\n"));

        
        directive = NS_strspnp(" \t", directive);

        if (*directive != '\0') {
          foundUnrecognizedTokens = true;
          STSLOG(("Extra stuff after includesubdomains: %s\n", directive));
        }
      } else {
        foundUnrecognizedTokens = true;
        STSLOG(("Unrecognized directive in header: %s\n", directive));
      }
    }
    else {
      
      foundUnrecognizedTokens = true;
      STSLOG(("Unrecognized directive in header: %s\n", directive));
    }
  }

  
  
  STS_PARSER_FAIL_IF(!foundMaxAge,
              ("Parse ERROR: couldn't locate max-age token\n"));

  
  SetStsState(aSourceURI, maxAge, includeSubdomains);

  return foundUnrecognizedTokens ?
         NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA :
         NS_OK;
}

NS_IMETHODIMP
nsStrictTransportSecurityService::IsStsHost(const char* aHost, bool* aResult)
{
  
  
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI> uri;
  nsDependentCString hostString(aHost);
  nsresult rv = NS_NewURI(getter_AddRefs(uri),
                          NS_LITERAL_CSTRING("https://") + hostString);
  NS_ENSURE_SUCCESS(rv, rv);
  return IsStsURI(uri, aResult);
}

int STSPreloadCompare(const void *key, const void *entry)
{
  const char *keyStr = (const char *)key;
  const nsSTSPreload *preloadEntry = (const nsSTSPreload *)entry;
  return strcmp(keyStr, preloadEntry->mHost);
}




const nsSTSPreload *
nsStrictTransportSecurityService::GetPreloadListEntry(const char *aHost)
{
  return (const nsSTSPreload *) bsearch(aHost,
                                        kSTSPreloadList,
                                        PR_ARRAY_SIZE(kSTSPreloadList),
                                        sizeof(nsSTSPreload),
                                        STSPreloadCompare);
}

NS_IMETHODIMP
nsStrictTransportSecurityService::IsStsURI(nsIURI* aURI, bool* aResult)
{
  
  
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);

  
  *aResult = false;

  nsCAutoString host;
  nsresult rv = GetHost(aURI, host);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsSTSPreload *preload = nullptr;
  nsSTSHostEntry *pbEntry = nullptr;

  if (mInPrivateMode) {
    pbEntry = mPrivateModeHostTable.GetEntry(host.get());
  }

  nsCOMPtr<nsIPrincipal> principal;
  rv = GetPrincipalForURI(aURI, getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 permMgrPermission;
  rv = mPermMgr->TestExactPermissionFromPrincipal(principal, STS_PERMISSION,
                                                  &permMgrPermission);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  
  if (pbEntry && pbEntry->mStsPermission != STS_UNSET) {
    STSLOG(("Found private browsing table entry for %s", host.get()));
    if (!pbEntry->IsExpired() && pbEntry->mStsPermission == STS_SET) {
      *aResult = true;
      return NS_OK;
    }
  }
  
  
  else if (permMgrPermission != STS_UNSET) {
    STSLOG(("Found permission manager entry for %s", host.get()));
    if (permMgrPermission == STS_SET) {
      *aResult = true;
      return NS_OK;
    }
  }
  
  
  else if (GetPreloadListEntry(host.get())) {
    STSLOG(("%s is a preloaded STS host", host.get()));
    *aResult = true;
    return NS_OK;
  }

  
  nsCOMPtr<nsIURI> domainWalkURI;
  nsCOMPtr<nsIPrincipal> domainWalkPrincipal;
  const char *subdomain;

  STSLOG(("no HSTS data for %s found, walking up domain", host.get()));
  PRUint32 offset = 0;
  for (offset = host.FindChar('.', offset) + 1;
       offset > 0;
       offset = host.FindChar('.', offset) + 1) {

    subdomain = host.get() + offset;

    
    if (strlen(subdomain) < 1) {
      break;
    }

    if (mInPrivateMode) {
      pbEntry = mPrivateModeHostTable.GetEntry(subdomain);
    }

    
    rv = NS_NewURI(getter_AddRefs(domainWalkURI),
                   NS_LITERAL_CSTRING("https://") + Substring(host, offset));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetPrincipalForURI(domainWalkURI, getter_AddRefs(domainWalkPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mPermMgr->TestExactPermissionFromPrincipal(domainWalkPrincipal,
                                                    STS_PERMISSION,
                                                    &permMgrPermission);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    
    
    if (pbEntry && pbEntry->mStsPermission != STS_UNSET) {
      STSLOG(("Found private browsing table entry for %s", subdomain));
      if (!pbEntry->IsExpired() && pbEntry->mStsPermission == STS_SET) {
        *aResult = pbEntry->mIncludeSubdomains;
        break;
      }
    }
    else if (permMgrPermission != STS_UNSET) {
      STSLOG(("Found permission manager entry for %s", subdomain));
      if (permMgrPermission == STS_SET) {
        PRUint32 subdomainPermission;
        rv = mPermMgr->TestExactPermissionFromPrincipal(domainWalkPrincipal,
                                                        STS_SUBDOMAIN_PERMISSION,
                                                        &subdomainPermission);
        NS_ENSURE_SUCCESS(rv, rv);
        *aResult = (subdomainPermission == STS_SET);
        break;
      }
    }
    
    
    else if ((preload = GetPreloadListEntry(subdomain)) != nullptr) {
      if (preload->mIncludeSubdomains) {
        STSLOG(("%s is a preloaded STS host", subdomain));
        *aResult = true;
        break;
      }
    }

    STSLOG(("no HSTS data for %s found, walking up domain", subdomain));
  }

  
  return NS_OK;
}



NS_IMETHODIMP
nsStrictTransportSecurityService::ShouldIgnoreStsHeader(nsISupports* aSecurityInfo,
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
nsStrictTransportSecurityService::Observe(nsISupports *subject,
                                          const char *topic,
                                          const PRUnichar *data)
{
  if (strcmp(topic, NS_PRIVATE_BROWSING_SWITCH_TOPIC) == 0) {
    if(NS_LITERAL_STRING(NS_PRIVATE_BROWSING_ENTER).Equals(data)) {
      
      

      if (!mPrivateModeHostTable.IsInitialized()) {
        mPrivateModeHostTable.Init();
      }
      mInPrivateMode = true;
    }
    else if (NS_LITERAL_STRING(NS_PRIVATE_BROWSING_LEAVE).Equals(data)) {
      mPrivateModeHostTable.Clear();
      mInPrivateMode = false;
    }
  }

  return NS_OK;
}





nsresult
nsStrictTransportSecurityService::AddPermission(nsIURI     *aURI,
                                                const char *aType,
                                                uint32_t   aPermission,
                                                uint32_t   aExpireType,
                                                int64_t    aExpireTime)
{
    
    
    if (!mInPrivateMode || aExpireType == nsIPermissionManager::EXPIRE_NEVER) {
      
      nsCOMPtr<nsIPrincipal> principal;
      nsresult rv = GetPrincipalForURI(aURI, getter_AddRefs(principal));
      NS_ENSURE_SUCCESS(rv, rv);

      return mPermMgr->AddFromPrincipal(principal, aType, aPermission,
                                        aExpireType, aExpireTime);
    }

    nsCAutoString host;
    nsresult rv = GetHost(aURI, host);
    NS_ENSURE_SUCCESS(rv, rv);
    STSLOG(("AddPermission for entry for %s", host.get()));

    
    

    
    
    
    
    
    

    
    
    nsSTSHostEntry* entry = mPrivateModeHostTable.PutEntry(host.get());
    if (!entry) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    STSLOG(("Created private mode entry for %s", host.get()));

    
    
    
    
    
    if (strcmp(aType, STS_SUBDOMAIN_PERMISSION) == 0) {
      entry->mIncludeSubdomains = true;
    }
    else if (strcmp(aType, STS_PERMISSION) == 0) {
      entry->mStsPermission = aPermission;
    }

    
    entry->SetExpireTime(aExpireTime);
    return NS_OK;
}

nsresult
nsStrictTransportSecurityService::RemovePermission(const nsCString  &aHost,
                                                   const char       *aType)
{
    
    
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_NewURI(getter_AddRefs(uri),
                            NS_LITERAL_CSTRING("https://") + aHost);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrincipal> principal;
    rv = GetPrincipalForURI(uri, getter_AddRefs(principal));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mInPrivateMode) {
      
      
      
      return mPermMgr->AddFromPrincipal(principal, aType,
                                        STS_KNOCKOUT,
                                        nsIPermissionManager::EXPIRE_NEVER, 0);
    }

    
    
    nsSTSHostEntry* entry = mPrivateModeHostTable.GetEntry(aHost.get());

    if (!entry) {
      entry = mPrivateModeHostTable.PutEntry(aHost.get());
      if (!entry) {
        return NS_ERROR_OUT_OF_MEMORY;
      }
      STSLOG(("Created private mode deleted mask for %s", aHost.get()));
    }

    if (strcmp(aType, STS_PERMISSION) == 0) {
      entry->mStsPermission = STS_KNOCKOUT;
    }
    else if (strcmp(aType, STS_SUBDOMAIN_PERMISSION) == 0) {
      entry->mIncludeSubdomains = false;
    }

    return NS_OK;
}
