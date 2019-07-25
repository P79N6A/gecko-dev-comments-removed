





































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
#include "nsIMutableArray.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIApplicationCache.h"
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
class nsIUTF8StringEnumerator;

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
                             nsIApplicationCache *aPreviousApplicationCache,
                             const nsACString &aClientID,
                             PRUint32 aType);
    virtual ~nsOfflineCacheUpdateItem();

    nsCOMPtr<nsIURI>           mURI;
    nsCOMPtr<nsIURI>           mReferrerURI;
    nsCOMPtr<nsIApplicationCache> mPreviousApplicationCache;
    nsCString                  mClientID;
    nsCString                  mCacheKey;
    PRUint32                   mItemType;

    nsresult OpenChannel();
    nsresult Cancel();
    nsresult GetRequestSucceeded(PRBool * succeeded);

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
                          nsIApplicationCache *aPreviousApplicationCache,
                          const nsACString &aClientID);
    virtual ~nsOfflineManifestItem();

    nsCOMArray<nsIURI> &GetExplicitURIs() { return mExplicitURIs; }
    nsCOMArray<nsIURI> &GetFallbackURIs() { return mFallbackURIs; }

    nsTArray<nsCString> &GetOpportunisticNamespaces()
        { return mOpportunisticNamespaces; }
    nsIArray *GetNamespaces()
        { return mNamespaces.get(); }

    PRBool ParseSucceeded()
        { return (mParserState != PARSE_INIT && mParserState != PARSE_ERROR); }
    PRBool NeedsUpdate() { return mParserState != PARSE_INIT && mNeedsUpdate; }

    void GetManifestHash(nsCString &aManifestHash)
        { aManifestHash = mManifestHashValue; }

private:
    static NS_METHOD ReadManifest(nsIInputStream *aInputStream,
                                  void *aClosure,
                                  const char *aFromSegment,
                                  PRUint32 aOffset,
                                  PRUint32 aCount,
                                  PRUint32 *aBytesConsumed);

    nsresult AddNamespace(PRUint32 namespaceType,
                          const nsCString &namespaceSpec,
                          const nsCString &data);

    nsresult HandleManifestLine(const nsCString::const_iterator &aBegin,
                                const nsCString::const_iterator &aEnd);

    




    nsresult GetOldManifestContentHash(nsIRequest *aRequest);
    





    nsresult CheckNewManifestContentHash(nsIRequest *aRequest);

    void ReadStrictFileOriginPolicyPref();

    enum {
        PARSE_INIT,
        PARSE_CACHE_ENTRIES,
        PARSE_FALLBACK_ENTRIES,
        PARSE_BYPASS_ENTRIES,
        PARSE_ERROR
    } mParserState;

    nsCString mReadBuf;

    nsCOMArray<nsIURI> mExplicitURIs;
    nsCOMArray<nsIURI> mFallbackURIs;

    
    
    nsTArray<nsCString> mOpportunisticNamespaces;

    
    
    nsCOMPtr<nsIMutableArray> mNamespaces;

    PRBool mNeedsUpdate;
    PRBool mStrictFileOriginPolicy;

    
    nsCOMPtr<nsICryptoHash> mManifestHash;
    PRBool mManifestHashInitialized;
    nsCString mManifestHashValue;
    nsCString mOldManifestHashValue;
};

class nsOfflineCacheUpdateOwner
{
public:
    virtual nsresult UpdateFinished(nsOfflineCacheUpdate *aUpdate) = 0;
};

class nsOfflineCacheUpdate : public nsIOfflineCacheUpdate
                           , public nsOfflineCacheUpdateOwner
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATE

    nsOfflineCacheUpdate();
    ~nsOfflineCacheUpdate();

    static nsresult GetCacheKey(nsIURI *aURI, nsACString &aKey);

    nsresult Init();

    nsresult Begin();
    nsresult Cancel();

    void LoadCompleted();
    void ManifestCheckCompleted(nsresult aStatus,
                                const nsCString &aManifestHash);
    void AddDocument(nsIDOMDocument *aDocument);

    void SetOwner(nsOfflineCacheUpdateOwner *aOwner);

    virtual nsresult UpdateFinished(nsOfflineCacheUpdate *aUpdate);

private:
    nsresult HandleManifest(PRBool *aDoUpdate);
    nsresult AddURI(nsIURI *aURI, PRUint32 aItemType);

    nsresult ProcessNextURI();

    
    
    
    nsresult AddExistingItems(PRUint32 aType,
                              nsTArray<nsCString>* namespaceFilter = nsnull);

    nsresult GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers);
    nsresult NotifyError();
    nsresult NotifyChecking();
    nsresult NotifyNoUpdate();
    nsresult NotifyObsolete();
    nsresult NotifyDownloading();
    nsresult NotifyStarted(nsOfflineCacheUpdateItem *aItem);
    nsresult NotifyCompleted(nsOfflineCacheUpdateItem *aItem);
    nsresult AssociateDocument(nsIDOMDocument *aDocument,
                               nsIApplicationCache *aApplicationCache);
    nsresult ScheduleImplicit();
    nsresult Finish();

    enum {
        STATE_UNINITIALIZED,
        STATE_INITIALIZED,
        STATE_CHECKING,
        STATE_DOWNLOADING,
        STATE_CANCELLED,
        STATE_FINISHED
    } mState;

    nsOfflineCacheUpdateOwner *mOwner;

    PRPackedBool mAddedItems;
    PRPackedBool mPartialUpdate;
    PRPackedBool mSucceeded;
    PRPackedBool mObsolete;

    nsCString mUpdateDomain;
    nsCOMPtr<nsIURI> mManifestURI;

    nsCOMPtr<nsIURI> mDocumentURI;

    nsCString mClientID;
    nsCOMPtr<nsIApplicationCache> mApplicationCache;
    nsCOMPtr<nsIApplicationCache> mPreviousApplicationCache;

    nsCOMPtr<nsIObserverService> mObserverService;

    nsRefPtr<nsOfflineManifestItem> mManifestItem;

    
    PRInt32 mCurrentItem;
    nsTArray<nsRefPtr<nsOfflineCacheUpdateItem> > mItems;

    
    nsCOMArray<nsIWeakReference> mWeakObservers;
    nsCOMArray<nsIOfflineCacheUpdateObserver> mObservers;

    
    nsCOMArray<nsIDOMDocument> mDocuments;

    

    PRUint32 mRescheduleCount;

    nsRefPtr<nsOfflineCacheUpdate> mImplicitUpdate;
};

class nsOfflineCacheUpdateService : public nsIOfflineCacheUpdateService
                                  , public nsIObserver
                                  , public nsOfflineCacheUpdateOwner
                                  , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATESERVICE
    NS_DECL_NSIOBSERVER

    nsOfflineCacheUpdateService();
    ~nsOfflineCacheUpdateService();

    nsresult Init();

    nsresult Schedule(nsOfflineCacheUpdate *aUpdate);
    nsresult Schedule(nsIURI *aManifestURI,
                      nsIURI *aDocumentURI,
                      nsIDOMDocument *aDocument,
                      nsIOfflineCacheUpdate **aUpdate);

    virtual nsresult UpdateFinished(nsOfflineCacheUpdate *aUpdate);

    



    static nsOfflineCacheUpdateService *EnsureService();

    
    static nsOfflineCacheUpdateService *GetInstance();

private:
    nsresult ProcessNextUpdate();

    nsTArray<nsRefPtr<nsOfflineCacheUpdate> > mUpdates;

    PRBool mDisabled;
    PRBool mUpdateRunning;
};

#endif
