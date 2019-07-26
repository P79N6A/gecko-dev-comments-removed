



#include "CacheLog.h"
#include "AppCacheStorage.h"
#include "CacheStorageService.h"

#include "OldWrappers.h"

#include "nsICacheEntryDoomCallback.h"

#include "nsICacheService.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheService.h"
#include "nsIURI.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS_INHERITED0(AppCacheStorage, CacheStorage)

AppCacheStorage::AppCacheStorage(nsILoadContextInfo* aInfo,
                                 nsIApplicationCache* aAppCache)
: CacheStorage(aInfo, true , false )
, mAppCache(aAppCache)
{
  MOZ_COUNT_CTOR(AppCacheStorage);
}

AppCacheStorage::~AppCacheStorage()
{
  ProxyReleaseMainThread(mAppCache);
  MOZ_COUNT_DTOR(AppCacheStorage);
}

NS_IMETHODIMP AppCacheStorage::AsyncOpenURI(nsIURI *aURI,
                                            const nsACString & aIdExtension,
                                            uint32_t aFlags,
                                            nsICacheEntryOpenCallback *aCallback)
{
  if (!CacheStorageService::Self())
    return NS_ERROR_NOT_INITIALIZED;

  NS_ENSURE_ARG(aURI);
  NS_ENSURE_ARG(aCallback);

  nsresult rv;

  nsCOMPtr<nsIApplicationCache> appCache = mAppCache;

  if (!appCache) {
    rv = ChooseApplicationCache(aURI, getter_AddRefs(appCache));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!appCache) {
    LOG(("AppCacheStorage::AsyncOpenURI entry not found in any appcache, giving up"));
    aCallback->OnCacheEntryAvailable(nullptr, false, nullptr, NS_ERROR_CACHE_KEY_NOT_FOUND);
    return NS_OK;
  }

  nsCOMPtr<nsIURI> noRefURI;
  rv = aURI->CloneIgnoringRef(getter_AddRefs(noRefURI));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString cacheKey;
  rv = noRefURI->GetAsciiSpec(cacheKey);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<_OldCacheLoad> appCacheLoad =
    new _OldCacheLoad(cacheKey, aCallback, appCache,
                      LoadInfo(), WriteToDisk(), aFlags);
  rv = appCacheLoad->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP AppCacheStorage::AsyncDoomURI(nsIURI *aURI, const nsACString & aIdExtension,
                                            nsICacheEntryDoomCallback* aCallback)
{
  if (!CacheStorageService::Self())
    return NS_ERROR_NOT_INITIALIZED;

  if (!mAppCache) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP AppCacheStorage::AsyncEvictStorage(nsICacheEntryDoomCallback* aCallback)
{
  if (!CacheStorageService::Self())
    return NS_ERROR_NOT_INITIALIZED;

  nsresult rv;

  nsCOMPtr<nsIApplicationCacheService> appCacheService =
    do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mAppCache) {
    if (LoadInfo()->AppId() == nsILoadContextInfo::NO_APP_ID &&
        !LoadInfo()->IsInBrowserElement()) {

      
      nsCOMPtr<nsICacheService> serv =
          do_GetService(NS_CACHESERVICE_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = serv->EvictEntries(nsICache::STORE_OFFLINE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      
      rv = appCacheService->DiscardByAppId(LoadInfo()->AppId(),
                                           LoadInfo()->IsInBrowserElement());
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    
    nsAutoCString groupID;
    rv = mAppCache->GetGroupID(groupID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = appCacheService->DeactivateGroup(groupID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aCallback)
    aCallback->OnCacheEntryDoomed(NS_OK);

  return NS_OK;
}

NS_IMETHODIMP AppCacheStorage::AsyncVisitStorage(nsICacheStorageVisitor* aVisitor,
                                                 bool aVisitEntries)
{
  if (!CacheStorageService::Self())
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("AppCacheStorage::AsyncVisitStorage [this=%p, cb=%p]", this, aVisitor));

  return NS_ERROR_NOT_IMPLEMENTED;
}

} 
} 
