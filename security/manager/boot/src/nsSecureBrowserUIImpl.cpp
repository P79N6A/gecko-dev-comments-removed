










































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif

#include "nspr.h"
#include "prlog.h"
#include "prmem.h"

#include "nsISecureBrowserUI.h"
#include "nsSecureBrowserUIImpl.h"
#include "nsCOMPtr.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsCURILoader.h"
#include "nsIDocShell.h"
#include "nsIDocumentViewer.h"
#include "nsIDocument.h"
#include "nsIPrincipal.h"
#include "nsIDOMElement.h"
#include "nsPIDOMWindow.h"
#include "nsIContent.h"
#include "nsIWebProgress.h"
#include "nsIChannel.h"
#include "nsIHttpChannel.h"
#include "nsIFileChannel.h"
#include "nsIWyciwygChannel.h"
#include "nsIFTPChannel.h"
#include "nsITransportSecurityInfo.h"
#include "nsIURI.h"
#include "nsISecurityEventSink.h"
#include "nsIPrompt.h"
#include "nsIFormSubmitObserver.h"
#include "nsISecurityWarningDialogs.h"
#include "nsIProxyObjectManager.h"
#include "nsThreadUtils.h"
#include "nsNetUtil.h"
#include "nsCRT.h"

#define SECURITY_STRING_BUNDLE_URL "chrome://pipnss/locale/security.properties"

#define IS_SECURE(state) ((state & 0xFFFF) == STATE_IS_SECURE)

#if defined(PR_LOGGING)











PRLogModuleInfo* gSecureDocLog = nsnull;
#endif 

struct RequestHashEntry : PLDHashEntryHdr {
    void *r;
};

PR_STATIC_CALLBACK(PRBool)
RequestMapMatchEntry(PLDHashTable *table, const PLDHashEntryHdr *hdr,
                         const void *key)
{
  const RequestHashEntry *entry = NS_STATIC_CAST(const RequestHashEntry*, hdr);
  return entry->r == key;
}

PR_STATIC_CALLBACK(PRBool)
RequestMapInitEntry(PLDHashTable *table, PLDHashEntryHdr *hdr,
                     const void *key)
{
  RequestHashEntry *entry = NS_STATIC_CAST(RequestHashEntry*, hdr);
  entry->r = (void*)key;
  return PR_TRUE;
}

static PLDHashTableOps gMapOps = {
  PL_DHashAllocTable,
  PL_DHashFreeTable,
  PL_DHashVoidPtrKeyStub,
  RequestMapMatchEntry,
  PL_DHashMoveEntryStub,
  PL_DHashClearEntryStub,
  PL_DHashFinalizeStub,
  RequestMapInitEntry
};


nsSecureBrowserUIImpl::nsSecureBrowserUIImpl()
  : mPreviousSecurityState(lis_no_security),
    mIsViewSource(PR_FALSE)
{
  mTransferringRequests.ops = nsnull;
  mNewToplevelSecurityState = STATE_IS_INSECURE;
  mNewToplevelSecurityStateKnown = PR_TRUE;
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
    mTransferringRequests.ops = nsnull;
  }
}

NS_IMPL_ISUPPORTS6(nsSecureBrowserUIImpl,
                   nsISecureBrowserUI,
                   nsIWebProgressListener,
                   nsIFormSubmitObserver,
                   nsIObserver,
                   nsISupportsWeakReference,
                   nsISSLStatusProvider)


