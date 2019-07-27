




#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nspr.h"
#include "prlog.h"

#include "nsISecureBrowserUI.h"
#include "nsSecureBrowserUIImpl.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsCURILoader.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocument.h"
#include "nsIPrincipal.h"
#include "nsIDOMElement.h"
#include "nsPIDOMWindow.h"
#include "nsIContent.h"
#include "nsIWebProgress.h"
#include "nsIWebProgressListener.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIFileChannel.h"
#include "nsIWyciwygChannel.h"
#include "nsIFTPChannel.h"
#include "nsITransportSecurityInfo.h"
#include "nsISSLStatus.h"
#include "nsIURI.h"
#include "nsISecurityEventSink.h"
#include "nsIPrompt.h"
#include "nsIFormSubmitObserver.h"
#include "nsISecurityWarningDialogs.h"
#include "nsISecurityInfoProvider.h"
#include "imgIRequest.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsNetCID.h"
#include "nsCRT.h"

using namespace mozilla;

#if defined(PR_LOGGING)











PRLogModuleInfo* gSecureDocLog = nullptr;
#endif 

struct RequestHashEntry : PLDHashEntryHdr {
    void *r;
};

static bool
RequestMapMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const RequestHashEntry *entry = static_cast<const RequestHashEntry*>(hdr);
  return entry->r == key;
}

static bool
RequestMapInitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     const void *key)
{
  RequestHashEntry *entry = static_cast<RequestHashEntry*>(hdr);
  entry->r = (void*)key;
  return true;
}

static const PLDHashTableOps gMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RequestMapMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  RequestMapInitEntry
};

#ifdef DEBUG
class nsAutoAtomic {
  public:
    explicit nsAutoAtomic(Atomic<int32_t> &i)
    :mI(i) {
      mI++;
    }

    ~nsAutoAtomic() {
      mI--;
    }

  protected:
    Atomic<int32_t> &mI;

  private:
    nsAutoAtomic(); 
};
#endif

nsSecureBrowserUIImpl::nsSecureBrowserUIImpl()
  : mReentrantMonitor("nsSecureBrowserUIImpl.mReentrantMonitor")
  , mNotifiedSecurityState(lis_no_security)
  , mNotifiedToplevelIsEV(false)
  , mNewToplevelSecurityState(STATE_IS_INSECURE)
  , mNewToplevelIsEV(false)
  , mNewToplevelSecurityStateKnown(true)
  , mIsViewSource(false)
  , mSubRequestsBrokenSecurity(0)
  , mSubRequestsNoSecurity(0)
  , mRestoreSubrequests(false)
  , mOnLocationChangeSeen(false)
#ifdef DEBUG
  , mOnStateLocationChangeReentranceDetection(0)
#endif
{
  mTransferringRequests.ops = nullptr;
  ResetStateTracking();
  
#if defined(PR_LOGGING)
  if (!gSecureDocLog)
    gSecureDocLog = PR_NewLogModule("nsSecureBrowserUI");
#endif 
}

nsSecureBrowserUIImpl::~nsSecureBrowserUIImpl()
{
  if (mTransferringRequests.ops) {
    PL_DHashTableFinish(&mTransferringRequests);
    mTransferringRequests.ops = nullptr;
  }
}

NS_IMPL_ISUPPORTS(nsSecureBrowserUIImpl,
                  nsISecureBrowserUI,
                  nsIWebProgressListener,
                  nsIFormSubmitObserver,
                  nsISupportsWeakReference,
                  nsISSLStatusProvider)

NS_IMETHODIMP
nsSecureBrowserUIImpl::Init(nsIDOMWindow *aWindow)
{

#ifdef PR_LOGGING
  nsCOMPtr<nsIDOMWindow> window(do_QueryReferent(mWindow));

  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: Init: mWindow: %p, aWindow: %p\n", this,
          window.get(), aWindow));
