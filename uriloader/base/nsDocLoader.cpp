




#include "nspr.h"
#include "prlog.h"

#include "nsDocLoader.h"
#include "nsCURILoader.h"
#include "nsNetUtil.h"
#include "nsIHttpChannel.h"
#include "nsIWebProgressListener2.h"

#include "nsIServiceManager.h"
#include "nsXPIDLString.h"

#include "nsIURL.h"
#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsWeakPtr.h"
#include "nsAutoPtr.h"
#include "nsQueryObject.h"

#include "nsIDOMWindow.h"

#include "nsIStringBundle.h"
#include "nsIScriptSecurityManager.h"

#include "nsITransport.h"
#include "nsISocketTransport.h"

#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsPresContext.h"
#include "nsIAsyncVerifyRedirectCallback.h"

static NS_DEFINE_CID(kThisImplCID, NS_THIS_DOCLOADER_IMPL_CID);

#if defined(PR_LOGGING)











PRLogModuleInfo* gDocLoaderLog = nullptr;
#endif 


#if defined(DEBUG)
void GetURIStringFromRequest(nsIRequest* request, nsACString &name)
{
    if (request)
        request->GetName(name);
    else
        name.AssignLiteral("???");
}
#endif 



void
nsDocLoader::RequestInfoHashInitEntry(PLDHashEntryHdr* entry,
                                      const void* key)
{
  
  new (entry) nsRequestInfo(key);
}

void
nsDocLoader::RequestInfoHashClearEntry(PLDHashTable* table,
                                       PLDHashEntryHdr* entry)
{
  nsRequestInfo* info = static_cast<nsRequestInfo *>(entry);
  info->~nsRequestInfo();
}


template <>
class nsDefaultComparator <nsDocLoader::nsListenerInfo, nsIWebProgressListener*> {
  public:
    bool Equals(const nsDocLoader::nsListenerInfo& aInfo,
                nsIWebProgressListener* const& aListener) const {
      nsCOMPtr<nsIWebProgressListener> listener =
                                       do_QueryReferent(aInfo.mWeakListener);
      return aListener == listener;
    }
};

nsDocLoader::nsDocLoader()
  : mParent(nullptr),
    mCurrentSelfProgress(0),
    mMaxSelfProgress(0),
    mCurrentTotalProgress(0),
    mMaxTotalProgress(0),
    mCompletedTotalProgress(0),
    mIsLoadingDocument(false),
    mIsRestoringDocument(false),
    mDontFlushLayout(false),
    mIsFlushingLayout(false)
{
#if defined(PR_LOGGING)
  if (nullptr == gDocLoaderLog) {
      gDocLoaderLog = PR_NewLogModule("DocLoader");
  }
#endif 

  static const PLDHashTableOps hash_table_ops =
  {
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    RequestInfoHashClearEntry,
    RequestInfoHashInitEntry
  };

  PL_DHashTableInit(&mRequestInfoHash, &hash_table_ops, sizeof(nsRequestInfo));

  ClearInternalProgress();

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: created.\n", this));
}

nsresult
nsDocLoader::SetDocLoaderParent(nsDocLoader *aParent)
{
  mParent = aParent;
  return NS_OK;
}

nsresult
nsDocLoader::Init()
{
  if (!mRequestInfoHash.IsInitialized()) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = NS_NewLoadGroup(getter_AddRefs(mLoadGroup), this);
  if (NS_FAILED(rv)) return rv;

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: load group %x.\n", this, mLoadGroup.get()));

  return NS_OK;
}

nsDocLoader::~nsDocLoader()
{
		








  
  
  ClearWeakReferences();

  Destroy();

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: deleted.\n", this));

  if (mRequestInfoHash.IsInitialized()) {
    PL_DHashTableFinish(&mRequestInfoHash);
  }
}





NS_IMPL_ADDREF(nsDocLoader)
NS_IMPL_RELEASE(nsDocLoader)

NS_INTERFACE_MAP_BEGIN(nsDocLoader)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIRequestObserver)
   NS_INTERFACE_MAP_ENTRY(nsIRequestObserver)
   NS_INTERFACE_MAP_ENTRY(nsIDocumentLoader)
   NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgress)
   NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink)
   NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
   NS_INTERFACE_MAP_ENTRY(nsIChannelEventSink)
   NS_INTERFACE_MAP_ENTRY(nsISecurityEventSink)
   NS_INTERFACE_MAP_ENTRY(nsISupportsPriority)
   if (aIID.Equals(kThisImplCID))
     foundInterface = static_cast<nsIDocumentLoader *>(this);
   else
NS_INTERFACE_MAP_END