NS_IMETHODIMP
nsSecureBrowserUIImpl::Init(nsIDOMWindow *window)
{
  PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
         ("SecureUI:%p: Init: mWindow: %p, window: %p\n", this, mWindow.get(),
          window));

  if (!window) {
    NS_WARNING("Null window passed to nsSecureBrowserUIImpl::Init()");
    return NS_ERROR_INVALID_ARG;
  }

  if (mWindow) {
    NS_WARNING("Trying to init an nsSecureBrowserUIImpl twice");
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  
  nsresult rv = NS_OK;
  mWindow = window;

  nsCOMPtr<nsIStringBundleService> service(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;
  
  
  
  
  
  
  service->CreateBundle(SECURITY_STRING_BUNDLE_URL, getter_AddRefs(mStringBundle));
  
  
  
  nsCOMPtr<nsIObserverService> svc(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = svc->AddObserver(this, NS_FORMSUBMIT_SUBJECT, PR_TRUE);
  }
  
  nsCOMPtr<nsPIDOMWindow> piwindow(do_QueryInterface(mWindow));
  if (!piwindow) return NS_ERROR_FAILURE;

  nsIDocShell *docShell = piwindow->GetDocShell();

  
  if (!docShell)
    return NS_ERROR_FAILURE;

  docShell->SetSecurityUI(this);

  
  
  nsCOMPtr<nsIWebProgress> wp(do_GetInterface(docShell));
  if (!wp) return NS_ERROR_FAILURE;
  
  
  wp->AddProgressListener(NS_STATIC_CAST(nsIWebProgressListener*,this),
                          nsIWebProgress::NOTIFY_STATE_ALL | 
                          nsIWebProgress::NOTIFY_LOCATION  |
                          nsIWebProgress::NOTIFY_SECURITY);


  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::GetState(PRUint32* aState)
{
  NS_ENSURE_ARG(aState);
  
  switch (mPreviousSecurityState)
  {
    case lis_broken_security:
      *aState = STATE_IS_BROKEN;
      break;

    case lis_mixed_security:
      *aState = STATE_IS_BROKEN;
      break;

    case lis_low_security:
      *aState = STATE_IS_SECURE | STATE_SECURE_LOW;
      break;

    case lis_high_security:
      *aState = STATE_IS_SECURE | STATE_SECURE_HIGH;
      break;

    default:
    case lis_no_security:
      *aState = STATE_IS_INSECURE;
      break;

  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::GetTooltipText(nsAString& aText)
{
  if (mPreviousSecurityState == lis_mixed_security)
  {
    GetBundleString(NS_LITERAL_STRING("SecurityButtonMixedContentTooltipText").get(),
                    aText);
  }
  else if (!mInfoTooltip.IsEmpty())
  {
    aText = mInfoTooltip;
  }
  else
  {
    GetBundleString(NS_LITERAL_STRING("SecurityButtonTooltipText").get(),
                    aText);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::Observe(nsISupports*, const char*,
                               const PRUnichar*)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


static nsresult IsChildOfDomWindow(nsIDOMWindow *parent, nsIDOMWindow *child,
                                   PRBool* value)
{
  *value = PR_FALSE;
  
  if (parent == child) {
    *value = PR_TRUE;
    return NS_OK;
  }
  
  nsCOMPtr<nsIDOMWindow> childsParent;
  child->GetParent(getter_AddRefs(childsParent));
  
  if (childsParent && childsParent.get() != child)
    IsChildOfDomWindow(parent, childsParent, value);
  
  return NS_OK;
}

static PRUint32 GetSecurityStateFromChannel(nsIChannel* aChannel)
{
  nsresult res;
  PRUint32 securityState;

  
  nsCOMPtr<nsISupports> info;
  aChannel->GetSecurityInfo(getter_AddRefs(info));
  nsCOMPtr<nsITransportSecurityInfo> psmInfo(do_QueryInterface(info));
  if (!psmInfo) {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState:%p - no nsITransportSecurityInfo for %p\n",
                                         aChannel, (nsISupports *)info));
    return nsIWebProgressListener::STATE_IS_INSECURE;
  }
  PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState:%p - info is %p\n", aChannel,
                                       (nsISupports *)info));
  
  res = psmInfo->GetSecurityState(&securityState);
  if (NS_FAILED(res)) {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState:%p - GetSecurityState failed: %d\n",
                                         aChannel, res));
    securityState = nsIWebProgressListener::STATE_IS_BROKEN;
  }
  
  PR_LOG(gSecureDocLog, PR_LOG_DEBUG, ("SecureUI: GetSecurityState:%p - Returning %d\n", aChannel,
                                       securityState));
  return securityState;
}