#endif

  if (!aWindow) {
    NS_WARNING("Null window passed to nsSecureBrowserUIImpl::Init()");
    return NS_ERROR_INVALID_ARG;
  }

  if (mWindow) {
    NS_WARNING("Trying to init an nsSecureBrowserUIImpl twice");
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  nsCOMPtr<nsPIDOMWindow> pwin(do_QueryInterface(aWindow));
  if (pwin->IsInnerWindow()) {
    pwin = pwin->GetOuterWindow();
  }

  nsresult rv;
  mWindow = do_GetWeakReference(pwin, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsPIDOMWindow> piwindow(do_QueryInterface(aWindow));
  if (!piwindow) return NS_ERROR_FAILURE;

  nsIDocShell *docShell = piwindow->GetDocShell();

  
  if (!docShell)
    return NS_ERROR_FAILURE;

  docShell->SetSecurityUI(this);

  
  
  nsCOMPtr<nsIWebProgress> wp(do_GetInterface(docShell));
  if (!wp) return NS_ERROR_FAILURE;
  
  
  wp->AddProgressListener(static_cast<nsIWebProgressListener*>(this),
                          nsIWebProgress::NOTIFY_STATE_ALL | 
                          nsIWebProgress::NOTIFY_LOCATION  |
                          nsIWebProgress::NOTIFY_SECURITY);


  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::GetState(uint32_t* aState)
{
  ReentrantMonitorAutoEnter lock(mReentrantMonitor);
  return MapInternalToExternalState(aState, mNotifiedSecurityState, mNotifiedToplevelIsEV);
}


already_AddRefed<nsISupports> 
nsSecureBrowserUIImpl::ExtractSecurityInfo(nsIRequest* aRequest)
{
  nsCOMPtr<nsISupports> retval;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel)
    channel->GetSecurityInfo(getter_AddRefs(retval));
  
  if (!retval) {
    nsCOMPtr<nsISecurityInfoProvider> provider(do_QueryInterface(aRequest));
    if (provider)
      provider->GetSecurityInfo(getter_AddRefs(retval));
  }

  return retval.forget();
}

nsresult
nsSecureBrowserUIImpl::MapInternalToExternalState(uint32_t* aState, lockIconState lock, bool ev)
{
  NS_ENSURE_ARG(aState);

  switch (lock)
  {
    case lis_broken_security:
      *aState = STATE_IS_BROKEN;
      break;

    case lis_mixed_security:
      *aState = STATE_IS_BROKEN;
      break;

    case lis_high_security:
      *aState = STATE_IS_SECURE | STATE_SECURE_HIGH;
      break;

    default:
    case lis_no_security:
      *aState = STATE_IS_INSECURE;
      break;
  }

  if (ev && (*aState & STATE_IS_SECURE))
    *aState |= nsIWebProgressListener::STATE_IDENTITY_EV_TOPLEVEL;

  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocShell);
  if (!docShell)
    return NS_OK;

  
  if (docShell->ItemType() == nsIDocShellTreeItem::typeContent) {
    nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(docShell));
    nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
    docShellTreeItem->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
    NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");
    docShell = do_QueryInterface(sameTypeRoot);
    if (!docShell)
      return NS_OK;
  }

  
  
  
  if (docShell->GetHasMixedActiveContentLoaded() &&
      docShell->GetHasMixedDisplayContentLoaded()) {
      *aState = STATE_IS_BROKEN |
                nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT |
                nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT;
  } else if (docShell->GetHasMixedActiveContentLoaded()) {
      *aState = STATE_IS_BROKEN |
                nsIWebProgressListener::STATE_LOADED_MIXED_ACTIVE_CONTENT;
  } else if (docShell->GetHasMixedDisplayContentLoaded()) {
      *aState = STATE_IS_BROKEN |
                nsIWebProgressListener::STATE_LOADED_MIXED_DISPLAY_CONTENT;
  }

  
  if (docShell->GetHasMixedActiveContentBlocked())
    *aState |= nsIWebProgressListener::STATE_BLOCKED_MIXED_ACTIVE_CONTENT;

  if (docShell->GetHasMixedDisplayContentBlocked())
    *aState |= nsIWebProgressListener::STATE_BLOCKED_MIXED_DISPLAY_CONTENT;

  
  if (docShell->GetHasTrackingContentBlocked())
    *aState |= nsIWebProgressListener::STATE_BLOCKED_TRACKING_CONTENT;

  if (docShell->GetHasTrackingContentLoaded())
    *aState |= nsIWebProgressListener::STATE_LOADED_TRACKING_CONTENT;

  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::SetDocShell(nsIDocShell *aDocShell)
{
  nsresult rv;
  mDocShell = do_GetWeakReference(aDocShell, &rv);
  return rv;
}

static nsresult IsChildOfDomWindow(nsIDOMWindow *parent, nsIDOMWindow *child,
                                   bool* value)
{
  *value = false;
  
  if (parent == child) {
    *value = true;
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMWindow> childsParent;
  child->GetParent(getter_AddRefs(childsParent));
  
  if (childsParent && childsParent.get() != child)
    IsChildOfDomWindow(parent, childsParent, value);
  
  return NS_OK;
}

static uint32_t GetSecurityStateFromSecurityInfoAndRequest(nsISupports* info,
                                                           nsIRequest* request)
{
  nsresult res;
  uint32_t securityState;

  nsCOMPtr<nsITransportSecurityInfo> psmInfo(do_QueryInterface(info));
  if (!psmInfo) {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState: - no nsITransportSecurityInfo for %p\n",
                                         (nsISupports *)info));
    return nsIWebProgressListener::STATE_IS_INSECURE;
  }
  PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState: - info is %p\n", 
                                       (nsISupports *)info));
  
  res = psmInfo->GetSecurityState(&securityState);
  if (NS_FAILED(res)) {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState: - GetSecurityState failed: %d\n",
                                         res));
    securityState = nsIWebProgressListener::STATE_IS_BROKEN;
  }

  if (securityState != nsIWebProgressListener::STATE_IS_INSECURE) {
    
    

    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(request));
    if (channel) {
      channel->GetURI(getter_AddRefs(uri));
    } else {
      nsCOMPtr<imgIRequest> imgRequest(do_QueryInterface(request));
      if (imgRequest) {
        imgRequest->GetURI(getter_AddRefs(uri));
      }
    }
    if (uri) {
      bool isHttp, isFtp;
      if ((NS_SUCCEEDED(uri->SchemeIs("http", &isHttp)) && isHttp) ||
          (NS_SUCCEEDED(uri->SchemeIs("ftp", &isFtp)) && isFtp)) {
        PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState: - "
                                             "channel scheme is insecure.\n"));
        securityState = nsIWebProgressListener::STATE_IS_INSECURE;
      }
    }
  }

  PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState: - Returning %d\n", 
                                       securityState));
  return securityState;
}


