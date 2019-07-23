







































#include "nsDocShell.h"
#include "nsWebShell.h"
#include "nsIWebBrowserChrome2.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIWebProgress.h"
#include "nsIContentViewer.h"
#include "nsIDocumentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIClipboardCommands.h"
#include "nsILinkHandler.h"
#include "nsIStreamListener.h"
#include "nsIPrompt.h"
#include "nsNetUtil.h"
#include "nsIRefreshURI.h"
#include "nsIDOMEvent.h"
#include "nsPresContext.h"
#include "nsIComponentManager.h"
#include "nsCRT.h"
#include "nsVoidArray.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prprf.h"
#include "nsIPluginHost.h"
#include "nsplugin.h"
#include "nsIPluginManager.h"
#include "nsCDefaultURIFixup.h"
#include "nsIContent.h"
#include "prlog.h"
#include "nsCOMPtr.h"
#include "nsIPresShell.h"
#include "nsIWebShellServices.h"
#include "nsIGlobalHistory.h"
#include "prmem.h"
#include "prthread.h"
#include "nsXPIDLString.h"
#include "nsDOMError.h"
#include "nsIDOMRange.h"
#include "nsIURIContentListener.h"
#include "nsIDOMDocument.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsCURILoader.h"
#include "nsIDOMWindowInternal.h"
#include "nsEscape.h"
#include "nsIPlatformCharset.h"
#include "nsICharsetConverterManager.h"
#include "nsISocketTransportService.h"
#include "nsTextFormatter.h"
#include "nsPIDOMWindow.h"
#include "nsPICommandUpdater.h"
#include "nsIController.h"
#include "nsIFocusController.h"
#include "nsGUIEvent.h"
#include "nsISHistoryInternal.h"

#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIUploadChannel.h"
#include "nsISeekableStream.h"
#include "nsStreamUtils.h"
#include "nsThreadUtils.h"

#include "nsILocaleService.h"
#include "nsIStringBundle.h"

#include "nsICachingChannel.h"

#include "nsIDocument.h"
#include "nsITextToSubURI.h"

#include "nsIExternalProtocolService.h"
#include "nsCExternalHandlerService.h"

#include "nsIIDNService.h"

#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsITimer.h"
#include "nsIScriptSecurityManager.h"
#include "nsContentPolicyUtils.h"

#ifdef NS_DEBUG




static PRLogModuleInfo* gLogModule = PR_NewLogModule("webshell");
#endif

#define WEB_TRACE_CALLS        0x1
#define WEB_TRACE_HISTORY      0x2

#define WEB_LOG_TEST(_lm,_bit) (PRIntn((_lm)->level) & (_bit))

#ifdef NS_DEBUG
#define WEB_TRACE(_bit,_args)            \
  PR_BEGIN_MACRO                         \
    if (WEB_LOG_TEST(gLogModule,_bit)) { \
      PR_LogPrint _args;                 \
    }                                    \
  PR_END_MACRO
#else
#define WEB_TRACE(_bit,_args)
#endif



#define PREF_PINGS_ENABLED           "browser.send_pings"
#define PREF_PINGS_MAX_PER_LINK      "browser.send_pings.max_per_link"
#define PREF_PINGS_REQUIRE_SAME_HOST "browser.send_pings.require_same_host"

















static PRBool
PingsEnabled(PRInt32 *maxPerLink, PRBool *requireSameHost)
{
  PRBool allow = PR_FALSE;

  *maxPerLink = 1;
  *requireSameHost = PR_TRUE;

  nsCOMPtr<nsIPrefBranch> prefs =
      do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRBool val;
    if (NS_SUCCEEDED(prefs->GetBoolPref(PREF_PINGS_ENABLED, &val)))
      allow = val;
    if (allow) {
      prefs->GetIntPref(PREF_PINGS_MAX_PER_LINK, maxPerLink);
      prefs->GetBoolPref(PREF_PINGS_REQUIRE_SAME_HOST, requireSameHost);
    }
  }

  return allow;
}

static PRBool
CheckPingURI(nsIURI* uri, nsIContent* content)
{
  if (!uri)
    return PR_FALSE;

  
  nsCOMPtr<nsIScriptSecurityManager> ssmgr =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(ssmgr, PR_FALSE);

  nsresult rv =
    ssmgr->CheckLoadURIWithPrincipal(content->NodePrincipal(), uri,
                                     nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }

  
  PRBool match;
  if ((NS_FAILED(uri->SchemeIs("http", &match)) || !match) &&
      (NS_FAILED(uri->SchemeIs("https", &match)) || !match)) {
    return PR_FALSE;
  }

  
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  nsIURI* docURI = nsnull;
  nsIDocument* doc = content->GetOwnerDoc();
  if (doc) {
    docURI = doc->GetDocumentURI();
  }
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_PING,
                                 uri,
                                 docURI,
                                 content,
                                 EmptyCString(), 
                                 nsnull, 
                                 &shouldLoad);
  return NS_SUCCEEDED(rv) && NS_CP_ACCEPTED(shouldLoad);
}

