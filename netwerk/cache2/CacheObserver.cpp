



#include "CacheObserver.h"

#include "CacheStorageService.h"
#include "CacheFileIOManager.h"
#include "LoadContextInfo.h"
#include "nsICacheStorage.h"
#include "nsIObserverService.h"
#include "mozIApplicationClearPrivateDataParams.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "nsServiceManagerUtils.h"
#include <time.h>

namespace mozilla {
namespace net {

CacheObserver* CacheObserver::sSelf = nullptr;

static uint32_t const kDefaultMemoryLimit = 50 * 1024; 
uint32_t CacheObserver::sMemoryLimit = kDefaultMemoryLimit;

static uint32_t const kDefaultUseNewCache = 0; 
uint32_t CacheObserver::sUseNewCache = kDefaultUseNewCache;

static int32_t const kDefaultHalfLifeExperiment = -1; 
int32_t CacheObserver::sHalfLifeExperiment = kDefaultHalfLifeExperiment;

static uint32_t const kDefaultHalfLifeHours = 6; 
uint32_t CacheObserver::sHalfLifeHours = kDefaultHalfLifeHours;

NS_IMPL_ISUPPORTS2(CacheObserver,
                   nsIObserver,
                   nsISupportsWeakReference)

nsresult
CacheObserver::Init()
{
  if (sSelf) {
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
  if (!obs) {
    return NS_ERROR_UNEXPECTED;
  }

  sSelf = new CacheObserver();
  NS_ADDREF(sSelf);

  obs->AddObserver(sSelf, "prefservice:after-app-defaults", true);
  obs->AddObserver(sSelf, "profile-do-change", true);
  obs->AddObserver(sSelf, "profile-before-change", true);
  obs->AddObserver(sSelf, "xpcom-shutdown", true);
  obs->AddObserver(sSelf, "last-pb-context-exited", true);
  obs->AddObserver(sSelf, "webapps-clear-data", true);
  obs->AddObserver(sSelf, "memory-pressure", true);

  return NS_OK;
}

nsresult
CacheObserver::Shutdown()
{
  if (!sSelf) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_RELEASE(sSelf);
  return NS_OK;
}

void
CacheObserver::AttachToPreferences()
{
  mozilla::Preferences::AddUintVarCache(
    &sMemoryLimit, "browser.cache.memory_limit", kDefaultMemoryLimit);
  mozilla::Preferences::AddUintVarCache(
    &sUseNewCache, "browser.cache.use_new_backend", kDefaultUseNewCache);

  sHalfLifeExperiment = mozilla::Preferences::GetInt(
    "browser.cache.frecency_experiment", kDefaultHalfLifeExperiment);

  if (sHalfLifeExperiment == 0) {
    
    
    
    srand(time(NULL));
    sHalfLifeExperiment = (rand() % 4) + 1;
    mozilla::Preferences::SetInt(
      "browser.cache.frecency_experiment", sHalfLifeExperiment);
  }

  switch (sHalfLifeExperiment) {
  case 1: 
    sHalfLifeHours = 6;
    break;
  case 2:
    sHalfLifeHours = 24;
    break;
  case 3:
    sHalfLifeHours = 7 * 24;
    break;
  case 4:
    sHalfLifeHours = 50 * 24;
    break;

  case -1:
  default: 
    sHalfLifeExperiment = -1;
    sHalfLifeHours = std::max(1U, std::min(1440U, mozilla::Preferences::GetUint(
      "browser.cache.frecency_half_life_hours", kDefaultHalfLifeHours)));
    break;
  }
}


bool const CacheObserver::UseNewCache()
{
  switch (sUseNewCache) {
    case 0: 
      return false;

    case 1: 
      return true;

    case 2: 
    {
      static bool const sABTest = rand() & 1;
      return sABTest;
    }
  }

  return true;
}

namespace { 

class CacheStorageEvictHelper
{
public:
  nsresult Run(mozIApplicationClearPrivateDataParams* aParams);

private:
  uint32_t mAppId;
  nsresult ClearStorage(bool const aPrivate,
                        bool const aInBrowser,
                        bool const aAnonymous);
};

nsresult
CacheStorageEvictHelper::Run(mozIApplicationClearPrivateDataParams* aParams)
{
  nsresult rv;

  rv = aParams->GetAppId(&mAppId);
  NS_ENSURE_SUCCESS(rv, rv);

  bool aBrowserOnly;
  rv = aParams->GetBrowserOnly(&aBrowserOnly);
  NS_ENSURE_SUCCESS(rv, rv);

  MOZ_ASSERT(mAppId != nsILoadContextInfo::UNKNOWN_APP_ID);

  
  rv = ClearStorage(false, aBrowserOnly, false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ClearStorage(false, aBrowserOnly, true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ClearStorage(true, aBrowserOnly, false);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ClearStorage(true, aBrowserOnly, true);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
CacheStorageEvictHelper::ClearStorage(bool const aPrivate,
                                      bool const aInBrowser,
                                      bool const aAnonymous)
{
  nsresult rv;

  nsRefPtr<LoadContextInfo> info = GetLoadContextInfo(
    aPrivate, mAppId, aInBrowser, aAnonymous);

  nsCOMPtr<nsICacheStorage> storage;
  nsRefPtr<CacheStorageService> service = CacheStorageService::Self();
  NS_ENSURE_TRUE(service, NS_ERROR_FAILURE);

  
  rv = service->DiskCacheStorage(info, false, getter_AddRefs(storage));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storage->AsyncEvictStorage(nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = service->MemoryCacheStorage(info, getter_AddRefs(storage));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = storage->AsyncEvictStorage(nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aInBrowser) {
    rv = ClearStorage(aPrivate, true, aAnonymous);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

} 

NS_IMETHODIMP
CacheObserver::Observe(nsISupports* aSubject,
                       const char* aTopic,
                       const char16_t* aData)
{
  if (!strcmp(aTopic, "prefservice:after-app-defaults")) {
    CacheFileIOManager::Init();
    return NS_OK;
  }

  if (!strcmp(aTopic, "profile-do-change")) {
    AttachToPreferences();
    CacheFileIOManager::Init();
    CacheFileIOManager::OnProfile();
    return NS_OK;
  }

  if (!strcmp(aTopic, "profile-before-change")) {
    nsRefPtr<CacheStorageService> service = CacheStorageService::Self();
    if (service)
      service->Shutdown();

    return NS_OK;
  }

  if (!strcmp(aTopic, "xpcom-shutdown")) {
    nsRefPtr<CacheStorageService> service = CacheStorageService::Self();
    if (service)
      service->Shutdown();

    CacheFileIOManager::Shutdown();
    return NS_OK;
  }

  if (!strcmp(aTopic, "last-pb-context-exited")) {
    nsRefPtr<CacheStorageService> service = CacheStorageService::Self();
    if (service)
      service->DropPrivateBrowsingEntries();

    return NS_OK;
  }

  if (!strcmp(aTopic, "webapps-clear-data")) {
    nsCOMPtr<mozIApplicationClearPrivateDataParams> params =
            do_QueryInterface(aSubject);
    if (!params) {
      NS_ERROR("'webapps-clear-data' notification's subject should be a mozIApplicationClearPrivateDataParams");
      return NS_ERROR_UNEXPECTED;
    }

    CacheStorageEvictHelper helper;
    nsresult rv = helper.Run(params);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  if (!strcmp(aTopic, "memory-pressure")) {
    nsRefPtr<CacheStorageService> service = CacheStorageService::Self();
    if (service)
      service->PurgeFromMemory(nsICacheStorageService::PURGE_EVERYTHING);

    return NS_OK;
  }

  return NS_OK;
}

} 
} 