NS_IMETHODIMP
nsSecureBrowserUIImpl::Notify(nsIDOMHTMLFormElement* aDOMForm,
                              nsIDOMWindow* aWindow, nsIURI* actionURL,
                              bool* cancelSubmit)
{
  
  *cancelSubmit = false;
  if (!aWindow || !actionURL || !aDOMForm)
    return NS_OK;
  
  nsCOMPtr<nsIContent> formNode = do_QueryInterface(aDOMForm);

  nsCOMPtr<nsIDocument> document = formNode->GetComposedDoc();
  if (!document) return NS_OK;

  nsIPrincipal *principal = formNode->NodePrincipal();
  
  if (!principal)
  {
    *cancelSubmit = true;
    return NS_OK;
  }

  nsCOMPtr<nsIURI> formURL;
  if (NS_FAILED(principal->GetURI(getter_AddRefs(formURL))) ||
      !formURL)
  {
    formURL = document->GetDocumentURI();
  }

  nsCOMPtr<nsIDOMWindow> postingWindow =
    do_QueryInterface(document->GetWindow());
  
  if (!postingWindow)
  {
    NS_WARNING("If you see this and can explain why it should be allowed, note in Bug 332324");
    *cancelSubmit = true;
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindow> window;
  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    window = do_QueryReferent(mWindow);

    
    if (!window)
      return NS_OK;
  }

  bool isChild;
  IsChildOfDomWindow(window, postingWindow, &isChild);
  
  
  if (!isChild)
    return NS_OK;
  
  bool okayToPost;
  nsresult res = CheckPost(formURL, actionURL, &okayToPost);
  
  if (NS_SUCCEEDED(res) && !okayToPost)
    *cancelSubmit = true;
  
  return res;
}


NS_IMETHODIMP 
nsSecureBrowserUIImpl::OnProgressChange(nsIWebProgress* aWebProgress,
                                        nsIRequest* aRequest,
                                        int32_t aCurSelfProgress,
                                        int32_t aMaxSelfProgress,
                                        int32_t aCurTotalProgress,
                                        int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

void nsSecureBrowserUIImpl::ResetStateTracking()
{
  ReentrantMonitorAutoEnter lock(mReentrantMonitor);

  mDocumentRequestsInProgress = 0;
  if (mTransferringRequests.ops) {
    PL_DHashTableFinish(&mTransferringRequests);
    mTransferringRequests.ops = nullptr;
  }
  PL_DHashTableInit(&mTransferringRequests, &gMapOps, nullptr,
                    sizeof(RequestHashEntry));
}

nsresult
nsSecureBrowserUIImpl::EvaluateAndUpdateSecurityState(nsIRequest* aRequest,
                                                      nsISupports *info,
                                                      bool withNewLocation,
                                                      bool withNewSink)
{
  



  uint32_t temp_NewToplevelSecurityState = nsIWebProgressListener::STATE_IS_INSECURE;
  bool temp_NewToplevelIsEV = false;

  bool updateStatus = false;
  nsCOMPtr<nsISSLStatus> temp_SSLStatus;

    temp_NewToplevelSecurityState =
      GetSecurityStateFromSecurityInfoAndRequest(info, aRequest);

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: remember mNewToplevelSecurityState => %x\n", this,
            temp_NewToplevelSecurityState));

    nsCOMPtr<nsISSLStatusProvider> sp = do_QueryInterface(info);
    if (sp) {
      
      updateStatus = true;
      (void) sp->GetSSLStatus(getter_AddRefs(temp_SSLStatus));
      if (temp_SSLStatus) {
        bool aTemp;
        if (NS_SUCCEEDED(temp_SSLStatus->GetIsExtendedValidation(&aTemp))) {
          temp_NewToplevelIsEV = aTemp;
        }
      }
    }

  
  

  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    mNewToplevelSecurityStateKnown = true;
    mNewToplevelSecurityState = temp_NewToplevelSecurityState;
    mNewToplevelIsEV = temp_NewToplevelIsEV;
    if (updateStatus) {
      mSSLStatus = temp_SSLStatus;
    }
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: remember securityInfo %p\n", this,
            info));
    nsCOMPtr<nsIAssociatedContentSecurity> associatedContentSecurityFromRequest =
        do_QueryInterface(aRequest);
    if (associatedContentSecurityFromRequest)
        mCurrentToplevelSecurityInfo = aRequest;
    else
        mCurrentToplevelSecurityInfo = info;

    
    
    
    mRestoreSubrequests = false;
  }

  return UpdateSecurityState(aRequest, withNewLocation,
                             withNewSink || updateStatus);
}

