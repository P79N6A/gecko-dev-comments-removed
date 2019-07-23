





































#ifndef nsDOMOfflineResourceList_h___
#define nsDOMOfflineResourceList_h___

#include "nscore.h"
#include "nsIDOMOfflineResourceList.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIApplicationCacheService.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"
#include "nsCOMArray.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMEventTarget.h"
#include "nsDOMEvent.h"
#include "nsIObserver.h"
#include "nsIScriptContext.h"
#include "nsCycleCollectionParticipant.h"

class nsIDOMWindow;

class nsDOMOfflineResourceList : public nsIDOMOfflineResourceList,
                                 public nsIObserver,
                                 public nsIOfflineCacheUpdateObserver,
                                 public nsIDOMEventTarget,
                                 public nsSupportsWeakReference
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMOFFLINERESOURCELIST
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER
  NS_DECL_NSIDOMEVENTTARGET

  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(nsDOMOfflineResourceList,
                                           nsIDOMOfflineResourceList)

  nsDOMOfflineResourceList(PRBool aToplevel,
                           nsIURI* aManifestURI,
                           nsIURI* aDocumentURI,
                           nsIDOMWindow* aWindow);
  virtual ~nsDOMOfflineResourceList();

  void FirePendingEvents();
  void Disconnect();

private:
  nsresult Init();

  void NotifyEventListeners(nsIDOMEventListener *aListener,
                            const nsCOMArray<nsIDOMEventListener>& aListeners,
                            nsIDOMEvent* aEvent);

  nsresult SendEvent(const nsAString &aEventName,
                     nsIDOMEventListener *aListener,
                     const nsCOMArray<nsIDOMEventListener> &aListeners);

  nsresult UpdateAdded(nsIOfflineCacheUpdate *aUpdate);
  nsresult UpdateCompleted(nsIOfflineCacheUpdate *aUpdate);

  already_AddRefed<nsIApplicationCacheContainer> GetDocumentAppCacheContainer();
  already_AddRefed<nsIApplicationCache> GetDocumentAppCache();

  nsresult GetCacheKey(const nsAString &aURI, nsCString &aKey);
  nsresult GetCacheKey(nsIURI *aURI, nsCString &aKey);

  nsresult CacheKeys();
  void ClearCachedKeys();

  PRBool mInitialized;
  PRBool mToplevel;

  nsCOMPtr<nsIURI> mManifestURI;
  
  nsCString mManifestSpec;

  nsCOMPtr<nsIURI> mDocumentURI;
  nsCOMPtr<nsIWeakReference> mWindow;
  nsCOMPtr<nsIApplicationCacheService> mApplicationCacheService;
  nsCOMPtr<nsIOfflineCacheUpdate> mCacheUpdate;

  
  char **mCachedKeys;
  PRUint32 mCachedKeysCount;

  nsCOMArray<nsIDOMEventListener> mCheckingListeners;
  nsCOMArray<nsIDOMEventListener> mErrorListeners;
  nsCOMArray<nsIDOMEventListener> mNoUpdateListeners;
  nsCOMArray<nsIDOMEventListener> mDownloadingListeners;
  nsCOMArray<nsIDOMEventListener> mProgressListeners;
  nsCOMArray<nsIDOMEventListener> mCachedListeners;
  nsCOMArray<nsIDOMEventListener> mUpdateReadyListeners;
  nsCOMArray<nsIDOMEventListener> mObsoleteListeners;

  nsCOMPtr<nsIDOMEventListener> mOnCheckingListener;
  nsCOMPtr<nsIDOMEventListener> mOnErrorListener;
  nsCOMPtr<nsIDOMEventListener> mOnNoUpdateListener;
  nsCOMPtr<nsIDOMEventListener> mOnDownloadingListener;
  nsCOMPtr<nsIDOMEventListener> mOnProgressListener;
  nsCOMPtr<nsIDOMEventListener> mOnCachedListener;
  nsCOMPtr<nsIDOMEventListener> mOnUpdateReadyListener;
  nsCOMPtr<nsIDOMEventListener> mOnObsoleteListener;

  struct PendingEvent {
    nsCOMPtr<nsIDOMEvent> event;
    nsCOMPtr<nsIDOMEventListener> listener;
    nsCOMArray<nsIDOMEventListener> listeners;
  };

  nsTArray<PendingEvent> mPendingEvents;
};

#endif