typedef void (* ForEachPingCallback)(void *closure, nsIContent *content,
                                     nsIURI *uri, nsIIOService *ios);

static void
ForEachPing(nsIContent *content, ForEachPingCallback callback, void *closure)
{
  
  
  
  

  
  
  if (!content->IsNodeOfType(nsINode::eHTML))
    return;
  nsIAtom *nameAtom = content->Tag();
  if (!nameAtom->EqualsUTF8(NS_LITERAL_CSTRING("a")) &&
      !nameAtom->EqualsUTF8(NS_LITERAL_CSTRING("area")))
    return;

  nsCOMPtr<nsIAtom> pingAtom = do_GetAtom("ping");
  if (!pingAtom)
    return;

  nsAutoString value;
  content->GetAttr(kNameSpaceID_None, pingAtom, value);
  if (value.IsEmpty())
    return;

  nsCOMPtr<nsIIOService> ios = do_GetIOService();
  if (!ios)
    return;

  nsIDocument *doc = content->GetOwnerDoc();
  if (!doc)
    return;

  
  const PRUnichar *start = value.BeginReading();
  const PRUnichar *end   = value.EndReading();
  const PRUnichar *iter  = start;
  for (;;) {
    if (iter < end && *iter != ' ') {
      ++iter;
    } else {  
      while (*start == ' ' && start < iter)
        ++start;
      if (iter != start) {
        nsCOMPtr<nsIURI> uri, baseURI = content->GetBaseURI();
        ios->NewURI(NS_ConvertUTF16toUTF8(Substring(start, iter)),
                    doc->GetDocumentCharacterSet().get(),
                    baseURI, getter_AddRefs(uri));
        if (CheckPingURI(uri, content)) {
          callback(closure, content, uri, ios);
        }
      }
      start = iter = iter + 1;
      if (iter >= end)
        break;
    }
  }
}




#define PING_TIMEOUT 10000

static void
OnPingTimeout(nsITimer *timer, void *closure)
{
  nsILoadGroup *loadGroup = static_cast<nsILoadGroup *>(closure);
  loadGroup->Cancel(NS_ERROR_ABORT);
  loadGroup->Release();
}


static PRBool
IsSameHost(nsIURI *uri1, nsIURI *uri2)
{
  nsCAutoString host1, host2;
  uri1->GetAsciiHost(host1);
  uri2->GetAsciiHost(host2);
  return host1.Equals(host2);
}

class nsPingListener : public nsIStreamListener
                     , public nsIInterfaceRequestor
                     , public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  nsPingListener(PRBool requireSameHost, nsIContent* content)
    : mRequireSameHost(requireSameHost),
      mContent(content)
  {}

private:
  PRBool mRequireSameHost;
  nsCOMPtr<nsIContent> mContent;
};

NS_IMPL_ISUPPORTS4(nsPingListener, nsIStreamListener, nsIRequestObserver,
                   nsIInterfaceRequestor, nsIChannelEventSink)

NS_IMETHODIMP
nsPingListener::OnStartRequest(nsIRequest *request, nsISupports *context)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPingListener::OnDataAvailable(nsIRequest *request, nsISupports *context,
                                nsIInputStream *stream, PRUint32 offset,
                                PRUint32 count)
{
  PRUint32 result;
  return stream->ReadSegments(NS_DiscardSegment, nsnull, count, &result);
}

