







































#ifndef nsDocLoader_h__
#define nsDocLoader_h__

#include "nsIDocumentLoader.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIRequestObserver.h"
#include "nsWeakReference.h"
#include "nsILoadGroup.h"
#include "nsCOMArray.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "nsIChannel.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIChannelEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsISupportsPriority.h"
#include "nsInt64.h"
#include "nsCOMPtr.h"
#include "pldhash.h"

struct nsRequestInfo;
struct nsListenerInfo;





#define NS_THIS_DOCLOADER_IMPL_CID                    \
 { /* b4ec8387-98aa-4c08-93b6-6d23069c06f2 */         \
     0xb4ec8387,                                      \
     0x98aa,                                          \
     0x4c08,                                          \
     {0x93, 0xb6, 0x6d, 0x23, 0x06, 0x9c, 0x06, 0xf2} \
 }

class nsDocLoader : public nsIDocumentLoader, 
                    public nsIRequestObserver,
                    public nsSupportsWeakReference,
                    public nsIProgressEventSink,
                    public nsIWebProgress,
                    public nsIInterfaceRequestor,
                    public nsIChannelEventSink,
                    public nsISecurityEventSink,
                    public nsISupportsPriority
{
public:
    NS_DECLARE_STATIC_IID_ACCESSOR(NS_THIS_DOCLOADER_IMPL_CID)

    nsDocLoader();

    virtual nsresult Init();

    static already_AddRefed<nsDocLoader> GetAsDocLoader(nsISupports* aSupports);
    
    static nsISupports* GetAsSupports(nsDocLoader* aDocLoader) {
        return NS_STATIC_CAST(nsIDocumentLoader*, aDocLoader);
    }

    
    static nsresult AddDocLoaderAsChildOfRoot(nsDocLoader* aDocLoader);

    NS_DECL_ISUPPORTS
    NS_DECL_NSIDOCUMENTLOADER
    
    
    NS_DECL_NSIPROGRESSEVENTSINK

    NS_DECL_NSISECURITYEVENTSINK

    
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIWEBPROGRESS

    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSISUPPORTSPRIORITY

    

    
    
    nsresult RemoveChildLoader(nsDocLoader *aChild);
    
    
    nsresult AddChildLoader(nsDocLoader* aChild);
    nsDocLoader* GetParent() const { return mParent; }

protected:
    virtual ~nsDocLoader();

    virtual nsresult SetDocLoaderParent(nsDocLoader * aLoader);

    PRBool IsBusy();

    void Destroy();
    virtual void DestroyChildren();

    nsIDocumentLoader* ChildAt(PRInt32 i) {
        return NS_STATIC_CAST(nsDocLoader*, mChildList[i]);
    }

    nsIDocumentLoader* SafeChildAt(PRInt32 i) {
        return NS_STATIC_CAST(nsDocLoader*, mChildList.SafeElementAt(i));
    }

    void FireOnProgressChange(nsDocLoader* aLoadInitiator,
                              nsIRequest *request,
                              PRInt64 aProgress,
                              PRInt64 aProgressMax,
                              PRInt64 aProgressDelta,
                              PRInt64 aTotalProgress,
                              PRInt64 aMaxTotalProgress);

    void FireOnStateChange(nsIWebProgress *aProgress,
                           nsIRequest* request,
                           PRInt32 aStateFlags,
                           nsresult aStatus);

    void FireOnStatusChange(nsIWebProgress *aWebProgress,
                            nsIRequest *aRequest,
                            nsresult aStatus,
                            const PRUnichar* aMessage);

    void FireOnLocationChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest,
                              nsIURI *aUri);

    PRBool RefreshAttempted(nsIWebProgress* aWebProgress,
                            nsIURI *aURI,
                            PRInt32 aDelay,
                            PRBool aSameURI);

    
    
    
    
    
    
    virtual void OnRedirectStateChange(nsIChannel* aOldChannel,
                                       nsIChannel* aNewChannel,
                                       PRUint32 aRedirectFlags,
                                       PRUint32 aStateFlags) {}

    void doStartDocumentLoad();
    void doStartURLLoad(nsIRequest *request);
    void doStopURLLoad(nsIRequest *request, nsresult aStatus);
    void doStopDocumentLoad(nsIRequest *request, nsresult aStatus);

    
    
    PRBool ChildEnteringOnload(nsIDocumentLoader* aChild) {
        
        
        
        return mChildrenInOnload.AppendObject(aChild);
    }

    
    
    void ChildDoneWithOnload(nsIDocumentLoader* aChild) {
        mChildrenInOnload.RemoveObject(aChild);
        DocLoaderIsEmpty();
    }        

protected:
    
    
    
    
    
  
    nsCOMPtr<nsIRequest>       mDocumentRequest;       

    nsDocLoader*               mParent;                

    nsVoidArray                mListenerInfoList;
    




    PRBool mIsLoadingDocument;

    nsCOMPtr<nsILoadGroup>        mLoadGroup;
    
    nsVoidArray                   mChildList;

    
    
    PRInt32 mProgressStateFlags;

    nsInt64 mCurrentSelfProgress;
    nsInt64 mMaxSelfProgress;

    nsInt64 mCurrentTotalProgress;
    nsInt64 mMaxTotalProgress;

    PLDHashTable mRequestInfoHash;

    
    PRBool mIsRestoringDocument;

private:
    
    
    
    
    nsCOMArray<nsIDocumentLoader> mChildrenInOnload;
    
    
    
    
    void DocLoaderIsEmpty();

    nsListenerInfo *GetListenerInfo(nsIWebProgressListener* aListener);

    PRInt64 GetMaxTotalProgress();

    nsresult AddRequestInfo(nsIRequest* aRequest);
    nsRequestInfo *GetRequestInfo(nsIRequest* aRequest);
    void ClearRequestInfoHash();
    PRInt64 CalculateMaxProgress();


    
    void ClearInternalProgress(); 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocLoader, NS_THIS_DOCLOADER_IMPL_CID)

#endif 
