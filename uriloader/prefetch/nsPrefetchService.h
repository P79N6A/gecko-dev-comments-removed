




































#ifndef nsPrefetchService_h__
#define nsPrefetchService_h__

#include "nsCPrefetchService.h"
#include "nsIGenericFactory.h"
#include "nsIObserver.h"
#include "nsIInterfaceRequestor.h"
#include "nsIChannelEventSink.h"
#include "nsIWebProgressListener.h"
#include "nsIStreamListener.h"
#include "nsIChannel.h"
#include "nsIURI.h"
#include "nsIDOMDocument.h"
#include "nsIDOMLoadStatus.h"
#include "nsWeakReference.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsPrefetchService;
class nsPrefetchListener;
class nsPrefetchNode;
class nsIOfflineCacheSession;





class nsPrefetchService : public nsIPrefetchService
                        , public nsIWebProgressListener
                        , public nsIObserver
                        , public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPREFETCHSERVICE
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIOBSERVER

    nsPrefetchService();

    nsresult Init();
    void     ProcessNextURI();

    nsPrefetchNode *GetCurrentNode() { return mCurrentNode.get(); }
    nsPrefetchNode *GetQueueHead() { return mQueueHead; }

    void NotifyLoadRequested(nsPrefetchNode *node);
    void NotifyLoadCompleted(nsPrefetchNode *node);

private:
    ~nsPrefetchService();

    nsresult Prefetch(nsIURI *aURI,
                      nsIURI *aReferrerURI,
                      nsIDOMNode *aSource,
                      PRBool aExplicit,
                      PRBool aOffline);

    void     AddProgressListener();
    void     RemoveProgressListener();
    nsresult EnqueueURI(nsIURI *aURI, nsIURI *aReferrerURI,
                        nsIDOMNode *aSource, PRBool aOffline,
                        nsPrefetchNode **node);
    nsresult EnqueueNode(nsPrefetchNode *node);
    nsresult DequeueNode(nsPrefetchNode **node);
    void     EmptyQueue(PRBool includeOffline);
    nsresult SaveOfflineList(nsIURI *aDocumentUri,
                             nsIDOMDocument *aDoc);
    nsresult GetOfflineCacheSession(nsIOfflineCacheSession **aSession);

    void     StartPrefetching();
    void     StopPrefetching();

    nsCOMPtr<nsIOfflineCacheSession>  mOfflineCacheSession;
    nsPrefetchNode                   *mQueueHead;
    nsPrefetchNode                   *mQueueTail;
    nsRefPtr<nsPrefetchNode>          mCurrentNode;
    PRInt32                           mStopCount;
    
    PRInt32                           mHaveProcessed;
    PRBool                            mDisabled;
    PRBool                            mFetchedOffline;

};





class nsPrefetchNode : public nsIDOMLoadStatus
                     , public nsIStreamListener
                     , public nsIInterfaceRequestor
                     , public nsIChannelEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOMLOADSTATUS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICHANNELEVENTSINK

    nsPrefetchNode(nsPrefetchService *aPrefetchService,
                   nsIURI *aURI,
                   nsIURI *aReferrerURI,
                   nsIDOMNode *aSource,
                   PRBool aOffline);

    ~nsPrefetchNode() {}

    nsresult OpenChannel();
    nsresult CancelChannel(nsresult error);

    nsPrefetchNode             *mNext;
    nsCOMPtr<nsIURI>            mURI;
    nsCOMPtr<nsIURI>            mReferrerURI;
    nsCOMPtr<nsIWeakReference>  mSource;
    PRBool                      mOffline;

private:
    nsRefPtr<nsPrefetchService> mService;
    nsCOMPtr<nsIChannel>        mChannel;
    PRUint16                    mState;
    PRInt32                     mBytesRead;
};

#endif 