NS_IMETHODIMP
nsPingListener::OnStopRequest(nsIRequest *request, nsISupports *context,
                              nsresult status)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPingListener::GetInterface(const nsIID &iid, void **result)
{
  if (iid.Equals(NS_GET_IID(nsIChannelEventSink))) {
    NS_ADDREF_THIS();
    *result = (nsIChannelEventSink *) this;
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsPingListener::OnChannelRedirect(nsIChannel *oldChan, nsIChannel *newChan,
                                  PRUint32 flags)
{
  nsCOMPtr<nsIURI> newURI;
  newChan->GetURI(getter_AddRefs(newURI));

  if (!CheckPingURI(newURI, mContent))
    return NS_ERROR_ABORT;

  if (!mRequireSameHost)
    return NS_OK;

  nsCOMPtr<nsIURI> oldURI;
  oldChan->GetURI(getter_AddRefs(oldURI));
  NS_ENSURE_STATE(oldURI && newURI);

  if (!IsSameHost(oldURI, newURI))
    return NS_ERROR_ABORT;

  return NS_OK;
}

struct SendPingInfo {
  PRInt32 numPings;
  PRInt32 maxPings;
  PRBool  requireSameHost;
  nsIURI *referrer;
};

static void
SendPing(void *closure, nsIContent *content, nsIURI *uri, nsIIOService *ios)
{
  SendPingInfo *info = static_cast<SendPingInfo *>(closure);
  if (info->numPings >= info->maxPings)
    return;

  if (info->requireSameHost) {
    
    
    if (!IsSameHost(uri, info->referrer))
      return;
  }

  nsIDocument *doc = content->GetOwnerDoc();
  if (!doc)
    return;

  nsCOMPtr<nsIChannel> chan;
  ios->NewChannelFromURI(uri, getter_AddRefs(chan));
  if (!chan)
    return;

  
  chan->SetLoadFlags(nsIRequest::INHIBIT_CACHING);

  nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(chan);
  if (!httpChan)
    return;

  
  nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(httpChan);
  if (httpInternal)
    httpInternal->SetDocumentURI(doc->GetDocumentURI());

  if (info->referrer)
    httpChan->SetReferrer(info->referrer);

  httpChan->SetRequestMethod(NS_LITERAL_CSTRING("POST"));

  
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept"),
                             EmptyCString(), PR_FALSE);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-language"),
                             EmptyCString(), PR_FALSE);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-charset"),
                             EmptyCString(), PR_FALSE);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-encoding"),
                             EmptyCString(), PR_FALSE);

  nsCOMPtr<nsIUploadChannel> uploadChan = do_QueryInterface(httpChan);
  if (!uploadChan)
    return;

  
  
  NS_NAMED_LITERAL_CSTRING(uploadData, "Content-Length: 0\r\n\r\n");

  nsCOMPtr<nsIInputStream> uploadStream;
  NS_NewPostDataStream(getter_AddRefs(uploadStream), PR_FALSE,
                       uploadData, 0);
  if (!uploadStream)
    return;

  uploadChan->SetUploadStream(uploadStream, EmptyCString(), -1);

  
  
  nsCOMPtr<nsILoadGroup> loadGroup =
      do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  if (!loadGroup)
    return;
  chan->SetLoadGroup(loadGroup);

  
  
  
  nsCOMPtr<nsIStreamListener> listener =
      new nsPingListener(info->requireSameHost, content);
  if (!listener)
    return;

  
  nsCOMPtr<nsIInterfaceRequestor> callbacks = do_QueryInterface(listener);
  NS_ASSERTION(callbacks, "oops");
  loadGroup->SetNotificationCallbacks(callbacks);

  chan->AsyncOpen(listener, nsnull);

  
  
  
  info->numPings++;

  
  nsCOMPtr<nsITimer> timer =
      do_CreateInstance(NS_TIMER_CONTRACTID);
  if (timer) {
    nsresult rv = timer->InitWithFuncCallback(OnPingTimeout, loadGroup,
                                              PING_TIMEOUT,
                                              nsITimer::TYPE_ONE_SHOT);
    if (NS_SUCCEEDED(rv)) {
      
      
      static_cast<nsILoadGroup *>(loadGroup.get())->AddRef();
      loadGroup = 0;
    }
  }
  
  
  
  if (loadGroup)
    chan->Cancel(NS_ERROR_ABORT);
}


static void
DispatchPings(nsIContent *content, nsIURI *referrer)
{
  SendPingInfo info;

  if (!PingsEnabled(&info.maxPings, &info.requireSameHost))
    return;
  if (info.maxPings == 0)
    return;

  info.numPings = 0;
  info.referrer = referrer;

  ForEachPing(content, SendPing, &info);
}




nsWebShell::nsWebShell() : nsDocShell()
{
#ifdef DEBUG
  
  ++gNumberOfWebShells;
#endif
#ifdef DEBUG
    printf("++WEBSHELL %p == %ld\n", (void*) this, gNumberOfWebShells);
#endif

  InitFrameData();
  mItemType = typeContent;
  mCharsetReloadState = eCharsetReloadInit;
}

nsWebShell::~nsWebShell()
{
   Destroy();

  ++mRefCnt; 
             

  mContentViewer=nsnull;
  mDeviceContext=nsnull;

  InitFrameData();

#ifdef DEBUG
  
  --gNumberOfWebShells;
#endif
#ifdef DEBUG
  printf("--WEBSHELL %p == %ld\n", (void*) this, gNumberOfWebShells);
#endif
}

void nsWebShell::InitFrameData()
{
  SetMarginWidth(-1);    
  SetMarginHeight(-1);
}