NS_IMETHODIMP
nsSecureBrowserUIImpl::Notify(nsIDOMHTMLFormElement* aDOMForm,
                              nsIDOMWindowInternal* window, nsIURI* actionURL,
                              PRBool* cancelSubmit)
{
  
  *cancelSubmit = PR_FALSE;
  if (!window || !actionURL || !aDOMForm)
    return NS_OK;
  
  nsCOMPtr<nsIContent> formNode = do_QueryInterface(aDOMForm);

  nsCOMPtr<nsIDocument> document = formNode->GetDocument();
  if (!document) return NS_OK;

  nsIPrincipal *principal = formNode->NodePrincipal();
  
  if (!principal)
  {
    *cancelSubmit = PR_TRUE;
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
    *cancelSubmit = PR_TRUE;
    return NS_OK;
  }

  PRBool isChild;
  IsChildOfDomWindow(mWindow, postingWindow, &isChild);
  
  
  if (!isChild)
    return NS_OK;
  
  PRBool okayToPost;
  nsresult res = CheckPost(formURL, actionURL, &okayToPost);
  
  if (NS_SUCCEEDED(res) && !okayToPost)
    *cancelSubmit = PR_TRUE;
  
  return res;
}


NS_IMETHODIMP 
nsSecureBrowserUIImpl::OnProgressChange(nsIWebProgress* aWebProgress,
                                        nsIRequest* aRequest,
                                        PRInt32 aCurSelfProgress,
                                        PRInt32 aMaxSelfProgress,
                                        PRInt32 aCurTotalProgress,
                                        PRInt32 aMaxTotalProgress)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

void nsSecureBrowserUIImpl::ResetStateTracking()
{
  mInfoTooltip.Truncate();
  mDocumentRequestsInProgress = 0;
  mSubRequestsHighSecurity = 0;
  mSubRequestsLowSecurity = 0;
  mSubRequestsBrokenSecurity = 0;
  mSubRequestsNoSecurity = 0;
  if (mTransferringRequests.ops) {
    PL_DHashTableFinish(&mTransferringRequests);
    mTransferringRequests.ops = nsnull;
  }
  PL_DHashTableInit(&mTransferringRequests, &gMapOps, nsnull,
                    sizeof(RequestHashEntry), 16);
}

nsresult
nsSecureBrowserUIImpl::EvaluateAndUpdateSecurityState(nsIRequest *aRequest)
{
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));

  if (!channel) {
    mNewToplevelSecurityState = nsIWebProgressListener::STATE_IS_INSECURE;
  } else {
    mNewToplevelSecurityState = GetSecurityStateFromChannel(channel);

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: remember mNewToplevelSecurityState => %x\n", this,
            mNewToplevelSecurityState));

    
    nsCOMPtr<nsISupports> info;
    channel->GetSecurityInfo(getter_AddRefs(info));
    nsCOMPtr<nsISSLStatusProvider> sp = do_QueryInterface(info);
    if (sp) {
      
      sp->GetSSLStatus(getter_AddRefs(mSSLStatus));
    }

    if (info) {
      nsCOMPtr<nsITransportSecurityInfo> secInfo(do_QueryInterface(info));
      if (secInfo) {
        secInfo->GetShortSecurityDescription(getter_Copies(mInfoTooltip));
      }
    }
  }

  
  

  mNewToplevelSecurityStateKnown = PR_TRUE;
  return UpdateSecurityState(aRequest);
}

void
nsSecureBrowserUIImpl::UpdateSubrequestMembers(nsIRequest *aRequest)
{
  
  
  PRUint32 reqState = nsIWebProgressListener::STATE_IS_INSECURE;
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));

  if (channel) {
    reqState = GetSecurityStateFromChannel(channel);
  }

  if (reqState & STATE_IS_SECURE) {
    if (reqState & STATE_SECURE_LOW || reqState & STATE_SECURE_MED) {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: subreq LOW\n", this));
      ++mSubRequestsLowSecurity;
    } else {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: subreq HIGH\n", this));
      ++mSubRequestsHighSecurity;
    }
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
                                     PRUint32 aProgressStateFlags,
                                     nsresult aStatus)
{
  

























































































  nsCOMPtr<nsIDOMWindow> windowForProgress;
  aWebProgress->GetDOMWindow(getter_AddRefs(windowForProgress));

  const PRBool isToplevelProgress = (windowForProgress.get() == mWindow.get());
  
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

  if (mIsViewSource)
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

  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));

  if (channel)
  {
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    if (uri)
    {
      PRBool vs;
      if (NS_SUCCEEDED(uri->SchemeIs("javascript", &vs)) && vs)
      {
        
        
        return NS_OK;
      }
    }
  }

  PRUint32 loadFlags = 0;
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

  PRBool isSubDocumentRelevant = PR_TRUE;

  
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
          isSubDocumentRelevant = PR_FALSE;
        }
      }
    }
  }

