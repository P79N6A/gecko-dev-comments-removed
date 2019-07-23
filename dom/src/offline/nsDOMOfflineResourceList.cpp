





































#include "nsDOMOfflineResourceList.h"
#include "nsDOMClassInfo.h"
#include "nsDOMError.h"
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

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(nsDOMOfflineResourceList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mCacheUpdate)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mCheckingListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mErrorListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mNoUpdateListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mDownloadingListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mProgressListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mCachedListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mUpdateReadyListeners)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mObsoleteListeners)

  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCheckingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnNoUpdateListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnDownloadingListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCachedListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnUpdateReadyListener)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnObsoleteListener)

  for (PRUint32 i = 0; i < tmp->mPendingEvents.Length(); i++) {
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPendingEvents[i].event);
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mPendingEvents[i].listener);
    NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMARRAY(mPendingEvents[i].listeners);
  }

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(nsDOMOfflineResourceList)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mCacheUpdate)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mCheckingListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mErrorListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mNoUpdateListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mDownloadingListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mProgressListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mCachedListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mUpdateReadyListeners)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mObsoleteListeners)

  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCheckingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnErrorListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnNoUpdateListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnDownloadingListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnProgressListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCachedListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnUpdateReadyListener)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnObsoleteListener)

  for (PRUint32 i = 0; i < tmp->mPendingEvents.Length(); i++) {
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPendingEvents[i].event);
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mPendingEvents[i].listener);
    NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMARRAY(mPendingEvents[i].listeners);
  }

NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(nsDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIDOMOfflineResourceList)
  NS_INTERFACE_MAP_ENTRY(nsIOfflineCacheUpdateObserver)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventTarget)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(OfflineResourceList)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(nsDOMOfflineResourceList)
NS_IMPL_CYCLE_COLLECTING_RELEASE(nsDOMOfflineResourceList)

nsDOMOfflineResourceList::nsDOMOfflineResourceList(PRBool aToplevel,
                                                   nsIURI *aManifestURI,
                                                   nsIURI *aDocumentURI,
                                                   nsIDOMWindow *aWindow)
  : mInitialized(PR_FALSE)
  , mToplevel(aToplevel)
  , mManifestURI(aManifestURI)
  , mDocumentURI(aDocumentURI)
  , mCachedKeys(nsnull)
  , mCachedKeysCount(0)
{
  mWindow = do_GetWeakReference(aWindow);
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
  mCheckingListeners.Clear();
  mErrorListeners.Clear();
  mNoUpdateListeners.Clear();
  mDownloadingListeners.Clear();
  mProgressListeners.Clear();
  mCachedListeners.Clear();
  mUpdateReadyListeners.Clear();
  mObsoleteListeners.Clear();

  mOnCheckingListener = nsnull;
  mOnErrorListener = nsnull;
  mOnNoUpdateListener = nsnull;
  mOnDownloadingListener = nsnull;
  mOnProgressListener = nsnull;
  mOnCachedListener = nsnull;
  mOnUpdateReadyListener = nsnull;
  mOnObsoleteListener = nsnull;

  mPendingEvents.Clear();
}