NS_IMETHODIMP nsDocLoader::GetInterface(const nsIID& aIID, void** aSink)
{
  nsresult rv = NS_ERROR_NO_INTERFACE;

  NS_ENSURE_ARG_POINTER(aSink);

  if(aIID.Equals(NS_GET_IID(nsILoadGroup))) {
    *aSink = mLoadGroup;
    NS_IF_ADDREF((nsISupports*)*aSink);
    rv = NS_OK;
  } else {
    rv = QueryInterface(aIID, aSink);
  }

  return rv;
}


already_AddRefed<nsDocLoader>
nsDocLoader::GetAsDocLoader(nsISupports* aSupports)
{
  nsRefPtr<nsDocLoader> ret = do_QueryObject(aSupports);
  return ret.forget();
}


nsresult
nsDocLoader::AddDocLoaderAsChildOfRoot(nsDocLoader* aDocLoader)
{
  nsresult rv;
  nsCOMPtr<nsIDocumentLoader> docLoaderService =
    do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<nsDocLoader> rootDocLoader = GetAsDocLoader(docLoaderService);
  NS_ENSURE_TRUE(rootDocLoader, NS_ERROR_UNEXPECTED);

  return rootDocLoader->AddChildLoader(aDocLoader);
}

NS_IMETHODIMP
nsDocLoader::Stop(void)
{
  nsresult rv = NS_OK;

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: Stop() called\n", this));

  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mChildList, nsDocLoader, Stop, ());

  if (mLoadGroup)
    rv = mLoadGroup->Cancel(NS_BINDING_ABORTED);

  
  
  mIsFlushingLayout = false;

  
  
  
  
  mChildrenInOnload.Clear();

  
  
  
  
  
  

  
  

  NS_ASSERTION(!IsBusy(), "Shouldn't be busy here");
  DocLoaderIsEmpty(false);

  return rv;
}


bool
nsDocLoader::IsBusy()
{
  nsresult rv;

  
  
  
  
  
  
  
  
  

  if (mChildrenInOnload.Count() || mIsFlushingLayout) {
    return true;
  }

  
  if (!mIsLoadingDocument) {
    return false;
  }

  bool busy;
  rv = mLoadGroup->IsPending(&busy);
  if (NS_FAILED(rv)) {
    return false;
  }
  if (busy) {
    return true;
  }

  
  uint32_t count = mChildList.Length();
  for (uint32_t i=0; i < count; i++) {
    nsIDocumentLoader* loader = ChildAt(i);

    
    
    if (loader && static_cast<nsDocLoader*>(loader)->IsBusy())
      return true;
  }

  return false;
}

NS_IMETHODIMP
nsDocLoader::GetContainer(nsISupports** aResult)
{
   NS_ADDREF(*aResult = static_cast<nsIDocumentLoader*>(this));

   return NS_OK;
}

NS_IMETHODIMP
nsDocLoader::GetLoadGroup(nsILoadGroup** aResult)
{
  nsresult rv = NS_OK;

  if (nullptr == aResult) {
    rv = NS_ERROR_NULL_POINTER;
  } else {
    *aResult = mLoadGroup;
    NS_IF_ADDREF(*aResult);
  }
  return rv;
}

void
nsDocLoader::Destroy()
{
  Stop();

  
  if (mParent)
  {
    mParent->RemoveChildLoader(this);
  }

  
  ClearRequestInfoHash();

  mListenerInfoList.Clear();
  mListenerInfoList.Compact();

  mDocumentRequest = 0;

  if (mLoadGroup)
    mLoadGroup->SetGroupObserver(nullptr);

  DestroyChildren();
}

void
nsDocLoader::DestroyChildren()
{
  uint32_t count = mChildList.Length();
  
  
  
  for (uint32_t i=0; i < count; i++)
  {
    nsIDocumentLoader* loader = ChildAt(i);

    if (loader) {
      
      
      static_cast<nsDocLoader*>(loader)->SetDocLoaderParent(nullptr);
    }
  }
  mChildList.Clear();
}

NS_IMETHODIMP
nsDocLoader::OnStartRequest(nsIRequest *request, nsISupports *aCtxt)
{
  

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gDocLoaderLog, PR_LOG_DEBUG)) {
    nsAutoCString name;
    request->GetName(name);

    uint32_t count = 0;
    if (mLoadGroup)
      mLoadGroup->GetActiveCount(&count);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
           ("DocLoader:%p: OnStartRequest[%p](%s) mIsLoadingDocument=%s, %u active URLs",
            this, request, name.get(),
            (mIsLoadingDocument ? "true" : "false"),
            count));
  }