#if defined(DEBUG)
  nsCString info2;
  PRUint32 testFlags = loadFlags;

  if (testFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    testFlags -= nsIChannel::LOAD_DOCUMENT_URI;
    info2.Append("LOAD_DOCUMENT_URI ");
  }
  if (testFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
  {
    testFlags -= nsIChannel::LOAD_RETARGETED_DOCUMENT_URI;
    info2.Append("LOAD_RETARGETED_DOCUMENT_URI ");
  }
  if (testFlags & nsIChannel::LOAD_REPLACE)
  {
    testFlags -= nsIChannel::LOAD_REPLACE;
    info2.Append("LOAD_REPLACE ");
  }

  const char *_status = NS_SUCCEEDED(aStatus) ? "1" : "0";

  nsCString info;
  PRUint32 f = aProgressStateFlags;
  if (f & nsIWebProgressListener::STATE_START)
  {
    f -= nsIWebProgressListener::STATE_START;
    info.Append("START ");
  }
  if (f & nsIWebProgressListener::STATE_REDIRECTING)
  {
    f -= nsIWebProgressListener::STATE_REDIRECTING;
    info.Append("REDIRECTING ");
  }
  if (f & nsIWebProgressListener::STATE_TRANSFERRING)
  {
    f -= nsIWebProgressListener::STATE_TRANSFERRING;
    info.Append("TRANSFERRING ");
  }
  if (f & nsIWebProgressListener::STATE_NEGOTIATING)
  {
    f -= nsIWebProgressListener::STATE_NEGOTIATING;
    info.Append("NEGOTIATING ");
  }
  if (f & nsIWebProgressListener::STATE_STOP)
  {
    f -= nsIWebProgressListener::STATE_STOP;
    info.Append("STOP ");
  }
  if (f & nsIWebProgressListener::STATE_IS_REQUEST)
  {
    f -= nsIWebProgressListener::STATE_IS_REQUEST;
    info.Append("IS_REQUEST ");
  }
  if (f & nsIWebProgressListener::STATE_IS_DOCUMENT)
  {
    f -= nsIWebProgressListener::STATE_IS_DOCUMENT;
    info.Append("IS_DOCUMENT ");
  }
  if (f & nsIWebProgressListener::STATE_IS_NETWORK)
  {
    f -= nsIWebProgressListener::STATE_IS_NETWORK;
    info.Append("IS_NETWORK ");
  }
  if (f & nsIWebProgressListener::STATE_IS_WINDOW)
  {
    f -= nsIWebProgressListener::STATE_IS_WINDOW;
    info.Append("IS_WINDOW ");
  }
  if (f & nsIWebProgressListener::STATE_IS_INSECURE)
  {
    f -= nsIWebProgressListener::STATE_IS_INSECURE;
    info.Append("IS_INSECURE ");
  }
  if (f & nsIWebProgressListener::STATE_IS_BROKEN)
  {
    f -= nsIWebProgressListener::STATE_IS_BROKEN;
    info.Append("IS_BROKEN ");
  }
  if (f & nsIWebProgressListener::STATE_IS_SECURE)
  {
    f -= nsIWebProgressListener::STATE_IS_SECURE;
    info.Append("IS_SECURE ");
  }
  if (f & nsIWebProgressListener::STATE_SECURE_HIGH)
  {
    f -= nsIWebProgressListener::STATE_SECURE_HIGH;
    info.Append("SECURE_HIGH ");
  }
  if (f & nsIWebProgressListener::STATE_SECURE_MED)
  {
    f -= nsIWebProgressListener::STATE_SECURE_MED;
    info.Append("SECURE_MED ");
  }
  if (f & nsIWebProgressListener::STATE_SECURE_LOW)
  {
    f -= nsIWebProgressListener::STATE_SECURE_LOW;
    info.Append("SECURE_LOW ");
  }

  if (f > 0)
  {
    info.Append("f contains unknown flag!");
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
            GetSecurityStateFromChannel(channel)
            ));
  }
