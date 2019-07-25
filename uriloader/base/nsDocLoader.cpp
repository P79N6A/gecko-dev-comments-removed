




































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











PRLogModuleInfo* gDocLoaderLog = nsnull;
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

struct nsStatusInfo : public PRCList
{
  nsString mStatusMessage;
  nsresult mStatusCode;
  
  nsIRequest * const mRequest;

  nsStatusInfo(nsIRequest *aRequest) :
    mRequest(aRequest)
  {
    MOZ_COUNT_CTOR(nsStatusInfo);
    PR_INIT_CLIST(this);
  }
  ~nsStatusInfo()
  {
    MOZ_COUNT_DTOR(nsStatusInfo);
    PR_REMOVE_LINK(this);
  }
};

struct nsRequestInfo : public PLDHashEntryHdr
{
  nsRequestInfo(const void *key)
    : mKey(key), mCurrentProgress(0), mMaxProgress(0), mUploading(false)
    , mLastStatus(nsnull)
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
  PRInt64 mCurrentProgress;
  PRInt64 mMaxProgress;
  bool mUploading;

  nsAutoPtr<nsStatusInfo> mLastStatus;
};


static bool
RequestInfoHashInitEntry(PLDHashTable *table, PLDHashEntryHdr *entry,
                         const void *key)
{
  
  new (entry) nsRequestInfo(key);
  return true;
}

static void
RequestInfoHashClearEntry(PLDHashTable *table, PLDHashEntryHdr *entry)
{
  nsRequestInfo* info = static_cast<nsRequestInfo *>(entry);
  info->~nsRequestInfo();
}

struct nsListenerInfo {
  nsListenerInfo(nsIWeakReference *aListener, unsigned long aNotifyMask) 
    : mWeakListener(aListener),
      mNotifyMask(aNotifyMask)
  {
  }

  
  nsWeakPtr mWeakListener;

  
  unsigned long mNotifyMask;
};


nsDocLoader::nsDocLoader()
  : mParent(nsnull),
    mListenerInfoList(8),
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
  if (nsnull == gDocLoaderLog) {
      gDocLoaderLog = PR_NewLogModule("DocLoader");
  }
#endif 

  static PLDHashTableOps hash_table_ops =
  {
    PL_DHashAllocTable,
    PL_DHashFreeTable,
    PL_DHashVoidPtrKeyStub,
    PL_DHashMatchEntryStub,
    PL_DHashMoveEntryStub,
    RequestInfoHashClearEntry,
    PL_DHashFinalizeStub,
    RequestInfoHashInitEntry
  };

  if (!PL_DHashTableInit(&mRequestInfoHash, &hash_table_ops, nsnull,
                         sizeof(nsRequestInfo), 16)) {
    mRequestInfoHash.ops = nsnull;
  }

  ClearInternalProgress();

  PR_INIT_CLIST(&mStatusInfoList);

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
  if (!mRequestInfoHash.ops) {
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

  if (mRequestInfoHash.ops) {
    PL_DHashTableFinish(&mRequestInfoHash);
  }
}