#endif 
  bool bJustStartedLoading = false;

  nsLoadFlags loadFlags = 0;
  request->GetLoadFlags(&loadFlags);

  if (!mIsLoadingDocument && (loadFlags & nsIChannel::LOAD_DOCUMENT_URI)) {
      bJustStartedLoading = true;
      mIsLoadingDocument = true;
      ClearInternalProgress(); 
  }

  
  
  
  
  AddRequestInfo(request);

  
  
  
  
  
  if (mIsLoadingDocument) {
    if (loadFlags & nsIChannel::LOAD_DOCUMENT_URI) {
      
      
      
      
      NS_ASSERTION((loadFlags & nsIChannel::LOAD_REPLACE) ||
                   !(mDocumentRequest.get()),
                   "Overwriting an existing document channel!");

      
      mDocumentRequest = request;
      mLoadGroup->SetDefaultLoadRequest(request);

      
      
      
      if (bJustStartedLoading) {
        
        mProgressStateFlags = nsIWebProgressListener::STATE_START;

        
        doStartDocumentLoad();
        return NS_OK;
      }
    }
  }

  NS_ASSERTION(!mIsLoadingDocument || mDocumentRequest,
               "mDocumentRequest MUST be set for the duration of a page load!");

  doStartURLLoad(request);

  return NS_OK;
}

NS_IMETHODIMP
nsDocLoader::OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aCtxt,
                           nsresult aStatus)
{
  nsresult rv = NS_OK;

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gDocLoaderLog, PR_LOG_DEBUG)) {
    nsAutoCString name;
    aRequest->GetName(name);

    uint32_t count = 0;
    if (mLoadGroup)
      mLoadGroup->GetActiveCount(&count);

    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
           ("DocLoader:%p: OnStopRequest[%p](%s) status=%x mIsLoadingDocument=%s, %u active URLs",
           this, aRequest, name.get(),
           aStatus, (mIsLoadingDocument ? "true" : "false"),
           count));
  }
#endif

  bool bFireTransferring = false;

  
  
  
  
  
  
  nsRequestInfo *info = GetRequestInfo(aRequest);
  if (info) {
    
    
    
    info->mLastStatus = nullptr;

    int64_t oldMax = info->mMaxProgress;

    info->mMaxProgress = info->mCurrentProgress;

    
    
    
    
    
    if ((oldMax < int64_t(0)) && (mMaxSelfProgress < int64_t(0))) {
      mMaxSelfProgress = CalculateMaxProgress();
    }

    
    
    
    mCompletedTotalProgress += info->mMaxProgress;

    
    
    
    
    
    
    
    
    if ((oldMax == 0) && (info->mCurrentProgress == 0)) {
      nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));

      
      
      
      if (channel) {
        if (NS_SUCCEEDED(aStatus)) {
          bFireTransferring = true;
        }
        
        
        
        
        
        else if (aStatus != NS_BINDING_REDIRECTED &&
                 aStatus != NS_BINDING_RETARGETED) {
          
          
          
          uint32_t lf;
          channel->GetLoadFlags(&lf);
          if (lf & nsIChannel::LOAD_TARGETED) {
            nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
            if (httpChannel) {
              uint32_t responseCode;
              rv = httpChannel->GetResponseStatus(&responseCode);
              if (NS_SUCCEEDED(rv)) {
                
                
                
                
                
                bFireTransferring = true;
              }
            }
          }
        }
      }
    }
  }

  if (bFireTransferring) {
    
    int32_t flags;

    flags = nsIWebProgressListener::STATE_TRANSFERRING |
            nsIWebProgressListener::STATE_IS_REQUEST;
    
    
    
    if (mProgressStateFlags & nsIWebProgressListener::STATE_START) {
      mProgressStateFlags = nsIWebProgressListener::STATE_TRANSFERRING;

      
      flags |= nsIWebProgressListener::STATE_IS_DOCUMENT;
    }

    FireOnStateChange(this, aRequest, flags, NS_OK);
  }

  
  
  
  doStopURLLoad(aRequest, aStatus);

  
  
  RemoveRequestInfo(aRequest);

  
  
  
  
  if (mIsLoadingDocument) {
    DocLoaderIsEmpty(true);
  }

  return NS_OK;
}


nsresult nsDocLoader::RemoveChildLoader(nsDocLoader* aChild)
{
  nsresult rv = mChildList.RemoveElement(aChild) ? NS_OK : NS_ERROR_FAILURE;
  if (NS_SUCCEEDED(rv)) {
    aChild->SetDocLoaderParent(nullptr);
  }
  return rv;
}

nsresult nsDocLoader::AddChildLoader(nsDocLoader* aChild)
{
  nsresult rv = mChildList.AppendElement(aChild) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  if (NS_SUCCEEDED(rv)) {
    aChild->SetDocLoaderParent(this);
  }
  return rv;
}

NS_IMETHODIMP nsDocLoader::GetDocumentChannel(nsIChannel ** aChannel)
{
  if (!mDocumentRequest) {
    *aChannel = nullptr;
    return NS_OK;
  }

  return CallQueryInterface(mDocumentRequest, aChannel);
}


