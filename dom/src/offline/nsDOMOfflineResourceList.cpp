





































#include "nsDOMOfflineResourceList.h"
#include "nsDOMClassInfo.h"
#include "nsDOMError.h"
#include "nsDOMLists.h"
#include "nsIPrefetchService.h"
#include "nsCPrefetchService.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsICacheSession.h"
#include "nsICacheService.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsIDOMLoadStatus.h"
#include "nsAutoPtr.h"
#include "nsContentUtils.h"
#include "nsIJSContextStack.h"
#include "nsEventDispatcher.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIObserverService.h"
#include "nsIScriptGlobalObject.h"
#include "nsIWebNavigation.h"



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





NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMOfflineResourceList)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMOfflineResourceList,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCacheUpdate)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCheckingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnNoUpdateListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnDownloadingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCachedListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnUpdateReadyListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnObsoleteListener)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mPendingEvents);

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMOfflineResourceList,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCacheUpdate)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCheckingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnNoUpdateListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnDownloadingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCachedListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnUpdateReadyListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnObsoleteListener)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mPendingEvents)

NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIOfflineCacheUpdateObserver)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(OfflineResourceList)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsDOMOfflineResourceList, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsDOMOfflineResourceList, nsDOMEventTargetHelper)

nsDOMOfflineResourceList::nsDOMOfflineResourceList(nsIURI *aManifestURI,
                                                   nsIURI *aDocumentURI,
                                                   nsPIDOMWindow *aWindow,
                                                   nsIScriptContext* aScriptContext)
  : mInitialized(PR_FALSE)
  , mManifestURI(aManifestURI)
  , mDocumentURI(aDocumentURI)
  , mCachedKeys(nsnull)
  , mCachedKeysCount(0)
{
  mOwner = aWindow;
  mScriptContext = aScriptContext;
}