NS_IMPL_THREADSAFE_ADDREF(nsDocLoader)
NS_IMPL_THREADSAFE_RELEASE(nsDocLoader)

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
  if (!aSupports) {
    return nsnull;
  }
  
  nsDocLoader* ptr;
  CallQueryInterface(aSupports, &ptr);
  return ptr;
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
  PRInt32 count, i;

  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Stop() called\n", this));

  count = mChildList.Count();

  nsCOMPtr<nsIDocumentLoader> loader;
  for (i=0; i < count; i++) {
    loader = ChildAt(i);

    if (loader) {
      (void) loader->Stop();
    }
  }

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

  
  PRInt32 count, i;

  count = mChildList.Count();

  for (i=0; i < count; i++) {
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

  if (nsnull == aResult) {
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

  
  PRInt32 count = mListenerInfoList.Count();
  for(PRInt32 i = 0; i < count; i++) {
    nsListenerInfo *info =
      static_cast<nsListenerInfo*>(mListenerInfoList.ElementAt(i));

    delete info;
  }

  mListenerInfoList.Clear();
  mListenerInfoList.Compact();

  mDocumentRequest = 0;

  if (mLoadGroup)
    mLoadGroup->SetGroupObserver(nsnull);

  DestroyChildren();
}

void
nsDocLoader::DestroyChildren()
{
  PRInt32 i, count;
  
  count = mChildList.Count();
  
  
  
  for (i=0; i < count; i++)
  {
    nsIDocumentLoader* loader = ChildAt(i);

    if (loader) {
      
      
      static_cast<nsDocLoader*>(loader)->SetDocLoaderParent(nsnull);
    }
  }
  mChildList.Clear();
}

NS_IMETHODIMP
nsDocLoader::OnStartRequest(nsIRequest *request, nsISupports *aCtxt)
{
  

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gDocLoaderLog, PR_LOG_DEBUG)) {
    nsCAutoString name;
    request->GetName(name);

    PRUint32 count = 0;
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
    nsCAutoString name;
    aRequest->GetName(name);

    PRUint32 count = 0;
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
    
    
    
    info->mLastStatus = nsnull;

    PRInt64 oldMax = info->mMaxProgress;

    info->mMaxProgress = info->mCurrentProgress;
    
    
    
    
    
    
    if ((oldMax < PRInt64(0)) && (mMaxSelfProgress < PRInt64(0))) {
      mMaxSelfProgress = CalculateMaxProgress();
    }

    
    
    
    mCompletedTotalProgress += info->mMaxProgress;
    
    
    
    
    
    
    
    
    
    if ((oldMax == LL_ZERO) && (info->mCurrentProgress == LL_ZERO)) {
      nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));

      
      
      
      if (channel) {
        if (NS_SUCCEEDED(aStatus)) {
          bFireTransferring = true;
        }
        
        
        
        
        
        else if (aStatus != NS_BINDING_REDIRECTED &&
                 aStatus != NS_BINDING_RETARGETED) {
          
          
          
          PRUint32 lf;
          channel->GetLoadFlags(&lf);
          if (lf & nsIChannel::LOAD_TARGETED) {
            nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aRequest));
            if (httpChannel) {
              PRUint32 responseCode;
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
    
    PRInt32 flags;
    
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
    aChild->SetDocLoaderParent(nsnull);
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
    *aChannel = nsnull;
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

      
      
      
      
      mLoadGroup->SetDefaultLoadRequest(nsnull); 

      
      
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
  nsCAutoString buffer;

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
  nsCAutoString buffer;

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
  nsCAutoString buffer;

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

  
  
  if (!PR_CLIST_IS_EMPTY(&mStatusInfoList)) {
    nsStatusInfo* statusInfo =
      static_cast<nsStatusInfo*>(PR_LIST_HEAD(&mStatusInfoList));
    FireOnStatusChange(this, statusInfo->mRequest,
                       statusInfo->mStatusCode,
                       statusInfo->mStatusMessage.get());
  }
}

void nsDocLoader::doStopDocumentLoad(nsIRequest *request,
                                         nsresult aStatus)
{
#if defined(DEBUG)
  nsCAutoString buffer;

  GetURIStringFromRequest(request, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: ++ Firing OnStateChange for end document load (...)."
         "\tURI: %s Status=%x\n",
          this, buffer.get(), aStatus));
#endif 

  
  
  
  
  WebProgressList list;
  GatherAncestorWebProgresses(list);
  
  
  
  
  
  PRInt32 flags = nsIWebProgressListener::STATE_STOP |
                  nsIWebProgressListener::STATE_IS_DOCUMENT;
  for (PRUint32 i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(this, request, flags, aStatus);
  }

  
  
  
  
  flags = nsIWebProgressListener::STATE_STOP |
          nsIWebProgressListener::STATE_IS_WINDOW |
          nsIWebProgressListener::STATE_IS_NETWORK;
  for (PRUint32 i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(this, request, flags, aStatus);
  }
}