void nsDocLoader::DocLoaderIsEmpty(bool aFlushLayout)
{
  if (mIsLoadingDocument) {
    



    nsCOMPtr<nsIDocumentLoader> kungFuDeathGrip(this);

    
    if (IsBusy()) {
      return;
    }

    NS_ASSERTION(!mIsFlushingLayout, "Someone screwed up");

    
    if (aFlushLayout && !mDontFlushLayout) {
      nsCOMPtr<nsIDOMDocument> domDoc = do_GetInterface(GetAsSupports(this));
      nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
      if (doc) {
        
        
        
        mozFlushType flushType = Flush_Style;
        nsIPresShell* shell = doc->GetShell();
        if (shell) {
          
          nsPresContext* presContext = shell->GetPresContext();
          if (presContext && presContext->GetUserFontSet()) {
            flushType = Flush_Layout;
          }
        }
        mDontFlushLayout = mIsFlushingLayout = true;
        doc->FlushPendingNotifications(flushType);
        mDontFlushLayout = mIsFlushingLayout = false;
      }
    }

    
    
    if (!IsBusy()) {
      
      
      ClearInternalProgress();

      PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
             ("DocLoader:%p: Is now idle...\n", this));

      nsCOMPtr<nsIRequest> docRequest = mDocumentRequest;

      NS_ASSERTION(mDocumentRequest, "No Document Request!");
      mDocumentRequest = 0;
      mIsLoadingDocument = false;

      
      mProgressStateFlags = nsIWebProgressListener::STATE_STOP;


      nsresult loadGroupStatus = NS_OK;
      mLoadGroup->GetStatus(&loadGroupStatus);

      
      
      
      
      mLoadGroup->SetDefaultLoadRequest(nullptr);

      
      
      nsRefPtr<nsDocLoader> parent = mParent;

      
      
      
      if (!parent || parent->ChildEnteringOnload(this)) {
        
        
        
        
        doStopDocumentLoad(docRequest, loadGroupStatus);

        if (parent) {
          parent->ChildDoneWithOnload(this);
        }
      }
    }
  }
}

void nsDocLoader::doStartDocumentLoad(void)
{

#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(mDocumentRequest, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: ++ Firing OnStateChange for start document load (...)."
          "\tURI: %s \n",
          this, buffer.get()));
#endif 

  
  
  
  FireOnStateChange(this,
                    mDocumentRequest,
                    nsIWebProgressListener::STATE_START |
                    nsIWebProgressListener::STATE_IS_DOCUMENT |
                    nsIWebProgressListener::STATE_IS_REQUEST |
                    nsIWebProgressListener::STATE_IS_WINDOW |
                    nsIWebProgressListener::STATE_IS_NETWORK,
                    NS_OK);
}

void nsDocLoader::doStartURLLoad(nsIRequest *request)
{
#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(request, buffer);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
          ("DocLoader:%p: ++ Firing OnStateChange start url load (...)."
           "\tURI: %s\n",
            this, buffer.get()));
#endif 

  FireOnStateChange(this,
                    request,
                    nsIWebProgressListener::STATE_START |
                    nsIWebProgressListener::STATE_IS_REQUEST,
                    NS_OK);
}

void nsDocLoader::doStopURLLoad(nsIRequest *request, nsresult aStatus)
{
#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(request, buffer);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
          ("DocLoader:%p: ++ Firing OnStateChange for end url load (...)."
           "\tURI: %s status=%x\n",
            this, buffer.get(), aStatus));
#endif 

  FireOnStateChange(this,
                    request,
                    nsIWebProgressListener::STATE_STOP |
                    nsIWebProgressListener::STATE_IS_REQUEST,
                    aStatus);

  
  
  if (!mStatusInfoList.isEmpty()) {
    nsStatusInfo* statusInfo = mStatusInfoList.getFirst();
    FireOnStatusChange(this, statusInfo->mRequest,
                       statusInfo->mStatusCode,
                       statusInfo->mStatusMessage.get());
  }
}

void nsDocLoader::doStopDocumentLoad(nsIRequest *request,
                                         nsresult aStatus)
{
#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(request, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: ++ Firing OnStateChange for end document load (...)."
         "\tURI: %s Status=%x\n",
          this, buffer.get(), aStatus));
#endif 

  
  
  
  
  WebProgressList list;
  GatherAncestorWebProgresses(list);

  
  
  
  
  int32_t flags = nsIWebProgressListener::STATE_STOP |
                  nsIWebProgressListener::STATE_IS_DOCUMENT;
  for (uint32_t i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(this, request, flags, aStatus);
  }

  
  
  
  
  flags = nsIWebProgressListener::STATE_STOP |
          nsIWebProgressListener::STATE_IS_WINDOW |
          nsIWebProgressListener::STATE_IS_NETWORK;
  for (uint32_t i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(this, request, flags, aStatus);
  }
}