#endif

  if (aProgressStateFlags & STATE_TRANSFERRING
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    
    

    PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_ADD);
    
    return NS_OK;
  }

  PRBool requestHasTransferedData = PR_FALSE;

  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    PLDHashEntryHdr *entry = PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_LOOKUP);
    if (PL_DHASH_ENTRY_IS_BUSY(entry))
    {
      PL_DHashTableOperate(&mTransferringRequests, aRequest, PL_DHASH_REMOVE);

      requestHasTransferedData = PR_TRUE;
    }
  }

  if (loadFlags & nsIChannel::LOAD_RETARGETED_DOCUMENT_URI)
  {
    
    
    
    return NS_OK;
  }

  if (aProgressStateFlags & STATE_START
      &&
      aProgressStateFlags & STATE_IS_REQUEST
      &&
      isToplevelProgress
      &&
      loadFlags & nsIChannel::LOAD_DOCUMENT_URI)
  {
    if (!mDocumentRequestsInProgress)
    {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnStateChange: start for toplevel document\n", this
              ));

      ResetStateTracking();
      mNewToplevelSecurityStateKnown = PR_FALSE;
    }

    
    
    

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: ++mDocumentRequestsInProgress\n", this
            ));

    ++mDocumentRequestsInProgress;
    
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
    if (mDocumentRequestsInProgress <= 0)
    {
      
      
      return NS_OK;
    }

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnStateChange: --mDocumentRequestsInProgress\n", this
            ));

    if (!mToplevelEventSink && channel)
    {
      ObtainEventSink(channel);
    }

    --mDocumentRequestsInProgress;

    if (requestHasTransferedData) {
      
      

      return EvaluateAndUpdateSecurityState(aRequest);
    }
    
    return NS_OK;
  }
  
  if (aProgressStateFlags & STATE_STOP
      &&
      aProgressStateFlags & STATE_IS_REQUEST)
  {
    if (!isSubDocumentRelevant)
      return NS_OK;
    
    
    
    

    if (requestHasTransferedData)
    {  
      UpdateSubrequestMembers(aRequest);
      
      
      
      
      
      
      
      
      
      

      if (mNewToplevelSecurityStateKnown)
        return UpdateSecurityState(aRequest);
    }

    return NS_OK;
  }

  return NS_OK;
}

void nsSecureBrowserUIImpl::ObtainEventSink(nsIChannel *channel)
{
  if (!mToplevelEventSink)
    NS_QueryNotificationCallbacks(channel, mToplevelEventSink);
}