NS_IMETHODIMP
nsDocLoader::AddProgressListener(nsIWebProgressListener *aListener,
                                     PRUint32 aNotifyMask)
{
  nsresult rv;

  nsListenerInfo* info = GetListenerInfo(aListener);
  if (info) {
    
    return NS_ERROR_FAILURE;
  }

  nsWeakPtr listener = do_GetWeakReference(aListener);
  if (!listener) {
    return NS_ERROR_INVALID_ARG;
  }

  info = new nsListenerInfo(listener, aNotifyMask);
  if (!info) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  rv = mListenerInfoList.AppendElement(info) ? NS_OK : NS_ERROR_FAILURE;
  return rv;
}

NS_IMETHODIMP
nsDocLoader::RemoveProgressListener(nsIWebProgressListener *aListener)
{
  nsresult rv;

  nsListenerInfo* info = GetListenerInfo(aListener);
  if (info) {
    rv = mListenerInfoList.RemoveElement(info) ? NS_OK : NS_ERROR_FAILURE;
    delete info;
  } else {
    
    rv = NS_ERROR_FAILURE;
  }
  return rv;
}

NS_IMETHODIMP
nsDocLoader::GetDOMWindow(nsIDOMWindow **aResult)
{
  return CallGetInterface(this, aResult);
}

NS_IMETHODIMP
nsDocLoader::GetIsLoadingDocument(bool *aIsLoadingDocument)
{
  *aIsLoadingDocument = mIsLoadingDocument;

  return NS_OK;
}

PRInt64 nsDocLoader::GetMaxTotalProgress()
{
  PRInt64 newMaxTotal = 0;

  PRInt32 count = mChildList.Count();
  nsCOMPtr<nsIWebProgress> webProgress;
  for (PRInt32 i=0; i < count; i++) 
  {
    PRInt64 individualProgress = 0;
    nsIDocumentLoader* docloader = ChildAt(i);
    if (docloader)
    {
      
      individualProgress = ((nsDocLoader *) docloader)->GetMaxTotalProgress();
    }
    if (individualProgress < PRInt64(0)) 
                                         
    {
       newMaxTotal = PRInt64(-1);
       break;
    }
    else
     newMaxTotal += individualProgress;
  }

  PRInt64 progress = -1;
  if (mMaxSelfProgress >= PRInt64(0) && newMaxTotal >= PRInt64(0))
    progress = newMaxTotal + mMaxSelfProgress;
  
  return progress;
}







NS_IMETHODIMP nsDocLoader::OnProgress(nsIRequest *aRequest, nsISupports* ctxt, 
                                      PRUint64 aProgress, PRUint64 aProgressMax)
{
  nsRequestInfo *info;
  PRInt64 progressDelta = 0;

  
  
  
  info = GetRequestInfo(aRequest);
  if (info) {
    
    if (!info->mUploading && (PRInt64(0) == info->mCurrentProgress) && (PRInt64(0) == info->mMaxProgress)) {
      
      
      
      
      
      
      nsLoadFlags lf = 0;
      aRequest->GetLoadFlags(&lf);
      if ((lf & nsIChannel::LOAD_DOCUMENT_URI) && !(lf & nsIChannel::LOAD_TARGETED)) {
        PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
            ("DocLoader:%p Ignoring OnProgress while load is not targeted\n", this));
        return NS_OK;
      }

      
      
      
      
      
      
      if (PRUint64(aProgressMax) != LL_MAXUINT) {
        mMaxSelfProgress  += PRInt64(aProgressMax);
        info->mMaxProgress = PRInt64(aProgressMax);
      } else {
        mMaxSelfProgress   =  PRInt64(-1);
        info->mMaxProgress =  PRInt64(-1);
      }

      
      PRInt32 flags;
    
      flags = nsIWebProgressListener::STATE_TRANSFERRING | 
              nsIWebProgressListener::STATE_IS_REQUEST;
      
      
      
      if (mProgressStateFlags & nsIWebProgressListener::STATE_START) {
        mProgressStateFlags = nsIWebProgressListener::STATE_TRANSFERRING;

        
        flags |= nsIWebProgressListener::STATE_IS_DOCUMENT;
      }

      FireOnStateChange(this, aRequest, flags, NS_OK);
    }

    
    progressDelta = PRInt64(aProgress) - info->mCurrentProgress;
    mCurrentSelfProgress += progressDelta;

    info->mCurrentProgress = PRInt64(aProgress);
  }
  
  
  
  
  else {
#if defined(DEBUG)
    nsCAutoString buffer;

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
                                        nsresult aStatus, const PRUnichar* aStatusArg)
{
  
  
  
  if (aStatus) {
    
    nsRequestInfo *info;
    info = GetRequestInfo(aRequest);
    if (info) {
      bool uploading = (aStatus == nsITransport::STATUS_WRITING ||
                          aStatus == nsISocketTransport::STATUS_SENDING_TO);
      
      
      
      
      if (info->mUploading != uploading) {
        mCurrentSelfProgress  = mMaxSelfProgress  = LL_ZERO;
        mCurrentTotalProgress = mMaxTotalProgress = LL_ZERO;
        mCompletedTotalProgress = LL_ZERO;
        info->mUploading = uploading;
        info->mCurrentProgress = LL_ZERO;
        info->mMaxProgress = LL_ZERO;
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
        
        
        PR_REMOVE_LINK(info->mLastStatus);
      }
      info->mLastStatus->mStatusMessage = msg;
      info->mLastStatus->mStatusCode = aStatus;
      
      PR_INSERT_LINK(info->mLastStatus, &mStatusInfoList);
    }
    FireOnStatusChange(this, aRequest, aStatus, msg);
  }
  return NS_OK;
}

