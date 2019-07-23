









































#include "nsCacheSession.h"
#include "nsCacheService.h"
#include "nsCRT.h"

NS_IMPL_ADDREF(nsCacheSession)
NS_IMPL_RELEASE(nsCacheSession)

NS_INTERFACE_MAP_BEGIN(nsCacheSession)
    NS_INTERFACE_MAP_ENTRY(nsICacheSession)
    NS_INTERFACE_MAP_ENTRY_CONDITIONAL(
        nsIOfflineCacheSession, (StoragePolicy() == nsICache::STORE_OFFLINE))
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsICacheSession)
NS_INTERFACE_MAP_END

nsCacheSession::nsCacheSession(const char *         clientID,
                               nsCacheStoragePolicy storagePolicy,
                               PRBool               streamBased)
    : mClientID(clientID),
      mInfo(0)
{
  SetStoragePolicy(storagePolicy);

  if (streamBased) MarkStreamBased();
  else SetStoragePolicy(nsICache::STORE_IN_MEMORY);

  MarkDoomEntriesIfExpired();
}

nsCacheSession::~nsCacheSession()
{
  
    
}


NS_IMETHODIMP nsCacheSession::GetDoomEntriesIfExpired(PRBool *result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = WillDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP nsCacheSession::SetDoomEntriesIfExpired(PRBool doomEntriesIfExpired)
{
    if (doomEntriesIfExpired)  MarkDoomEntriesIfExpired();
    else                       ClearDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheSession::OpenCacheEntry(const nsACString &         key, 
                               nsCacheAccessMode          accessRequested,
                               PRBool                     blockingMode,
                               nsICacheEntryDescriptor ** result)
{
    nsresult rv;
    rv =  nsCacheService::OpenCacheEntry(this,
                                         key,
                                         accessRequested,
                                         blockingMode,
                                         nsnull, 
                                         result);
    return rv;
}


NS_IMETHODIMP nsCacheSession::AsyncOpenCacheEntry(const nsACString & key,
                                                  nsCacheAccessMode accessRequested,
                                                  nsICacheListener *listener)
{
    nsresult rv;
    rv = nsCacheService::OpenCacheEntry(this,
                                        key,
                                        accessRequested,
                                        nsICache::BLOCKING,
                                        listener,
                                        nsnull); 

    if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) rv = NS_OK;
    return rv;
}

NS_IMETHODIMP nsCacheSession::EvictEntries()
{
    return nsCacheService::EvictEntriesForSession(this);
}


NS_IMETHODIMP nsCacheSession::IsStorageEnabled(PRBool *result)
{

    return nsCacheService::IsStorageEnabledForPolicy(StoragePolicy(), result);
}

NS_IMETHODIMP nsCacheSession::GetOwnerDomains(PRUint32 * count,
                                              char *** domains)
{
    return nsCacheService::GetOfflineOwnerDomains(this, count, domains);
}

NS_IMETHODIMP nsCacheSession::GetOwnerURIs(const nsACString & domain,
                                           PRUint32 * count,
                                           char *** uris)
{
    return nsCacheService::GetOfflineOwnerURIs(this, domain, count, uris);
}

NS_IMETHODIMP nsCacheSession::SetOwnedKeys(const nsACString & domain,
                                           const nsACString & uri,
                                           PRUint32 count,
                                           const char ** keys)
{
    return nsCacheService::SetOfflineOwnedKeys(this, domain, uri, count, keys);
}

NS_IMETHODIMP nsCacheSession::GetOwnedKeys(const nsACString & domain,
                                           const nsACString & uri,
                                           PRUint32 * count,
                                           char *** keys)
{
    return nsCacheService::GetOfflineOwnedKeys(this, domain, uri, count, keys);
}

NS_IMETHODIMP nsCacheSession::AddOwnedKey(const nsACString & domain,
                                          const nsACString & uri,
                                          const nsACString & key)
{
    return nsCacheService::AddOfflineOwnedKey(this, domain, uri, key);
}

NS_IMETHODIMP nsCacheSession::RemoveOwnedKey(const nsACString & domain,
                                             const nsACString & uri,
                                             const nsACString & key)
{
    return nsCacheService::RemoveOfflineOwnedKey(this, domain, uri, key);
}

NS_IMETHODIMP nsCacheSession::KeyIsOwned(const nsACString & domain,
                                         const nsACString & uri,
                                         const nsACString & key,
                                         PRBool * isOwned)
{
    return nsCacheService::OfflineKeyIsOwned(this, domain, uri, key, isOwned);
}

NS_IMETHODIMP nsCacheSession::ClearKeysOwnedByDomain(const nsACString & domain)
{
    return nsCacheService::ClearOfflineKeysOwnedByDomain(this, domain);
}

NS_IMETHODIMP nsCacheSession::EvictUnownedEntries()
{
    return nsCacheService::EvictUnownedOfflineEntries(this);
}