void
nsSecureBrowserUIImpl::UpdateSubrequestMembers(nsISupports* securityInfo,
                                               nsIRequest* request)
{
  
  
  uint32_t reqState = GetSecurityStateFromSecurityInfoAndRequest(securityInfo,
                                                                 request);

  
  ReentrantMonitorAutoEnter lock(mReentrantMonitor);

  if (reqState & STATE_IS_SECURE) {
    
  } else if (reqState & STATE_IS_BROKEN) {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: subreq BROKEN\n", this));
    ++mSubRequestsBrokenSecurity;
  } else {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: subreq INSECURE\n", this));
    ++mSubRequestsNoSecurity;
  }
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::OnStateChange(nsIWebProgress* aWebProgress,
                                     nsIRequest* aRequest,
                                     uint32_t aProgressStateFlags,
                                     nsresult aStatus)
{
#ifdef DEBUG
  nsAutoAtomic atomic(mOnStateLocationChangeReentranceDetection);
  NS_ASSERTION(mOnStateLocationChangeReentranceDetection == 1,
               "unexpected parallel nsIWebProgress OnStateChange and/or OnLocationChange notification");
#endif
  

























































































  nsCOMPtr<nsIDOMWindow> windowForProgress;
  aWebProgress->GetDOMWindow(getter_AddRefs(windowForProgress));

  nsCOMPtr<nsIDOMWindow> window;
  bool isViewSource;

  nsCOMPtr<nsINetUtil> ioService;

  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    window = do_QueryReferent(mWindow);
    NS_ASSERTION(window, "Window has gone away?!");
    isViewSource = mIsViewSource;
    ioService = mIOService;
  }

  if (!ioService)
  {
    ioService = do_GetService(NS_IOSERVICE_CONTRACTID);
    if (ioService)
    {
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);
      mIOService = ioService;
    }
  }

  bool isNoContentResponse = false;
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aRequest);
  if (httpChannel) 
  {
    uint32_t response;
    isNoContentResponse = NS_SUCCEEDED(httpChannel->GetResponseStatus(&response)) &&
        (response == 204 || response == 205);
  }
  const bool isToplevelProgress = (windowForProgress.get() == window.get()) && !isNoContentResponse;
  
#ifdef PR_LOGGING
  if (windowForProgress)
  {
    if (isToplevelProgress)
    {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: progress: for toplevel\n", this));
    }
    else
    {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: progress: for something else\n", this));
    }
  }
  else
  {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: progress: no window known\n", this));
  }
#endif

  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: OnStateChange\n", this));

  if (isViewSource)
    return NS_OK;

  if (!aRequest)
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange with null request\n", this));
    return NS_ERROR_NULL_POINTER;
  }

#ifdef PR_LOGGING
  if (PR_LOG_TEST(gSecureDocLog, PR_LOG_DEBUG)) {
    nsXPIDLCString reqname;
    aRequest->GetName(reqname);
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: %p %p OnStateChange %x %s\n", this, aWebProgress,
            aRequest, aProgressStateFlags, reqname.get()));
  }
#endif

  nsCOMPtr<nsISupports> securityInfo(ExtractSecurityInfo(aRequest));

  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (channel) {
    channel->GetURI(getter_AddRefs(uri));
  }

  nsCOMPtr<imgIRequest> imgRequest(do_QueryInterface(aRequest));
  if (imgRequest) {
    NS_ASSERTION(!channel, "How did that happen, exactly?");
    
    imgRequest->GetURI(getter_AddRefs(uri));
  }
  
  if (uri) {
    bool vs;
    if (NS_SUCCEEDED(uri->SchemeIs("javascript", &vs)) && vs) {
      
      
      return NS_OK;
    }
  }

  uint32_t loadFlags = 0;
  aRequest->GetLoadFlags(&loadFlags);

#ifdef PR_LOGGING
  if (aProgressStateFlags & STATE_START
      &&
      aProgressStateFlags & STATE_IS_REQUEST
      &&
      isToplevelProgress
      &&
      loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: SOMETHING STARTS FOR TOPMOST DOCUMENT\n", this));
  }

  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST
      &&
      isToplevelProgress
      &&
      loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: SOMETHING STOPS FOR TOPMOST DOCUMENT\n", this));
  }
#endif

  bool isSubDocumentRelevant = true;

  
  if (!imgRequest) { 
    nsCOMPtr<nsIHttpChannel> httpRequest(do_QueryInterface(aRequest));
    if (!httpRequest) {
      nsCOMPtr<nsIFileChannel> fileRequest(do_QueryInterface(aRequest));
      if (!fileRequest) {
        nsCOMPtr<nsIWyciwygChannel> wyciwygRequest(do_QueryInterface(aRequest));
        if (!wyciwygRequest) {
          nsCOMPtr<nsIFTPChannel> ftpRequest(do_QueryInterface(aRequest));
          if (!ftpRequest) {
            PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
                   ("SecureUI:%p: OnStateChange: not relevant for sub content\n", this));
            isSubDocumentRelevant = false;
          }
        }
      }
    }
  }

  
  
  if (uri && ioService) {
    bool hasFlag;
    nsresult rv = 
      ioService->URIChainHasFlags(uri, 
                                  nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                                  &hasFlag);
    if (NS_SUCCEEDED(rv) && hasFlag) {
      isSubDocumentRelevant = false;
    }
  }