void nsDocLoader::ClearInternalProgress()
{
  ClearRequestInfoHash();

  mCurrentSelfProgress  = mMaxSelfProgress  = LL_ZERO;
  mCurrentTotalProgress = mMaxTotalProgress = LL_ZERO;
  mCompletedTotalProgress = LL_ZERO;

  mProgressStateFlags = nsIWebProgressListener::STATE_STOP;
}


void nsDocLoader::FireOnProgressChange(nsDocLoader *aLoadInitiator,
                                       nsIRequest *request,
                                       PRInt64 aProgress,
                                       PRInt64 aProgressMax,
                                       PRInt64 aProgressDelta,
                                       PRInt64 aTotalProgress,
                                       PRInt64 aMaxTotalProgress)
{
  if (mIsLoadingDocument) {
    mCurrentTotalProgress += aProgressDelta;
    mMaxTotalProgress = GetMaxTotalProgress();

    aTotalProgress    = mCurrentTotalProgress;
    aMaxTotalProgress = mMaxTotalProgress;
  }

#if defined(DEBUG)
  nsCAutoString buffer;

  GetURIStringFromRequest(request, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Progress (%s): curSelf: %d maxSelf: %d curTotal: %d maxTotal %d\n",
          this, buffer.get(), aProgress, aProgressMax, aTotalProgress, aMaxTotalProgress));
#endif 

  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRInt32 count = mListenerInfoList.Count();

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & nsIWebProgress::NOTIFY_PROGRESS)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    
    listener->OnProgressChange(aLoadInitiator,request,
                               PRInt32(aProgress), PRInt32(aProgressMax),
                               PRInt32(aTotalProgress), PRInt32(aMaxTotalProgress));
  }

  mListenerInfoList.Compact();

  
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
                                    PRInt32 aStateFlags,
                                    nsresult aStatus)
{
  WebProgressList list;
  GatherAncestorWebProgresses(list);
  for (PRUint32 i = 0; i < list.Length(); ++i) {
    list[i]->DoFireOnStateChange(aProgress, aRequest, aStateFlags, aStatus);
  }
}

void nsDocLoader::DoFireOnStateChange(nsIWebProgress * const aProgress,
                                      nsIRequest * const aRequest,
                                      PRInt32 &aStateFlags,
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
  nsCAutoString buffer;

  GetURIStringFromRequest(aRequest, buffer);
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: Status (%s): code: %x\n",
         this, buffer.get(), aStateFlags));
#endif 

  NS_ASSERTION(aRequest, "Firing OnStateChange(...) notification with a NULL request!");

  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRInt32 count = mListenerInfoList.Count();
  PRInt32 notifyMask = (aStateFlags >> 16) & nsIWebProgress::NOTIFY_STATE_ALL;

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & notifyMask)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    listener->OnStateChange(aProgress, aRequest, aStateFlags, aStatus);
  }

  mListenerInfoList.Compact();
}



