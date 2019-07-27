







#ifndef nsDocLoader_h__
#define nsDocLoader_h__

#include "nsIDocumentLoader.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIRequestObserver.h"
#include "nsWeakReference.h"
#include "nsILoadGroup.h"
#include "nsCOMArray.h"
#include "nsTObserverArray.h"
#include "nsString.h"
#include "nsIChannel.h"
#include "nsIProgressEventSink.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIChannelEventSink.h"
#include "nsISecurityEventSink.h"
#include "nsISupportsPriority.h"
#include "nsCOMPtr.h"
#include "pldhash.h"
#include "nsAutoPtr.h"

#include "mozilla/LinkedList.h"





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
        return static_cast<nsIDocumentLoader*>(aDocLoader);
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

    struct nsListenerInfo {
      nsListenerInfo(nsIWeakReference *aListener, unsigned long aNotifyMask) 
        : mWeakListener(aListener),
          mNotifyMask(aNotifyMask)
      {
      }

      
      nsWeakPtr mWeakListener;

      
      unsigned long mNotifyMask;
    };

protected:
    virtual ~nsDocLoader();

    virtual nsresult SetDocLoaderParent(nsDocLoader * aLoader);

    bool IsBusy();

    void Destroy();
    virtual void DestroyChildren();

    nsIDocumentLoader* ChildAt(int32_t i) {
        return mChildList.SafeElementAt(i, nullptr);
    }

    void FireOnProgressChange(nsDocLoader* aLoadInitiator,
                              nsIRequest *request,
                              int64_t aProgress,
                              int64_t aProgressMax,
                              int64_t aProgressDelta,
                              int64_t aTotalProgress,
                              int64_t aMaxTotalProgress);

    
    
    
    
    
    typedef nsAutoTArray<nsRefPtr<nsDocLoader>, 8> WebProgressList;
    void GatherAncestorWebProgresses(WebProgressList& aList);

    void FireOnStateChange(nsIWebProgress *aProgress,
                           nsIRequest* request,
                           int32_t aStateFlags,
                           nsresult aStatus);

    
    
    
    
    
    void DoFireOnStateChange(nsIWebProgress * const aProgress,
                             nsIRequest* const request,
                             int32_t &aStateFlags,
                             const nsresult aStatus);

    void FireOnStatusChange(nsIWebProgress *aWebProgress,
                            nsIRequest *aRequest,
                            nsresult aStatus,
                            const char16_t* aMessage);

    void FireOnLocationChange(nsIWebProgress* aWebProgress,
                              nsIRequest* aRequest,
                              nsIURI *aUri,
                              uint32_t aFlags);

    bool RefreshAttempted(nsIWebProgress* aWebProgress,
                            nsIURI *aURI,
                            int32_t aDelay,
                            bool aSameURI);

    
    
    
    
    
    
    virtual void OnRedirectStateChange(nsIChannel* aOldChannel,
                                       nsIChannel* aNewChannel,
                                       uint32_t aRedirectFlags,
                                       uint32_t aStateFlags) {}

    void doStartDocumentLoad();
    void doStartURLLoad(nsIRequest *request);
    void doStopURLLoad(nsIRequest *request, nsresult aStatus);
    void doStopDocumentLoad(nsIRequest *request, nsresult aStatus);

    
    
    bool ChildEnteringOnload(nsIDocumentLoader* aChild) {
        
        
        
        return mChildrenInOnload.AppendObject(aChild);
    }

    
    
    void ChildDoneWithOnload(nsIDocumentLoader* aChild) {
        mChildrenInOnload.RemoveObject(aChild);
        DocLoaderIsEmpty(true);
    }        

protected:
    struct nsStatusInfo : public mozilla::LinkedListElement<nsStatusInfo>
    {
        nsString mStatusMessage;
        nsresult mStatusCode;
        
        nsIRequest * const mRequest;

        explicit nsStatusInfo(nsIRequest* aRequest) :
            mRequest(aRequest)
        {
            MOZ_COUNT_CTOR(nsStatusInfo);
        }
        ~nsStatusInfo()
        {
            MOZ_COUNT_DTOR(nsStatusInfo);
        }
    };

    struct nsRequestInfo : public PLDHashEntryHdr
    {
        explicit nsRequestInfo(const void* key)
            : mKey(key), mCurrentProgress(0), mMaxProgress(0), mUploading(false)
            , mLastStatus(nullptr)
        {
            MOZ_COUNT_CTOR(nsRequestInfo);
        }

        ~nsRequestInfo()
        {
            MOZ_COUNT_DTOR(nsRequestInfo);
        }

        nsIRequest* Request() {
            return static_cast<nsIRequest*>(const_cast<void*>(mKey));
        }

        const void* mKey; 
        int64_t mCurrentProgress;
        int64_t mMaxProgress;
        bool mUploading;

        nsAutoPtr<nsStatusInfo> mLastStatus;
    };

    static bool RequestInfoHashInitEntry(PLDHashTable* table, PLDHashEntryHdr* entry,
                                         const void* key);
    static void RequestInfoHashClearEntry(PLDHashTable* table, PLDHashEntryHdr* entry);

    
    
    
    
    
  
    nsCOMPtr<nsIRequest>       mDocumentRequest;       

    nsDocLoader*               mParent;                

    typedef nsAutoTObserverArray<nsListenerInfo, 8> ListenerArray;
    ListenerArray              mListenerInfoList;

    nsCOMPtr<nsILoadGroup>        mLoadGroup;
    
    nsTObserverArray<nsDocLoader*> mChildList;

    
    
    int32_t mProgressStateFlags;

    int64_t mCurrentSelfProgress;
    int64_t mMaxSelfProgress;

    int64_t mCurrentTotalProgress;
    int64_t mMaxTotalProgress;

    PLDHashTable mRequestInfoHash;
    int64_t mCompletedTotalProgress;

    mozilla::LinkedList<nsStatusInfo> mStatusInfoList;

    




    bool mIsLoadingDocument;

    
    bool mIsRestoringDocument;

    

    bool mDontFlushLayout;

    



    bool mIsFlushingLayout;

private:
    
    
    
    
    nsCOMArray<nsIDocumentLoader> mChildrenInOnload;
    
    
    
    
    
    
    void DocLoaderIsEmpty(bool aFlushLayout);

    int64_t GetMaxTotalProgress();

    nsresult AddRequestInfo(nsIRequest* aRequest);
    void RemoveRequestInfo(nsIRequest* aRequest);
    nsRequestInfo *GetRequestInfo(nsIRequest* aRequest);
    void ClearRequestInfoHash();
    int64_t CalculateMaxProgress();
    static PLDHashOperator CalcMaxProgressCallback(PLDHashTable* table,
                                                   PLDHashEntryHdr* hdr,
                                                   uint32_t number, void* arg);


    
    void ClearInternalProgress(); 
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsDocLoader, NS_THIS_DOCLOADER_IMPL_CID)

#endif 