#if defined(DEBUG)
  nsCString info2;
  uint32_t testFlags = loadFlags;

  if (testFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    testFlags -= nsIChannel::LOAD_DOCUMENT_URI;
    info2.AppendLiteral("LOAD_DOCUMENT_URI ");
  }
  if (testFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
  {
    testFlags -= nsIChannel::LOAD_RETARGETED_DOCUMENT_URI;
    info2.AppendLiteral("LOAD_RETARGETED_DOCUMENT_URI ");
  }
  if (testFlags & nsIChannel::LOAD_REPLACE)
  {
    testFlags -= nsIChannel::LOAD_REPLACE;
    info2.AppendLiteral("LOAD_REPLACE ");
  }

  const char *_status = NS_SUCCEEDED(aStatus) ? "1" : "0";

  nsCString info;
  uint32_t f = aProgressStateFlags;
  if (f & nsIWebProgressListener::STATE_START)
  {
    f -= nsIWebProgressListener::STATE_START;
    info.AppendLiteral("START ");
  }
  if (f & nsIWebProgressListener::STATE_REDIRECTING)
  {
    f -= nsIWebProgressListener::STATE_REDIRECTING;
    info.AppendLiteral("REDIRECTING ");
  }
  if (f & nsIWebProgressListener::STATE_TRANSFERRING)
  {
    f -= nsIWebProgressListener::STATE_TRANSFERRING;
    info.AppendLiteral("TRANSFERRING ");
  }
  if (f & nsIWebProgressListener::STATE_NEGOTIATING)
  {
    f -= nsIWebProgressListener::STATE_NEGOTIATING;
    info.AppendLiteral("NEGOTIATING ");
  }
  if (f & nsIWebProgressListener::STATE_STOP)
  {
    f -= nsIWebProgressListener::STATE_STOP;
    info.AppendLiteral("STOP ");
  }
  if (f & nsIWebProgressListener::STATE_IS_REQUEST)
  {
    f -= nsIWebProgressListener::STATE_IS_REQUEST;
    info.AppendLiteral("IS_REQUEST ");
  }
  if (f & nsIWebProgressListener::STATE_IS_DOCUMENT)
  {
    f -= nsIWebProgressListener::STATE_IS_DOCUMENT;
    info.AppendLiteral("IS_DOCUMENT ");
  }
  if (f & nsIWebProgressListener::STATE_IS_NETWORK)
  {
    f -= nsIWebProgressListener::STATE_IS_NETWORK;
    info.AppendLiteral("IS_NETWORK ");
  }
  if (f & nsIWebProgressListener::STATE_IS_WINDOW)
  {
    f -= nsIWebProgressListener::STATE_IS_WINDOW;
    info.AppendLiteral("IS_WINDOW ");
  }
  if (f & nsIWebProgressListener::STATE_IS_INSECURE)
  {
    f -= nsIWebProgressListener::STATE_IS_INSECURE;
    info.AppendLiteral("IS_INSECURE ");
  }
  if (f & nsIWebProgressListener::STATE_IS_BROKEN)
  {
    f -= nsIWebProgressListener::STATE_IS_BROKEN;
    info.AppendLiteral("IS_BROKEN ");
  }
  if (f & nsIWebProgressListener::STATE_IS_SECURE)
  {
    f -= nsIWebProgressListener::STATE_IS_SECURE;
    info.AppendLiteral("IS_SECURE ");
  }
  if (f & nsIWebProgressListener::STATE_SECURE_HIGH)
  {
    f -= nsIWebProgressListener::STATE_SECURE_HIGH;
    info.AppendLiteral("SECURE_HIGH ");
  }
  if (f & nsIWebProgressListener::STATE_RESTORING)
  {
    f -= nsIWebProgressListener::STATE_RESTORING;
    info.AppendLiteral("STATE_RESTORING ");
  }

  if (f > 0)
  {
    info.AppendLiteral("f contains unknown flag!");
  }

  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: OnStateChange: %s %s -- %s\n", this, _status, 
          info.get(), info2.get()));

  if (aProgressStateFlags & STATE_STOP
      &&
      channel)
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: seeing STOP with security state: %d\n", this,
            GetSecurityStateFromSecurityInfoAndRequest(securityInfo, aRequest)
            ));
  }