void
nsDocLoader::FireOnLocationChange(nsIWebProgress* aWebProgress,
                                  nsIRequest* aRequest,
                                  nsIURI *aUri)
{
  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRInt32 count = mListenerInfoList.Count();

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & nsIWebProgress::NOTIFY_LOCATION)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    listener->OnLocationChange(aWebProgress, aRequest, aUri);
  }

  mListenerInfoList.Compact();

  
  if (mParent) {
    mParent->FireOnLocationChange(aWebProgress, aRequest, aUri);
  }
}

void
nsDocLoader::FireOnStatusChange(nsIWebProgress* aWebProgress,
                                nsIRequest* aRequest,
                                nsresult aStatus,
                                const PRUnichar* aMessage)
{
  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRInt32 count = mListenerInfoList.Count();

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & nsIWebProgress::NOTIFY_STATUS)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    listener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  }
  mListenerInfoList.Compact();
  
  
  if (mParent) {
    mParent->FireOnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
  }
}

bool
nsDocLoader::RefreshAttempted(nsIWebProgress* aWebProgress,
                              nsIURI *aURI,
                              PRInt32 aDelay,
                              bool aSameURI)
{
  








  bool allowRefresh = true;
  PRInt32 count = mListenerInfoList.Count();

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & nsIWebProgress::NOTIFY_REFRESH)) {
      continue;
    }

    nsCOMPtr<nsIWebProgressListener> listener =
      do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    nsCOMPtr<nsIWebProgressListener2> listener2 =
      do_QueryReferent(info->mWeakListener);
    if (!listener2)
      continue;

    bool listenerAllowedRefresh;
    nsresult listenerRV = listener2->OnRefreshAttempted(
        aWebProgress, aURI, aDelay, aSameURI, &listenerAllowedRefresh);
    if (NS_FAILED(listenerRV))
      continue;

    allowRefresh = allowRefresh && listenerAllowedRefresh;
  }

  mListenerInfoList.Compact();

  
  if (mParent) {
    allowRefresh = allowRefresh &&
      mParent->RefreshAttempted(aWebProgress, aURI, aDelay, aSameURI);
  }

  return allowRefresh;
}

nsListenerInfo * 
nsDocLoader::GetListenerInfo(nsIWebProgressListener *aListener)
{
  PRInt32 i, count;
  nsListenerInfo *info;

  nsCOMPtr<nsISupports> listener1 = do_QueryInterface(aListener);
  count = mListenerInfoList.Count();
  for (i=0; i<count; i++) {
    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(i));

    NS_ASSERTION(info, "There should NEVER be a null listener in the list");
    if (info) {
      nsCOMPtr<nsISupports> listener2 = do_QueryReferent(info->mWeakListener);
      if (listener1 == listener2)
        return info;
    }
  }
  return nsnull;
}