nsresult
nsWebShell::EnsureCommandHandler()
{
  if (!mCommandManager)
  {
    mCommandManager = do_CreateInstance("@mozilla.org/embedcomp/command-manager;1");
    if (!mCommandManager) return NS_ERROR_OUT_OF_MEMORY;
    
    nsCOMPtr<nsPICommandUpdater>       commandUpdater = do_QueryInterface(mCommandManager);
    if (!commandUpdater) return NS_ERROR_FAILURE;
    
    nsCOMPtr<nsIDOMWindow> domWindow = do_GetInterface(static_cast<nsIInterfaceRequestor *>(this));
#ifdef DEBUG
    nsresult rv =
#endif
    commandUpdater->Init(domWindow);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Initting command manager failed");
  }
  
  return mCommandManager ? NS_OK : NS_ERROR_FAILURE;
}



NS_IMPL_ADDREF_INHERITED(nsWebShell, nsDocShell)
NS_IMPL_RELEASE_INHERITED(nsWebShell, nsDocShell)

NS_INTERFACE_MAP_BEGIN(nsWebShell)
   NS_INTERFACE_MAP_ENTRY(nsIWebShellServices)
   NS_INTERFACE_MAP_ENTRY(nsILinkHandler)
   NS_INTERFACE_MAP_ENTRY(nsIClipboardCommands)
NS_INTERFACE_MAP_END_INHERITING(nsDocShell)

NS_IMETHODIMP
nsWebShell::GetInterface(const nsIID &aIID, void** aInstancePtr)
{
   NS_PRECONDITION(aInstancePtr, "null out param");

   *aInstancePtr = nsnull;

   if(aIID.Equals(NS_GET_IID(nsICommandManager)))
      {
      NS_ENSURE_SUCCESS(EnsureCommandHandler(), NS_ERROR_FAILURE);
      *aInstancePtr = mCommandManager;
      NS_ADDREF((nsISupports*) *aInstancePtr);
      return NS_OK;
      }

   return nsDocShell::GetInterface(aIID, aInstancePtr);
}

nsEventStatus PR_CALLBACK
nsWebShell::HandleEvent(nsGUIEvent *aEvent)
{
  return nsEventStatus_eIgnore;
}







NS_IMETHODIMP
nsWebShell::ReloadDocument(const char* aCharset,
                           PRInt32 aSource)
{

  
  nsCOMPtr<nsIContentViewer> cv;
  NS_ENSURE_SUCCESS(GetContentViewer(getter_AddRefs(cv)), NS_ERROR_FAILURE);
  if (cv)
  {
    nsCOMPtr<nsIMarkupDocumentViewer> muDV = do_QueryInterface(cv);  
    if (muDV)
    {
      PRInt32 hint;
      muDV->GetHintCharacterSetSource(&hint);
      if( aSource > hint ) 
      {
         muDV->SetHintCharacterSet(nsDependentCString(aCharset));
         muDV->SetHintCharacterSetSource(aSource);
         if(eCharsetReloadRequested != mCharsetReloadState) 
         {
            mCharsetReloadState = eCharsetReloadRequested;
            return Reload(LOAD_FLAGS_CHARSET_CHANGE);
         }
      }
    }
  }
  
  return NS_ERROR_WEBSHELL_REQUEST_REJECTED;
}


NS_IMETHODIMP
nsWebShell::StopDocumentLoad(void)
{
  if(eCharsetReloadRequested != mCharsetReloadState) 
  {
    Stop(nsIWebNavigation::STOP_ALL);
    return NS_OK;
  }
  
  return NS_ERROR_WEBSHELL_REQUEST_REJECTED;
}

NS_IMETHODIMP
nsWebShell::SetRendering(PRBool aRender)
{
  if(eCharsetReloadRequested != mCharsetReloadState) 
  {
    if (mContentViewer) {
       mContentViewer->SetEnableRendering(aRender);
       return NS_OK;
    }
  }
  
  return NS_ERROR_WEBSHELL_REQUEST_REJECTED;
}





class OnLinkClickEvent : public nsRunnable {
public:
  OnLinkClickEvent(nsWebShell* aHandler, nsIContent* aContent,
                   nsIURI* aURI,
                   const PRUnichar* aTargetSpec,
                   nsIInputStream* aPostDataStream = 0, 
                   nsIInputStream* aHeadersDataStream = 0);

  NS_IMETHOD Run() {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mHandler->mScriptGlobal));
    nsAutoPopupStatePusher popupStatePusher(window, mPopupState);

    mHandler->OnLinkClickSync(mContent, mURI,
                              mTargetSpec.get(), mPostDataStream,
                              mHeadersDataStream,
                              nsnull, nsnull);
    return NS_OK;
  }