#endif

  if (aProgressStateFlags & STATE_TRANSFERRING
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    
    

    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_ADD);
    
    return NS_OK;
  }

  bool requestHasTransferedData = false;

  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    { 
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);
      PLDHashEntryHdr *entry = PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_LOOKUP);
      if (PL_DHASH_ENTRY_IS_BUSY(entry))
      {
        PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_REMOVE);

        requestHasTransferedData = true;
      }
    }

    if (!requestHasTransferedData) {
      
      
      
      nsCOMPtr<nsISecurityInfoProvider> securityInfoProvider =
        do_QueryInterface(aRequest);
      
      
      
      bool hasTransferred;
      requestHasTransferedData =
        securityInfoProvider &&
        (NS_FAILED(securityInfoProvider->GetHasTransferredData(&hasTransferred)) ||
         hasTransferred);
    }
  }

  bool allowSecurityStateChange = true;
  if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
  {
    
    
    
    allowSecurityStateChange = false;
  }

  if (aProgressStateFlags & STATE_START
      &&
      aProgressStateFlags & STATE_IS_REQUEST
      &&
      isToplevelProgress
      &&
      loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    bool inProgress;

    int32_t saveSubBroken;
    int32_t saveSubNo;
    nsCOMPtr<nsIAssociatedContentSecurity> prevContentSecurity;

    int32_t newSubBroken = 0;
    int32_t newSubNo = 0;

    {
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);
      inProgress = (mDocumentRequestsInProgress!=0);

      if (allowSecurityStateChange && !inProgress)
      {
        saveSubBroken = mSubRequestsBrokenSecurity;
        saveSubNo = mSubRequestsNoSecurity;
        prevContentSecurity = do_QueryInterface(mCurrentToplevelSecurityInfo);
      }
    }

    if (allowSecurityStateChange && !inProgress)
    {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: start for toplevel document\n", this
              ));

      if (prevContentSecurity)
      {
        PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
               ("SecureUI:%p: OnStateChange: start, saving current sub state\n", this
                ));
  
        
        
        prevContentSecurity->SetCountSubRequestsBrokenSecurity(saveSubBroken);
        prevContentSecurity->SetCountSubRequestsNoSecurity(saveSubNo);
        prevContentSecurity->Flush();
        PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI:%p: Saving subs in START to %p as %d,%d\n", 
          this, prevContentSecurity.get(), saveSubBroken, saveSubNo));      
      }

      bool retrieveAssociatedState = false;

      if (securityInfo &&
          (aProgressStateFlags & nsIWebProgressListener::STATE_RESTORING) != 0) {
        retrieveAssociatedState = true;
      } else {
        nsCOMPtr<nsIWyciwygChannel> wyciwygRequest(do_QueryInterface(aRequest));
        if (wyciwygRequest) {
          retrieveAssociatedState = true;
        }
      }

      if (retrieveAssociatedState)
      {
        
        
        
    
        nsCOMPtr<nsIAssociatedContentSecurity> 
          newContentSecurity(do_QueryInterface(securityInfo));
    
        if (newContentSecurity)
        {
          PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
                 ("SecureUI:%p: OnStateChange: start, loading old sub state\n", this
                  ));
    
          newContentSecurity->GetCountSubRequestsBrokenSecurity(&newSubBroken);
          newContentSecurity->GetCountSubRequestsNoSecurity(&newSubNo);
          PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI:%p: Restoring subs in START from %p to %d,%d\n", 
            this, newContentSecurity.get(), newSubBroken, newSubNo));      
        }
      }
      else
      {
        
        
        
        
        
        mRestoreSubrequests = true;
      }
    }

    {
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);

      if (allowSecurityStateChange && !inProgress)
      {
        ResetStateTracking();
        mSubRequestsBrokenSecurity = newSubBroken;
        mSubRequestsNoSecurity = newSubNo;
        mNewToplevelSecurityStateKnown = false;
      }

      
      
      
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: ++mDocumentRequestsInProgress\n", this
              ));
      ++mDocumentRequestsInProgress;
    }

    return NS_OK;
  }

  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST
      &&
      isToplevelProgress
      &&
      loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    int32_t temp_DocumentRequestsInProgress;
    nsCOMPtr<nsISecurityEventSink> temp_ToplevelEventSink;

    {
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);
      temp_DocumentRequestsInProgress = mDocumentRequestsInProgress;
      if (allowSecurityStateChange)
      {
        temp_ToplevelEventSink = mToplevelEventSink;
      }
    }

    if (temp_DocumentRequestsInProgress <= 0)
    {
      
      
      return NS_OK;
    }

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: --mDocumentRequestsInProgress\n", this
            ));

    if (!temp_ToplevelEventSink && channel)
    {
      if (allowSecurityStateChange)
      {
        ObtainEventSink(channel, temp_ToplevelEventSink);
      }
    }

    bool sinkChanged = false;
    bool inProgress;
    {
      ReentrantMonitorAutoEnter lock(mReentrantMonitor);
      if (allowSecurityStateChange)
      {
        sinkChanged = (mToplevelEventSink != temp_ToplevelEventSink);
        mToplevelEventSink = temp_ToplevelEventSink;
      }
      --mDocumentRequestsInProgress;
      inProgress = mDocumentRequestsInProgress > 0;
    }

    if (allowSecurityStateChange && requestHasTransferedData) {
      
      

      
      
      
      

      if (sinkChanged || mOnLocationChangeSeen)
        return EvaluateAndUpdateSecurityState(aRequest, securityInfo,
                                              false, sinkChanged);
    }
    mOnLocationChangeSeen = false;

    if (mRestoreSubrequests && !inProgress)
    {
      
      
      
      
      
      
      nsCOMPtr<nsIAssociatedContentSecurity> currentContentSecurity;
      {
        ReentrantMonitorAutoEnter lock(mReentrantMonitor);
        currentContentSecurity = do_QueryInterface(mCurrentToplevelSecurityInfo);

        
        
        mRestoreSubrequests = false;

        
        mNewToplevelSecurityStateKnown = true;
      }

      int32_t subBroken = 0;
      int32_t subNo = 0;

      if (currentContentSecurity)
      {
        currentContentSecurity->GetCountSubRequestsBrokenSecurity(&subBroken);
        currentContentSecurity->GetCountSubRequestsNoSecurity(&subNo);
        PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI:%p: Restoring subs in STOP from %p to %d,%d\n", 
          this, currentContentSecurity.get(), subBroken, subNo));      
      }

      {
        ReentrantMonitorAutoEnter lock(mReentrantMonitor);
        mSubRequestsBrokenSecurity = subBroken;
        mSubRequestsNoSecurity = subNo;
      }
    }
    
    return NS_OK;
  }
  
  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    if (!isSubDocumentRelevant)
      return NS_OK;
    
    
    
    

    if (allowSecurityStateChange && requestHasTransferedData)
    {  
      UpdateSubrequestMembers(securityInfo, aRequest);
      
      
      
      
      
      
      
      
      
      

      bool temp_NewToplevelSecurityStateKnown;
      {
        ReentrantMonitorAutoEnter lock(mReentrantMonitor);
        temp_NewToplevelSecurityStateKnown = mNewToplevelSecurityStateKnown;
      }

      if (temp_NewToplevelSecurityStateKnown)
        return UpdateSecurityState(aRequest, false, false);
    }

    return NS_OK;
  }

  return NS_OK;
}