nsresult nsDocLoader::AddRequestInfo(nsIRequest *aRequest)
{
  if (!PL_DHashTableOperate(&mRequestInfoHash, aRequest, PL_DHASH_ADD)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void nsDocLoader::RemoveRequestInfo(nsIRequest *aRequest)
{
  PL_DHashTableOperate(&mRequestInfoHash, aRequest, PL_DHASH_REMOVE);
}

nsRequestInfo * nsDocLoader::GetRequestInfo(nsIRequest *aRequest)
{
  nsRequestInfo *info =
    static_cast<nsRequestInfo *>
               (PL_DHashTableOperate(&mRequestInfoHash, aRequest,
                                        PL_DHASH_LOOKUP));

  if (PL_DHASH_ENTRY_IS_FREE(info)) {
    

    return nsnull;
  }

  

  return info;
}



static PLDHashOperator
RemoveInfoCallback(PLDHashTable *table, PLDHashEntryHdr *hdr, PRUint32 number,
                   void *arg)
{
  return PL_DHASH_REMOVE;
}

void nsDocLoader::ClearRequestInfoHash(void)
{
  if (!mRequestInfoHash.ops || !mRequestInfoHash.entryCount) {
    

    return;
  }

  PL_DHashTableEnumerate(&mRequestInfoHash, RemoveInfoCallback, nsnull);
}


static PLDHashOperator
CalcMaxProgressCallback(PLDHashTable *table, PLDHashEntryHdr *hdr,
                        PRUint32 number, void *arg)
{
  const nsRequestInfo *info = static_cast<const nsRequestInfo *>(hdr);
  PRInt64 *max = static_cast<PRInt64 *>(arg);

  if (info->mMaxProgress < info->mCurrentProgress) {
    *max = PRInt64(-1);

    return PL_DHASH_STOP;
  }

  *max += info->mMaxProgress;

  return PL_DHASH_NEXT;
}

PRInt64 nsDocLoader::CalculateMaxProgress()
{
  PRInt64 max = mCompletedTotalProgress;
  PL_DHashTableEnumerate(&mRequestInfoHash, CalcMaxProgressCallback, &max);
  return max;
}

NS_IMETHODIMP nsDocLoader::AsyncOnChannelRedirect(nsIChannel *aOldChannel,
                                                  nsIChannel *aNewChannel,
                                                  PRUint32 aFlags,
                                                  nsIAsyncVerifyRedirectCallback *cb)
{
  if (aOldChannel)
  {
    nsLoadFlags loadFlags = 0;
    PRInt32 stateFlags = nsIWebProgressListener::STATE_REDIRECTING |
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
                                            PRUint32 aState)
{
  
  
  
  
  nsCOMPtr<nsIRequest> request = do_QueryInterface(aContext);
  nsIWebProgress* webProgress = static_cast<nsIWebProgress*>(this);

  





  nsCOMPtr<nsIWebProgressListener> listener;
  PRInt32 count = mListenerInfoList.Count();

  while (--count >= 0) {
    nsListenerInfo *info;

    info = static_cast<nsListenerInfo*>(mListenerInfoList.SafeElementAt(count));
    if (!info || !(info->mNotifyMask & nsIWebProgress::NOTIFY_SECURITY)) {
      continue;
    }

    listener = do_QueryReferent(info->mWeakListener);
    if (!listener) {
      
      mListenerInfoList.RemoveElementAt(count);
      delete info;
      continue;
    }

    listener->OnSecurityChange(webProgress, request, aState);
  }

  mListenerInfoList.Compact();

  
  if (mParent) {
    mParent->OnSecurityChange(aContext, aState);
  }
  return NS_OK;
}










NS_IMETHODIMP nsDocLoader::GetPriority(PRInt32 *aPriority)
{
  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    return p->GetPriority(aPriority);

  *aPriority = 0;
  return NS_OK;
}

NS_IMETHODIMP nsDocLoader::SetPriority(PRInt32 aPriority)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: SetPriority(%d) called\n", this, aPriority));

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    p->SetPriority(aPriority);

  PRInt32 count = mChildList.Count();

  nsDocLoader *loader;
  for (PRInt32 i=0; i < count; i++) {
    loader = static_cast<nsDocLoader*>(ChildAt(i));
    if (loader) {
      loader->SetPriority(aPriority);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocLoader::AdjustPriority(PRInt32 aDelta)
{
  PR_LOG(gDocLoaderLog, PR_LOG_DEBUG, 
         ("DocLoader:%p: AdjustPriority(%d) called\n", this, aDelta));

  nsCOMPtr<nsISupportsPriority> p = do_QueryInterface(mLoadGroup);
  if (p)
    p->AdjustPriority(aDelta);

  PRInt32 count = mChildList.Count();

  nsDocLoader *loader;
  for (PRInt32 i=0; i < count; i++) {
    loader = static_cast<nsDocLoader*>(ChildAt(i));
    if (loader) {
      loader->AdjustPriority(aDelta);
    }
  }

  return NS_OK;
}




#if 0
void nsDocLoader::DumpChannelInfo()
{
  nsChannelInfo *info;
  PRInt32 i, count;
  PRInt32 current=0, max=0;

  
  printf("==== DocLoader=%x\n", this);

  count = mChannelInfoList.Count();
  for(i=0; i<count; i++) {
    info = (nsChannelInfo *)mChannelInfoList.ElementAt(i);

#if defined(DEBUG)
    nsCAutoString buffer;
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
