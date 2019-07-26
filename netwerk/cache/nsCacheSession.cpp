





#include "nsCacheSession.h"
#include "nsCacheService.h"
#include "nsCRT.h"

NS_IMPL_ISUPPORTS1(nsCacheSession, nsICacheSession)

nsCacheSession::nsCacheSession(const char *         clientID,
                               nsCacheStoragePolicy storagePolicy,
                               bool                 streamBased)
    : mClientID(clientID),
      mInfo(0)
{
  SetStoragePolicy(storagePolicy);

  if (streamBased) MarkStreamBased();
  else SetStoragePolicy(nsICache::STORE_IN_MEMORY);

  MarkPublic();

  MarkDoomEntriesIfExpired();
}

nsCacheSession::~nsCacheSession()
{
  
    
}


NS_IMETHODIMP nsCacheSession::GetDoomEntriesIfExpired(bool *result)
{
    NS_ENSURE_ARG_POINTER(result);
    *result = WillDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP nsCacheSession::SetProfileDirectory(nsILocalFile *profileDir)
{
  if (StoragePolicy() != nsICache::STORE_OFFLINE && profileDir) {
        
        
        
        
        return NS_ERROR_UNEXPECTED;
    }

    mProfileDir = profileDir;
    return NS_OK;
}

NS_IMETHODIMP nsCacheSession::GetProfileDirectory(nsILocalFile **profileDir)
{
    if (mProfileDir)
        NS_ADDREF(*profileDir = mProfileDir);
    else
        *profileDir = nsnull;

    return NS_OK;
}


NS_IMETHODIMP nsCacheSession::SetDoomEntriesIfExpired(bool doomEntriesIfExpired)
{
    if (doomEntriesIfExpired)  MarkDoomEntriesIfExpired();
    else                       ClearDoomEntriesIfExpired();
    return NS_OK;
}


NS_IMETHODIMP
nsCacheSession::OpenCacheEntry(const nsACString &         key, 
                               nsCacheAccessMode          accessRequested,
                               bool                       blockingMode,
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
                                                  nsICacheListener *listener,
                                                  bool              noWait)
{
    nsresult rv;
    rv = nsCacheService::OpenCacheEntry(this,
                                        key,
                                        accessRequested,
                                        !noWait,
                                        listener,
                                        nsnull); 

    if (rv == NS_ERROR_CACHE_WAIT_FOR_VALIDATION) rv = NS_OK;
    return rv;
}

NS_IMETHODIMP nsCacheSession::EvictEntries()
{
    return nsCacheService::EvictEntriesForSession(this);
}


NS_IMETHODIMP nsCacheSession::IsStorageEnabled(bool *result)
{

    return nsCacheService::IsStorageEnabledForPolicy(StoragePolicy(), result);
}

NS_IMETHODIMP nsCacheSession::DoomEntry(const nsACString &key,
                                        nsICacheListener *listener)
{
    return nsCacheService::DoomEntry(this, key, listener);
}

NS_IMETHODIMP nsCacheSession::GetIsPrivate(bool* aPrivate)
{
    *aPrivate = IsPrivate();
    return NS_OK;
}

NS_IMETHODIMP nsCacheSession::SetIsPrivate(bool aPrivate)
{
    if (aPrivate)
        MarkPrivate();
    else
        MarkPublic();
    return NS_OK;
}