void nsSecureBrowserUIImpl::ObtainEventSink(nsIChannel *channel, 
                                            nsCOMPtr<nsISecurityEventSink> &sink)
{
  if (!sink)
    NS_QueryNotificationCallbacks(channel, sink);
}

nsresult nsSecureBrowserUIImpl::UpdateSecurityState(nsIRequest* aRequest, 
                                                    bool withNewLocation, 
                                                    bool withUpdateStatus)
{
  lockIconState warnSecurityState = lis_no_security;
  nsresult rv = NS_OK;

  
  bool flagsChanged = UpdateMyFlags(warnSecurityState);

  if (flagsChanged || withNewLocation || withUpdateStatus)
    rv = TellTheWorld(warnSecurityState, aRequest);

  return rv;
}




bool nsSecureBrowserUIImpl::UpdateMyFlags(lockIconState &warnSecurityState)
{
  ReentrantMonitorAutoEnter lock(mReentrantMonitor);
  bool mustTellTheWorld = false;

  lockIconState newSecurityState;

  if (mNewToplevelSecurityState & STATE_IS_SECURE)
  {
    if (mSubRequestsBrokenSecurity
        ||
        mSubRequestsNoSecurity)
    {
      newSecurityState = lis_mixed_security;
    }
    else
    {
      newSecurityState = lis_high_security;
    }
  }
  else
  if (mNewToplevelSecurityState & STATE_IS_BROKEN)
  {
    
  
    newSecurityState = lis_broken_security;
  }
  else
  {
    newSecurityState = lis_no_security;
  }

  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: UpdateSecurityState:  old-new  %d - %d\n", this,
         mNotifiedSecurityState, newSecurityState
          ));

  if (mNotifiedSecurityState != newSecurityState)
  {
    mustTellTheWorld = true;

    

    









    mNotifiedSecurityState = newSecurityState;

    if (lis_no_security == newSecurityState)
    {
      mSSLStatus = nullptr;
    }
  }

  if (mNotifiedToplevelIsEV != mNewToplevelIsEV) {
    mustTellTheWorld = true;
    mNotifiedToplevelIsEV = mNewToplevelIsEV;
  }

  return mustTellTheWorld;
}

