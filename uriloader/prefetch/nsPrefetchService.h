




































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
#include "nsWeakReference.h"
#include "nsCOMPtr.h"

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
    void     UpdateCurrentChannel(nsIChannel *c) { mCurrentChannel = c; }

private:
    ~nsPrefetchService();

    nsresult Prefetch(nsIURI *aURI,
                      nsIURI *aReferrerURI,
                      PRBool aExplicit,
                      PRBool aOffline);

    void     AddProgressListener();
    void     RemoveProgressListener();
    nsresult EnqueueURI(nsIURI *aURI, nsIURI *aReferrerURI, PRBool aOffline);
    nsresult DequeueURI(nsIURI **aURI, nsIURI **aReferrerURI, PRBool *aOffline);
    void     EmptyQueue(PRBool includeOffline);
    nsresult SaveOfflineList(nsIURI *aDocumentUri,
                             nsIDOMDocument *aDoc);
    nsresult GetOfflineCacheSession(nsIOfflineCacheSession **aSession);

    void     StartPrefetching();
    void     StopPrefetching();

    nsCOMPtr<nsIOfflineCacheSession>  mOfflineCacheSession;
    nsPrefetchNode                   *mQueueHead;
    nsPrefetchNode                   *mQueueTail;
    nsCOMPtr<nsIChannel>              mCurrentChannel;
    PRInt32                           mStopCount;
    PRBool                            mDisabled;
    PRBool                            mFetchedOffline;

};





class nsPrefetchListener : public nsIStreamListener
                         , public nsIInterfaceRequestor
                         , public nsIChannelEventSink
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICHANNELEVENTSINK

    nsPrefetchListener(nsPrefetchService *aPrefetchService);

private:
    ~nsPrefetchListener();

    static NS_METHOD ConsumeSegments(nsIInputStream *, void *, const char *,
                                     PRUint32, PRUint32, PRUint32 *);

    nsPrefetchService *mService;
};





class nsPrefetchNode
{
public:
    nsPrefetchNode(nsIURI *aURI,
                   nsIURI *aReferrerURI,
                   PRBool aOffline)
        : mNext(nsnull)
        , mURI(aURI)
        , mReferrerURI(aReferrerURI)
        , mOffline(aOffline)
        { }

    nsPrefetchNode  *mNext;
    nsCOMPtr<nsIURI> mURI;
    nsCOMPtr<nsIURI> mReferrerURI;
    PRBool           mOffline;
};

#endif 
