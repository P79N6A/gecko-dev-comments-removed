





































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
#include "nsClassHashtable.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsWeakReference.h"
#include "nsICryptoHash.h"

class nsOfflineCacheUpdate;

class nsICacheEntryDescriptor;

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
                             const nsACString &aClientID);
    virtual ~nsOfflineCacheUpdateItem();

    nsCOMPtr<nsIURI>           mURI;
    nsCOMPtr<nsIURI>           mReferrerURI;
    nsCString                  mClientID;

    nsresult OpenChannel();
    nsresult Cancel();

private:
    nsOfflineCacheUpdate*          mUpdate;
    nsCOMPtr<nsIChannel>           mChannel;
    PRUint16                       mState;

protected:
    PRInt32                        mBytesRead;
};


class nsOfflineManifestItem : public nsOfflineCacheUpdateItem
{
public:
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER

    nsOfflineManifestItem(nsOfflineCacheUpdate *aUpdate,
                          nsIURI *aURI,
                          nsIURI *aReferrerURI,
                          const nsACString &aClientID);
    virtual ~nsOfflineManifestItem();

    nsCOMArray<nsIURI> &GetExplicitURIs() { return mExplicitURIs; }

    PRBool ParseSucceeded()
        { return (mParserState != PARSE_INIT && mParserState != PARSE_ERROR); }
    PRBool NeedsUpdate() { return mParserState != PARSE_INIT && mNeedsUpdate; }

private:
    static NS_METHOD ReadManifest(nsIInputStream *aInputStream,
                                  void *aClosure,
                                  const char *aFromSegment,
                                  PRUint32 aOffset,
                                  PRUint32 aCount,
                                  PRUint32 *aBytesConsumed);

    nsresult HandleManifestLine(const nsCString::const_iterator &aBegin,
                                const nsCString::const_iterator &aEnd);

    




    nsresult GetOldManifestContentHash(nsIRequest *aRequest);
    





    nsresult CheckNewManifestContentHash(nsIRequest *aRequest);

    enum {
        PARSE_INIT,
        PARSE_CACHE_ENTRIES,
        PARSE_FALLBACK_ENTRIES,
        PARSE_NETWORK_ENTRIES,
        PARSE_ERROR
    } mParserState;

    nsCString mReadBuf;
    nsCOMArray<nsIURI> mExplicitURIs;
    PRBool mNeedsUpdate;

    
    nsCOMPtr<nsICryptoHash> mManifestHash;
    PRBool mManifestHashInitialized;
    nsCString mOldManifestHashValue;
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
    nsresult HandleManifest(PRBool *aDoUpdate);
    nsresult AddURI(nsIURI *aURI, const nsACString &aOwnerSpec);

    nsresult ProcessNextURI();

    nsresult AddOwnedItems(const nsACString &aOwnerURI);

    nsresult GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers);
    nsresult NotifyError();
    nsresult NotifyChecking();
    nsresult NotifyNoUpdate();
    nsresult NotifyDownloading();
    nsresult NotifyStarted(nsOfflineCacheUpdateItem *aItem);
    nsresult NotifyCompleted(nsOfflineCacheUpdateItem *aItem);
    nsresult Finish();

    enum {
        STATE_UNINITIALIZED,
        STATE_INITIALIZED,
        STATE_CHECKING,
        STATE_DOWNLOADING,
        STATE_CANCELLED,
        STATE_FINISHED
    } mState;

    PRBool mAddedItems;
    PRBool mPartialUpdate;
    PRBool mSucceeded;
    nsCString mUpdateDomain;
    nsCOMPtr<nsIURI> mManifestURI;
    nsCString mManifestOwnerSpec;
    nsCString mDynamicOwnerSpec;

    nsCOMPtr<nsIURI> mDocumentURI;

    nsCString mClientID;
    nsCOMPtr<nsIOfflineCacheSession> mCacheSession;
    nsCOMPtr<nsIOfflineCacheSession> mMainCacheSession;

    nsCOMPtr<nsIObserverService> mObserverService;

    nsRefPtr<nsOfflineManifestItem> mManifestItem;

    
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
    nsresult UpdateFinished(nsOfflineCacheUpdate *aUpdate);

    



    static nsOfflineCacheUpdateService *EnsureService();

    
    static nsOfflineCacheUpdateService *GetInstance();
    
private:
    nsresult ProcessNextUpdate();

    nsTArray<nsRefPtr<nsOfflineCacheUpdate> > mUpdates;

    struct PendingUpdate {
        nsCOMPtr<nsIURI> mManifestURI;
        nsCOMPtr<nsIURI> mDocumentURI;
    };
    nsClassHashtable<nsVoidPtrHashKey, PendingUpdate> mDocUpdates;

    PRBool mDisabled;
    PRBool mUpdateRunning;
};

#endif