nsDOMOfflineResourceList::~nsDOMOfflineResourceList()
{
  ClearCachedKeys();
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
                   CheckSameOriginURI(mManifestURI, mDocumentURI, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(mDocumentURI);
  if (!innerURI)
    return NS_ERROR_FAILURE;

  mApplicationCacheService =
    do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIOfflineCacheUpdateService> cacheUpdateService =
    do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 numUpdates;
  rv = cacheUpdateService->GetNumUpdates(&numUpdates);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < numUpdates; i++) {
    nsCOMPtr<nsIOfflineCacheUpdate> cacheUpdate;
    rv = cacheUpdateService->GetUpdate(i, getter_AddRefs(cacheUpdate));
    NS_ENSURE_SUCCESS(rv, rv);

    UpdateAdded(cacheUpdate);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  nsCOMPtr<nsIObserverService> observerServ =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerServ->AddObserver(this, "offline-cache-update-added", PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = observerServ->AddObserver(this, "offline-cache-update-completed", PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

void
nsDOMOfflineResourceList::Disconnect()
{
  mOnCheckingListener = nsnull;
  mOnErrorListener = nsnull;
  mOnNoUpdateListener = nsnull;
  mOnDownloadingListener = nsnull;
  mOnProgressListener = nsnull;
  mOnCachedListener = nsnull;
  mOnUpdateReadyListener = nsnull;
  mOnObsoleteListener = nsnull;

  mPendingEvents.Clear();

  if (mListenerManager) {
    mListenerManager->Disconnect();
    mListenerManager = nsnull;
  }
}





NS_IMETHODIMP
nsDOMOfflineResourceList::GetMozItems(nsIDOMDOMStringList **aItems)
{
  *aItems = nsnull;

  nsRefPtr<nsDOMStringList> items = new nsDOMStringList();
  NS_ENSURE_TRUE(items, NS_ERROR_OUT_OF_MEMORY);

  
  
  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    NS_ADDREF(*aItems = items);
    return NS_OK;
  }

  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  char **keys;
  rv = appCache->GatherEntries(nsIApplicationCache::ITEM_DYNAMIC,
                               &length, &keys);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    items->Add(NS_ConvertUTF8toUTF16(keys[i]));
  }

  NS_FREE_XPCOM_ALLOCATED_POINTER_ARRAY(length, keys);

  NS_ADDREF(*aItems = items);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::MozHasItem(const nsAString& aURI, PRBool* aExists)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsCAutoString key;
  rv = GetCacheKey(aURI, key);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 types;
  rv = appCache->GetTypes(key, &types);
  if (rv == NS_ERROR_CACHE_KEY_NOT_FOUND) {
    *aExists = PR_FALSE;
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *aExists = ((types & nsIApplicationCache::ITEM_DYNAMIC) != 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetMozLength(PRUint32 *aLength)
{
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
nsDOMOfflineResourceList::MozItem(PRUint32 aIndex, nsAString& aURI)
{
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

  nsCAutoString scheme;
  rv = requestedURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool match;
  rv = mManifestURI->SchemeIs(scheme.get(), &match);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!match) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  PRUint32 length;
  rv = GetMozLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 maxEntries = nsContentUtils::GetIntPref(kMaxEntriesPref,
                                                   DEFAULT_MAX_ENTRIES);

  if (length > maxEntries) return NS_ERROR_NOT_AVAILABLE;

  ClearCachedKeys();

  nsCOMPtr<nsIOfflineCacheUpdate> update =
    do_CreateInstance(NS_OFFLINECACHEUPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString clientID;
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
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  if (!nsContentUtils::OfflineAppAllowed(mDocumentURI)) {
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsCAutoString key;
  rv = GetCacheKey(aURI, key);
  NS_ENSURE_SUCCESS(rv, rv);

  ClearCachedKeys();

  
  
  
  
  

  rv = appCache->UnmarkEntry(key, nsIApplicationCache::ITEM_DYNAMIC);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetStatus(PRUint16 *aStatus)
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


  
  if (mCacheUpdate) {
    rv = mCacheUpdate->GetStatus(aStatus);
    if (NS_SUCCEEDED(rv) && *aStatus != nsIDOMOfflineResourceList::IDLE) {
      return NS_OK;
    }
  }

  nsCOMPtr<nsIApplicationCache> activeCache;
  rv = mApplicationCacheService->GetActiveCache(mManifestSpec,
                                                getter_AddRefs(activeCache));
  NS_ENSURE_SUCCESS(rv, rv);

  if (activeCache == nsnull) {
    *aStatus = nsIDOMOfflineResourceList::OBSOLETE;
  } else if (appCache == activeCache) {
    *aStatus = nsIDOMOfflineResourceList::IDLE;
  } else {
    *aStatus = nsIDOMOfflineResourceList::UPDATEREADY;
  }

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

  nsCOMPtr<nsIOfflineCacheUpdate> update;
  rv = updateService->ScheduleUpdate(mManifestURI, mDocumentURI,
                                     getter_AddRefs(update));
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

  nsCOMPtr<nsIApplicationCacheService> serv =
    do_GetService(NS_APPLICATIONCACHESERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIApplicationCache> currentAppCache = GetDocumentAppCache();

  nsCOMPtr<nsIApplicationCache> newAppCache;
  rv = serv->GetActiveCache(mManifestSpec, getter_AddRefs(newAppCache));
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  if (newAppCache == currentAppCache) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  ClearCachedKeys();

  nsCOMPtr<nsIApplicationCacheContainer> appCacheContainer =
    GetDocumentAppCacheContainer();

  if (appCacheContainer) {
    rv = appCacheContainer->SetApplicationCache(newAppCache);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}





NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnchecking(nsIDOMEventListener **aOnchecking)
{
  return GetInnerEventListener(mOnCheckingListener, aOnchecking);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnchecking(nsIDOMEventListener *aOnchecking)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(CHECKING_STR),
                                mOnCheckingListener, aOnchecking);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnerror(nsIDOMEventListener **aOnerror)
{
  return GetInnerEventListener(mOnErrorListener, aOnerror);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnerror(nsIDOMEventListener *aOnerror)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(ERROR_STR), mOnErrorListener,
                                aOnerror);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnnoupdate(nsIDOMEventListener **aOnnoupdate)
{
  return GetInnerEventListener(mOnNoUpdateListener, aOnnoupdate);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnnoupdate(nsIDOMEventListener *aOnnoupdate)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(NOUPDATE_STR),
                                mOnNoUpdateListener, aOnnoupdate);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOndownloading(nsIDOMEventListener **aOndownloading)
{
  return GetInnerEventListener(mOnDownloadingListener, aOndownloading);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOndownloading(nsIDOMEventListener *aOndownloading)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(DOWNLOADING_STR),
                                mOnDownloadingListener, aOndownloading);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnprogress(nsIDOMEventListener **aOnprogress)
{
  return GetInnerEventListener(mOnProgressListener, aOnprogress);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnprogress(nsIDOMEventListener *aOnprogress)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(PROGRESS_STR),
                                mOnProgressListener, aOnprogress);
}


NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnupdateready(nsIDOMEventListener **aOnupdateready)
{
  return GetInnerEventListener(mOnUpdateReadyListener, aOnupdateready);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOncached(nsIDOMEventListener *aOncached)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(CACHED_STR),
                                mOnCachedListener, aOncached);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOncached(nsIDOMEventListener **aOncached)
{
  return GetInnerEventListener(mOnCachedListener, aOncached);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnupdateready(nsIDOMEventListener *aOnupdateready)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(UPDATEREADY_STR),
                                mOnUpdateReadyListener, aOnupdateready);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnobsolete(nsIDOMEventListener **aOnobsolete)
{
  return GetInnerEventListener(mOnObsoleteListener, aOnobsolete);
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnobsolete(nsIDOMEventListener *aOnobsolete)
{
  return RemoveAddEventListener(NS_LITERAL_STRING(OBSOLETE_STR),
                                mOnObsoleteListener, aOnobsolete);
}

void
nsDOMOfflineResourceList::FirePendingEvents()
{
  for (PRInt32 i = 0; i < mPendingEvents.Count(); ++i) {
    PRBool dummy;
    nsCOMPtr<nsIDOMEvent> event = mPendingEvents[i];
    DispatchEvent(event, &dummy);
  }
  mPendingEvents.Clear();
}

nsresult
nsDOMOfflineResourceList::SendEvent(const nsAString &aEventName)
{
  
  if (!mOwner) {
    return NS_OK;
  }

  if (!mOwner->GetDocShell()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = nsEventDispatcher::CreateEvent(nsnull, nsnull,
                                               NS_LITERAL_STRING("Events"),
                                               getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrivateDOMEvent> privevent = do_QueryInterface(event);
  if (!privevent) {
    return NS_ERROR_FAILURE;
  }

  event->InitEvent(aEventName, PR_FALSE, PR_TRUE);

  
  privevent->SetTrusted(PR_TRUE);

  
  
  if (mOwner->IsFrozen() || mPendingEvents.Count() > 0) {
    mPendingEvents.AppendObject(event);
    return NS_OK;
  }

  PRBool dummy;
  DispatchEvent(event, &dummy);

  return NS_OK;
}





NS_IMETHODIMP
nsDOMOfflineResourceList::Observe(nsISupports *aSubject,
                                    const char *aTopic,
                                    const PRUnichar *aData)
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
nsDOMOfflineResourceList::Error(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(ERROR_STR));

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Checking(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(CHECKING_STR));
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::NoUpdate(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(NOUPDATE_STR));
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Downloading(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(DOWNLOADING_STR));
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::ItemStarted(nsIOfflineCacheUpdate *aUpdate,
                                      nsIDOMLoadStatus *aItem)
{
  SendEvent(NS_LITERAL_STRING(PROGRESS_STR));
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::ItemCompleted(nsIOfflineCacheUpdate *aUpdate,
                                        nsIDOMLoadStatus *aItem)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Obsolete(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(OBSOLETE_STR));
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
  
  PRBool partial;
  nsresult rv = aUpdate->GetPartial(&partial);
  NS_ENSURE_SUCCESS(rv, rv);

  if (partial) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> updateURI;
  rv = aUpdate->GetManifestURI(getter_AddRefs(updateURI));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool equals;
  rv = updateURI->Equals(mManifestURI, &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!equals) {
    
    return NS_OK;
  }

  NS_ENSURE_TRUE(!mCacheUpdate, NS_ERROR_FAILURE);

  
  
  
  

  mCacheUpdate = aUpdate;
  mCacheUpdate->AddObserver(this, PR_TRUE);

  return NS_OK;
}

already_AddRefed<nsIApplicationCacheContainer>
nsDOMOfflineResourceList::GetDocumentAppCacheContainer()
{
  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(mOwner);
  if (!webnav) {
    return nsnull;
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

  return nsnull;
}

nsresult
nsDOMOfflineResourceList::UpdateCompleted(nsIOfflineCacheUpdate *aUpdate)
{
  if (aUpdate != mCacheUpdate) {
    
    return NS_OK;
  }

  PRBool partial;
  mCacheUpdate->GetPartial(&partial);
  PRBool isUpgrade;
  mCacheUpdate->GetIsUpgrade(&isUpgrade);

  PRBool succeeded;
  nsresult rv = mCacheUpdate->GetSucceeded(&succeeded);

  mCacheUpdate->RemoveObserver(this);
  mCacheUpdate = nsnull;

  if (NS_SUCCEEDED(rv) && succeeded && !partial) {
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

  
  nsCAutoString::const_iterator specStart, specEnd;
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
  if (mCachedKeys)
    return NS_OK;

  nsCOMPtr<nsIApplicationCache> appCache;
  mApplicationCacheService->GetActiveCache(mManifestSpec,
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
    mCachedKeys = nsnull;
    mCachedKeysCount = 0;
  }
}