nsresult nsSecureBrowserUIImpl::UpdateSecurityState(nsIRequest* aRequest)
{
  lockIconState newSecurityState;

  PRBool showWarning = PR_FALSE;
  lockIconState warnSecurityState;

  if (mNewToplevelSecurityState & STATE_IS_SECURE)
  {
    if (mNewToplevelSecurityState & STATE_SECURE_LOW
        ||
        mNewToplevelSecurityState & STATE_SECURE_MED)
    {
      if (mSubRequestsBrokenSecurity
          ||
          mSubRequestsNoSecurity)
      {
        newSecurityState = lis_mixed_security;
      }
      else
      {
        newSecurityState = lis_low_security;
      }
    }
    else
    {
      

      if (mSubRequestsBrokenSecurity
          ||
          mSubRequestsNoSecurity)
      {
        newSecurityState = lis_mixed_security;
      }
      else if (mSubRequestsLowSecurity)
      {
        newSecurityState = lis_low_security;
      }
      else
      {
        newSecurityState = lis_high_security;
      }
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
         mPreviousSecurityState, newSecurityState
          ));

  if (mPreviousSecurityState != newSecurityState)
  {
    
    
    
    

    































    showWarning = PR_TRUE;
    
    switch (mPreviousSecurityState)
    {
      case lis_no_security:
      case lis_broken_security:
        switch (newSecurityState)
        {
          case lis_no_security:
          case lis_broken_security:
            showWarning = PR_FALSE;
            break;
          
          default:
            break;
        }
      
      default:
        break;
    }

    if (showWarning)
    {
      warnSecurityState = newSecurityState;
    }
    
    mPreviousSecurityState = newSecurityState;

    if (lis_no_security == newSecurityState)
    {
      mSSLStatus = nsnull;
      mInfoTooltip.Truncate();
    }
  }

  if (mToplevelEventSink)
  {
    PRUint32 newState = STATE_IS_INSECURE;

    switch (newSecurityState)
    {
      case lis_broken_security:
        newState = STATE_IS_BROKEN;
        break;

      case lis_mixed_security:
        newState = STATE_IS_BROKEN;
        break;

      case lis_low_security:
        newState = STATE_IS_SECURE | STATE_SECURE_LOW;
        break;

      case lis_high_security:
        newState = STATE_IS_SECURE | STATE_SECURE_HIGH;
        break;

      default:
      case lis_no_security:
        newState = STATE_IS_INSECURE;
        break;

    }

    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: UpdateSecurityState: calling OnSecurityChange\n", this
            ));

    mToplevelEventSink->OnSecurityChange(aRequest, newState);
  }
  else
  {
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: UpdateSecurityState: NO mToplevelEventSink!\n", this
            ));

  }

  if (showWarning)
  {
    switch (warnSecurityState)
    {
      case lis_no_security:
      case lis_broken_security:
        ConfirmLeavingSecure();
        break;

      case lis_mixed_security:
        ConfirmMixedMode();
        break;

      case lis_low_security:
        ConfirmEnteringWeak();
        break;

      case lis_high_security:
        ConfirmEnteringSecure();
        break;
    }
  }

  return NS_OK; 
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::OnLocationChange(nsIWebProgress* aWebProgress,
                                        nsIRequest* aRequest,
                                        nsIURI* aLocation)
{
  if (aLocation)
  {
    PRBool vs;

    nsresult rv = aLocation->SchemeIs("view-source", &vs);
    NS_ENSURE_SUCCESS(rv, rv);

    if (vs) {
      PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
             ("SecureUI:%p: OnLocationChange: view-source\n", this));
    }

    mIsViewSource = vs;
  }

  mCurrentURI = aLocation;

  
  
  if (!aRequest)
    return NS_OK;

  
  
  
  
  
  
  

  nsCOMPtr<nsIDOMWindow> windowForProgress;
  aWebProgress->GetDOMWindow(getter_AddRefs(windowForProgress));

  if (windowForProgress.get() == mWindow.get()) {
    
    return EvaluateAndUpdateSecurityState(aRequest);
  }

  
  UpdateSubrequestMembers(aRequest);

  

  
  
  
  
  
  
  
  

  if (mNewToplevelSecurityStateKnown)
    return UpdateSecurityState(aRequest);

  return NS_OK;
}

NS_IMETHODIMP
nsSecureBrowserUIImpl::OnStatusChange(nsIWebProgress* aWebProgress,
                                      nsIRequest* aRequest,
                                      nsresult aStatus,
                                      const PRUnichar* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

nsresult
nsSecureBrowserUIImpl::OnSecurityChange(nsIWebProgress *aWebProgress,
                                        nsIRequest *aRequest,
                                        PRUint32 state)
{
#if defined(DEBUG)
  nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
  if (!channel)
    return NS_OK;

  nsCOMPtr<nsIURI> aURI;
  channel->GetURI(getter_AddRefs(aURI));
  
  if (aURI) {
    nsCAutoString temp;
    aURI->GetSpec(temp);
    PR_LOG(gSecureDocLog, PR_LOG_DEBUG,
           ("SecureUI:%p: OnSecurityChange: (%x) %s\n", this,
            state, temp.get()));
  }
#endif

  return NS_OK;
}


NS_IMETHODIMP
nsSecureBrowserUIImpl::GetSSLStatus(nsISupports** _result)
{
  NS_ENSURE_ARG_POINTER(_result);

  *_result = mSSLStatus;
  NS_IF_ADDREF(*_result);

  return NS_OK;
}

nsresult
nsSecureBrowserUIImpl::IsURLHTTPS(nsIURI* aURL, PRBool* value)
{
  *value = PR_FALSE;

  if (!aURL)
    return NS_OK;

  return aURL->SchemeIs("https", value);
}

nsresult
nsSecureBrowserUIImpl::IsURLJavaScript(nsIURI* aURL, PRBool* value)
{
  *value = PR_FALSE;

  if (!aURL)
    return NS_OK;

  return aURL->SchemeIs("javascript", value);
}

void
nsSecureBrowserUIImpl::GetBundleString(const PRUnichar* name,
                                       nsAString &outString)
{
  if (mStringBundle && name) {
    PRUnichar *ptrv = nsnull;
    if (NS_SUCCEEDED(mStringBundle->GetStringFromName(name,
                                                      &ptrv)))
      outString = ptrv;
    else
      outString.SetLength(0);

    nsMemory::Free(ptrv);

  } else {
    outString.SetLength(0);
  }
}

nsresult
nsSecureBrowserUIImpl::CheckPost(nsIURI *formURL, nsIURI *actionURL, PRBool *okayToPost)
{
  PRBool formSecure, actionSecure, actionJavaScript;
  *okayToPost = PR_TRUE;

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
  } else {
    *okayToPost = ConfirmPostToInsecure();
  }

  return NS_OK;
}