NS_IMETHODIMP
nsDOMOfflineResourceList::GetLength(PRUint32 *aLength)
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
nsDOMOfflineResourceList::Item(PRUint32 aIndex, nsAString& aURI)
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
nsDOMOfflineResourceList::Add(const nsAString& aURI)
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
  rv = GetLength(&length);
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
nsDOMOfflineResourceList::Remove(const nsAString& aURI)
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

  
  if (mCacheUpdate) {
    rv = mCacheUpdate->GetStatus(aStatus);
    if (NS_SUCCEEDED(rv) && *aStatus != nsIDOMOfflineResourceList::IDLE) {
      return NS_OK;
    }
  }

  
  nsCOMPtr<nsIApplicationCache> appCache = GetDocumentAppCache();
  if (!appCache) {
    *aStatus = nsIDOMOfflineResourceList::UNCACHED;
    return NS_OK;
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

  if (!mToplevel) {
    return NS_ERROR_DOM_INVALID_STATE_ERR;
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
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnchecking);
  NS_IF_ADDREF(*aOnchecking = mOnCheckingListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnchecking(nsIDOMEventListener *aOnchecking)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnCheckingListener = aOnchecking;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnerror(nsIDOMEventListener **aOnerror)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnerror);
  NS_IF_ADDREF(*aOnerror = mOnErrorListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnerror(nsIDOMEventListener *aOnerror)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnErrorListener = aOnerror;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnnoupdate(nsIDOMEventListener **aOnnoupdate)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnnoupdate);
  NS_IF_ADDREF(*aOnnoupdate = mOnNoUpdateListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnnoupdate(nsIDOMEventListener *aOnnoupdate)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnNoUpdateListener = aOnnoupdate;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOndownloading(nsIDOMEventListener **aOndownloading)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOndownloading);
  NS_IF_ADDREF(*aOndownloading = mOnDownloadingListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOndownloading(nsIDOMEventListener *aOndownloading)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnDownloadingListener = aOndownloading;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnprogress(nsIDOMEventListener **aOnprogress)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnprogress);
  NS_IF_ADDREF(*aOnprogress = mOnProgressListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnprogress(nsIDOMEventListener *aOnprogress)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnProgressListener = aOnprogress;
  return NS_OK;
}


NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnupdateready(nsIDOMEventListener **aOnupdateready)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnupdateready);
  NS_IF_ADDREF(*aOnupdateready = mOnUpdateReadyListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOncached(nsIDOMEventListener *aOncached)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnCachedListener = aOncached;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOncached(nsIDOMEventListener **aOncached)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOncached);
  NS_IF_ADDREF(*aOncached = mOnCachedListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnupdateready(nsIDOMEventListener *aOnupdateready)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnUpdateReadyListener = aOnupdateready;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::GetOnobsolete(nsIDOMEventListener **aOnobsolete)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_POINTER(aOnobsolete);
  NS_IF_ADDREF(*aOnobsolete = mOnObsoleteListener);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::SetOnobsolete(nsIDOMEventListener *aOnobsolete)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mOnObsoleteListener = aOnobsolete;
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::AddEventListener(const nsAString& aType,
                                           nsIDOMEventListener *aListener,
                                           PRBool aUseCapture)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG(aListener);

  nsCOMArray<nsIDOMEventListener> *array;

#define IMPL_ADD_LISTENER(_type, _member)    \
  if (aType.EqualsLiteral(_type)) {           \
    array = &(_member);                      \
  } else

  IMPL_ADD_LISTENER(CHECKING_STR, mCheckingListeners)
  IMPL_ADD_LISTENER(ERROR_STR, mErrorListeners)
  IMPL_ADD_LISTENER(NOUPDATE_STR, mNoUpdateListeners)
  IMPL_ADD_LISTENER(DOWNLOADING_STR, mDownloadingListeners)
  IMPL_ADD_LISTENER(PROGRESS_STR, mProgressListeners)
  IMPL_ADD_LISTENER(CACHED_STR, mCachedListeners)
  IMPL_ADD_LISTENER(UPDATEREADY_STR, mUpdateReadyListeners)
  IMPL_ADD_LISTENER(OBSOLETE_STR, mObsoleteListeners)
  {
    return NS_ERROR_INVALID_ARG;
  }

  array->AppendObject(aListener);
#undef IMPL_ADD_LISTENER

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::RemoveEventListener(const nsAString &aType,
                                              nsIDOMEventListener *aListener,
                                              PRBool aUseCapture)
{
  nsresult rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG(aListener);

  nsCOMArray<nsIDOMEventListener> *array;

#define IMPL_REMOVE_LISTENER(_type, _member)  \
  if (aType.EqualsLiteral(_type)) {            \
    array = &(_member);                       \
  } else

  IMPL_REMOVE_LISTENER(CHECKING_STR, mCheckingListeners)
  IMPL_REMOVE_LISTENER(ERROR_STR, mErrorListeners)
  IMPL_REMOVE_LISTENER(NOUPDATE_STR, mNoUpdateListeners)
  IMPL_REMOVE_LISTENER(DOWNLOADING_STR, mDownloadingListeners)
  IMPL_REMOVE_LISTENER(PROGRESS_STR, mProgressListeners)
  IMPL_REMOVE_LISTENER(CACHED_STR, mCachedListeners)
  IMPL_REMOVE_LISTENER(UPDATEREADY_STR, mUpdateReadyListeners)
  IMPL_REMOVE_LISTENER(OBSOLETE_STR, mObsoleteListeners)
  {
    return NS_ERROR_INVALID_ARG;
  }

  
  for (PRUint32 i = array->Count() - 1; i != PRUint32(-1); --i) {
    if (array->ObjectAt(i) == aListener) {
      array->RemoveObjectAt(i);
      break;
    }
  }

#undef IMPL_REMOVE_LISTENER

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::DispatchEvent(nsIDOMEvent *evt, PRBool *_retval)
{
  

  return NS_OK;
}

void
nsDOMOfflineResourceList::NotifyEventListeners(nsIDOMEventListener *aListener,
                                               const nsCOMArray<nsIDOMEventListener>& aListeners,
                                               nsIDOMEvent* aEvent)
{
  
  
  
  
  if (!aEvent)
    return;

  nsCOMPtr<nsIJSContextStack> stack;
  JSContext *cx = nsnull;

  nsCOMPtr<nsIScriptGlobalObject> scriptGlobal = do_QueryReferent(mWindow);
  if (!scriptGlobal)
    return;

  nsCOMPtr<nsIScriptContext> context = scriptGlobal->GetContext();
  if (context) {
    stack = do_GetService("@mozilla.org/js/xpc/ContextStack;1");

    if (stack) {
      cx = (JSContext *)context->GetNativeContext();

      if (cx) {
        stack->Push(cx);
      }
    }
  }

  if (aListener) {
    aListener->HandleEvent(aEvent);
  }

  PRInt32 count = aListeners.Count();
  for (PRInt32 index = 0; index < count; ++index) {
    nsIDOMEventListener* listener = aListeners[index];

    if (listener) {
      listener->HandleEvent(aEvent);
    }
  }

  if (cx) {
    stack->Pop(&cx);
  }
}

void
nsDOMOfflineResourceList::FirePendingEvents()
{
  for (PRUint32 i = 0; i < mPendingEvents.Length(); i++) {
    const PendingEvent &pending = mPendingEvents[i];
    NotifyEventListeners(pending.listener, pending.listeners, pending.event);
  }
  mPendingEvents.Clear();
}

nsresult
nsDOMOfflineResourceList::SendEvent(const nsAString &aEventName,
                                    nsIDOMEventListener *aListener,
                                    const nsCOMArray<nsIDOMEventListener> &aListeners)
{
  
  if (!mToplevel) {
    return NS_OK;
  }

  if (!aListener && aListeners.Count() == 0) {
    return NS_OK;
  }

  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(mWindow);
  if (!window) {
    return NS_OK;
  }

  if (!window->GetDocShell()) {
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

  privevent->SetTarget(this);
  privevent->SetCurrentTarget(this);
  privevent->SetOriginalTarget(this);

  
  privevent->SetTrusted(PR_TRUE);

  
  
  if (window->IsFrozen() || mPendingEvents.Length() > 0) {
    PendingEvent *pending = mPendingEvents.AppendElement();
    pending->event = event;
    pending->listener = aListener;
    pending->listeners.SetCapacity(aListeners.Count());
    pending->listeners.AppendObjects(aListeners);

    return NS_OK;
  }

  NotifyEventListeners(aListener, aListeners, event);

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
  SendEvent(NS_LITERAL_STRING(ERROR_STR), mOnErrorListener, mErrorListeners);

  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Checking(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(CHECKING_STR),
            mOnCheckingListener, mCheckingListeners);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::NoUpdate(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(NOUPDATE_STR),
            mOnNoUpdateListener, mNoUpdateListeners);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::Downloading(nsIOfflineCacheUpdate *aUpdate)
{
  SendEvent(NS_LITERAL_STRING(DOWNLOADING_STR),
            mOnDownloadingListener, mDownloadingListeners);
  return NS_OK;
}

NS_IMETHODIMP
nsDOMOfflineResourceList::ItemStarted(nsIOfflineCacheUpdate *aUpdate,
                                      nsIDOMLoadStatus *aItem)
{
  SendEvent(NS_LITERAL_STRING(PROGRESS_STR),
            mOnProgressListener, mProgressListeners);
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
  SendEvent(NS_LITERAL_STRING(OBSOLETE_STR),
            mOnObsoleteListener, mObsoleteListeners);
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
  nsCOMPtr<nsIDOMWindow> window = do_QueryReferent(mWindow);
  if (!window) {
    return nsnull;
  }

  nsCOMPtr<nsIWebNavigation> webnav = do_GetInterface(window);
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
      SendEvent(NS_LITERAL_STRING(UPDATEREADY_STR),
                mOnUpdateReadyListener, mUpdateReadyListeners);
    } else {
      SendEvent(NS_LITERAL_STRING(CACHED_STR),
                mOnCachedListener, mCachedListeners);
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