private:
  nsRefPtr<nsWebShell>     mHandler;
  nsCOMPtr<nsIURI>         mURI;
  nsString                 mTargetSpec;
  nsCOMPtr<nsIInputStream> mPostDataStream;
  nsCOMPtr<nsIInputStream> mHeadersDataStream;
  nsCOMPtr<nsIContent>     mContent;
  PopupControlState        mPopupState;
};

OnLinkClickEvent::OnLinkClickEvent(nsWebShell* aHandler,
                                   nsIContent *aContent,
                                   nsIURI* aURI,
                                   const PRUnichar* aTargetSpec,
                                   nsIInputStream* aPostDataStream,
                                   nsIInputStream* aHeadersDataStream)
  : mHandler(aHandler)
  , mURI(aURI)
  , mTargetSpec(aTargetSpec)
  , mPostDataStream(aPostDataStream)
  , mHeadersDataStream(aHeadersDataStream)
  , mContent(aContent)
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mHandler->mScriptGlobal));

  mPopupState = window->GetPopupControlState();
}



NS_IMETHODIMP
nsWebShell::OnLinkClick(nsIContent* aContent,
                        nsIURI* aURI,
                        const PRUnichar* aTargetSpec,
                        nsIInputStream* aPostDataStream,
                        nsIInputStream* aHeadersDataStream)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");

  if (mFiredUnloadEvent) {
    return NS_OK;
  }

  if (aContent->HasFlag(NODE_IS_EDITABLE)) {
    return NS_OK;
  }

  nsCOMPtr<nsIRunnable> ev =
      new OnLinkClickEvent(this, aContent, aURI, aTargetSpec,
                           aPostDataStream, aHeadersDataStream);
  return NS_DispatchToCurrentThread(ev);
}

NS_IMETHODIMP
nsWebShell::OnLinkClickSync(nsIContent *aContent,
                            nsIURI* aURI,
                            const PRUnichar* aTargetSpec,
                            nsIInputStream* aPostDataStream,
                            nsIInputStream* aHeadersDataStream,
                            nsIDocShell** aDocShell,
                            nsIRequest** aRequest)
{
  
  if (aDocShell) {
    *aDocShell = nsnull;
  }
  if (aRequest) {
    *aRequest = nsnull;
  }

  if (mFiredUnloadEvent) {
    return NS_OK;
  }

  if (aContent->HasFlag(NODE_IS_EDITABLE)) {
    return NS_OK;
  }

  {
    
    nsCOMPtr<nsIExternalProtocolService> extProtService = do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
    if (extProtService) {
      nsCAutoString scheme;
      aURI->GetScheme(scheme);
      if (!scheme.IsEmpty()) {
        
        
        PRBool isExposed;
        nsresult rv = extProtService->IsExposedProtocol(scheme.get(), &isExposed);
        if (NS_SUCCEEDED(rv) && !isExposed) {
          return extProtService->LoadUrl(aURI);
        }
      }
    }
  }

  nsCOMPtr<nsIDOMNode> node(do_QueryInterface(aContent));
  NS_ENSURE_TRUE(node, NS_ERROR_UNEXPECTED);

  PRBool inherit;
  nsresult rv = URIInheritsSecurityContext(aURI, &inherit);
  if (NS_FAILED(rv) || inherit) {
    nsCOMPtr<nsIDocument> sourceDoc = aContent->GetDocument();

    if (!sourceDoc) {
      
      
      

      return NS_OK;
    }

    nsCOMPtr<nsIPresShell> presShell;
    GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    if (presShell->GetDocument() != sourceDoc) {
      
      

      return NS_OK;
    }
  }

  
  
  
  
  
  nsCOMPtr<nsIDOMDocument> refererOwnerDoc;
  node->GetOwnerDocument(getter_AddRefs(refererOwnerDoc));

  nsCOMPtr<nsIDocument> refererDoc(do_QueryInterface(refererOwnerDoc));
  NS_ENSURE_TRUE(refererDoc, NS_ERROR_UNEXPECTED);

  nsIURI *referer = refererDoc->GetDocumentURI();

  
  

  nsAutoString target(aTargetSpec);

  
  nsAutoString typeHint;
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(aContent));
  if (anchor) {
    anchor->GetType(typeHint);
  }
  
  rv = InternalLoad(aURI,               
                    referer,            
                    nsnull,             
                    INTERNAL_LOAD_FLAGS_INHERIT_OWNER, 
                    target.get(),       
                    NS_LossyConvertUTF16toASCII(typeHint).get(),
                    aPostDataStream,    
                    aHeadersDataStream, 
                    LOAD_LINK,          
                    nsnull,             
                    PR_TRUE,            
                    aDocShell,          
                    aRequest);          
  if (NS_SUCCEEDED(rv)) {
    DispatchPings(aContent, referer);
  }
  return rv;
}