NS_IMETHODIMP
nsDocLoader::AddProgressListener(nsIWebProgressListener *aListener,
                                 uint32_t aNotifyMask)
{
  if (mListenerInfoList.Contains(aListener)) {
    
    return NS_ERROR_FAILURE;
  }

  nsWeakPtr listener = do_GetWeakReference(aListener);
  if (!listener) {
    return NS_ERROR_INVALID_ARG;
  }

  return mListenerInfoList.AppendElement(nsListenerInfo(listener, aNotifyMask)) ?
         NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsDocLoader::RemoveProgressListener(nsIWebProgressListener *aListener)
{
  return mListenerInfoList.RemoveElement(aListener) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocLoader::GetDOMWindow(nsIDOMWindow **aResult)
{
  return CallGetInterface(this, aResult);
}

NS_IMETHODIMP
nsDocLoader::GetDOMWindowID(uint64_t *aResult)
{
  *aResult = 0;

  nsCOMPtr<nsIDOMWindow> window;
  nsresult rv = GetDOMWindow(getter_AddRefs(window));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(window);
  NS_ENSURE_STATE(piwindow);

  MOZ_ASSERT(piwindow->IsOuterWindow());
  *aResult = piwindow->WindowID();
  return NS_OK;
}

NS_IMETHODIMP
nsDocLoader::GetIsTopLevel(bool *aResult)
{
  *aResult = false;

  nsCOMPtr<nsIDOMWindow> window;
  GetDOMWindow(getter_AddRefs(window));
  if (window) {
    nsCOMPtr<nsPIDOMWindow> piwindow = do_QueryInterface(window);
    NS_ENSURE_STATE(piwindow);

    nsCOMPtr<nsIDOMWindow> topWindow;
    nsresult rv = piwindow->GetTop(getter_AddRefs(topWindow));
    NS_ENSURE_SUCCESS(rv, rv);

    *aResult = piwindow == topWindow;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocLoader::GetIsLoadingDocument(bool *aIsLoadingDocument)
{
  *aIsLoadingDocument = mIsLoadingDocument;

  return NS_OK;
}

NS_IMETHODIMP
nsDocLoader::GetLoadType(uint32_t *aLoadType)
{
  *aLoadType = 0;

  return NS_ERROR_NOT_IMPLEMENTED;
}

int64_t nsDocLoader::GetMaxTotalProgress()
{
  int64_t newMaxTotal = 0;

  uint32_t count = mChildList.Length();
  nsCOMPtr<nsIWebProgress> webProgress;
  for (uint32_t i=0; i < count; i++)
  {
    int64_t individualProgress = 0;
    nsIDocumentLoader* docloader = ChildAt(i);
    if (docloader)
    {
      
      individualProgress = ((nsDocLoader *) docloader)->GetMaxTotalProgress();
    }
    if (individualProgress < int64_t(0)) 
                                         
    {
       newMaxTotal = int64_t(-1);
       break;
    }
    else
     newMaxTotal += individualProgress;
  }

  int64_t progress = -1;
  if (mMaxSelfProgress >= int64_t(0) && newMaxTotal >= int64_t(0))
    progress = newMaxTotal + mMaxSelfProgress;

  return progress;
}







NS_IMETHODIMP nsDocLoader::OnProgress(nsIRequest *aRequest, nsISupports* ctxt,
                                      int64_t aProgress, int64_t aProgressMax)
{
  int64_t progressDelta = 0;

  
  
  
  if (nsRequestInfo* info = GetRequestInfo(aRequest)) {
    
    
    int64_t oldCurrentProgress = info->mCurrentProgress;
    progressDelta = aProgress - oldCurrentProgress;
    info->mCurrentProgress = aProgress;

    
    if (!info->mUploading && (int64_t(0) == oldCurrentProgress) && (int64_t(0) == info->mMaxProgress)) {
      
      
      
      
      
      
      nsLoadFlags lf = 0;
      aRequest->GetLoadFlags(&lf);
      if ((lf & nsIChannel::LOAD_DOCUMENT_URI) && !(lf & nsIChannel::LOAD_TARGETED)) {
        PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
            ("DocLoader:%p Ignoring OnProgress while load is not targeted\n", this));
        return NS_OK;
      }

      
      
      
      
      
      
      if (aProgressMax != -1) {
        mMaxSelfProgress  += aProgressMax;
        info->mMaxProgress = aProgressMax;
      } else {
        mMaxSelfProgress   =  int64_t(-1);
        info->mMaxProgress =  int64_t(-1);
      }

      
      int32_t flags;

      flags = nsIWebProgressListener::STATE_TRANSFERRING |
              nsIWebProgressListener::STATE_IS_REQUEST;
      
      
      
      if (mProgressStateFlags & nsIWebProgressListener::STATE_START) {
        mProgressStateFlags = nsIWebProgressListener::STATE_TRANSFERRING;

        
        flags |= nsIWebProgressListener::STATE_IS_DOCUMENT;
      }

      FireOnStateChange(this, aRequest, flags, NS_OK);
    }

    
    mCurrentSelfProgress += progressDelta;
  }
  
  
  
  
  else {
#if defined(DEBUG)
    nsAutoCString buffer;

    GetURIStringFromRequest(aRequest, buffer);
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
           ("DocLoader:%p OOPS - No Request Info for: %s\n",
            this, buffer.get()));
#endif 

    return NS_OK;
  }

  
  
  
  FireOnProgressChange(this, aRequest, aProgress, aProgressMax, progressDelta,
                       mCurrentTotalProgress, mMaxTotalProgress);

  return NS_OK;
}

NS_IMETHODIMP nsDocLoader::OnStatus(nsIRequest* aRequest, nsISupports* ctxt,
                                        nsresult aStatus, const char16_t* aStatusArg)
{
  
  
  
  if (aStatus != NS_OK) {
    
    nsRequestInfo *info;
    info = GetRequestInfo(aRequest);
    if (info) {
      bool uploading = (aStatus == NS_NET_STATUS_WRITING ||
                        aStatus == NS_NET_STATUS_SENDING_TO);
      
      
      
      
      if (info->mUploading != uploading) {
        mCurrentSelfProgress  = mMaxSelfProgress  = 0;
        mCurrentTotalProgress = mMaxTotalProgress = 0;
        mCompletedTotalProgress = 0;
        info->mUploading = uploading;
        info->mCurrentProgress = 0;
        info->mMaxProgress = 0;
      }
    }

    nsCOMPtr<nsIStringBundleService> sbs =
      mozilla::services::GetStringBundleService();
    if (!sbs)
      return NS_ERROR_FAILURE;
    nsXPIDLString msg;
    nsresult rv = sbs->FormatStatusMessage(aStatus, aStatusArg,
                                           getter_Copies(msg));
    if (NS_FAILED(rv))
      return rv;

    
    
    
    
    if (info) {
      if (!info->mLastStatus) {
        info->mLastStatus = new nsStatusInfo(aRequest);
      } else {
        
        
        info->mLastStatus->remove();
      }
      info->mLastStatus->mStatusMessage = msg;
      info->mLastStatus->mStatusCode = aStatus;
      
      mStatusInfoList.insertFront(info->mLastStatus);
    }
    FireOnStatusChange(this, aRequest, aStatus, msg);
  }
  return NS_OK;
}

void nsDocLoader::ClearInternalProgress()
{
  ClearRequestInfoHash();

  mCurrentSelfProgress  = mMaxSelfProgress  = 0;
  mCurrentTotalProgress = mMaxTotalProgress = 0;
  mCompletedTotalProgress = 0;

  mProgressStateFlags = nsIWebProgressListener::STATE_STOP;
}





#define NOTIFY_LISTENERS(_flag, _code)                     \
PR_BEGIN_MACRO                                             \
  nsCOMPtr<nsIWebProgressListener> listener;               \
  ListenerArray::BackwardIterator iter(mListenerInfoList); \
  while (iter.HasMore()) {                                 \
    nsListenerInfo &info = iter.GetNext();                 \
    if (!(info.mNotifyMask & (_flag))) {                   \
      continue;                                            \
    }                                                      \
    listener = do_QueryReferent(info.mWeakListener);       \
    if (!listener) {                                       \
      iter.Remove();                                       \
      continue;                                            \
    }                                                      \
    _code                                                  \
  }                                                        \
  mListenerInfoList.Compact();                             \
PR_END_MACRO

void nsDocLoader::FireOnProgressChange(nsDocLoader *aLoadInitiator,
                                       nsIRequest *request,
                                       int64_t aProgress,
                                       int64_t aProgressMax,
                                       int64_t aProgressDelta,
                                       int64_t aTotalProgress,
                                       int64_t aMaxTotalProgress)
{
  if (mIsLoadingDocument) {
    mCurrentTotalProgress += aProgressDelta;
    mMaxTotalProgress = GetMaxTotalProgress();

    aTotalProgress    = mCurrentTotalProgress;
    aMaxTotalProgress = mMaxTotalProgress;
  }

#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(request, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: Progress (%s): curSelf: %d maxSelf: %d curTotal: %d maxTotal %d\n",
          this, buffer.get(), aProgress, aProgressMax, aTotalProgress, aMaxTotalProgress));
#endif 

  NOTIFY_LISTENERS(nsIWebProgress::NOTIFY_PROGRESS,
    
    listener->OnProgressChange(aLoadInitiator,request,
                               int32_t(aProgress), int32_t(aProgressMax),
                               int32_t(aTotalProgress), int32_t(aMaxTotalProgress));
  );

  
  if (mParent) {
    mParent->FireOnProgressChange(aLoadInitiator, request,
                                  aProgress, aProgressMax,
                                  aProgressDelta,
                                  aTotalProgress, aMaxTotalProgress);
  }
}