class nsUIContext : public nsIInterfaceRequestor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERFACEREQUESTOR

  nsUIContext(nsIDOMWindow *window);
  virtual ~nsUIContext();

private:
  nsCOMPtr<nsIDOMWindow> mWindow;
};

NS_IMPL_ISUPPORTS1(nsUIContext, nsIInterfaceRequestor)

nsUIContext::nsUIContext(nsIDOMWindow *aWindow)
: mWindow(aWindow)
{
}

nsUIContext::~nsUIContext()
{
}


NS_IMETHODIMP nsUIContext::GetInterface(const nsIID & uuid, void * *result)
{
  nsresult rv;

  if (uuid.Equals(NS_GET_IID(nsIPrompt))) {
    nsCOMPtr<nsIDOMWindowInternal> internal = do_QueryInterface(mWindow, &rv);
    if (NS_FAILED(rv)) return rv;

    nsIPrompt *prompt;

    rv = internal->GetPrompter(&prompt);
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

nsresult nsSecureBrowserUIImpl::
GetNSSDialogs(nsISecurityWarningDialogs **result)
{
  nsresult rv;
  nsCOMPtr<nsISecurityWarningDialogs> my_result(do_GetService(NS_SECURITYWARNINGDIALOGS_CONTRACTID, &rv));

  if (NS_FAILED(rv)) 
    return rv;

  nsCOMPtr<nsISupports> proxiedResult;
  NS_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                       NS_GET_IID(nsISecurityWarningDialogs),
                       my_result, NS_PROXY_SYNC,
                       getter_AddRefs(proxiedResult));

  if (!proxiedResult) {
    return NS_ERROR_FAILURE;
  }

  return CallQueryInterface(proxiedResult, result);
}

PRBool nsSecureBrowserUIImpl::
ConfirmEnteringSecure()
{
  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool confirms;
  dialogs->ConfirmEnteringSecure(ctx, &confirms);

  return confirms;
}

PRBool nsSecureBrowserUIImpl::
ConfirmEnteringWeak()
{
  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool confirms;
  dialogs->ConfirmEnteringWeak(ctx, &confirms);

  return confirms;
}

PRBool nsSecureBrowserUIImpl::
ConfirmLeavingSecure()
{
  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool confirms;
  dialogs->ConfirmLeavingSecure(ctx, &confirms);

  return confirms;
}

PRBool nsSecureBrowserUIImpl::
ConfirmMixedMode()
{
  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool confirms;
  dialogs->ConfirmMixedMode(ctx, &confirms);

  return confirms;
}






PRBool nsSecureBrowserUIImpl::
ConfirmPostToInsecure()
{
  nsresult rv;

  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool result;

  rv = dialogs->ConfirmPostToInsecure(ctx, &result);
  if (NS_FAILED(rv)) return PR_FALSE;

  return result;
}






PRBool nsSecureBrowserUIImpl::
ConfirmPostToInsecureFromSecure()
{
  nsresult rv;

  nsCOMPtr<nsISecurityWarningDialogs> dialogs;

  GetNSSDialogs(getter_AddRefs(dialogs));
  if (!dialogs) return PR_FALSE;  

  nsCOMPtr<nsIInterfaceRequestor> ctx = new nsUIContext(mWindow);

  PRBool result;

  rv = dialogs->ConfirmPostToInsecureFromSecure(ctx, &result);
  if (NS_FAILED(rv)) return PR_FALSE;

  return result;
}
