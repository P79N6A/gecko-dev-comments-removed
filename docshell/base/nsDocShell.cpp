










































#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif

#include "nsIBrowserDOMWindow.h"
#include "nsIComponentManager.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMNSDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMStorageObsolete.h"
#include "nsIDOMStorage.h"
#include "nsPIDOMStorage.h"
#include "nsIDocumentViewer.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsCURILoader.h"
#include "nsURILoader.h"
#include "nsDocShellCID.h"
#include "nsLayoutCID.h"
#include "nsDOMCID.h"
#include "nsIDOMScriptObjectFactory.h"
#include "nsNetUtil.h"
#include "nsRect.h"
#include "prprf.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIWebBrowserChrome.h"
#include "nsPoint.h"
#include "nsGfxCIID.h"
#include "nsIObserverService.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIAuthPrompt2.h"
#include "nsTextFormatter.h"
#include "nsIChannelEventSink.h"
#include "nsIUploadChannel.h"
#include "nsISecurityEventSink.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsDocumentCharsetInfoCID.h"
#include "nsICanvasFrame.h"
#include "nsIScrollableFrame.h"
#include "nsContentPolicyUtils.h" 
#include "nsICategoryManager.h"
#include "nsXPCOMCID.h"
#include "nsISeekableStream.h"
#include "nsAutoPtr.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefBranch2.h"
#include "nsIWritablePropertyBag2.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"
#include "nsDOMJSUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIScrollableView.h"
#include "nsIScriptChannel.h"
#include "nsIURIClassifier.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsCPrefetchService.h"




#include "nsIHttpChannelInternal.h"  



#include "nsDocShell.h"
#include "nsDocShellLoadInfo.h"
#include "nsCDefaultURIFixup.h"
#include "nsDocShellEnumerator.h"
#include "nsSHistory.h"
#include "nsDocShellEditorData.h"


#include "nsDOMError.h"
#include "nsEscape.h"


#include "nsIUploadChannel.h"
#include "nsIProgressEventSink.h"
#include "nsIWebProgress.h"
#include "nsILayoutHistoryState.h"
#include "nsITimer.h"
#include "nsISHistoryInternal.h"
#include "nsIPrincipal.h"
#include "nsIFileURL.h"
#include "nsIHistoryEntry.h"
#include "nsISHistoryListener.h"
#include "nsIWindowWatcher.h"
#include "nsIPromptFactory.h"
#include "nsIObserver.h"
#include "nsINestedURI.h"
#include "nsITransportSecurityInfo.h"
#include "nsINSSErrorsService.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIPermissionManager.h"
#include "nsStreamUtils.h"
#include "nsIController.h"
#include "nsPICommandUpdater.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIWebBrowserChrome2.h"


#include "nsIEditingSession.h"

#include "nsPIDOMWindow.h"
#include "nsIDOMDocument.h"
#include "nsICachingChannel.h"
#include "nsICacheVisitor.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIMultiPartChannel.h"
#include "nsIWyciwygChannel.h"


#include "nsIHTMLDocument.h"



#include "nsIConsoleService.h"
#include "nsIScriptError.h"


#include "nsCExternalHandlerService.h"
#include "nsIExternalProtocolService.h"

#include "nsIFocusController.h"
#include "nsFocusManager.h"

#include "nsITextToSubURI.h"

#include "nsIJARChannel.h"

#include "prlog.h"
#include "prmem.h"

#include "nsISelectionDisplay.h"

#include "nsIGlobalHistory2.h"
#include "nsIGlobalHistory3.h"

#ifdef DEBUG_DOCSHELL_FOCUS
#include "nsIEventStateManager.h"
#endif

#include "nsIFrame.h"


#include "nsIWebBrowserChromeFocus.h"

#include "nsPluginError.h"

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#if defined(DEBUG_bryner) || defined(DEBUG_chb)

#define DEBUG_PAGE_CACHE
#endif

#include "nsContentErrors.h"


static PRInt32 gNumberOfDocumentsLoading = 0;


static PRInt32 gDocShellCount = 0;


nsIURIFixup *nsDocShell::sURIFixup = 0;




static PRBool gValidateOrigin = (PRBool)0xffffffff;




#define NS_EVENT_STARVATION_DELAY_HINT 2000





#define NS_ERROR_DOCUMENT_IS_PRINTMODE  NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL,2001)

#ifdef PR_LOGGING
#ifdef DEBUG
static PRLogModuleInfo* gDocShellLog;
#endif
static PRLogModuleInfo* gDocShellLeakLog;
#endif

const char kBrandBundleURL[]      = "chrome://branding/locale/brand.properties";
const char kAppstringsBundleURL[] = "chrome://global/locale/appstrings.properties";

static void
FavorPerformanceHint(PRBool perfOverStarvation, PRUint32 starvationDelay)
{
    nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
    if (appShell)
        appShell->FavorPerformanceHint(perfOverStarvation, starvationDelay);
}





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
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_PING,
                                 uri,
                                 content->NodePrincipal(),
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






nsDocShell::nsDocShell():
    nsDocLoader(),
    mDefaultScrollbarPref(Scrollbar_Auto, Scrollbar_Auto),
    mTreeOwner(nsnull),
    mChromeEventHandler(nsnull),
    mCharsetReloadState(eCharsetReloadInit),
    mChildOffset(0),
    mBusyFlags(BUSY_FLAGS_NONE),
    mAppType(nsIDocShell::APP_TYPE_UNKNOWN),
    mMarginWidth(-1),
    mMarginHeight(-1),
    mItemType(typeContent),
    mPreviousTransIndex(-1),
    mLoadedTransIndex(-1),
    mAllowSubframes(PR_TRUE),
    mAllowPlugins(PR_TRUE),
    mAllowJavascript(PR_TRUE),
    mAllowMetaRedirects(PR_TRUE),
    mAllowImages(PR_TRUE),
    mAllowDNSPrefetch(PR_TRUE),
    mCreatingDocument(PR_FALSE),
    mUseErrorPages(PR_FALSE),
    mObserveErrorPages(PR_TRUE),
    mAllowAuth(PR_TRUE),
    mAllowKeywordFixup(PR_FALSE),
    mIsOffScreenBrowser(PR_FALSE),
    mFiredUnloadEvent(PR_FALSE),
    mEODForCurrentDocument(PR_FALSE),
    mURIResultedInDocument(PR_FALSE),
    mIsBeingDestroyed(PR_FALSE),
    mIsExecutingOnLoadHandler(PR_FALSE),
    mIsPrintingOrPP(PR_FALSE),
    mSavingOldViewer(PR_FALSE)
#ifdef DEBUG
    , mInEnsureScriptEnv(PR_FALSE)
#endif
{
    if (gDocShellCount++ == 0) {
        NS_ASSERTION(sURIFixup == nsnull,
                     "Huh, sURIFixup not null in first nsDocShell ctor!");

        CallGetService(NS_URIFIXUP_CONTRACTID, &sURIFixup);
    }

#ifdef PR_LOGGING
#ifdef DEBUG
    if (! gDocShellLog)
        gDocShellLog = PR_NewLogModule("nsDocShell");
#endif
    if (nsnull == gDocShellLeakLog)
        gDocShellLeakLog = PR_NewLogModule("nsDocShellLeak");
    if (gDocShellLeakLog)
        PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p created\n", this));
#endif

#ifdef DEBUG
  
  ++gNumberOfDocShells;
#endif
#ifdef DEBUG
  printf("++DOCSHELL %p == %ld\n", (void*) this, gNumberOfDocShells);
#endif
}

nsDocShell::~nsDocShell()
{
    Destroy();

    if (--gDocShellCount == 0) {
        NS_IF_RELEASE(sURIFixup);
    }

#ifdef PR_LOGGING
    if (gDocShellLeakLog)
        PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p destroyed\n", this));
#endif

#ifdef DEBUG
    
    --gNumberOfDocShells;
#endif
#ifdef DEBUG
    printf("--DOCSHELL %p == %ld\n", (void*) this, gNumberOfDocShells);
#endif
}

nsresult
nsDocShell::Init()
{
    nsresult rv = nsDocLoader::Init();
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(mLoadGroup, "Something went wrong!");

    mContentListener = new nsDSURIContentListener(this);
    NS_ENSURE_TRUE(mContentListener, NS_ERROR_OUT_OF_MEMORY);

    rv = mContentListener->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    if (!mStorages.Init())
        return NS_ERROR_OUT_OF_MEMORY;

    
    
    nsCOMPtr<InterfaceRequestorProxy> proxy =
        new InterfaceRequestorProxy(static_cast<nsIInterfaceRequestor*>
                                               (this));
    NS_ENSURE_TRUE(proxy, NS_ERROR_OUT_OF_MEMORY);
    mLoadGroup->SetNotificationCallbacks(proxy);

    rv = nsDocLoader::AddDocLoaderAsChildOfRoot(this);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    
    return AddProgressListener(this, nsIWebProgress::NOTIFY_STATE_DOCUMENT |
                                     nsIWebProgress::NOTIFY_STATE_NETWORK);
    
}

void
nsDocShell::DestroyChildren()
{
    nsCOMPtr<nsIDocShellTreeItem> shell;
    PRInt32 n = mChildList.Count();
    for (PRInt32 i = 0; i < n; i++) {
        shell = do_QueryInterface(ChildAt(i));
        NS_ASSERTION(shell, "docshell has null child");

        if (shell) {
            shell->SetTreeOwner(nsnull);
        }
    }

    nsDocLoader::DestroyChildren();
}





NS_IMPL_ADDREF_INHERITED(nsDocShell, nsDocLoader)
NS_IMPL_RELEASE_INHERITED(nsDocShell, nsDocLoader)

NS_INTERFACE_MAP_BEGIN(nsDocShell)
    NS_INTERFACE_MAP_ENTRY(nsIDocShell)
    NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeItem)
    NS_INTERFACE_MAP_ENTRY(nsIDocShellTreeNode)
    NS_INTERFACE_MAP_ENTRY(nsIDocShellHistory)
    NS_INTERFACE_MAP_ENTRY(nsIWebNavigation)
    NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
    NS_INTERFACE_MAP_ENTRY(nsIScrollable)
    NS_INTERFACE_MAP_ENTRY(nsITextScroll)
    NS_INTERFACE_MAP_ENTRY(nsIDocCharset)
    NS_INTERFACE_MAP_ENTRY(nsIScriptGlobalObjectOwner)
    NS_INTERFACE_MAP_ENTRY(nsIRefreshURI)
    NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
    NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
    NS_INTERFACE_MAP_ENTRY(nsIContentViewerContainer)
    NS_INTERFACE_MAP_ENTRY(nsIEditorDocShell)
    NS_INTERFACE_MAP_ENTRY(nsIWebPageDescriptor)
    NS_INTERFACE_MAP_ENTRY(nsIAuthPromptProvider)
    NS_INTERFACE_MAP_ENTRY(nsIObserver)
    NS_INTERFACE_MAP_ENTRY(nsILoadContext)
    NS_INTERFACE_MAP_ENTRY(nsIWebShellServices)
    NS_INTERFACE_MAP_ENTRY(nsILinkHandler)
    NS_INTERFACE_MAP_ENTRY(nsIClipboardCommands)
NS_INTERFACE_MAP_END_INHERITING(nsDocLoader)




NS_IMETHODIMP nsDocShell::GetInterface(const nsIID & aIID, void **aSink)
{
    NS_PRECONDITION(aSink, "null out param");

    *aSink = nsnull;

    if (aIID.Equals(NS_GET_IID(nsICommandManager))) {
        NS_ENSURE_SUCCESS(EnsureCommandHandler(), NS_ERROR_FAILURE);
        *aSink = mCommandManager;
    }
    else if (aIID.Equals(NS_GET_IID(nsIURIContentListener))) {
        *aSink = mContentListener;
    }
    else if (aIID.Equals(NS_GET_IID(nsIScriptGlobalObject)) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
        *aSink = mScriptGlobal;
    }
    else if ((aIID.Equals(NS_GET_IID(nsIDOMWindowInternal)) ||
              aIID.Equals(NS_GET_IID(nsPIDOMWindow)) ||
              aIID.Equals(NS_GET_IID(nsIDOMWindow))) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
        return mScriptGlobal->QueryInterface(aIID, aSink);
    }
    else if (aIID.Equals(NS_GET_IID(nsIDOMDocument)) &&
             NS_SUCCEEDED(EnsureContentViewer())) {
        mContentViewer->GetDOMDocument((nsIDOMDocument **) aSink);
        return *aSink ? NS_OK : NS_NOINTERFACE;
    }
    else if (aIID.Equals(NS_GET_IID(nsIApplicationCacheContainer))) {
        *aSink = nsnull;

        

        nsCOMPtr<nsIContentViewer> contentViewer;
        GetContentViewer(getter_AddRefs(contentViewer));
        if (!contentViewer)
            return NS_ERROR_NO_INTERFACE;

        nsCOMPtr<nsIDOMDocument> domDoc;
        contentViewer->GetDOMDocument(getter_AddRefs(domDoc));
        NS_ASSERTION(domDoc, "Should have a document.");
        if (!domDoc)
            return NS_ERROR_NO_INTERFACE;

#if defined(PR_LOGGING) && defined(DEBUG)
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]: returning app cache container %p",
                this, domDoc.get()));
#endif
        return domDoc->QueryInterface(aIID, aSink);
    }
    else if (aIID.Equals(NS_GET_IID(nsIPrompt)) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
        nsresult rv;
        nsCOMPtr<nsIWindowWatcher> wwatch =
            do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(mScriptGlobal));

        
        

        nsIPrompt *prompt;
        rv = wwatch->GetNewPrompter(window, &prompt);
        NS_ENSURE_SUCCESS(rv, rv);

        *aSink = prompt;
        return NS_OK;
    }
    else if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
             aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
        return NS_SUCCEEDED(
                GetAuthPrompt(PROMPT_NORMAL, aIID, aSink)) ?
                NS_OK : NS_NOINTERFACE;
    }
    else if (aIID.Equals(NS_GET_IID(nsISHistory))) {
        nsCOMPtr<nsISHistory> shistory;
        nsresult
            rv =
            GetSessionHistory(getter_AddRefs(shistory));
        if (NS_SUCCEEDED(rv) && shistory) {
            *aSink = shistory;
            NS_ADDREF((nsISupports *) * aSink);
            return NS_OK;
        }
        return NS_NOINTERFACE;
    }
    else if (aIID.Equals(NS_GET_IID(nsIWebBrowserFind))) {
        nsresult rv = EnsureFind();
        if (NS_FAILED(rv)) return rv;

        *aSink = mFind;
        NS_ADDREF((nsISupports*)*aSink);
        return NS_OK;
    }
    else if (aIID.Equals(NS_GET_IID(nsIEditingSession)) && NS_SUCCEEDED(EnsureEditorData())) {
      nsCOMPtr<nsIEditingSession> editingSession;
      mEditorData->GetEditingSession(getter_AddRefs(editingSession));
      if (editingSession)
      {
        *aSink = editingSession;
        NS_ADDREF((nsISupports *)*aSink);
        return NS_OK;
      }  

      return NS_NOINTERFACE;   
    }
    else if (aIID.Equals(NS_GET_IID(nsIClipboardDragDropHookList)) 
            && NS_SUCCEEDED(EnsureTransferableHookData())) {
        *aSink = mTransferableHookData;
        NS_ADDREF((nsISupports *)*aSink);
        return NS_OK;
    }
    else if (aIID.Equals(NS_GET_IID(nsISelectionDisplay))) {
      nsCOMPtr<nsIPresShell> shell;
      nsresult rv = GetPresShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell)
        return shell->QueryInterface(aIID,aSink);    
    }
    else if (aIID.Equals(NS_GET_IID(nsIDocShellTreeOwner))) {
      nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
      nsresult rv = GetTreeOwner(getter_AddRefs(treeOwner));
      if (NS_SUCCEEDED(rv) && treeOwner)
        return treeOwner->QueryInterface(aIID, aSink);
    }
    else {
        return nsDocLoader::GetInterface(aIID, aSink);
    }

    NS_IF_ADDREF(((nsISupports *) * aSink));
    return *aSink ? NS_OK : NS_NOINTERFACE;
}

PRUint32
nsDocShell::
ConvertDocShellLoadInfoToLoadType(nsDocShellInfoLoadType aDocShellLoadType)
{
    PRUint32 loadType = LOAD_NORMAL;

    switch (aDocShellLoadType) {
    case nsIDocShellLoadInfo::loadNormal:
        loadType = LOAD_NORMAL;
        break;
    case nsIDocShellLoadInfo::loadNormalReplace:
        loadType = LOAD_NORMAL_REPLACE;
        break;
    case nsIDocShellLoadInfo::loadNormalExternal:
        loadType = LOAD_NORMAL_EXTERNAL;
        break;
    case nsIDocShellLoadInfo::loadHistory:
        loadType = LOAD_HISTORY;
        break;
    case nsIDocShellLoadInfo::loadNormalBypassCache:
        loadType = LOAD_NORMAL_BYPASS_CACHE;
        break;
    case nsIDocShellLoadInfo::loadNormalBypassProxy:
        loadType = LOAD_NORMAL_BYPASS_PROXY;
        break;
    case nsIDocShellLoadInfo::loadNormalBypassProxyAndCache:
        loadType = LOAD_NORMAL_BYPASS_PROXY_AND_CACHE;
        break;
    case nsIDocShellLoadInfo::loadReloadNormal:
        loadType = LOAD_RELOAD_NORMAL;
        break;
    case nsIDocShellLoadInfo::loadReloadCharsetChange:
        loadType = LOAD_RELOAD_CHARSET_CHANGE;
        break;
    case nsIDocShellLoadInfo::loadReloadBypassCache:
        loadType = LOAD_RELOAD_BYPASS_CACHE;
        break;
    case nsIDocShellLoadInfo::loadReloadBypassProxy:
        loadType = LOAD_RELOAD_BYPASS_PROXY;
        break;
    case nsIDocShellLoadInfo::loadReloadBypassProxyAndCache:
        loadType = LOAD_RELOAD_BYPASS_PROXY_AND_CACHE;
        break;
    case nsIDocShellLoadInfo::loadLink:
        loadType = LOAD_LINK;
        break;
    case nsIDocShellLoadInfo::loadRefresh:
        loadType = LOAD_REFRESH;
        break;
    case nsIDocShellLoadInfo::loadBypassHistory:
        loadType = LOAD_BYPASS_HISTORY;
        break;
    case nsIDocShellLoadInfo::loadStopContent:
        loadType = LOAD_STOP_CONTENT;
        break;
    case nsIDocShellLoadInfo::loadStopContentAndReplace:
        loadType = LOAD_STOP_CONTENT_AND_REPLACE;
        break;
    default:
        NS_NOTREACHED("Unexpected nsDocShellInfoLoadType value");
    }

    return loadType;
}


nsDocShellInfoLoadType
nsDocShell::ConvertLoadTypeToDocShellLoadInfo(PRUint32 aLoadType)
{
    nsDocShellInfoLoadType docShellLoadType = nsIDocShellLoadInfo::loadNormal;
    switch (aLoadType) {
    case LOAD_NORMAL:
        docShellLoadType = nsIDocShellLoadInfo::loadNormal;
        break;
    case LOAD_NORMAL_REPLACE:
        docShellLoadType = nsIDocShellLoadInfo::loadNormalReplace;
        break;
    case LOAD_NORMAL_EXTERNAL:
        docShellLoadType = nsIDocShellLoadInfo::loadNormalExternal;
        break;
    case LOAD_NORMAL_BYPASS_CACHE:
        docShellLoadType = nsIDocShellLoadInfo::loadNormalBypassCache;
        break;
    case LOAD_NORMAL_BYPASS_PROXY:
        docShellLoadType = nsIDocShellLoadInfo::loadNormalBypassProxy;
        break;
    case LOAD_NORMAL_BYPASS_PROXY_AND_CACHE:
        docShellLoadType = nsIDocShellLoadInfo::loadNormalBypassProxyAndCache;
        break;
    case LOAD_HISTORY:
        docShellLoadType = nsIDocShellLoadInfo::loadHistory;
        break;
    case LOAD_RELOAD_NORMAL:
        docShellLoadType = nsIDocShellLoadInfo::loadReloadNormal;
        break;
    case LOAD_RELOAD_CHARSET_CHANGE:
        docShellLoadType = nsIDocShellLoadInfo::loadReloadCharsetChange;
        break;
    case LOAD_RELOAD_BYPASS_CACHE:
        docShellLoadType = nsIDocShellLoadInfo::loadReloadBypassCache;
        break;
    case LOAD_RELOAD_BYPASS_PROXY:
        docShellLoadType = nsIDocShellLoadInfo::loadReloadBypassProxy;
        break;
    case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:
        docShellLoadType = nsIDocShellLoadInfo::loadReloadBypassProxyAndCache;
        break;
    case LOAD_LINK:
        docShellLoadType = nsIDocShellLoadInfo::loadLink;
        break;
    case LOAD_REFRESH:
        docShellLoadType = nsIDocShellLoadInfo::loadRefresh;
        break;
    case LOAD_BYPASS_HISTORY:
    case LOAD_ERROR_PAGE:
        docShellLoadType = nsIDocShellLoadInfo::loadBypassHistory;
        break;
    case LOAD_STOP_CONTENT:
        docShellLoadType = nsIDocShellLoadInfo::loadStopContent;
        break;
    case LOAD_STOP_CONTENT_AND_REPLACE:
        docShellLoadType = nsIDocShellLoadInfo::loadStopContentAndReplace;
        break;
    default:
        NS_NOTREACHED("Unexpected load type value");
    }

    return docShellLoadType;
}                                                                               




NS_IMETHODIMP
nsDocShell::LoadURI(nsIURI * aURI,
                    nsIDocShellLoadInfo * aLoadInfo,
                    PRUint32 aLoadFlags,
                    PRBool aFirstParty)
{
    NS_PRECONDITION(aLoadInfo || (aLoadFlags & EXTRA_LOAD_FLAGS) == 0,
                    "Unexpected flags");
    NS_PRECONDITION((aLoadFlags & 0xf) == 0, "Should not have these flags set");
    
    
    
    if (IsPrintingOrPP()) {
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsIURI> referrer;
    nsCOMPtr<nsIInputStream> postStream;
    nsCOMPtr<nsIInputStream> headersStream;
    nsCOMPtr<nsISupports> owner;
    PRBool inheritOwner = PR_FALSE;
    PRBool ownerIsExplicit = PR_FALSE;
    PRBool sendReferrer = PR_TRUE;
    nsCOMPtr<nsISHEntry> shEntry;
    nsXPIDLString target;
    PRUint32 loadType = MAKE_LOAD_TYPE(LOAD_NORMAL, aLoadFlags);    

    NS_ENSURE_ARG(aURI);

    
    if (aLoadInfo) {
        aLoadInfo->GetReferrer(getter_AddRefs(referrer));

        nsDocShellInfoLoadType lt = nsIDocShellLoadInfo::loadNormal;
        aLoadInfo->GetLoadType(&lt);
        
        loadType = ConvertDocShellLoadInfoToLoadType(lt);

        aLoadInfo->GetOwner(getter_AddRefs(owner));
        aLoadInfo->GetInheritOwner(&inheritOwner);
        aLoadInfo->GetOwnerIsExplicit(&ownerIsExplicit);
        aLoadInfo->GetSHEntry(getter_AddRefs(shEntry));
        aLoadInfo->GetTarget(getter_Copies(target));
        aLoadInfo->GetPostDataStream(getter_AddRefs(postStream));
        aLoadInfo->GetHeadersStream(getter_AddRefs(headersStream));
        aLoadInfo->GetSendReferrer(&sendReferrer);
    }

#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsCAutoString uristr;
        aURI->GetAsciiSpec(uristr);
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]: loading %s with flags 0x%08x",
                this, uristr.get(), aLoadFlags));
    }
#endif

    if (!shEntry &&
        !LOAD_TYPE_HAS_FLAGS(loadType, LOAD_FLAGS_REPLACE_HISTORY)) {
        
        nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
        GetSameTypeParent(getter_AddRefs(parentAsItem));
        nsCOMPtr<nsIDocShell> parentDS(do_QueryInterface(parentAsItem));
        PRUint32 parentLoadType;

        if (parentDS && parentDS != static_cast<nsIDocShell *>(this)) {
            







            
            
            parentDS->GetLoadType(&parentLoadType);            

            nsCOMPtr<nsIDocShellHistory> parent(do_QueryInterface(parentAsItem));
            if (parent) {
                
                parent->GetChildSHEntry(mChildOffset, getter_AddRefs(shEntry));
                
                
                if (mCurrentURI == nsnull) {
                    
                    
                    if (shEntry && (parentLoadType == LOAD_NORMAL ||
                                    parentLoadType == LOAD_LINK   ||
                                    parentLoadType == LOAD_NORMAL_EXTERNAL)) {
                        
                        
                        
                        
                        
                        PRBool inOnLoadHandler=PR_FALSE;
                        parentDS->GetIsExecutingOnLoadHandler(&inOnLoadHandler);
                        if (inOnLoadHandler) {
                            loadType = LOAD_NORMAL_REPLACE;
                            shEntry = nsnull;
                        }
                    }   
                    else if (parentLoadType == LOAD_REFRESH) {
                        
                        
                        shEntry = nsnull;
                    }
                    else if ((parentLoadType == LOAD_BYPASS_HISTORY) ||
                             (parentLoadType == LOAD_ERROR_PAGE) ||
                              (shEntry && 
                               ((parentLoadType & LOAD_CMD_HISTORY) || 
                                (parentLoadType == LOAD_RELOAD_NORMAL) || 
                                (parentLoadType == LOAD_RELOAD_CHARSET_CHANGE)))) {
                        
                        
                        
                        
                        loadType = parentLoadType;
                    }
                }
                else {
                    
                    
                    
                    
                    
                    PRUint32 parentBusy = BUSY_FLAGS_NONE;
                    PRUint32 selfBusy = BUSY_FLAGS_NONE;
                    parentDS->GetBusyFlags(&parentBusy);                    
                    GetBusyFlags(&selfBusy);
                    if (((parentBusy & BUSY_FLAGS_BUSY) ||
                         (selfBusy & BUSY_FLAGS_BUSY)) &&
                        shEntry) {
                        loadType = LOAD_NORMAL_REPLACE;
                        shEntry = nsnull; 
                    }
                }
            } 
        } 
        else {  
            
            
            
            PRBool inOnLoadHandler=PR_FALSE;
            GetIsExecutingOnLoadHandler(&inOnLoadHandler);
            if (inOnLoadHandler) {
                loadType = LOAD_NORMAL_REPLACE;
            }
        } 
    } 

    if (shEntry) {
#ifdef DEBUG
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
              ("nsDocShell[%p]: loading from session history", this));
#endif

        return LoadHistoryEntry(shEntry, loadType);
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (owner && mItemType != typeChrome) {
        nsCOMPtr<nsIPrincipal> ownerPrincipal = do_QueryInterface(owner);
        PRBool isSystem;
        rv = secMan->IsSystemPrincipal(ownerPrincipal, &isSystem);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isSystem) {
            if (ownerIsExplicit) {
                return NS_ERROR_DOM_SECURITY_ERR;
            }
            owner = nsnull;
            inheritOwner = PR_TRUE;
        }
    }
    if (!owner && !inheritOwner && !ownerIsExplicit) {
        
        rv = secMan->SubjectPrincipalIsSystem(&inheritOwner);
        if (NS_FAILED(rv)) {
            
            inheritOwner = PR_FALSE;
        }
    }

    PRUint32 flags = 0;

    if (inheritOwner)
        flags |= INTERNAL_LOAD_FLAGS_INHERIT_OWNER;

    if (!sendReferrer)
        flags |= INTERNAL_LOAD_FLAGS_DONT_SEND_REFERRER;
            
    if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP)
        flags |= INTERNAL_LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;

    if (aLoadFlags & LOAD_FLAGS_FIRST_LOAD)
        flags |= INTERNAL_LOAD_FLAGS_FIRST_LOAD;

    if (aLoadFlags & LOAD_FLAGS_BYPASS_CLASSIFIER)
        flags |= INTERNAL_LOAD_FLAGS_BYPASS_CLASSIFIER;

    if (aLoadFlags & LOAD_FLAGS_FORCE_ALLOW_COOKIES)
        flags |= INTERNAL_LOAD_FLAGS_FORCE_ALLOW_COOKIES;

    return InternalLoad(aURI,
                        referrer,
                        owner,
                        flags,
                        target.get(),
                        nsnull,         
                        postStream,
                        headersStream,
                        loadType,
                        nsnull,         
                        aFirstParty,
                        nsnull,         
                        nsnull);        
}

NS_IMETHODIMP
nsDocShell::LoadStream(nsIInputStream *aStream, nsIURI * aURI,
                       const nsACString &aContentType,
                       const nsACString &aContentCharset,
                       nsIDocShellLoadInfo * aLoadInfo)
{
    NS_ENSURE_ARG(aStream);

    mAllowKeywordFixup = PR_FALSE;

    
    
    
    nsCOMPtr<nsIURI> uri = aURI;
    if (!uri) {
        
        nsresult rv = NS_OK;
        uri = do_CreateInstance(NS_SIMPLEURI_CONTRACTID, &rv);
        if (NS_FAILED(rv))
            return rv;
        
        
        rv = uri->SetSpec(NS_LITERAL_CSTRING("internal:load-stream"));
        if (NS_FAILED(rv))
            return rv;
    }

    PRUint32 loadType = LOAD_NORMAL;
    if (aLoadInfo) {
        nsDocShellInfoLoadType lt = nsIDocShellLoadInfo::loadNormal;
        (void) aLoadInfo->GetLoadType(&lt);
        
        loadType = ConvertDocShellLoadInfoToLoadType(lt);
    }

    NS_ENSURE_SUCCESS(Stop(nsIWebNavigation::STOP_NETWORK), NS_ERROR_FAILURE);

    mLoadType = loadType;

    
    nsCOMPtr<nsIChannel> channel;
    NS_ENSURE_SUCCESS(NS_NewInputStreamChannel
                      (getter_AddRefs(channel), uri, aStream,
                       aContentType, aContentCharset),
                      NS_ERROR_FAILURE);

    nsCOMPtr<nsIURILoader>
        uriLoader(do_GetService(NS_URI_LOADER_CONTRACTID));
    NS_ENSURE_TRUE(uriLoader, NS_ERROR_FAILURE);

    NS_ENSURE_SUCCESS(DoChannelLoad(channel, uriLoader, PR_FALSE),
                      NS_ERROR_FAILURE);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::CreateLoadInfo(nsIDocShellLoadInfo ** aLoadInfo)
{
    nsDocShellLoadInfo *loadInfo = new nsDocShellLoadInfo();
    NS_ENSURE_TRUE(loadInfo, NS_ERROR_OUT_OF_MEMORY);
    nsCOMPtr<nsIDocShellLoadInfo> localRef(loadInfo);

    *aLoadInfo = localRef;
    NS_ADDREF(*aLoadInfo);
    return NS_OK;
}






NS_IMETHODIMP
nsDocShell::PrepareForNewContentModel()
{
  mEODForCurrentDocument = PR_FALSE;
  return NS_OK;
}


NS_IMETHODIMP
nsDocShell::FirePageHideNotification(PRBool aIsUnload)
{
    if (mContentViewer && !mFiredUnloadEvent) {
        
        
        nsCOMPtr<nsIContentViewer> kungFuDeathGrip(mContentViewer);
        mFiredUnloadEvent = PR_TRUE;

        mContentViewer->PageHide(aIsUnload);

        nsAutoTArray<nsCOMPtr<nsIDocShell>, 8> kids;
        PRInt32 i, n = mChildList.Count();
        kids.SetCapacity(n);
        for (i = 0; i < n; i++) {
            kids.AppendElement(do_QueryInterface(ChildAt(i)));
        }

        n = kids.Length();
        for (i = 0; i < n; ++i) {
            if (kids[i]) {
                kids[i]->FirePageHideNotification(aIsUnload);
            }
        }
        
        
        DetachEditorFromWindow();
    }

    return NS_OK;
}











PRBool
nsDocShell::ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                           nsIDocShellTreeItem* aTargetTreeItem)
{
    nsCOMPtr<nsIScriptSecurityManager> securityManager =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    NS_ENSURE_TRUE(securityManager, PR_FALSE);

    nsCOMPtr<nsIPrincipal> subjectPrincipal;
    nsresult rv =
        securityManager->GetSubjectPrincipal(getter_AddRefs(subjectPrincipal));
    NS_ENSURE_SUCCESS(rv, PR_FALSE);

    if (subjectPrincipal) {
        
        
        PRBool ubwEnabled = PR_FALSE;
        rv = securityManager->IsCapabilityEnabled("UniversalBrowserWrite",
                                                  &ubwEnabled);
        NS_ENSURE_SUCCESS(rv, PR_FALSE);

        if (ubwEnabled) {
            return PR_TRUE;
        }
    }

    
    nsCOMPtr<nsIDOMDocument> originDOMDocument =
        do_GetInterface(aOriginTreeItem);
    nsCOMPtr<nsIDocument> originDocument(do_QueryInterface(originDOMDocument));
    NS_ENSURE_TRUE(originDocument, PR_FALSE);
    
    
    nsCOMPtr<nsIDOMDocument> targetDOMDocument =
        do_GetInterface(aTargetTreeItem);
    nsCOMPtr<nsIDocument> targetDocument(do_QueryInterface(targetDOMDocument));
    NS_ENSURE_TRUE(targetDocument, PR_FALSE);

    PRBool equal;
    rv = originDocument->NodePrincipal()->
            Equals(targetDocument->NodePrincipal(), &equal);
    if (NS_SUCCEEDED(rv) && equal) {
        return PR_TRUE;
    }

    
    PRBool originIsFile = PR_FALSE;
    PRBool targetIsFile = PR_FALSE;
    nsCOMPtr<nsIURI> originURI;
    nsCOMPtr<nsIURI> targetURI;
    nsCOMPtr<nsIURI> innerOriginURI;
    nsCOMPtr<nsIURI> innerTargetURI;

    rv = originDocument->NodePrincipal()->GetURI(getter_AddRefs(originURI));
    if (NS_SUCCEEDED(rv) && originURI)
        innerOriginURI = NS_GetInnermostURI(originURI);

    rv = targetDocument->NodePrincipal()->GetURI(getter_AddRefs(targetURI));
    if (NS_SUCCEEDED(rv) && targetURI)
        innerTargetURI = NS_GetInnermostURI(targetURI);

    return innerOriginURI && innerTargetURI &&
        NS_SUCCEEDED(innerOriginURI->SchemeIs("file", &originIsFile)) &&
        NS_SUCCEEDED(innerTargetURI->SchemeIs("file", &targetIsFile)) &&
        originIsFile && targetIsFile;
}