void nsDocLoader::GatherAncestorWebProgresses(WebProgressList& aList)
{
  for (nsDocLoader* loader = this; loader; loader = loader->mParent) {
    aList.AppendElement(loader);
  }
}

void nsDocLoader::FireOnStateChange(nsIWebProgress *aProgress,
                                    nsIRequest *aRequest,
                                    int32_t aStateFlags,
                                    nsresult aStatus)
{
  WebProgressList list;
  GatherAncestorWebProgresses(list);
  for (uint32_t i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(aProgress, aRequest, aStateFlags, aStatus);
  }
}

void nsDocLoader::DoFireOnStateChange(nsIWebProgress * const aProgress,
                                      nsIRequest * const aRequest,
                                      int32_t &aStateFlags,
                                      const nsresult aStatus)
{
  
  
  
  
  
  
  
  if (mIsLoadingDocument &&
      (aStateFlags & nsIWebProgressListener::STATE_IS_NETWORK) &&
      (this != aProgress)) {
    aStateFlags &= ~nsIWebProgressListener::STATE_IS_NETWORK;
  }

  
  if (mIsRestoringDocument)
    aStateFlags |= nsIWebProgressListener::STATE_RESTORING;

#if defined(DEBUG)
  nsAutoCString buffer;

  GetURIStringFromRequest(aRequest, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: Status (%s): code: %x\n",
         this, buffer.get(), aStateFlags));