NS_IMETHODIMP
nsWebShell::OnOverLink(nsIContent* aContent,
                       nsIURI* aURI,
                       const PRUnichar* aTargetSpec)
{
  if (aContent->HasFlag(NODE_IS_EDITABLE)) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebBrowserChrome2> browserChrome2 = do_GetInterface(mTreeOwner);
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome;
  if (!browserChrome2) {
    browserChrome = do_GetInterface(mTreeOwner);
    if (!browserChrome)
      return rv;
  }

  nsCOMPtr<nsITextToSubURI> textToSubURI =
      do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  
  nsCAutoString charset;
  rv = aURI->GetOriginCharset(charset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString uStr;
  rv = textToSubURI->UnEscapeURIForUI(charset, spec, uStr);    
  NS_ENSURE_SUCCESS(rv, rv);

  if (browserChrome2) {
    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(aContent);
    rv = browserChrome2->SetStatusWithContext(nsIWebBrowserChrome::STATUS_LINK,
                                              uStr, element);
  } else {
    rv = browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_LINK, uStr.get());
  }
  return rv;
}

NS_IMETHODIMP
nsWebShell::OnLeaveLink()
{
  nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(mTreeOwner));
  nsresult rv = NS_ERROR_FAILURE;

  if (browserChrome)  {
      rv = browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_LINK,
                                    EmptyString().get());
  }
  return rv;
}

NS_IMETHODIMP
nsWebShell::GetLinkState(nsIURI* aLinkURI, nsLinkState& aState)
{
  if (!aLinkURI) {
    
    aState = eLinkState_NotLink;
    return NS_OK;
  }
    
  aState = eLinkState_Unvisited;

  
  if (!mGlobalHistory)
    return NS_OK;

  PRBool isVisited;
  NS_ENSURE_SUCCESS(mGlobalHistory->IsVisited(aLinkURI, &isVisited),
                    NS_ERROR_FAILURE);
  if (isVisited)
    aState = eLinkState_Visited;
  
  return NS_OK;
}


