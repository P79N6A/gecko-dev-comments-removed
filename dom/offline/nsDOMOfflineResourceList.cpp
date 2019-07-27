




#include "nsDOMOfflineResourceList.h"
#include "nsIDOMEvent.h"
#include "nsIScriptSecurityManager.h"
#include "nsError.h"
#include "mozilla/dom/DOMStringList.h"
#include "nsIPrefetchService.h"
#include "nsCPrefetchService.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsIObserverService.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebNavigation.h"
#include "mozilla/dom/OfflineResourceListBinding.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/Preferences.h"

#include "nsXULAppAPI.h"
#define IS_CHILD_PROCESS() \
    (GeckoProcessType_Default != XRE_GetProcessType())

using namespace mozilla;
using namespace mozilla::dom;



#define CHECKING_STR    "checking"
#define ERROR_STR       "error"
#define NOUPDATE_STR    "noupdate"
#define DOWNLOADING_STR "downloading"
#define PROGRESS_STR    "progress"
#define CACHED_STR      "cached"
#define UPDATEREADY_STR "updateready"
#define OBSOLETE_STR    "obsolete"




static const char kMaxEntriesPref[] =  "offline.max_site_resources";
#define DEFAULT_MAX_ENTRIES 100
#define MAX_URI_LENGTH 2048





NS_IMPL_CYCLE_COLLECTION_INHERITED(nsDOMOfflineResourceList,
                                   DOMEventTargetHelper,
                                   mCacheUpdate,
                                   mPendingEvents)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIOfflineCacheUpdateObserver)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsDOMOfflineResourceList, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsDOMOfflineResourceList, DOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, checking)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, error)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, noupdate)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, downloading)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, progress)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, cached)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, updateready)
NS_IMPL_EVENT_HANDLER(nsDOMOfflineResourceList, obsolete)

nsDOMOfflineResourceList::nsDOMOfflineResourceList(nsIURI *aManifestURI,
                                                   nsIURI *aDocumentURI,
                                                   nsPIDOMWindow *aWindow)
  : DOMEventTargetHelper(aWindow)
  , mInitialized(false)
  , mManifestURI(aManifestURI)
  , mDocumentURI(aDocumentURI)
  , mExposeCacheUpdateStatus(true)
  , mStatus(nsIDOMOfflineResourceList::IDLE)
  , mCachedKeys(nullptr)
  , mCachedKeysCount(0)
{
}

nsDOMOfflineResourceList::~nsDOMOfflineResourceList()
{
  ClearCachedKeys();
}

JSObject*
nsDOMOfflineResourceList::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OfflineResourceListBinding::Wrap(aCx, this, aGivenProto);
}

