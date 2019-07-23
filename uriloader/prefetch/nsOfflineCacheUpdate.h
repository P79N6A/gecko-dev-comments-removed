





































#ifndef nsOfflineCacheUpdate_h__
#define nsOfflineCacheUpdate_h__

#include "nsIOfflineCacheUpdate.h"

#include "nsAutoPtr.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsICacheService.h"
#include "nsIChannelEventSink.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNode.h"
#include "nsIDOMLoadStatus.h"
#include "nsIInterfaceRequestor.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIOfflineCacheSession.h"
#include "nsIPrefetchService.h"
#include "nsIRequestObserver.h"
#include "nsIRunnable.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIWebProgressListener.h"
#include "nsRefPtrHashtable.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWeakReference.h"

class nsOfflineCacheUpdate;

class nsOfflineCacheUpdateItem : public nsIDOMLoadStatus
                               , public nsIStreamListener
                               , public nsIRunnable
                               , public nsIInterfaceRequestor
                               , public nsIChannelEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMLOADSTATUS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIRUNNABLE
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICHANNELEVENTSINK

    nsOfflineCacheUpdateItem(nsOfflineCacheUpdate *aUpdate,
                             nsIURI *aURI,
                             nsIURI *aReferrerURI,
                             nsIDOMNode *aSource,
                             const nsACString &aClientID);
    ~nsOfflineCacheUpdateItem();

    nsCOMPtr<nsIURI>           mURI;
    nsCOMPtr<nsIURI>           mReferrerURI;
    nsCOMPtr<nsIWeakReference> mSource;
    nsCString                  mClientID;

    nsresult OpenChannel();
    nsresult Cancel();

private:
    nsOfflineCacheUpdate*          mUpdate;
    nsCOMPtr<nsIChannel>           mChannel;
    PRUint16                       mState;
    PRInt32                        mBytesRead;
};

class nsOfflineCacheUpdate : public nsIOfflineCacheUpdate
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATE

    nsOfflineCacheUpdate();
    ~nsOfflineCacheUpdate();

    nsresult Init();

    nsresult Begin();
    nsresult Cancel();

    void LoadCompleted();
private:
    nsresult ProcessNextURI();
    nsresult AddOwnedItems(const nsACString &aOwnerURI);
    nsresult AddDomainItems();
    nsresult NotifyAdded(nsOfflineCacheUpdateItem *aItem);
    nsresult NotifyCompleted(nsOfflineCacheUpdateItem *aItem);
    nsresult Finish();

    enum {
        STATE_UNINITIALIZED,
        STATE_INITIALIZED,
        STATE_RUNNING,
        STATE_CANCELLED,
        STATE_FINISHED
    } mState;

    PRBool mAddedItems;
    PRBool mPartialUpdate;
    PRBool mSucceeded;
    nsCString mUpdateDomain;
    nsCString mOwnerURI;
    nsCOMPtr<nsIURI> mReferrerURI;

    nsCString mClientID;
    nsCOMPtr<nsIOfflineCacheSession> mCacheSession;
    nsCOMPtr<nsIOfflineCacheSession> mMainCacheSession;

    nsCOMPtr<nsIObserverService> mObserverService;

    
    PRInt32 mCurrentItem;
    nsTArray<nsRefPtr<nsOfflineCacheUpdateItem> > mItems;

    
    nsCOMArray<nsIWeakReference> mWeakObservers;
    nsCOMArray<nsIOfflineCacheUpdateObserver> mObservers;
};

class nsOfflineCacheUpdateService : public nsIOfflineCacheUpdateService
                                  , public nsIWebProgressListener
                                  , public nsIObserver
                                  , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATESERVICE
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIOBSERVER

    nsOfflineCacheUpdateService();
    ~nsOfflineCacheUpdateService();

    nsresult Init();

    nsresult Schedule(nsOfflineCacheUpdate *aUpdate);
    nsresult ScheduleOnDocumentStop(nsOfflineCacheUpdate *aUpdate,
                                    nsIDOMDocument *aDocument);
    nsresult UpdateFinished(nsOfflineCacheUpdate *aUpdate);

    static nsOfflineCacheUpdateService *GetInstance();

private:
    nsresult ProcessNextUpdate();

    nsTArray<nsRefPtr<nsOfflineCacheUpdate> > mUpdates;
    nsRefPtrHashtable<nsVoidPtrHashKey, nsOfflineCacheUpdate> mDocUpdates;

    PRBool mDisabled;
    PRBool mUpdateRunning;
};

#endif