nsresult nsWebShell::EndPageLoad(nsIWebProgress *aProgress,
                                 nsIChannel* channel,
                                 nsresult aStatus)
{
  nsresult rv = NS_OK;

  if(!channel)
    return NS_ERROR_NULL_POINTER;
    
  nsCOMPtr<nsIURI> url;
  rv = channel->GetURI(getter_AddRefs(url));
  if (NS_FAILED(rv)) return rv;
  
  
  if(eCharsetReloadRequested == mCharsetReloadState)
    mCharsetReloadState = eCharsetReloadStopOrigional;
  else 
    mCharsetReloadState = eCharsetReloadInit;

  
  
  
  nsCOMPtr<nsISHEntry> loadingSHE = mLSHE;
  
  
  
  
  
  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);
  nsDocShell::EndPageLoad(aProgress, channel, aStatus);

  
  PRBool isTopFrame = PR_TRUE;
  nsCOMPtr<nsIDocShellTreeItem> targetParentTreeItem;
  rv = GetSameTypeParent(getter_AddRefs(targetParentTreeItem));
  if (NS_SUCCEEDED(rv) && targetParentTreeItem) 
  {
    isTopFrame = PR_FALSE;
  }

  
  
  
  
  
  
  
  
  
  

  if (url && NS_FAILED(aStatus))
  {
    if (aStatus == NS_ERROR_FILE_NOT_FOUND ||
        aStatus == NS_ERROR_INVALID_CONTENT_ENCODING)
    {
      DisplayLoadError(aStatus, url, nsnull, channel);
      return NS_OK;
    }

    if (sURIFixup)
    {
      
      
      
      nsCOMPtr<nsIURI> newURI;

      nsCAutoString oldSpec;
      url->GetSpec(oldSpec);
      
      
      
      
      if (aStatus == NS_ERROR_UNKNOWN_HOST && mAllowKeywordFixup)
      {
        PRBool keywordsEnabled = PR_FALSE;

        if (mPrefs && NS_FAILED(mPrefs->GetBoolPref("keyword.enabled", &keywordsEnabled)))
            keywordsEnabled = PR_FALSE;

        nsCAutoString host;
        url->GetHost(host);

        nsCAutoString scheme;
        url->GetScheme(scheme);

        PRInt32 dotLoc = host.FindChar('.');

        
        
        
        
        
        
        
        
        
        if (keywordsEnabled && !scheme.IsEmpty() &&
           (scheme.Find("http") != 0)) {
            keywordsEnabled = PR_FALSE;
        }

        if(keywordsEnabled && (kNotFound == dotLoc)) {
          
          
          
          
          
          
          
          
          
          
          
          PRBool isACE;
          nsCAutoString utf8Host;
          nsCOMPtr<nsIIDNService> idnSrv =
              do_GetService(NS_IDNSERVICE_CONTRACTID);
          if (idnSrv &&
              NS_SUCCEEDED(idnSrv->IsACE(host, &isACE)) && isACE &&
              NS_SUCCEEDED(idnSrv->ConvertACEtoUTF8(host, utf8Host)))
            sURIFixup->KeywordToURI(utf8Host, getter_AddRefs(newURI));
          else
            sURIFixup->KeywordToURI(host, getter_AddRefs(newURI));
        } 
      }

      
      
      
      if (aStatus == NS_ERROR_UNKNOWN_HOST ||
          aStatus == NS_ERROR_NET_RESET)
      {
        PRBool doCreateAlternate = PR_TRUE;
        
        
        
        
        if (mLoadType != LOAD_NORMAL || !isTopFrame)
        {
          doCreateAlternate = PR_FALSE;
        }
        else
        {
          
          if (newURI)
          {
            PRBool sameURI = PR_FALSE;
            url->Equals(newURI, &sameURI);
            if (!sameURI)
            {
              
              
              doCreateAlternate = PR_FALSE;
            }
          }
        }
        if (doCreateAlternate)
        {
          newURI = nsnull;
          sURIFixup->CreateFixupURI(oldSpec,
              nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI, getter_AddRefs(newURI));
        }
      }

      
      
      
      if (newURI)
      {
        
        
        PRBool sameURI = PR_FALSE;
        url->Equals(newURI, &sameURI);
        if (!sameURI)
        {
          nsCAutoString newSpec;
          newURI->GetSpec(newSpec);
          NS_ConvertUTF8toUTF16 newSpecW(newSpec);

          
          rv = url->SetSpec(newSpec);
          if (NS_FAILED(rv)) return rv;

          return LoadURI(newSpecW.get(),      
                         LOAD_FLAGS_NONE, 
                         nsnull,          
                         nsnull,          
                         nsnull);         
        }
      }
    }

    
    
    
    

    
    if ((aStatus == NS_ERROR_UNKNOWN_HOST || 
         aStatus == NS_ERROR_CONNECTION_REFUSED ||
         aStatus == NS_ERROR_UNKNOWN_PROXY_HOST || 
         aStatus == NS_ERROR_PROXY_CONNECTION_REFUSED) &&
            (isTopFrame || mUseErrorPages)) {
      DisplayLoadError(aStatus, url, nsnull, channel);
    }
    
    else if (aStatus == NS_ERROR_NET_TIMEOUT ||
             aStatus == NS_ERROR_REDIRECT_LOOP ||
             aStatus == NS_ERROR_UNKNOWN_SOCKET_TYPE ||
             aStatus == NS_ERROR_NET_INTERRUPT ||
             aStatus == NS_ERROR_NET_RESET ||
             NS_ERROR_GET_MODULE(aStatus) == NS_ERROR_MODULE_SECURITY) {
      DisplayLoadError(aStatus, url, nsnull, channel);
    }
    else if (aStatus == NS_ERROR_DOCUMENT_NOT_CACHED) {
      






      nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
      nsCAutoString method;
      if (httpChannel)
        httpChannel->GetRequestMethod(method);
      if (method.Equals("POST") && !NS_IsOffline()) {
        PRBool repost;
        rv = ConfirmRepost(&repost);
        if (NS_FAILED(rv)) return rv;
        
        
        if (!repost)
          return NS_OK;

        
        
        
        
        
        nsCOMPtr<nsISHistory> rootSH=mSessionHistory;
        if (!mSessionHistory) {
          nsCOMPtr<nsIDocShellTreeItem> root;
          
          GetSameTypeRootTreeItem(getter_AddRefs(root));
          if (root) {
            
            nsCOMPtr<nsIWebNavigation> rootAsWebnav(do_QueryInterface(root));
            if (rootAsWebnav) {
            
            rootAsWebnav->GetSessionHistory(getter_AddRefs(rootSH));
            }
          }
        }  

        if (rootSH && (mLoadType & LOAD_CMD_HISTORY)) {
          nsCOMPtr<nsISHistoryInternal> shInternal(do_QueryInterface(rootSH));
          if (shInternal) {
            rootSH->GetIndex(&mPreviousTransIndex);
            shInternal->UpdateIndex();
            rootSH->GetIndex(&mLoadedTransIndex);
#ifdef DEBUG_PAGE_CACHE
            printf("Previous index: %d, Loaded index: %d\n\n",
                  mPreviousTransIndex, mLoadedTransIndex);
#endif
          }
        }

        
        
        
        
        
        
        SetHistoryEntry(&mOSHE, loadingSHE);

        
        

        
        nsCOMPtr<nsIInputStream> inputStream;
        nsCOMPtr<nsIURI> referrer;
        if (channel) {
          nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));

          if(httpChannel) {
            httpChannel->GetReferrer(getter_AddRefs(referrer));
            nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(channel));
            if (uploadChannel) {
              uploadChannel->GetUploadStream(getter_AddRefs(inputStream));
            }
          }
        }
        nsCOMPtr<nsISeekableStream> postDataSeekable(do_QueryInterface(inputStream));
        if (postDataSeekable)
        {
          postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
        }
        InternalLoad(url,                               
                    referrer,                          
                    nsnull,                            
                    INTERNAL_LOAD_FLAGS_INHERIT_OWNER, 
                    nsnull,                            
                    nsnull,                            
                    inputStream,                       
                    nsnull,                            
                    LOAD_RELOAD_BYPASS_PROXY_AND_CACHE,
                    nsnull,                            
                    PR_TRUE,                           
                    nsnull,                            
                    nsnull);                           
      }
      else {
        DisplayLoadError(aStatus, url, nsnull, channel);
      }
    }
  } 

  return NS_OK;
}