#endif 

  NS_ASSERTION(aRequest, "Firing OnStateChange(...) notification with a NULL request!");

  NOTIFY_LISTENERS(((aStateFlags >> 16) & nsIWebProgress::NOTIFY_STATE_ALL),
    listener->OnStateChange(aProgress, aRequest, aStateFlags, aStatus);
  );
}



void
nsDocLoader::FireOnLocationChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsIURI *aUri,
                                  uint32_t aFlags)
{
  NOTIFY_LISTENERS(nsIWebProgress::NOTIFY_LOCATION,
    PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, ("DocLoader [%p] calling %p->OnLocationChange", this, listener.get()));
    listener->OnLocationChange(aWebProgress, aRequest, aUri, aFlags);
  );

  
  if (mParent) {
    mParent->FireOnLocationChange(aWebProgress, aRequest, aUri, aFlags);
  }
}

void
nsDocLoader::FireOnStatusChange(nsIWebProgress* aWebProgress,
                                nsIRequest* aRequest,
                                nsresult aStatus,
                                const char16_t* aMessage)
{
  NOTIFY_LISTENERS(nsIWebProgress::NOTIFY_STATUS,
    listener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  );

  
  if (mParent) {
    mParent->FireOnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  }
}

bool
nsDocLoader::RefreshAttempted(nsIWebProgress* aWebProgress,
                              nsIURI *aURI,
                              int32_t aDelay,
                              bool aSameURI)
{
  



  bool allowRefresh = true;

  NOTIFY_LISTENERS(nsIWebProgress::NOTIFY_REFRESH,
    nsCOMPtr<nsIWebProgressListener2> listener2 =
      do_QueryReferent(info.mWeakListener);
    if (!listener2)
      continue;

    bool listenerAllowedRefresh;
    nsresult listenerRV = listener2->OnRefreshAttempted(
        aWebProgress, aURI, aDelay, aSameURI, &listenerAllowedRefresh);
    if (NS_FAILED(listenerRV))
      continue;

    allowRefresh = allowRefresh && listenerAllowedRefresh;
  );

  
  if (mParent) {
    allowRefresh = allowRefresh &&
      mParent->RefreshAttempted(aWebProgress, aURI, aDelay, aSameURI);
  }

  return allowRefresh;
}