nsresult nsSecureBrowserUIImpl::TellTheWorld(lockIconState warnSecurityState, 
                                             nsIRequest* aRequest)
{
  nsCOMPtr<nsISecurityEventSink> temp_ToplevelEventSink;
  lockIconState temp_NotifiedSecurityState;
  bool temp_NotifiedToplevelIsEV;

  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    temp_ToplevelEventSink = mToplevelEventSink;
    temp_NotifiedSecurityState = mNotifiedSecurityState;
    temp_NotifiedToplevelIsEV = mNotifiedToplevelIsEV;
  }

  if (temp_ToplevelEventSink)
  {
    uint32_t newState = STATE_IS_INSECURE;
    MapInternalToExternalState(&newState, 
                               temp_NotifiedSecurityState, 
                               temp_NotifiedToplevelIsEV);

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: UpdateSecurityState: calling OnSecurityChange\n", this
            ));

    temp_ToplevelEventSink->OnSecurityChange(aRequest, newState);
  }
  else
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: UpdateSecurityState: NO mToplevelEventSink!\n", this
            ));

  }

  return NS_OK; 
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::OnLocationChange(nsIWebProgress* aWebProgress,
                                        nsIRequest* aRequest,
                                        nsIURI* aLocation,
                                        uint32_t aFlags)
{
#ifdef DEBUG
  nsAutoAtomic atomic(mOnStateLocationChangeReentranceDetection);
  NS_ASSERTION(mOnStateLocationChangeReentranceDetection == 1,
               "unexpected parallel nsIWebProgress OnStateChange and/or OnLocationChange notification");
#endif
  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: OnLocationChange\n", this));

  bool updateIsViewSource = false;
  bool temp_IsViewSource = false;
  nsCOMPtr<nsIDOMWindow> window;

  if (aLocation)
  {
    bool vs;

    nsresult rv = aLocation->SchemeIs("view-source", &vs);
    NS_ENSURE_SUCCESS(rv, rv);

    if (vs) {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnLocationChange: view-source\n", this));
    }

    updateIsViewSource = true;
    temp_IsViewSource = vs;
  }

  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    if (updateIsViewSource) {
      mIsViewSource = temp_IsViewSource;
    }
    mCurrentURI = aLocation;
    window = do_QueryReferent(mWindow);
    NS_ASSERTION(window, "Window has gone away?!");
  }

  
  
  
  if (aFlags & LOCATION_CHANGE_SAME_DOCUMENT)
    return NS_OK;

  
  
  
  
  
  
  

  nsCOMPtr<nsIDOMWindow> windowForProgress;
  aWebProgress->GetDOMWindow(getter_AddRefs(windowForProgress));

  nsCOMPtr<nsISupports> securityInfo(ExtractSecurityInfo(aRequest));

  if (windowForProgress.get() == window.get()) {
    
    mOnLocationChangeSeen = true;
    return EvaluateAndUpdateSecurityState(aRequest, securityInfo, true, false);
  }

  
  UpdateSubrequestMembers(securityInfo, aRequest);

  

  
  
  
  
  
  
  
  

  bool temp_NewToplevelSecurityStateKnown;
  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    temp_NewToplevelSecurityStateKnown = mNewToplevelSecurityStateKnown;
  }

  if (temp_NewToplevelSecurityStateKnown)
    return UpdateSecurityState(aRequest, true, false);

  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::OnStatusChange(nsIWebProgress* aWebProgress,
                                      nsIRequest* aRequest,
                                      nsresult aStatus,
                                      const char16_t* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

nsresult
nsSecureBrowserUIImpl::OnSecurityChange(nsIWebProgress *aWebProgress,
                                        nsIRequest *aRequest,
                                        uint32_t state)
{
#if defined(DEBUG)
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (!channel)
    return NS_OK;

  nsCOMPtr<nsIURI> aURI;
  channel->GetURI(getter_AddRefs(aURI));
  
  if (aURI) {
    nsAutoCString temp;
    aURI->GetSpec(temp);
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnSecurityChange: (%x) %s\n", this,
            state, temp.get()));
  }
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsSecureBrowserUIImpl::GetSSLStatus(nsISSLStatus** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  ReentrantMonitorAutoEnter lock(mReentrantMonitor);

  switch (mNotifiedSecurityState)
  {
    case lis_mixed_security:
    case lis_high_security:
      break;

    default:
      NS_NOTREACHED("if this is reached you must add more entries to the switch");
    case lis_no_security:
    case lis_broken_security:
      *_result = nullptr;
      return NS_OK;
  }
 
  *_result = mSSLStatus;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult
nsSecureBrowserUIImpl::IsURLHTTPS(nsIURI* aURL, bool* value)
{
  *value = false;

  if (!aURL)
    return NS_OK;

  return aURL->SchemeIs("https", value);
}

nsresult
nsSecureBrowserUIImpl::IsURLJavaScript(nsIURI* aURL, bool* value)
{
  *value = false;

  if (!aURL)
    return NS_OK;

  return aURL->SchemeIs("javascript", value);
}

nsresult
nsSecureBrowserUIImpl::CheckPost(nsIURI *formURL, nsIURI *actionURL, bool *okayToPost)
{
  bool formSecure, actionSecure, actionJavaScript;
  *okayToPost = true;

  nsresult rv = IsURLHTTPS(formURL, &formSecure);
  if (NS_FAILED(rv))
    return rv;

  rv = IsURLHTTPS(actionURL, &actionSecure);
  if (NS_FAILED(rv))
    return rv;

  rv = IsURLJavaScript(actionURL, &actionJavaScript);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  if (actionSecure) {
    return NS_OK;
  }

  
  if (actionJavaScript) {
    return NS_OK;
  }

  
  if (formSecure) {
    *okayToPost = ConfirmPostToInsecureFromSecure();
  }

  return NS_OK;
}





class nsUIContext : public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  explicit nsUIContext(nsIDOMWindow *window);

protected:
  virtual ~nsUIContext();

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
};

NS_IMPL_ISUPPORTS(nsUIContext, nsIInterfaceRequestor)

nsUIContext::nsUIContext(nsIDOMWindow *aWindow)
: mWindow(aWindow)
{
}

nsUIContext::~nsUIContext()
{
}


NS_IMETHODIMP nsUIContext::GetInterface(const nsIID & uuid, void * *result)
{
  NS_ENSURE_TRUE(mWindow, NS_ERROR_FAILURE);
  nsresult rv;

  if (uuid.Equals(NS_GET_IID(nsIPrompt))) {
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(mWindow, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIPrompt *prompt;

    rv = window->GetPrompter(&prompt);
    *result = prompt;
  } else if (uuid.Equals(NS_GET_IID(nsIDOMWindow))) {
    *result = mWindow;
    NS_ADDREF ((nsISupports*) *result);
    rv = NS_OK;
  } else {
    rv = NS_ERROR_NO_INTERFACE;
  }

  return rv;
}

bool
nsSecureBrowserUIImpl::GetNSSDialogs(nsCOMPtr<nsISecurityWarningDialogs> & dialogs,
                                     nsCOMPtr<nsIInterfaceRequestor> & ctx)
{
  if (!NS_IsMainThread()) {
    NS_ERROR("nsSecureBrowserUIImpl::GetNSSDialogs called off the main thread");
    return false;
  }

  dialogs = do_GetService(NS_SECURITYWARNINGDIALOGS_CONTRACTID);
  if (!dialogs)
    return false;

  nsCOMPtr<nsIDOMWindow> window;
  {
    ReentrantMonitorAutoEnter lock(mReentrantMonitor);
    window = do_QueryReferent(mWindow);
    NS_ASSERTION(window, "Window has gone away?!");
  }
  ctx = new nsUIContext(window);
  
  return true;
}






bool nsSecureBrowserUIImpl::
ConfirmPostToInsecureFromSecure()
{
  nsCOMPtr<nsISecurityWarningDialogs> dialogs;
  nsCOMPtr<nsIInterfaceRequestor> ctx;

  if (!GetNSSDialogs(dialogs, ctx)) {
    return false; 
  }

  bool result;

  nsresult rv = dialogs->ConfirmPostToInsecureFromSecure(ctx, &result);
  if (NS_FAILED(rv)) return false;

  return result;
}