#ifdef XP_MAC
#pragma mark -
#endif

nsresult
nsWebShell::GetControllerForCommand ( const char * inCommand, nsIController** outController )
{
  NS_ENSURE_ARG_POINTER(outController);
  *outController = nsnull;
  
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsPIDOMWindow> window ( do_QueryInterface(mScriptGlobal) );
  if ( window ) {
    nsIFocusController *focusController = window->GetRootFocusController();
    if ( focusController )
      rv = focusController->GetControllerForCommand ( inCommand, outController );
  } 

  return rv;
  
} 


nsresult
nsWebShell::IsCommandEnabled ( const char * inCommand, PRBool* outEnabled )
{
  NS_ENSURE_ARG_POINTER(outEnabled);
  *outEnabled = PR_FALSE;

  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand ( inCommand, getter_AddRefs(controller) );
  if ( controller )
    rv = controller->IsCommandEnabled(inCommand, outEnabled);
  
  return rv;
}


nsresult
nsWebShell::DoCommand ( const char * inCommand )
{
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand ( inCommand, getter_AddRefs(controller) );
  if ( controller )
    rv = controller->DoCommand(inCommand);
  
  return rv;
}


NS_IMETHODIMP
nsWebShell::CanCutSelection(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_cut", aResult );
}

NS_IMETHODIMP
nsWebShell::CanCopySelection(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_copy", aResult );
}

NS_IMETHODIMP
nsWebShell::CanCopyLinkLocation(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_copyLink", aResult );
}

NS_IMETHODIMP
nsWebShell::CanCopyImageLocation(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_copyImageLocation",
                            aResult );
}

NS_IMETHODIMP
nsWebShell::CanCopyImageContents(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_copyImageContents",
                            aResult );
}

NS_IMETHODIMP
nsWebShell::CanPaste(PRBool* aResult)
{
  return IsCommandEnabled ( "cmd_paste", aResult );
}

NS_IMETHODIMP
nsWebShell::CutSelection(void)
{
  return DoCommand ( "cmd_cut" );
}

NS_IMETHODIMP
nsWebShell::CopySelection(void)
{
  return DoCommand ( "cmd_copy" );
}

NS_IMETHODIMP
nsWebShell::CopyLinkLocation(void)
{
  return DoCommand ( "cmd_copyLink" );
}

NS_IMETHODIMP
nsWebShell::CopyImageLocation(void)
{
  return DoCommand ( "cmd_copyImageLocation" );
}

NS_IMETHODIMP
nsWebShell::CopyImageContents(void)
{
  return DoCommand ( "cmd_copyImageContents" );
}

NS_IMETHODIMP
nsWebShell::Paste(void)
{
  return DoCommand ( "cmd_paste" );
}

NS_IMETHODIMP
nsWebShell::SelectAll(void)
{
  return DoCommand ( "cmd_selectAll" );
}








NS_IMETHODIMP
nsWebShell::SelectNone(void)
{
  return DoCommand ( "cmd_selectNone" );
}


#ifdef XP_MAC
#pragma mark -
#endif





NS_IMETHODIMP nsWebShell::Create()
{
  if (mPrefs) {
    
    return NS_OK;
  }
  
  WEB_TRACE(WEB_TRACE_CALLS,
            ("nsWebShell::Init: this=%p", this));

  return nsDocShell::Create();
}

#ifdef DEBUG
unsigned long nsWebShell::gNumberOfWebShells = 0;
#endif