nsresult nsDocLoader::AddRequestInfo(nsIRequest *aRequest)
{
  if (!PL_DHashTableAdd(&mRequestInfoHash, aRequest, mozilla::fallible)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void nsDocLoader::RemoveRequestInfo(nsIRequest *aRequest)
{
  PL_DHashTableRemove(&mRequestInfoHash, aRequest);
}

nsDocLoader::nsRequestInfo* nsDocLoader::GetRequestInfo(nsIRequest* aRequest)
{
  return static_cast<nsRequestInfo*>
                    (PL_DHashTableSearch(&mRequestInfoHash, aRequest));
}



static PLDHashOperator
RemoveInfoCallback(PLDHashTable *table, PLDHashEntryHdr *hdr, uint32_t number,
                   void *arg)
{
  return PL_DHASH_REMOVE;
}

void nsDocLoader::ClearRequestInfoHash(void)
{
  if (!mRequestInfoHash.IsInitialized() || !mRequestInfoHash.EntryCount()) {
    

    return;
  }

  PL_DHashTableEnumerate(&mRequestInfoHash, RemoveInfoCallback, nullptr);
}


PLDHashOperator
nsDocLoader::CalcMaxProgressCallback(PLDHashTable* table, PLDHashEntryHdr* hdr,
                                     uint32_t number, void* arg)
{
  const nsRequestInfo* info = static_cast<const nsRequestInfo*>(hdr);
  int64_t* max = static_cast<int64_t* >(arg);

  if (info->mMaxProgress < info->mCurrentProgress) {
    *max = int64_t(-1);

    return PL_DHASH_STOP;
  }

  *max += info->mMaxProgress;

  return PL_DHASH_NEXT;
}

int64_t nsDocLoader::CalculateMaxProgress()
{
  int64_t max = mCompletedTotalProgress;
  PL_DHashTableEnumerate(&mRequestInfoHash, CalcMaxProgressCallback, &max);
  return max;
}

NS_IMETHODIMP nsDocLoader::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                                  nsIChannel *aNewChannel,
                                                  uint32_t aFlags,
                                                  nsIAsyncVerifyRedirectCallback *cb)
{
  if (aOldChannel)
  {
    nsLoadFlags loadFlags = 0;
    int32_t stateFlags = nsIWebProgressListener::STATE_REDIRECTING |
                         nsIWebProgressListener::STATE_IS_REQUEST;

    aOldChannel->GetLoadFlags(&loadFlags);
    
    
    if (loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
    {
      stateFlags |= nsIWebProgressListener::STATE_IS_DOCUMENT;

#if defined(DEBUG)
      nsCOMPtr<nsIRequest> request(do_QueryInterface(aOldChannel));
      NS_ASSERTION(request == mDocumentRequest, "Wrong Document Channel");
#endif 
    }

    OnRedirectStateChange(aOldChannel, aNewChannel, aFlags, stateFlags);
    FireOnStateChange(this, aOldChannel, stateFlags, NS_OK);
  }

  cb->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}





NS_IMETHODIMP nsDocLoader::OnSecurityChange(nsISupports * aContext,
                                            uint32_t aState)
{
  
  
  

  nsCOMPtr<nsIRequest> request = do_QueryInterface(aContext);
  nsIWebProgress* webProgress = static_cast<nsIWebProgress*>(this);

  NOTIFY_LISTENERS(nsIWebProgress::NOTIFY_SECURITY,
    listener->OnSecurityChange(webProgress, request, aState);
  );

  
  if (mParent) {
    mParent->OnSecurityChange(aContext, aState);
  }
  return NS_OK;
}










NS_IMETHODIMP nsDocLoader::GetPriority(int32_t *aPriority)
{
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    return p->GetPriority(aPriority);

  *aPriority = 0;
  return NS_OK;
}

NS_IMETHODIMP nsDocLoader::SetPriority(int32_t aPriority)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: SetPriority(%d) called\n", this, aPriority));

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    p->SetPriority(aPriority);

  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mChildList, nsDocLoader,
                                           SetPriority, (aPriority));

  return NS_OK;
}

NS_IMETHODIMP nsDocLoader::AdjustPriority(int32_t aDelta)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG,
         ("DocLoader:%p: AdjustPriority(%d) called\n", this, aDelta));

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    p->AdjustPriority(aDelta);

  NS_OBSERVER_ARRAY_NOTIFY_XPCOM_OBSERVERS(mChildList, nsDocLoader,
                                           AdjustPriority, (aDelta));

  return NS_OK;
}




#if 0
void nsDocLoader::DumpChannelInfo()
{
  nsChannelInfo *info;
  int32_t i, count;
  int32_t current=0, max=0;


  printf("==== DocLoader=%x\n", this);

  count = mChannelInfoList.Count();
  for(i=0; i<count; i++) {
    info = (nsChannelInfo *)mChannelInfoList.ElementAt(i);

#if defined(DEBUG)
    nsAutoCString buffer;
    nsresult rv = NS_OK;
    if (info->mURI) {
      rv = info->mURI->GetSpec(buffer);
    }

    printf("  [%d] current=%d  max=%d [%s]\n", i,
           info->mCurrentProgress,
           info->mMaxProgress, buffer.get());
#endif 

    current += info->mCurrentProgress;
    if (max >= 0) {
      if (info->mMaxProgress < info->mCurrentProgress) {
        max = -1;
      } else {
        max += info->mMaxProgress;
      }
    }
  }

  printf("\nCurrent=%d   Total=%d\n====\n", current, max);
}
#endif 