NS_IMETHODIMP
nsDocShell::GetEldestPresContext(nsPresContext** aPresContext)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aPresContext);
    *aPresContext = nsnull;

    nsCOMPtr<nsIContentViewer> viewer = mContentViewer;
    while (viewer) {
        nsCOMPtr<nsIContentViewer> prevViewer;
        viewer->GetPreviousViewer(getter_AddRefs(prevViewer));
        if (prevViewer)
            viewer = prevViewer;
        else {
            nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(viewer));
            if (docv)
                rv = docv->GetPresContext(aPresContext);
            break;
        }
    }

    return rv;
}

NS_IMETHODIMP
nsDocShell::GetPresContext(nsPresContext ** aPresContext)
{
    NS_ENSURE_ARG_POINTER(aPresContext);
    *aPresContext = nsnull;

    if (!mContentViewer)
      return NS_OK;

    nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(mContentViewer));
    NS_ENSURE_TRUE(docv, NS_ERROR_NO_INTERFACE);

    return docv->GetPresContext(aPresContext);
}

NS_IMETHODIMP
nsDocShell::GetPresShell(nsIPresShell ** aPresShell)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aPresShell);
    *aPresShell = nsnull;

    nsCOMPtr<nsPresContext> presContext;
    (void) GetPresContext(getter_AddRefs(presContext));

    if (presContext) {
        NS_IF_ADDREF(*aPresShell = presContext->GetPresShell());
    }

    return rv;
}

NS_IMETHODIMP
nsDocShell::GetEldestPresShell(nsIPresShell** aPresShell)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aPresShell);
    *aPresShell = nsnull;

    nsCOMPtr<nsPresContext> presContext;
    (void) GetEldestPresContext(getter_AddRefs(presContext));

    if (presContext) {
        NS_IF_ADDREF(*aPresShell = presContext->GetPresShell());
    }

    return rv;
}