nsresult
nsDOMOfflineResourceList::Init()
{
  if (mInitialized) {
    return NS_OK;
  }

  if (!mManifestURI) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  mManifestURI->GetAsciiSpec(mManifestSpec);

  nsresult rv = nsContentUtils::GetSecurityManager()->
                   CheckSameOriginURI(mManifestURI, mDocumentURI, true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(mDocumentURI);
  if (!innerURI)
    return NS_ERROR_FAILURE;

  if (!IS_CHILD_PROCESS())
  {
    mApplicationCacheService =
      do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIOfflineCacheUpdateService> cacheUpdateService =
      do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t numUpdates;
    rv = cacheUpdateService->GetNumUpdates(&numUpdates);
    NS_ENSURE_SUCCESS(rv, rv);

    for (uint32_t i = 0; i < numUpdates; i++) {
      nsCOMPtr<nsIOfflineCacheUpdate> cacheUpdate;
      rv = cacheUpdateService->GetUpdate(i, getter_AddRefs(cacheUpdate));
      NS_ENSURE_SUCCESS(rv, rv);

      UpdateAdded(cacheUpdate);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();
  if (!observerService)
    return NS_ERROR_FAILURE;

  rv = observerService->AddObserver(this, "offline-cache-update-added", true);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = observerService->AddObserver(this, "offline-cache-update-completed", true);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = true;

  return NS_OK;
}

void
nsDOMOfflineResourceList::Disconnect()
{
  mPendingEvents.Clear();

  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nullptr;
  }
}





already_AddRefed<DOMStringList>
nsDOMOfflineResourceList::GetMozItems(ErrorResult& aRv)
{
  if (IS_CHILD_PROCESS()) {
    aRv.Throw(NS_ERROR_NOT_IMPLEMENTED);
    return nullptr;
  }

  nsRefPtr<DOMStringList> items = new DOMStringList();

  
  
  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return items.forget();
  }

  aRv = Init();
  if (aRv.Failed()) {
    return nullptr;
  }

  uint32_t length;
  char **keys;
  aRv = appCache->GatherEntries(nsIApplicationCache::ITEM_DYNAMIC,
                                &length, &keys);
  if (aRv.Failed()) {
    return nullptr;
  }

  for (uint32_t i = 0; i < length; i++) {
    items->Add(NS_ConvertUTF8toUTF16(keys[i]));
  }

  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(length, keys);

  return items.forget();
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetMozItems(nsISupports** aItems)
{
  ErrorResult rv;
  nsRefPtr<DOMStringList> items = GetMozItems(rv);
  items.forget(aItems);
  return rv.StealNSResult();
}

NS_IMETHODIMP
nsDOMOfflineResourceList::MozHasItem(const nsAString& aURI, bool* aExists)
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsAutoCString key;
  rv = GetCacheKey(aURI, key);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t types;
  rv = appCache->GetTypes(key, &types);
  if (rv == NS_ERROR_CACHE_KEY_NOT_FOUND) {
    *aExists = false;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *aExists = ((types & nsIApplicationCache::ITEM_DYNAMIC) != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetMozLength(uint32_t *aLength)
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  if (!mManifestURI) {
    *aLength = 0;
    return NS_OK;
  }

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CacheKeys();
  NS_ENSURE_SUCCESS(rv, rv);

  *aLength = mCachedKeysCount;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::MozItem(uint32_t aIndex, nsAString& aURI)
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  SetDOMStringToNull(aURI);

  rv = CacheKeys();
  NS_ENSURE_SUCCESS(rv, rv);

  if (aIndex >= mCachedKeysCount)
    return NS_ERROR_NOT_AVAILABLE;

  CopyUTF8toUTF16(mCachedKeys[aIndex], aURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::MozAdd(const nsAString& aURI)
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (aURI.Length() > MAX_URI_LENGTH) return NS_ERROR_DOM_BAD_URI;

  
  nsCOMPtr<nsIURI> requestedURI;
  rv = NS_NewURI(getter_AddRefs(requestedURI), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString scheme;
  rv = requestedURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  bool match;
  rv = mManifestURI->SchemeIs(scheme.get(), &match);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!match) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  uint32_t length;
  rv = GetMozLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  uint32_t maxEntries =
    Preferences::GetUint(kMaxEntriesPref, DEFAULT_MAX_ENTRIES);

  if (length > maxEntries) return NS_ERROR_NOT_AVAILABLE;

  ClearCachedKeys();

  nsCOMPtr<nsIOfflineCacheUpdate> update =
    do_CreateInstance(NS_OFFLINECACHEUPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString clientID;
  rv = appCache->GetClientID(clientID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->InitPartial(mManifestURI, clientID, mDocumentURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddDynamicURI(requestedURI);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->Schedule();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::MozRemove(const nsAString& aURI)
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsAutoCString key;
  rv = GetCacheKey(aURI, key);
  NS_ENSURE_SUCCESS(rv, rv);

  ClearCachedKeys();

  
  
  
  
  

  rv = appCache->UnmarkEntry(key, nsIApplicationCache::ITEM_DYNAMIC);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetStatus(uint16_t *aStatus)
{
  nsresult rv = Init();

  
  
  
  if (rv == NS_ERROR_DOM_INVALID_STATE_ERR ||
      !nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    *aStatus = nsIDOMOfflineResourceList::UNCACHED;
    return NS_OK;
  }

  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    *aStatus = nsIDOMOfflineResourceList::UNCACHED;
    return NS_OK;
  }


  
  if (mCacheUpdate && mExposeCacheUpdateStatus) {
    rv = mCacheUpdate->GetStatus(aStatus);
    if (NS_SUCCEEDED(rv) && *aStatus != nsIDOMOfflineResourceList::IDLE) {
      return NS_OK;
    }
  }

  if (mAvailableApplicationCache) {
    *aStatus = nsIDOMOfflineResourceList::UPDATEREADY;
    return NS_OK;
  }

  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Update()
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIOfflineCacheUpdateService> updateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMWindow> window = 
    do_QueryInterface(GetOwner());

  nsCOMPtr<nsIOfflineCacheUpdate> update;
  rv = updateService->ScheduleUpdate(mManifestURI, mDocumentURI,
                                     window, getter_AddRefs(update));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SwapCache()
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIApplicationCache> currentAppCache = GetDocumentAppCache();
  if (!currentAppCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  
  if (mAvailableApplicationCache == currentAppCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (mAvailableApplicationCache) {
    nsCString currClientId, availClientId;
    currentAppCache->GetClientID(currClientId);
    mAvailableApplicationCache->GetClientID(availClientId);
    if (availClientId == currClientId)
      return NS_ERROR_DOM_INVALID_STATE_ERR;
  } else if (mStatus != OBSOLETE) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  ClearCachedKeys();

  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer =
    GetDocumentAppCacheContainer();

  
  
  if (appCacheContainer) {
    rv = appCacheContainer->SetApplicationCache(mAvailableApplicationCache);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mAvailableApplicationCache = nullptr;
  mStatus = nsIDOMOfflineResourceList::IDLE;

  return NS_OK;
}





void
nsDOMOfflineResourceList::FirePendingEvents()
{
  for (int32_t i = 0; i < mPendingEvents.Count(); ++i) {
    bool dummy;
    nsCOMPtr<nsIDOMEvent> event = mPendingEvents[i];
    DispatchEvent(event, &dummy);
  }
  mPendingEvents.Clear();
}

nsresult
nsDOMOfflineResourceList::SendEvent(const nsAString &aEventName)
{
  
  if (!GetOwner()) {
    return NS_OK;
  }

  if (!GetOwner()->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = EventDispatcher::CreateEvent(this, nullptr, nullptr,
                                             NS_LITERAL_STRING("Events"),
                                             getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);
  event->InitEvent(aEventName, false, true);

  
  event->SetTrusted(true);

  
  
  if (GetOwner()->IsFrozen() || mPendingEvents.Count() > 0) {
    mPendingEvents.AppendObject(event);
    return NS_OK;
  }

  bool dummy;
  DispatchEvent(event, &dummy);

  return NS_OK;
}





NS_IMETHODIMP
nsDOMOfflineResourceList::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const char16_t *aData)
{
  if (!strcmp(aTopic, "offline-cache-update-added")) {
    nsCOMPtr<nsIOfflineCacheUpdate> update = do_QueryInterface(aSubject);
    if (update) {
      UpdateAdded(update);
    }
  } else if (!strcmp(aTopic, "offline-cache-update-completed")) {
    nsCOMPtr<nsIOfflineCacheUpdate> update = do_QueryInterface(aSubject);
    if (update) {
      UpdateCompleted(update);
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsDOMOfflineResourceList::UpdateStateChanged(nsIOfflineCacheUpdate *aUpdate,
                                     uint32_t event)
{
  mExposeCacheUpdateStatus = 
      (event == STATE_CHECKING) ||
      (event == STATE_DOWNLOADING) ||
      (event == STATE_ITEMSTARTED) ||
      (event == STATE_ITEMCOMPLETED) ||
      
      (event == STATE_OBSOLETE);

  switch (event) {
    case STATE_ERROR:
      SendEvent(NS_LITERAL_STRING(ERROR_STR));
      break;
    case STATE_CHECKING:
      SendEvent(NS_LITERAL_STRING(CHECKING_STR));
      break;
    case STATE_NOUPDATE:
      SendEvent(NS_LITERAL_STRING(NOUPDATE_STR));
      break;
    case STATE_OBSOLETE:
      mStatus = nsIDOMOfflineResourceList::OBSOLETE;
      mAvailableApplicationCache = nullptr;
      SendEvent(NS_LITERAL_STRING(OBSOLETE_STR));
      break;
    case STATE_DOWNLOADING:
      SendEvent(NS_LITERAL_STRING(DOWNLOADING_STR));
      break;
    case STATE_ITEMSTARTED:
      SendEvent(NS_LITERAL_STRING(PROGRESS_STR));
      break;
    case STATE_ITEMCOMPLETED:
      
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::ApplicationCacheAvailable(nsIApplicationCache *aApplicationCache)
{
  nsCOMPtr<nsIApplicationCache> currentAppCache = GetDocumentAppCache();
  if (currentAppCache) {
    
    

    
    
    if (aApplicationCache == currentAppCache) {
      return NS_OK;
    }

    nsCString currClientId, availClientId;
    currentAppCache->GetClientID(currClientId);
    aApplicationCache->GetClientID(availClientId);
    if (availClientId == currClientId) {
      return NS_OK;
    }

    mAvailableApplicationCache = aApplicationCache;
    return NS_OK;
  }

  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer =
    GetDocumentAppCacheContainer();

  if (appCacheContainer) {
    appCacheContainer->SetApplicationCache(aApplicationCache);
  }

  mAvailableApplicationCache = nullptr;
  return NS_OK;
}

nsresult
nsDOMOfflineResourceList::GetCacheKey(const nsAString &aURI, nsCString &aKey)
{
  nsCOMPtr<nsIURI> requestedURI;
  nsresult rv = NS_NewURI(getter_AddRefs(requestedURI), aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return GetCacheKey(requestedURI, aKey);
}

nsresult
nsDOMOfflineResourceList::UpdateAdded(nsIOfflineCacheUpdate *aUpdate)
{
  
  bool partial;
  nsresult rv = aUpdate->GetPartial(&partial);
  NS_ENSURE_SUCCESS(rv, rv);

  if (partial) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> updateURI;
  rv = aUpdate->GetManifestURI(getter_AddRefs(updateURI));
  NS_ENSURE_SUCCESS(rv, rv);

  bool equals;
  rv = updateURI->Equals(mManifestURI, &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!equals) {
    
    return NS_OK;
  }

  NS_ENSURE_TRUE(!mCacheUpdate, NS_ERROR_FAILURE);

  
  
  
  

  mCacheUpdate = aUpdate;
  mCacheUpdate->AddObserver(this, true);

  return NS_OK;
}

already_AddRefed<nsIApplicationCacheContainer>
nsDOMOfflineResourceList::GetDocumentAppCacheContainer()
{
  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(GetOwner());
  if (!webnav) {
    return nullptr;
  }

  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer =
    do_GetInterface(webnav);
  return appCacheContainer.forget();
}

already_AddRefed<nsIApplicationCache>
nsDOMOfflineResourceList::GetDocumentAppCache()
{
  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer =
    GetDocumentAppCacheContainer();

  if (appCacheContainer) {
    nsCOMPtr<nsIApplicationCache> applicationCache;
    appCacheContainer->GetApplicationCache(
      getter_AddRefs(applicationCache));
    return applicationCache.forget();
  }

  return nullptr;
}

nsresult
nsDOMOfflineResourceList::UpdateCompleted(nsIOfflineCacheUpdate *aUpdate)
{
  if (aUpdate != mCacheUpdate) {
    
    return NS_OK;
  }

  bool partial;
  mCacheUpdate->GetPartial(&partial);
  bool isUpgrade;
  mCacheUpdate->GetIsUpgrade(&isUpgrade);

  bool succeeded;
  nsresult rv = mCacheUpdate->GetSucceeded(&succeeded);

  mCacheUpdate->RemoveObserver(this);
  mCacheUpdate = nullptr;

  if (NS_SUCCEEDED(rv) && succeeded && !partial) {
    mStatus = nsIDOMOfflineResourceList::IDLE;
    if (isUpgrade) {
      SendEvent(NS_LITERAL_STRING(UPDATEREADY_STR));
    } else {
      SendEvent(NS_LITERAL_STRING(CACHED_STR));
    }
  }

  return NS_OK;
}

nsresult
nsDOMOfflineResourceList::GetCacheKey(nsIURI *aURI, nsCString &aKey)
{
  nsresult rv = aURI->GetAsciiSpec(aKey);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsAutoCString::const_iterator specStart, specEnd;
  aKey.BeginReading(specStart);
  aKey.EndReading(specEnd);
  if (FindCharInReadable('#', specStart, specEnd)) {
    aKey.BeginReading(specEnd);
    aKey = Substring(specEnd, specStart);
  }

  return NS_OK;
}

nsresult
nsDOMOfflineResourceList::CacheKeys()
{
  if (IS_CHILD_PROCESS()) 
    return NS_ERROR_NOT_IMPLEMENTED;

  if (mCachedKeys)
    return NS_OK;

  nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(GetOwner());
  nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(window);
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(webNav);

  uint32_t appId = 0;
  bool inBrowser = false;
  if (loadContext) {
    loadContext->GetAppId(&appId);
    loadContext->GetIsInBrowserElement(&inBrowser);
  }

  nsAutoCString groupID;
  mApplicationCacheService->BuildGroupIDForApp(
      mManifestURI, appId, inBrowser, groupID);

  nsCOMPtr<nsIApplicationCache> appCache;
  mApplicationCacheService->GetActiveCache(groupID,
                                           getter_AddRefs(appCache));

  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  return appCache->GatherEntries(nsIApplicationCache::ITEM_DYNAMIC,
                                 &mCachedKeysCount, &mCachedKeys);
}

void
nsDOMOfflineResourceList::ClearCachedKeys()
{
  if (mCachedKeys) {
    NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(mCachedKeysCount, mCachedKeys);
    mCachedKeys = nullptr;
    mCachedKeysCount = 0;
  }
}