NS_IMETHODIMP
nsDocShell::GetContentViewer(nsIContentViewer ** aContentViewer)
{
    NS_ENSURE_ARG_POINTER(aContentViewer);

    *aContentViewer = mContentViewer;
    NS_IF_ADDREF(*aContentViewer);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetChromeEventHandler(nsIDOMEventTarget* aChromeEventHandler)
{
    nsCOMPtr<nsPIDOMEventTarget> piTarget =
      do_QueryInterface(aChromeEventHandler);
    
    mChromeEventHandler = piTarget;

    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
    if (win) {
        win->SetChromeEventHandler(piTarget);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChromeEventHandler(nsIDOMEventTarget** aChromeEventHandler)
{
    NS_ENSURE_ARG_POINTER(aChromeEventHandler);
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mChromeEventHandler);
    target.swap(*aChromeEventHandler);
    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::SetCurrentURI(nsIURI *aURI)
{
    SetCurrentURI(aURI, nsnull, PR_TRUE);
    return NS_OK;
}

PRBool
nsDocShell::SetCurrentURI(nsIURI *aURI, nsIRequest *aRequest,
                          PRBool aFireOnLocationChange)
{
#ifdef PR_LOGGING
    if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
        nsCAutoString spec;
        if (aURI)
            aURI->GetSpec(spec);
        PR_LogPrint("DOCSHELL %p SetCurrentURI %s\n", this, spec.get());
    }
#endif

    
    
    if (mLoadType == LOAD_ERROR_PAGE) {
        return PR_FALSE;
    }

    mCurrentURI = NS_TryToMakeImmutable(aURI);
    
    PRBool isRoot = PR_FALSE;   
    PRBool isSubFrame = PR_FALSE;  

    nsCOMPtr<nsIDocShellTreeItem> root;

    GetSameTypeRootTreeItem(getter_AddRefs(root));
    if (root.get() == static_cast<nsIDocShellTreeItem *>(this)) 
    {
        
        isRoot = PR_TRUE;
    }
    if (mLSHE) {
        mLSHE->GetIsSubFrame(&isSubFrame);
    }

    if (!isSubFrame && !isRoot) {
      




      return PR_FALSE; 
    }

    if (aFireOnLocationChange) {
        FireOnLocationChange(this, aRequest, aURI);
    }
    return !aFireOnLocationChange;
}

NS_IMETHODIMP
nsDocShell::GetCharset(char** aCharset)
{
    NS_ENSURE_ARG_POINTER(aCharset);
    *aCharset = nsnull; 

    nsCOMPtr<nsIPresShell> presShell;
    GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
    nsIDocument *doc = presShell->GetDocument();
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
    *aCharset = ToNewCString(doc->GetDocumentCharacterSet());
    if (!*aCharset) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetCharset(const char* aCharset)
{
    
    nsCOMPtr<nsIContentViewer> viewer;
    GetContentViewer(getter_AddRefs(viewer));
    if (viewer) {
      nsCOMPtr<nsIMarkupDocumentViewer> muDV(do_QueryInterface(viewer));
      if (muDV) {
        NS_ENSURE_SUCCESS(muDV->SetDefaultCharacterSet(nsDependentCString(aCharset)),
                          NS_ERROR_FAILURE);
      }
    }

    
    nsCOMPtr<nsIDocumentCharsetInfo> dcInfo;
    GetDocumentCharsetInfo(getter_AddRefs(dcInfo));
    if (dcInfo) {
      nsCOMPtr<nsIAtom> csAtom;
      csAtom = do_GetAtom(aCharset);
      dcInfo->SetForcedCharset(csAtom);
    }

    return NS_OK;
} 

NS_IMETHODIMP
nsDocShell::GetDocumentCharsetInfo(nsIDocumentCharsetInfo **
                                   aDocumentCharsetInfo)
{
    NS_ENSURE_ARG_POINTER(aDocumentCharsetInfo);

    
    if (!mDocumentCharsetInfo) {
        mDocumentCharsetInfo = do_CreateInstance(NS_DOCUMENTCHARSETINFO_CONTRACTID);
        if (!mDocumentCharsetInfo)
            return NS_ERROR_FAILURE;
    }

    *aDocumentCharsetInfo = mDocumentCharsetInfo;
    NS_IF_ADDREF(*aDocumentCharsetInfo);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetDocumentCharsetInfo(nsIDocumentCharsetInfo *
                                   aDocumentCharsetInfo)
{
    mDocumentCharsetInfo = aDocumentCharsetInfo;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChannelIsUnsafe(PRBool *aUnsafe)
{
    *aUnsafe = PR_FALSE;

    nsCOMPtr<nsIChannel> channel;
    GetCurrentDocumentChannel(getter_AddRefs(channel));
    if (!channel) {
        return NS_OK;
    }

    nsCOMPtr<nsIJARChannel> jarChannel = do_QueryInterface(channel);
    if (!jarChannel) {
        return NS_OK;
    }

    return jarChannel->GetIsUnsafe(aUnsafe);
}

NS_IMETHODIMP
nsDocShell::GetAllowPlugins(PRBool * aAllowPlugins)
{
    NS_ENSURE_ARG_POINTER(aAllowPlugins);

    *aAllowPlugins = mAllowPlugins;
    if (!mAllowPlugins) {
        return NS_OK;
    }

    PRBool unsafe;
    *aAllowPlugins = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowPlugins(PRBool aAllowPlugins)
{
    mAllowPlugins = aAllowPlugins;
    
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowJavascript(PRBool * aAllowJavascript)
{
    NS_ENSURE_ARG_POINTER(aAllowJavascript);

    *aAllowJavascript = mAllowJavascript;
    if (!mAllowJavascript) {
        return NS_OK;
    }

    PRBool unsafe;
    *aAllowJavascript = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowJavascript(PRBool aAllowJavascript)
{
    mAllowJavascript = aAllowJavascript;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowMetaRedirects(PRBool * aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    *aReturn = mAllowMetaRedirects;
    if (!mAllowMetaRedirects) {
        return NS_OK;
    }

    PRBool unsafe;
    *aReturn = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowMetaRedirects(PRBool aValue)
{
    mAllowMetaRedirects = aValue;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowSubframes(PRBool * aAllowSubframes)
{
    NS_ENSURE_ARG_POINTER(aAllowSubframes);

    *aAllowSubframes = mAllowSubframes;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowSubframes(PRBool aAllowSubframes)
{
    mAllowSubframes = aAllowSubframes;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowImages(PRBool * aAllowImages)
{
    NS_ENSURE_ARG_POINTER(aAllowImages);

    *aAllowImages = mAllowImages;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowImages(PRBool aAllowImages)
{
    mAllowImages = aAllowImages;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowDNSPrefetch(PRBool * aAllowDNSPrefetch)
{
    *aAllowDNSPrefetch = mAllowDNSPrefetch;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowDNSPrefetch(PRBool aAllowDNSPrefetch)
{
    mAllowDNSPrefetch = aAllowDNSPrefetch;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDocShellEnumerator(PRInt32 aItemType, PRInt32 aDirection, nsISimpleEnumerator **outEnum)
{
    NS_ENSURE_ARG_POINTER(outEnum);
    *outEnum = nsnull;
    
    nsRefPtr<nsDocShellEnumerator> docShellEnum;
    if (aDirection == ENUMERATE_FORWARDS)
        docShellEnum = new nsDocShellForwardsEnumerator;
    else
        docShellEnum = new nsDocShellBackwardsEnumerator;
    
    if (!docShellEnum) return NS_ERROR_OUT_OF_MEMORY;
    
    nsresult rv = docShellEnum->SetEnumDocShellType(aItemType);
    if (NS_FAILED(rv)) return rv;

    rv = docShellEnum->SetEnumerationRootItem((nsIDocShellTreeItem *)this);
    if (NS_FAILED(rv)) return rv;

    rv = docShellEnum->First();
    if (NS_FAILED(rv)) return rv;

    rv = docShellEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator), (void **)outEnum);

    return rv;
}

NS_IMETHODIMP
nsDocShell::GetAppType(PRUint32 * aAppType)
{
    *aAppType = mAppType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAppType(PRUint32 aAppType)
{
    mAppType = aAppType;
    if (mAppType == APP_TYPE_MAIL || mAppType == APP_TYPE_EDITOR) {
        SetAllowDNSPrefetch(PR_FALSE);
    }
    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::GetAllowAuth(PRBool * aAllowAuth)
{
    *aAllowAuth = mAllowAuth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowAuth(PRBool aAllowAuth)
{
    mAllowAuth = aAllowAuth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetZoom(float *zoom)
{
    NS_ENSURE_ARG_POINTER(zoom);
    *zoom = 1.0f;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetZoom(float zoom)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetMarginWidth(PRInt32 * aWidth)
{
    NS_ENSURE_ARG_POINTER(aWidth);

    *aWidth = mMarginWidth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMarginWidth(PRInt32 aWidth)
{
    mMarginWidth = aWidth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMarginHeight(PRInt32 * aHeight)
{
    NS_ENSURE_ARG_POINTER(aHeight);

    *aHeight = mMarginHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMarginHeight(PRInt32 aHeight)
{
    mMarginHeight = aHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetBusyFlags(PRUint32 * aBusyFlags)
{
    NS_ENSURE_ARG_POINTER(aBusyFlags);

    *aBusyFlags = mBusyFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::TabToTreeOwner(PRBool aForward, PRBool* aTookFocus)
{
    NS_ENSURE_ARG_POINTER(aTookFocus);
    
    nsCOMPtr<nsIWebBrowserChromeFocus> chromeFocus = do_GetInterface(mTreeOwner);
    if (chromeFocus) {
        if (aForward)
            *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusNextElement());
        else
            *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusPrevElement());
    } else
        *aTookFocus = PR_FALSE;
    
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSecurityUI(nsISecureBrowserUI **aSecurityUI)
{
    NS_IF_ADDREF(*aSecurityUI = mSecurityUI);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetSecurityUI(nsISecureBrowserUI *aSecurityUI)
{
    mSecurityUI = aSecurityUI;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetUseErrorPages(PRBool *aUseErrorPages)
{
    *aUseErrorPages = mUseErrorPages;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetUseErrorPages(PRBool aUseErrorPages)
{
    
    if (mObserveErrorPages) {
        nsCOMPtr<nsIPrefBranch2> prefs(do_QueryInterface(mPrefs));
        if (prefs) {
            prefs->RemoveObserver("browser.xul.error_pages.enabled", this);
            mObserveErrorPages = PR_FALSE;
        }
    }
    mUseErrorPages = aUseErrorPages;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPreviousTransIndex(PRInt32 *aPreviousTransIndex)
{
    *aPreviousTransIndex = mPreviousTransIndex;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLoadedTransIndex(PRInt32 *aLoadedTransIndex)
{
    *aLoadedTransIndex = mLoadedTransIndex;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::HistoryPurged(PRInt32 aNumEntries)
{
    
    
    
    
    
    mPreviousTransIndex = PR_MAX(-1, mPreviousTransIndex - aNumEntries);
    mLoadedTransIndex = PR_MAX(0, mLoadedTransIndex - aNumEntries);

    PRInt32 count = mChildList.Count();
    for (PRInt32 i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell) {
            shell->HistoryPurged(aNumEntries);
        }
    }

    return NS_OK;
}

static
nsresult
GetPrincipalDomain(nsIPrincipal* aPrincipal, nsACString& aDomain)
{
  aDomain.Truncate();

  nsCOMPtr<nsIURI> codebaseURI;
  nsresult rv = aPrincipal->GetDomain(getter_AddRefs(codebaseURI));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!codebaseURI) {
     rv = aPrincipal->GetURI(getter_AddRefs(codebaseURI));
     NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!codebaseURI)
     return NS_OK;

  nsCOMPtr<nsIURI> innerURI = NS_GetInnermostURI(codebaseURI);
  NS_ASSERTION(innerURI, "Failed to get innermost URI");
  NS_ENSURE_SUCCESS(rv, rv);

  rv = innerURI->GetAsciiHost(aDomain);
  if (NS_FAILED(rv))
      return rv;

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSessionStorageForPrincipal(nsIPrincipal* aPrincipal,
                                          PRBool aCreate,
                                          nsIDOMStorage** aStorage)
{
    NS_ENSURE_ARG_POINTER(aStorage);
    *aStorage = nsnull;

    if (!aPrincipal)
        return NS_OK;

    nsresult rv;

    nsCOMPtr<nsIDocShellTreeItem> topItem;
    rv = GetSameTypeRootTreeItem(getter_AddRefs(topItem));
    if (NS_FAILED(rv))
        return rv;

    if (!topItem)
        return NS_ERROR_FAILURE;

    nsDocShell* topDocShell = static_cast<nsDocShell*>(topItem.get());
    if (topDocShell != this)
        return topDocShell->GetSessionStorageForPrincipal(aPrincipal, aCreate,
                                                          aStorage);

    nsCAutoString currentDomain;
    rv = GetPrincipalDomain(aPrincipal, currentDomain);
    if (NS_FAILED(rv))
        return rv;

    if (currentDomain.IsEmpty())
        return NS_OK;

    if (!mStorages.Get(currentDomain, aStorage) && aCreate) {
        nsCOMPtr<nsIDOMStorage> newstorage =
            do_CreateInstance("@mozilla.org/dom/storage;2");
        if (!newstorage)
            return NS_ERROR_OUT_OF_MEMORY;

        nsCOMPtr<nsPIDOMStorage> pistorage = do_QueryInterface(newstorage);
        if (!pistorage)
            return NS_ERROR_FAILURE;
        rv = pistorage->InitAsSessionStorage(aPrincipal);
        if (NS_FAILED(rv))
            return rv;

        if (!mStorages.Put(currentDomain, newstorage))
            return NS_ERROR_OUT_OF_MEMORY;

        newstorage.swap(*aStorage);
        return NS_OK;
    }

    nsCOMPtr<nsPIDOMStorage> piStorage = do_QueryInterface(*aStorage);
    if (piStorage) {
        PRBool canAccess = piStorage->CanAccess(aPrincipal);
        NS_ASSERTION(canAccess,
                     "GetSessionStorageForPrincipal got a storage "
                     "that could not be accessed!");
        if (!canAccess) {
            NS_RELEASE(*aStorage);
            return NS_ERROR_DOM_SECURITY_ERR;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSessionStorageForURI(nsIURI* aURI,
                                    nsIDOMStorage** aStorage)
{
    return GetSessionStorageForURI(aURI, PR_TRUE, aStorage);
}

nsresult
nsDocShell::GetSessionStorageForURI(nsIURI* aURI,
                                    PRBool aCreate,
                                    nsIDOMStorage** aStorage)
{
    NS_ENSURE_ARG(aURI);
    NS_ENSURE_ARG_POINTER(aStorage);

    *aStorage = nsnull;

    nsresult rv;

    nsCOMPtr<nsIScriptSecurityManager> securityManager =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIPrincipal> principal;
    rv = securityManager->GetCodebasePrincipal(aURI, getter_AddRefs(principal));
    if (NS_FAILED(rv))
        return rv;

    return GetSessionStorageForPrincipal(principal, aCreate, aStorage);
}

nsresult
nsDocShell::AddSessionStorage(nsIPrincipal* aPrincipal,
                              nsIDOMStorage* aStorage)
{
    NS_ENSURE_ARG_POINTER(aStorage);

    if (!aPrincipal)
        return NS_OK;

    nsCOMPtr<nsIDocShellTreeItem> topItem;
    nsresult rv = GetSameTypeRootTreeItem(getter_AddRefs(topItem));
    if (NS_FAILED(rv))
        return rv;

    if (topItem) {
        nsCOMPtr<nsIDocShell> topDocShell = do_QueryInterface(topItem);
        if (topDocShell == this) {
            nsCAutoString currentDomain;
            rv = GetPrincipalDomain(aPrincipal, currentDomain);
            if (NS_FAILED(rv))
                return rv;

            if (currentDomain.IsEmpty())
                return NS_ERROR_FAILURE;

            
            if (mStorages.GetWeak(currentDomain))
                return NS_ERROR_NOT_AVAILABLE;

            if (!mStorages.Put(currentDomain, aStorage))
                return NS_ERROR_OUT_OF_MEMORY;
        }
        else {
            return topDocShell->AddSessionStorage(aPrincipal, aStorage);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetCurrentDocumentChannel(nsIChannel** aResult)
{
    *aResult = nsnull;
    if (!mContentViewer)
        return NS_OK;

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsresult rv = mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
    if (doc) {
      *aResult = doc->GetChannel();
      NS_IF_ADDREF(*aResult);
    }
  
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetName(PRUnichar ** aName)
{
    NS_ENSURE_ARG_POINTER(aName);
    *aName = ToNewUnicode(mName);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetName(const PRUnichar * aName)
{
    mName = aName;              
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::NameEquals(const PRUnichar *aName, PRBool *_retval)
{
    NS_ENSURE_ARG_POINTER(aName);
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mName.Equals(aName);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetItemType(PRInt32 * aItemType)
{
    NS_ENSURE_ARG_POINTER(aItemType);

    *aItemType = mItemType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetItemType(PRInt32 aItemType)
{
    NS_ENSURE_ARG((aItemType == typeChrome) || (typeContent == aItemType));

    
    
    nsCOMPtr<nsIDocumentLoader> docLoaderService =
        do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(docLoaderService, NS_ERROR_UNEXPECTED);
    
    NS_ENSURE_STATE(!mParent || mParent == docLoaderService);

    mItemType = aItemType;

    
    mAllowAuth = mItemType == typeContent; 

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParent(nsIDocShellTreeItem ** aParent)
{
    if (!mParent) {
        *aParent = nsnull;
    } else {
        CallQueryInterface(mParent, aParent);
    }
    
    
    return NS_OK;
}

nsresult
nsDocShell::SetDocLoaderParent(nsDocLoader * aParent)
{
    nsDocLoader::SetDocLoaderParent(aParent);

    
    nsISupports* parent = GetAsSupports(aParent);

    
    
    nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(parent));
    if (parentAsDocShell)
    {
        PRBool value;
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowPlugins(&value)))
        {
            SetAllowPlugins(value);
        }
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowJavascript(&value)))
        {
            SetAllowJavascript(value);
        }
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowMetaRedirects(&value)))
        {
            SetAllowMetaRedirects(value);
        }
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowSubframes(&value)))
        {
            SetAllowSubframes(value);
        }
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowImages(&value)))
        {
            SetAllowImages(value);
        }
        if (NS_FAILED(parentAsDocShell->GetAllowDNSPrefetch(&value))) {
            value = PR_FALSE;
        }
        SetAllowDNSPrefetch(value);
    }

    nsCOMPtr<nsIURIContentListener> parentURIListener(do_GetInterface(parent));
    if (parentURIListener)
        mContentListener->SetParentContentListener(parentURIListener);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSameTypeParent(nsIDocShellTreeItem ** aParent)
{
    NS_ENSURE_ARG_POINTER(aParent);
    *aParent = nsnull;

    nsCOMPtr<nsIDocShellTreeItem> parent =
        do_QueryInterface(GetAsSupports(mParent));
    if (!parent)
        return NS_OK;

    PRInt32 parentType;
    NS_ENSURE_SUCCESS(parent->GetItemType(&parentType), NS_ERROR_FAILURE);

    if (parentType == mItemType) {
        parent.swap(*aParent);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRootTreeItem(nsIDocShellTreeItem ** aRootTreeItem)
{
    NS_ENSURE_ARG_POINTER(aRootTreeItem);
    *aRootTreeItem = static_cast<nsIDocShellTreeItem *>(this);

    nsCOMPtr<nsIDocShellTreeItem> parent;
    NS_ENSURE_SUCCESS(GetParent(getter_AddRefs(parent)), NS_ERROR_FAILURE);
    while (parent) {
        *aRootTreeItem = parent;
        NS_ENSURE_SUCCESS((*aRootTreeItem)->GetParent(getter_AddRefs(parent)),
                          NS_ERROR_FAILURE);
    }
    NS_ADDREF(*aRootTreeItem);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSameTypeRootTreeItem(nsIDocShellTreeItem ** aRootTreeItem)
{
    NS_ENSURE_ARG_POINTER(aRootTreeItem);
    *aRootTreeItem = static_cast<nsIDocShellTreeItem *>(this);

    nsCOMPtr<nsIDocShellTreeItem> parent;
    NS_ENSURE_SUCCESS(GetSameTypeParent(getter_AddRefs(parent)),
                      NS_ERROR_FAILURE);
    while (parent) {
        *aRootTreeItem = parent;
        NS_ENSURE_SUCCESS((*aRootTreeItem)->
                          GetSameTypeParent(getter_AddRefs(parent)),
                          NS_ERROR_FAILURE);
    }
    NS_ADDREF(*aRootTreeItem);
    return NS_OK;
}


PRBool
nsDocShell::CanAccessItem(nsIDocShellTreeItem* aTargetItem,
                          nsIDocShellTreeItem* aAccessingItem,
                          PRBool aConsiderOpener)
{
    NS_PRECONDITION(aTargetItem, "Must have target item!");

    if (!gValidateOrigin || !aAccessingItem) {
        
        return PR_TRUE;
    }

    
    

    
    
    
    
    
    

    
    
    
    
    
    
    
    

    if (aTargetItem == aAccessingItem) {
        
        return PR_TRUE;  
    }

    nsCOMPtr<nsIDocShellTreeItem> accessingRoot;
    aAccessingItem->GetSameTypeRootTreeItem(getter_AddRefs(accessingRoot));

    if (aTargetItem == accessingRoot) {
        
        return PR_TRUE;
    }

    
    nsCOMPtr<nsIDocShellTreeItem> target = aTargetItem;
    do {
        if (ValidateOrigin(aAccessingItem, target)) {
            return PR_TRUE;
        }
            
        nsCOMPtr<nsIDocShellTreeItem> parent;
        target->GetSameTypeParent(getter_AddRefs(parent));
        parent.swap(target);
    } while (target);

    nsCOMPtr<nsIDocShellTreeItem> targetRoot;
    aTargetItem->GetSameTypeRootTreeItem(getter_AddRefs(targetRoot));

    if (aTargetItem != targetRoot) {
        
        
        
        return PR_FALSE;
    }

    if (!aConsiderOpener) {
        
        return PR_FALSE;
    }

    nsCOMPtr<nsIDOMWindow> targetWindow(do_GetInterface(aTargetItem));
    nsCOMPtr<nsIDOMWindowInternal> targetInternal(do_QueryInterface(targetWindow));
    if (!targetInternal) {
        NS_ERROR("This should not happen, really");
        return PR_FALSE;
    }

    nsCOMPtr<nsIDOMWindowInternal> targetOpener;
    targetInternal->GetOpener(getter_AddRefs(targetOpener));
    nsCOMPtr<nsIWebNavigation> openerWebNav(do_GetInterface(targetOpener));
    nsCOMPtr<nsIDocShellTreeItem> openerItem(do_QueryInterface(openerWebNav));

    if (!openerItem) {
        return PR_FALSE;
    }

    return CanAccessItem(openerItem, aAccessingItem, PR_FALSE);    
}

static PRBool
ItemIsActive(nsIDocShellTreeItem *aItem)
{
    nsCOMPtr<nsIDOMWindow> tmp(do_GetInterface(aItem));
    nsCOMPtr<nsIDOMWindowInternal> window(do_QueryInterface(tmp));

    if (window) {
        PRBool isClosed;

        if (NS_SUCCEEDED(window->GetClosed(&isClosed)) && !isClosed) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}

NS_IMETHODIMP
nsDocShell::FindItemWithName(const PRUnichar * aName,
                             nsISupports * aRequestor,
                             nsIDocShellTreeItem * aOriginalRequestor,
                             nsIDocShellTreeItem ** _retval)
{
    NS_ENSURE_ARG(aName);
    NS_ENSURE_ARG_POINTER(_retval);

    
    *_retval = nsnull;

    if (!*aName)
        return NS_OK;

    if (!aRequestor)
    {
        nsCOMPtr<nsIDocShellTreeItem> foundItem;

        
        
        

        nsDependentString name(aName);
        if (name.LowerCaseEqualsLiteral("_self")) {
            foundItem = this;
        }
        else if (name.LowerCaseEqualsLiteral("_blank"))
        {
            
            
            return NS_OK;
        }
        else if (name.LowerCaseEqualsLiteral("_parent"))
        {
            GetSameTypeParent(getter_AddRefs(foundItem));
            if(!foundItem)
                foundItem = this;
        }
        else if (name.LowerCaseEqualsLiteral("_top"))
        {
            GetSameTypeRootTreeItem(getter_AddRefs(foundItem));
            NS_ASSERTION(foundItem, "Must have this; worst case it's us!");
        }
        
        
        else if (name.LowerCaseEqualsLiteral("_content") ||
                 name.EqualsLiteral("_main"))
        {
            
            
            nsCOMPtr<nsIDocShellTreeItem> root;
            GetSameTypeRootTreeItem(getter_AddRefs(root));
            if (mTreeOwner) {
                NS_ASSERTION(root, "Must have this; worst case it's us!");
                mTreeOwner->FindItemWithName(aName, root, aOriginalRequestor,
                                             getter_AddRefs(foundItem));
            }
#ifdef DEBUG
            else {
                NS_ERROR("Someone isn't setting up the tree owner.  "
                         "You might like to try that.  "
                         "Things will.....you know, work.");
                
                
                
                
                
                
            }
#endif
        }

        if (foundItem && !CanAccessItem(foundItem, aOriginalRequestor)) {
            foundItem = nsnull;
        }

        if (foundItem) {
            
            
            

            foundItem.swap(*_retval);
            return NS_OK;
        }
    }

    
        
    
    if (mName.Equals(aName) && ItemIsActive(this) &&
        CanAccessItem(this, aOriginalRequestor)) {
        NS_ADDREF(*_retval = this);
        return NS_OK;
    }

    
    
    nsCOMPtr<nsIDocShellTreeItem> reqAsTreeItem(do_QueryInterface(aRequestor));

    
    
#ifdef DEBUG
    nsresult rv =
#endif
    FindChildWithName(aName, PR_TRUE, PR_TRUE, reqAsTreeItem,
                      aOriginalRequestor, _retval);
    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "FindChildWithName should not be failing here.");
    if (*_retval)
        return NS_OK;
        
    
    
    
    
    
    nsCOMPtr<nsIDocShellTreeItem> parentAsTreeItem =
        do_QueryInterface(GetAsSupports(mParent));
    if (parentAsTreeItem) {
        if (parentAsTreeItem == reqAsTreeItem)
            return NS_OK;

        PRInt32 parentType;
        parentAsTreeItem->GetItemType(&parentType);
        if (parentType == mItemType) {
            return parentAsTreeItem->
                FindItemWithName(aName,
                                 static_cast<nsIDocShellTreeItem*>
                                            (this),
                                 aOriginalRequestor,
                                 _retval);
        }
    }

    
    

    
    nsCOMPtr<nsIDocShellTreeOwner>
        reqAsTreeOwner(do_QueryInterface(aRequestor));

    if (mTreeOwner && mTreeOwner != reqAsTreeOwner) {
        return mTreeOwner->
            FindItemWithName(aName, this, aOriginalRequestor, _retval);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetTreeOwner(nsIDocShellTreeOwner ** aTreeOwner)
{
    NS_ENSURE_ARG_POINTER(aTreeOwner);

    *aTreeOwner = mTreeOwner;
    NS_IF_ADDREF(*aTreeOwner);
    return NS_OK;
}

#ifdef DEBUG_DOCSHELL_FOCUS
static void 
PrintDocTree(nsIDocShellTreeItem * aParentNode, int aLevel)
{
  for (PRInt32 i=0;i<aLevel;i++) printf("  ");

  PRInt32 childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentNode));
  PRInt32 type;
  aParentNode->GetItemType(&type);
  nsCOMPtr<nsIPresShell> presShell;
  parentAsDocShell->GetPresShell(getter_AddRefs(presShell));
  nsCOMPtr<nsPresContext> presContext;
  parentAsDocShell->GetPresContext(getter_AddRefs(presContext));
  nsIDocument *doc = presShell->GetDocument();

  nsCOMPtr<nsIDOMWindowInternal> domwin(doc->GetWindow());

  nsCOMPtr<nsIWidget> widget;
  nsIViewManager* vm = presShell->GetViewManager();
  if (vm) {
    vm->GetWidget(getter_AddRefs(widget));
  }
  nsIContent* rootContent = doc->GetRootContent();

  printf("DS %p  Ty %s  Doc %p DW %p EM %p CN %p\n",  
    (void*)parentAsDocShell.get(), 
    type==nsIDocShellTreeItem::typeChrome?"Chr":"Con", 
     (void*)doc, (void*)domwin.get(),
     (void*)presContext->EventStateManager(), (void*)rootContent);

  if (childWebshellCount > 0) {
    for (PRInt32 i=0;i<childWebshellCount;i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentNode->GetChildAt(i, getter_AddRefs(child));
      PrintDocTree(child, aLevel+1);
    }
  }
}

static void 
PrintDocTree(nsIDocShellTreeItem * aParentNode)
{
  NS_ASSERTION(aParentNode, "Pointer is null!");

  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  aParentNode->GetParent(getter_AddRefs(parentItem));
  while (parentItem) {
    nsCOMPtr<nsIDocShellTreeItem>tmp;
    parentItem->GetParent(getter_AddRefs(tmp));
    if (!tmp) {
      break;
    }
    parentItem = tmp;
  }

  if (!parentItem) {
    parentItem = aParentNode;
  }

  PrintDocTree(parentItem, 0);
}
#endif

NS_IMETHODIMP
nsDocShell::SetTreeOwner(nsIDocShellTreeOwner * aTreeOwner)
{
#ifdef DEBUG_DOCSHELL_FOCUS
    nsCOMPtr<nsIDocShellTreeItem> item(do_QueryInterface(aTreeOwner));
    if (item) {
      PrintDocTree(item);
    }
#endif

    
    if (!IsFrame()) {
        nsCOMPtr<nsIWebProgress> webProgress =
            do_QueryInterface(GetAsSupports(this));

        if (webProgress) {
            nsCOMPtr<nsIWebProgressListener>
                oldListener(do_QueryInterface(mTreeOwner));
            nsCOMPtr<nsIWebProgressListener>
                newListener(do_QueryInterface(aTreeOwner));

            if (oldListener) {
                webProgress->RemoveProgressListener(oldListener);
            }

            if (newListener) {
                webProgress->AddProgressListener(newListener,
                                                 nsIWebProgress::NOTIFY_ALL);
            }
        }
    }

    mTreeOwner = aTreeOwner;    

    PRInt32 i, n = mChildList.Count();
    for (i = 0; i < n; i++) {
        nsCOMPtr<nsIDocShellTreeItem> child = do_QueryInterface(ChildAt(i));
        NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);
        PRInt32 childType = ~mItemType; 
        child->GetItemType(&childType); 
        if (childType == mItemType)
            child->SetTreeOwner(aTreeOwner);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetChildOffset(PRUint32 aChildOffset)
{
    mChildOffset = aChildOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsInUnload(PRBool* aIsInUnload)
{
    *aIsInUnload = mFiredUnloadEvent;
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetChildCount(PRInt32 * aChildCount)
{
    NS_ENSURE_ARG_POINTER(aChildCount);
    *aChildCount = mChildList.Count();
    return NS_OK;
}



NS_IMETHODIMP
nsDocShell::AddChild(nsIDocShellTreeItem * aChild)
{
    NS_ENSURE_ARG_POINTER(aChild);

    nsRefPtr<nsDocLoader> childAsDocLoader = GetAsDocLoader(aChild);
    NS_ENSURE_TRUE(childAsDocLoader, NS_ERROR_UNEXPECTED);

    
    nsDocLoader* ancestor = this;
    do {
        if (childAsDocLoader == ancestor) {
            return NS_ERROR_ILLEGAL_VALUE;
        }
        ancestor = ancestor->GetParent();
    } while (ancestor);
    
    
    nsDocLoader* childsParent = childAsDocLoader->GetParent();
    if (childsParent) {
        childsParent->RemoveChildLoader(childAsDocLoader);
    }

    
    
    aChild->SetTreeOwner(nsnull);
    
    nsresult res = AddChildLoader(childAsDocLoader);
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(mChildList.Count() > 0,
                 "child list must not be empty after a successful add");

    
    
    
    {
        nsCOMPtr<nsIDocShell> childDocShell = do_QueryInterface(aChild);
        if (childDocShell) {
            
            
            
            nsCOMPtr<nsIDOMDocument> domDoc =
              do_GetInterface(GetAsSupports(this));
            nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
            PRUint32 offset = mChildList.Count() - 1;
            if (doc) {
               PRUint32 oldChildCount = offset; 
               for (PRUint32 i = 0; i < oldChildCount; ++i) {
                 nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
                 if (doc->FrameLoaderScheduledToBeFinalized(child)) {
                   --offset;
                 }
               }
            }

            childDocShell->SetChildOffset(offset);
        }
    }

    
    if (mGlobalHistory) {
        nsCOMPtr<nsIDocShellHistory>
            dsHistoryChild(do_QueryInterface(aChild));
        if (dsHistoryChild)
            dsHistoryChild->SetUseGlobalHistory(PR_TRUE);
    }


    PRInt32 childType = ~mItemType;     
    aChild->GetItemType(&childType);
    if (childType != mItemType)
        return NS_OK;
    


    aChild->SetTreeOwner(mTreeOwner);

    nsCOMPtr<nsIDocShell> childAsDocShell(do_QueryInterface(aChild));
    if (!childAsDocShell)
        return NS_OK;

    

    
    
    
    
    
    
    

    
    if (mItemType == nsIDocShellTreeItem::typeChrome)
        return NS_OK;

    
    nsCOMPtr<nsIDocumentCharsetInfo> dcInfo = NULL;
    res = childAsDocShell->GetDocumentCharsetInfo(getter_AddRefs(dcInfo));
    if (NS_FAILED(res) || (!dcInfo))
        return NS_OK;

    
    nsCOMPtr<nsIDocumentViewer> docv(do_QueryInterface(mContentViewer));
    if (!docv)
        return NS_OK;
    nsCOMPtr<nsIDocument> doc;
    res = docv->GetDocument(getter_AddRefs(doc));
    if (NS_FAILED(res) || (!doc))
        return NS_OK;
    const nsACString &parentCS = doc->GetDocumentCharacterSet();

    PRBool isWyciwyg = PR_FALSE;

    if (mCurrentURI) {
        
        mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);      
    }

    if (!isWyciwyg) {
        
        
        
        

        
        nsCOMPtr<nsIAtom> parentCSAtom(do_GetAtom(parentCS));
        res = dcInfo->SetParentCharset(parentCSAtom);
        if (NS_FAILED(res))
            return NS_OK;

        PRInt32 charsetSource = doc->GetDocumentCharacterSetSource();

        
        res = dcInfo->SetParentCharsetSource(charsetSource);
        if (NS_FAILED(res))
            return NS_OK;
    }

    

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::RemoveChild(nsIDocShellTreeItem * aChild)
{
    NS_ENSURE_ARG_POINTER(aChild);

    nsRefPtr<nsDocLoader> childAsDocLoader = GetAsDocLoader(aChild);
    NS_ENSURE_TRUE(childAsDocLoader, NS_ERROR_UNEXPECTED);
    
    nsresult rv = RemoveChildLoader(childAsDocLoader);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aChild->SetTreeOwner(nsnull);

    return nsDocLoader::AddDocLoaderAsChildOfRoot(childAsDocLoader);
}

NS_IMETHODIMP
nsDocShell::GetChildAt(PRInt32 aIndex, nsIDocShellTreeItem ** aChild)
{
    NS_ENSURE_ARG_POINTER(aChild);

#ifdef DEBUG
    if (aIndex < 0) {
      NS_WARNING("Negative index passed to GetChildAt");
    }
    else if (aIndex >= mChildList.Count()) {
      NS_WARNING("Too large an index passed to GetChildAt");
    }
#endif

    nsIDocumentLoader* child = SafeChildAt(aIndex);
    NS_ENSURE_TRUE(child, NS_ERROR_UNEXPECTED);
    
    return CallQueryInterface(child, aChild);
}

NS_IMETHODIMP
nsDocShell::FindChildWithName(const PRUnichar * aName,
                              PRBool aRecurse, PRBool aSameType,
                              nsIDocShellTreeItem * aRequestor,
                              nsIDocShellTreeItem * aOriginalRequestor,
                              nsIDocShellTreeItem ** _retval)
{
    NS_ENSURE_ARG(aName);
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nsnull;          

    if (!*aName)
        return NS_OK;

    nsXPIDLString childName;
    PRInt32 i, n = mChildList.Count();
    for (i = 0; i < n; i++) {
        nsCOMPtr<nsIDocShellTreeItem> child = do_QueryInterface(ChildAt(i));
        NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);
        PRInt32 childType;
        child->GetItemType(&childType);

        if (aSameType && (childType != mItemType))
            continue;

        PRBool childNameEquals = PR_FALSE;
        child->NameEquals(aName, &childNameEquals);
        if (childNameEquals && ItemIsActive(child) &&
            CanAccessItem(child, aOriginalRequestor)) {
            child.swap(*_retval);
            break;
        }

        if (childType != mItemType)     
            continue;

        if (aRecurse && (aRequestor != child))  
        {
            
#ifdef DEBUG
            nsresult rv =
#endif
            child->FindChildWithName(aName, PR_TRUE,
                                     aSameType,
                                     static_cast<nsIDocShellTreeItem*>
                                                (this),
                                     aOriginalRequestor,
                                     _retval);
            NS_ASSERTION(NS_SUCCEEDED(rv),
                         "FindChildWithName should not fail here");
            if (*_retval)           
                return NS_OK;
        }
    }
    return NS_OK;
}




NS_IMETHODIMP
nsDocShell::GetChildSHEntry(PRInt32 aChildOffset, nsISHEntry ** aResult)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nsnull;

    
    
    
    
    if (mLSHE) {
        




        PRBool parentExpired=PR_FALSE;
        mLSHE->GetExpirationStatus(&parentExpired);
        
        


        PRUint32 loadType = nsIDocShellLoadInfo::loadHistory;
        mLSHE->GetLoadType(&loadType);  
        
        
        if (loadType == nsIDocShellLoadInfo::loadReloadBypassCache ||
            loadType == nsIDocShellLoadInfo::loadReloadBypassProxy ||
            loadType == nsIDocShellLoadInfo::loadReloadBypassProxyAndCache ||
            loadType == nsIDocShellLoadInfo::loadRefresh)
            return rv;
        
        


        if (parentExpired && (loadType == nsIDocShellLoadInfo::loadReloadNormal)) {
            
            *aResult = nsnull;
            return rv;
        }

        nsCOMPtr<nsISHContainer> container(do_QueryInterface(mLSHE));
        if (container) {
            
            rv = container->GetChildAt(aChildOffset, aResult);            
            if (*aResult) 
                (*aResult)->SetLoadType(loadType);            
        }
    }
    return rv;
}

NS_IMETHODIMP
nsDocShell::AddChildSHEntry(nsISHEntry * aCloneRef, nsISHEntry * aNewEntry,
                            PRInt32 aChildOffset)
{
    nsresult rv;

    if (mLSHE) {
        


        nsCOMPtr<nsISHContainer> container(do_QueryInterface(mLSHE, &rv));
        if (container) {
            rv = container->AddChild(aNewEntry, aChildOffset);
        }
    }
    else if (!aCloneRef) {
        
        nsCOMPtr<nsISHContainer> container(do_QueryInterface(mOSHE, &rv));
        if (container) {
            rv = container->AddChild(aNewEntry, aChildOffset);
        }
    }
    else if (mSessionHistory) {
        






        PRInt32 index = -1;
        nsCOMPtr<nsIHistoryEntry> currentHE;
        mSessionHistory->GetIndex(&index);
        if (index < 0)
            return NS_ERROR_FAILURE;

        rv = mSessionHistory->GetEntryAtIndex(index, PR_FALSE,
                                              getter_AddRefs(currentHE));
        NS_ENSURE_TRUE(currentHE, NS_ERROR_FAILURE);

        nsCOMPtr<nsISHEntry> currentEntry(do_QueryInterface(currentHE));
        if (currentEntry) {
            PRUint32 cloneID = 0;
            nsCOMPtr<nsISHEntry> nextEntry;
            aCloneRef->GetID(&cloneID);
            rv = CloneAndReplace(currentEntry, this, cloneID, aNewEntry,
                                 getter_AddRefs(nextEntry));

            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsISHistoryInternal>
                    shPrivate(do_QueryInterface(mSessionHistory));
                NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
                rv = shPrivate->AddEntry(nextEntry, PR_TRUE);
            }
        }
    }
    else {
        
        nsCOMPtr<nsIDocShellHistory> parent =
            do_QueryInterface(GetAsSupports(mParent), &rv);
        if (parent) {
            rv = parent->AddChildSHEntry(aCloneRef, aNewEntry, aChildOffset);
        }          
    }
    return rv;
}

nsresult
nsDocShell::DoAddChildSHEntry(nsISHEntry* aNewEntry, PRInt32 aChildOffset)
{
    






    
    
    nsCOMPtr<nsISHistory> rootSH;
    GetRootSessionHistory(getter_AddRefs(rootSH));
    if (rootSH) {
        rootSH->GetIndex(&mPreviousTransIndex);
    }

    nsresult rv;
    nsCOMPtr<nsIDocShellHistory> parent =
        do_QueryInterface(GetAsSupports(mParent), &rv);
    if (parent) {
        rv = parent->AddChildSHEntry(mOSHE, aNewEntry, aChildOffset);
    }


    if (rootSH) {
        rootSH->GetIndex(&mLoadedTransIndex);
#ifdef DEBUG_PAGE_CACHE
        printf("Previous index: %d, Loaded index: %d\n\n", mPreviousTransIndex,
               mLoadedTransIndex);
#endif
    }

    return rv;
}

NS_IMETHODIMP
nsDocShell::SetUseGlobalHistory(PRBool aUseGlobalHistory)
{
    nsresult rv;

    if (!aUseGlobalHistory) {
        mGlobalHistory = nsnull;
        return NS_OK;
    }

    if (mGlobalHistory) {
        return NS_OK;
    }

    mGlobalHistory = do_GetService(NS_GLOBALHISTORY2_CONTRACTID, &rv);
    return rv;
}

NS_IMETHODIMP
nsDocShell::GetUseGlobalHistory(PRBool *aUseGlobalHistory)
{
    *aUseGlobalHistory = (mGlobalHistory != nsnull);
    return NS_OK;
}




PRBool 
nsDocShell::IsPrintingOrPP(PRBool aDisplayErrorDialog)
{
  if (mIsPrintingOrPP && aDisplayErrorDialog) {
    DisplayLoadError(NS_ERROR_DOCUMENT_IS_PRINTMODE, nsnull, nsnull);
  }

  return mIsPrintingOrPP;
}

PRBool
nsDocShell::IsNavigationAllowed(PRBool aDisplayPrintErrorDialog)
{
    return !IsPrintingOrPP(aDisplayPrintErrorDialog) && !mFiredUnloadEvent;
}





NS_IMETHODIMP
nsDocShell::GetCanGoBack(PRBool * aCanGoBack)
{
    if (!IsNavigationAllowed(PR_FALSE)) {
      *aCanGoBack = PR_FALSE;
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(rootSH));
    NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);
    rv = webnav->GetCanGoBack(aCanGoBack);   
    return rv;

}

NS_IMETHODIMP
nsDocShell::GetCanGoForward(PRBool * aCanGoForward)
{
    if (!IsNavigationAllowed(PR_FALSE)) {
      *aCanGoForward = PR_FALSE;
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH)); 
    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(rootSH));
    NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);
    rv = webnav->GetCanGoForward(aCanGoForward);
    return rv;

}

NS_IMETHODIMP
nsDocShell::GoBack()
{
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(rootSH));
    NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);
    rv = webnav->GoBack();
    return rv;

}

NS_IMETHODIMP
nsDocShell::GoForward()
{
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(rootSH));
    NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);
    rv = webnav->GoForward();
    return rv;

}

NS_IMETHODIMP nsDocShell::GotoIndex(PRInt32 aIndex)
{
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsresult rv;
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(rootSH));
    NS_ENSURE_TRUE(webnav, NS_ERROR_FAILURE);
    rv = webnav->GotoIndex(aIndex);
    return rv;

}


NS_IMETHODIMP
nsDocShell::LoadURI(const PRUnichar * aURI,
                    PRUint32 aLoadFlags,
                    nsIURI * aReferringURI,
                    nsIInputStream * aPostStream,
                    nsIInputStream * aHeaderStream)
{
    NS_ASSERTION((aLoadFlags & 0xf) == 0, "Unexpected flags");
    
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsCOMPtr<nsIURI> uri;
    nsresult rv = NS_OK;

    
    
    

    NS_ConvertUTF16toUTF8 uriString(aURI);
    
    uriString.Trim(" ");
    
    uriString.StripChars("\r\n");
    NS_ENSURE_TRUE(!uriString.IsEmpty(), NS_ERROR_FAILURE);

    rv = NS_NewURI(getter_AddRefs(uri), uriString);
    if (uri) {
        aLoadFlags &= ~LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
    }
    
    if (sURIFixup) {
        
        
        
        
        PRUint32 fixupFlags = 0;
        if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) {
          fixupFlags |= nsIURIFixup::FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;
        }
        rv = sURIFixup->CreateFixupURI(uriString, fixupFlags,
                                       getter_AddRefs(uri));
    }
    
    

    if (NS_ERROR_MALFORMED_URI == rv) {
        DisplayLoadError(rv, uri, aURI);
    }

    if (NS_FAILED(rv) || !uri)
        return NS_ERROR_FAILURE;

    PopupControlState popupState;
    if (aLoadFlags & LOAD_FLAGS_ALLOW_POPUPS) {
        popupState = openAllowed;
        aLoadFlags &= ~LOAD_FLAGS_ALLOW_POPUPS;
    } else {
        popupState = openOverridden;
    }
    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
    nsAutoPopupStatePusher statePusher(win, popupState);

    
    
    
    PRUint32 extraFlags = (aLoadFlags & EXTRA_LOAD_FLAGS);
    aLoadFlags &= ~EXTRA_LOAD_FLAGS;

    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    rv = CreateLoadInfo(getter_AddRefs(loadInfo));
    if (NS_FAILED(rv)) return rv;
    
    PRUint32 loadType = MAKE_LOAD_TYPE(LOAD_NORMAL, aLoadFlags);
    loadInfo->SetLoadType(ConvertLoadTypeToDocShellLoadInfo(loadType));
    loadInfo->SetPostDataStream(aPostStream);
    loadInfo->SetReferrer(aReferringURI);
    loadInfo->SetHeadersStream(aHeaderStream);

    rv = LoadURI(uri, loadInfo, extraFlags, PR_TRUE);

    return rv;
}

NS_IMETHODIMP
nsDocShell::DisplayLoadError(nsresult aError, nsIURI *aURI,
                             const PRUnichar *aURL,
                             nsIChannel* aFailedChannel)
{
    
    nsCOMPtr<nsIPrompt> prompter;
    nsCOMPtr<nsIStringBundle> stringBundle;
    GetPromptAndStringBundle(getter_AddRefs(prompter),
                             getter_AddRefs(stringBundle));

    NS_ENSURE_TRUE(stringBundle, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(prompter, NS_ERROR_FAILURE);

    nsAutoString error;
    const PRUint32 kMaxFormatStrArgs = 3;
    nsAutoString formatStrs[kMaxFormatStrArgs];
    PRUint32 formatStrCount = 0;
    PRBool addHostPort = PR_FALSE;
    nsresult rv = NS_OK;
    nsAutoString messageStr;
    nsCAutoString cssClass;
    nsCAutoString errorPage;

    errorPage.AssignLiteral("neterror");

    
    if (NS_ERROR_UNKNOWN_PROTOCOL == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        
        nsCAutoString scheme;
        aURI->GetScheme(scheme);
        CopyASCIItoUTF16(scheme, formatStrs[0]);
        formatStrCount = 1;
        error.AssignLiteral("protocolNotFound");
    }
    else if (NS_ERROR_FILE_NOT_FOUND == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        error.AssignLiteral("fileNotFound");
    }
    else if (NS_ERROR_UNKNOWN_HOST == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        
        nsCAutoString host;
        nsCOMPtr<nsIURI> innermostURI = NS_GetInnermostURI(aURI);
        innermostURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;
        error.AssignLiteral("dnsNotFound");
    }
    else if(NS_ERROR_CONNECTION_REFUSED == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        addHostPort = PR_TRUE;
        error.AssignLiteral("connectionFailure");
    }
    else if(NS_ERROR_NET_INTERRUPT == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        addHostPort = PR_TRUE;
        error.AssignLiteral("netInterrupt");
    }
    else if (NS_ERROR_NET_TIMEOUT == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        
        nsCAutoString host;
        aURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;
        error.AssignLiteral("netTimeout");
    }
    else if (NS_ERROR_GET_MODULE(aError) == NS_ERROR_MODULE_SECURITY) {
        nsCOMPtr<nsINSSErrorsService> nsserr =
            do_GetService(NS_NSS_ERRORS_SERVICE_CONTRACTID);

        PRUint32 errorClass;
        if (!nsserr ||
            NS_FAILED(nsserr->GetErrorClass(aError, &errorClass))) {
          errorClass = nsINSSErrorsService::ERROR_CLASS_SSL_PROTOCOL;
        }

        nsCOMPtr<nsISupports> securityInfo;
        nsCOMPtr<nsITransportSecurityInfo> tsi;
        if (aFailedChannel)
            aFailedChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
        tsi = do_QueryInterface(securityInfo);
        if (tsi) {
            
            tsi->GetErrorMessage(getter_Copies(messageStr));
        }
        else {
            
            if (nsserr) {
                nsserr->GetErrorMessage(aError, messageStr);
            }
        }
        if (!messageStr.IsEmpty()) {
            if (errorClass == nsINSSErrorsService::ERROR_CLASS_BAD_CERT) {
                error.AssignLiteral("nssBadCert");
                PRBool expert = PR_FALSE;
                mPrefs->GetBoolPref("browser.xul.error_pages.expert_bad_cert",
                                    &expert);
                if (expert) {
                    cssClass.AssignLiteral("expertBadCert");
                }
                
                
                nsXPIDLCString alternateErrorPage;
                mPrefs->GetCharPref("security.alternate_certificate_error_page",
                                    getter_Copies(alternateErrorPage));
                if (alternateErrorPage)
                    errorPage.Assign(alternateErrorPage);
            } else {
                error.AssignLiteral("nssFailure2");
            }
        }
    } else if (NS_ERROR_PHISHING_URI == aError || NS_ERROR_MALWARE_URI == aError) {
        nsCAutoString host;
        aURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;

        
        
        nsXPIDLCString alternateErrorPage;
        mPrefs->GetCharPref("urlclassifier.alternate_error_page",
                            getter_Copies(alternateErrorPage));
        if (alternateErrorPage)
            errorPage.Assign(alternateErrorPage);

        if (NS_ERROR_PHISHING_URI == aError)
            error.AssignLiteral("phishingBlocked");
        else
            error.AssignLiteral("malwareBlocked");
        cssClass.AssignLiteral("blacklist");
    }
    else {
        
        switch (aError) {
        case NS_ERROR_MALFORMED_URI:
            
            error.AssignLiteral("malformedURI");
            break;
        case NS_ERROR_REDIRECT_LOOP:
            
            error.AssignLiteral("redirectLoop");
            break;
        case NS_ERROR_UNKNOWN_SOCKET_TYPE:
            
            error.AssignLiteral("unknownSocketType");
            break;
        case NS_ERROR_NET_RESET:
            
            
            error.AssignLiteral("netReset");
            break;
        case NS_ERROR_DOCUMENT_NOT_CACHED:
            
            
            error.AssignLiteral("netOffline");
            break;
        case NS_ERROR_DOCUMENT_IS_PRINTMODE:
            
            error.AssignLiteral("isprinting");
            break;
        case NS_ERROR_PORT_ACCESS_NOT_ALLOWED:
            
            addHostPort = PR_TRUE;
            error.AssignLiteral("deniedPortAccess");
            break;
        case NS_ERROR_UNKNOWN_PROXY_HOST:
            
            error.AssignLiteral("proxyResolveFailure");
            break;
        case NS_ERROR_PROXY_CONNECTION_REFUSED:
            
            error.AssignLiteral("proxyConnectFailure");
            break;
        case NS_ERROR_INVALID_CONTENT_ENCODING:
            
            error.AssignLiteral("contentEncodingError");
            break;
        case NS_ERROR_UNSAFE_CONTENT_TYPE:
            
            error.AssignLiteral("unsafeContentType");
            break;
        }
    }

    
    if (error.IsEmpty()) {
        return NS_OK;
    }

    
    if (!messageStr.IsEmpty()) {
        
    }
    else {
        if (addHostPort) {
            
            nsCAutoString hostport;
            if (aURI) {
                aURI->GetHostPort(hostport);
            } else {
                hostport.AssignLiteral("?");
            }
            CopyUTF8toUTF16(hostport, formatStrs[formatStrCount++]);
        }

        nsCAutoString spec;
        rv = NS_ERROR_NOT_AVAILABLE;
        if (aURI) {
            
            
            PRBool isFileURI = PR_FALSE;
            rv = aURI->SchemeIs("file", &isFileURI);
            if (NS_SUCCEEDED(rv) && isFileURI)
                aURI->GetPath(spec);
            else
                aURI->GetSpec(spec);

            nsCAutoString charset;
            
            aURI->GetOriginCharset(charset);
            nsCOMPtr<nsITextToSubURI> textToSubURI(
                do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv));
            if (NS_SUCCEEDED(rv)) {
                rv = textToSubURI->UnEscapeURIForUI(charset, spec, formatStrs[formatStrCount]);
            }
        } else {
            spec.AssignLiteral("?");
        }
        if (NS_FAILED(rv))
            CopyUTF8toUTF16(spec, formatStrs[formatStrCount]);
        rv = NS_OK;
        ++formatStrCount;

        const PRUnichar *strs[kMaxFormatStrArgs];
        for (PRUint32 i = 0; i < formatStrCount; i++) {
            strs[i] = formatStrs[i].get();
        }
        nsXPIDLString str;
        rv = stringBundle->FormatStringFromName(
            error.get(),
            strs, formatStrCount, getter_Copies(str));
        NS_ENSURE_SUCCESS(rv, rv);
        messageStr.Assign(str.get());
    }

    
    NS_ENSURE_FALSE(messageStr.IsEmpty(), NS_ERROR_FAILURE);
    
    
    if (mUseErrorPages && aURI && aFailedChannel) {
        
        LoadErrorPage(aURI, aURL, errorPage.get(), error.get(),
                      messageStr.get(), cssClass.get(), aFailedChannel);
    } 
    else
    {
        
        
        
        nsCOMPtr<nsPIDOMWindow> pwin(do_QueryInterface(mScriptGlobal));
        if (pwin) {
            nsCOMPtr<nsIDOMDocument> doc;
            pwin->GetDocument(getter_AddRefs(doc));
        }

        
        prompter->Alert(nsnull, messageStr.get());
    }

    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::LoadErrorPage(nsIURI *aURI, const PRUnichar *aURL,
                          const char *aErrorPage,
                          const PRUnichar *aErrorType,
                          const PRUnichar *aDescription,
                          const char *aCSSClass,
                          nsIChannel* aFailedChannel)
{
#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsCAutoString spec;
        aURI->GetSpec(spec);

        nsCAutoString chanName;
        if (aFailedChannel)
            aFailedChannel->GetName(chanName);
        else
            chanName.AssignLiteral("<no channel>");

        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]::LoadErrorPage(\"%s\", \"%s\", {...}, [%s])\n", this,
                spec.get(), NS_ConvertUTF16toUTF8(aURL).get(), chanName.get()));
    }
#endif
    
    if (aFailedChannel) {
        mURIResultedInDocument = PR_TRUE;
        OnLoadingSite(aFailedChannel, PR_TRUE, PR_FALSE);
    } else if (aURI) {
        mURIResultedInDocument = PR_TRUE;
        OnNewURI(aURI, nsnull, nsnull, mLoadType, PR_TRUE, PR_FALSE);
    }
    
    
    if (mSessionHistory && !mLSHE) {
        PRInt32 idx;
        mSessionHistory->GetRequestedIndex(&idx);
        if (idx == -1)
            mSessionHistory->GetIndex(&idx);

        nsCOMPtr<nsIHistoryEntry> entry;
        mSessionHistory->GetEntryAtIndex(idx, PR_FALSE,
                                         getter_AddRefs(entry));
        mLSHE = do_QueryInterface(entry);
    }

    nsCAutoString url;
    nsCAutoString charset;
    if (aURI)
    {
        
        SetCurrentURI(aURI);

        nsresult rv = aURI->GetSpec(url);
        rv |= aURI->GetOriginCharset(charset);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (aURL)
    {
        CopyUTF16toUTF8(aURL, url);
    }
    else
    {
        return NS_ERROR_INVALID_POINTER;
    }

    

    char *escapedUrl = nsEscape(url.get(), url_Path);
    char *escapedCharset = nsEscape(charset.get(), url_Path);
    char *escapedError = nsEscape(NS_ConvertUTF16toUTF8(aErrorType).get(), url_Path);
    char *escapedDescription = nsEscape(NS_ConvertUTF16toUTF8(aDescription).get(), url_Path);
    char *escapedCSSClass = nsEscape(aCSSClass, url_Path);

    nsCString errorPageUrl("about:");
    errorPageUrl.AppendASCII(aErrorPage);
    errorPageUrl.AppendLiteral("?e=");

    errorPageUrl.AppendASCII(escapedError);
    errorPageUrl.AppendLiteral("&u=");
    errorPageUrl.AppendASCII(escapedUrl);
    if (escapedCSSClass && escapedCSSClass[0]) {
        errorPageUrl.AppendASCII("&s=");
        errorPageUrl.AppendASCII(escapedCSSClass);
    }
    errorPageUrl.AppendLiteral("&c=");
    errorPageUrl.AppendASCII(escapedCharset);
    errorPageUrl.AppendLiteral("&d=");
    errorPageUrl.AppendASCII(escapedDescription);

    nsMemory::Free(escapedDescription);
    nsMemory::Free(escapedError);
    nsMemory::Free(escapedUrl);
    nsMemory::Free(escapedCharset);
    nsMemory::Free(escapedCSSClass);

    nsCOMPtr<nsIURI> errorPageURI;
    nsresult rv = NS_NewURI(getter_AddRefs(errorPageURI), errorPageUrl);
    NS_ENSURE_SUCCESS(rv, rv);

    return InternalLoad(errorPageURI, nsnull, nsnull,
                        INTERNAL_LOAD_FLAGS_INHERIT_OWNER, nsnull, nsnull,
                        nsnull, nsnull, LOAD_ERROR_PAGE,
                        nsnull, PR_TRUE, nsnull, nsnull);
}


NS_IMETHODIMP
nsDocShell::Reload(PRUint32 aReloadFlags)
{
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsresult rv;
    NS_ASSERTION(((aReloadFlags & 0xf) == 0),
                 "Reload command not updated to use load flags!");
    NS_ASSERTION((aReloadFlags & EXTRA_LOAD_FLAGS) == 0,
                 "Don't pass these flags to Reload");

    PRUint32 loadType = MAKE_LOAD_TYPE(LOAD_RELOAD_NORMAL, aReloadFlags);
    NS_ENSURE_TRUE(IsValidLoadType(loadType), NS_ERROR_INVALID_ARG);

    
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsISHistoryInternal> shistInt(do_QueryInterface(rootSH));
    PRBool canReload = PR_TRUE; 
    if (rootSH) {
      nsCOMPtr<nsISHistoryListener> listener;
      shistInt->GetListener(getter_AddRefs(listener));
      if (listener) {
        listener->OnHistoryReload(mCurrentURI, aReloadFlags, &canReload);
      }
    }

    if (!canReload)
      return NS_OK;
    
    
    if (mOSHE) {
        rv = LoadHistoryEntry(mOSHE, loadType);
    }
    else if (mLSHE) { 
        rv = LoadHistoryEntry(mLSHE, loadType);
    }
    else {
        nsCOMPtr<nsIDOMDocument> domDoc(do_GetInterface(GetAsSupports(this)));
        nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));

        nsIPrincipal* principal = nsnull;
        nsAutoString contentTypeHint;
        if (doc) {
            principal = doc->NodePrincipal();
            doc->GetContentType(contentTypeHint);
        }

        rv = InternalLoad(mCurrentURI,
                          mReferrerURI,
                          principal,
                          INTERNAL_LOAD_FLAGS_NONE, 
                          nsnull,         
                          NS_LossyConvertUTF16toASCII(contentTypeHint).get(),
                          nsnull,         
                          nsnull,         
                          loadType,       
                          nsnull,         
                          PR_TRUE,
                          nsnull,         
                          nsnull);        
    }
    

    return rv;
}

NS_IMETHODIMP
nsDocShell::Stop(PRUint32 aStopFlags)
{
    
    mRestorePresentationEvent.Revoke();

    if (mLoadType == LOAD_ERROR_PAGE && mLSHE) {
        
        SetHistoryEntry(&mOSHE, mLSHE);
        SetHistoryEntry(&mLSHE, nsnull);
    }

    if (nsIWebNavigation::STOP_CONTENT & aStopFlags) {
        
        if (mContentViewer)
            mContentViewer->Stop();
    }

    if (nsIWebNavigation::STOP_NETWORK & aStopFlags) {
        
        
        if (mRefreshURIList) {
            SuspendRefreshURIs();
            mSavedRefreshURIList.swap(mRefreshURIList);
            mRefreshURIList = nsnull;
        }

        if (mClassifier) {
            mClassifier->Cancel();
            mClassifier = nsnull;
        }

        
        
        
        Stop();
    }

    PRInt32 n;
    PRInt32 count = mChildList.Count();
    for (n = 0; n < count; n++) {
        nsCOMPtr<nsIWebNavigation> shellAsNav(do_QueryInterface(ChildAt(n)));
        if (shellAsNav)
            shellAsNav->Stop(aStopFlags);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDocument(nsIDOMDocument ** aDocument)
{
    NS_ENSURE_ARG_POINTER(aDocument);
    NS_ENSURE_SUCCESS(EnsureContentViewer(), NS_ERROR_FAILURE);

    return mContentViewer->GetDOMDocument(aDocument);
}

NS_IMETHODIMP
nsDocShell::GetCurrentURI(nsIURI ** aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);

    if (mCurrentURI) {
        return NS_EnsureSafeToReturn(mCurrentURI, aURI);
    }

    *aURI = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetReferringURI(nsIURI ** aURI)
{
    NS_ENSURE_ARG_POINTER(aURI);

    *aURI = mReferrerURI;
    NS_IF_ADDREF(*aURI);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetSessionHistory(nsISHistory * aSessionHistory)
{

    NS_ENSURE_TRUE(aSessionHistory, NS_ERROR_FAILURE);
    
    

    nsCOMPtr<nsIDocShellTreeItem> root;
    



    GetSameTypeRootTreeItem(getter_AddRefs(root));
    NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
    if (root.get() == static_cast<nsIDocShellTreeItem *>(this)) {
        mSessionHistory = aSessionHistory;
        nsCOMPtr<nsISHistoryInternal>
            shPrivate(do_QueryInterface(mSessionHistory));
        NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
        shPrivate->SetRootDocShell(this);
        return NS_OK;
    }
    return NS_ERROR_FAILURE;

}


NS_IMETHODIMP
nsDocShell::GetSessionHistory(nsISHistory ** aSessionHistory)
{
    NS_ENSURE_ARG_POINTER(aSessionHistory);
    *aSessionHistory = mSessionHistory;
    NS_IF_ADDREF(*aSessionHistory);
    return NS_OK;
}




NS_IMETHODIMP
nsDocShell::LoadPage(nsISupports *aPageDescriptor, PRUint32 aDisplayType)
{
    nsCOMPtr<nsISHEntry> shEntryIn(do_QueryInterface(aPageDescriptor));

    
    if (!shEntryIn) {
        return NS_ERROR_INVALID_POINTER;
    }

    
    
    nsCOMPtr<nsISHEntry> shEntry;
    nsresult rv = shEntryIn->Clone(getter_AddRefs(shEntry));
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    
    
    if (nsIWebPageDescriptor::DISPLAY_AS_SOURCE == aDisplayType) {
        nsCOMPtr<nsIURI> oldUri, newUri;
        nsCString spec, newSpec;

        
        rv = shEntry->GetURI(getter_AddRefs(oldUri));
        if (NS_FAILED(rv))
              return rv;

        oldUri->GetSpec(spec);
        newSpec.AppendLiteral("view-source:");
        newSpec.Append(spec);

        rv = NS_NewURI(getter_AddRefs(newUri), newSpec);
        if (NS_FAILED(rv)) {
            return rv;
        }
        shEntry->SetURI(newUri);
    }

    rv = LoadHistoryEntry(shEntry, LOAD_HISTORY);
    return rv;
}

NS_IMETHODIMP
nsDocShell::GetCurrentDescriptor(nsISupports **aPageDescriptor)
{
    NS_PRECONDITION(aPageDescriptor, "Null out param?");

    *aPageDescriptor = nsnull;

    nsISHEntry* src = mOSHE ? mOSHE : mLSHE;
    if (src) {
        nsCOMPtr<nsISHEntry> dest;

        nsresult rv = src->Clone(getter_AddRefs(dest));
        if (NS_FAILED(rv)) {
            return rv;
        }

        
        dest->SetParent(nsnull);
        dest->SetIsSubFrame(PR_FALSE);
        
        return CallQueryInterface(dest, aPageDescriptor);
    }

    return NS_ERROR_NOT_AVAILABLE;
}






NS_IMETHODIMP
nsDocShell::InitWindow(nativeWindow parentNativeWindow,
                       nsIWidget * parentWidget, PRInt32 x, PRInt32 y,
                       PRInt32 cx, PRInt32 cy)
{
    SetParentWidget(parentWidget);
    SetPositionAndSize(x, y, cx, cy, PR_FALSE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::Create()
{
    if (mPrefs) {
        
        return NS_OK;
    }

    NS_ASSERTION(mItemType == typeContent || mItemType == typeChrome,
                 "Unexpected item type in docshell");

    nsresult rv = NS_ERROR_FAILURE;
    mPrefs = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool tmpbool;

    rv = mPrefs->GetBoolPref("browser.frames.enabled", &tmpbool);
    if (NS_SUCCEEDED(rv))
        mAllowSubframes = tmpbool;

    if (gValidateOrigin == (PRBool)0xffffffff) {
        
        rv = mPrefs->GetBoolPref("browser.frame.validate_origin", &tmpbool);
        if (NS_SUCCEEDED(rv)) {
            gValidateOrigin = tmpbool;
        } else {
            gValidateOrigin = PR_TRUE;
        }
    }

    
    rv = mPrefs->GetBoolPref("browser.xul.error_pages.enabled", &tmpbool);
    if (NS_SUCCEEDED(rv))
        mUseErrorPages = tmpbool;

    nsCOMPtr<nsIPrefBranch2> prefs(do_QueryInterface(mPrefs, &rv));
    if (NS_SUCCEEDED(rv) && mObserveErrorPages) {
        prefs->AddObserver("browser.xul.error_pages.enabled", this, PR_FALSE);
    }

    nsCOMPtr<nsIObserverService> serv = do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
    if (serv) {
        const char* msg = mItemType == typeContent ?
            NS_WEBNAVIGATION_CREATE : NS_CHROME_WEBNAVIGATION_CREATE;
        serv->NotifyObservers(GetAsSupports(this), msg, nsnull);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::Destroy()
{
    NS_ASSERTION(mItemType == typeContent || mItemType == typeChrome,
                 "Unexpected item type in docshell");

    if (!mIsBeingDestroyed) {
        nsCOMPtr<nsIObserverService> serv =
            do_GetService(NS_OBSERVERSERVICE_CONTRACTID);
        if (serv) {
            const char* msg = mItemType == typeContent ?
                NS_WEBNAVIGATION_DESTROY : NS_CHROME_WEBNAVIGATION_DESTROY;
            serv->NotifyObservers(GetAsSupports(this), msg, nsnull);
        }
    }
    
    mIsBeingDestroyed = PR_TRUE;

    
    if (mObserveErrorPages) {
        nsCOMPtr<nsIPrefBranch2> prefs(do_QueryInterface(mPrefs));
        if (prefs) {
            prefs->RemoveObserver("browser.xul.error_pages.enabled", this);
            mObserveErrorPages = PR_FALSE;
        }
    }

    
    
    mLoadingURI = nsnull;

    
    (void) FirePageHideNotification(PR_TRUE);

    
    
    if (mOSHE)
      mOSHE->SetEditorData(nsnull);
    if (mLSHE)
      mLSHE->SetEditorData(nsnull);
      
    
    
    if (mContentListener) {
        mContentListener->DropDocShellreference();
        mContentListener->SetParentContentListener(nsnull);
        
        
        
        
        
    }

    
    Stop(nsIWebNavigation::STOP_ALL);

    mEditorData = nsnull;

    mTransferableHookData = nsnull;

    
    
    
    PersistLayoutHistoryState();

    
    nsCOMPtr<nsIDocShellTreeItem> docShellParentAsItem =
        do_QueryInterface(GetAsSupports(mParent));
    if (docShellParentAsItem)
        docShellParentAsItem->RemoveChild(this);

    if (mContentViewer) {
        mContentViewer->Close(nsnull);
        mContentViewer->Destroy();
        mContentViewer = nsnull;
    }

    nsDocLoader::Destroy();
    
    mParentWidget = nsnull;
    mCurrentURI = nsnull;

    if (mScriptGlobal) {
        nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
        win->SetDocShell(nsnull);

        mScriptGlobal = nsnull;
    }

    if (mSessionHistory) {
        
        
        
        nsCOMPtr<nsISHistoryInternal> shPrivate =
            do_QueryInterface(mSessionHistory);
        if (shPrivate) {
            shPrivate->EvictAllContentViewers();
        }
        mSessionHistory = nsnull;
    }

    SetTreeOwner(nsnull);

    
    mSecurityUI = nsnull;

    
    
    CancelRefreshURITimers();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetPosition(PRInt32 x, PRInt32 y)
{
    mBounds.x = x;
    mBounds.y = y;

    if (mContentViewer)
        NS_ENSURE_SUCCESS(mContentViewer->Move(x, y), NS_ERROR_FAILURE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPosition(PRInt32 * aX, PRInt32 * aY)
{
    PRInt32 dummyHolder;
    return GetPositionAndSize(aX, aY, &dummyHolder, &dummyHolder);
}

NS_IMETHODIMP
nsDocShell::SetSize(PRInt32 aCX, PRInt32 aCY, PRBool aRepaint)
{
    PRInt32 x = 0, y = 0;
    GetPosition(&x, &y);
    return SetPositionAndSize(x, y, aCX, aCY, aRepaint);
}

NS_IMETHODIMP
nsDocShell::GetSize(PRInt32 * aCX, PRInt32 * aCY)
{
    PRInt32 dummyHolder;
    return GetPositionAndSize(&dummyHolder, &dummyHolder, aCX, aCY);
}

NS_IMETHODIMP
nsDocShell::SetPositionAndSize(PRInt32 x, PRInt32 y, PRInt32 cx,
                               PRInt32 cy, PRBool fRepaint)
{
    mBounds.x = x;
    mBounds.y = y;
    mBounds.width = cx;
    mBounds.height = cy;

    
    nsCOMPtr<nsIContentViewer> viewer = mContentViewer;
    if (viewer) {
        
        NS_ENSURE_SUCCESS(viewer->SetBounds(mBounds), NS_ERROR_FAILURE);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPositionAndSize(PRInt32 * x, PRInt32 * y, PRInt32 * cx,
                               PRInt32 * cy)
{
    
    
    nsCOMPtr<nsIDOMDocument> document(do_GetInterface(GetAsSupports(mParent)));
    nsCOMPtr<nsIDocument> doc(do_QueryInterface(document));
    if (doc) {
        doc->FlushPendingNotifications(Flush_Layout);
    }
    
    DoGetPositionAndSize(x, y, cx, cy);
    return NS_OK;
}

void
nsDocShell::DoGetPositionAndSize(PRInt32 * x, PRInt32 * y, PRInt32 * cx,
                                 PRInt32 * cy)
{    
    if (x)
        *x = mBounds.x;
    if (y)
        *y = mBounds.y;
    if (cx)
        *cx = mBounds.width;
    if (cy)
        *cy = mBounds.height;
}

NS_IMETHODIMP
nsDocShell::Repaint(PRBool aForce)
{
    nsCOMPtr<nsIPresShell> presShell;
    GetPresShell(getter_AddRefs(presShell));
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsIViewManager* viewManager = presShell->GetViewManager();
    NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

    
    NS_ENSURE_SUCCESS(viewManager->UpdateAllViews(0), NS_ERROR_FAILURE);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParentWidget(nsIWidget ** parentWidget)
{
    NS_ENSURE_ARG_POINTER(parentWidget);

    *parentWidget = mParentWidget;
    NS_IF_ADDREF(*parentWidget);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetParentWidget(nsIWidget * aParentWidget)
{
    mParentWidget = aParentWidget;

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParentNativeWindow(nativeWindow * parentNativeWindow)
{
    NS_ENSURE_ARG_POINTER(parentNativeWindow);

    if (mParentWidget)
        *parentNativeWindow = mParentWidget->GetNativeData(NS_NATIVE_WIDGET);
    else
        *parentNativeWindow = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetParentNativeWindow(nativeWindow parentNativeWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetVisibility(PRBool * aVisibility)
{
    NS_ENSURE_ARG_POINTER(aVisibility);

    *aVisibility = PR_FALSE;

    if (!mContentViewer)
        return NS_OK;

    nsCOMPtr<nsIPresShell> presShell;
    GetPresShell(getter_AddRefs(presShell));
    if (!presShell)
        return NS_OK;

    
    nsIViewManager* vm = presShell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

    
    nsIView *view = nsnull; 
    NS_ENSURE_SUCCESS(vm->GetRootView(view), NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

    
    if (view->GetVisibility() == nsViewVisibility_kHide)
        return NS_OK;

    
    
    

    nsCOMPtr<nsIDocShellTreeItem> treeItem = this;
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    treeItem->GetParent(getter_AddRefs(parentItem));
    while (parentItem) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(treeItem));
        docShell->GetPresShell(getter_AddRefs(presShell));

        nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parentItem);
        nsCOMPtr<nsIPresShell> pPresShell;
        parentDS->GetPresShell(getter_AddRefs(pPresShell));

        
        if (!pPresShell) {
            NS_NOTREACHED("parent docshell has null pres shell");
            return NS_OK;
        }

        nsIContent *shellContent =
            pPresShell->GetDocument()->FindContentForSubDocument(presShell->GetDocument());
        NS_ASSERTION(shellContent, "subshell not in the map");

        nsIFrame* frame = pPresShell->GetPrimaryFrameFor(shellContent);
        PRBool isDocShellOffScreen = PR_FALSE;
        docShell->GetIsOffScreenBrowser(&isDocShellOffScreen);
        if (frame && !frame->AreAncestorViewsVisible() && !isDocShellOffScreen)
            return NS_OK;

        treeItem = parentItem;
        treeItem->GetParent(getter_AddRefs(parentItem));
    }

    nsCOMPtr<nsIBaseWindow> treeOwnerAsWin(do_QueryInterface(mTreeOwner));
    if (!treeOwnerAsWin) {
        *aVisibility = PR_TRUE;
        return NS_OK;
    }

    
    
    return treeOwnerAsWin->GetVisibility(aVisibility);
}

NS_IMETHODIMP
nsDocShell::SetIsOffScreenBrowser(PRBool aIsOffScreen) 
{
    mIsOffScreenBrowser = aIsOffScreen;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsOffScreenBrowser(PRBool *aIsOffScreen) 
{
    *aIsOffScreen = mIsOffScreenBrowser;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetVisibility(PRBool aVisibility)
{
    if (!mContentViewer)
        return NS_OK;
    if (aVisibility) {
        mContentViewer->Show();
    }
    else {
        mContentViewer->Hide();
    }
    
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetEnabled(PRBool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = PR_TRUE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::SetEnabled(PRBool aEnabled)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetBlurSuppression(PRBool *aBlurSuppression)
{
  NS_ENSURE_ARG_POINTER(aBlurSuppression);
  *aBlurSuppression = PR_FALSE;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::SetBlurSuppression(PRBool aBlurSuppression)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::SetFocus()
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMainWidget(nsIWidget ** aMainWidget)
{
    
    return GetParentWidget(aMainWidget);
}

NS_IMETHODIMP
nsDocShell::GetTitle(PRUnichar ** aTitle)
{
    NS_ENSURE_ARG_POINTER(aTitle);

    *aTitle = ToNewUnicode(mTitle);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetTitle(const PRUnichar * aTitle)
{
    
    mTitle = aTitle;

    nsCOMPtr<nsIDocShellTreeItem> parent;
    GetSameTypeParent(getter_AddRefs(parent));

    
    
    if (!parent) {
        nsCOMPtr<nsIBaseWindow>
            treeOwnerAsWin(do_QueryInterface(mTreeOwner));
        if (treeOwnerAsWin)
            treeOwnerAsWin->SetTitle(aTitle);
    }

    if (mGlobalHistory && mCurrentURI && mLoadType != LOAD_ERROR_PAGE) {
        mGlobalHistory->SetPageTitle(mCurrentURI, nsDependentString(aTitle));
    }


    
    
    
    
    
    if (mOSHE && (mLoadType != LOAD_BYPASS_HISTORY) &&
        (mLoadType != LOAD_HISTORY) && (mLoadType != LOAD_ERROR_PAGE)) {
        mOSHE->SetTitle(mTitle);    
    }


    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetCurScrollPos(PRInt32 scrollOrientation, PRInt32 * curPos)
{
    NS_ENSURE_ARG_POINTER(curPos);

    nsIScrollableView* scrollView;
    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    nscoord x, y;
    NS_ENSURE_SUCCESS(scrollView->GetScrollPosition(x, y), NS_ERROR_FAILURE);

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        *curPos = x;
        return NS_OK;

    case ScrollOrientation_Y:
        *curPos = y;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_INVALID_ARG);
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::SetCurScrollPos(PRInt32 scrollOrientation, PRInt32 curPos)
{
    nsIScrollableView* scrollView;
    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    PRInt32 other;
    PRInt32 x;
    PRInt32 y;

    GetCurScrollPos(scrollOrientation, &other);

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        x = curPos;
        y = other;
        break;

    case ScrollOrientation_Y:
        x = other;
        y = curPos;
        break;

    default:
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_INVALID_ARG);
        x = 0;
        y = 0;                  
    }

    NS_ENSURE_SUCCESS(scrollView->ScrollTo(x, y, 0),
                      NS_ERROR_FAILURE);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetCurScrollPosEx(PRInt32 curHorizontalPos, PRInt32 curVerticalPos)
{
    nsIScrollableView* scrollView;
    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    NS_ENSURE_SUCCESS(scrollView->ScrollTo(curHorizontalPos, curVerticalPos, 0),
                      NS_ERROR_FAILURE);
    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::GetScrollRange(PRInt32 scrollOrientation,
                           PRInt32 * minPos, PRInt32 * maxPos)
{
    NS_ENSURE_ARG_POINTER(minPos && maxPos);

    nsIScrollableView* scrollView;
    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    PRInt32 cx;
    PRInt32 cy;

    NS_ENSURE_SUCCESS(scrollView->GetContainerSize(&cx, &cy), NS_ERROR_FAILURE);
    *minPos = 0;

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        *maxPos = cx;
        return NS_OK;

    case ScrollOrientation_Y:
        *maxPos = cy;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_INVALID_ARG);
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::SetScrollRange(PRInt32 scrollOrientation,
                           PRInt32 minPos, PRInt32 maxPos)
{
    
    







    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::SetScrollRangeEx(PRInt32 minHorizontalPos,
                             PRInt32 maxHorizontalPos, PRInt32 minVerticalPos,
                             PRInt32 maxVerticalPos)
{
    
    







    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsDocShell::GetDefaultScrollbarPreferences(PRInt32 scrollOrientation,
                                           PRInt32 * scrollbarPref)
{
    NS_ENSURE_ARG_POINTER(scrollbarPref);
    switch (scrollOrientation) {
    case ScrollOrientation_X:
        *scrollbarPref = mDefaultScrollbarPref.x;
        return NS_OK;

    case ScrollOrientation_Y:
        *scrollbarPref = mDefaultScrollbarPref.y;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_INVALID_ARG);
    }
    return NS_ERROR_FAILURE;
}










NS_IMETHODIMP
nsDocShell::SetDefaultScrollbarPreferences(PRInt32 scrollOrientation,
                                           PRInt32 scrollbarPref)
{
    switch (scrollOrientation) {
    case ScrollOrientation_X:
        mDefaultScrollbarPref.x = scrollbarPref;
        return NS_OK;

    case ScrollOrientation_Y:
        mDefaultScrollbarPref.y = scrollbarPref;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_INVALID_ARG);
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::GetScrollbarVisibility(PRBool * verticalVisible,
                                   PRBool * horizontalVisible)
{
    nsIScrollableView* scrollView;
    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView)
        return NS_ERROR_FAILURE;

    
    
    nsIFrame* scrollFrame =
        static_cast<nsIFrame*>(scrollView->View()->GetParent()->GetClientData());
    if (!scrollFrame)
        return NS_ERROR_FAILURE;
    nsIScrollableFrame* scrollable = do_QueryFrame(scrollFrame);
    if (!scrollable)
        return NS_ERROR_FAILURE;

    nsMargin scrollbars = scrollable->GetActualScrollbarSizes();
    if (verticalVisible)
        *verticalVisible = scrollbars.left != 0 || scrollbars.right != 0;
    if (horizontalVisible)
        *horizontalVisible = scrollbars.top != 0 || scrollbars.bottom != 0;

    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::ScrollByLines(PRInt32 numLines)
{
    nsIScrollableView* scrollView;

    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    NS_ENSURE_SUCCESS(scrollView->ScrollByLines(0, numLines), NS_ERROR_FAILURE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ScrollByPages(PRInt32 numPages)
{
    nsIScrollableView* scrollView;

    NS_ENSURE_SUCCESS(GetRootScrollableView(&scrollView),
                      NS_ERROR_FAILURE);
    if (!scrollView) {
        return NS_ERROR_FAILURE;
    }

    NS_ENSURE_SUCCESS(scrollView->ScrollByPages(0, numPages), NS_ERROR_FAILURE);

    return NS_OK;
}





nsIScriptGlobalObject*
nsDocShell::GetScriptGlobalObject()
{
    NS_ENSURE_SUCCESS(EnsureScriptEnvironment(), nsnull);

    return mScriptGlobal;
}





NS_IMETHODIMP
nsDocShell::RefreshURI(nsIURI * aURI, PRInt32 aDelay, PRBool aRepeat,
                       PRBool aMetaRefresh)
{
    NS_ENSURE_ARG(aURI);

    





    PRBool allowRedirects = PR_TRUE;
    GetAllowMetaRedirects(&allowRedirects);
    if (!allowRedirects)
        return NS_OK;

    
    
    PRBool sameURI;
    nsresult rv = aURI->Equals(mCurrentURI, &sameURI);
    if (NS_FAILED(rv))
        sameURI = PR_FALSE;
    if (!RefreshAttempted(this, aURI, aDelay, sameURI))
        return NS_OK;

    nsRefreshTimer *refreshTimer = new nsRefreshTimer();
    NS_ENSURE_TRUE(refreshTimer, NS_ERROR_OUT_OF_MEMORY);
    PRUint32 busyFlags = 0;
    GetBusyFlags(&busyFlags);

    nsCOMPtr<nsISupports> dataRef = refreshTimer;    

    refreshTimer->mDocShell = this;
    refreshTimer->mURI = aURI;
    refreshTimer->mDelay = aDelay;
    refreshTimer->mRepeat = aRepeat;
    refreshTimer->mMetaRefresh = aMetaRefresh;

    if (!mRefreshURIList) {
        NS_ENSURE_SUCCESS(NS_NewISupportsArray(getter_AddRefs(mRefreshURIList)),
                          NS_ERROR_FAILURE);
    }

    if (busyFlags & BUSY_FLAGS_BUSY) {
        
        
        
        mRefreshURIList->AppendElement(refreshTimer);
    }
    else {
        
        
        nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
        NS_ENSURE_TRUE(timer, NS_ERROR_FAILURE);

        mRefreshURIList->AppendElement(timer);      
        timer->InitWithCallback(refreshTimer, aDelay, nsITimer::TYPE_ONE_SHOT);
    }
    return NS_OK;
}

nsresult
nsDocShell::ForceRefreshURIFromTimer(nsIURI * aURI,
                                     PRInt32 aDelay, 
                                     PRBool aMetaRefresh,
                                     nsITimer* aTimer)
{
    NS_PRECONDITION(aTimer, "Must have a timer here");

    
    if (mRefreshURIList) {
        PRUint32 n = 0;
        mRefreshURIList->Count(&n);

        for (PRUint32 i = 0;  i < n; ++i) {
            nsCOMPtr<nsITimer> timer = do_QueryElementAt(mRefreshURIList, i);
            if (timer == aTimer) {
                mRefreshURIList->RemoveElementAt(i);
                break;
            }
        }
    }

    return ForceRefreshURI(aURI, aDelay, aMetaRefresh);
}

NS_IMETHODIMP
nsDocShell::ForceRefreshURI(nsIURI * aURI,
                            PRInt32 aDelay, 
                            PRBool aMetaRefresh)
{
    NS_ENSURE_ARG(aURI);

    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    CreateLoadInfo(getter_AddRefs(loadInfo));
    NS_ENSURE_TRUE(loadInfo, NS_ERROR_OUT_OF_MEMORY);

    


    loadInfo->SetSendReferrer(PR_FALSE);

    


    loadInfo->SetReferrer(mCurrentURI);

    


    loadInfo->SetOwnerIsExplicit(PR_TRUE);

    


    PRBool equalUri = PR_FALSE;
    nsresult rv = aURI->Equals(mCurrentURI, &equalUri);
    if (NS_SUCCEEDED(rv) && (!equalUri) && aMetaRefresh &&
        aDelay <= REFRESH_REDIRECT_TIMER) {

        



        loadInfo->SetLoadType(nsIDocShellLoadInfo::loadNormalReplace);
            
        


        nsCOMPtr<nsIURI> internalReferrer;
        GetReferringURI(getter_AddRefs(internalReferrer));
        if (internalReferrer) {
            loadInfo->SetReferrer(internalReferrer);
        }
    }
    else {
        loadInfo->SetLoadType(nsIDocShellLoadInfo::loadRefresh);
    }

    



    LoadURI(aURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, PR_TRUE);

    return NS_OK;
}

nsresult
nsDocShell::SetupRefreshURIFromHeader(nsIURI * aBaseURI,
                                      const nsACString & aHeader)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    nsCAutoString uriAttrib;
    PRInt32 seconds = 0;
    PRBool specifiesSeconds = PR_FALSE;

    nsACString::const_iterator iter, tokenStart, doneIterating;

    aHeader.BeginReading(iter);
    aHeader.EndReading(doneIterating);

    
    while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter))
        ++iter;

    tokenStart = iter;

    
    if (iter != doneIterating && (*iter == '-' || *iter == '+'))
        ++iter;

    
    while (iter != doneIterating && (*iter >= '0' && *iter <= '9')) {
        seconds = seconds * 10 + (*iter - '0');
        specifiesSeconds = PR_TRUE;
        ++iter;
    }

    if (iter != doneIterating) {
        
        if (*tokenStart == '-')
            seconds = -seconds;

        
        nsACString::const_iterator iterAfterDigit = iter;
        while (iter != doneIterating && !(*iter == ';' || *iter == ','))
        {
            if (specifiesSeconds)
            {
                
                
                
                if (iter == iterAfterDigit &&
                    !nsCRT::IsAsciiSpace(*iter) && *iter != '.')
                {
                    
                    
                    
                    
                    return NS_ERROR_FAILURE;
                }
                else if (nsCRT::IsAsciiSpace(*iter))
                {
                    
                    
                    
                    ++iter;
                    break;
                }
            }
            ++iter;
        }

        
        while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter))
            ++iter;

        
        if (iter != doneIterating && (*iter == ';' || *iter == ',')) {
            ++iter;
        }

        
        while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter))
            ++iter;
    }

    
    tokenStart = iter;

    
    if (iter != doneIterating && (*iter == 'u' || *iter == 'U')) {
        ++iter;
        if (iter != doneIterating && (*iter == 'r' || *iter == 'R')) {
            ++iter;
            if (iter != doneIterating && (*iter == 'l' || *iter == 'L')) {
                ++iter;

                
                while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter))
                    ++iter;

                if (iter != doneIterating && *iter == '=') {
                    ++iter;

                    
                    while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter))
                        ++iter;

                    
                    tokenStart = iter;
                }
            }
        }
    }

    

    PRBool isQuotedURI = PR_FALSE;
    if (tokenStart != doneIterating && (*tokenStart == '"' || *tokenStart == '\''))
    {
        isQuotedURI = PR_TRUE;
        ++tokenStart;
    }

    
    iter = tokenStart;

    

    
    while (iter != doneIterating)
    {
        if (isQuotedURI && (*iter == '"' || *iter == '\''))
            break;
        ++iter;
    }

    
    if (iter != tokenStart && isQuotedURI) {
        --iter;
        if (!(*iter == '"' || *iter == '\''))
            ++iter;
    }

    
    

    nsresult rv = NS_OK;

    nsCOMPtr<nsIURI> uri;
    PRBool specifiesURI = PR_FALSE;
    if (tokenStart == iter) {
        uri = aBaseURI;
    }
    else {
        uriAttrib = Substring(tokenStart, iter);
        
        rv = NS_NewURI(getter_AddRefs(uri), uriAttrib, nsnull, aBaseURI);
        specifiesURI = PR_TRUE;
    }

    
    if (!specifiesSeconds && !specifiesURI)
    {
        
        
        return NS_ERROR_FAILURE;
    }

    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIScriptSecurityManager>
            securityManager(do_GetService
                            (NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
        if (NS_SUCCEEDED(rv)) {
            rv = securityManager->
                CheckLoadURI(aBaseURI, uri,
                             nsIScriptSecurityManager::
                             LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT);

            if (NS_SUCCEEDED(rv)) {
                PRBool isjs = PR_TRUE;
                rv = NS_URIChainHasFlags(uri,
                  nsIProtocolHandler::URI_OPENING_EXECUTES_SCRIPT, &isjs);
                NS_ENSURE_SUCCESS(rv, rv);

                if (isjs) {
                    return NS_ERROR_FAILURE;
                }
            }

            if (NS_SUCCEEDED(rv)) {
                
                
                if (seconds < 0)
                    return NS_ERROR_FAILURE;

                rv = RefreshURI(uri, seconds * 1000, PR_FALSE, PR_TRUE);
            }
        }
    }
    return rv;
}

NS_IMETHODIMP nsDocShell::SetupRefreshURI(nsIChannel * aChannel)
{
    nsresult rv;
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel, &rv));
    if (NS_SUCCEEDED(rv)) {
        nsCAutoString refreshHeader;
        rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("refresh"),
                                            refreshHeader);

        if (!refreshHeader.IsEmpty()) {
            SetupReferrerFromChannel(aChannel);
            rv = SetupRefreshURIFromHeader(mCurrentURI, refreshHeader);
            if (NS_SUCCEEDED(rv)) {
                return NS_REFRESHURI_HEADER_FOUND;
            }
        }
    }
    return rv;
}

static void
DoCancelRefreshURITimers(nsISupportsArray* aTimerList)
{
    if (!aTimerList)
        return;

    PRUint32 n=0;
    aTimerList->Count(&n);

    while (n) {
        nsCOMPtr<nsITimer> timer(do_QueryElementAt(aTimerList, --n));

        aTimerList->RemoveElementAt(n);    

        if (timer)
            timer->Cancel();        
    }
}

NS_IMETHODIMP
nsDocShell::CancelRefreshURITimers()
{
    DoCancelRefreshURITimers(mRefreshURIList);
    DoCancelRefreshURITimers(mSavedRefreshURIList);
    mRefreshURIList = nsnull;
    mSavedRefreshURIList = nsnull;

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRefreshPending(PRBool* _retval)
{
    if (!mRefreshURIList) {
        *_retval = PR_FALSE;
        return NS_OK;
    }

    PRUint32 count;
    nsresult rv = mRefreshURIList->Count(&count);
    if (NS_SUCCEEDED(rv))
        *_retval = (count != 0);
    return rv;
}

NS_IMETHODIMP
nsDocShell::SuspendRefreshURIs()
{
    if (mRefreshURIList) {
        PRUint32 n = 0;
        mRefreshURIList->Count(&n);

        for (PRUint32 i = 0;  i < n; ++i) {
            nsCOMPtr<nsITimer> timer = do_QueryElementAt(mRefreshURIList, i);
            if (!timer)
                continue;  

            
            nsCOMPtr<nsITimerCallback> callback;
            timer->GetCallback(getter_AddRefs(callback));

            timer->Cancel();

            nsCOMPtr<nsITimerCallback> rt = do_QueryInterface(callback);
            NS_ASSERTION(rt, "RefreshURIList timer callbacks should only be RefreshTimer objects");

            mRefreshURIList->ReplaceElementAt(rt, i);
        }
    }

    
    PRInt32 n = mChildList.Count();

    for (PRInt32 i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell)
            shell->SuspendRefreshURIs();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ResumeRefreshURIs()
{
    RefreshURIFromQueue();

    
    PRInt32 n = mChildList.Count();

    for (PRInt32 i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell)
            shell->ResumeRefreshURIs();
    }

    return NS_OK;
}

nsresult
nsDocShell::RefreshURIFromQueue()
{
    if (!mRefreshURIList)
        return NS_OK;
    PRUint32 n = 0;
    mRefreshURIList->Count(&n);

    while (n) {
        nsCOMPtr<nsISupports> element;
        mRefreshURIList->GetElementAt(--n, getter_AddRefs(element));
        nsCOMPtr<nsITimerCallback> refreshInfo(do_QueryInterface(element));

        if (refreshInfo) {   
            
            
            
            PRUint32 delay = static_cast<nsRefreshTimer*>(static_cast<nsITimerCallback*>(refreshInfo))->GetDelay();
            nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
            if (timer) {    
                
                
                
                
                mRefreshURIList->ReplaceElementAt(timer, n);
                timer->InitWithCallback(refreshInfo, delay, nsITimer::TYPE_ONE_SHOT);
            }           
        }        
    }  
 
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::Embed(nsIContentViewer * aContentViewer,
                  const char *aCommand, nsISupports * aExtraInfo)
{
    
    
    PersistLayoutHistoryState();

    nsresult rv = SetupNewViewer(aContentViewer);

    
    
    
    
    if (mCurrentURI &&
       (mLoadType & LOAD_CMD_HISTORY ||
        mLoadType == LOAD_RELOAD_NORMAL ||
        mLoadType == LOAD_RELOAD_CHARSET_CHANGE)){
        PRBool isWyciwyg = PR_FALSE;
        
        rv = mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);      
        if (isWyciwyg && NS_SUCCEEDED(rv))
            SetBaseUrlForWyciwyg(aContentViewer);
    }
    
    if (mLSHE) {
        
        if (mLSHE->HasDetachedEditor()) {
            ReattachEditorToWindow(mLSHE);
        }
        SetHistoryEntry(&mOSHE, mLSHE);
    }

    PRBool updateHistory = PR_TRUE;

    
    switch (mLoadType) {
    case LOAD_NORMAL_REPLACE:
    case LOAD_STOP_CONTENT_AND_REPLACE:
    case LOAD_RELOAD_BYPASS_CACHE:
    case LOAD_RELOAD_BYPASS_PROXY:
    case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:
        updateHistory = PR_FALSE;
        break;
    default:
        break;
    }

    if (!updateHistory)
        SetLayoutHistoryState(nsnull);

    return NS_OK;
}


NS_IMETHODIMP 
nsDocShell::SetIsPrinting(PRBool aIsPrinting)
{
    mIsPrintingOrPP = aIsPrinting;
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::OnProgressChange(nsIWebProgress * aProgress,
                             nsIRequest * aRequest,
                             PRInt32 aCurSelfProgress,
                             PRInt32 aMaxSelfProgress,
                             PRInt32 aCurTotalProgress,
                             PRInt32 aMaxTotalProgress)
{
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnStateChange(nsIWebProgress * aProgress, nsIRequest * aRequest,
                          PRUint32 aStateFlags, nsresult aStatus)
{
    nsresult rv;

    
    if ((~aStateFlags & (STATE_START | STATE_IS_NETWORK)) == 0) {
        nsCOMPtr<nsIWyciwygChannel>  wcwgChannel(do_QueryInterface(aRequest));
        nsCOMPtr<nsIWebProgress> webProgress =
            do_QueryInterface(GetAsSupports(this));

        
        if (wcwgChannel && !mLSHE && (mItemType == typeContent) && aProgress == webProgress.get()) {
            nsCOMPtr<nsIURI> uri;
            wcwgChannel->GetURI(getter_AddRefs(uri));
        
            PRBool equalUri = PR_TRUE;
            
            
            
            if (mCurrentURI &&
                NS_SUCCEEDED(uri->Equals(mCurrentURI, &equalUri)) &&
                !equalUri) {
                
                
                rv = AddToSessionHistory(uri, wcwgChannel, nsnull,
                                         getter_AddRefs(mLSHE));
                SetCurrentURI(uri, aRequest, PR_TRUE);
                
                rv = PersistLayoutHistoryState();
                
                
                SetHistoryEntry(&mOSHE, mLSHE);
            }
        
        }
        
        mBusyFlags = BUSY_FLAGS_BUSY | BUSY_FLAGS_BEFORE_PAGE_LOAD;
    }
    else if ((~aStateFlags & (STATE_TRANSFERRING | STATE_IS_DOCUMENT)) == 0) {
        
        mBusyFlags = BUSY_FLAGS_BUSY | BUSY_FLAGS_PAGE_LOADING;
    }
    else if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_NETWORK)) {
        
        mBusyFlags = BUSY_FLAGS_NONE;
    }
    if ((~aStateFlags & (STATE_IS_DOCUMENT | STATE_STOP)) == 0) {
        nsCOMPtr<nsIWebProgress> webProgress =
            do_QueryInterface(GetAsSupports(this));
        
        if (aProgress == webProgress.get()) {
            nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
            EndPageLoad(aProgress, channel, aStatus);
        }
    }
    
    
    
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnLocationChange(nsIWebProgress * aProgress,
                             nsIRequest * aRequest, nsIURI * aURI)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

void
nsDocShell::OnRedirectStateChange(nsIChannel* aOldChannel,
                                  nsIChannel* aNewChannel,
                                  PRUint32 aRedirectFlags,
                                  PRUint32 aStateFlags)
{
    NS_ASSERTION(aStateFlags & STATE_REDIRECTING,
                 "Calling OnRedirectStateChange when there is no redirect");
    if (!(aStateFlags & STATE_IS_DOCUMENT))
        return; 

    
    
    if (mClassifier) {
        mClassifier->OnRedirect(aOldChannel, aNewChannel);
    }

    nsCOMPtr<nsIGlobalHistory3> history3(do_QueryInterface(mGlobalHistory));
    nsresult result = NS_ERROR_NOT_IMPLEMENTED;
    if (history3) {
        
        result = history3->AddDocumentRedirect(aOldChannel, aNewChannel,
                                               aRedirectFlags, !IsFrame());
    }

    if (result == NS_ERROR_NOT_IMPLEMENTED) {
        
        
        
        
        nsCOMPtr<nsIURI> oldURI;
        aOldChannel->GetURI(getter_AddRefs(oldURI));
        if (! oldURI)
            return; 
        AddToGlobalHistory(oldURI, PR_TRUE, aOldChannel);
    }

    
    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(aNewChannel);
    if (appCacheChannel) {
        nsCOMPtr<nsIURI> newURI;
        aNewChannel->GetURI(getter_AddRefs(newURI));
        appCacheChannel->SetChooseApplicationCache(ShouldCheckAppCache(newURI));
    }
}

NS_IMETHODIMP
nsDocShell::OnStatusChange(nsIWebProgress * aWebProgress,
                           nsIRequest * aRequest,
                           nsresult aStatus, const PRUnichar * aMessage)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnSecurityChange(nsIWebProgress * aWebProgress,
                             nsIRequest * aRequest, PRUint32 state)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}


nsresult
nsDocShell::EndPageLoad(nsIWebProgress * aProgress,
                        nsIChannel * aChannel, nsresult aStatus)
{
    if(!aChannel)
        return NS_ERROR_NULL_POINTER;
    
    nsCOMPtr<nsIURI> url;
    nsresult rv = aChannel->GetURI(getter_AddRefs(url));
    if (NS_FAILED(rv)) return rv;
  
    
    if (eCharsetReloadRequested == mCharsetReloadState)
        mCharsetReloadState = eCharsetReloadStopOrigional;
    else 
        mCharsetReloadState = eCharsetReloadInit;

    
    
    
    nsCOMPtr<nsISHEntry> loadingSHE = mLSHE;
  
    
    
    
    
    
    nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

    
    mClassifier = nsnull;

    
    
    
    
    
    
    if (!mEODForCurrentDocument && mContentViewer) {
        mIsExecutingOnLoadHandler = PR_TRUE;
        mContentViewer->LoadComplete(aStatus);
        mIsExecutingOnLoadHandler = PR_FALSE;

        mEODForCurrentDocument = PR_TRUE;

        
        
        
        if (--gNumberOfDocumentsLoading == 0) {
          
          FavorPerformanceHint(PR_FALSE, NS_EVENT_STARVATION_DELAY_HINT);
        }
    }
    




    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
    if (!httpChannel) 
        GetHttpChannel(aChannel, getter_AddRefs(httpChannel));

    if (httpChannel) {
        
        PRBool discardLayoutState = ShouldDiscardLayoutState(httpChannel);       
        if (mLSHE && discardLayoutState && (mLoadType & LOAD_CMD_NORMAL) &&
            (mLoadType != LOAD_BYPASS_HISTORY) && (mLoadType != LOAD_ERROR_PAGE))
            mLSHE->SetSaveLayoutStateFlag(PR_FALSE);            
    }

    
    
    
    if (mLSHE) {
        mLSHE->SetLoadType(nsIDocShellLoadInfo::loadHistory);

        
        
        SetHistoryEntry(&mLSHE, nsnull);
    }
    
    
    RefreshURIFromQueue();

    
    PRBool isTopFrame = PR_TRUE;
    nsCOMPtr<nsIDocShellTreeItem> targetParentTreeItem;
    rv = GetSameTypeParent(getter_AddRefs(targetParentTreeItem));
    if (NS_SUCCEEDED(rv) && targetParentTreeItem) {
        isTopFrame = PR_FALSE;
    }

    
    
    
    
    
    
    
    
    
    
    if (url && NS_FAILED(aStatus)) {
        if (aStatus == NS_ERROR_FILE_NOT_FOUND ||
            aStatus == NS_ERROR_INVALID_CONTENT_ENCODING) {
            DisplayLoadError(aStatus, url, nsnull, aChannel);
            return NS_OK;
        }

        if (sURIFixup) {
            
            
            
            nsCOMPtr<nsIURI> newURI;

            nsCAutoString oldSpec;
            url->GetSpec(oldSpec);
      
            
            
            
            if (aStatus == NS_ERROR_UNKNOWN_HOST && mAllowKeywordFixup) {
                PRBool keywordsEnabled = PR_FALSE;

                if (mPrefs &&
                    NS_FAILED(mPrefs->GetBoolPref("keyword.enabled",
                                                  &keywordsEnabled)))
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

                if (keywordsEnabled && (kNotFound == dotLoc)) {
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    
                    PRBool isACE;
                    nsCAutoString utf8Host;
                    nsCOMPtr<nsIIDNService> idnSrv =
                        do_GetService(NS_IDNSERVICE_CONTRACTID);
                    if (idnSrv &&
                        NS_SUCCEEDED(idnSrv->IsACE(host, &isACE)) && isACE &&
                        NS_SUCCEEDED(idnSrv->ConvertACEtoUTF8(host, utf8Host)))
                        sURIFixup->KeywordToURI(utf8Host,
                                                getter_AddRefs(newURI));
                    else
                        sURIFixup->KeywordToURI(host, getter_AddRefs(newURI));
                } 
            }

            
            
            
            
            if (aStatus == NS_ERROR_UNKNOWN_HOST ||
                aStatus == NS_ERROR_NET_RESET) {
                PRBool doCreateAlternate = PR_TRUE;

                
                
        
                if (mLoadType != LOAD_NORMAL || !isTopFrame) {
                    doCreateAlternate = PR_FALSE;
                }
                else {
                    
                    if (newURI) {
                        PRBool sameURI = PR_FALSE;
                        url->Equals(newURI, &sameURI);
                        if (!sameURI) {
                            
                            
                            doCreateAlternate = PR_FALSE;
                        }
                    }
                }
                if (doCreateAlternate) {
                    newURI = nsnull;
                    sURIFixup->CreateFixupURI(oldSpec,
                      nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI,
                                              getter_AddRefs(newURI));
                }
            }

            
            
            
            if (newURI) {
                
                
                PRBool sameURI = PR_FALSE;
                url->Equals(newURI, &sameURI);
                if (!sameURI) {
                    nsCAutoString newSpec;
                    newURI->GetSpec(newSpec);
                    NS_ConvertUTF8toUTF16 newSpecW(newSpec);

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
            DisplayLoadError(aStatus, url, nsnull, aChannel);
        }
        
        else if (aStatus == NS_ERROR_NET_TIMEOUT ||
                 aStatus == NS_ERROR_REDIRECT_LOOP ||
                 aStatus == NS_ERROR_UNKNOWN_SOCKET_TYPE ||
                 aStatus == NS_ERROR_NET_INTERRUPT ||
                 aStatus == NS_ERROR_NET_RESET ||
                 aStatus == NS_ERROR_MALWARE_URI ||
                 aStatus == NS_ERROR_PHISHING_URI ||
                 aStatus == NS_ERROR_UNSAFE_CONTENT_TYPE ||
                 NS_ERROR_GET_MODULE(aStatus) == NS_ERROR_MODULE_SECURITY) {
            DisplayLoadError(aStatus, url, nsnull, aChannel);
        }
        else if (aStatus == NS_ERROR_DOCUMENT_NOT_CACHED) {
            






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
                        
                        nsCOMPtr<nsIWebNavigation> rootAsWebnav =
                            do_QueryInterface(root);
                        if (rootAsWebnav) {
                            
                            rootAsWebnav->GetSessionHistory(getter_AddRefs(rootSH));
                        }
                    }
                }  

                if (rootSH && (mLoadType & LOAD_CMD_HISTORY)) {
                    nsCOMPtr<nsISHistoryInternal> shInternal =
                        do_QueryInterface(rootSH);
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
                if (httpChannel) {
                    httpChannel->GetReferrer(getter_AddRefs(referrer));
                    nsCOMPtr<nsIUploadChannel> uploadChannel =
                        do_QueryInterface(aChannel);
                    if (uploadChannel) {
                        uploadChannel->GetUploadStream(getter_AddRefs(inputStream));
                    }
                }
                nsCOMPtr<nsISeekableStream> postDataSeekable =
                    do_QueryInterface(inputStream);
                if (postDataSeekable) {
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
                DisplayLoadError(aStatus, url, nsnull, aChannel);
            }
        }
  } 

  return NS_OK;
}






NS_IMETHODIMP
nsDocShell::EnsureContentViewer()
{
    if (mContentViewer)
        return NS_OK;
    if (mIsBeingDestroyed)
        return NS_ERROR_FAILURE;

    nsIPrincipal* principal = nsnull;
    nsCOMPtr<nsIURI> baseURI;

    nsCOMPtr<nsPIDOMWindow> piDOMWindow(do_QueryInterface(mScriptGlobal));
    if (piDOMWindow) {
        principal = piDOMWindow->GetOpenerScriptPrincipal();
    }

    if (!principal) {
        principal = GetInheritedPrincipal(PR_FALSE);
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        GetSameTypeParent(getter_AddRefs(parentItem));
        if (parentItem) {
            nsCOMPtr<nsPIDOMWindow> domWin = do_GetInterface(GetAsSupports(this));
            if (domWin) {
                nsCOMPtr<nsIContent> parentContent =
                    do_QueryInterface(domWin->GetFrameElementInternal());
                if (parentContent) {
                    baseURI = parentContent->GetBaseURI();
                }
            }
        }
    }

    nsresult rv = CreateAboutBlankContentViewer(principal, baseURI);

    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIDOMDocument> domDoc;
        mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));
        nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
        NS_ASSERTION(doc,
                     "Should have doc if CreateAboutBlankContentViewer "
                     "succeeded!");

        doc->SetIsInitialDocument(PR_TRUE);
    }

    return rv;
}

nsresult
nsDocShell::CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal,
                                          nsIURI* aBaseURI)
{
  nsCOMPtr<nsIDocument> blankDoc;
  nsCOMPtr<nsIContentViewer> viewer;
  nsresult rv = NS_ERROR_FAILURE;

  


  NS_ASSERTION(!mCreatingDocument, "infinite(?) loop creating document averted");
  if (mCreatingDocument)
    return NS_ERROR_FAILURE;

  mCreatingDocument = PR_TRUE;

  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);
  
  if (mContentViewer) {
    
    
    
    

    PRBool okToUnload;
    rv = mContentViewer->PermitUnload(&okToUnload);

    if (NS_SUCCEEDED(rv) && !okToUnload) {
      
      return NS_ERROR_FAILURE;
    }

    mSavingOldViewer = CanSavePresentation(LOAD_NORMAL, nsnull, nsnull);

    
    
    mLoadingURI = nsnull;
    
    
    
    
    
    
    
    (void) FirePageHideNotification(!mSavingOldViewer);
  }

  
  
  
  
  mFiredUnloadEvent = PR_FALSE;

  
  nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID));
  if (!catMan)
    return NS_ERROR_FAILURE;

  nsXPIDLCString contractId;
  rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", "text/html", getter_Copies(contractId));
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIDocumentLoaderFactory> docFactory(do_GetService(contractId));
  if (docFactory) {
    
    docFactory->CreateBlankDocument(mLoadGroup, aPrincipal,
                                    getter_AddRefs(blankDoc));
    if (blankDoc) {
      
      
      blankDoc->SetBaseURI(aBaseURI);

      blankDoc->SetContainer(static_cast<nsIDocShell *>(this));

      
      docFactory->CreateInstanceForDocument(NS_ISUPPORTS_CAST(nsIDocShell *, this),
                    blankDoc, "view", getter_AddRefs(viewer));

      
      if (viewer) {
        viewer->SetContainer(static_cast<nsIContentViewerContainer *>(this));
        nsCOMPtr<nsIDOMDocument> domdoc(do_QueryInterface(blankDoc));
        Embed(viewer, "", 0);
        viewer->SetDOMDocument(domdoc);

        SetCurrentURI(blankDoc->GetDocumentURI(), nsnull, PR_TRUE);
        rv = NS_OK;
      }
    }
  }
  mCreatingDocument = PR_FALSE;

  
  SetHistoryEntry(&mOSHE, nsnull);

  return rv;
}

PRBool
nsDocShell::CanSavePresentation(PRUint32 aLoadType,
                                nsIRequest *aNewRequest,
                                nsIDocument *aNewDocument)
{
    if (!mOSHE)
        return PR_FALSE; 

    
    
    
    if (aLoadType != LOAD_NORMAL &&
        aLoadType != LOAD_HISTORY &&
        aLoadType != LOAD_LINK &&
        aLoadType != LOAD_STOP_CONTENT &&
        aLoadType != LOAD_STOP_CONTENT_AND_REPLACE &&
        aLoadType != LOAD_ERROR_PAGE)
        return PR_FALSE;

    
    
    PRBool canSaveState;
    mOSHE->GetSaveLayoutStateFlag(&canSaveState);
    if (canSaveState == PR_FALSE)
        return PR_FALSE;

    
    nsCOMPtr<nsPIDOMWindow> pWin = do_QueryInterface(mScriptGlobal);
    if (!pWin || pWin->IsLoading())
        return PR_FALSE;

    if (pWin->WouldReuseInnerWindow(aNewDocument))
        return PR_FALSE;

    
    
    if (nsSHistory::GetMaxTotalViewers() == 0)
        return PR_FALSE;

    
    
    PRBool cacheFrames = PR_FALSE;
    mPrefs->GetBoolPref("browser.sessionhistory.cache_subframes",
                        &cacheFrames);
    if (!cacheFrames) {
        nsCOMPtr<nsIDocShellTreeItem> root;
        GetSameTypeParent(getter_AddRefs(root));
        if (root && root != this) {
            return PR_FALSE;  
        }
    }

    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(pWin->GetExtantDocument());
    if (!doc || !doc->CanSavePresentation(aNewRequest))
        return PR_FALSE;

    return PR_TRUE;
}

void
nsDocShell::ReattachEditorToWindow(nsISHEntry *aSHEntry)
{
    NS_ASSERTION(!mEditorData,
                 "Why reattach an editor when we already have one?");
    NS_ASSERTION(aSHEntry && aSHEntry->HasDetachedEditor(),
                 "Reattaching when there's not a detached editor.");

    if (mEditorData || !aSHEntry)
      return;

    mEditorData = aSHEntry->ForgetEditorData();
    if (mEditorData) {
        nsresult res = mEditorData->ReattachToWindow(this);
        NS_ASSERTION(NS_SUCCEEDED(res), "Failed to reattach editing session");
    }
}

void
nsDocShell::DetachEditorFromWindow()
{
    if (!mEditorData || mEditorData->WaitingForLoad()) {
        
        
        return;
    }

    NS_ASSERTION(!mOSHE || !mOSHE->HasDetachedEditor(),
                 "Detaching editor when it's already detached.");

    nsresult res = mEditorData->DetachFromWindow();
    NS_ASSERTION(NS_SUCCEEDED(res), "Failed to detach editor");

    if (NS_SUCCEEDED(res)) {
        
        if (mOSHE)
            mOSHE->SetEditorData(mEditorData.forget());
        else
            mEditorData = nsnull;
    }

#ifdef DEBUG
    {
        PRBool isEditable;
        GetEditable(&isEditable);
        NS_ASSERTION(!isEditable,
                     "Window is still editable after detaching editor.");
    }
#endif 
}

nsresult
nsDocShell::CaptureState()
{
    if (!mOSHE || mOSHE == mLSHE) {
        
        return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsPIDOMWindow> privWin = do_QueryInterface(mScriptGlobal);
    if (!privWin)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsISupports> windowState;
    nsresult rv = privWin->SaveWindowState(getter_AddRefs(windowState));
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG_PAGE_CACHE
    nsCOMPtr<nsIURI> uri;
    mOSHE->GetURI(getter_AddRefs(uri));
    nsCAutoString spec;
    if (uri)
        uri->GetSpec(spec);
    printf("Saving presentation into session history\n");
    printf("  SH URI: %s\n", spec.get());
#endif

    rv = mOSHE->SetWindowState(windowState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    rv = mOSHE->SetRefreshURIList(mSavedRefreshURIList);
    NS_ENSURE_SUCCESS(rv, rv);

    
    if (mContentViewer) {
        nsIntRect bounds;
        mContentViewer->GetBounds(bounds);
        rv = mOSHE->SetViewerBounds(bounds);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    
    mOSHE->ClearChildShells();

    PRInt32 childCount = mChildList.Count();
    for (PRInt32 i = 0; i < childCount; ++i) {
        nsCOMPtr<nsIDocShellTreeItem> childShell = do_QueryInterface(ChildAt(i));
        NS_ASSERTION(childShell, "null child shell");

        mOSHE->AddChildShell(childShell);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::RestorePresentationEvent::Run()
{
    if (mDocShell && NS_FAILED(mDocShell->RestoreFromHistory()))
        NS_WARNING("RestoreFromHistory failed");
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::BeginRestore(nsIContentViewer *aContentViewer, PRBool aTop)
{
    nsresult rv;
    if (!aContentViewer) {
        rv = EnsureContentViewer();
        NS_ENSURE_SUCCESS(rv, rv);

        aContentViewer = mContentViewer;
    }

    
    
    
    

    nsCOMPtr<nsIDOMDocument> domDoc;
    aContentViewer->GetDOMDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    if (doc) {
        nsIChannel *channel = doc->GetChannel();
        if (channel) {
            mEODForCurrentDocument = PR_FALSE;
            mIsRestoringDocument = PR_TRUE;
            mLoadGroup->AddRequest(channel, nsnull);
            mIsRestoringDocument = PR_FALSE;
        }
    }

    if (!aTop) {
        
        
        
        
        mFiredUnloadEvent = PR_FALSE;
        
        
        
        
        
        rv = BeginRestoreChildren();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
nsDocShell::BeginRestoreChildren()
{
    PRInt32 n = mChildList.Count();
    for (PRInt32 i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child) {
            nsresult rv = child->BeginRestore(nsnull, PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::FinishRestore()
{
    
    

    PRInt32 n = mChildList.Count();
    for (PRInt32 i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child) {
            child->FinishRestore();
        }
    }

    if (mOSHE && mOSHE->HasDetachedEditor()) {
      ReattachEditorToWindow(mOSHE);
    }

    if (mContentViewer) {
        nsCOMPtr<nsIDOMDocument> domDoc;
        mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));

        nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
        if (doc) {
            
            
            

            nsIChannel *channel = doc->GetChannel();
            if (channel) {
                mIsRestoringDocument = PR_TRUE;
                mLoadGroup->RemoveRequest(channel, nsnull, NS_OK);
                mIsRestoringDocument = PR_FALSE;
            }
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRestoringDocument(PRBool *aRestoring)
{
    *aRestoring = mIsRestoringDocument;
    return NS_OK;
}

nsresult
nsDocShell::RestorePresentation(nsISHEntry *aSHEntry, PRBool *aRestoring)
{
    NS_ASSERTION(mLoadType & LOAD_CMD_HISTORY,
                 "RestorePresentation should only be called for history loads");

    nsCOMPtr<nsIContentViewer> viewer;
    aSHEntry->GetContentViewer(getter_AddRefs(viewer));

#ifdef DEBUG_PAGE_CACHE
    nsCOMPtr<nsIURI> uri;
    aSHEntry->GetURI(getter_AddRefs(uri));

    nsCAutoString spec;
    if (uri)
        uri->GetSpec(spec);
#endif

    *aRestoring = PR_FALSE;

    if (!viewer) {
#ifdef DEBUG_PAGE_CACHE
        printf("no saved presentation for uri: %s\n", spec.get());
#endif
        return NS_OK;
    }

    
    
    
    
    

    nsCOMPtr<nsISupports> container;
    viewer->GetContainer(getter_AddRefs(container));
    if (!::SameCOMIdentity(container, GetAsSupports(this))) {
#ifdef DEBUG_PAGE_CACHE
        printf("No valid container, clearing presentation\n");
#endif
        aSHEntry->SetContentViewer(nsnull);
        return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(mContentViewer != viewer, "Restoring existing presentation");

#ifdef DEBUG_PAGE_CACHE
    printf("restoring presentation from session history: %s\n", spec.get());
#endif

    SetHistoryEntry(&mLSHE, aSHEntry);

    
    
    
    
    

    BeginRestore(viewer, PR_TRUE);

    
    
    

    
    NS_ASSERTION(!mRestorePresentationEvent.IsPending(),
        "should only have one RestorePresentationEvent");
    mRestorePresentationEvent.Revoke();

    nsRefPtr<RestorePresentationEvent> evt = new RestorePresentationEvent(this);
    nsresult rv = NS_DispatchToCurrentThread(evt);
    if (NS_SUCCEEDED(rv)) {
        mRestorePresentationEvent = evt.get();
        
        
        *aRestoring = PR_TRUE;
    }

    return rv;
}

nsresult
nsDocShell::RestoreFromHistory()
{
    mRestorePresentationEvent.Forget();

    
    if (!mLSHE)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIContentViewer> viewer;
    mLSHE->GetContentViewer(getter_AddRefs(viewer));
    if (!viewer)
        return NS_ERROR_FAILURE;

    if (mSavingOldViewer) {
        
        
        
        
        nsCOMPtr<nsIDOMDocument> domDoc;
        viewer->GetDOMDocument(getter_AddRefs(domDoc));
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
        nsIRequest *request = nsnull;
        if (doc)
            request = doc->GetChannel();
        mSavingOldViewer = CanSavePresentation(mLoadType, request, doc);
    }

    nsCOMPtr<nsIMarkupDocumentViewer> oldMUDV(do_QueryInterface(mContentViewer));
    nsCOMPtr<nsIMarkupDocumentViewer> newMUDV(do_QueryInterface(viewer));
    float textZoom = 1.0f;
    float pageZoom = 1.0f;
    if (oldMUDV && newMUDV) {
        oldMUDV->GetTextZoom(&textZoom);
        oldMUDV->GetFullZoom(&pageZoom);
    }

    
    
    nsCOMPtr<nsISHEntry> origLSHE = mLSHE;

    
    
    mLoadingURI = nsnull;
    
    
    FirePageHideNotification(!mSavingOldViewer);

    
    
    if (mLSHE != origLSHE)
      return NS_OK;

    
    
    mFiredUnloadEvent = PR_FALSE;

    mURIResultedInDocument = PR_TRUE;
    nsCOMPtr<nsISHistory> rootSH;
    GetRootSessionHistory(getter_AddRefs(rootSH));
    if (rootSH) {
        nsCOMPtr<nsISHistoryInternal> hist = do_QueryInterface(rootSH);
        rootSH->GetIndex(&mPreviousTransIndex);
        hist->UpdateIndex();
        rootSH->GetIndex(&mLoadedTransIndex);
#ifdef DEBUG_PAGE_CACHE
        printf("Previous index: %d, Loaded index: %d\n\n", mPreviousTransIndex,
                   mLoadedTransIndex);
#endif
    }

    
    
    
    PersistLayoutHistoryState();
    nsresult rv;
    if (mContentViewer) {
        if (mSavingOldViewer && NS_FAILED(CaptureState())) {
            if (mOSHE) {
                mOSHE->SyncPresentationState();
            }
            mSavingOldViewer = PR_FALSE;
        }
    }

    mSavedRefreshURIList = nsnull;

    
    
    
    
    

    if (mContentViewer) {
        nsCOMPtr<nsIContentViewer> previousViewer;
        mContentViewer->GetPreviousViewer(getter_AddRefs(previousViewer));
        if (previousViewer) {
            mContentViewer->SetPreviousViewer(nsnull);
            previousViewer->Destroy();
        }
    }

    
    
    

    nsIView *rootViewSibling = nsnull, *rootViewParent = nsnull;
    nsIntRect newBounds(0, 0, 0, 0);

    nsCOMPtr<nsIPresShell> oldPresShell;
    nsDocShell::GetPresShell(getter_AddRefs(oldPresShell));
    if (oldPresShell) {
        nsIViewManager *vm = oldPresShell->GetViewManager();
        if (vm) {
            nsIView *oldRootView = nsnull;
            vm->GetRootView(oldRootView);

            if (oldRootView) {
                rootViewSibling = oldRootView->GetNextSibling();
                rootViewParent = oldRootView->GetParent();

                mContentViewer->GetBounds(newBounds);
            }
        }
    }

    
    
    
    

    if (mContentViewer) {
        mContentViewer->Close(mSavingOldViewer ? mOSHE.get() : nsnull);
        viewer->SetPreviousViewer(mContentViewer);
    }

    mContentViewer.swap(viewer);
    viewer = nsnull; 

    
    
    nsCOMPtr<nsISupports> windowState;
    mLSHE->GetWindowState(getter_AddRefs(windowState));
    mLSHE->SetWindowState(nsnull);

    PRBool sticky;
    mLSHE->GetSticky(&sticky);

    nsCOMPtr<nsIDOMDocument> domDoc;
    mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));

    nsCOMArray<nsIDocShellTreeItem> childShells;
    PRInt32 i = 0;
    nsCOMPtr<nsIDocShellTreeItem> child;
    while (NS_SUCCEEDED(mLSHE->ChildShellAt(i++, getter_AddRefs(child))) &&
           child) {
        childShells.AppendObject(child);
    }

    
    nsIntRect oldBounds(0, 0, 0, 0);
    mLSHE->GetViewerBounds(oldBounds);

    
    
    nsCOMPtr<nsISupportsArray> refreshURIList;
    mLSHE->GetRefreshURIList(getter_AddRefs(refreshURIList));

    
    rv = mContentViewer->Open(windowState, mLSHE);

    
    mLSHE->SetContentViewer(nsnull);
    mEODForCurrentDocument = PR_FALSE;

#ifdef DEBUG
 {
     nsCOMPtr<nsISupportsArray> refreshURIs;
     mLSHE->GetRefreshURIList(getter_AddRefs(refreshURIs));
     nsCOMPtr<nsIDocShellTreeItem> childShell;
     mLSHE->ChildShellAt(0, getter_AddRefs(childShell));
     NS_ASSERTION(!refreshURIs && !childShell,
                  "SHEntry should have cleared presentation state");
 }
#endif

    
    
    
    mContentViewer->SetSticky(sticky);

    
    DestroyChildren();
    NS_ENSURE_SUCCESS(rv, rv);

    
    SetHistoryEntry(&mOSHE, mLSHE);
    
    

    
    
    SetLayoutHistoryState(nsnull);

    

    mSavingOldViewer = PR_FALSE;
    mEODForCurrentDocument = PR_FALSE;

    
    
    if (++gNumberOfDocumentsLoading == 1)
        FavorPerformanceHint(PR_TRUE, NS_EVENT_STARVATION_DELAY_HINT);


    if (oldMUDV && newMUDV) {
        newMUDV->SetTextZoom(textZoom);
        newMUDV->SetFullZoom(pageZoom);
    }

    nsCOMPtr<nsIDocument> document = do_QueryInterface(domDoc);
    PRUint32 parentSuspendCount = 0;
    if (document) {
        nsCOMPtr<nsIDocShellTreeItem> parent;
        GetParent(getter_AddRefs(parent));
        nsCOMPtr<nsIDOMDocument> parentDoc = do_GetInterface(parent);
        nsCOMPtr<nsIDocument> d = do_QueryInterface(parentDoc);
        if (d) {
            if (d->EventHandlingSuppressed()) {
                document->SuppressEventHandling(d->EventHandlingSuppressed());
            }
            nsCOMPtr<nsPIDOMWindow> parentWindow = d->GetWindow();
            if (parentWindow) {
                parentSuspendCount = parentWindow->TimeoutSuspendCount();
            }
        }

        
        
        
        
        
        nsCOMPtr<nsIURI> uri;
        origLSHE->GetURI(getter_AddRefs(uri));
        SetCurrentURI(uri, document->GetChannel(), PR_TRUE);
    }

    
    
    
    nsCOMPtr<nsPIDOMWindow> privWin =
        do_GetInterface(static_cast<nsIInterfaceRequestor*>(this));
    NS_ASSERTION(privWin, "could not get nsPIDOMWindow interface");

    rv = privWin->RestoreWindowState(windowState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    document->NotifyPossibleTitleChange(PR_FALSE);

    
    for (i = 0; i < childShells.Count(); ++i) {
        nsIDocShellTreeItem *childItem = childShells.ObjectAt(i);
        nsCOMPtr<nsIDocShell> childShell = do_QueryInterface(childItem);

        
        
        PRBool allowPlugins;
        childShell->GetAllowPlugins(&allowPlugins);

        PRBool allowJavascript;
        childShell->GetAllowJavascript(&allowJavascript);

        PRBool allowRedirects;
        childShell->GetAllowMetaRedirects(&allowRedirects);

        PRBool allowSubframes;
        childShell->GetAllowSubframes(&allowSubframes);

        PRBool allowImages;
        childShell->GetAllowImages(&allowImages);

        PRBool allowDNSPrefetch;
        childShell->GetAllowDNSPrefetch(&allowDNSPrefetch);
        
        AddChild(childItem);

        childShell->SetAllowPlugins(allowPlugins);
        childShell->SetAllowJavascript(allowJavascript);
        childShell->SetAllowMetaRedirects(allowRedirects);
        childShell->SetAllowSubframes(allowSubframes);
        childShell->SetAllowImages(allowImages);
        childShell->SetAllowDNSPrefetch(allowDNSPrefetch);

        rv = childShell->BeginRestore(nsnull, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIPresShell> shell;
    nsDocShell::GetPresShell(getter_AddRefs(shell));

    nsIViewManager *newVM = shell ? shell->GetViewManager() : nsnull;
    nsIView *newRootView = nsnull;
    if (newVM)
        newVM->GetRootView(newRootView);

    
    if (rootViewParent) {
        nsIViewManager *parentVM = rootViewParent->GetViewManager();

        if (parentVM && newRootView) {
            
            
            
            
            
            
            
            parentVM->InsertChild(rootViewParent, newRootView,
                                  rootViewSibling,
                                  rootViewSibling ? PR_TRUE : PR_FALSE);

            NS_ASSERTION(newRootView->GetNextSibling() == rootViewSibling,
                         "error in InsertChild");
        }
    }

    
    
    
    if (parentSuspendCount) {
      privWin->SuspendTimeouts(parentSuspendCount, PR_FALSE);
    }

    
    
    privWin->ResumeTimeouts();

    
    
    mRefreshURIList = refreshURIList;

    
    
    PRInt32 n = mChildList.Count();
    for (i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child)
            child->ResumeRefreshURIs();
    }

    
    
    

    
    
    
    
    
    
    
    

    if (newRootView) {
        if (!newBounds.IsEmpty() && newBounds != oldBounds) {
#ifdef DEBUG_PAGE_CACHE
            printf("resize widget(%d, %d, %d, %d)\n", newBounds.x,
                   newBounds.y, newBounds.width, newBounds.height);
#endif
            mContentViewer->SetBounds(newBounds);
        }
    }

    
    nsDocShell::FinishRestore();

    
    if (shell)
        shell->Thaw();

    return privWin->FireDelayedDOMEvents();
}

NS_IMETHODIMP
nsDocShell::CreateContentViewer(const char *aContentType,
                                nsIRequest * request,
                                nsIStreamListener ** aContentHandler)
{
    *aContentHandler = nsnull;

    
    

    NS_ASSERTION(mLoadGroup, "Someone ignored return from Init()?");

    
    nsCOMPtr<nsIContentViewer> viewer;
    nsresult rv = NewContentViewerObj(aContentType, request, mLoadGroup,
                                      aContentHandler, getter_AddRefs(viewer));

    if (NS_FAILED(rv))
        return NS_ERROR_FAILURE;

    
    
    
    
    
    

    if (mSavingOldViewer) {
        
        
        
        
        nsCOMPtr<nsIDOMDocument> domDoc;
        viewer->GetDOMDocument(getter_AddRefs(domDoc));
        nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
        mSavingOldViewer = CanSavePresentation(mLoadType, request, doc);
    }

    NS_ASSERTION(!mLoadingURI, "Re-entering unload?");
    
    nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(request);
    if (aOpenedChannel) {
        aOpenedChannel->GetURI(getter_AddRefs(mLoadingURI));
    }
    FirePageHideNotification(!mSavingOldViewer);
    mLoadingURI = nsnull;

    
    
    mFiredUnloadEvent = PR_FALSE;

    
    
    
    mURIResultedInDocument = PR_TRUE;

    PRBool onLocationChangeNeeded = OnLoadingSite(aOpenedChannel, PR_FALSE);

    
    nsCOMPtr<nsILoadGroup> currentLoadGroup;
    NS_ENSURE_SUCCESS(aOpenedChannel->
                      GetLoadGroup(getter_AddRefs(currentLoadGroup)),
                      NS_ERROR_FAILURE);

    if (currentLoadGroup != mLoadGroup) {
        nsLoadFlags loadFlags = 0;

        
        
        
        
        
        





        aOpenedChannel->SetLoadGroup(mLoadGroup);

        
        aOpenedChannel->GetLoadFlags(&loadFlags);
        loadFlags |= nsIChannel::LOAD_DOCUMENT_URI;

        aOpenedChannel->SetLoadFlags(loadFlags);

        mLoadGroup->AddRequest(request, nsnull);
        if (currentLoadGroup)
            currentLoadGroup->RemoveRequest(request, nsnull,
                                            NS_BINDING_RETARGETED);

        
        
        aOpenedChannel->SetNotificationCallbacks(this);
    }

    NS_ENSURE_SUCCESS(Embed(viewer, "", (nsISupports *) nsnull),
                      NS_ERROR_FAILURE);

    mSavedRefreshURIList = nsnull;
    mSavingOldViewer = PR_FALSE;
    mEODForCurrentDocument = PR_FALSE;

    
    
    nsCOMPtr<nsIMultiPartChannel> multiPartChannel(do_QueryInterface(request));
    if (multiPartChannel) {
      nsCOMPtr<nsIPresShell> shell;
      rv = GetPresShell(getter_AddRefs(shell));
      if (NS_SUCCEEDED(rv) && shell) {
        nsIDocument *doc = shell->GetDocument();
        if (doc) {
          PRUint32 partID;
          multiPartChannel->GetPartID(&partID);
          doc->SetPartID(partID);
        }
      }
    }

    
    
    
    if (++gNumberOfDocumentsLoading == 1) {
      
      
      
      FavorPerformanceHint(PR_TRUE, NS_EVENT_STARVATION_DELAY_HINT);
    }

    if (onLocationChangeNeeded) {
      FireOnLocationChange(this, request, mCurrentURI);
    }
  
    return NS_OK;
}

nsresult
nsDocShell::NewContentViewerObj(const char *aContentType,
                                nsIRequest * request, nsILoadGroup * aLoadGroup,
                                nsIStreamListener ** aContentHandler,
                                nsIContentViewer ** aViewer)
{
    nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(request);

    nsresult rv;
    nsCOMPtr<nsICategoryManager> catMan(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
    if (NS_FAILED(rv))
      return rv;
    
    nsXPIDLCString contractId;
    rv = catMan->GetCategoryEntry("Gecko-Content-Viewers", aContentType, getter_Copies(contractId));

    
    nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory;
    if (NS_SUCCEEDED(rv))
        docLoaderFactory = do_GetService(contractId.get());

    if (!docLoaderFactory) {
        return NS_ERROR_FAILURE;
    }

    
    
    NS_ENSURE_SUCCESS(docLoaderFactory->CreateInstance("view",
                                                       aOpenedChannel,
                                                       aLoadGroup, aContentType,
                                                       static_cast<nsIContentViewerContainer*>(this),
                                                       nsnull,
                                                       aContentHandler,
                                                       aViewer),
                      NS_ERROR_FAILURE);

    (*aViewer)->SetContainer(static_cast<nsIContentViewerContainer *>(this));
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetupNewViewer(nsIContentViewer * aNewViewer)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    PRInt32 x = 0;
    PRInt32 y = 0;
    PRInt32 cx = 0;
    PRInt32 cy = 0;

    
    
    DoGetPositionAndSize(&x, &y, &cx, &cy);

    nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
    NS_ENSURE_SUCCESS(GetSameTypeParent(getter_AddRefs(parentAsItem)),
                      NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocShell> parent(do_QueryInterface(parentAsItem));

    nsCAutoString defaultCharset;
    nsCAutoString forceCharset;
    nsCAutoString hintCharset;
    PRInt32 hintCharsetSource;
    nsCAutoString prevDocCharset;
    float textZoom;
    float pageZoom;
    PRBool styleDisabled;
    
    nsCOMPtr<nsIMarkupDocumentViewer> newMUDV;

    if (mContentViewer || parent) {
        nsCOMPtr<nsIMarkupDocumentViewer> oldMUDV;
        if (mContentViewer) {
            
            
            
            oldMUDV = do_QueryInterface(mContentViewer);

            
            

            if (mSavingOldViewer && NS_FAILED(CaptureState())) {
                if (mOSHE) {
                    mOSHE->SyncPresentationState();
                }
                mSavingOldViewer = PR_FALSE;
            }
        }
        else {
            
            nsCOMPtr<nsIContentViewer> parentContentViewer;
            parent->GetContentViewer(getter_AddRefs(parentContentViewer));
            oldMUDV = do_QueryInterface(parentContentViewer);
        }

        if (oldMUDV) {
            nsresult rv;

            newMUDV = do_QueryInterface(aNewViewer,&rv);
            if (newMUDV) {
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetDefaultCharacterSet(defaultCharset),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetForceCharacterSet(forceCharset),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetHintCharacterSet(hintCharset),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetHintCharacterSetSource(&hintCharsetSource),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetTextZoom(&textZoom),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetFullZoom(&pageZoom),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetAuthorStyleDisabled(&styleDisabled),
                                  NS_ERROR_FAILURE);
                NS_ENSURE_SUCCESS(oldMUDV->
                                  GetPrevDocCharacterSet(prevDocCharset),
                                  NS_ERROR_FAILURE);
            }
        }
    }

    nscolor bgcolor = NS_RGBA(0, 0, 0, 0);
    
    nsCOMPtr<nsIContentViewer> kungfuDeathGrip = mContentViewer;
    if (mContentViewer) {
        
        
        mContentViewer->Stop();

        
        
        nsCOMPtr<nsIDocumentViewer> docviewer =
        do_QueryInterface(mContentViewer);

        if (docviewer) {
            nsCOMPtr<nsIPresShell> shell;
            docviewer->GetPresShell(getter_AddRefs(shell));

            if (shell) {
                bgcolor = shell->GetCanvasBackground();
            }
        }

        mContentViewer->Close(mSavingOldViewer ? mOSHE.get() : nsnull);
        aNewViewer->SetPreviousViewer(mContentViewer);

        mContentViewer = nsnull;
    }

    mContentViewer = aNewViewer;

    nsCOMPtr<nsIWidget> widget;
    NS_ENSURE_SUCCESS(GetMainWidget(getter_AddRefs(widget)), NS_ERROR_FAILURE);

    nsIntRect bounds(x, y, cx, cy);

    if (NS_FAILED(mContentViewer->Init(widget, bounds))) {
        mContentViewer = nsnull;
        NS_ERROR("ContentViewer Initialization failed");
        return NS_ERROR_FAILURE;
    }

    
    
    if (newMUDV) {
        NS_ENSURE_SUCCESS(newMUDV->SetDefaultCharacterSet(defaultCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetForceCharacterSet(forceCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetHintCharacterSet(hintCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->
                          SetHintCharacterSetSource(hintCharsetSource),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetPrevDocCharacterSet(prevDocCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetTextZoom(textZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetFullZoom(pageZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetAuthorStyleDisabled(styleDisabled),
                          NS_ERROR_FAILURE);
    }

    
    
    nsCOMPtr<nsIDocumentViewer> docviewer =
        do_QueryInterface(mContentViewer);

    if (docviewer) {
        nsCOMPtr<nsIPresShell> shell;
        docviewer->GetPresShell(getter_AddRefs(shell));

        if (shell) {
            shell->SetCanvasBackground(bgcolor);
        }
    }




    
    
    

    
    DestroyChildren();

    return NS_OK;
}


nsresult
nsDocShell::CheckLoadingPermissions()
{
    
    
    
    
    
    
    
    nsresult rv = NS_OK, sameOrigin = NS_OK;

    if (!gValidateOrigin || !IsFrame()) {
        
        

        return rv;
    }

    
    
    

    nsCOMPtr<nsIScriptSecurityManager> securityManager =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool ubwEnabled = PR_FALSE;
    rv = securityManager->IsCapabilityEnabled("UniversalBrowserWrite",
                                              &ubwEnabled);
    if (NS_FAILED(rv) || ubwEnabled) {
        return rv;
    }

    nsCOMPtr<nsIPrincipal> subjPrincipal;
    rv = securityManager->GetSubjectPrincipal(getter_AddRefs(subjPrincipal));
    NS_ENSURE_TRUE(NS_SUCCEEDED(rv) && subjPrincipal, rv);

    
    
    nsCOMPtr<nsIDocShellTreeItem> item(this);
    do {
        nsCOMPtr<nsIScriptGlobalObject> sgo(do_GetInterface(item));
        nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(sgo));

        nsIPrincipal *p;
        if (!sop || !(p = sop->GetPrincipal())) {
            return NS_ERROR_UNEXPECTED;
        }

        
        PRBool equal;
        sameOrigin = subjPrincipal->Equals(p, &equal);
        if (NS_SUCCEEDED(sameOrigin)) {
            if (equal) {
                

                return sameOrigin;
            }

            sameOrigin = NS_ERROR_DOM_PROP_ACCESS_DENIED;
        }

        nsCOMPtr<nsIDocShellTreeItem> tmp;
        item->GetSameTypeParent(getter_AddRefs(tmp));
        item.swap(tmp);
    } while (item);

    return sameOrigin;
}




class InternalLoadEvent : public nsRunnable
{
public:
    InternalLoadEvent(nsDocShell* aDocShell, nsIURI * aURI, nsIURI * aReferrer,
                      nsISupports * aOwner, PRUint32 aFlags,
                      const char* aTypeHint, nsIInputStream * aPostData,
                      nsIInputStream * aHeadersData, PRUint32 aLoadType,
                      nsISHEntry * aSHEntry, PRBool aFirstParty) :
        mDocShell(aDocShell),
        mURI(aURI),
        mReferrer(aReferrer),
        mOwner(aOwner),
        mPostData(aPostData),
        mHeadersData(aHeadersData),
        mSHEntry(aSHEntry),
        mFlags(aFlags),
        mLoadType(aLoadType),
        mFirstParty(aFirstParty)
    {
        
        if (aTypeHint) {
            mTypeHint = aTypeHint;
        }
    }
    
    NS_IMETHOD Run() {
        return mDocShell->InternalLoad(mURI, mReferrer, mOwner, mFlags,
                                       nsnull, mTypeHint.get(),
                                       mPostData, mHeadersData, mLoadType,
                                       mSHEntry, mFirstParty, nsnull, nsnull);
    }

private:

    
    nsXPIDLString mWindowTarget;
    nsXPIDLCString mTypeHint;

    nsRefPtr<nsDocShell> mDocShell;
    nsCOMPtr<nsIURI> mURI;
    nsCOMPtr<nsIURI> mReferrer;
    nsCOMPtr<nsISupports> mOwner;
    nsCOMPtr<nsIInputStream> mPostData;
    nsCOMPtr<nsIInputStream> mHeadersData;
    nsCOMPtr<nsISHEntry> mSHEntry;
    PRUint32 mFlags;
    PRUint32 mLoadType;
    PRBool mFirstParty;
};

NS_IMETHODIMP
nsDocShell::InternalLoad(nsIURI * aURI,
                         nsIURI * aReferrer,
                         nsISupports * aOwner,
                         PRUint32 aFlags,
                         const PRUnichar *aWindowTarget,
                         const char* aTypeHint,
                         nsIInputStream * aPostData,
                         nsIInputStream * aHeadersData,
                         PRUint32 aLoadType,
                         nsISHEntry * aSHEntry,
                         PRBool aFirstParty,
                         nsIDocShell** aDocShell,
                         nsIRequest** aRequest)
{
    nsresult rv = NS_OK;

#ifdef PR_LOGGING
    if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
        nsCAutoString spec;
        if (aURI)
            aURI->GetSpec(spec);
        PR_LogPrint("DOCSHELL %p InternalLoad %s\n", this, spec.get());
    }
#endif
    
    
    if (aDocShell) {
        *aDocShell = nsnull;
    }
    if (aRequest) {
        *aRequest = nsnull;
    }

    if (!aURI) {
        return NS_ERROR_NULL_POINTER;
    }

    NS_ENSURE_TRUE(IsValidLoadType(aLoadType), NS_ERROR_INVALID_ARG);

    NS_ENSURE_TRUE(!mIsBeingDestroyed, NS_ERROR_NOT_AVAILABLE);

    
    
    if (aLoadType & LOAD_CMD_NORMAL) {
        PRBool isWyciwyg = PR_FALSE;
        rv = aURI->SchemeIs("wyciwyg", &isWyciwyg);   
        if ((isWyciwyg && NS_SUCCEEDED(rv)) || NS_FAILED(rv)) 
            return NS_ERROR_FAILURE;
    }

    PRBool bIsJavascript = PR_FALSE;
    if (NS_FAILED(aURI->SchemeIs("javascript", &bIsJavascript))) {
        bIsJavascript = PR_FALSE;
    }

    
    
    
    
    nsCOMPtr<nsIDOMElement> requestingElement;
    
    nsCOMPtr<nsPIDOMWindow> privateWin(do_QueryInterface(mScriptGlobal));
    if (privateWin)
        requestingElement = privateWin->GetFrameElementInternal();

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    PRUint32 contentType;
    if (IsFrame()) {
        NS_ASSERTION(requestingElement, "A frame but no DOM element!?");
        contentType = nsIContentPolicy::TYPE_SUBDOCUMENT;
    } else {
        contentType = nsIContentPolicy::TYPE_DOCUMENT;
    }

    nsISupports* context = requestingElement;
    if (!context) {
        context =  mScriptGlobal;
    }

    
    nsCOMPtr<nsIPrincipal> loadingPrincipal;
    if (aReferrer) {
        nsCOMPtr<nsIScriptSecurityManager> secMan =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = secMan->GetCodebasePrincipal(aReferrer,
                                          getter_AddRefs(loadingPrincipal));
    }
    
    rv = NS_CheckContentLoadPolicy(contentType,
                                   aURI,
                                   loadingPrincipal,
                                   context,
                                   EmptyCString(), 
                                   nsnull,         
                                   &shouldLoad);

    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
        if (NS_SUCCEEDED(rv) && shouldLoad == nsIContentPolicy::REJECT_TYPE) {
            return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
        }

        return NS_ERROR_CONTENT_BLOCKED;
    }

    nsCOMPtr<nsISupports> owner(aOwner);
    
    
    
    
    
    
    
    
    
    
    {
        PRBool inherits;
        
        if (aLoadType != LOAD_NORMAL_EXTERNAL && !owner &&
            (aFlags & INTERNAL_LOAD_FLAGS_INHERIT_OWNER) &&
            NS_SUCCEEDED(URIInheritsSecurityContext(aURI, &inherits)) &&
            inherits) {

            
            
            nsCOMPtr<nsIDocShellTreeItem> treeItem = this;
            do {
                nsCOMPtr<nsIDocShell> itemDocShell =
                    do_QueryInterface(treeItem);
                PRBool isUnsafe;
                if (itemDocShell &&
                    NS_SUCCEEDED(itemDocShell->GetChannelIsUnsafe(&isUnsafe)) &&
                    isUnsafe) {
                    return NS_ERROR_DOM_SECURITY_ERR;
                }

                nsCOMPtr<nsIDocShellTreeItem> parent;
                treeItem->GetSameTypeParent(getter_AddRefs(parent));
                parent.swap(treeItem);
            } while (treeItem);

            owner = GetInheritedPrincipal(PR_TRUE);
        }
    }

    
    
    
    
    
    if (aWindowTarget && *aWindowTarget) {
        
        
        
        aFlags = aFlags & ~INTERNAL_LOAD_FLAGS_INHERIT_OWNER;
        
        
        
        
        nsCOMPtr<nsIDocShellTreeItem> targetItem;
        FindItemWithName(aWindowTarget, nsnull, this,
                         getter_AddRefs(targetItem));

        nsCOMPtr<nsIDocShell> targetDocShell = do_QueryInterface(targetItem);
        
        PRBool isNewWindow = PR_FALSE;
        if (!targetDocShell) {
            nsCOMPtr<nsIDOMWindowInternal> win =
                do_GetInterface(GetAsSupports(this));
            NS_ENSURE_TRUE(win, NS_ERROR_NOT_AVAILABLE);

            nsDependentString name(aWindowTarget);
            nsCOMPtr<nsIDOMWindow> newWin;
            rv = win->Open(EmptyString(), 
                           name,          
                           EmptyString(), 
                           getter_AddRefs(newWin));

            
            
            
            nsCOMPtr<nsPIDOMWindow> piNewWin = do_QueryInterface(newWin);
            if (piNewWin) {
                nsCOMPtr<nsIDocument> newDoc =
                    do_QueryInterface(piNewWin->GetExtantDocument());
                if (!newDoc || newDoc->IsInitialDocument()) {
                    isNewWindow = PR_TRUE;
                    aFlags |= INTERNAL_LOAD_FLAGS_FIRST_LOAD;
                }
            }

            nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(newWin);
            targetDocShell = do_QueryInterface(webNav);
        }

        
        
        
        
        if (NS_SUCCEEDED(rv) && targetDocShell) {
            rv = targetDocShell->InternalLoad(aURI,
                                              aReferrer,
                                              owner,
                                              aFlags,
                                              nsnull,         
                                              aTypeHint,
                                              aPostData,
                                              aHeadersData,
                                              aLoadType,
                                              aSHEntry,
                                              aFirstParty,
                                              aDocShell,
                                              aRequest);
            if (rv == NS_ERROR_NO_CONTENT) {
                
                if (isNewWindow) {
                    
                    
                    
                    
                    
                    
                    
                    nsCOMPtr<nsIDOMWindowInternal> domWin =
                        do_GetInterface(targetDocShell);
                    if (domWin) {
                        domWin->Close();
                    }
                }
                
                
                
                
                
                
                rv = NS_OK;
            }
            else if (isNewWindow) {
                
                
                
            }
        }

        
        
        
        return rv;
    }

    
    
    
    
    if (mIsBeingDestroyed) {
        return NS_ERROR_FAILURE;
    }

    rv = CheckLoadingPermissions();
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    
    nsCOMPtr<nsIDocShellTreeItem> parent;
    GetParent(getter_AddRefs(parent));
    if (parent) {
      nsCOMPtr<nsIDOMDocument> domDoc = do_GetInterface(parent);
      nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
      if (doc) {
        doc->TryCancelFrameLoaderInitialization(this);
      }
    }

    if (mFiredUnloadEvent) {
        if (IsOKToLoadURI(aURI)) {
            NS_PRECONDITION(!aWindowTarget || !*aWindowTarget,
                            "Shouldn't have a window target here!");

            
            
            
            if (LOAD_TYPE_HAS_FLAGS(aLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {
                mLoadType = LOAD_NORMAL_REPLACE;
            }
            
            
            nsCOMPtr<nsIRunnable> ev =
                new InternalLoadEvent(this, aURI, aReferrer, aOwner, aFlags,
                                      aTypeHint, aPostData, aHeadersData,
                                      aLoadType, aSHEntry, aFirstParty);
            return NS_DispatchToCurrentThread(ev);
        }

        
        return NS_OK;
    }

    
    if (aLoadType == LOAD_NORMAL_EXTERNAL) {
        
        PRBool isChrome = PR_FALSE;
        if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome) {
            NS_WARNING("blocked external chrome: url -- use '-chrome' option");
            return NS_ERROR_FAILURE;
        }

        
        rv = CreateAboutBlankContentViewer(nsnull, nsnull);
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;

        
        
        aLoadType = LOAD_NORMAL;
    }

    mAllowKeywordFixup =
      (aFlags & INTERNAL_LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) != 0;
    mURIResultedInDocument = PR_FALSE;  
   
    
    
    
    
    
    
    
    
    PRBool allowScroll = PR_TRUE;
    if (!aSHEntry) {
        allowScroll = (aPostData == nsnull);
    } else if (mOSHE) {
        PRUint32 ourPageIdent;
        mOSHE->GetPageIdentifier(&ourPageIdent);
        PRUint32 otherPageIdent;
        aSHEntry->GetPageIdentifier(&otherPageIdent);
        allowScroll = (ourPageIdent == otherPageIdent);
#ifdef DEBUG
        if (allowScroll) {
            nsCOMPtr<nsIInputStream> currentPostData;
            mOSHE->GetPostData(getter_AddRefs(currentPostData));
            NS_ASSERTION(currentPostData == aPostData,
                         "Different POST data for entries for the same page?");
        }
#endif
    }
    
    if ((aLoadType == LOAD_NORMAL ||
         aLoadType == LOAD_STOP_CONTENT ||
         LOAD_TYPE_HAS_FLAGS(aLoadType, LOAD_FLAGS_REPLACE_HISTORY) ||
         aLoadType == LOAD_HISTORY ||
         aLoadType == LOAD_LINK) && allowScroll) {
        PRBool wasAnchor = PR_FALSE;
        PRBool doHashchange = PR_FALSE;
        nscoord cx, cy;
        NS_ENSURE_SUCCESS(ScrollIfAnchor(aURI, &wasAnchor, aLoadType, &cx, &cy,
                                         &doHashchange),
                          NS_ERROR_FAILURE);

        if (wasAnchor) {
            mLoadType = aLoadType;
            mURIResultedInDocument = PR_TRUE;

            




            SetHistoryEntry(&mLSHE, aSHEntry);

            



            nsCOMPtr<nsISupports> owner;
            if (mOSHE) {
                mOSHE->GetOwner(getter_AddRefs(owner));
            }
            OnNewURI(aURI, nsnull, owner, mLoadType, PR_TRUE);
            nsCOMPtr<nsIInputStream> postData;
            PRUint32 pageIdent = PR_UINT32_MAX;
            nsCOMPtr<nsISupports> cacheKey;
            
            if (mOSHE) {
                
                mOSHE->SetScrollPosition(cx, cy);
                
                
                
                
                
                
                if (aLoadType & LOAD_CMD_NORMAL) {
                    mOSHE->GetPostData(getter_AddRefs(postData));
                    mOSHE->GetPageIdentifier(&pageIdent);
                    mOSHE->GetCacheKey(getter_AddRefs(cacheKey));
                }
            }
            
            


            if (mLSHE) {
                SetHistoryEntry(&mOSHE, mLSHE);
                
                
                
                
                if (postData)
                    mOSHE->SetPostData(postData);

                
                
                if (cacheKey)
                    mOSHE->SetCacheKey(cacheKey);
                
                
                
                if (pageIdent != PR_UINT32_MAX)
                    mOSHE->SetPageIdentifier(pageIdent);
            }

            


            if (mOSHE && (aLoadType == LOAD_HISTORY || aLoadType == LOAD_RELOAD_NORMAL))
            {
                nscoord bx, by;
                mOSHE->GetScrollPosition(&bx, &by);
                SetCurScrollPosEx(bx, by);
            }

            


            SetHistoryEntry(&mLSHE, nsnull);
            


            if (mSessionHistory) {
                PRInt32 index = -1;
                mSessionHistory->GetIndex(&index);
                nsCOMPtr<nsIHistoryEntry> hEntry;
                mSessionHistory->GetEntryAtIndex(index, PR_FALSE,
                                                 getter_AddRefs(hEntry));
                NS_ENSURE_TRUE(hEntry, NS_ERROR_FAILURE);
                nsCOMPtr<nsISHEntry> shEntry(do_QueryInterface(hEntry));
                if (shEntry)
                    shEntry->SetTitle(mTitle);
            }

            if (doHashchange) {
                nsCOMPtr<nsPIDOMWindow> window =
                    do_QueryInterface(mScriptGlobal);

                if (window)
                    window->DispatchAsyncHashchange();
            }

            return NS_OK;
        }
    }
    
    
    
    
    
    nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

    
    
    if (!bIsJavascript && mContentViewer) {
        PRBool okToUnload;
        rv = mContentViewer->PermitUnload(&okToUnload);

        if (NS_SUCCEEDED(rv) && !okToUnload) {
            
            
            return NS_OK;
        }
    }

    
    
    
    
    
    
    PRBool savePresentation = CanSavePresentation(aLoadType, nsnull, nsnull);

    
    
    
    
    
    if (!bIsJavascript) {
        
        
        
        
        
        
        

        nsCOMPtr<nsIContentViewer> zombieViewer;
        if (mContentViewer) {
            mContentViewer->GetPreviousViewer(getter_AddRefs(zombieViewer));
        }

        if (zombieViewer ||
            LOAD_TYPE_HAS_FLAGS(aLoadType, LOAD_FLAGS_STOP_CONTENT)) {
            rv = Stop(nsIWebNavigation::STOP_ALL);
        } else {
            rv = Stop(nsIWebNavigation::STOP_NETWORK);
        }

        if (NS_FAILED(rv)) 
            return rv;
    }

    mLoadType = aLoadType;

    
    
    
    if (mLoadType != LOAD_ERROR_PAGE)
        SetHistoryEntry(&mLSHE, aSHEntry);

    mSavingOldViewer = savePresentation;

    
    if (aSHEntry && (mLoadType & LOAD_CMD_HISTORY)) {
        
        
        
        
        if (mContentViewer) {
            nsCOMPtr<nsIContentViewer> prevViewer;
            mContentViewer->GetPreviousViewer(getter_AddRefs(prevViewer));
            if (prevViewer) {
#ifdef DEBUG
                nsCOMPtr<nsIContentViewer> prevPrevViewer;
                prevViewer->GetPreviousViewer(getter_AddRefs(prevPrevViewer));
                NS_ASSERTION(!prevPrevViewer, "Should never have viewer chain here");
#endif
                nsCOMPtr<nsISHEntry> viewerEntry;
                prevViewer->GetHistoryEntry(getter_AddRefs(viewerEntry));
                if (viewerEntry == aSHEntry) {
                    
                    mContentViewer->SetPreviousViewer(nsnull);
                    prevViewer->Destroy();
                }
            }
        }
        nsCOMPtr<nsISHEntry> oldEntry = mOSHE;
        PRBool restoring;
        rv = RestorePresentation(aSHEntry, &restoring);
        if (restoring)
            return rv;

        
        
        
        if (NS_FAILED(rv)) {
            if (oldEntry)
                oldEntry->SyncPresentationState();

            aSHEntry->SyncPresentationState();
        }
    }

    nsCOMPtr<nsIRequest> req;
    rv = DoURILoad(aURI, aReferrer,
                   !(aFlags & INTERNAL_LOAD_FLAGS_DONT_SEND_REFERRER),
                   owner, aTypeHint, aPostData, aHeadersData, aFirstParty,
                   aDocShell, getter_AddRefs(req),
                   (aFlags & INTERNAL_LOAD_FLAGS_FIRST_LOAD) != 0,
                   (aFlags & INTERNAL_LOAD_FLAGS_BYPASS_CLASSIFIER) != 0,
                   (aFlags & INTERNAL_LOAD_FLAGS_FORCE_ALLOW_COOKIES) != 0);
    if (req && aRequest)
        NS_ADDREF(*aRequest = req);

    if (NS_FAILED(rv)) {
        nsCOMPtr<nsIChannel> chan(do_QueryInterface(req));
        DisplayLoadError(rv, aURI, nsnull, chan);
    }

    return rv;
}

nsIPrincipal*
nsDocShell::GetInheritedPrincipal(PRBool aConsiderCurrentDocument)
{
    nsCOMPtr<nsIDocument> document;

    if (aConsiderCurrentDocument && mContentViewer) {
        nsCOMPtr<nsIDocumentViewer>
            docViewer(do_QueryInterface(mContentViewer));
        if (!docViewer)
            return nsnull;
        docViewer->GetDocument(getter_AddRefs(document));
    }

    if (!document) {
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        GetSameTypeParent(getter_AddRefs(parentItem));
        if (parentItem) {
            nsCOMPtr<nsIDOMDocument> parentDomDoc(do_GetInterface(parentItem));
            document = do_QueryInterface(parentDomDoc);
        }
    }

    if (!document) {
        if (!aConsiderCurrentDocument) {
            return nsnull;
        }

        
        
        EnsureContentViewer();  
                                

        nsCOMPtr<nsIDocumentViewer>
            docViewer(do_QueryInterface(mContentViewer));
        if (!docViewer)
            return nsnull;
        docViewer->GetDocument(getter_AddRefs(document));
    }

    
    if (document) {
        return document->NodePrincipal();
    }

    return nsnull;
}

PRBool
nsDocShell::ShouldCheckAppCache(nsIURI *aURI)
{
    nsCOMPtr<nsIOfflineCacheUpdateService> offlineService =
        do_GetService(NS_OFFLINECACHEUPDATESERVICE_CONTRACTID);
    if (!offlineService) {
        return PR_FALSE;
    }

    PRBool allowed;
    nsresult rv = offlineService->OfflineAppAllowedForURI(aURI,
                                                          nsnull,
                                                          &allowed);
    return NS_SUCCEEDED(rv) && allowed;
}

nsresult
nsDocShell::DoURILoad(nsIURI * aURI,
                      nsIURI * aReferrerURI,
                      PRBool aSendReferrer,
                      nsISupports * aOwner,
                      const char * aTypeHint,
                      nsIInputStream * aPostData,
                      nsIInputStream * aHeadersData,
                      PRBool aFirstParty,
                      nsIDocShell ** aDocShell,
                      nsIRequest ** aRequest,
                      PRBool aIsNewWindowTarget,
                      PRBool aBypassClassifier,
                      PRBool aForceAllowCookies)
{
    nsresult rv;
    nsCOMPtr<nsIURILoader> uriLoader;

    uriLoader = do_GetService(NS_URI_LOADER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;

    nsLoadFlags loadFlags = nsIRequest::LOAD_NORMAL;
    if (aFirstParty) {
        
        loadFlags |= nsIChannel::LOAD_INITIAL_DOCUMENT_URI;
    }

    if (mLoadType == LOAD_ERROR_PAGE) {
        
        loadFlags |= nsIChannel::LOAD_BACKGROUND;
    }

    
    nsCOMPtr<nsIChannel> channel;

    rv = NS_NewChannel(getter_AddRefs(channel),
                       aURI,
                       nsnull,
                       nsnull,
                       static_cast<nsIInterfaceRequestor *>(this),
                       loadFlags);
    if (NS_FAILED(rv)) {
        if (rv == NS_ERROR_UNKNOWN_PROTOCOL) {
            
            
            
            
            PRBool abort = PR_FALSE;
            nsresult rv2 = mContentListener->OnStartURIOpen(aURI, &abort);
            if (NS_SUCCEEDED(rv2) && abort) {
                
                return NS_OK;
            }
        }
            
        return rv;
    }

    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(channel);
    if (appCacheChannel) {
        
        appCacheChannel->SetInheritApplicationCache(PR_FALSE);

        
        
        appCacheChannel->SetChooseApplicationCache(ShouldCheckAppCache(aURI));
    }

    
    
    if (aRequest)
        NS_ADDREF(*aRequest = channel);

    channel->SetOriginalURI(aURI);
    if (aTypeHint && *aTypeHint) {
        channel->SetContentType(nsDependentCString(aTypeHint));
        mContentTypeHint = aTypeHint;
    }
    else {
        mContentTypeHint.Truncate();
    }
    
    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal(do_QueryInterface(channel));
    if (httpChannelInternal) {
      if (aForceAllowCookies) {
        httpChannelInternal->SetForceAllowThirdPartyCookie(PR_TRUE);
      } 
      if (aFirstParty) {
        httpChannelInternal->SetDocumentURI(aURI);
      } else {
        httpChannelInternal->SetDocumentURI(aReferrerURI);
      }
    }

    nsCOMPtr<nsIWritablePropertyBag2> props(do_QueryInterface(channel));
    if (props)
    {
      
      
      props->SetPropertyAsInterface(NS_LITERAL_STRING("docshell.internalReferrer"),
                                    aReferrerURI);
    }

    
    
    
    
    if (httpChannel) {
        nsCOMPtr<nsICachingChannel>  cacheChannel(do_QueryInterface(httpChannel));
        
        nsCOMPtr<nsISupports> cacheKey;
        if (mLSHE) {
            mLSHE->GetCacheKey(getter_AddRefs(cacheKey));
        }
        else if (mOSHE)          
            mOSHE->GetCacheKey(getter_AddRefs(cacheKey));

        
        
        if (aPostData) {
            
            
            
            nsCOMPtr<nsISeekableStream>
                postDataSeekable(do_QueryInterface(aPostData));
            if (postDataSeekable) {
                rv = postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
                NS_ENSURE_SUCCESS(rv, rv);
            }

            nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
            NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

            
            uploadChannel->SetUploadStream(aPostData, EmptyCString(), -1);
            




            if (cacheChannel && cacheKey) {
                if (mLoadType == LOAD_HISTORY || mLoadType == LOAD_RELOAD_CHARSET_CHANGE) {
                    cacheChannel->SetCacheKey(cacheKey);
                    PRUint32 loadFlags;
                    if (NS_SUCCEEDED(channel->GetLoadFlags(&loadFlags)))
                        channel->SetLoadFlags(loadFlags | nsICachingChannel::LOAD_ONLY_FROM_CACHE);
                }
                else if (mLoadType == LOAD_RELOAD_NORMAL)
                    cacheChannel->SetCacheKey(cacheKey);
            }         
        }
        else {
            





            if (mLoadType == LOAD_HISTORY || mLoadType == LOAD_RELOAD_NORMAL 
                || mLoadType == LOAD_RELOAD_CHARSET_CHANGE) {
                if (cacheChannel && cacheKey)
                    cacheChannel->SetCacheKey(cacheKey);
            }
        }
        if (aHeadersData) {
            rv = AddHeadersToChannel(aHeadersData, httpChannel);
        }
        
        if (aReferrerURI && aSendReferrer) {
            
            httpChannel->SetReferrer(aReferrerURI);
        }
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    PRBool inherit;
    
    
    rv = URIInheritsSecurityContext(aURI, &inherit);
    if (NS_SUCCEEDED(rv) && (inherit || IsAboutBlank(aURI))) {
        channel->SetOwner(aOwner);
    }

    
    
    
    
    
    
    
    
    nsCOMPtr<nsIPrincipal> ownerPrincipal(do_QueryInterface(aOwner));
    if (URIIsLocalFile(aURI) && ownerPrincipal &&
        NS_SUCCEEDED(ownerPrincipal->CheckMayLoad(aURI, PR_FALSE))) {
        
        
        PRBool isSystem;
        nsCOMPtr<nsIScriptSecurityManager> secMan =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
        if (secMan &&
            NS_SUCCEEDED(secMan->IsSystemPrincipal(ownerPrincipal,
                                                   &isSystem)) &&
            !isSystem) {
            channel->SetOwner(aOwner);
        }
    }

    nsCOMPtr<nsIScriptChannel> scriptChannel = do_QueryInterface(channel);
    if (scriptChannel) {
        
        scriptChannel->
            SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
    }

    if (aIsNewWindowTarget) {
        nsCOMPtr<nsIWritablePropertyBag2> props = do_QueryInterface(channel);
        if (props) {
            props->SetPropertyAsBool(
                NS_LITERAL_STRING("docshell.newWindowTarget"),
                PR_TRUE);
        }
    }

    rv = DoChannelLoad(channel, uriLoader, aBypassClassifier);

    
    
    
    
    if (NS_SUCCEEDED(rv)) {
        if (aDocShell) {
          *aDocShell = this;
          NS_ADDREF(*aDocShell);
        }
    }

    return rv;
}

static NS_METHOD
AppendSegmentToString(nsIInputStream *in,
                      void *closure,
                      const char *fromRawSegment,
                      PRUint32 toOffset,
                      PRUint32 count,
                      PRUint32 *writeCount)
{
    

    nsCAutoString *buf = static_cast<nsCAutoString *>(closure);
    buf->Append(fromRawSegment, count);

    
    *writeCount = count;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::AddHeadersToChannel(nsIInputStream *aHeadersData,
                                nsIChannel *aGenericChannel)
{
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aGenericChannel);
    NS_ENSURE_STATE(httpChannel);

    PRUint32 numRead;
    nsCAutoString headersString;
    nsresult rv = aHeadersData->ReadSegments(AppendSegmentToString,
                                             &headersString,
                                             PR_UINT32_MAX,
                                             &numRead);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCAutoString headerName;
    nsCAutoString headerValue;
    PRInt32 crlf;
    PRInt32 colon;

    
    
    
    

    static const char kWhitespace[] = "\b\t\r\n ";
    while (PR_TRUE) {
        crlf = headersString.Find("\r\n");
        if (crlf == kNotFound)
            return NS_OK;

        const nsCSubstring &oneHeader = StringHead(headersString, crlf);

        colon = oneHeader.FindChar(':');
        if (colon == kNotFound)
            return NS_ERROR_UNEXPECTED;

        headerName = StringHead(oneHeader, colon);
        headerValue = Substring(oneHeader, colon + 1);

        headerName.Trim(kWhitespace);
        headerValue.Trim(kWhitespace);

        headersString.Cut(0, crlf + 2);

        
        
        

        rv = httpChannel->SetRequestHeader(headerName, headerValue, PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_NOTREACHED("oops");
    return NS_ERROR_UNEXPECTED;
}

nsresult nsDocShell::DoChannelLoad(nsIChannel * aChannel,
                                   nsIURILoader * aURILoader,
                                   PRBool aBypassClassifier)
{
    nsresult rv;
    
    nsLoadFlags loadFlags = 0;
    (void) aChannel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_DOCUMENT_URI |
                 nsIChannel::LOAD_CALL_CONTENT_SNIFFERS;

    
    switch (mLoadType) {
    case LOAD_HISTORY:
        loadFlags |= nsIRequest::VALIDATE_NEVER;
        break;

    case LOAD_RELOAD_CHARSET_CHANGE:
        loadFlags |= nsIRequest::LOAD_FROM_CACHE;
        break;
    
    case LOAD_RELOAD_NORMAL:
    case LOAD_REFRESH:
        loadFlags |= nsIRequest::VALIDATE_ALWAYS;
        break;

    case LOAD_NORMAL_BYPASS_CACHE:
    case LOAD_NORMAL_BYPASS_PROXY:
    case LOAD_NORMAL_BYPASS_PROXY_AND_CACHE:
    case LOAD_RELOAD_BYPASS_CACHE:
    case LOAD_RELOAD_BYPASS_PROXY:
    case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:
        loadFlags |= nsIRequest::LOAD_BYPASS_CACHE;
        break;

    case LOAD_NORMAL:
    case LOAD_LINK:
        
        PRInt32 prefSetting;
        if (NS_SUCCEEDED
            (mPrefs->
             GetIntPref("browser.cache.check_doc_frequency",
                        &prefSetting))) {
            switch (prefSetting) {
            case 0:
                loadFlags |= nsIRequest::VALIDATE_ONCE_PER_SESSION;
                break;
            case 1:
                loadFlags |= nsIRequest::VALIDATE_ALWAYS;
                break;
            case 2:
                loadFlags |= nsIRequest::VALIDATE_NEVER;
                break;
            }
        }
        break;
    }

    (void) aChannel->SetLoadFlags(loadFlags);

    rv = aURILoader->OpenURI(aChannel,
                             (mLoadType == LOAD_LINK),
                             this);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!aBypassClassifier) {
        rv = CheckClassifier(aChannel);
        if (NS_FAILED(rv)) {
            aChannel->Cancel(rv);
            return rv;
        }
    }

    return NS_OK;
}

nsresult
nsDocShell::CheckClassifier(nsIChannel *aChannel)
{
    nsRefPtr<nsClassifierCallback> classifier = new nsClassifierCallback();
    if (!classifier) return NS_ERROR_OUT_OF_MEMORY;

    nsresult rv = classifier->Start(aChannel, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    mClassifier = classifier;

    return NS_OK;
}

nsresult
nsDocShell::ScrollIfAnchor(nsIURI * aURI, PRBool * aWasAnchor,
                           PRUint32 aLoadType, nscoord *cx, nscoord *cy,
                           PRBool * aDoHashchange)
{
    NS_ASSERTION(aURI, "null uri arg");
    NS_ASSERTION(aWasAnchor, "null anchor arg");
    NS_PRECONDITION(aDoHashchange, "null hashchange arg");

    if (!aURI || !aWasAnchor) {
        return NS_ERROR_FAILURE;
    }

    *aWasAnchor = PR_FALSE;
    *aDoHashchange = PR_FALSE;

    if (!mCurrentURI) {
        return NS_OK;
    }

    nsCOMPtr<nsIPresShell> shell;
    nsresult rv = GetPresShell(getter_AddRefs(shell));
    if (NS_FAILED(rv) || !shell) {
        
        
        
        return rv;
    }

    

    nsCAutoString currentSpec;
    NS_ENSURE_SUCCESS(mCurrentURI->GetSpec(currentSpec),
                      NS_ERROR_FAILURE);

    nsCAutoString newSpec;
    NS_ENSURE_SUCCESS(aURI->GetSpec(newSpec), NS_ERROR_FAILURE);

    
    
    

    const char kHash = '#';

    
    
    nsACString::const_iterator urlStart, urlEnd, refStart, refEnd;
    newSpec.BeginReading(urlStart);
    newSpec.EndReading(refEnd);
    
    PRInt32 hashNew = newSpec.FindChar(kHash);
    if (hashNew == 0) {
        return NS_OK;           
    }

    if (hashNew > 0) {
        
        urlEnd = urlStart;
        urlEnd.advance(hashNew);
        
        refStart = urlEnd;
        ++refStart;             
        
    }
    else {
        
        urlEnd = refStart = refEnd;
    }
    const nsACString& sNewLeft = Substring(urlStart, urlEnd);
    const nsACString& sNewRef =  Substring(refStart, refEnd);
                                          
    
    nsACString::const_iterator currentLeftStart, currentLeftEnd,
                               currentRefStart, currentRefEnd;
    currentSpec.BeginReading(currentLeftStart);
    currentSpec.EndReading(currentRefEnd);

    PRInt32 hashCurrent = currentSpec.FindChar(kHash);
    if (hashCurrent == 0) {
        return NS_OK;           
    }

    if (hashCurrent > 0) {
        currentLeftEnd = currentLeftStart;
        currentLeftEnd.advance(hashCurrent);

        currentRefStart = currentLeftEnd;
        ++currentRefStart; 
    }
    else {
        
        currentLeftEnd = currentRefStart = currentRefEnd;
    }

    
    
    
    
    NS_ASSERTION(hashNew != 0 && hashCurrent != 0,
                 "What happened to the early returns above?");
    if (hashNew == kNotFound &&
        (hashCurrent == kNotFound || aLoadType != LOAD_HISTORY)) {
        return NS_OK;
    }

    
    
    
    
    
    
    
    

    if (!Substring(currentLeftStart, currentLeftEnd).Equals(sNewLeft)) {
        return NS_OK;           
    }

    
    *aWasAnchor = PR_TRUE;

    
    
    *aDoHashchange = !Substring(currentRefStart, currentRefEnd).Equals(sNewRef);

    
    
    
    
    

    GetCurScrollPos(ScrollOrientation_X, cx);
    GetCurScrollPos(ScrollOrientation_Y, cy);

    if (!sNewRef.IsEmpty()) {
        
        
        PRBool scroll = aLoadType != LOAD_HISTORY &&
                        aLoadType != LOAD_RELOAD_NORMAL;

        char *str = ToNewCString(sNewRef);
        if (!str) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        
        nsUnescape(str);

        
        
        

        
        
        
        
        
        rv = NS_ERROR_FAILURE;
        NS_ConvertUTF8toUTF16 uStr(str);
        if (!uStr.IsEmpty()) {
            rv = shell->GoToAnchor(NS_ConvertUTF8toUTF16(str), scroll);
        }
        nsMemory::Free(str);

        
        
        if (NS_FAILED(rv)) {
                
            
            NS_ENSURE_TRUE(mContentViewer, NS_ERROR_FAILURE);
            nsCOMPtr<nsIDocumentViewer>
                docv(do_QueryInterface(mContentViewer));
            NS_ENSURE_TRUE(docv, NS_ERROR_FAILURE);
            nsCOMPtr<nsIDocument> doc;
            rv = docv->GetDocument(getter_AddRefs(doc));
            NS_ENSURE_SUCCESS(rv, rv);
            const nsACString &aCharset = doc->GetDocumentCharacterSet();

            nsCOMPtr<nsITextToSubURI> textToSubURI =
                do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            
            nsXPIDLString uStr;

            rv = textToSubURI->UnEscapeAndConvert(PromiseFlatCString(aCharset).get(),
                                                  PromiseFlatCString(sNewRef).get(),
                                                  getter_Copies(uStr));
            NS_ENSURE_SUCCESS(rv, rv);

            
            
            
            
            
            shell->GoToAnchor(uStr, scroll);
        }
    }
    else {

        
        shell->GoToAnchor(EmptyString(), PR_FALSE);
        
        
        
        
        
        if (aLoadType == LOAD_HISTORY || aLoadType == LOAD_RELOAD_NORMAL)
            return rv;
        
        rv = SetCurScrollPosEx(0, 0);
    }

    return rv;
}

void
nsDocShell::SetupReferrerFromChannel(nsIChannel * aChannel)
{
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
    if (httpChannel) {
        nsCOMPtr<nsIURI> referrer;
        nsresult rv = httpChannel->GetReferrer(getter_AddRefs(referrer));
        if (NS_SUCCEEDED(rv)) {
            SetReferrerURI(referrer);
        }
    }
}

PRBool
nsDocShell::OnNewURI(nsIURI * aURI, nsIChannel * aChannel, nsISupports* aOwner,
                     PRUint32 aLoadType, PRBool aFireOnLocationChange,
                     PRBool aAddToGlobalHistory)
{
    NS_PRECONDITION(aURI, "uri is null");
    NS_PRECONDITION(!aChannel || !aOwner, "Shouldn't have both set");

#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsCAutoString spec;
        aURI->GetSpec(spec);

        nsCAutoString chanName;
        if (aChannel)
            aChannel->GetName(chanName);
        else
            chanName.AssignLiteral("<no channel>");

        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]::OnNewURI(\"%s\", [%s], 0x%x)\n", this, spec.get(),
                chanName.get(), aLoadType));
    }
#endif

    PRBool updateHistory = PR_TRUE;
    PRBool equalUri = PR_FALSE;
    PRBool shAvailable = PR_TRUE;  

    
    nsCOMPtr<nsIInputStream> inputStream;
    if (aChannel) {
        nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
        
        
        if (!httpChannel)  {
            GetHttpChannel(aChannel, getter_AddRefs(httpChannel));
        }

        if (httpChannel) {
            nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
            if (uploadChannel) {
                uploadChannel->GetUploadStream(getter_AddRefs(inputStream));
            }
        }
    }
    


    nsCOMPtr<nsISHistory> rootSH = mSessionHistory;
    if (!rootSH) {
        
        GetRootSessionHistory(getter_AddRefs(rootSH));
        if (!rootSH)
            shAvailable = PR_FALSE;
    }  


    
    if (aLoadType == LOAD_BYPASS_HISTORY ||
        aLoadType == LOAD_ERROR_PAGE ||
        aLoadType & LOAD_CMD_HISTORY ||
        aLoadType & LOAD_CMD_RELOAD)
        updateHistory = PR_FALSE;

    
    if (mCurrentURI)
        aURI->Equals(mCurrentURI, &equalUri);

#ifdef DEBUG
    PR_LOG(gDocShellLog, PR_LOG_DEBUG,
           ("  shAvailable=%i updateHistory=%i equalURI=%i\n",
            shAvailable, updateHistory, equalUri));
#endif

    











    if (equalUri &&
        (mLoadType == LOAD_NORMAL ||
         mLoadType == LOAD_LINK ||
         mLoadType == LOAD_STOP_CONTENT) &&
        !inputStream)
    {
        mLoadType = LOAD_NORMAL_REPLACE;
    }

    
    
    if (mLoadType == LOAD_REFRESH && !inputStream && equalUri) {
        SetHistoryEntry(&mLSHE, mOSHE);
    }


    



    if (aChannel &&
        (aLoadType == LOAD_RELOAD_BYPASS_CACHE ||
         aLoadType == LOAD_RELOAD_BYPASS_PROXY ||
         aLoadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE)) {
        NS_ASSERTION(!updateHistory,
                     "We shouldn't be updating history for forced reloads!");
        
        nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(aChannel));
        nsCOMPtr<nsISupports>  cacheKey;
        
        if (cacheChannel) 
            cacheChannel->GetCacheKey(getter_AddRefs(cacheKey));
        
        
        
        if (mLSHE)
            mLSHE->SetCacheKey(cacheKey);
        else if (mOSHE)
            mOSHE->SetCacheKey(cacheKey);
    }

    if (updateHistory && shAvailable) { 
        
        if (!mLSHE && (mItemType == typeContent) && mURIResultedInDocument) {
            



            (void) AddToSessionHistory(aURI, aChannel, aOwner,
                                       getter_AddRefs(mLSHE));
        }

        
        if (aAddToGlobalHistory) {
            
            AddToGlobalHistory(aURI, PR_FALSE, aChannel);
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
    PRBool onLocationChangeNeeded = SetCurrentURI(aURI, aChannel,
                                                  aFireOnLocationChange);
    
    SetupReferrerFromChannel(aChannel);
    return onLocationChangeNeeded;
}

PRBool
nsDocShell::OnLoadingSite(nsIChannel * aChannel, PRBool aFireOnLocationChange,
                          PRBool aAddToGlobalHistory)
{
    nsCOMPtr<nsIURI> uri;
    
    
    
    
    NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
    NS_ENSURE_TRUE(uri, PR_FALSE);

    return OnNewURI(uri, aChannel, nsnull, mLoadType, aFireOnLocationChange,
                    aAddToGlobalHistory);

}

void
nsDocShell::SetReferrerURI(nsIURI * aURI)
{
    mReferrerURI = aURI;        
}




PRBool
nsDocShell::ShouldAddToSessionHistory(nsIURI * aURI)
{
    
    
    
    
    nsresult rv;
    nsCAutoString buf;

    rv = aURI->GetScheme(buf);
    if (NS_FAILED(rv))
        return PR_FALSE;

    if (buf.Equals("about")) {
        rv = aURI->GetPath(buf);
        if (NS_FAILED(rv))
            return PR_FALSE;

        if (buf.Equals("blank")) {
            return PR_FALSE;
        }
    }
    return PR_TRUE;
}

nsresult
nsDocShell::AddToSessionHistory(nsIURI * aURI, nsIChannel * aChannel,
                                nsISupports* aOwner, nsISHEntry ** aNewEntry)
{
    NS_PRECONDITION(aURI, "uri is null");
    NS_PRECONDITION(!aChannel || !aOwner, "Shouldn't have both set");

#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsCAutoString spec;
        aURI->GetSpec(spec);

        nsCAutoString chanName;
        if (aChannel)
            aChannel->GetName(chanName);
        else
            chanName.AssignLiteral("<no channel>");

        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]::AddToSessionHistory(\"%s\", [%s])\n", this, spec.get(),
                chanName.get()));
    }
#endif

    nsresult rv = NS_OK;
    nsCOMPtr<nsISHEntry> entry;
    PRBool shouldPersist;

    shouldPersist = ShouldAddToSessionHistory(aURI);

    
    nsCOMPtr<nsIDocShellTreeItem> root;
    GetSameTypeRootTreeItem(getter_AddRefs(root));     
    




    if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY) &&
        root != static_cast<nsIDocShellTreeItem *>(this)) {
        
        entry = mOSHE;
        nsCOMPtr<nsISHContainer> shContainer(do_QueryInterface(entry));
        if (shContainer) {
            PRInt32 childCount = 0;
            shContainer->GetChildCount(&childCount);
            
            for (PRInt32 i = childCount - 1; i >= 0; i--) {
                nsCOMPtr<nsISHEntry> child;
                shContainer->GetChildAt(i, getter_AddRefs(child));
                shContainer->RemoveChild(child);
            }  
        }  
    }

    
    if (!entry) {
        entry = do_CreateInstance(NS_SHENTRY_CONTRACTID);

        if (!entry) {
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }

    
    nsCOMPtr<nsIInputStream> inputStream;
    nsCOMPtr<nsIURI> referrerURI;
    nsCOMPtr<nsISupports> cacheKey;
    nsCOMPtr<nsISupports> cacheToken;
    nsCOMPtr<nsISupports> owner = aOwner;
    PRBool expired = PR_FALSE;
    PRBool discardLayoutState = PR_FALSE;
    if (aChannel) {
        nsCOMPtr<nsICachingChannel>
            cacheChannel(do_QueryInterface(aChannel));
        


        if (cacheChannel) {
            cacheChannel->GetCacheKey(getter_AddRefs(cacheKey));
            cacheChannel->GetCacheToken(getter_AddRefs(cacheToken));
        }
        nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
        
        
        if (!httpChannel) {
            GetHttpChannel(aChannel, getter_AddRefs(httpChannel));
        }
        if (httpChannel) {
            nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
            if (uploadChannel) {
                uploadChannel->GetUploadStream(getter_AddRefs(inputStream));
            }
            httpChannel->GetReferrer(getter_AddRefs(referrerURI));

            discardLayoutState = ShouldDiscardLayoutState(httpChannel);
        }
        aChannel->GetOwner(getter_AddRefs(owner));
    }

    
    entry->Create(aURI,              
                  EmptyString(),     
                  inputStream,       
                  nsnull,            
                  cacheKey,          
                  mContentTypeHint,  
                  owner);            
    entry->SetReferrerURI(referrerURI);
    


    
    if (discardLayoutState) {
        entry->SetSaveLayoutStateFlag(PR_FALSE);
    }
    if (cacheToken) {
        
        nsCOMPtr<nsICacheEntryInfo> cacheEntryInfo(do_QueryInterface(cacheToken));
        if (cacheEntryInfo) {        
            PRUint32 expTime;         
            cacheEntryInfo->GetExpirationTime(&expTime);         
            PRUint32 now = PRTimeToSeconds(PR_Now());                  
            if (expTime <=  now)            
                expired = PR_TRUE;         
         
        }
    }
    if (expired == PR_TRUE)
        entry->SetExpirationStatus(PR_TRUE);


    if (root == static_cast<nsIDocShellTreeItem *>(this) && mSessionHistory) {
        
        if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {            
            
            PRInt32  index = 0;   
            mSessionHistory->GetIndex(&index);
            nsCOMPtr<nsISHistoryInternal>   shPrivate(do_QueryInterface(mSessionHistory));
            
            if (shPrivate)
                rv = shPrivate->ReplaceEntry(index, entry);          
        }
        else {
            
            nsCOMPtr<nsISHistoryInternal>
                shPrivate(do_QueryInterface(mSessionHistory));
            NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
            mSessionHistory->GetIndex(&mPreviousTransIndex);
            rv = shPrivate->AddEntry(entry, shouldPersist);
            mSessionHistory->GetIndex(&mLoadedTransIndex);
#ifdef DEBUG_PAGE_CACHE
            printf("Previous index: %d, Loaded index: %d\n\n",
                   mPreviousTransIndex, mLoadedTransIndex);
#endif
        }
    }
    else {  
        
        if (!mOSHE || !LOAD_TYPE_HAS_FLAGS(mLoadType,
                                           LOAD_FLAGS_REPLACE_HISTORY))
            rv = DoAddChildSHEntry(entry, mChildOffset);
    }

    
    if (aNewEntry) {
        *aNewEntry = nsnull;
        if (NS_SUCCEEDED(rv)) {
            *aNewEntry = entry;
            NS_ADDREF(*aNewEntry);
        }
    }

    return rv;
}


NS_IMETHODIMP
nsDocShell::LoadHistoryEntry(nsISHEntry * aEntry, PRUint32 aLoadType)
{
    if (!IsNavigationAllowed()) {
        return NS_OK;
    }
    
    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIInputStream> postData;
    nsCOMPtr<nsIURI> referrerURI;
    nsCAutoString contentType;
    nsCOMPtr<nsISupports> owner;

    NS_ENSURE_TRUE(aEntry, NS_ERROR_FAILURE);

    NS_ENSURE_SUCCESS(aEntry->GetURI(getter_AddRefs(uri)), NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(aEntry->GetReferrerURI(getter_AddRefs(referrerURI)),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(aEntry->GetPostData(getter_AddRefs(postData)),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(aEntry->GetContentType(contentType), NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(aEntry->GetOwner(getter_AddRefs(owner)),
                      NS_ERROR_FAILURE);

    
    
    
    
    nsCOMPtr<nsISHEntry> kungFuDeathGrip(aEntry);
    PRBool isJS;
    nsresult rv = uri->SchemeIs("javascript", &isJS);
    if (NS_FAILED(rv) || isJS) {
        
        
        
        
        nsCOMPtr<nsIPrincipal> prin = do_QueryInterface(owner);
        rv = CreateAboutBlankContentViewer(prin, nsnull);

        if (NS_FAILED(rv)) {
            
            
            
            return NS_OK;
        }

        if (!owner) {
            
            
            
            owner = do_CreateInstance("@mozilla.org/nullprincipal;1");
            NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);
        }
    }

    



    if ((aLoadType & LOAD_CMD_RELOAD) && postData) {
      PRBool repost;
      rv = ConfirmRepost(&repost);
      if (NS_FAILED(rv)) return rv;

      
      if (!repost)
        return NS_BINDING_ABORTED;
    }

    rv = InternalLoad(uri,
                      referrerURI,
                      owner,
                      INTERNAL_LOAD_FLAGS_NONE, 
                      nsnull,            
                      contentType.get(), 
                      postData,          
                      nsnull,            
                      aLoadType,         
                      aEntry,            
                      PR_TRUE,
                      nsnull,            
                      nsnull);           
    return rv;
}

NS_IMETHODIMP nsDocShell::GetShouldSaveLayoutState(PRBool* aShould)
{
    *aShould = PR_FALSE;
    if (mOSHE) {
        
        
        mOSHE->GetSaveLayoutStateFlag(aShould);
    }

    return NS_OK;
}

NS_IMETHODIMP nsDocShell::PersistLayoutHistoryState()
{
    nsresult  rv = NS_OK;
    
    if (mOSHE) {
        nsCOMPtr<nsIPresShell> shell;
        rv = GetPresShell(getter_AddRefs(shell));
        if (NS_SUCCEEDED(rv) && shell) {
            nsCOMPtr<nsILayoutHistoryState> layoutState;
            rv = shell->CaptureHistoryState(getter_AddRefs(layoutState),
                                            PR_TRUE);
        }
    }

    return rv;
}

 nsresult
nsDocShell::WalkHistoryEntries(nsISHEntry *aRootEntry,
                               nsDocShell *aRootShell,
                               WalkHistoryEntriesFunc aCallback,
                               void *aData)
{
    NS_ENSURE_TRUE(aRootEntry, NS_ERROR_FAILURE);

    nsCOMPtr<nsISHContainer> container(do_QueryInterface(aRootEntry));
    if (!container)
        return NS_ERROR_FAILURE;

    PRInt32 childCount;
    container->GetChildCount(&childCount);
    for (PRInt32 i = 0; i < childCount; i++) {
        nsCOMPtr<nsISHEntry> childEntry;
        container->GetChildAt(i, getter_AddRefs(childEntry));
        if (!childEntry) {
            
            
            continue;
        }

        nsDocShell *childShell = nsnull;
        if (aRootShell) {
            
            

            PRInt32 childCount = aRootShell->mChildList.Count();
            for (PRInt32 j = 0; j < childCount; ++j) {
                nsDocShell *child =
                    static_cast<nsDocShell*>(aRootShell->ChildAt(j));

                if (child->HasHistoryEntry(childEntry)) {
                    childShell = child;
                    break;
                }
            }
        }
        nsresult rv = aCallback(childEntry, childShell, i, aData);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}


struct NS_STACK_CLASS CloneAndReplaceData
{
    CloneAndReplaceData(PRUint32 aCloneID, nsISHEntry *aReplaceEntry,
                        nsISHEntry *aDestTreeParent)
        : cloneID(aCloneID),
          replaceEntry(aReplaceEntry),
          destTreeParent(aDestTreeParent) { }

    PRUint32              cloneID;
    nsISHEntry           *replaceEntry;
    nsISHEntry           *destTreeParent;
    nsCOMPtr<nsISHEntry>  resultEntry;
};

 nsresult
nsDocShell::CloneAndReplaceChild(nsISHEntry *aEntry, nsDocShell *aShell,
                                 PRInt32 aEntryIndex, void *aData)
{
    nsresult result = NS_OK;
    nsCOMPtr<nsISHEntry> dest;

    CloneAndReplaceData *data = static_cast<CloneAndReplaceData*>(aData);
    PRUint32 cloneID = data->cloneID;
    nsISHEntry *replaceEntry = data->replaceEntry;

    PRUint32 srcID;
    aEntry->GetID(&srcID);

    if (srcID == cloneID) {
        
        dest = replaceEntry;
        dest->SetIsSubFrame(PR_TRUE);
    } else {
        
        result = aEntry->Clone(getter_AddRefs(dest));
        if (NS_FAILED(result))
            return result;

        
        dest->SetIsSubFrame(PR_TRUE);

        
        CloneAndReplaceData childData(cloneID, replaceEntry, dest);
        result = WalkHistoryEntries(aEntry, aShell,
                                    CloneAndReplaceChild, &childData);
        if (NS_FAILED(result))
            return result;

        if (aShell)
            aShell->SwapHistoryEntries(aEntry, dest);
    }

    nsCOMPtr<nsISHContainer> container =
        do_QueryInterface(data->destTreeParent);
    if (container)
        container->AddChild(dest, aEntryIndex);

    data->resultEntry = dest;
    return result;
}

 nsresult
nsDocShell::CloneAndReplace(nsISHEntry *aSrcEntry,
                                   nsDocShell *aSrcShell,
                                   PRUint32 aCloneID,
                                   nsISHEntry *aReplaceEntry,
                                   nsISHEntry **aResultEntry)
{
    NS_ENSURE_ARG_POINTER(aResultEntry);
    NS_ENSURE_TRUE(aReplaceEntry, NS_ERROR_FAILURE);

    CloneAndReplaceData data(aCloneID, aReplaceEntry, nsnull);
    nsresult rv = CloneAndReplaceChild(aSrcEntry, aSrcShell, 0, &data);

    data.resultEntry.swap(*aResultEntry);
    return rv;
}


void
nsDocShell::SwapHistoryEntries(nsISHEntry *aOldEntry, nsISHEntry *aNewEntry)
{
    if (aOldEntry == mOSHE)
        mOSHE = aNewEntry;

    if (aOldEntry == mLSHE)
        mLSHE = aNewEntry;
}


struct SwapEntriesData
{
    nsDocShell *ignoreShell;     
    nsISHEntry *destTreeRoot;    
    nsISHEntry *destTreeParent;  
                                 
};


nsresult
nsDocShell::SetChildHistoryEntry(nsISHEntry *aEntry, nsDocShell *aShell,
                                 PRInt32 aEntryIndex, void *aData)
{
    SwapEntriesData *data = static_cast<SwapEntriesData*>(aData);
    nsDocShell *ignoreShell = data->ignoreShell;

    if (!aShell || aShell == ignoreShell)
        return NS_OK;

    nsISHEntry *destTreeRoot = data->destTreeRoot;

    nsCOMPtr<nsISHEntry> destEntry;
    nsCOMPtr<nsISHContainer> container =
        do_QueryInterface(data->destTreeParent);

    if (container) {
        
        
        
        

        PRUint32 targetID, id;
        aEntry->GetID(&targetID);

        
        nsCOMPtr<nsISHEntry> entry;
        container->GetChildAt(aEntryIndex, getter_AddRefs(entry));
        if (entry && NS_SUCCEEDED(entry->GetID(&id)) && id == targetID) {
            destEntry.swap(entry);
        } else {
            PRInt32 childCount;
            container->GetChildCount(&childCount);
            for (PRInt32 i = 0; i < childCount; ++i) {
                container->GetChildAt(i, getter_AddRefs(entry));
                if (!entry)
                    continue;

                entry->GetID(&id);
                if (id == targetID) {
                    destEntry.swap(entry);
                    break;
                }
            }
        }
    } else {
        destEntry = destTreeRoot;
    }

    aShell->SwapHistoryEntries(aEntry, destEntry);

    
    SwapEntriesData childData = { ignoreShell, destTreeRoot, destEntry };
    return WalkHistoryEntries(aEntry, aShell,
                              SetChildHistoryEntry, &childData);
}


static nsISHEntry*
GetRootSHEntry(nsISHEntry *aEntry)
{
    nsCOMPtr<nsISHEntry> rootEntry = aEntry;
    nsISHEntry *result = nsnull;
    while (rootEntry) {
        result = rootEntry;
        result->GetParent(getter_AddRefs(rootEntry));
    }

    return result;
}


void
nsDocShell::SetHistoryEntry(nsCOMPtr<nsISHEntry> *aPtr, nsISHEntry *aEntry)
{
    
    
    
    
    
    

    nsISHEntry *newRootEntry = GetRootSHEntry(aEntry);
    if (newRootEntry) {
        
        

        
        
        nsCOMPtr<nsISHEntry> oldRootEntry = GetRootSHEntry(*aPtr);
        if (oldRootEntry) {
            nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
            GetSameTypeParent(getter_AddRefs(parentAsItem));
            nsCOMPtr<nsIDocShell> rootShell = do_QueryInterface(parentAsItem);
            if (rootShell) { 
                SwapEntriesData data = { this, newRootEntry };
                nsIDocShell *rootIDocShell =
                    static_cast<nsIDocShell*>(rootShell);
                nsDocShell *rootDocShell = static_cast<nsDocShell*>
                                                      (rootIDocShell);

#ifdef NS_DEBUG
                nsresult rv =
#endif
                SetChildHistoryEntry(oldRootEntry, rootDocShell,
                                                   0, &data);
                NS_ASSERTION(NS_SUCCEEDED(rv), "SetChildHistoryEntry failed");
            }
        }
    }

    *aPtr = aEntry;
}


nsresult
nsDocShell::GetRootSessionHistory(nsISHistory ** aReturn)
{
    nsresult rv;

    nsCOMPtr<nsIDocShellTreeItem> root;
    
    rv = GetSameTypeRootTreeItem(getter_AddRefs(root));
    
    nsCOMPtr<nsIWebNavigation> rootAsWebnav(do_QueryInterface(root));
    if (rootAsWebnav) {
        
        rv = rootAsWebnav->GetSessionHistory(aReturn);
    }
    return rv;
}

nsresult
nsDocShell::GetHttpChannel(nsIChannel * aChannel, nsIHttpChannel ** aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);
    if (!aChannel)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIMultiPartChannel>  multiPartChannel(do_QueryInterface(aChannel));
    if (multiPartChannel) {
        nsCOMPtr<nsIChannel> baseChannel;
        multiPartChannel->GetBaseChannel(getter_AddRefs(baseChannel));
        nsCOMPtr<nsIHttpChannel>  httpChannel(do_QueryInterface(baseChannel));
        *aReturn = httpChannel;
        NS_IF_ADDREF(*aReturn);
    }
    return NS_OK;
}

PRBool 
nsDocShell::ShouldDiscardLayoutState(nsIHttpChannel * aChannel)
{    
    
    if (!aChannel)
        return PR_FALSE;

    
    nsCOMPtr<nsISupports> securityInfo;
    PRBool noStore = PR_FALSE, noCache = PR_FALSE;
    aChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
    aChannel->IsNoStoreResponse(&noStore);
    aChannel->IsNoCacheResponse(&noCache);

    return (noStore || (noCache && securityInfo));
}





NS_IMETHODIMP nsDocShell::GetEditor(nsIEditor * *aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  if (!mEditorData) {
    *aEditor = nsnull;
    return NS_OK;
  }

  return mEditorData->GetEditor(aEditor);
}

NS_IMETHODIMP nsDocShell::SetEditor(nsIEditor * aEditor)
{
  nsresult rv = EnsureEditorData();
  if (NS_FAILED(rv)) return rv;

  return mEditorData->SetEditor(aEditor);
}


NS_IMETHODIMP nsDocShell::GetEditable(PRBool *aEditable)
{
  NS_ENSURE_ARG_POINTER(aEditable);
  *aEditable = mEditorData && mEditorData->GetEditable();
  return NS_OK;
}


NS_IMETHODIMP nsDocShell::GetHasEditingSession(PRBool *aHasEditingSession)
{
  NS_ENSURE_ARG_POINTER(aHasEditingSession);
  
  if (mEditorData)
  {
    nsCOMPtr<nsIEditingSession> editingSession;
    mEditorData->GetEditingSession(getter_AddRefs(editingSession));
    *aHasEditingSession = (editingSession.get() != nsnull);
  }
  else
  {
    *aHasEditingSession = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocShell::MakeEditable(PRBool inWaitForUriLoad)
{
  nsresult rv = EnsureEditorData();
  if (NS_FAILED(rv)) return rv;

  return mEditorData->MakeEditable(inWaitForUriLoad);
}

nsresult
nsDocShell::AddToGlobalHistory(nsIURI * aURI, PRBool aRedirect,
                               nsIChannel * aChannel)
{
    if (mItemType != typeContent || !mGlobalHistory)
        return NS_OK;

    
    
    nsCOMPtr<nsIHttpChannel> hchan(do_QueryInterface(aChannel));
    if (hchan) {
        nsCAutoString type;
        nsresult rv = hchan->GetRequestMethod(type);
        if (NS_SUCCEEDED(rv) && type.EqualsLiteral("POST"))
            return NS_OK;
    }

    PRBool visited;
    nsresult rv = mGlobalHistory->IsVisited(aURI, &visited);
    if (NS_FAILED(rv))
        return rv;

    nsCOMPtr<nsIURI> referrer;
    if (aChannel)
        NS_GetReferrerFromChannel(aChannel, getter_AddRefs(referrer));

    rv = mGlobalHistory->AddURI(aURI, aRedirect, !IsFrame(), referrer);
    if (NS_FAILED(rv))
        return rv;

    if (!visited) {
        nsCOMPtr<nsIObserverService> obsService =
            do_GetService("@mozilla.org/observer-service;1");
        if (obsService) {
            obsService->NotifyObservers(aURI, NS_LINK_VISITED_EVENT_TOPIC, nsnull);
        }
    }

    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::SetLoadType(PRUint32 aLoadType)
{
    mLoadType = aLoadType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLoadType(PRUint32 * aLoadType)
{
    *aLoadType = mLoadType;
    return NS_OK;
}

nsresult
nsDocShell::ConfirmRepost(PRBool * aRepost)
{
  nsresult rv;
  nsCOMPtr<nsIPrompt> prompter;
  CallGetInterface(this, static_cast<nsIPrompt**>(getter_AddRefs(prompter)));

  nsCOMPtr<nsIStringBundleService> 
      stringBundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> appBundle;
  rv = stringBundleService->CreateBundle(kAppstringsBundleURL,
                                         getter_AddRefs(appBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> brandBundle;
  rv = stringBundleService->CreateBundle(kBrandBundleURL, getter_AddRefs(brandBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(prompter && brandBundle && appBundle,
               "Unable to set up repost prompter.");

  nsXPIDLString brandName;
  rv = brandBundle->GetStringFromName(NS_LITERAL_STRING("brandShortName").get(),
                                      getter_Copies(brandName));

  nsXPIDLString msgString, button0Title;
  if (NS_FAILED(rv)) { 
    rv = appBundle->GetStringFromName(NS_LITERAL_STRING("confirmRepostPrompt").get(),
                                      getter_Copies(msgString));
  }
  else {
    
    
    const PRUnichar *formatStrings[] = { brandName.get() };
    rv = appBundle->FormatStringFromName(NS_LITERAL_STRING("confirmRepostPrompt").get(),
                                         formatStrings, NS_ARRAY_LENGTH(formatStrings),
                                         getter_Copies(msgString));
  }
  if (NS_FAILED(rv)) return rv;

  rv = appBundle->GetStringFromName(NS_LITERAL_STRING("resendButton.label").get(),
                                    getter_Copies(button0Title));
  if (NS_FAILED(rv)) return rv;

  PRInt32 buttonPressed;
  PRBool checkState;
  rv = prompter->
         ConfirmEx(nsnull, msgString.get(),
                   (nsIPrompt::BUTTON_POS_0 * nsIPrompt::BUTTON_TITLE_IS_STRING) +
                   (nsIPrompt::BUTTON_POS_1 * nsIPrompt::BUTTON_TITLE_CANCEL),
                   button0Title.get(), nsnull, nsnull, nsnull, &checkState, &buttonPressed);
  if (NS_FAILED(rv)) return rv;

  *aRepost = (buttonPressed == 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPromptAndStringBundle(nsIPrompt ** aPrompt,
                                     nsIStringBundle ** aStringBundle)
{
    NS_ENSURE_SUCCESS(GetInterface(NS_GET_IID(nsIPrompt), (void **) aPrompt),
                      NS_ERROR_FAILURE);

    nsCOMPtr<nsIStringBundleService>
        stringBundleService(do_GetService(NS_STRINGBUNDLE_CONTRACTID));
    NS_ENSURE_TRUE(stringBundleService, NS_ERROR_FAILURE);

    NS_ENSURE_SUCCESS(stringBundleService->
                      CreateBundle(kAppstringsBundleURL,
                                   aStringBundle),
                      NS_ERROR_FAILURE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChildOffset(nsIDOMNode * aChild, nsIDOMNode * aParent,
                           PRInt32 * aOffset)
{
    NS_ENSURE_ARG_POINTER(aChild || aParent);

    nsCOMPtr<nsIDOMNodeList> childNodes;
    NS_ENSURE_SUCCESS(aParent->GetChildNodes(getter_AddRefs(childNodes)),
                      NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(childNodes, NS_ERROR_FAILURE);

    PRInt32 i = 0;

    for (; PR_TRUE; i++) {
        nsCOMPtr<nsIDOMNode> childNode;
        NS_ENSURE_SUCCESS(childNodes->Item(i, getter_AddRefs(childNode)),
                          NS_ERROR_FAILURE);
        NS_ENSURE_TRUE(childNode, NS_ERROR_FAILURE);

        if (childNode.get() == aChild) {
            *aOffset = i;
            return NS_OK;
        }
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::GetRootScrollableView(nsIScrollableView ** aOutScrollView)
{
    NS_ENSURE_ARG_POINTER(aOutScrollView);

    nsCOMPtr<nsIPresShell> shell;
    NS_ENSURE_SUCCESS(GetPresShell(getter_AddRefs(shell)), NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(shell, NS_ERROR_NULL_POINTER);

    NS_ENSURE_SUCCESS(shell->GetViewManager()->GetRootScrollableView(aOutScrollView),
                      NS_ERROR_FAILURE);

    if (*aOutScrollView == nsnull) {
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}

#ifdef DEBUG
class nsDebugAutoBoolTrueSetter
{
public:
    nsDebugAutoBoolTrueSetter(PRPackedBool *aBool)
        : mBool(aBool)
    {
        *mBool = PR_TRUE;
    }

    ~nsDebugAutoBoolTrueSetter()
    {
        *mBool = PR_FALSE;
    }
protected:
    PRPackedBool *mBool;
};
#endif

NS_IMETHODIMP
nsDocShell::EnsureScriptEnvironment()
{
    if (mScriptGlobal)
        return NS_OK;

    if (mIsBeingDestroyed) {
        return NS_ERROR_NOT_AVAILABLE;
    }

#ifdef DEBUG
    NS_ASSERTION(!mInEnsureScriptEnv,
                 "Infinite loop! Calling EnsureScriptEnvironment() from "
                 "within EnsureScriptEnvironment()!");

    
    
    nsDebugAutoBoolTrueSetter boolSetter(&mInEnsureScriptEnv);
#endif

    nsCOMPtr<nsIDOMScriptObjectFactory> factory =
        do_GetService(kDOMScriptObjectFactoryCID);
    NS_ENSURE_TRUE(factory, NS_ERROR_FAILURE);

    nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(mTreeOwner));

    PRUint32 chromeFlags;
    if (browserChrome) {
        browserChrome->GetChromeFlags(&chromeFlags);
    } else {
        chromeFlags = 0;
    }

    PRBool isModalContentWindow =
        (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL) &&
        !(chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME);

    
    
    factory->NewScriptGlobalObject(mItemType == typeChrome,
                                   isModalContentWindow,
                                   getter_AddRefs(mScriptGlobal));
    NS_ENSURE_TRUE(mScriptGlobal, NS_ERROR_FAILURE);

    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
    win->SetDocShell(static_cast<nsIDocShell *>(this));

    
    
    
    nsresult rv;
    rv = mScriptGlobal->EnsureScriptEnvironment(nsIProgrammingLanguage::JAVASCRIPT);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::EnsureEditorData()
{
    PRBool openDocHasDetachedEditor = mOSHE && mOSHE->HasDetachedEditor();
    if (!mEditorData && !mIsBeingDestroyed && !openDocHasDetachedEditor) {
        
        
        
        
        mEditorData = new nsDocShellEditorData(this);
    }

    return mEditorData ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult
nsDocShell::EnsureTransferableHookData()
{
    if (!mTransferableHookData) {
        mTransferableHookData = new nsTransferableHookData();
        if (!mTransferableHookData) return NS_ERROR_OUT_OF_MEMORY;
    }

    return NS_OK;
}


NS_IMETHODIMP nsDocShell::EnsureFind()
{
    nsresult rv;
    if (!mFind)
    {
        mFind = do_CreateInstance("@mozilla.org/embedcomp/find;1", &rv);
        if (NS_FAILED(rv)) return rv;
    }
    
    
    
    

    nsIScriptGlobalObject* scriptGO = GetScriptGlobalObject();
    NS_ENSURE_TRUE(scriptGO, NS_ERROR_UNEXPECTED);

    
    nsCOMPtr<nsIDOMWindow> windowToSearch(do_QueryInterface(mScriptGlobal));

    nsCOMPtr<nsIDocShellTreeItem> root;
    GetRootTreeItem(getter_AddRefs(root));

    
    
    nsCOMPtr<nsIDOMWindow> rootWindow = do_GetInterface(root);
    nsCOMPtr<nsIFocusManager> fm = do_GetService(FOCUSMANAGER_CONTRACTID);
    if (fm) {
      nsCOMPtr<nsIDOMWindow> activeWindow;
      fm->GetActiveWindow(getter_AddRefs(activeWindow));
      if (activeWindow == rootWindow)
        fm->GetFocusedWindow(getter_AddRefs(windowToSearch));
    }

    nsCOMPtr<nsIWebBrowserFindInFrames> findInFrames = do_QueryInterface(mFind);
    if (!findInFrames) return NS_ERROR_NO_INTERFACE;
    
    rv = findInFrames->SetRootSearchFrame(rootWindow);
    if (NS_FAILED(rv)) return rv;
    rv = findInFrames->SetCurrentSearchFrame(windowToSearch);
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}

PRBool
nsDocShell::IsFrame()
{
    nsCOMPtr<nsIDocShellTreeItem> parent =
        do_QueryInterface(GetAsSupports(mParent));
    if (parent) {
        PRInt32 parentType = ~mItemType;        
        parent->GetItemType(&parentType);
        if (parentType == mItemType)    
            return PR_TRUE;
    }

    return PR_FALSE;
}


NS_IMETHODIMP 
nsDocShell::IsBeingDestroyed(PRBool *aDoomed)
{
  NS_ENSURE_ARG(aDoomed);
  *aDoomed = mIsBeingDestroyed;
  return NS_OK;
}


NS_IMETHODIMP 
nsDocShell::GetIsExecutingOnLoadHandler(PRBool *aResult)
{
  NS_ENSURE_ARG(aResult);
  *aResult = mIsExecutingOnLoadHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLayoutHistoryState(nsILayoutHistoryState **aLayoutHistoryState)
{
  if (mOSHE)
    mOSHE->GetLayoutHistoryState(aLayoutHistoryState);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetLayoutHistoryState(nsILayoutHistoryState *aLayoutHistoryState)
{
  if (mOSHE)
    mOSHE->SetLayoutHistoryState(aLayoutHistoryState);
  return NS_OK;
}





nsRefreshTimer::nsRefreshTimer()
    : mDelay(0), mRepeat(PR_FALSE), mMetaRefresh(PR_FALSE)
{
}

nsRefreshTimer::~nsRefreshTimer()
{
}





NS_IMPL_THREADSAFE_ADDREF(nsRefreshTimer)
NS_IMPL_THREADSAFE_RELEASE(nsRefreshTimer)

NS_INTERFACE_MAP_BEGIN(nsRefreshTimer)
    NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITimerCallback)
    NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_THREADSAFE




NS_IMETHODIMP
nsRefreshTimer::Notify(nsITimer * aTimer)
{
    NS_ASSERTION(mDocShell, "DocShell is somehow null");

    if (mDocShell && aTimer) {
        
        PRUint32 delay = 0;
        aTimer->GetDelay(&delay);
        mDocShell->ForceRefreshURIFromTimer(mURI, delay, mMetaRefresh, aTimer);
    }
    return NS_OK;
}




nsDocShell::InterfaceRequestorProxy::InterfaceRequestorProxy(nsIInterfaceRequestor* p)
{
    if (p) {
        mWeakPtr = do_GetWeakReference(p);
    }
}
 
nsDocShell::InterfaceRequestorProxy::~InterfaceRequestorProxy()
{
    mWeakPtr = nsnull;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(nsDocShell::InterfaceRequestorProxy, nsIInterfaceRequestor) 
  
NS_IMETHODIMP 
nsDocShell::InterfaceRequestorProxy::GetInterface(const nsIID & aIID, void **aSink)
{
    NS_ENSURE_ARG_POINTER(aSink);
    nsCOMPtr<nsIInterfaceRequestor> ifReq = do_QueryReferent(mWeakPtr);
    if (ifReq) {
        return ifReq->GetInterface(aIID, aSink);
    }
    *aSink = nsnull;
    return NS_NOINTERFACE;
}

nsresult
nsDocShell::SetBaseUrlForWyciwyg(nsIContentViewer * aContentViewer)
{
    if (!aContentViewer)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIURI> baseURI;
    nsCOMPtr<nsIDocument> document;
    nsresult rv = NS_ERROR_NOT_AVAILABLE;

    if (sURIFixup)
        rv = sURIFixup->CreateExposableURI(mCurrentURI,
                                           getter_AddRefs(baseURI));

    
    if (baseURI) {
        nsCOMPtr<nsIDocumentViewer> docViewer(do_QueryInterface(aContentViewer));
        if (docViewer) {
            rv = docViewer->GetDocument(getter_AddRefs(document));
            if (document)
                rv = document->SetBaseURI(baseURI);
        }
    }
    return rv;
}





NS_IMETHODIMP
nsDocShell::GetAuthPrompt(PRUint32 aPromptReason, const nsIID& iid,
                          void** aResult)
{
    
    PRBool priorityPrompt = (aPromptReason == PROMPT_PROXY);

    if (!mAllowAuth && !priorityPrompt)
        return NS_ERROR_NOT_AVAILABLE;

    
    nsresult rv;
    nsCOMPtr<nsIPromptFactory> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = EnsureScriptEnvironment();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMWindow> window(do_QueryInterface(mScriptGlobal));

    
    

    return wwatch->GetPrompt(window, iid,
                             reinterpret_cast<void**>(aResult));
}





NS_IMETHODIMP
nsDocShell::Observe(nsISupports *aSubject, const char *aTopic,
                    const PRUnichar *aData)
{
    nsresult rv = NS_OK;
    if (mObserveErrorPages &&
        !nsCRT::strcmp(aTopic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) &&
        !nsCRT::strcmp(aData,
          NS_LITERAL_STRING("browser.xul.error_pages.enabled").get())) {

        nsCOMPtr<nsIPrefBranch> prefs(do_QueryInterface(aSubject, &rv));
        NS_ENSURE_SUCCESS(rv, rv);

        PRBool tmpbool;
        rv = prefs->GetBoolPref("browser.xul.error_pages.enabled", &tmpbool);
        if (NS_SUCCEEDED(rv))
            mUseErrorPages = tmpbool;

    } else {
        rv = NS_ERROR_UNEXPECTED;
    }
    return rv;
}




NS_IMETHODIMP
nsDocShell::GetAssociatedWindow(nsIDOMWindow** aWindow)
{
    return CallGetInterface(this, aWindow);
}

NS_IMETHODIMP
nsDocShell::GetTopWindow(nsIDOMWindow** aWindow)
{
    nsresult rv;
    nsCOMPtr<nsIDOMWindow> win = do_GetInterface(GetAsSupports(this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    return win->GetTop(aWindow);
}

NS_IMETHODIMP
nsDocShell::IsAppOfType(PRUint32 aAppType, PRBool *aIsOfType)
{
    nsCOMPtr<nsIDocShell> shell = this;
    while (shell) {
        PRUint32 type;
        shell->GetAppType(&type);
        if (type == aAppType) {
            *aIsOfType = PR_TRUE;
            return NS_OK;
        }
        nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(shell);
        nsCOMPtr<nsIDocShellTreeItem> parent;
        item->GetParent(getter_AddRefs(parent));
        shell = do_QueryInterface(parent);
    }

    *aIsOfType = PR_FALSE;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsContent(PRBool *aIsContent)
{
    *aIsContent = (mItemType == typeContent);
    return NS_OK;
}


nsresult
nsDocShell::URIInheritsSecurityContext(nsIURI* aURI, PRBool* aResult)
{
    
    
    return NS_URIChainHasFlags(aURI,
                               nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT,
                               aResult);
}


PRBool
nsDocShell::URIIsLocalFile(nsIURI *aURI)
{
    PRBool isFile;
    nsCOMPtr<nsINetUtil> util = do_GetIOService();

    return util && NS_SUCCEEDED(util->ProtocolHasFlags(aURI,
                                    nsIProtocolHandler::URI_IS_LOCAL_FILE,
                                    &isFile)) &&
           isFile;
}


PRBool
nsDocShell::IsAboutBlank(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "Must have URI");
    
    
    PRBool isAbout = PR_FALSE;
    if (NS_FAILED(aURI->SchemeIs("about", &isAbout)) || !isAbout) {
        return PR_FALSE;
    }
    
    nsCAutoString str;
    aURI->GetSpec(str);
    return str.EqualsLiteral("about:blank");
}

PRBool
nsDocShell::IsOKToLoadURI(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "Must have a URI!");
    
    if (!mFiredUnloadEvent) {
        return PR_TRUE;
    }

    if (!mLoadingURI) {
        return PR_FALSE;
    }

    nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    return
        secMan &&
        NS_SUCCEEDED(secMan->CheckSameOriginURI(aURI, mLoadingURI, PR_FALSE));
}





NS_IMPL_ISUPPORTS5(nsClassifierCallback,
                   nsIChannelClassifier,
                   nsIURIClassifierCallback,
                   nsIRunnable,
                   nsIChannelEventSink,
                   nsIInterfaceRequestor)

NS_IMETHODIMP
nsClassifierCallback::Run()
{
    if (!mChannel) {
        return NS_OK;
    }

    NS_ASSERTION(!mSuspendedChannel,
                 "nsClassifierCallback::Run() called while a "
                 "channel is still suspended.");

    nsCOMPtr<nsIChannel> channel;
    channel.swap(mChannel);

    
    
    PRUint32 status;
    channel->GetStatus(&status);
    if (NS_FAILED(status))
        return NS_OK;

    
    
    if (HasBeenClassified(channel)) {
        return NS_OK;
    }

    nsCOMPtr<nsIURI> uri;
    nsresult rv = channel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);

    
    PRBool hasFlags;
    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_DANGEROUS_TO_LOAD,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_FILE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_UI_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    rv = NS_URIChainHasFlags(uri,
                             nsIProtocolHandler::URI_IS_LOCAL_RESOURCE,
                             &hasFlags);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasFlags) return NS_OK;

    nsCOMPtr<nsIURIClassifier> uriClassifier =
        do_GetService(NS_URICLASSIFIERSERVICE_CONTRACTID, &rv);
    if (rv == NS_ERROR_FACTORY_NOT_REGISTERED ||
        rv == NS_ERROR_NOT_AVAILABLE) {
        
        return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool expectCallback;
    rv = uriClassifier->Classify(uri, this, &expectCallback);
    if (NS_FAILED(rv)) return rv;

    if (expectCallback) {
        
        
        rv = channel->Suspend();
        if (NS_FAILED(rv)) {
            
            
            
            return NS_OK;
        }

        mSuspendedChannel = channel;
#ifdef DEBUG
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsClassifierCallback[%p]: suspended channel %p",
                this, mSuspendedChannel.get()));
#endif
    }

    return NS_OK;
}



void
nsClassifierCallback::MarkEntryClassified(nsresult status)
{
    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(mSuspendedChannel);
    if (!cachingChannel) {
        return;
    }

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken) {
        return;
    }

    nsCOMPtr<nsICacheEntryDescriptor> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return;
    }

    cacheEntry->SetMetaDataElement("docshell:classified",
                                   NS_SUCCEEDED(status) ? "1" : nsnull);
}

PRBool
nsClassifierCallback::HasBeenClassified(nsIChannel *aChannel)
{
    nsCOMPtr<nsICachingChannel> cachingChannel =
        do_QueryInterface(aChannel);
    if (!cachingChannel) {
        return PR_FALSE;
    }

    
    
    PRBool fromCache;
    if (NS_FAILED(cachingChannel->IsFromCache(&fromCache)) || !fromCache) {
        return PR_FALSE;
    }

    nsCOMPtr<nsISupports> cacheToken;
    cachingChannel->GetCacheToken(getter_AddRefs(cacheToken));
    if (!cacheToken) {
        return PR_FALSE;
    }

    nsCOMPtr<nsICacheEntryDescriptor> cacheEntry =
        do_QueryInterface(cacheToken);
    if (!cacheEntry) {
        return PR_FALSE;
    }

    nsXPIDLCString tag;
    cacheEntry->GetMetaDataElement("docshell:classified", getter_Copies(tag));
    return tag.EqualsLiteral("1");
}

NS_IMETHODIMP
nsClassifierCallback::OnClassifyComplete(nsresult aErrorCode)
{
    if (mSuspendedChannel) {
        MarkEntryClassified(aErrorCode);

        if (NS_FAILED(aErrorCode)) {
#ifdef DEBUG
            PR_LOG(gDocShellLog, PR_LOG_DEBUG,
                   ("nsClassifierCallback[%p]: cancelling channel %p with error code: %d",
                    this, mSuspendedChannel.get(), aErrorCode));
#endif
            mSuspendedChannel->Cancel(aErrorCode);
        }
#ifdef DEBUG
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsClassifierCallback[%p]: resuming channel %p from OnClassifyComplete",
                this, mSuspendedChannel.get()));
#endif
        mSuspendedChannel->Resume();
        mSuspendedChannel = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClassifierCallback::Start(nsIChannel *aChannel, PRBool aInstallListener)
{
    mChannel = aChannel;

    if (aInstallListener) {
        nsresult rv = aChannel->GetNotificationCallbacks
            (getter_AddRefs(mNotificationCallbacks));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aChannel->SetNotificationCallbacks
            (static_cast<nsIInterfaceRequestor*>(this));
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return Run();
}

NS_IMETHODIMP
nsClassifierCallback::OnRedirect(nsIChannel *aOldChannel,
                                 nsIChannel *aNewChannel)
{
    mChannel = aNewChannel;

    
    
    NS_DispatchToCurrentThread(this);

    return NS_OK;
}

NS_IMETHODIMP
nsClassifierCallback::Cancel()
{
    if (mSuspendedChannel) {
#ifdef DEBUG
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsClassifierCallback[%p]: resuming channel %p from Cancel()",
                this, mSuspendedChannel.get()));
#endif
        mSuspendedChannel->Resume();
        mSuspendedChannel = nsnull;
    }

    if (mChannel) {
        mChannel = nsnull;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClassifierCallback::OnChannelRedirect(nsIChannel *aOldChannel,
                                        nsIChannel *aNewChannel,
                                        PRUint32 aFlags)
{
    nsresult rv = OnRedirect(aOldChannel, aNewChannel);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mNotificationCallbacks) {
        nsCOMPtr<nsIChannelEventSink> sink =
            do_GetInterface(mNotificationCallbacks);
        if (sink) {
            return sink->OnChannelRedirect(aOldChannel, aNewChannel, aFlags);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsClassifierCallback::GetInterface(const nsIID &aIID, void **aResult)
{
    if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
        NS_ADDREF_THIS();
        *aResult = static_cast<nsIChannelEventSink *>(this);
        return NS_OK;
    } else if (mNotificationCallbacks) {
        return mNotificationCallbacks->GetInterface(aIID, aResult);
    } else {
        return NS_ERROR_NO_INTERFACE;
    }
}




nsresult
nsDocShell::GetControllerForCommand(const char * inCommand,
                                    nsIController** outController)
{
  NS_ENSURE_ARG_POINTER(outController);
  *outController = nsnull;
  
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsPIDOMWindow> window ( do_QueryInterface(mScriptGlobal) );
  if (window) {
    nsIFocusController *focusController = window->GetRootFocusController();
    if (focusController)
      rv = focusController->GetControllerForCommand (inCommand, outController);
  } 

  return rv;
  
}

nsresult
nsDocShell::IsCommandEnabled(const char * inCommand, PRBool* outEnabled)
{
  NS_ENSURE_ARG_POINTER(outEnabled);
  *outEnabled = PR_FALSE;

  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand (inCommand, getter_AddRefs(controller));
  if (controller)
    rv = controller->IsCommandEnabled(inCommand, outEnabled);
  
  return rv;
}

nsresult
nsDocShell::DoCommand(const char * inCommand)
{
  nsresult rv = NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand(inCommand, getter_AddRefs(controller));
  if (controller)
    rv = controller->DoCommand(inCommand);
  
  return rv;
}

nsresult
nsDocShell::EnsureCommandHandler()
{
  if (!mCommandManager)
  {
    nsCOMPtr<nsPICommandUpdater> commandUpdater =
      do_CreateInstance("@mozilla.org/embedcomp/command-manager;1");
    if (!commandUpdater) return NS_ERROR_OUT_OF_MEMORY;
    
    nsCOMPtr<nsIDOMWindow> domWindow =
      do_GetInterface(static_cast<nsIInterfaceRequestor *>(this));

    nsresult rv = commandUpdater->Init(domWindow);
    if (NS_SUCCEEDED(rv))
      mCommandManager = do_QueryInterface(commandUpdater);
  }
  
  return mCommandManager ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::CanCutSelection(PRBool* aResult)
{
  return IsCommandEnabled("cmd_cut", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopySelection(PRBool* aResult)
{
  return IsCommandEnabled("cmd_copy", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyLinkLocation(PRBool* aResult)
{
  return IsCommandEnabled("cmd_copyLink", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyImageLocation(PRBool* aResult)
{
  return IsCommandEnabled("cmd_copyImageLocation",
                          aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyImageContents(PRBool* aResult)
{
  return IsCommandEnabled("cmd_copyImageContents",
                          aResult);
}

NS_IMETHODIMP
nsDocShell::CanPaste(PRBool* aResult)
{
  return IsCommandEnabled("cmd_paste", aResult);
}

NS_IMETHODIMP
nsDocShell::CutSelection(void)
{
  return DoCommand ( "cmd_cut" );
}

NS_IMETHODIMP
nsDocShell::CopySelection(void)
{
  return DoCommand ( "cmd_copy" );
}

NS_IMETHODIMP
nsDocShell::CopyLinkLocation(void)
{
  return DoCommand ( "cmd_copyLink" );
}

NS_IMETHODIMP
nsDocShell::CopyImageLocation(void)
{
  return DoCommand ( "cmd_copyImageLocation" );
}

NS_IMETHODIMP
nsDocShell::CopyImageContents(void)
{
  return DoCommand ( "cmd_copyImageContents" );
}

NS_IMETHODIMP
nsDocShell::Paste(void)
{
  return DoCommand ( "cmd_paste" );
}

NS_IMETHODIMP
nsDocShell::SelectAll(void)
{
  return DoCommand ( "cmd_selectAll" );
}







NS_IMETHODIMP
nsDocShell::SelectNone(void)
{
  return DoCommand ( "cmd_selectNone" );
}





class OnLinkClickEvent : public nsRunnable {
public:
  OnLinkClickEvent(nsDocShell* aHandler, nsIContent* aContent,
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
  nsRefPtr<nsDocShell>     mHandler;
  nsCOMPtr<nsIURI>         mURI;
  nsString                 mTargetSpec;
  nsCOMPtr<nsIInputStream> mPostDataStream;
  nsCOMPtr<nsIInputStream> mHeadersDataStream;
  nsCOMPtr<nsIContent>     mContent;
  PopupControlState        mPopupState;
};

OnLinkClickEvent::OnLinkClickEvent(nsDocShell* aHandler,
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
nsDocShell::OnLinkClick(nsIContent* aContent,
                        nsIURI* aURI,
                        const PRUnichar* aTargetSpec,
                        nsIInputStream* aPostDataStream,
                        nsIInputStream* aHeadersDataStream)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");

  if (!IsOKToLoadURI(aURI)) {
    return NS_OK;
  }

  if (aContent->IsEditable()) {
    return NS_OK;
  }

  nsCOMPtr<nsIRunnable> ev =
      new OnLinkClickEvent(this, aContent, aURI, aTargetSpec,
                           aPostDataStream, aHeadersDataStream);
  return NS_DispatchToCurrentThread(ev);
}

NS_IMETHODIMP
nsDocShell::OnLinkClickSync(nsIContent *aContent,
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

  if (!IsOKToLoadURI(aURI)) {
    return NS_OK;
  }

  if (aContent->IsEditable()) {
    return NS_OK;
  }

  {
    
    nsCOMPtr<nsIExternalProtocolService> extProtService =
        do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
    if (extProtService) {
      nsCAutoString scheme;
      aURI->GetScheme(scheme);
      if (!scheme.IsEmpty()) {
        
        
        PRBool isExposed;
        nsresult rv = extProtService->IsExposedProtocol(scheme.get(), &isExposed);
        if (NS_SUCCEEDED(rv) && !isExposed) {
          return extProtService->LoadURI(aURI, this); 
        }
      }
    }
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> refererDoc = aContent->GetOwnerDoc();
  NS_ENSURE_TRUE(refererDoc, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIURI> referer = refererDoc->GetDocumentURI();

  
  

  nsAutoString target(aTargetSpec);

  
  nsAutoString typeHint;
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(aContent));
  if (anchor) {
    anchor->GetType(typeHint);
  }
  
  nsresult rv = InternalLoad(aURI,                      
                             referer,                   
                             aContent->NodePrincipal(), 
                                                        
                             INTERNAL_LOAD_FLAGS_NONE,
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
nsDocShell::OnOverLink(nsIContent* aContent,
                       nsIURI* aURI,
                       const PRUnichar* aTargetSpec)
{
  if (aContent->IsEditable()) {
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
nsDocShell::OnLeaveLink()
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
nsDocShell::GetLinkState(nsIURI* aLinkURI, nsLinkState& aState)
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






NS_IMETHODIMP
nsDocShell::ReloadDocument(const char* aCharset,
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
  
  return NS_ERROR_DOCSHELL_REQUEST_REJECTED;
}


NS_IMETHODIMP
nsDocShell::StopDocumentLoad(void)
{
  if(eCharsetReloadRequested != mCharsetReloadState) 
  {
    Stop(nsIWebNavigation::STOP_ALL);
    return NS_OK;
  }
  
  return NS_ERROR_DOCSHELL_REQUEST_REJECTED;
}

NS_IMETHODIMP
nsDocShell::SetRendering(PRBool aRender)
{
  if(eCharsetReloadRequested != mCharsetReloadState) 
  {
    if (mContentViewer) {
       mContentViewer->SetEnableRendering(aRender);
       return NS_OK;
    }
  }
  
  return NS_ERROR_DOCSHELL_REQUEST_REJECTED;
}

#ifdef DEBUG
unsigned long nsDocShell::gNumberOfDocShells = 0;
#endif
