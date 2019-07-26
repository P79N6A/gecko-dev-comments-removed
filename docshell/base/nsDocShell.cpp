





#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/Util.h"
#include <algorithm>

#ifdef MOZ_LOGGING

#define FORCE_PR_LOG 1
#endif

#include "nsIBrowserDOMWindow.h"
#include "nsIComponentManager.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"
#include "nsIDOMStorage.h"
#include "nsPIDOMStorage.h"
#include "nsIContentViewer.h"
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
#include "prenv.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsXPIDLString.h"
#include "nsReadableUtils.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindow.h"
#include "nsIWebBrowserChrome.h"
#include "nsPoint.h"
#include "nsGfxCIID.h"
#include "nsIObserverService.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIAuthPrompt2.h"
#include "nsTextFormatter.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIUploadChannel.h"
#include "nsISecurityEventSink.h"
#include "nsIScriptSecurityManager.h"
#include "nsIJSContextStack.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScrollableFrame.h"
#include "nsContentPolicyUtils.h" 
#include "nsICategoryManager.h"
#include "nsXPCOMCID.h"
#include "nsISeekableStream.h"
#include "nsAutoPtr.h"
#include "nsIWritablePropertyBag2.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"
#include "nsDOMJSUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIScriptChannel.h"
#include "nsIOfflineCacheUpdate.h"
#include "nsITimedChannel.h"
#include "nsIPrivacyTransitionObserver.h"
#include "nsCPrefetchService.h"
#include "nsJSON.h"
#include "nsIDocShellTreeItem.h"
#include "nsIChannel.h"
#include "IHistory.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Attributes.h"




#include "nsIHttpChannelInternal.h"  



#include "nsDocShell.h"
#include "nsDocShellLoadInfo.h"
#include "nsCDefaultURIFixup.h"
#include "nsDocShellEnumerator.h"
#include "nsSHistory.h"
#include "nsDocShellEditorData.h"


#include "nsError.h"
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
#include "nsIWebBrowserChrome3.h"
#include "nsITabChild.h"
#include "nsIStrictTransportSecurityService.h"
#include "nsStructuredCloneContainer.h"
#include "nsIStructuredCloneContainer.h"
#ifdef MOZ_PLACES
#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"
#endif


#include "nsIEditingSession.h"

#include "nsPIDOMWindow.h"
#include "nsGlobalWindow.h"
#include "nsPIWindowRoot.h"
#include "nsIDOMDocument.h"
#include "nsICachingChannel.h"
#include "nsICacheVisitor.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIMultiPartChannel.h"
#include "nsIWyciwygChannel.h"



#include "nsIScriptError.h"


#include "nsCExternalHandlerService.h"
#include "nsIExternalProtocolService.h"

#include "nsFocusManager.h"

#include "nsITextToSubURI.h"

#include "nsIJARChannel.h"

#include "prlog.h"

#include "nsISelectionDisplay.h"

#include "nsIGlobalHistory2.h"

#include "nsEventStateManager.h"

#include "nsIFrame.h"
#include "nsSubDocumentFrame.h"


#include "nsIWebBrowserChromeFocus.h"

#if NS_PRINT_PREVIEW
#include "nsIDocumentViewerPrint.h"
#include "nsIWebBrowserPrint.h"
#endif

#include "nsContentUtils.h"
#include "nsIChannelPolicy.h"
#include "nsIContentSecurityPolicy.h"
#include "nsSandboxFlags.h"

#include "nsXULAppAPI.h"

#include "nsDOMNavigationTiming.h"
#include "nsITimedChannel.h"
#include "mozilla/StartupTimeline.h"

#include "mozilla/Telemetry.h"
#include "nsISecurityUITelemetry.h"

#include "nsIAppShellService.h"
#include "nsAppShellCID.h"

static NS_DEFINE_CID(kDOMScriptObjectFactoryCID,
                     NS_DOM_SCRIPT_OBJECT_FACTORY_CID);
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#if defined(DEBUG_bryner) || defined(DEBUG_chb)

#define DEBUG_PAGE_CACHE
#endif

using namespace mozilla;
using namespace mozilla::dom;


static int32_t gNumberOfDocumentsLoading = 0;


static int32_t gDocShellCount = 0;


static uint32_t gNumberOfPrivateDocShells = 0;


nsIURIFixup *nsDocShell::sURIFixup = 0;




static uint32_t gValidateOrigin = 0xffffffff;




#define NS_EVENT_STARVATION_DELAY_HINT 2000

#ifdef PR_LOGGING
#ifdef DEBUG
static PRLogModuleInfo* gDocShellLog;
#endif
static PRLogModuleInfo* gDocShellLeakLog;
#endif

const char kBrandBundleURL[]      = "chrome://branding/locale/brand.properties";
const char kAppstringsBundleURL[] = "chrome://global/locale/appstrings.properties";

static void
FavorPerformanceHint(bool perfOverStarvation, uint32_t starvationDelay)
{
    nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
    if (appShell)
        appShell->FavorPerformanceHint(perfOverStarvation, starvationDelay);
}





#define PREF_PINGS_ENABLED           "browser.send_pings"
#define PREF_PINGS_MAX_PER_LINK      "browser.send_pings.max_per_link"
#define PREF_PINGS_REQUIRE_SAME_HOST "browser.send_pings.require_same_host"

















static bool
PingsEnabled(int32_t *maxPerLink, bool *requireSameHost)
{
  bool allow = Preferences::GetBool(PREF_PINGS_ENABLED, false);

  *maxPerLink = 1;
  *requireSameHost = true;

  if (allow) {
    Preferences::GetInt(PREF_PINGS_MAX_PER_LINK, maxPerLink);
    Preferences::GetBool(PREF_PINGS_REQUIRE_SAME_HOST, requireSameHost);
  }

  return allow;
}

static bool
CheckPingURI(nsIURI* uri, nsIContent* content)
{
  if (!uri)
    return false;

  
  nsCOMPtr<nsIScriptSecurityManager> ssmgr =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(ssmgr, false);

  nsresult rv =
    ssmgr->CheckLoadURIWithPrincipal(content->NodePrincipal(), uri,
                                     nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return false;
  }

  
  bool match;
  if ((NS_FAILED(uri->SchemeIs("http", &match)) || !match) &&
      (NS_FAILED(uri->SchemeIs("https", &match)) || !match)) {
    return false;
  }

  
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_PING,
                                 uri,
                                 content->NodePrincipal(),
                                 content,
                                 EmptyCString(), 
                                 nullptr, 
                                 &shouldLoad);
  return NS_SUCCEEDED(rv) && NS_CP_ACCEPTED(shouldLoad);
}

typedef void (* ForEachPingCallback)(void *closure, nsIContent *content,
                                     nsIURI *uri, nsIIOService *ios);

static void
ForEachPing(nsIContent *content, ForEachPingCallback callback, void *closure)
{
  
  
  
  

  
  
  if (!content->IsHTML())
    return;
  nsIAtom *nameAtom = content->Tag();
  if (!nameAtom->Equals(NS_LITERAL_STRING("a")) &&
      !nameAtom->Equals(NS_LITERAL_STRING("area")))
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

  nsIDocument *doc = content->OwnerDoc();

  
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


static bool
IsSameHost(nsIURI *uri1, nsIURI *uri2)
{
  nsAutoCString host1, host2;
  uri1->GetAsciiHost(host1);
  uri2->GetAsciiHost(host2);
  return host1.Equals(host2);
}

class nsPingListener MOZ_FINAL : public nsIStreamListener
                               , public nsIInterfaceRequestor
                               , public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  nsPingListener(bool requireSameHost, nsIContent* content)
    : mRequireSameHost(requireSameHost),
      mContent(content)
  {}

private:
  bool mRequireSameHost;
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
                                nsIInputStream *stream, uint64_t offset,
                                uint32_t count)
{
  uint32_t result;
  return stream->ReadSegments(NS_DiscardSegment, nullptr, count, &result);
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
nsPingListener::AsyncOnChannelRedirect(nsIChannel *oldChan, nsIChannel *newChan,
                                       uint32_t flags,
                                       nsIAsyncVerifyRedirectCallback *callback)
{
  nsCOMPtr<nsIURI> newURI;
  newChan->GetURI(getter_AddRefs(newURI));

  if (!CheckPingURI(newURI, mContent))
    return NS_ERROR_ABORT;

  if (!mRequireSameHost) {
    callback->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIURI> oldURI;
  oldChan->GetURI(getter_AddRefs(oldURI));
  NS_ENSURE_STATE(oldURI && newURI);

  if (!IsSameHost(oldURI, newURI))
    return NS_ERROR_ABORT;

  callback->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

struct SendPingInfo {
  int32_t numPings;
  int32_t maxPings;
  bool    requireSameHost;
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

  nsIDocument *doc = content->OwnerDoc();

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
                             EmptyCString(), false);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-language"),
                             EmptyCString(), false);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-encoding"),
                             EmptyCString(), false);

  nsCOMPtr<nsIUploadChannel> uploadChan = do_QueryInterface(httpChan);
  if (!uploadChan)
    return;

  
  
  NS_NAMED_LITERAL_CSTRING(uploadData, "Content-Length: 0\r\n\r\n");

  nsCOMPtr<nsIInputStream> uploadStream;
  NS_NewPostDataStream(getter_AddRefs(uploadStream), false,
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

  chan->AsyncOpen(listener, nullptr);

  
  
  
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

static nsDOMPerformanceNavigationType
ConvertLoadTypeToNavigationType(uint32_t aLoadType)
{
  
  if (aLoadType == 0) {
    aLoadType = LOAD_NORMAL;
  }

  nsDOMPerformanceNavigationType result = dom::PerformanceNavigation::TYPE_RESERVED;
  switch (aLoadType) {
    case LOAD_NORMAL:
    case LOAD_NORMAL_EXTERNAL:
    case LOAD_NORMAL_BYPASS_CACHE:
    case LOAD_NORMAL_BYPASS_PROXY:
    case LOAD_NORMAL_BYPASS_PROXY_AND_CACHE:
    case LOAD_NORMAL_REPLACE:
    case LOAD_LINK:
    case LOAD_STOP_CONTENT:
    case LOAD_REPLACE_BYPASS_CACHE:
        result = dom::PerformanceNavigation::TYPE_NAVIGATE;
        break;
    case LOAD_HISTORY:
        result = dom::PerformanceNavigation::TYPE_BACK_FORWARD;
        break;
    case LOAD_RELOAD_NORMAL:
    case LOAD_RELOAD_CHARSET_CHANGE:
    case LOAD_RELOAD_BYPASS_CACHE:
    case LOAD_RELOAD_BYPASS_PROXY:
    case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:
    case LOAD_RELOAD_ALLOW_MIXED_CONTENT:
        result = dom::PerformanceNavigation::TYPE_RELOAD;
        break;
    case LOAD_STOP_CONTENT_AND_REPLACE:
    case LOAD_REFRESH:
    case LOAD_BYPASS_HISTORY:
    case LOAD_ERROR_PAGE:
    case LOAD_PUSHSTATE:
        result = dom::PerformanceNavigation::TYPE_RESERVED;
        break;
    default:
        
        result = dom::PerformanceNavigation::TYPE_RESERVED;
        break;
  }

  return result;
}

static nsISHEntry* GetRootSHEntry(nsISHEntry *entry);

static void
IncreasePrivateDocShellCount()
{
    gNumberOfPrivateDocShells++;
    if (gNumberOfPrivateDocShells > 1 ||
        XRE_GetProcessType() != GeckoProcessType_Content) {
        return;
    }

    mozilla::dom::ContentChild* cc = mozilla::dom::ContentChild::GetSingleton();
    cc->SendPrivateDocShellsExist(true);
}

static void
DecreasePrivateDocShellCount()
{
    MOZ_ASSERT(gNumberOfPrivateDocShells > 0);
    gNumberOfPrivateDocShells--;
    if (!gNumberOfPrivateDocShells)
    {
        if (XRE_GetProcessType() == GeckoProcessType_Content) {
            mozilla::dom::ContentChild* cc = mozilla::dom::ContentChild::GetSingleton();
            cc->SendPrivateDocShellsExist(false);
            return;
        }

        nsCOMPtr<nsIObserverService> obsvc = mozilla::services::GetObserverService();
        if (obsvc) {
            obsvc->NotifyObservers(nullptr, "last-pb-context-exited", nullptr);
        }
    }
}





static uint64_t gDocshellIDCounter = 0;


nsDocShell::nsDocShell():
    nsDocLoader(),
    mDefaultScrollbarPref(Scrollbar_Auto, Scrollbar_Auto),
    mTreeOwner(nullptr),
    mChromeEventHandler(nullptr),
    mCharsetReloadState(eCharsetReloadInit),
    mChildOffset(0),
    mBusyFlags(BUSY_FLAGS_NONE),
    mAppType(nsIDocShell::APP_TYPE_UNKNOWN),
    mLoadType(0),
    mMarginWidth(-1),
    mMarginHeight(-1),
    mItemType(typeContent),
    mPreviousTransIndex(-1),
    mLoadedTransIndex(-1),
    mSandboxFlags(0),
    mFullscreenAllowed(CHECK_ATTRIBUTES),
    mCreated(false),
    mAllowSubframes(true),
    mAllowPlugins(true),
    mAllowJavascript(true),
    mAllowMetaRedirects(true),
    mAllowImages(true),
    mAllowDNSPrefetch(true),
    mAllowWindowControl(true),
    mCreatingDocument(false),
    mUseErrorPages(false),
    mObserveErrorPages(true),
    mAllowAuth(true),
    mAllowKeywordFixup(false),
    mIsOffScreenBrowser(false),
    mIsActive(true),
    mIsAppTab(false),
    mUseGlobalHistory(false),
    mInPrivateBrowsing(false),
    mFiredUnloadEvent(false),
    mEODForCurrentDocument(false),
    mURIResultedInDocument(false),
    mIsBeingDestroyed(false),
    mIsExecutingOnLoadHandler(false),
    mIsPrintingOrPP(false),
    mSavingOldViewer(false),
#ifdef DEBUG
    mInEnsureScriptEnv(false),
#endif
    mAffectPrivateSessionLifetime(true),
    mFrameType(eFrameTypeRegular),
    mOwnOrContainingAppId(nsIScriptSecurityManager::UNKNOWN_APP_ID),
    mParentCharsetSource(0)
{
    mHistoryID = ++gDocshellIDCounter;
    if (gDocShellCount++ == 0) {
        NS_ASSERTION(sURIFixup == nullptr,
                     "Huh, sURIFixup not null in first nsDocShell ctor!");

        CallGetService(NS_URIFIXUP_CONTRACTID, &sURIFixup);
    }

#ifdef PR_LOGGING
#ifdef DEBUG
    if (! gDocShellLog)
        gDocShellLog = PR_NewLogModule("nsDocShell");
#endif
    if (nullptr == gDocShellLeakLog)
        gDocShellLeakLog = PR_NewLogModule("nsDocShellLeak");
    if (gDocShellLeakLog)
        PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p created\n", this));
#endif

#ifdef DEBUG
  
  ++gNumberOfDocShells;
  if (!PR_GetEnv("MOZ_QUIET")) {
      printf("++DOCSHELL %p == %ld [id = %llu]\n", (void*) this,
             gNumberOfDocShells, mHistoryID);
  }
#endif
}

nsDocShell::~nsDocShell()
{
    Destroy();

    nsCOMPtr<nsISHistoryInternal>
        shPrivate(do_QueryInterface(mSessionHistory));
    if (shPrivate) {
        shPrivate->SetRootDocShell(nullptr);
    }

    if (--gDocShellCount == 0) {
        NS_IF_RELEASE(sURIFixup);
    }

#ifdef PR_LOGGING
    if (gDocShellLeakLog)
        PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p destroyed\n", this));
#endif

#ifdef DEBUG
    
    --gNumberOfDocShells;
    if (!PR_GetEnv("MOZ_QUIET")) {
        printf("--DOCSHELL %p == %ld [id = %llu]\n", (void*) this,
               gNumberOfDocShells, mHistoryID);
    }
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

    mStorages.Init();

    
    
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
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; i++) {
        shell = do_QueryInterface(ChildAt(i));
        NS_ASSERTION(shell, "docshell has null child");

        if (shell) {
            shell->SetTreeOwner(nullptr);
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

    *aSink = nullptr;

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
    else if ((aIID.Equals(NS_GET_IID(nsPIDOMWindow)) ||
              aIID.Equals(NS_GET_IID(nsIDOMWindow)) ||
              aIID.Equals(NS_GET_IID(nsIDOMWindowInternal))) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
        return mScriptGlobal->QueryInterface(aIID, aSink);
    }
    else if (aIID.Equals(NS_GET_IID(nsIDOMDocument)) &&
             NS_SUCCEEDED(EnsureContentViewer())) {
        mContentViewer->GetDOMDocument((nsIDOMDocument **) aSink);
        return *aSink ? NS_OK : NS_NOINTERFACE;
    }
    else if (aIID.Equals(NS_GET_IID(nsIDocument)) &&
             NS_SUCCEEDED(EnsureContentViewer())) {
        nsCOMPtr<nsIDocument> doc = mContentViewer->GetDocument();
        doc.forget(aSink);
        return *aSink ? NS_OK : NS_NOINTERFACE;
    }
    else if (aIID.Equals(NS_GET_IID(nsIApplicationCacheContainer))) {
        *aSink = nullptr;

        

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
      nsIPresShell* shell = GetPresShell();
      if (shell)
        return shell->QueryInterface(aIID,aSink);    
    }
    else if (aIID.Equals(NS_GET_IID(nsIDocShellTreeOwner))) {
      nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
      nsresult rv = GetTreeOwner(getter_AddRefs(treeOwner));
      if (NS_SUCCEEDED(rv) && treeOwner)
        return treeOwner->QueryInterface(aIID, aSink);
    }
    else if (aIID.Equals(NS_GET_IID(nsITabChild))) {
      nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
      nsresult rv = GetTreeOwner(getter_AddRefs(treeOwner));
      if (NS_SUCCEEDED(rv) && treeOwner) {
        nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryInterface(treeOwner);
        if (ir)
          return ir->GetInterface(aIID, aSink);
      }
    }
    else if (aIID.Equals(NS_GET_IID(nsIContentFrameMessageManager))) {
      nsCOMPtr<nsITabChild> tabChild =
        do_GetInterface(static_cast<nsIDocShell*>(this));
      nsCOMPtr<nsIContentFrameMessageManager> mm;
      if (tabChild) {
        tabChild->
          GetMessageManager(getter_AddRefs(mm));
      } else {
        nsCOMPtr<nsPIDOMWindow> win =
          do_GetInterface(static_cast<nsIDocShell*>(this));
        if (win) {
          mm = do_QueryInterface(win->GetParentTarget());
        }
      }
      *aSink = mm.get();
    }
    else {
      return nsDocLoader::GetInterface(aIID, aSink);
    }

    NS_IF_ADDREF(((nsISupports *) * aSink));
    return *aSink ? NS_OK : NS_NOINTERFACE;
}

uint32_t
nsDocShell::
ConvertDocShellLoadInfoToLoadType(nsDocShellInfoLoadType aDocShellLoadType)
{
    uint32_t loadType = LOAD_NORMAL;

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
    case nsIDocShellLoadInfo::loadPushState:
        loadType = LOAD_PUSHSTATE;
        break;
    case nsIDocShellLoadInfo::loadReplaceBypassCache:
        loadType = LOAD_REPLACE_BYPASS_CACHE;
        break;
    case nsIDocShellLoadInfo::loadMixedContent:
        loadType = LOAD_RELOAD_ALLOW_MIXED_CONTENT;
        break;
    default:
        NS_NOTREACHED("Unexpected nsDocShellInfoLoadType value");
    }

    return loadType;
}


nsDocShellInfoLoadType
nsDocShell::ConvertLoadTypeToDocShellLoadInfo(uint32_t aLoadType)
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
    case LOAD_PUSHSTATE:
        docShellLoadType = nsIDocShellLoadInfo::loadPushState;
        break;
    case LOAD_REPLACE_BYPASS_CACHE:
        docShellLoadType = nsIDocShellLoadInfo::loadReplaceBypassCache;
        break;
    case LOAD_RELOAD_ALLOW_MIXED_CONTENT:
        docShellLoadType = nsIDocShellLoadInfo::loadMixedContent;
        break;
    default:
        NS_NOTREACHED("Unexpected load type value");
    }

    return docShellLoadType;
}                                                                               




NS_IMETHODIMP
nsDocShell::LoadURI(nsIURI * aURI,
                    nsIDocShellLoadInfo * aLoadInfo,
                    uint32_t aLoadFlags,
                    bool aFirstParty)
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
    bool inheritOwner = false;
    bool ownerIsExplicit = false;
    bool sendReferrer = true;
    nsCOMPtr<nsISHEntry> shEntry;
    nsXPIDLString target;
    uint32_t loadType = MAKE_LOAD_TYPE(LOAD_NORMAL, aLoadFlags);    

    NS_ENSURE_ARG(aURI);

    if (!StartupTimeline::HasRecord(StartupTimeline::FIRST_LOAD_URI) &&
        mItemType == typeContent && !NS_IsAboutBlank(aURI)) {
        StartupTimeline::RecordOnce(StartupTimeline::FIRST_LOAD_URI);
    }

    
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
        nsAutoCString uristr;
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
        uint32_t parentLoadType;

        if (parentDS && parentDS != static_cast<nsIDocShell *>(this)) {
            







            
            
            parentDS->GetLoadType(&parentLoadType);            

            nsCOMPtr<nsIDocShellHistory> parent(do_QueryInterface(parentAsItem));
            if (parent) {
                
                nsCOMPtr<nsISHEntry> currentSH;
                bool oshe = false;
                parent->GetCurrentSHEntry(getter_AddRefs(currentSH), &oshe);
                bool dynamicallyAddedChild = mDynamicallyCreated;
                if (!dynamicallyAddedChild && !oshe && currentSH) {
                    currentSH->HasDynamicallyAddedChild(&dynamicallyAddedChild);
                }
                if (!dynamicallyAddedChild) {
                    
                    
                    parent->GetChildSHEntry(mChildOffset, getter_AddRefs(shEntry));
                }

                
                
                if (mCurrentURI == nullptr) {
                    
                    
                    if (shEntry && (parentLoadType == LOAD_NORMAL ||
                                    parentLoadType == LOAD_LINK   ||
                                    parentLoadType == LOAD_NORMAL_EXTERNAL)) {
                        
                        
                        
                        
                        
                        bool inOnLoadHandler=false;
                        parentDS->GetIsExecutingOnLoadHandler(&inOnLoadHandler);
                        if (inOnLoadHandler) {
                            loadType = LOAD_NORMAL_REPLACE;
                            shEntry = nullptr;
                        }
                    }   
                    else if (parentLoadType == LOAD_REFRESH) {
                        
                        
                        shEntry = nullptr;
                    }
                    else if ((parentLoadType == LOAD_BYPASS_HISTORY) ||
                              (shEntry && 
                               ((parentLoadType & LOAD_CMD_HISTORY) || 
                                (parentLoadType == LOAD_RELOAD_NORMAL) || 
                                (parentLoadType == LOAD_RELOAD_CHARSET_CHANGE)))) {
                        
                        
                        
                        
                        loadType = parentLoadType;
                    }
                    else if (parentLoadType == LOAD_ERROR_PAGE) {
                        
                        
                        
                        loadType = LOAD_BYPASS_HISTORY;
                    }
                }
                else {
                    
                    
                    
                    
                    
                    uint32_t parentBusy = BUSY_FLAGS_NONE;
                    uint32_t selfBusy = BUSY_FLAGS_NONE;
                    parentDS->GetBusyFlags(&parentBusy);                    
                    GetBusyFlags(&selfBusy);
                    if (parentBusy & BUSY_FLAGS_BUSY ||
                        selfBusy & BUSY_FLAGS_BUSY) {
                        loadType = LOAD_NORMAL_REPLACE;
                        shEntry = nullptr; 
                    }
                }
            } 
        } 
        else {  
            
            
            
            bool inOnLoadHandler=false;
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

    
    
    
    
    
    
    if ((loadType == LOAD_NORMAL || loadType == LOAD_STOP_CONTENT) &&
        ShouldBlockLoadingForBackButton()) {
        return NS_OK;
    }

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (owner && mItemType != typeChrome) {
        nsCOMPtr<nsIPrincipal> ownerPrincipal = do_QueryInterface(owner);
        bool isSystem;
        rv = secMan->IsSystemPrincipal(ownerPrincipal, &isSystem);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isSystem) {
            if (ownerIsExplicit) {
                return NS_ERROR_DOM_SECURITY_ERR;
            }
            owner = nullptr;
            inheritOwner = true;
        }
    }
    if (!owner && !inheritOwner && !ownerIsExplicit) {
        
        rv = secMan->SubjectPrincipalIsSystem(&inheritOwner);
        if (NS_FAILED(rv)) {
            
            inheritOwner = false;
        }
    }

    if (aLoadFlags & LOAD_FLAGS_DISALLOW_INHERIT_OWNER) {
        inheritOwner = false;
        owner = do_CreateInstance("@mozilla.org/nullprincipal;1");
    }

    uint32_t flags = 0;

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
                        nullptr,         
                        NullString(),    
                        postStream,
                        headersStream,
                        loadType,
                        nullptr,         
                        aFirstParty,
                        nullptr,         
                        nullptr);        
}

NS_IMETHODIMP
nsDocShell::LoadStream(nsIInputStream *aStream, nsIURI * aURI,
                       const nsACString &aContentType,
                       const nsACString &aContentCharset,
                       nsIDocShellLoadInfo * aLoadInfo)
{
    NS_ENSURE_ARG(aStream);

    mAllowKeywordFixup = false;

    
    
    
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

    uint32_t loadType = LOAD_NORMAL;
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

    NS_ENSURE_SUCCESS(DoChannelLoad(channel, uriLoader, false),
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
  mEODForCurrentDocument = false;
  return NS_OK;
}


NS_IMETHODIMP
nsDocShell::FirePageHideNotification(bool aIsUnload)
{
    if (mContentViewer && !mFiredUnloadEvent) {
        
        
        nsCOMPtr<nsIContentViewer> kungFuDeathGrip(mContentViewer);
        mFiredUnloadEvent = true;

        if (mTiming) {
            mTiming->NotifyUnloadEventStart();
        }

        mContentViewer->PageHide(aIsUnload);

        if (mTiming) {
            mTiming->NotifyUnloadEventEnd();
        }

        nsAutoTArray<nsCOMPtr<nsIDocShell>, 8> kids;
        uint32_t n = mChildList.Length();
        kids.SetCapacity(n);
        for (uint32_t i = 0; i < n; i++) {
            kids.AppendElement(do_QueryInterface(ChildAt(i)));
        }

        n = kids.Length();
        for (uint32_t i = 0; i < n; ++i) {
            if (kids[i]) {
                kids[i]->FirePageHideNotification(aIsUnload);
            }
        }
        
        
        DetachEditorFromWindow();
    }

    return NS_OK;
}

nsresult
nsDocShell::MaybeInitTiming()
{
    if (mTiming) {
        return NS_OK;
    }

    if (Preferences::GetBool("dom.enable_performance", false)) {
        mTiming = new nsDOMNavigationTiming();
        mTiming->NotifyNavigationStart();
    }
    return NS_OK;
}












bool
nsDocShell::ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                           nsIDocShellTreeItem* aTargetTreeItem)
{
    
    
    if (nsContentUtils::GetCurrentJSContext() && nsContentUtils::IsCallerChrome()) {
        return true;
    }

    
    nsCOMPtr<nsIDocument> originDocument(do_GetInterface(aOriginTreeItem));
    NS_ENSURE_TRUE(originDocument, false);

    
    nsCOMPtr<nsIDocument> targetDocument(do_GetInterface(aTargetTreeItem));
    NS_ENSURE_TRUE(targetDocument, false);

    bool equal;
    nsresult rv = originDocument->NodePrincipal()->Equals(targetDocument->NodePrincipal(),
                                                          &equal);
    if (NS_SUCCEEDED(rv) && equal) {
        return true;
    }

    
    bool originIsFile = false;
    bool targetIsFile = false;
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
    NS_ENSURE_ARG_POINTER(aPresContext);
    *aPresContext = nullptr;

    nsCOMPtr<nsIContentViewer> viewer = mContentViewer;
    while (viewer) {
        nsCOMPtr<nsIContentViewer> prevViewer;
        viewer->GetPreviousViewer(getter_AddRefs(prevViewer));
        if (!prevViewer) {
            return viewer->GetPresContext(aPresContext);
        }
        viewer = prevViewer;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPresContext(nsPresContext ** aPresContext)
{
    NS_ENSURE_ARG_POINTER(aPresContext);
    *aPresContext = nullptr;

    if (!mContentViewer)
      return NS_OK;

    return mContentViewer->GetPresContext(aPresContext);
}

NS_IMETHODIMP_(nsIPresShell*)
nsDocShell::GetPresShell()
{
    nsRefPtr<nsPresContext> presContext;
    (void) GetPresContext(getter_AddRefs(presContext));
    return presContext ? presContext->GetPresShell() : nullptr;
}

NS_IMETHODIMP
nsDocShell::GetEldestPresShell(nsIPresShell** aPresShell)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aPresShell);
    *aPresShell = nullptr;

    nsRefPtr<nsPresContext> presContext;
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
    
    mChromeEventHandler = aChromeEventHandler;

    nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
    if (win) {
        win->SetChromeEventHandler(aChromeEventHandler);
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
    
    
    SetCurrentURI(aURI, nullptr, true, 0);
    return NS_OK;
}

bool
nsDocShell::SetCurrentURI(nsIURI *aURI, nsIRequest *aRequest,
                          bool aFireOnLocationChange, uint32_t aLocationFlags)
{
#ifdef PR_LOGGING
    if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
        nsAutoCString spec;
        if (aURI)
            aURI->GetSpec(spec);
        PR_LogPrint("DOCSHELL %p SetCurrentURI %s\n", this, spec.get());
    }
#endif

    
    
    if (mLoadType == LOAD_ERROR_PAGE) {
        return false;
    }

    mCurrentURI = NS_TryToMakeImmutable(aURI);
    
    bool isRoot = false;   
    bool isSubFrame = false;  

    nsCOMPtr<nsIDocShellTreeItem> root;

    GetSameTypeRootTreeItem(getter_AddRefs(root));
    if (root.get() == static_cast<nsIDocShellTreeItem *>(this)) 
    {
        
        isRoot = true;
    }
    if (mLSHE) {
        mLSHE->GetIsSubFrame(&isSubFrame);
    }

    if (!isSubFrame && !isRoot) {
      




      return false; 
    }

    if (aFireOnLocationChange) {
        FireOnLocationChange(this, aRequest, aURI, aLocationFlags);
    }
    return !aFireOnLocationChange;
}

NS_IMETHODIMP
nsDocShell::GetCharset(char** aCharset)
{
    NS_ENSURE_ARG_POINTER(aCharset);
    *aCharset = nullptr; 

    nsIPresShell* presShell = GetPresShell();
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
        nsCString charset(aCharset);
        NS_ENSURE_SUCCESS(muDV->SetDefaultCharacterSet(charset),
                          NS_ERROR_FAILURE);
      }
    }

    
    nsCOMPtr<nsIAtom> csAtom = do_GetAtom(aCharset);
    SetForcedCharset(csAtom);

    return NS_OK;
} 

NS_IMETHODIMP nsDocShell::SetForcedCharset(nsIAtom * aCharset)
{
  mForcedCharset = aCharset;
  return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetForcedCharset(nsIAtom ** aResult)
{
  *aResult = mForcedCharset;
  if (mForcedCharset) NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetParentCharset(nsIAtom * aCharset)
{
  mParentCharset = aCharset;
  return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetParentCharset(nsIAtom ** aResult)
{
  *aResult = mParentCharset;
  if (mParentCharset) NS_ADDREF(*aResult);
  return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetParentCharsetSource(int32_t aCharsetSource)
{
  mParentCharsetSource = aCharsetSource;
  return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetParentCharsetSource(int32_t * aParentCharsetSource)
{
  *aParentCharsetSource = mParentCharsetSource;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChannelIsUnsafe(bool *aUnsafe)
{
    *aUnsafe = false;

    nsIChannel* channel = GetCurrentDocChannel();
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
nsDocShell::GetHasMixedActiveContentLoaded(bool* aHasMixedActiveContentLoaded)
{
    nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));
    *aHasMixedActiveContentLoaded = doc && doc->GetHasMixedActiveContentLoaded();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedActiveContentBlocked(bool* aHasMixedActiveContentBlocked)
{
    nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));
    *aHasMixedActiveContentBlocked = doc && doc->GetHasMixedActiveContentBlocked();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedDisplayContentLoaded(bool* aHasMixedDisplayContentLoaded)
{
    nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));
    *aHasMixedDisplayContentLoaded = doc && doc->GetHasMixedDisplayContentLoaded();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedDisplayContentBlocked(bool* aHasMixedDisplayContentBlocked)
{
    nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));
    *aHasMixedDisplayContentBlocked = doc && doc->GetHasMixedDisplayContentBlocked();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowPlugins(bool * aAllowPlugins)
{
    NS_ENSURE_ARG_POINTER(aAllowPlugins);

    *aAllowPlugins = mAllowPlugins;
    if (!mAllowPlugins) {
        return NS_OK;
    }

    bool unsafe;
    *aAllowPlugins = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowPlugins(bool aAllowPlugins)
{
    mAllowPlugins = aAllowPlugins;
    
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowJavascript(bool * aAllowJavascript)
{
    NS_ENSURE_ARG_POINTER(aAllowJavascript);

    *aAllowJavascript = mAllowJavascript;
    if (!mAllowJavascript) {
        return NS_OK;
    }

    bool unsafe;
    *aAllowJavascript = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowJavascript(bool aAllowJavascript)
{
    mAllowJavascript = aAllowJavascript;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetUsePrivateBrowsing(bool* aUsePrivateBrowsing)
{
    NS_ENSURE_ARG_POINTER(aUsePrivateBrowsing);
    
    *aUsePrivateBrowsing = mInPrivateBrowsing;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetUsePrivateBrowsing(bool aUsePrivateBrowsing)
{
    nsContentUtils::ReportToConsoleNonLocalized(
        NS_LITERAL_STRING("Only internal code is allowed to set the usePrivateBrowsing attribute"),
        nsIScriptError::warningFlag,
        "Internal API Used",
        mContentViewer ? mContentViewer->GetDocument() : nullptr);

    return SetPrivateBrowsing(aUsePrivateBrowsing);
}

NS_IMETHODIMP
nsDocShell::SetPrivateBrowsing(bool aUsePrivateBrowsing)
{
    bool changed = aUsePrivateBrowsing != mInPrivateBrowsing;
    if (changed) {
        mInPrivateBrowsing = aUsePrivateBrowsing;
        if (mAffectPrivateSessionLifetime) {
            if (aUsePrivateBrowsing) {
                IncreasePrivateDocShellCount();
            } else {
                DecreasePrivateDocShellCount();
            }
        }
    }

    uint32_t count = mChildList.Length();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsILoadContext> shell = do_QueryInterface(ChildAt(i));
        if (shell) {
            shell->SetPrivateBrowsing(aUsePrivateBrowsing);
        }
    }

    if (changed) {
        nsTObserverArray<nsWeakPtr>::ForwardIterator iter(mPrivacyObservers);
        while (iter.HasMore()) {
            nsWeakPtr ref = iter.GetNext();
            nsCOMPtr<nsIPrivacyTransitionObserver> obs = do_QueryReferent(ref);
            if (!obs) {
                mPrivacyObservers.RemoveElement(ref);
            } else {
                obs->PrivateModeChanged(aUsePrivateBrowsing);
            }
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAffectPrivateSessionLifetime(bool aAffectLifetime)
{
    bool change = aAffectLifetime != mAffectPrivateSessionLifetime;
    if (change && mInPrivateBrowsing) {
        if (aAffectLifetime) {
            IncreasePrivateDocShellCount();
        } else {
            DecreasePrivateDocShellCount();
        }
    }
    mAffectPrivateSessionLifetime = aAffectLifetime;

    uint32_t count = mChildList.Length();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell) {
            shell->SetAffectPrivateSessionLifetime(aAffectLifetime);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAffectPrivateSessionLifetime(bool* aAffectLifetime)
{
    *aAffectLifetime = mAffectPrivateSessionLifetime;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::AddWeakPrivacyTransitionObserver(nsIPrivacyTransitionObserver* aObserver)
{
    nsWeakPtr weakObs = do_GetWeakReference(aObserver);
    if (!weakObs) {
        return NS_ERROR_NOT_AVAILABLE;
    }
    return mPrivacyObservers.AppendElement(weakObs) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsDocShell::GetAllowMetaRedirects(bool * aReturn)
{
    NS_ENSURE_ARG_POINTER(aReturn);

    *aReturn = mAllowMetaRedirects;
    if (!mAllowMetaRedirects) {
        return NS_OK;
    }

    bool unsafe;
    *aReturn = NS_SUCCEEDED(GetChannelIsUnsafe(&unsafe)) && !unsafe;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowMetaRedirects(bool aValue)
{
    mAllowMetaRedirects = aValue;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowSubframes(bool * aAllowSubframes)
{
    NS_ENSURE_ARG_POINTER(aAllowSubframes);

    *aAllowSubframes = mAllowSubframes;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowSubframes(bool aAllowSubframes)
{
    mAllowSubframes = aAllowSubframes;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowImages(bool * aAllowImages)
{
    NS_ENSURE_ARG_POINTER(aAllowImages);

    *aAllowImages = mAllowImages;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowImages(bool aAllowImages)
{
    mAllowImages = aAllowImages;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowDNSPrefetch(bool * aAllowDNSPrefetch)
{
    *aAllowDNSPrefetch = mAllowDNSPrefetch;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowDNSPrefetch(bool aAllowDNSPrefetch)
{
    mAllowDNSPrefetch = aAllowDNSPrefetch;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::GetAllowWindowControl(bool * aAllowWindowControl)
{
    *aAllowWindowControl = mAllowWindowControl;
    return NS_OK;
}

NS_IMETHODIMP nsDocShell::SetAllowWindowControl(bool aAllowWindowControl)
{
    mAllowWindowControl = aAllowWindowControl;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetFullscreenAllowed(bool* aFullscreenAllowed)
{
    NS_ENSURE_ARG_POINTER(aFullscreenAllowed);

    
    
    if (mFullscreenAllowed != CHECK_ATTRIBUTES) {
        *aFullscreenAllowed = (mFullscreenAllowed == PARENT_ALLOWS);
        return NS_OK;
    }

    
    *aFullscreenAllowed = false;

    
    
    
    
    nsCOMPtr<nsPIDOMWindow> win = do_GetInterface(GetAsSupports(this));
    if (!win) {
        return NS_OK;
    }
    nsCOMPtr<nsIContent> frameElement = do_QueryInterface(win->GetFrameElementInternal());
    if (frameElement &&
        frameElement->IsHTML(nsGkAtoms::iframe) &&
        !frameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::allowfullscreen) &&
        !frameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::mozallowfullscreen)) {
        return NS_OK;
    }

    
    
    
    nsCOMPtr<nsIDocShellTreeItem> dsti = do_GetInterface(GetAsSupports(this));
    NS_ENSURE_TRUE(dsti, NS_OK);

    nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
    dsti->GetParent(getter_AddRefs(parentTreeItem));
    if (!parentTreeItem) {
        *aFullscreenAllowed = true;
        return NS_OK;
    }
    
    
    nsCOMPtr<nsIDocShell> parent = do_QueryInterface(parentTreeItem);
    NS_ENSURE_TRUE(parent, NS_OK);
    
    return parent->GetFullscreenAllowed(aFullscreenAllowed);
}

NS_IMETHODIMP
nsDocShell::SetFullscreenAllowed(bool aFullscreenAllowed)
{
    if (!nsIDocShell::GetIsBrowserOrApp()) {
        
        
        
        
        
        
        return NS_ERROR_UNEXPECTED;
    }
    mFullscreenAllowed = (aFullscreenAllowed ? PARENT_ALLOWS : PARENT_PROHIBITS);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMayEnableCharacterEncodingMenu(bool* aMayEnableCharacterEncodingMenu)
{
  *aMayEnableCharacterEncodingMenu = false;
  if (!mContentViewer) {
    return NS_OK;
  }
  nsIDocument* doc = mContentViewer->GetDocument();
  if (!doc) {
    return NS_OK;
  }
  if (doc->WillIgnoreCharsetOverride()) {
    return NS_OK;
  }

  *aMayEnableCharacterEncodingMenu = true;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDocShellEnumerator(int32_t aItemType, int32_t aDirection, nsISimpleEnumerator **outEnum)
{
    NS_ENSURE_ARG_POINTER(outEnum);
    *outEnum = nullptr;
    
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
nsDocShell::GetAppType(uint32_t * aAppType)
{
    *aAppType = mAppType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAppType(uint32_t aAppType)
{
    mAppType = aAppType;
    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::GetAllowAuth(bool * aAllowAuth)
{
    *aAllowAuth = mAllowAuth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowAuth(bool aAllowAuth)
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
nsDocShell::GetMarginWidth(int32_t * aWidth)
{
    NS_ENSURE_ARG_POINTER(aWidth);

    *aWidth = mMarginWidth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMarginWidth(int32_t aWidth)
{
    mMarginWidth = aWidth;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMarginHeight(int32_t * aHeight)
{
    NS_ENSURE_ARG_POINTER(aHeight);

    *aHeight = mMarginHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMarginHeight(int32_t aHeight)
{
    mMarginHeight = aHeight;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetBusyFlags(uint32_t * aBusyFlags)
{
    NS_ENSURE_ARG_POINTER(aBusyFlags);

    *aBusyFlags = mBusyFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::TabToTreeOwner(bool aForward, bool* aTookFocus)
{
    NS_ENSURE_ARG_POINTER(aTookFocus);
    
    nsCOMPtr<nsIWebBrowserChromeFocus> chromeFocus = do_GetInterface(mTreeOwner);
    if (chromeFocus) {
        if (aForward)
            *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusNextElement());
        else
            *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusPrevElement());
    } else
        *aTookFocus = false;
    
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
nsDocShell::GetUseErrorPages(bool *aUseErrorPages)
{
    *aUseErrorPages = mUseErrorPages;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetUseErrorPages(bool aUseErrorPages)
{
    
    if (mObserveErrorPages) {
        Preferences::RemoveObserver(this, "browser.xul.error_pages.enabled");
        mObserveErrorPages = false;
    }
    mUseErrorPages = aUseErrorPages;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPreviousTransIndex(int32_t *aPreviousTransIndex)
{
    *aPreviousTransIndex = mPreviousTransIndex;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLoadedTransIndex(int32_t *aLoadedTransIndex)
{
    *aLoadedTransIndex = mLoadedTransIndex;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::HistoryPurged(int32_t aNumEntries)
{
    
    
    
    
    
    mPreviousTransIndex = std::max(-1, mPreviousTransIndex - aNumEntries);
    mLoadedTransIndex = std::max(0, mLoadedTransIndex - aNumEntries);

    uint32_t count = mChildList.Length();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell) {
            shell->HistoryPurged(aNumEntries);
        }
    }

    return NS_OK;
}

nsresult
nsDocShell::HistoryTransactionRemoved(int32_t aIndex)
{
    
    
    
    
    
    if (aIndex == mPreviousTransIndex) {
        mPreviousTransIndex = -1;
    } else if (aIndex < mPreviousTransIndex) {
        --mPreviousTransIndex;
    }
    if (mLoadedTransIndex == aIndex) {
        mLoadedTransIndex = 0;
    } else if (aIndex < mLoadedTransIndex) {
        --mLoadedTransIndex;
    }
                            
    uint32_t count = mChildList.Length();
    for (uint32_t i = 0; i < count; ++i) {
        nsCOMPtr<nsIDocShell> shell = do_QueryInterface(ChildAt(i));
        if (shell) {
            static_cast<nsDocShell*>(shell.get())->
                HistoryTransactionRemoved(aIndex);
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSessionStorageForPrincipal(nsIPrincipal* aPrincipal,
                                          const nsAString& aDocumentURI,
                                          bool aCreate,
                                          nsIDOMStorage** aStorage)
{
    NS_ENSURE_ARG_POINTER(aStorage);
    *aStorage = nullptr;

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
        return topDocShell->GetSessionStorageForPrincipal(aPrincipal,
                                                          aDocumentURI,
                                                          aCreate,
                                                          aStorage);

    nsXPIDLCString origin;
    rv = aPrincipal->GetOrigin(getter_Copies(origin));
    if (NS_FAILED(rv))
        return rv;

    if (origin.IsEmpty())
        return NS_OK;

    if (!mStorages.Get(origin, aStorage) && aCreate) {
        nsCOMPtr<nsIDOMStorage> newstorage =
            do_CreateInstance("@mozilla.org/dom/storage;2");
        if (!newstorage)
            return NS_ERROR_OUT_OF_MEMORY;

        nsCOMPtr<nsPIDOMStorage> pistorage = do_QueryInterface(newstorage);
        if (!pistorage)
            return NS_ERROR_FAILURE;

        rv = pistorage->InitAsSessionStorage(aPrincipal, aDocumentURI, mInPrivateBrowsing);
        if (NS_FAILED(rv))
            return rv;

        mStorages.Put(origin, newstorage);

        newstorage.swap(*aStorage);
#if defined(PR_LOGGING) && defined(DEBUG)
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]: created a new sessionStorage %p",
                this, *aStorage));
#endif
    }
    else if (*aStorage) {
        nsCOMPtr<nsPIDOMStorage> piStorage = do_QueryInterface(*aStorage);
        if (piStorage) {
            nsCOMPtr<nsIPrincipal> storagePrincipal = piStorage->Principal();

            
            
            
            
            
            
            bool equals;
            nsresult rv = aPrincipal->EqualsIgnoringDomain(storagePrincipal, &equals);
            NS_ASSERTION(NS_SUCCEEDED(rv) && equals,
                         "GetSessionStorageForPrincipal got a storage "
                         "that could not be accessed!");

            if (NS_FAILED(rv) || !equals) {
                NS_RELEASE(*aStorage);
                return NS_ERROR_DOM_SECURITY_ERR;
            }
        }

#if defined(PR_LOGGING) && defined(DEBUG)
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]: returns existing sessionStorage %p",
                this, *aStorage));
#endif
    }

    if (aCreate) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        nsCOMPtr<nsPIDOMStorage> piStorage = do_QueryInterface(*aStorage);
        nsCOMPtr<nsIDOMStorage> fork = piStorage->Fork(aDocumentURI);
#if defined(PR_LOGGING) && defined(DEBUG)
        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]: forked sessionStorage %p to %p",
                this, *aStorage, fork.get()));
#endif
        fork.swap(*aStorage);
    }

    return NS_OK;
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
            nsXPIDLCString origin;
            rv = aPrincipal->GetOrigin(getter_Copies(origin));
            if (NS_FAILED(rv))
                return rv;

            if (origin.IsEmpty())
                return NS_ERROR_FAILURE;

            
            if (mStorages.GetWeak(origin))
                return NS_ERROR_NOT_AVAILABLE;

#if defined(PR_LOGGING) && defined(DEBUG)
            PR_LOG(gDocShellLog, PR_LOG_DEBUG,
                   ("nsDocShell[%p]: was added a sessionStorage %p",
                    this, aStorage));
#endif
            mStorages.Put(origin, aStorage);
        }
        else {
            return topDocShell->AddSessionStorage(aPrincipal, aStorage);
        }
    }

    return NS_OK;
}

static PLDHashOperator
CloneSessionStorages(nsCStringHashKey::KeyType aKey, nsIDOMStorage* aStorage,
                     void* aUserArg)
{
    nsIDocShell *docShell = static_cast<nsIDocShell*>(aUserArg);
    nsCOMPtr<nsPIDOMStorage> pistorage = do_QueryInterface(aStorage);

    if (pistorage) {
        nsCOMPtr<nsIDOMStorage> storage = pistorage->Clone();
        docShell->AddSessionStorage(pistorage->Principal(), storage);
    }

    return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsDocShell::CloneSessionStoragesTo(nsIDocShell* aDocShell)
{
    aDocShell->ClearSessionStorages();
    mStorages.EnumerateRead(CloneSessionStorages, aDocShell);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ClearSessionStorages()
{
    mStorages.Clear();
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetCurrentDocumentChannel(nsIChannel** aResult)
{
    NS_IF_ADDREF(*aResult = GetCurrentDocChannel()); 
    return NS_OK;
}

nsIChannel*
nsDocShell::GetCurrentDocChannel()
{
    if (mContentViewer) {
        nsIDocument* doc = mContentViewer->GetDocument();
        if (doc) {
            return doc->GetChannel();
        }
    }
    return nullptr;
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
nsDocShell::NameEquals(const PRUnichar *aName, bool *_retval)
{
    NS_ENSURE_ARG_POINTER(aName);
    NS_ENSURE_ARG_POINTER(_retval);
    *_retval = mName.Equals(aName);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetItemType(int32_t * aItemType)
{
    NS_ENSURE_ARG_POINTER(aItemType);

    *aItemType = mItemType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetItemType(int32_t aItemType)
{
    NS_ENSURE_ARG((aItemType == typeChrome) || (typeContent == aItemType));

    
    
    nsCOMPtr<nsIDocumentLoader> docLoaderService =
        do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID);
    NS_ENSURE_TRUE(docLoaderService, NS_ERROR_UNEXPECTED);
    
    NS_ENSURE_STATE(!mParent || mParent == docLoaderService);

    mItemType = aItemType;

    
    mAllowAuth = mItemType == typeContent; 

    nsRefPtr<nsPresContext> presContext = nullptr;
    GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
        presContext->InvalidateIsChromeCache();
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParent(nsIDocShellTreeItem ** aParent)
{
    if (!mParent) {
        *aParent = nullptr;
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

    
    
    bool value;
    nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(parent));
    if (parentAsDocShell)
    {
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
        if (NS_SUCCEEDED(parentAsDocShell->GetAllowWindowControl(&value)))
        {
            SetAllowWindowControl(value);
        }
        if (NS_SUCCEEDED(parentAsDocShell->GetIsActive(&value)))
        {
            SetIsActive(value);
        }
        if (NS_FAILED(parentAsDocShell->GetAllowDNSPrefetch(&value))) {
            value = false;
        }
        SetAllowDNSPrefetch(value);
        value = parentAsDocShell->GetAffectPrivateSessionLifetime();
        SetAffectPrivateSessionLifetime(value);
    }

    nsCOMPtr<nsILoadContext> parentAsLoadContext(do_QueryInterface(parent));
    if (parentAsLoadContext &&
        NS_SUCCEEDED(parentAsLoadContext->GetUsePrivateBrowsing(&value)))
    {
        SetPrivateBrowsing(value);
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
    *aParent = nullptr;

    if (nsIDocShell::GetIsBrowserOrApp()) {
        return NS_OK;
    }

    nsCOMPtr<nsIDocShellTreeItem> parent =
        do_QueryInterface(GetAsSupports(mParent));
    if (!parent)
        return NS_OK;

    int32_t parentType;
    NS_ENSURE_SUCCESS(parent->GetItemType(&parentType), NS_ERROR_FAILURE);

    if (parentType == mItemType) {
        parent.swap(*aParent);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSameTypeParentIgnoreBrowserAndAppBoundaries(nsIDocShell** aParent)
{
    NS_ENSURE_ARG_POINTER(aParent);
    *aParent = nullptr;

    nsCOMPtr<nsIDocShellTreeItem> parent =
        do_QueryInterface(GetAsSupports(mParent));
    if (!parent)
        return NS_OK;

    int32_t parentType;
    NS_ENSURE_SUCCESS(parent->GetItemType(&parentType), NS_ERROR_FAILURE);

    if (parentType == mItemType) {
        nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parent);
        parentDS.forget(aParent);
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


bool
nsDocShell::CanAccessItem(nsIDocShellTreeItem* aTargetItem,
                          nsIDocShellTreeItem* aAccessingItem,
                          bool aConsiderOpener)
{
    NS_PRECONDITION(aTargetItem, "Must have target item!");

    if (!gValidateOrigin || !aAccessingItem) {
        
        return true;
    }

    
    

    
    
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    

    if (aTargetItem == aAccessingItem) {
        
        return true;
    }

    nsCOMPtr<nsIDocShell> targetDS = do_QueryInterface(aTargetItem);
    nsCOMPtr<nsIDocShell> accessingDS = do_QueryInterface(aAccessingItem);
    if (!!targetDS != !!accessingDS) {
        
        return false;
    }

    if (targetDS && accessingDS &&
        (targetDS->GetIsInBrowserElement() !=
           accessingDS->GetIsInBrowserElement() ||
         targetDS->GetAppId() != accessingDS->GetAppId())) {
        return false;
    }

    nsCOMPtr<nsIDocShellTreeItem> accessingRoot;
    aAccessingItem->GetSameTypeRootTreeItem(getter_AddRefs(accessingRoot));

    if (aTargetItem == accessingRoot) {
        
        return true;
    }

    
    nsCOMPtr<nsIDocShellTreeItem> target = aTargetItem;
    do {
        if (ValidateOrigin(aAccessingItem, target)) {
            return true;
        }
            
        nsCOMPtr<nsIDocShellTreeItem> parent;
        target->GetSameTypeParent(getter_AddRefs(parent));
        parent.swap(target);
    } while (target);

    nsCOMPtr<nsIDocShellTreeItem> targetRoot;
    aTargetItem->GetSameTypeRootTreeItem(getter_AddRefs(targetRoot));

    if (aTargetItem != targetRoot) {
        
        
        
        return false;
    }

    if (!aConsiderOpener) {
        
        return false;
    }

    nsCOMPtr<nsIDOMWindow> targetWindow = do_GetInterface(aTargetItem);
    if (!targetWindow) {
        NS_ERROR("This should not happen, really");
        return false;
    }

    nsCOMPtr<nsIDOMWindow> targetOpener;
    targetWindow->GetOpener(getter_AddRefs(targetOpener));
    nsCOMPtr<nsIWebNavigation> openerWebNav(do_GetInterface(targetOpener));
    nsCOMPtr<nsIDocShellTreeItem> openerItem(do_QueryInterface(openerWebNav));

    if (!openerItem) {
        return false;
    }

    return CanAccessItem(openerItem, aAccessingItem, false);    
}

static bool
ItemIsActive(nsIDocShellTreeItem *aItem)
{
    nsCOMPtr<nsIDOMWindow> window(do_GetInterface(aItem));

    if (window) {
        bool isClosed;

        if (NS_SUCCEEDED(window->GetClosed(&isClosed)) && !isClosed) {
            return true;
        }
    }

    return false;
}

NS_IMETHODIMP
nsDocShell::FindItemWithName(const PRUnichar * aName,
                             nsISupports * aRequestor,
                             nsIDocShellTreeItem * aOriginalRequestor,
                             nsIDocShellTreeItem ** _retval)
{
    NS_ENSURE_ARG(aName);
    NS_ENSURE_ARG_POINTER(_retval);

    
    *_retval = nullptr;

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
            foundItem = nullptr;
        }

        if (foundItem) {
            
            
            

            
            uint32_t sandboxFlags = 0;

            nsCOMPtr<nsIDocument> doc = do_GetInterface(aOriginalRequestor);

            if (doc) {
                sandboxFlags = doc->GetSandboxFlags();
            }

            if (sandboxFlags) {
                nsCOMPtr<nsIDocShellTreeItem> root;
                GetSameTypeRootTreeItem(getter_AddRefs(root));

                
                nsCOMPtr<nsIDocShellTreeItem> selfAsItem = static_cast<nsIDocShellTreeItem *>(this);
                if (foundItem != root && foundItem != selfAsItem) {
                    
                    bool isAncestor = false;

                    nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
                    GetSameTypeParent(getter_AddRefs(parentAsItem));

                    while (parentAsItem) {
                        nsCOMPtr<nsIDocShellTreeItem> tmp;
                        parentAsItem->GetParent(getter_AddRefs(tmp));

                        if (tmp && tmp == selfAsItem) {
                            isAncestor = true;
                            break;
                        }
                        parentAsItem = tmp;
                    }

                    if (!isAncestor) {
                        
                        
                        foundItem = nullptr;
                    }
                } else {
                    
                    nsCOMPtr<nsIDocShellTreeItem> tmp;
                    GetSameTypeParent(getter_AddRefs(tmp));

                    while (tmp) {
                        if (tmp && tmp == foundItem) {
                            
                            
                            if (sandboxFlags & SANDBOXED_TOPLEVEL_NAVIGATION) {
                                foundItem = nullptr;
                            }
                            break;
                        }
                        tmp->GetParent(getter_AddRefs(tmp));
                    }
                }
            }

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
    FindChildWithName(aName, true, true, reqAsTreeItem,
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

        int32_t parentType;
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
  for (int32_t i=0;i<aLevel;i++) printf("  ");

  int32_t childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentNode));
  int32_t type;
  aParentNode->GetItemType(&type);
  nsCOMPtr<nsIPresShell> presShell = parentAsDocShell->GetPresShell();
  nsRefPtr<nsPresContext> presContext;
  parentAsDocShell->GetPresContext(getter_AddRefs(presContext));
  nsIDocument *doc = presShell->GetDocument();

  nsCOMPtr<nsIDOMWindow> domwin(doc->GetWindow());

  nsCOMPtr<nsIWidget> widget;
  nsViewManager* vm = presShell->GetViewManager();
  if (vm) {
    vm->GetWidget(getter_AddRefs(widget));
  }
  dom::Element* rootElement = doc->GetRootElement();

  printf("DS %p  Ty %s  Doc %p DW %p EM %p CN %p\n",  
    (void*)parentAsDocShell.get(), 
    type==nsIDocShellTreeItem::typeChrome?"Chr":"Con", 
     (void*)doc, (void*)domwin.get(),
     (void*)presContext->EventStateManager(), (void*)rootElement);

  if (childWebshellCount > 0) {
    for (int32_t i=0;i<childWebshellCount;i++) {
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

    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; i++) {
        nsCOMPtr<nsIDocShellTreeItem> child = do_QueryInterface(ChildAt(i));
        NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);
        int32_t childType = ~mItemType; 
        child->GetItemType(&childType); 
        if (childType == mItemType)
            child->SetTreeOwner(aTreeOwner);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetChildOffset(uint32_t aChildOffset)
{
    mChildOffset = aChildOffset;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHistoryID(uint64_t* aID)
{
  *aID = mHistoryID;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsInUnload(bool* aIsInUnload)
{
    *aIsInUnload = mFiredUnloadEvent;
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetChildCount(int32_t * aChildCount)
{
    NS_ENSURE_ARG_POINTER(aChildCount);
    *aChildCount = mChildList.Length();
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

    
    
    aChild->SetTreeOwner(nullptr);
    
    nsresult res = AddChildLoader(childAsDocLoader);
    NS_ENSURE_SUCCESS(res, res);
    NS_ASSERTION(!mChildList.IsEmpty(),
                 "child list must not be empty after a successful add");

    nsCOMPtr<nsIDocShellHistory> docshellhistory = do_QueryInterface(aChild);
    bool dynamic = false;
    docshellhistory->GetCreatedDynamically(&dynamic);
    if (!dynamic) {
        nsCOMPtr<nsISHEntry> currentSH;
        bool oshe = false;
        GetCurrentSHEntry(getter_AddRefs(currentSH), &oshe);
        if (currentSH) {
            currentSH->HasDynamicallyAddedChild(&dynamic);
        }
    }
    nsCOMPtr<nsIDocShell> childDocShell = do_QueryInterface(aChild);
    childDocShell->SetChildOffset(dynamic ? -1 : mChildList.Length() - 1);

    
    if (mUseGlobalHistory) {
        nsCOMPtr<nsIDocShellHistory>
            dsHistoryChild(do_QueryInterface(aChild));
        if (dsHistoryChild)
            dsHistoryChild->SetUseGlobalHistory(true);
    }


    int32_t childType = ~mItemType;     
    aChild->GetItemType(&childType);
    if (childType != mItemType)
        return NS_OK;
    


    aChild->SetTreeOwner(mTreeOwner);

    nsCOMPtr<nsIDocShell> childAsDocShell(do_QueryInterface(aChild));
    if (!childAsDocShell)
        return NS_OK;

    

    
    
    
    
    
    
    

    
    if (mItemType == nsIDocShellTreeItem::typeChrome)
        return NS_OK;

    
    if (!mContentViewer)
        return NS_OK;
    nsIDocument* doc = mContentViewer->GetDocument();
    if (!doc)
        return NS_OK;
    const nsACString &parentCS = doc->GetDocumentCharacterSet();

    bool isWyciwyg = false;

    if (mCurrentURI) {
        
        mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);      
    }

    if (!isWyciwyg) {
        
        
        
        

        
        nsCOMPtr<nsIAtom> parentCSAtom(do_GetAtom(parentCS));
        res = childAsDocShell->SetParentCharset(parentCSAtom);
        if (NS_FAILED(res))
            return NS_OK;

        int32_t charsetSource = doc->GetDocumentCharacterSetSource();

        
        res = childAsDocShell->SetParentCharsetSource(charsetSource);
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
    
    aChild->SetTreeOwner(nullptr);

    return nsDocLoader::AddDocLoaderAsChildOfRoot(childAsDocLoader);
}

NS_IMETHODIMP
nsDocShell::GetChildAt(int32_t aIndex, nsIDocShellTreeItem ** aChild)
{
    NS_ENSURE_ARG_POINTER(aChild);

#ifdef DEBUG
    if (aIndex < 0) {
      NS_WARNING("Negative index passed to GetChildAt");
    }
    else if (aIndex >= mChildList.Length()) {
      NS_WARNING("Too large an index passed to GetChildAt");
    }
#endif

    nsIDocumentLoader* child = SafeChildAt(aIndex);
    NS_ENSURE_TRUE(child, NS_ERROR_UNEXPECTED);
    
    return CallQueryInterface(child, aChild);
}

NS_IMETHODIMP
nsDocShell::FindChildWithName(const PRUnichar * aName,
                              bool aRecurse, bool aSameType,
                              nsIDocShellTreeItem * aRequestor,
                              nsIDocShellTreeItem * aOriginalRequestor,
                              nsIDocShellTreeItem ** _retval)
{
    NS_ENSURE_ARG(aName);
    NS_ENSURE_ARG_POINTER(_retval);

    *_retval = nullptr;          

    if (!*aName)
        return NS_OK;

    nsXPIDLString childName;
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; i++) {
        nsCOMPtr<nsIDocShellTreeItem> child = do_QueryInterface(ChildAt(i));
        NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);
        int32_t childType;
        child->GetItemType(&childType);

        if (aSameType && (childType != mItemType))
            continue;

        bool childNameEquals = false;
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
            child->FindChildWithName(aName, true,
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
nsDocShell::GetChildSHEntry(int32_t aChildOffset, nsISHEntry ** aResult)
{
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = nullptr;

    
    
    
    
    if (mLSHE) {
        




        bool parentExpired=false;
        mLSHE->GetExpirationStatus(&parentExpired);
        
        


        uint32_t loadType = nsIDocShellLoadInfo::loadHistory;
        mLSHE->GetLoadType(&loadType);  
        
        
        if (loadType == nsIDocShellLoadInfo::loadReloadBypassCache ||
            loadType == nsIDocShellLoadInfo::loadReloadBypassProxy ||
            loadType == nsIDocShellLoadInfo::loadReloadBypassProxyAndCache ||
            loadType == nsIDocShellLoadInfo::loadRefresh)
            return rv;
        
        


        if (parentExpired && (loadType == nsIDocShellLoadInfo::loadReloadNormal)) {
            
            *aResult = nullptr;
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
                            int32_t aChildOffset, uint32_t loadType,
                            bool aCloneChildren)
{
    nsresult rv;

    if (mLSHE && loadType != LOAD_PUSHSTATE) {
        


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
        






        int32_t index = -1;
        nsCOMPtr<nsIHistoryEntry> currentHE;
        mSessionHistory->GetIndex(&index);
        if (index < 0)
            return NS_ERROR_FAILURE;

        rv = mSessionHistory->GetEntryAtIndex(index, false,
                                              getter_AddRefs(currentHE));
        NS_ENSURE_TRUE(currentHE, NS_ERROR_FAILURE);

        nsCOMPtr<nsISHEntry> currentEntry(do_QueryInterface(currentHE));
        if (currentEntry) {
            uint32_t cloneID = 0;
            nsCOMPtr<nsISHEntry> nextEntry;
            aCloneRef->GetID(&cloneID);
            rv = CloneAndReplace(currentEntry, this, cloneID, aNewEntry,
                                 aCloneChildren, getter_AddRefs(nextEntry));

            if (NS_SUCCEEDED(rv)) {
                nsCOMPtr<nsISHistoryInternal>
                    shPrivate(do_QueryInterface(mSessionHistory));
                NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
                rv = shPrivate->AddEntry(nextEntry, true);
            }
        }
    }
    else {
        
        nsCOMPtr<nsIDocShellHistory> parent =
            do_QueryInterface(GetAsSupports(mParent), &rv);
        if (parent) {
            rv = parent->AddChildSHEntry(aCloneRef, aNewEntry, aChildOffset,
                                         loadType, aCloneChildren);
        }          
    }
    return rv;
}

nsresult
nsDocShell::DoAddChildSHEntry(nsISHEntry* aNewEntry, int32_t aChildOffset,
                              bool aCloneChildren)
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
        rv = parent->AddChildSHEntry(mOSHE, aNewEntry, aChildOffset, mLoadType,
                                     aCloneChildren);
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
nsDocShell::SetUseGlobalHistory(bool aUseGlobalHistory)
{
    nsresult rv;

    mUseGlobalHistory = aUseGlobalHistory;

    if (!aUseGlobalHistory) {
        mGlobalHistory = nullptr;
        return NS_OK;
    }

    
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
        return NS_OK;
    }

    if (mGlobalHistory) {
        return NS_OK;
    }

    mGlobalHistory = do_GetService(NS_GLOBALHISTORY2_CONTRACTID, &rv);
    return rv;
}

NS_IMETHODIMP
nsDocShell::GetUseGlobalHistory(bool *aUseGlobalHistory)
{
    *aUseGlobalHistory = mUseGlobalHistory;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::RemoveFromSessionHistory()
{
    nsCOMPtr<nsISHistoryInternal> internalHistory;
    nsCOMPtr<nsISHistory> sessionHistory;
    nsCOMPtr<nsIDocShellTreeItem> root;
    GetSameTypeRootTreeItem(getter_AddRefs(root));
    if (root) {
        nsCOMPtr<nsIWebNavigation> rootAsWebnav =
            do_QueryInterface(root);
        if (rootAsWebnav) {
            rootAsWebnav->GetSessionHistory(getter_AddRefs(sessionHistory));
            internalHistory = do_QueryInterface(sessionHistory);
        }
    }
    if (!internalHistory) {
        return NS_OK;
    }

    int32_t index = 0;
    sessionHistory->GetIndex(&index);
    nsAutoTArray<uint64_t, 16> ids;
    ids.AppendElement(mHistoryID);
    internalHistory->RemoveEntries(ids, index);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetCreatedDynamically(bool aDynamic)
{
    mDynamicallyCreated = aDynamic;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetCreatedDynamically(bool* aDynamic)
{
    *aDynamic = mDynamicallyCreated;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetCurrentSHEntry(nsISHEntry** aEntry, bool* aOSHE)
{
    *aOSHE = false;
    *aEntry = nullptr;
    if (mLSHE) {
        NS_ADDREF(*aEntry = mLSHE);
    } else if (mOSHE) {
        NS_ADDREF(*aEntry = mOSHE);
        *aOSHE = true;
    }
    return NS_OK;
}

void
nsDocShell::ClearFrameHistory(nsISHEntry* aEntry)
{
  nsCOMPtr<nsISHContainer> shcontainer = do_QueryInterface(aEntry);
  nsCOMPtr<nsISHistory> rootSH;
  GetRootSessionHistory(getter_AddRefs(rootSH));
  nsCOMPtr<nsISHistoryInternal> history = do_QueryInterface(rootSH);
  if (!history || !shcontainer) {
    return;
  }

  int32_t count = 0;
  shcontainer->GetChildCount(&count);
  nsAutoTArray<uint64_t, 16> ids;
  for (int32_t i = 0; i < count; ++i) {
    nsCOMPtr<nsISHEntry> child;
    shcontainer->GetChildAt(i, getter_AddRefs(child));
    if (child) {
      uint64_t id = 0;
      child->GetDocshellID(&id);
      ids.AppendElement(id);
    }
  }
  int32_t index = 0;
  rootSH->GetIndex(&index);
  history->RemoveEntries(ids, index);
}




bool 
nsDocShell::IsPrintingOrPP(bool aDisplayErrorDialog)
{
  if (mIsPrintingOrPP && aDisplayErrorDialog) {
    DisplayLoadError(NS_ERROR_DOCUMENT_IS_PRINTMODE, nullptr, nullptr);
  }

  return mIsPrintingOrPP;
}

bool
nsDocShell::IsNavigationAllowed(bool aDisplayPrintErrorDialog)
{
    return !IsPrintingOrPP(aDisplayPrintErrorDialog) && !mFiredUnloadEvent;
}





NS_IMETHODIMP
nsDocShell::GetCanGoBack(bool * aCanGoBack)
{
    if (!IsNavigationAllowed(false)) {
      *aCanGoBack = false;
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
nsDocShell::GetCanGoForward(bool * aCanGoForward)
{
    if (!IsNavigationAllowed(false)) {
      *aCanGoForward = false;
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

NS_IMETHODIMP nsDocShell::GotoIndex(int32_t aIndex)
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
                    uint32_t aLoadFlags,
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
        
        
        
        
        uint32_t fixupFlags = 0;
        if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) {
          fixupFlags |= nsIURIFixup::FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;
        }
        if (aLoadFlags & LOAD_FLAGS_URI_IS_UTF8) {
          fixupFlags |= nsIURIFixup::FIXUP_FLAG_USE_UTF8;
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

    
    
    
    uint32_t extraFlags = (aLoadFlags & EXTRA_LOAD_FLAGS);
    aLoadFlags &= ~EXTRA_LOAD_FLAGS;

    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    rv = CreateLoadInfo(getter_AddRefs(loadInfo));
    if (NS_FAILED(rv)) return rv;
    
    uint32_t loadType = MAKE_LOAD_TYPE(LOAD_NORMAL, aLoadFlags);
    loadInfo->SetLoadType(ConvertLoadTypeToDocShellLoadInfo(loadType));
    loadInfo->SetPostDataStream(aPostStream);
    loadInfo->SetReferrer(aReferringURI);
    loadInfo->SetHeadersStream(aHeaderStream);

    rv = LoadURI(uri, loadInfo, extraFlags, true);

    
    
    mOriginalUriString = uriString; 

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
    const uint32_t kMaxFormatStrArgs = 3;
    nsAutoString formatStrs[kMaxFormatStrArgs];
    uint32_t formatStrCount = 0;
    bool addHostPort = false;
    nsresult rv = NS_OK;
    nsAutoString messageStr;
    nsAutoCString cssClass;
    nsAutoCString errorPage;

    errorPage.AssignLiteral("neterror");

    
    if (NS_ERROR_UNKNOWN_PROTOCOL == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        
        nsAutoCString scheme;
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
        
        nsAutoCString host;
        nsCOMPtr<nsIURI> innermostURI = NS_GetInnermostURI(aURI);
        innermostURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;
        error.AssignLiteral("dnsNotFound");
    }
    else if(NS_ERROR_CONNECTION_REFUSED == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        addHostPort = true;
        error.AssignLiteral("connectionFailure");
    }
    else if(NS_ERROR_NET_INTERRUPT == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        addHostPort = true;
        error.AssignLiteral("netInterrupt");
    }
    else if (NS_ERROR_NET_TIMEOUT == aError) {
        NS_ENSURE_ARG_POINTER(aURI);
        
        nsAutoCString host;
        aURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;
        error.AssignLiteral("netTimeout");
    }
    else if (NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION == aError) {
        
        cssClass.AssignLiteral("neterror");
        error.AssignLiteral("cspFrameAncestorBlocked");
    }
    else if (NS_ERROR_GET_MODULE(aError) == NS_ERROR_MODULE_SECURITY) {
        nsCOMPtr<nsINSSErrorsService> nsserr =
            do_GetService(NS_NSS_ERRORS_SERVICE_CONTRACTID);

        uint32_t errorClass;
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

                
                
                nsCOMPtr<nsIStrictTransportSecurityService> stss =
                          do_GetService(NS_STSSERVICE_CONTRACTID, &rv);
                NS_ENSURE_SUCCESS(rv, rv);
                uint32_t flags =
                  mInPrivateBrowsing ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
                
                bool isStsHost = false;
                rv = stss->IsStsURI(aURI, flags, &isStsHost);
                NS_ENSURE_SUCCESS(rv, rv);

                uint32_t bucketId;
                if (isStsHost) {
                  cssClass.AssignLiteral("badStsCert");
                  
                  
                  bucketId = nsISecurityUITelemetry::WARNING_BAD_CERT_STS;
                } else {
                  bucketId = nsISecurityUITelemetry::WARNING_BAD_CERT;
                }


                if (Preferences::GetBool(
                        "browser.xul.error_pages.expert_bad_cert", false)) {
                    cssClass.AssignLiteral("expertBadCert");
                }

                
                nsAdoptingCString alternateErrorPage =
                    Preferences::GetCString(
                        "security.alternate_certificate_error_page");
                if (alternateErrorPage)
                    errorPage.Assign(alternateErrorPage);

                if (errorPage.EqualsIgnoreCase("certerror")) 
                    mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI, bucketId);

            } else {
                error.AssignLiteral("nssFailure2");
            }
        }
    } else if (NS_ERROR_PHISHING_URI == aError || NS_ERROR_MALWARE_URI == aError) {
        nsAutoCString host;
        aURI->GetHost(host);
        CopyUTF8toUTF16(host, formatStrs[0]);
        formatStrCount = 1;

        
        
        nsAdoptingCString alternateErrorPage =
            Preferences::GetCString("urlclassifier.alternate_error_page");
        if (alternateErrorPage)
            errorPage.Assign(alternateErrorPage);

        uint32_t bucketId;
        if (NS_ERROR_PHISHING_URI == aError) {
            error.AssignLiteral("phishingBlocked");
            bucketId = nsISecurityUITelemetry::WARNING_PHISHING_PAGE;
        } else {
            error.AssignLiteral("malwareBlocked");
            bucketId = nsISecurityUITelemetry::WARNING_MALWARE_PAGE;
        }

        if (errorPage.EqualsIgnoreCase("blocked"))
            mozilla::Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI,
                                           bucketId);

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
            
            
            error.AssignLiteral("notCached");
            break;
        case NS_ERROR_OFFLINE:
            
            error.AssignLiteral("netOffline");
            break;
        case NS_ERROR_DOCUMENT_IS_PRINTMODE:
            
            error.AssignLiteral("isprinting");
            break;
        case NS_ERROR_PORT_ACCESS_NOT_ALLOWED:
            
            addHostPort = true;
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
        case NS_ERROR_REMOTE_XUL:
        {
            error.AssignLiteral("remoteXUL");
            break;
        }
        case NS_ERROR_UNSAFE_CONTENT_TYPE:
            
            error.AssignLiteral("unsafeContentType");
            break;
        case NS_ERROR_CORRUPTED_CONTENT:
            
            error.AssignLiteral("corruptedContentError");
            break;
        default:
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
            
            nsAutoCString hostport;
            if (aURI) {
                aURI->GetHostPort(hostport);
            } else {
                hostport.AssignLiteral("?");
            }
            CopyUTF8toUTF16(hostport, formatStrs[formatStrCount++]);
        }

        nsAutoCString spec;
        rv = NS_ERROR_NOT_AVAILABLE;
        if (aURI) {
            
            
            bool isFileURI = false;
            rv = aURI->SchemeIs("file", &isFileURI);
            if (NS_SUCCEEDED(rv) && isFileURI)
                aURI->GetPath(spec);
            else
                aURI->GetSpec(spec);

            nsAutoCString charset;
            
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
        for (uint32_t i = 0; i < formatStrCount; i++) {
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
    if (mUseErrorPages) {
        
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

        
        prompter->Alert(nullptr, messageStr.get());
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
        nsAutoCString spec;
        aURI->GetSpec(spec);

        nsAutoCString chanName;
        if (aFailedChannel)
            aFailedChannel->GetName(chanName);
        else
            chanName.AssignLiteral("<no channel>");

        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]::LoadErrorPage(\"%s\", \"%s\", {...}, [%s])\n", this,
                spec.get(), NS_ConvertUTF16toUTF8(aURL).get(), chanName.get()));
    }
#endif
    mFailedChannel = aFailedChannel;
    mFailedURI = aURI;
    mFailedLoadType = mLoadType;

    if (mLSHE) {
        
        
        
        mLSHE->AbandonBFCacheEntry();
    }

    nsAutoCString url;
    nsAutoCString charset;
    if (aURI)
    {
        nsresult rv = aURI->GetSpec(url);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = aURI->GetOriginCharset(charset);
        NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (aURL)
    {
        
        nsresult rv = NS_NewURI(getter_AddRefs(mFailedURI), "about:blank");
        NS_ENSURE_SUCCESS(rv, rv);

        CopyUTF16toUTF8(aURL, url);
    }
    else
    {
        return NS_ERROR_INVALID_POINTER;
    }

    

#undef SAFE_ESCAPE
#define SAFE_ESCAPE(cstring, escArg1, escArg2)  \
    {                                           \
        char* s = nsEscape(escArg1, escArg2);   \
        if (!s)                                 \
            return NS_ERROR_OUT_OF_MEMORY;      \
        cstring.Adopt(s);                       \
    }
    nsCString escapedUrl, escapedCharset, escapedError, escapedDescription,
              escapedCSSClass;
    SAFE_ESCAPE(escapedUrl, url.get(), url_Path);
    SAFE_ESCAPE(escapedCharset, charset.get(), url_Path);
    SAFE_ESCAPE(escapedError,
                NS_ConvertUTF16toUTF8(aErrorType).get(), url_Path);
    SAFE_ESCAPE(escapedDescription,
                NS_ConvertUTF16toUTF8(aDescription).get(), url_Path);
    if (aCSSClass) {
        SAFE_ESCAPE(escapedCSSClass, aCSSClass, url_Path);
    }
    nsCString errorPageUrl("about:");
    errorPageUrl.AppendASCII(aErrorPage);
    errorPageUrl.AppendLiteral("?e=");

    errorPageUrl.AppendASCII(escapedError.get());
    errorPageUrl.AppendLiteral("&u=");
    errorPageUrl.AppendASCII(escapedUrl.get());
    if (!escapedCSSClass.IsEmpty()) {
        errorPageUrl.AppendASCII("&s=");
        errorPageUrl.AppendASCII(escapedCSSClass.get());
    }
    errorPageUrl.AppendLiteral("&c=");
    errorPageUrl.AppendASCII(escapedCharset.get());
    errorPageUrl.AppendLiteral("&d=");
    errorPageUrl.AppendASCII(escapedDescription.get());

    nsCOMPtr<nsIURI> errorPageURI;
    nsresult rv = NS_NewURI(getter_AddRefs(errorPageURI), errorPageUrl);
    NS_ENSURE_SUCCESS(rv, rv);

    return InternalLoad(errorPageURI, nullptr, nullptr,
                        INTERNAL_LOAD_FLAGS_INHERIT_OWNER, nullptr, nullptr,
                        NullString(), nullptr, nullptr, LOAD_ERROR_PAGE,
                        nullptr, true, nullptr, nullptr);
}


NS_IMETHODIMP
nsDocShell::Reload(uint32_t aReloadFlags)
{
    if (!IsNavigationAllowed()) {
      return NS_OK; 
    }
    nsresult rv;
    NS_ASSERTION(((aReloadFlags & 0xf) == 0),
                 "Reload command not updated to use load flags!");
    NS_ASSERTION((aReloadFlags & EXTRA_LOAD_FLAGS) == 0,
                 "Don't pass these flags to Reload");

    uint32_t loadType = MAKE_LOAD_TYPE(LOAD_RELOAD_NORMAL, aReloadFlags);
    NS_ENSURE_TRUE(IsValidLoadType(loadType), NS_ERROR_INVALID_ARG);

    
    nsCOMPtr<nsISHistory> rootSH;
    rv = GetRootSessionHistory(getter_AddRefs(rootSH));
    nsCOMPtr<nsISHistoryInternal> shistInt(do_QueryInterface(rootSH));
    bool canReload = true;
    if (rootSH) {
      shistInt->NotifyOnHistoryReload(mCurrentURI, aReloadFlags, &canReload);
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
        nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));

        nsIPrincipal* principal = nullptr;
        nsAutoString contentTypeHint;
        if (doc) {
            principal = doc->NodePrincipal();
            doc->GetContentType(contentTypeHint);
        }

        rv = InternalLoad(mCurrentURI,
                          mReferrerURI,
                          principal,
                          INTERNAL_LOAD_FLAGS_NONE, 
                          nullptr,         
                          NS_LossyConvertUTF16toASCII(contentTypeHint).get(),
                          NullString(),    
                          nullptr,         
                          nullptr,         
                          loadType,        
                          nullptr,         
                          true,
                          nullptr,         
                          nullptr);        
    }

    return rv;
}

NS_IMETHODIMP
nsDocShell::Stop(uint32_t aStopFlags)
{
    
    mRestorePresentationEvent.Revoke();

    if (mLoadType == LOAD_ERROR_PAGE) {
        if (mLSHE) {
            
            SetHistoryEntry(&mOSHE, mLSHE);
            SetHistoryEntry(&mLSHE, nullptr);
        }

        mFailedChannel = nullptr;
        mFailedURI = nullptr;
    }

    if (nsIWebNavigation::STOP_CONTENT & aStopFlags) {
        
        if (mContentViewer)
            mContentViewer->Stop();
    }

    if (nsIWebNavigation::STOP_NETWORK & aStopFlags) {
        
        
        if (mRefreshURIList) {
            SuspendRefreshURIs();
            mSavedRefreshURIList.swap(mRefreshURIList);
            mRefreshURIList = nullptr;
        }

        
        
        
        Stop();
    }

    uint32_t count = mChildList.Length();
    for (uint32_t n = 0; n < count; n++) {
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

    *aURI = nullptr;
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
nsDocShell::LoadPage(nsISupports *aPageDescriptor, uint32_t aDisplayType)
{
    nsCOMPtr<nsISHEntry> shEntryIn(do_QueryInterface(aPageDescriptor));

    
    if (!shEntryIn) {
        return NS_ERROR_INVALID_POINTER;
    }

    
    
    nsCOMPtr<nsISHEntry> shEntry;
    nsresult rv = shEntryIn->Clone(getter_AddRefs(shEntry));
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    rv = shEntry->AbandonBFCacheEntry();
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

    *aPageDescriptor = nullptr;

    nsISHEntry* src = mOSHE ? mOSHE : mLSHE;
    if (src) {
        nsCOMPtr<nsISHEntry> dest;

        nsresult rv = src->Clone(getter_AddRefs(dest));
        if (NS_FAILED(rv)) {
            return rv;
        }

        
        dest->SetParent(nullptr);
        dest->SetIsSubFrame(false);
        
        return CallQueryInterface(dest, aPageDescriptor);
    }

    return NS_ERROR_NOT_AVAILABLE;
}






NS_IMETHODIMP
nsDocShell::InitWindow(nativeWindow parentNativeWindow,
                       nsIWidget * parentWidget, int32_t x, int32_t y,
                       int32_t cx, int32_t cy)
{
    SetParentWidget(parentWidget);
    SetPositionAndSize(x, y, cx, cy, false);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::Create()
{
    if (mCreated) {
        
        return NS_OK;
    }

    NS_ASSERTION(mItemType == typeContent || mItemType == typeChrome,
                 "Unexpected item type in docshell");

    NS_ENSURE_TRUE(Preferences::GetRootBranch(), NS_ERROR_FAILURE);
    mCreated = true;

    mAllowSubframes =
        Preferences::GetBool("browser.frames.enabled", mAllowSubframes);

    if (gValidateOrigin == 0xffffffff) {
        
        gValidateOrigin =
            Preferences::GetBool("browser.frame.validate_origin", true);
    }

    
    mUseErrorPages =
        Preferences::GetBool("browser.xul.error_pages.enabled", mUseErrorPages);

    if (mObserveErrorPages) {
        Preferences::AddStrongObserver(this, "browser.xul.error_pages.enabled");
    }

    nsCOMPtr<nsIObserverService> serv = services::GetObserverService();
    if (serv) {
        const char* msg = mItemType == typeContent ?
            NS_WEBNAVIGATION_CREATE : NS_CHROME_WEBNAVIGATION_CREATE;
        serv->NotifyObservers(GetAsSupports(this), msg, nullptr);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::Destroy()
{
    NS_ASSERTION(mItemType == typeContent || mItemType == typeChrome,
                 "Unexpected item type in docshell");

    if (!mIsBeingDestroyed) {
        nsCOMPtr<nsIObserverService> serv = services::GetObserverService();
        if (serv) {
            const char* msg = mItemType == typeContent ?
                NS_WEBNAVIGATION_DESTROY : NS_CHROME_WEBNAVIGATION_DESTROY;
            serv->NotifyObservers(GetAsSupports(this), msg, nullptr);
        }
    }
    
    mIsBeingDestroyed = true;

    
    if (mObserveErrorPages) {
        Preferences::RemoveObserver(this, "browser.xul.error_pages.enabled");
        mObserveErrorPages = false;
    }

    
    
    mLoadingURI = nullptr;

    
    (void) FirePageHideNotification(true);

    
    
    if (mOSHE)
      mOSHE->SetEditorData(nullptr);
    if (mLSHE)
      mLSHE->SetEditorData(nullptr);
      
    
    
    if (mContentListener) {
        mContentListener->DropDocShellreference();
        mContentListener->SetParentContentListener(nullptr);
        
        
        
        
        
    }

    
    Stop(nsIWebNavigation::STOP_ALL);

    mEditorData = nullptr;

    mTransferableHookData = nullptr;

    
    
    
    PersistLayoutHistoryState();

    
    nsCOMPtr<nsIDocShellTreeItem> docShellParentAsItem =
        do_QueryInterface(GetAsSupports(mParent));
    if (docShellParentAsItem)
        docShellParentAsItem->RemoveChild(this);

    if (mContentViewer) {
        mContentViewer->Close(nullptr);
        mContentViewer->Destroy();
        mContentViewer = nullptr;
    }

    nsDocLoader::Destroy();
    
    mParentWidget = nullptr;
    mCurrentURI = nullptr;

    if (mScriptGlobal) {
        nsCOMPtr<nsPIDOMWindow> win(do_QueryInterface(mScriptGlobal));
        win->DetachFromDocShell();

        mScriptGlobal = nullptr;
    }

    if (mSessionHistory) {
        
        
        
        nsCOMPtr<nsISHistoryInternal> shPrivate =
            do_QueryInterface(mSessionHistory);
        if (shPrivate) {
            shPrivate->EvictAllContentViewers();
        }
        mSessionHistory = nullptr;
    }

    SetTreeOwner(nullptr);

    
    mSecurityUI = nullptr;

    
    
    CancelRefreshURITimers();

    if (mInPrivateBrowsing) {
        mInPrivateBrowsing = false;
        if (mAffectPrivateSessionLifetime) {
            DecreasePrivateDocShellCount();
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetUnscaledDevicePixelsPerCSSPixel(double *aScale)
{
    if (mParentWidget) {
        *aScale = mParentWidget->GetDefaultScale();
        return NS_OK;
    }

    nsCOMPtr<nsIBaseWindow> ownerWindow(do_QueryInterface(mTreeOwner));
    if (ownerWindow) {
        return ownerWindow->GetUnscaledDevicePixelsPerCSSPixel(aScale);
    }

    *aScale = 1.0;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetPosition(int32_t x, int32_t y)
{
    mBounds.x = x;
    mBounds.y = y;

    if (mContentViewer)
        NS_ENSURE_SUCCESS(mContentViewer->Move(x, y), NS_ERROR_FAILURE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPosition(int32_t * aX, int32_t * aY)
{
    int32_t dummyHolder;
    return GetPositionAndSize(aX, aY, &dummyHolder, &dummyHolder);
}

NS_IMETHODIMP
nsDocShell::SetSize(int32_t aCX, int32_t aCY, bool aRepaint)
{
    int32_t x = 0, y = 0;
    GetPosition(&x, &y);
    return SetPositionAndSize(x, y, aCX, aCY, aRepaint);
}

NS_IMETHODIMP
nsDocShell::GetSize(int32_t * aCX, int32_t * aCY)
{
    int32_t dummyHolder;
    return GetPositionAndSize(&dummyHolder, &dummyHolder, aCX, aCY);
}

NS_IMETHODIMP
nsDocShell::SetPositionAndSize(int32_t x, int32_t y, int32_t cx,
                               int32_t cy, bool fRepaint)
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
nsDocShell::GetPositionAndSize(int32_t * x, int32_t * y, int32_t * cx,
                               int32_t * cy)
{
    if (mParentWidget) {
        
        nsIntRect r;
        mParentWidget->GetClientBounds(r);
        SetPositionAndSize(mBounds.x, mBounds.y, r.width, r.height, false);
    }

    
    
    if (cx || cy) {
        
        
        nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(mParent)));
        if (doc) {
            doc->FlushPendingNotifications(Flush_Layout);
        }
    }
    
    DoGetPositionAndSize(x, y, cx, cy);
    return NS_OK;
}

void
nsDocShell::DoGetPositionAndSize(int32_t * x, int32_t * y, int32_t * cx,
                                 int32_t * cy)
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
nsDocShell::Repaint(bool aForce)
{
    nsCOMPtr<nsIPresShell> presShell =GetPresShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    nsViewManager* viewManager = presShell->GetViewManager();
    NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

    viewManager->InvalidateAllViews();
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
        *parentNativeWindow = nullptr;

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetParentNativeWindow(nativeWindow parentNativeWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetNativeHandle(nsAString& aNativeHandle)
{
    
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetVisibility(bool * aVisibility)
{
    NS_ENSURE_ARG_POINTER(aVisibility);

    *aVisibility = false;

    if (!mContentViewer)
        return NS_OK;

    nsCOMPtr<nsIPresShell> presShell = GetPresShell();
    if (!presShell)
        return NS_OK;

    
    nsViewManager* vm = presShell->GetViewManager();
    NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

    
    nsView *view = vm->GetRootView(); 
    NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

    
    if (view->GetVisibility() == nsViewVisibility_kHide)
        return NS_OK;

    
    
    

    nsCOMPtr<nsIDocShellTreeItem> treeItem = this;
    nsCOMPtr<nsIDocShellTreeItem> parentItem;
    treeItem->GetParent(getter_AddRefs(parentItem));
    while (parentItem) {
        nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(treeItem));
        presShell = docShell->GetPresShell();

        nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parentItem);
        nsCOMPtr<nsIPresShell> pPresShell = parentDS->GetPresShell();

        
        if (!pPresShell) {
            NS_NOTREACHED("parent docshell has null pres shell");
            return NS_OK;
        }

        nsIContent *shellContent =
            pPresShell->GetDocument()->FindContentForSubDocument(presShell->GetDocument());
        NS_ASSERTION(shellContent, "subshell not in the map");

        nsIFrame* frame = shellContent ? shellContent->GetPrimaryFrame() : nullptr;
        bool isDocShellOffScreen = false;
        docShell->GetIsOffScreenBrowser(&isDocShellOffScreen);
        if (frame &&
            !frame->IsVisibleConsideringAncestors(nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY) &&
            !isDocShellOffScreen) {
            return NS_OK;
        }

        treeItem = parentItem;
        treeItem->GetParent(getter_AddRefs(parentItem));
    }

    nsCOMPtr<nsIBaseWindow> treeOwnerAsWin(do_QueryInterface(mTreeOwner));
    if (!treeOwnerAsWin) {
        *aVisibility = true;
        return NS_OK;
    }

    
    
    return treeOwnerAsWin->GetVisibility(aVisibility);
}

NS_IMETHODIMP
nsDocShell::SetIsOffScreenBrowser(bool aIsOffScreen) 
{
    mIsOffScreenBrowser = aIsOffScreen;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsOffScreenBrowser(bool *aIsOffScreen) 
{
    *aIsOffScreen = mIsOffScreenBrowser;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsActive(bool aIsActive)
{
  
  if (mItemType == nsIDocShellTreeItem::typeChrome)
    return NS_ERROR_INVALID_ARG;

  
  mIsActive = aIsActive;

  
  nsCOMPtr<nsIPresShell> pshell = GetPresShell();
  if (pshell)
    pshell->SetIsActive(aIsActive);

  
  nsCOMPtr<nsPIDOMWindow> win = do_QueryInterface(mScriptGlobal);
  if (win) {
      win->SetIsBackground(!aIsActive);
      nsCOMPtr<nsIDocument> doc = do_QueryInterface(win->GetExtantDocument());
      if (doc) {
          doc->PostVisibilityUpdateEvent();
      }
  }

  
  
  uint32_t n = mChildList.Length();
  for (uint32_t i = 0; i < n; ++i) {
      nsCOMPtr<nsIDocShell> docshell = do_QueryInterface(ChildAt(i));
      if (!docshell) {
          continue;
      }

      if (!docshell->GetIsBrowserOrApp()) {
          docshell->SetIsActive(aIsActive);
      }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsActive(bool *aIsActive)
{
  *aIsActive = mIsActive;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsAppTab(bool aIsAppTab)
{
  mIsAppTab = aIsAppTab;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsAppTab(bool *aIsAppTab)
{
  *aIsAppTab = mIsAppTab;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetSandboxFlags(uint32_t aSandboxFlags)
{
    mSandboxFlags = aSandboxFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSandboxFlags(uint32_t  *aSandboxFlags)
{
    *aSandboxFlags = mSandboxFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMixedContentChannel(nsIChannel* aMixedContentChannel)
{
#ifdef DEBUG
     
     if (aMixedContentChannel) {
       
       nsCOMPtr<nsIDocShellTreeItem> root;
       GetSameTypeRootTreeItem(getter_AddRefs(root));
       NS_WARN_IF_FALSE(
         root.get() == static_cast<nsIDocShellTreeItem *>(this), 
         "Setting mMixedContentChannel on a docshell that is not the root docshell"
       );
    }
#endif
     mMixedContentChannel = aMixedContentChannel;
     return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMixedContentChannel(nsIChannel **aMixedContentChannel)
{
    NS_ENSURE_ARG_POINTER(aMixedContentChannel);
    NS_IF_ADDREF(*aMixedContentChannel = mMixedContentChannel);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowMixedContentAndConnectionData(bool* aRootHasSecureConnection, bool* aAllowMixedContent, bool* aIsRootDocShell)
{
  *aRootHasSecureConnection = true;
  *aAllowMixedContent = false;
  *aIsRootDocShell = false;

  nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
  GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
  NS_ASSERTION(sameTypeRoot, "No document shell root tree item from document shell tree item!");
  *aIsRootDocShell = sameTypeRoot.get() == static_cast<nsIDocShellTreeItem *>(this);

  
  nsCOMPtr<nsIDocument> rootDoc = do_GetInterface(sameTypeRoot);
  if (rootDoc) {
    nsCOMPtr<nsIPrincipal> rootPrincipal = rootDoc->NodePrincipal();

    
    
    nsCOMPtr<nsIURI> rootUri;
    if (nsContentUtils::IsSystemPrincipal(rootPrincipal) ||
        NS_FAILED(rootPrincipal->GetURI(getter_AddRefs(rootUri))) || !rootUri ||
        NS_FAILED(rootUri->SchemeIs("https", aRootHasSecureConnection))) {
      *aRootHasSecureConnection = false;
    }

    
    
    
    nsCOMPtr<nsIDocShell> rootDocShell = do_QueryInterface(sameTypeRoot);
    nsCOMPtr<nsIChannel> mixedChannel;
    rootDocShell->GetMixedContentChannel(getter_AddRefs(mixedChannel));
    *aAllowMixedContent = mixedChannel && (mixedChannel == rootDoc->GetChannel());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetVisibility(bool aVisibility)
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
nsDocShell::GetEnabled(bool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = true;
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::SetEnabled(bool aEnabled)
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

    if (mCurrentURI && mLoadType != LOAD_ERROR_PAGE && mUseGlobalHistory &&
        !mInPrivateBrowsing) {
        nsCOMPtr<IHistory> history = services::GetHistoryService();
        if (history) {
            history->SetURITitle(mCurrentURI, mTitle);
        }
        else if (mGlobalHistory) {
            mGlobalHistory->SetPageTitle(mCurrentURI, nsString(mTitle));
        }
    }

    
    if (mOSHE && mLoadType != LOAD_BYPASS_HISTORY &&
        mLoadType != LOAD_ERROR_PAGE) {

        mOSHE->SetTitle(mTitle);    
    }

    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetCurScrollPos(int32_t scrollOrientation, int32_t * curPos)
{
    NS_ENSURE_ARG_POINTER(curPos);

    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    nsPoint pt = sf->GetScrollPosition();

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        *curPos = pt.x;
        return NS_OK;

    case ScrollOrientation_Y:
        *curPos = pt.y;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
    }
}

NS_IMETHODIMP
nsDocShell::SetCurScrollPos(int32_t scrollOrientation, int32_t curPos)
{
    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    nsPoint pt = sf->GetScrollPosition();

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        pt.x = curPos;
        break;

    case ScrollOrientation_Y:
        pt.y = curPos;
        break;

    default:
        NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
    }

    sf->ScrollTo(pt, nsIScrollableFrame::INSTANT);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetCurScrollPosEx(int32_t curHorizontalPos, int32_t curVerticalPos)
{
    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    sf->ScrollTo(nsPoint(curHorizontalPos, curVerticalPos),
                 nsIScrollableFrame::INSTANT);
    return NS_OK;
}


NS_IMETHODIMP
nsDocShell::GetScrollRange(int32_t scrollOrientation,
                           int32_t * minPos, int32_t * maxPos)
{
    NS_ENSURE_ARG_POINTER(minPos && maxPos);

    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    nsSize portSize = sf->GetScrollPortRect().Size();
    nsRect range = sf->GetScrollRange();

    switch (scrollOrientation) {
    case ScrollOrientation_X:
        *minPos = range.x;
        *maxPos = range.XMost() + portSize.width;
        return NS_OK;

    case ScrollOrientation_Y:
        *minPos = range.y;
        *maxPos = range.YMost() + portSize.height;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
    }
}

NS_IMETHODIMP
nsDocShell::SetScrollRange(int32_t scrollOrientation,
                           int32_t minPos, int32_t maxPos)
{
    
    







    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::SetScrollRangeEx(int32_t minHorizontalPos,
                             int32_t maxHorizontalPos, int32_t minVerticalPos,
                             int32_t maxVerticalPos)
{
    
    







    return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsDocShell::GetDefaultScrollbarPreferences(int32_t scrollOrientation,
                                           int32_t * scrollbarPref)
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
        NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
    }
    return NS_ERROR_FAILURE;
}










NS_IMETHODIMP
nsDocShell::SetDefaultScrollbarPreferences(int32_t scrollOrientation,
                                           int32_t scrollbarPref)
{
    switch (scrollOrientation) {
    case ScrollOrientation_X:
        mDefaultScrollbarPref.x = scrollbarPref;
        return NS_OK;

    case ScrollOrientation_Y:
        mDefaultScrollbarPref.y = scrollbarPref;
        return NS_OK;

    default:
        NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
    }
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::GetScrollbarVisibility(bool * verticalVisible,
                                   bool * horizontalVisible)
{
    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    uint32_t scrollbarVisibility = sf->GetScrollbarVisibility();
    if (verticalVisible)
        *verticalVisible = (scrollbarVisibility & nsIScrollableFrame::VERTICAL) != 0;
    if (horizontalVisible)
        *horizontalVisible = (scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) != 0;

    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::ScrollByLines(int32_t numLines)
{
    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    sf->ScrollBy(nsIntPoint(0, numLines), nsIScrollableFrame::LINES,
                 nsIScrollableFrame::SMOOTH);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ScrollByPages(int32_t numPages)
{
    nsIScrollableFrame* sf = GetRootScrollFrame();
    NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

    sf->ScrollBy(nsIntPoint(0, numPages), nsIScrollableFrame::PAGES,
                 nsIScrollableFrame::SMOOTH);
    return NS_OK;
}





nsIScriptGlobalObject*
nsDocShell::GetScriptGlobalObject()
{
    NS_ENSURE_SUCCESS(EnsureScriptEnvironment(), nullptr);

    return mScriptGlobal;
}





NS_IMETHODIMP
nsDocShell::RefreshURI(nsIURI * aURI, int32_t aDelay, bool aRepeat,
                       bool aMetaRefresh)
{
    NS_ENSURE_ARG(aURI);

    





    bool allowRedirects = true;
    GetAllowMetaRedirects(&allowRedirects);
    if (!allowRedirects)
        return NS_OK;

    
    
    bool sameURI;
    nsresult rv = aURI->Equals(mCurrentURI, &sameURI);
    if (NS_FAILED(rv))
        sameURI = false;
    if (!RefreshAttempted(this, aURI, aDelay, sameURI))
        return NS_OK;

    nsRefreshTimer *refreshTimer = new nsRefreshTimer();
    NS_ENSURE_TRUE(refreshTimer, NS_ERROR_OUT_OF_MEMORY);
    uint32_t busyFlags = 0;
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
                                     int32_t aDelay, 
                                     bool aMetaRefresh,
                                     nsITimer* aTimer)
{
    NS_PRECONDITION(aTimer, "Must have a timer here");

    
    if (mRefreshURIList) {
        uint32_t n = 0;
        mRefreshURIList->Count(&n);

        for (uint32_t i = 0;  i < n; ++i) {
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
                            int32_t aDelay, 
                            bool aMetaRefresh)
{
    NS_ENSURE_ARG(aURI);

    nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
    CreateLoadInfo(getter_AddRefs(loadInfo));
    NS_ENSURE_TRUE(loadInfo, NS_ERROR_OUT_OF_MEMORY);

    


    loadInfo->SetSendReferrer(false);

    


    loadInfo->SetReferrer(mCurrentURI);

    


    loadInfo->SetOwnerIsExplicit(true);

    


    bool equalUri = false;
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

    



    LoadURI(aURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, true);

    return NS_OK;
}

nsresult
nsDocShell::SetupRefreshURIFromHeader(nsIURI * aBaseURI,
                                      nsIPrincipal* aPrincipal,
                                      const nsACString & aHeader)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    
    MOZ_ASSERT(aPrincipal);

    nsAutoCString uriAttrib;
    int32_t seconds = 0;
    bool specifiesSeconds = false;

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
        specifiesSeconds = true;
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

    

    bool isQuotedURI = false;
    if (tokenStart != doneIterating && (*tokenStart == '"' || *tokenStart == '\''))
    {
        isQuotedURI = true;
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
    bool specifiesURI = false;
    if (tokenStart == iter) {
        uri = aBaseURI;
    }
    else {
        uriAttrib = Substring(tokenStart, iter);
        
        rv = NS_NewURI(getter_AddRefs(uri), uriAttrib, nullptr, aBaseURI);
        specifiesURI = true;
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
                CheckLoadURIWithPrincipal(aPrincipal, uri,
                                          nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT);

            if (NS_SUCCEEDED(rv)) {
                bool isjs = true;
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

                rv = RefreshURI(uri, seconds * 1000, false, true);
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
        nsAutoCString refreshHeader;
        rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("refresh"),
                                            refreshHeader);

        if (!refreshHeader.IsEmpty()) {
            nsCOMPtr<nsIScriptSecurityManager> secMan =
                do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            nsCOMPtr<nsIPrincipal> principal;
            rv = secMan->GetChannelPrincipal(aChannel, getter_AddRefs(principal));
            NS_ENSURE_SUCCESS(rv, rv);

            SetupReferrerFromChannel(aChannel);
            rv = SetupRefreshURIFromHeader(mCurrentURI, principal, refreshHeader);
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

    uint32_t n=0;
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
    mRefreshURIList = nullptr;
    mSavedRefreshURIList = nullptr;

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRefreshPending(bool* _retval)
{
    if (!mRefreshURIList) {
        *_retval = false;
        return NS_OK;
    }

    uint32_t count;
    nsresult rv = mRefreshURIList->Count(&count);
    if (NS_SUCCEEDED(rv))
        *_retval = (count != 0);
    return rv;
}

NS_IMETHODIMP
nsDocShell::SuspendRefreshURIs()
{
    if (mRefreshURIList) {
        uint32_t n = 0;
        mRefreshURIList->Count(&n);

        for (uint32_t i = 0;  i < n; ++i) {
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

    
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; ++i) {
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

    
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; ++i) {
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
    uint32_t n = 0;
    mRefreshURIList->Count(&n);

    while (n) {
        nsCOMPtr<nsISupports> element;
        mRefreshURIList->GetElementAt(--n, getter_AddRefs(element));
        nsCOMPtr<nsITimerCallback> refreshInfo(do_QueryInterface(element));

        if (refreshInfo) {   
            
            
            
            uint32_t delay = static_cast<nsRefreshTimer*>(static_cast<nsITimerCallback*>(refreshInfo))->GetDelay();
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
        bool isWyciwyg = false;
        
        rv = mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);      
        if (isWyciwyg && NS_SUCCEEDED(rv))
            SetBaseUrlForWyciwyg(aContentViewer);
    }
    
    if (mLSHE) {
        
        if (mLSHE->HasDetachedEditor()) {
            ReattachEditorToWindow(mLSHE);
        }
        
        SetDocCurrentStateObj(mLSHE);

        SetHistoryEntry(&mOSHE, mLSHE);
    }

    bool updateHistory = true;

    
    switch (mLoadType) {
    case LOAD_NORMAL_REPLACE:
    case LOAD_STOP_CONTENT_AND_REPLACE:
    case LOAD_RELOAD_BYPASS_CACHE:
    case LOAD_RELOAD_BYPASS_PROXY:
    case LOAD_RELOAD_BYPASS_PROXY_AND_CACHE:
    case LOAD_REPLACE_BYPASS_CACHE:
        updateHistory = false;
        break;
    default:
        break;
    }

    if (!updateHistory)
        SetLayoutHistoryState(nullptr);

    return NS_OK;
}


NS_IMETHODIMP 
nsDocShell::SetIsPrinting(bool aIsPrinting)
{
    mIsPrintingOrPP = aIsPrinting;
    return NS_OK;
}





NS_IMETHODIMP
nsDocShell::OnProgressChange(nsIWebProgress * aProgress,
                             nsIRequest * aRequest,
                             int32_t aCurSelfProgress,
                             int32_t aMaxSelfProgress,
                             int32_t aCurTotalProgress,
                             int32_t aMaxTotalProgress)
{
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnStateChange(nsIWebProgress * aProgress, nsIRequest * aRequest,
                          uint32_t aStateFlags, nsresult aStatus)
{
    if ((~aStateFlags & (STATE_START | STATE_IS_NETWORK)) == 0) {
        
        nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        nsAutoCString aURI;
        uri->GetAsciiSpec(aURI);

        nsCOMPtr<nsIWyciwygChannel>  wcwgChannel(do_QueryInterface(aRequest));
        nsCOMPtr<nsIWebProgress> webProgress =
            do_QueryInterface(GetAsSupports(this));

        
        if (this == aProgress && !wcwgChannel){
            MaybeInitTiming();
            if (mTiming) {
                mTiming->NotifyFetchStart(uri, ConvertLoadTypeToNavigationType(mLoadType));
            } 
        }

        
        if (wcwgChannel && !mLSHE && (mItemType == typeContent) && aProgress == webProgress.get()) {
            bool equalUri = true;
            
            
            
            if (mCurrentURI &&
                NS_SUCCEEDED(uri->Equals(mCurrentURI, &equalUri)) &&
                !equalUri) {

                nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
                GetSameTypeParent(getter_AddRefs(parentAsItem));
                nsCOMPtr<nsIDocShell> parentDS(do_QueryInterface(parentAsItem));
                bool inOnLoadHandler = false;
                if (parentDS) {
                  parentDS->GetIsExecutingOnLoadHandler(&inOnLoadHandler);
                }
                if (inOnLoadHandler) {
                    
                    
                    
                    
                    
                    nsCOMPtr<nsIDocShellHistory> parent =
                        do_QueryInterface(parentAsItem);
                    if (parent) {
                        bool oshe = false;
                        nsCOMPtr<nsISHEntry> entry;
                        parent->GetCurrentSHEntry(getter_AddRefs(entry), &oshe);
                        static_cast<nsDocShell*>(parent.get())->
                            ClearFrameHistory(entry);
                    }
                }

                
                
                
                
                AddToSessionHistory(uri, wcwgChannel, nullptr, false,
                                    getter_AddRefs(mLSHE));
                SetCurrentURI(uri, aRequest, true, 0);
                
                PersistLayoutHistoryState();
                
                
                SetHistoryEntry(&mOSHE, mLSHE);
            }
        
        }
        
        mBusyFlags = BUSY_FLAGS_BUSY | BUSY_FLAGS_BEFORE_PAGE_LOAD;

        if ((aStateFlags & STATE_RESTORING) == 0) {
            
            if (Preferences::GetBool("ui.use_activity_cursor", false)) {
                nsCOMPtr<nsIWidget> mainWidget;
                GetMainWidget(getter_AddRefs(mainWidget));
                if (mainWidget) {
                    mainWidget->SetCursor(eCursor_spinning);
                }
            }
        }
    }
    else if ((~aStateFlags & (STATE_TRANSFERRING | STATE_IS_DOCUMENT)) == 0) {
        
        mBusyFlags = BUSY_FLAGS_BUSY | BUSY_FLAGS_PAGE_LOADING;
    }
    else if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_NETWORK)) {
        
        mBusyFlags = BUSY_FLAGS_NONE;

        
        if (Preferences::GetBool("ui.use_activity_cursor", false)) {
            nsCOMPtr<nsIWidget> mainWidget;
            GetMainWidget(getter_AddRefs(mainWidget));
            if (mainWidget) {
                mainWidget->SetCursor(eCursor_standard);
            }
        }
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
nsDocShell::OnLocationChange(nsIWebProgress * aProgress, nsIRequest * aRequest,
                             nsIURI * aURI, uint32_t aFlags)
{
    NS_NOTREACHED("notification excluded in AddProgressListener(...)");
    return NS_OK;
}

void
nsDocShell::OnRedirectStateChange(nsIChannel* aOldChannel,
                                  nsIChannel* aNewChannel,
                                  uint32_t aRedirectFlags,
                                  uint32_t aStateFlags)
{
    NS_ASSERTION(aStateFlags & STATE_REDIRECTING,
                 "Calling OnRedirectStateChange when there is no redirect");
    if (!(aStateFlags & STATE_IS_DOCUMENT))
        return; 

    nsCOMPtr<nsIURI> oldURI, newURI;
    aOldChannel->GetURI(getter_AddRefs(oldURI));
    aNewChannel->GetURI(getter_AddRefs(newURI));
    if (!oldURI || !newURI) {
        return;
    }
    
    bool equals = false;
    if (mTiming &&
        !(mLoadType == LOAD_HISTORY &&
          NS_SUCCEEDED(newURI->Equals(oldURI, &equals)) && equals)) {
        mTiming->NotifyRedirect(oldURI, newURI);
    }

    
    
    
    
    
    

    
    nsCOMPtr<nsIURI> previousURI;
    uint32_t previousFlags = 0;
    ExtractLastVisit(aOldChannel, getter_AddRefs(previousURI), &previousFlags);

    if (aRedirectFlags & nsIChannelEventSink::REDIRECT_INTERNAL ||
        ChannelIsPost(aOldChannel)) {
        
        
        
        
        
        
        SaveLastVisit(aNewChannel, previousURI, previousFlags);
    }
    else {
        nsCOMPtr<nsIURI> referrer;
        
        (void)NS_GetReferrerFromChannel(aOldChannel,
                                        getter_AddRefs(referrer));

        
        uint32_t responseStatus = 0;
        nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aOldChannel);
        if (httpChannel) {
            (void)httpChannel->GetResponseStatus(&responseStatus);
        }

        
        AddURIVisit(oldURI, referrer, previousURI, previousFlags,
                    responseStatus);

        
        
        SaveLastVisit(aNewChannel, oldURI, aRedirectFlags);
    }

    
    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(aNewChannel);
    if (appCacheChannel) {
        if (GeckoProcessType_Default != XRE_GetProcessType()) {
            
            appCacheChannel->SetChooseApplicationCache(true);
        } else {
            appCacheChannel->SetChooseApplicationCache(
                                NS_ShouldCheckAppCache(newURI,
                                                       mInPrivateBrowsing));
        }
    }

    if (!(aRedirectFlags & nsIChannelEventSink::REDIRECT_INTERNAL) && 
        mLoadType & (LOAD_CMD_RELOAD | LOAD_CMD_HISTORY)) {
        mLoadType = LOAD_NORMAL_REPLACE;
        SetHistoryEntry(&mLSHE, nullptr);
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
                             nsIRequest * aRequest, uint32_t state)
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

    nsCOMPtr<nsITimedChannel> timingChannel =
        do_QueryInterface(aChannel);
    if (timingChannel) {
        TimeStamp channelCreationTime;
        rv = timingChannel->GetChannelCreation(&channelCreationTime);
        if (NS_SUCCEEDED(rv) && !channelCreationTime.IsNull())
            Telemetry::AccumulateTimeDelta(
                Telemetry::TOTAL_CONTENT_PAGE_LOAD_TIME,
                channelCreationTime);
    }

    
    mTiming = nullptr;

    
    if (eCharsetReloadRequested == mCharsetReloadState)
        mCharsetReloadState = eCharsetReloadStopOrigional;
    else 
        mCharsetReloadState = eCharsetReloadInit;

    
    
    
    nsCOMPtr<nsISHEntry> loadingSHE = mLSHE;
  
    
    
    
    
    
    nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

    
    
    if (!mEODForCurrentDocument && mContentViewer) {
        mIsExecutingOnLoadHandler = true;
        mContentViewer->LoadComplete(aStatus);
        mIsExecutingOnLoadHandler = false;

        mEODForCurrentDocument = true;

        
        
        
        if (--gNumberOfDocumentsLoading == 0) {
          
          FavorPerformanceHint(false, NS_EVENT_STARVATION_DELAY_HINT);
        }
    }
    




    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
    if (!httpChannel) 
        GetHttpChannel(aChannel, getter_AddRefs(httpChannel));

    if (httpChannel) {
        
        bool discardLayoutState = ShouldDiscardLayoutState(httpChannel);       
        if (mLSHE && discardLayoutState && (mLoadType & LOAD_CMD_NORMAL) &&
            (mLoadType != LOAD_BYPASS_HISTORY) && (mLoadType != LOAD_ERROR_PAGE))
            mLSHE->SetSaveLayoutStateFlag(false);            
    }

    
    
    
    if (mLSHE) {
        mLSHE->SetLoadType(nsIDocShellLoadInfo::loadHistory);

        
        
        SetHistoryEntry(&mLSHE, nullptr);
    }
    
    
    RefreshURIFromQueue();

    
    bool isTopFrame = true;
    nsCOMPtr<nsIDocShellTreeItem> targetParentTreeItem;
    rv = GetSameTypeParent(getter_AddRefs(targetParentTreeItem));
    if (NS_SUCCEEDED(rv) && targetParentTreeItem) {
        isTopFrame = false;
    }

    
    
    
    
    
    
    
    
    
    
    if (url && NS_FAILED(aStatus)) {
        if (aStatus == NS_ERROR_FILE_NOT_FOUND ||
            aStatus == NS_ERROR_CORRUPTED_CONTENT ||
            aStatus == NS_ERROR_INVALID_CONTENT_ENCODING) {
            DisplayLoadError(aStatus, url, nullptr, aChannel);
            return NS_OK;
        }

        if (sURIFixup) {
            
            
            
            nsCOMPtr<nsIURI> newURI;

            nsAutoCString oldSpec;
            url->GetSpec(oldSpec);
      
            
            
            
            if (aStatus == NS_ERROR_UNKNOWN_HOST && mAllowKeywordFixup) {
                bool keywordsEnabled =
                    Preferences::GetBool("keyword.enabled", false);

                nsAutoCString host;
                url->GetHost(host);

                nsAutoCString scheme;
                url->GetScheme(scheme);

                int32_t dotLoc = host.FindChar('.');

                
                
                
                
                
                
                
                
                
                
                
                
                if (keywordsEnabled && !scheme.IsEmpty() &&
                    (scheme.Find("http") != 0)) {
                    keywordsEnabled = false;
                }

                if (keywordsEnabled && (kNotFound == dotLoc)) {
                    
                    if (!mOriginalUriString.IsEmpty()) {
                        sURIFixup->KeywordToURI(mOriginalUriString,
                                                getter_AddRefs(newURI));
                    }
                    else {
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        
                        bool isACE;
                        nsAutoCString utf8Host;
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
            }

            
            
            
            
            if (aStatus == NS_ERROR_UNKNOWN_HOST ||
                aStatus == NS_ERROR_NET_RESET) {
                bool doCreateAlternate = true;

                
                
        
                if (mLoadType != LOAD_NORMAL || !isTopFrame) {
                    doCreateAlternate = false;
                }
                else {
                    
                    if (newURI) {
                        bool sameURI = false;
                        url->Equals(newURI, &sameURI);
                        if (!sameURI) {
                            
                            
                            doCreateAlternate = false;
                        }
                    }
                }
                if (doCreateAlternate) {
                    newURI = nullptr;
                    sURIFixup->CreateFixupURI(oldSpec,
                      nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI,
                                              getter_AddRefs(newURI));
                }
            }

            
            
            
            if (newURI) {
                
                
                bool sameURI = false;
                url->Equals(newURI, &sameURI);
                if (!sameURI) {
                    nsAutoCString newSpec;
                    newURI->GetSpec(newSpec);
                    NS_ConvertUTF8toUTF16 newSpecW(newSpec);

                    return LoadURI(newSpecW.get(),  
                                   LOAD_FLAGS_NONE, 
                                   nullptr,          
                                   nullptr,          
                                   nullptr);         
                }
            }
        }

        
        

        
        if ((aStatus == NS_ERROR_UNKNOWN_HOST || 
             aStatus == NS_ERROR_CONNECTION_REFUSED ||
             aStatus == NS_ERROR_UNKNOWN_PROXY_HOST || 
             aStatus == NS_ERROR_PROXY_CONNECTION_REFUSED) &&
            (isTopFrame || mUseErrorPages)) {
            DisplayLoadError(aStatus, url, nullptr, aChannel);
        }
        
        else if (aStatus == NS_ERROR_NET_TIMEOUT ||
                 aStatus == NS_ERROR_REDIRECT_LOOP ||
                 aStatus == NS_ERROR_UNKNOWN_SOCKET_TYPE ||
                 aStatus == NS_ERROR_NET_INTERRUPT ||
                 aStatus == NS_ERROR_NET_RESET ||
                 aStatus == NS_ERROR_OFFLINE ||
                 aStatus == NS_ERROR_MALWARE_URI ||
                 aStatus == NS_ERROR_PHISHING_URI ||
                 aStatus == NS_ERROR_UNSAFE_CONTENT_TYPE ||
                 aStatus == NS_ERROR_REMOTE_XUL ||
                 aStatus == NS_ERROR_OFFLINE ||
                 NS_ERROR_GET_MODULE(aStatus) == NS_ERROR_MODULE_SECURITY) {
            DisplayLoadError(aStatus, url, nullptr, aChannel);
        }
        else if (aStatus == NS_ERROR_DOCUMENT_NOT_CACHED) {
            
            
            
            if (!(mLoadType & LOAD_CMD_HISTORY))
                aStatus = NS_ERROR_OFFLINE;
            DisplayLoadError(aStatus, url, nullptr, aChannel);
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

    nsCOMPtr<nsIURI> baseURI;
    nsIPrincipal* principal = GetInheritedPrincipal(false);
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

    nsresult rv = CreateAboutBlankContentViewer(principal, baseURI);

    if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(this)));
        NS_ASSERTION(doc,
                     "Should have doc if CreateAboutBlankContentViewer "
                     "succeeded!");

        doc->SetIsInitialDocument(true);
    }

    return rv;
}

nsresult
nsDocShell::CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal,
                                          nsIURI* aBaseURI,
                                          bool aTryToSaveOldPresentation)
{
  nsCOMPtr<nsIDocument> blankDoc;
  nsCOMPtr<nsIContentViewer> viewer;
  nsresult rv = NS_ERROR_FAILURE;

  


  NS_ASSERTION(!mCreatingDocument, "infinite(?) loop creating document averted");
  if (mCreatingDocument)
    return NS_ERROR_FAILURE;

  mCreatingDocument = true;

  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);
  
  if (mContentViewer) {
    
    
    
    

    
    
    rv = MaybeInitTiming();
    if (mTiming) {
      mTiming->NotifyBeforeUnload();
    }

    bool okToUnload;
    rv = mContentViewer->PermitUnload(false, &okToUnload);

    if (NS_SUCCEEDED(rv) && !okToUnload) {
      
      return NS_ERROR_FAILURE;
    }

    mSavingOldViewer = aTryToSaveOldPresentation && 
                       CanSavePresentation(LOAD_NORMAL, nullptr, nullptr);

    if (mTiming) {
      mTiming->NotifyUnloadAccepted(mCurrentURI);
    }

    
    
    mLoadingURI = nullptr;
    
    
    
    Stop();

    
    
    
    
    
    
    (void) FirePageHideNotification(!mSavingOldViewer);
  }

  
  
  
  
  mFiredUnloadEvent = false;

  nsCOMPtr<nsIDocumentLoaderFactory> docFactory =
      nsContentUtils::FindInternalContentViewer("text/html");

  if (docFactory) {
    nsCOMPtr<nsIPrincipal> principal;
    if (mSandboxFlags & SANDBOXED_ORIGIN) {
      principal = do_CreateInstance("@mozilla.org/nullprincipal;1");
    } else {
      principal = aPrincipal;
    }
    
    docFactory->CreateBlankDocument(mLoadGroup, principal,
                                    getter_AddRefs(blankDoc));
    if (blankDoc) {
      
      
      blankDoc->SetBaseURI(aBaseURI);

      blankDoc->SetContainer(static_cast<nsIDocShell *>(this));

      
      
      blankDoc->SetSandboxFlags(mSandboxFlags);

      
      docFactory->CreateInstanceForDocument(NS_ISUPPORTS_CAST(nsIDocShell *, this),
                    blankDoc, "view", getter_AddRefs(viewer));

      
      if (viewer) {
        viewer->SetContainer(static_cast<nsIContentViewerContainer *>(this));
        Embed(viewer, "", 0);

        SetCurrentURI(blankDoc->GetDocumentURI(), nullptr, true, 0);
        rv = mIsBeingDestroyed ? NS_ERROR_NOT_AVAILABLE : NS_OK;
      }
    }
  }
  mCreatingDocument = false;

  
  SetHistoryEntry(&mOSHE, nullptr);

  return rv;
}

NS_IMETHODIMP
nsDocShell::CreateAboutBlankContentViewer(nsIPrincipal *aPrincipal)
{
    return CreateAboutBlankContentViewer(aPrincipal, nullptr);
}

bool
nsDocShell::CanSavePresentation(uint32_t aLoadType,
                                nsIRequest *aNewRequest,
                                nsIDocument *aNewDocument)
{
    if (!mOSHE)
        return false; 

    nsCOMPtr<nsIContentViewer> viewer;
    mOSHE->GetContentViewer(getter_AddRefs(viewer));
    if (viewer) {
        NS_WARNING("mOSHE already has a content viewer!");
        return false;
    }

    
    
    
    if (aLoadType != LOAD_NORMAL &&
        aLoadType != LOAD_HISTORY &&
        aLoadType != LOAD_LINK &&
        aLoadType != LOAD_STOP_CONTENT &&
        aLoadType != LOAD_STOP_CONTENT_AND_REPLACE &&
        aLoadType != LOAD_ERROR_PAGE)
        return false;

    
    
    bool canSaveState;
    mOSHE->GetSaveLayoutStateFlag(&canSaveState);
    if (!canSaveState)
        return false;

    
    nsCOMPtr<nsPIDOMWindow> pWin = do_QueryInterface(mScriptGlobal);
    if (!pWin || pWin->IsLoading())
        return false;

    if (pWin->WouldReuseInnerWindow(aNewDocument))
        return false;

    
    
    if (nsSHistory::GetMaxTotalViewers() == 0)
        return false;

    
    
    bool cacheFrames =
        Preferences::GetBool("browser.sessionhistory.cache_subframes",
                             false);
    if (!cacheFrames) {
        nsCOMPtr<nsIDocShellTreeItem> root;
        GetSameTypeParent(getter_AddRefs(root));
        if (root && root != this) {
            return false;  
        }
    }

    
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(pWin->GetExtantDocument());
    if (!doc || !doc->CanSavePresentation(aNewRequest))
        return false;

    return true;
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
#ifdef DEBUG
        nsresult rv =
#endif
        mEditorData->ReattachToWindow(this);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to reattach editing session");
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
            mEditorData = nullptr;
    }

#ifdef DEBUG
    {
        bool isEditable;
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

    nsCOMPtr<nsISupports> windowState = privWin->SaveWindowState();
    NS_ENSURE_TRUE(windowState, NS_ERROR_FAILURE);

#ifdef DEBUG_PAGE_CACHE
    nsCOMPtr<nsIURI> uri;
    mOSHE->GetURI(getter_AddRefs(uri));
    nsAutoCString spec;
    if (uri)
        uri->GetSpec(spec);
    printf("Saving presentation into session history\n");
    printf("  SH URI: %s\n", spec.get());
#endif

    nsresult rv = mOSHE->SetWindowState(windowState);
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

    uint32_t childCount = mChildList.Length();
    for (uint32_t i = 0; i < childCount; ++i) {
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
nsDocShell::BeginRestore(nsIContentViewer *aContentViewer, bool aTop)
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
            mEODForCurrentDocument = false;
            mIsRestoringDocument = true;
            mLoadGroup->AddRequest(channel, nullptr);
            mIsRestoringDocument = false;
        }
    }

    if (!aTop) {
        
        
        
        
        mFiredUnloadEvent = false;

        
        
        
        
        rv = BeginRestoreChildren();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

nsresult
nsDocShell::BeginRestoreChildren()
{
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child) {
            nsresult rv = child->BeginRestore(nullptr, false);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::FinishRestore()
{
    
    

    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child) {
            child->FinishRestore();
        }
    }

    if (mOSHE && mOSHE->HasDetachedEditor()) {
      ReattachEditorToWindow(mOSHE);
    }

    nsCOMPtr<nsIDocument> doc = do_GetInterface(GetAsSupports(this));
    if (doc) {
        
        
        

        nsIChannel *channel = doc->GetChannel();
        if (channel) {
            mIsRestoringDocument = true;
            mLoadGroup->RemoveRequest(channel, nullptr, NS_OK);
            mIsRestoringDocument = false;
        }
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRestoringDocument(bool *aRestoring)
{
    *aRestoring = mIsRestoringDocument;
    return NS_OK;
}

nsresult
nsDocShell::RestorePresentation(nsISHEntry *aSHEntry, bool *aRestoring)
{
    NS_ASSERTION(mLoadType & LOAD_CMD_HISTORY,
                 "RestorePresentation should only be called for history loads");

    nsCOMPtr<nsIContentViewer> viewer;
    aSHEntry->GetContentViewer(getter_AddRefs(viewer));

#ifdef DEBUG_PAGE_CACHE
    nsCOMPtr<nsIURI> uri;
    aSHEntry->GetURI(getter_AddRefs(uri));

    nsAutoCString spec;
    if (uri)
        uri->GetSpec(spec);
#endif

    *aRestoring = false;

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
        aSHEntry->SetContentViewer(nullptr);
        return NS_ERROR_FAILURE;
    }

    NS_ASSERTION(mContentViewer != viewer, "Restoring existing presentation");

#ifdef DEBUG_PAGE_CACHE
    printf("restoring presentation from session history: %s\n", spec.get());
#endif

    SetHistoryEntry(&mLSHE, aSHEntry);

    
    
    
    
    

    BeginRestore(viewer, true);

    
    
    

    
    NS_ASSERTION(!mRestorePresentationEvent.IsPending(),
        "should only have one RestorePresentationEvent");
    mRestorePresentationEvent.Revoke();

    nsRefPtr<RestorePresentationEvent> evt = new RestorePresentationEvent(this);
    nsresult rv = NS_DispatchToCurrentThread(evt);
    if (NS_SUCCEEDED(rv)) {
        mRestorePresentationEvent = evt.get();
        
        
        *aRestoring = true;
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
        nsIRequest *request = nullptr;
        if (doc)
            request = doc->GetChannel();
        mSavingOldViewer = CanSavePresentation(mLoadType, request, doc);
    }

    nsCOMPtr<nsIMarkupDocumentViewer> oldMUDV(
        do_QueryInterface(mContentViewer));
    nsCOMPtr<nsIMarkupDocumentViewer> newMUDV(
        do_QueryInterface(viewer));
    int32_t minFontSize = 0;
    float textZoom = 1.0f;
    float pageZoom = 1.0f;
    bool styleDisabled = false;
    if (oldMUDV && newMUDV) {
        oldMUDV->GetMinFontSize(&minFontSize);
        oldMUDV->GetTextZoom(&textZoom);
        oldMUDV->GetFullZoom(&pageZoom);
        oldMUDV->GetAuthorStyleDisabled(&styleDisabled);
    }

    
    
    nsCOMPtr<nsISHEntry> origLSHE = mLSHE;

    
    
    mLoadingURI = nullptr;

    
    FirePageHideNotification(!mSavingOldViewer);

    
    
    if (mLSHE != origLSHE)
      return NS_OK;

    
    
    mFiredUnloadEvent = false;

    mURIResultedInDocument = true;
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
            mSavingOldViewer = false;
        }
    }

    mSavedRefreshURIList = nullptr;

    
    
    
    
    

    if (mContentViewer) {
        nsCOMPtr<nsIContentViewer> previousViewer;
        mContentViewer->GetPreviousViewer(getter_AddRefs(previousViewer));
        if (previousViewer) {
            mContentViewer->SetPreviousViewer(nullptr);
            previousViewer->Destroy();
        }
    }

    
    
    

    nsView *rootViewSibling = nullptr, *rootViewParent = nullptr;
    nsIntRect newBounds(0, 0, 0, 0);

    nsCOMPtr<nsIPresShell> oldPresShell = GetPresShell();
    if (oldPresShell) {
        nsViewManager *vm = oldPresShell->GetViewManager();
        if (vm) {
            nsView *oldRootView = vm->GetRootView();

            if (oldRootView) {
                rootViewSibling = oldRootView->GetNextSibling();
                rootViewParent = oldRootView->GetParent();

                mContentViewer->GetBounds(newBounds);
            }
        }
    }

    nsCOMPtr<nsIContent> container;
    nsCOMPtr<nsIDocument> sibling;
    if (rootViewParent && rootViewParent->GetParent()) {
        nsIFrame* frame = rootViewParent->GetParent()->GetFrame();
        container = frame ? frame->GetContent() : nullptr;
    }
    if (rootViewSibling) {
        nsIFrame *frame = rootViewSibling->GetFrame();
        sibling = frame ? frame->PresContext()->PresShell()->GetDocument() : nullptr;
    }

    
    
    
    

    if (mContentViewer) {
        mContentViewer->Close(mSavingOldViewer ? mOSHE.get() : nullptr);
        viewer->SetPreviousViewer(mContentViewer);
    }
    if (mOSHE && (!mContentViewer || !mSavingOldViewer)) {
        
        
        mOSHE->SyncPresentationState();
    }

    
    mContentViewer = nullptr;

    
    
    DestroyChildren();

    mContentViewer.swap(viewer);

    
    
    nsCOMPtr<nsISupports> windowState;
    mLSHE->GetWindowState(getter_AddRefs(windowState));
    mLSHE->SetWindowState(nullptr);

    bool sticky;
    mLSHE->GetSticky(&sticky);

    nsCOMPtr<nsIDOMDocument> domDoc;
    mContentViewer->GetDOMDocument(getter_AddRefs(domDoc));

    nsCOMArray<nsIDocShellTreeItem> childShells;
    int32_t i = 0;
    nsCOMPtr<nsIDocShellTreeItem> child;
    while (NS_SUCCEEDED(mLSHE->ChildShellAt(i++, getter_AddRefs(child))) &&
           child) {
        childShells.AppendObject(child);
    }

    
    nsIntRect oldBounds(0, 0, 0, 0);
    mLSHE->GetViewerBounds(oldBounds);

    
    
    nsCOMPtr<nsISupportsArray> refreshURIList;
    mLSHE->GetRefreshURIList(getter_AddRefs(refreshURIList));

    
    mIsRestoringDocument = true; 
    rv = mContentViewer->Open(windowState, mLSHE);
    mIsRestoringDocument = false;

    
    
    nsAutoPtr<nsDocShellEditorData> data(mLSHE->ForgetEditorData());

    
    mLSHE->SetContentViewer(nullptr);
    mEODForCurrentDocument = false;

    mLSHE->SetEditorData(data.forget());

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

    NS_ENSURE_SUCCESS(rv, rv);

    
    SetHistoryEntry(&mOSHE, mLSHE);

    

    
    
    SetLayoutHistoryState(nullptr);

    

    mSavingOldViewer = false;
    mEODForCurrentDocument = false;

    
    
    if (++gNumberOfDocumentsLoading == 1)
        FavorPerformanceHint(true, NS_EVENT_STARVATION_DELAY_HINT);


    if (oldMUDV && newMUDV) {
        newMUDV->SetMinFontSize(minFontSize);
        newMUDV->SetTextZoom(textZoom);
        newMUDV->SetFullZoom(pageZoom);
        newMUDV->SetAuthorStyleDisabled(styleDisabled);
    }

    nsCOMPtr<nsIDocument> document = do_QueryInterface(domDoc);
    uint32_t parentSuspendCount = 0;
    if (document) {
        nsCOMPtr<nsIDocShellTreeItem> parent;
        GetParent(getter_AddRefs(parent));
        nsCOMPtr<nsIDocument> d = do_GetInterface(parent);
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
        SetCurrentURI(uri, document->GetChannel(), true, 0);
    }

    
    
    
    nsCOMPtr<nsPIDOMWindow> privWin =
        do_GetInterface(static_cast<nsIInterfaceRequestor*>(this));
    NS_ASSERTION(privWin, "could not get nsPIDOMWindow interface");

    rv = privWin->RestoreWindowState(windowState);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    document->NotifyPossibleTitleChange(false);

    
    for (i = 0; i < childShells.Count(); ++i) {
        nsIDocShellTreeItem *childItem = childShells.ObjectAt(i);
        nsCOMPtr<nsIDocShell> childShell = do_QueryInterface(childItem);

        
        
        bool allowPlugins;
        childShell->GetAllowPlugins(&allowPlugins);

        bool allowJavascript;
        childShell->GetAllowJavascript(&allowJavascript);

        bool allowRedirects;
        childShell->GetAllowMetaRedirects(&allowRedirects);

        bool allowSubframes;
        childShell->GetAllowSubframes(&allowSubframes);

        bool allowImages;
        childShell->GetAllowImages(&allowImages);

        bool allowDNSPrefetch;
        childShell->GetAllowDNSPrefetch(&allowDNSPrefetch);

        
        
        
        
        AddChild(childItem);

        childShell->SetAllowPlugins(allowPlugins);
        childShell->SetAllowJavascript(allowJavascript);
        childShell->SetAllowMetaRedirects(allowRedirects);
        childShell->SetAllowSubframes(allowSubframes);
        childShell->SetAllowImages(allowImages);
        childShell->SetAllowDNSPrefetch(allowDNSPrefetch);

        rv = childShell->BeginRestore(nullptr, false);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIPresShell> shell = GetPresShell();

    nsViewManager *newVM = shell ? shell->GetViewManager() : nullptr;
    nsView *newRootView = newVM ? newVM->GetRootView() : nullptr;

    
    if (container) {
        nsSubDocumentFrame* subDocFrame = do_QueryFrame(container->GetPrimaryFrame());
        rootViewParent = subDocFrame ? subDocFrame->EnsureInnerView() : nullptr;
    }
    if (sibling &&
        sibling->GetShell() &&
        sibling->GetShell()->GetViewManager()) {
        rootViewSibling = sibling->GetShell()->GetViewManager()->GetRootView();
    } else {
        rootViewSibling = nullptr;
    }
    if (rootViewParent && newRootView && newRootView->GetParent() != rootViewParent) {
        nsViewManager *parentVM = rootViewParent->GetViewManager();
        if (parentVM) {
            
            
            
            
            
            
            
            parentVM->InsertChild(rootViewParent, newRootView,
                                  rootViewSibling,
                                  rootViewSibling ? true : false);

            NS_ASSERTION(newRootView->GetNextSibling() == rootViewSibling,
                         "error in InsertChild");
        }
    }

    
    
    
    if (parentSuspendCount) {
      privWin->SuspendTimeouts(parentSuspendCount, false);
    }

    
    
    privWin->ResumeTimeouts();

    
    
    mRefreshURIList = refreshURIList;

    
    
    uint32_t n = mChildList.Length();
    for (uint32_t i = 0; i < n; ++i) {
        nsCOMPtr<nsIDocShell> child = do_QueryInterface(ChildAt(i));
        if (child)
            child->ResumeRefreshURIs();
    }

    
    
    

    
    
    
    
    
    
    
    

    if (newRootView) {
        if (!newBounds.IsEmpty() && !newBounds.IsEqualEdges(oldBounds)) {
#ifdef DEBUG_PAGE_CACHE
            printf("resize widget(%d, %d, %d, %d)\n", newBounds.x,
                   newBounds.y, newBounds.width, newBounds.height);
#endif
            mContentViewer->SetBounds(newBounds);
        } else {
            nsIScrollableFrame *rootScrollFrame =
              shell->GetRootScrollFrameAsScrollableExternal();
            if (rootScrollFrame) {
                rootScrollFrame->PostScrolledAreaEventForCurrentArea();
            }
        }
    }

    
    
    newRootView = rootViewSibling = rootViewParent = nullptr;
    newVM = nullptr;

    
    nsDocShell::FinishRestore();

    
    if (shell) {
        shell->Thaw();

        newVM = shell->GetViewManager();
        if (newVM) {
            
            
            
            newRootView = newVM->GetRootView();
            if (newRootView) {
                newVM->InvalidateView(newRootView);
            }
        }
    }

    return privWin->FireDelayedDOMEvents();
}

NS_IMETHODIMP
nsDocShell::CreateContentViewer(const char *aContentType,
                                nsIRequest * request,
                                nsIStreamListener ** aContentHandler)
{
    *aContentHandler = nullptr;

    
    

    NS_ASSERTION(mLoadGroup, "Someone ignored return from Init()?");

    
    nsCOMPtr<nsIContentViewer> viewer;
    nsresult rv = NewContentViewerObj(aContentType, request, mLoadGroup,
                                      aContentHandler, getter_AddRefs(viewer));

    if (NS_FAILED(rv))
        return rv;

    
    
    
    
    
    

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
    mLoadingURI = nullptr;

    
    
    mFiredUnloadEvent = false;

    
    
    
    mURIResultedInDocument = true;

    if (mLoadType == LOAD_ERROR_PAGE) {
        
        
        

        
        
        mLoadType = mFailedLoadType;

        nsCOMPtr<nsIChannel> failedChannel = mFailedChannel;

        
        nsCOMPtr<nsIURI> failedURI;
        if (failedChannel) {
            NS_GetFinalChannelURI(failedChannel, getter_AddRefs(failedURI));
        }

        if (!failedURI) {
            failedURI = mFailedURI;
        }

        
        
        MOZ_ASSERT(failedURI, "We don't have a URI for history APIs.");

        mFailedChannel = nullptr;
        mFailedURI = nullptr;

        
        if (failedURI) {
            bool errorOnLocationChangeNeeded =
                OnNewURI(failedURI, failedChannel, nullptr, mLoadType, false,
                         false, false);

            if (errorOnLocationChangeNeeded) {
                FireOnLocationChange(this, failedChannel, failedURI,
                                     LOCATION_CHANGE_ERROR_PAGE);
            }
        }

        
        
        if (mSessionHistory && !mLSHE) {
            int32_t idx;
            mSessionHistory->GetRequestedIndex(&idx);
            if (idx == -1)
                mSessionHistory->GetIndex(&idx);

            nsCOMPtr<nsIHistoryEntry> entry;
            mSessionHistory->GetEntryAtIndex(idx, false,
                                             getter_AddRefs(entry));
            mLSHE = do_QueryInterface(entry);
        }

        mLoadType = LOAD_ERROR_PAGE;
    }

    bool onLocationChangeNeeded = OnLoadingSite(aOpenedChannel, false);

    
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

        mLoadGroup->AddRequest(request, nullptr);
        if (currentLoadGroup)
            currentLoadGroup->RemoveRequest(request, nullptr,
                                            NS_BINDING_RETARGETED);

        
        
        aOpenedChannel->SetNotificationCallbacks(this);
    }

    NS_ENSURE_SUCCESS(Embed(viewer, "", (nsISupports *) nullptr),
                      NS_ERROR_FAILURE);

    mSavedRefreshURIList = nullptr;
    mSavingOldViewer = false;
    mEODForCurrentDocument = false;

    
    
    nsCOMPtr<nsIMultiPartChannel> multiPartChannel(do_QueryInterface(request));
    if (multiPartChannel) {
      nsCOMPtr<nsIPresShell> shell = GetPresShell();
      if (NS_SUCCEEDED(rv) && shell) {
        nsIDocument *doc = shell->GetDocument();
        if (doc) {
          uint32_t partID;
          multiPartChannel->GetPartID(&partID);
          doc->SetPartID(partID);
        }
      }
    }

    
    
    
    if (++gNumberOfDocumentsLoading == 1) {
      
      
      
      FavorPerformanceHint(true, NS_EVENT_STARVATION_DELAY_HINT);
    }

    if (onLocationChangeNeeded) {
      FireOnLocationChange(this, request, mCurrentURI, 0);
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

    nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
        nsContentUtils::FindInternalContentViewer(aContentType);
    if (!docLoaderFactory) {
        return NS_ERROR_FAILURE;
    }

    
    
    nsresult rv = docLoaderFactory->CreateInstance("view",
                                                   aOpenedChannel,
                                                   aLoadGroup, aContentType,
                                                   static_cast<nsIContentViewerContainer*>(this),
                                                   nullptr,
                                                   aContentHandler,
                                                   aViewer);
    NS_ENSURE_SUCCESS(rv, rv);

    (*aViewer)->SetContainer(static_cast<nsIContentViewerContainer *>(this));
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetupNewViewer(nsIContentViewer * aNewViewer)
{
    
    
    
    
    
    
    
    
    
    
    
    
    

    int32_t x = 0;
    int32_t y = 0;
    int32_t cx = 0;
    int32_t cy = 0;

    
    
    DoGetPositionAndSize(&x, &y, &cx, &cy);

    nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
    NS_ENSURE_SUCCESS(GetSameTypeParent(getter_AddRefs(parentAsItem)),
                      NS_ERROR_FAILURE);
    nsCOMPtr<nsIDocShell> parent(do_QueryInterface(parentAsItem));

    nsAutoCString defaultCharset;
    nsAutoCString forceCharset;
    nsAutoCString hintCharset;
    int32_t hintCharsetSource;
    nsAutoCString prevDocCharset;
    int32_t minFontSize;
    float textZoom;
    float pageZoom;
    bool styleDisabled;
    
    nsCOMPtr<nsIMarkupDocumentViewer> newMUDV;

    if (mContentViewer || parent) {
        nsCOMPtr<nsIMarkupDocumentViewer> oldMUDV;
        if (mContentViewer) {
            
            
            
            oldMUDV = do_QueryInterface(mContentViewer);

            
            

            if (mSavingOldViewer && NS_FAILED(CaptureState())) {
                if (mOSHE) {
                    mOSHE->SyncPresentationState();
                }
                mSavingOldViewer = false;
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
                                  GetMinFontSize(&minFontSize),
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

        
        
        nsCOMPtr<nsIPresShell> shell;
        mContentViewer->GetPresShell(getter_AddRefs(shell));

        if (shell) {
            bgcolor = shell->GetCanvasBackground();
        }

        mContentViewer->Close(mSavingOldViewer ? mOSHE.get() : nullptr);
        aNewViewer->SetPreviousViewer(mContentViewer);
    }
    if (mOSHE && (!mContentViewer || !mSavingOldViewer)) {
        
        
        mOSHE->SyncPresentationState();
    }

    mContentViewer = nullptr;

    
    
    DestroyChildren();

    mContentViewer = aNewViewer;

    nsCOMPtr<nsIWidget> widget;
    NS_ENSURE_SUCCESS(GetMainWidget(getter_AddRefs(widget)), NS_ERROR_FAILURE);

    nsIntRect bounds(x, y, cx, cy);

    mContentViewer->SetNavigationTiming(mTiming);

    if (NS_FAILED(mContentViewer->Init(widget, bounds))) {
        mContentViewer = nullptr;
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
        NS_ENSURE_SUCCESS(newMUDV->SetMinFontSize(minFontSize),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetTextZoom(textZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetFullZoom(pageZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(newMUDV->SetAuthorStyleDisabled(styleDisabled),
                          NS_ERROR_FAILURE);
    }

    
    
    nsCOMPtr<nsIPresShell> shell;
    mContentViewer->GetPresShell(getter_AddRefs(shell));

    if (shell) {
        shell->SetCanvasBackground(bgcolor);
    }




    
    
    

    return NS_OK;
}

nsresult
nsDocShell::SetDocCurrentStateObj(nsISHEntry *shEntry)
{
    nsCOMPtr<nsIDocument> document = do_GetInterface(GetAsSupports(this));
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

    nsCOMPtr<nsIStructuredCloneContainer> scContainer;
    if (shEntry) {
        nsresult rv = shEntry->GetStateData(getter_AddRefs(scContainer));
        NS_ENSURE_SUCCESS(rv, rv);

        
    }

    
    
    document->SetStateObject(scContainer);

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

        
        bool subsumes;
        sameOrigin = subjPrincipal->Subsumes(p, &subsumes);
        if (NS_SUCCEEDED(sameOrigin)) {
            if (subsumes) {
                

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




namespace
{

#ifdef MOZ_PLACES


class nsCopyFaviconCallback MOZ_FINAL : public nsIFaviconDataCallback
{
public:
    NS_DECL_ISUPPORTS

    nsCopyFaviconCallback(nsIURI *aNewURI, bool aInPrivateBrowsing)
      : mNewURI(aNewURI)
      , mInPrivateBrowsing(aInPrivateBrowsing)
    {
    }

    NS_IMETHODIMP
    OnComplete(nsIURI *aFaviconURI, uint32_t aDataLen,
               const uint8_t *aData, const nsACString &aMimeType)
    {
        
        if (!aFaviconURI) {
          return NS_OK;
        }

        NS_ASSERTION(aDataLen == 0,
                     "We weren't expecting the callback to deliver data.");
        nsCOMPtr<mozIAsyncFavicons> favSvc =
            do_GetService("@mozilla.org/browser/favicon-service;1");
        NS_ENSURE_STATE(favSvc);

        return favSvc->SetAndFetchFaviconForPage(mNewURI, aFaviconURI,
                                                 false,
                                                 mInPrivateBrowsing ?
                                                   nsIFaviconService::FAVICON_LOAD_PRIVATE :
                                                   nsIFaviconService::FAVICON_LOAD_NON_PRIVATE,
                                                 nullptr);
    }

private:
    nsCOMPtr<nsIURI> mNewURI;
    bool mInPrivateBrowsing;
};

NS_IMPL_ISUPPORTS1(nsCopyFaviconCallback, nsIFaviconDataCallback)
#endif


void CopyFavicon(nsIURI *aOldURI, nsIURI *aNewURI, bool inPrivateBrowsing)
{
#ifdef MOZ_PLACES
    nsCOMPtr<mozIAsyncFavicons> favSvc =
        do_GetService("@mozilla.org/browser/favicon-service;1");
    if (favSvc) {
        nsCOMPtr<nsIFaviconDataCallback> callback =
            new nsCopyFaviconCallback(aNewURI, inPrivateBrowsing);
        favSvc->GetFaviconURLForPage(aOldURI, callback);
    }
#endif
}

} 

class InternalLoadEvent : public nsRunnable
{
public:
    InternalLoadEvent(nsDocShell* aDocShell, nsIURI * aURI, nsIURI * aReferrer,
                      nsISupports * aOwner, uint32_t aFlags,
                      const char* aTypeHint, nsIInputStream * aPostData,
                      nsIInputStream * aHeadersData, uint32_t aLoadType,
                      nsISHEntry * aSHEntry, bool aFirstParty) :
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
                                       nullptr, mTypeHint.get(),
                                       NullString(), mPostData, mHeadersData,
                                       mLoadType, mSHEntry, mFirstParty,
                                       nullptr, nullptr);
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
    uint32_t mFlags;
    uint32_t mLoadType;
    bool mFirstParty;
};











bool
nsDocShell::JustStartedNetworkLoad()
{
    return mDocumentRequest &&
           mDocumentRequest != GetCurrentDocChannel();
}

NS_IMETHODIMP
nsDocShell::InternalLoad(nsIURI * aURI,
                         nsIURI * aReferrer,
                         nsISupports * aOwner,
                         uint32_t aFlags,
                         const PRUnichar *aWindowTarget,
                         const char* aTypeHint,
                         const nsAString& aFileName,
                         nsIInputStream * aPostData,
                         nsIInputStream * aHeadersData,
                         uint32_t aLoadType,
                         nsISHEntry * aSHEntry,
                         bool aFirstParty,
                         nsIDocShell** aDocShell,
                         nsIRequest** aRequest)
{
    nsresult rv = NS_OK;
    mOriginalUriString.Truncate();

#ifdef PR_LOGGING
    if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
        nsAutoCString spec;
        if (aURI)
            aURI->GetSpec(spec);
        PR_LogPrint("DOCSHELL %p InternalLoad %s\n", this, spec.get());
    }
#endif
    
    if (aDocShell) {
        *aDocShell = nullptr;
    }
    if (aRequest) {
        *aRequest = nullptr;
    }

    if (!aURI) {
        return NS_ERROR_NULL_POINTER;
    }

    NS_ENSURE_TRUE(IsValidLoadType(aLoadType), NS_ERROR_INVALID_ARG);

    NS_ENSURE_TRUE(!mIsBeingDestroyed, NS_ERROR_NOT_AVAILABLE);

    
    
    if (aLoadType & LOAD_CMD_NORMAL) {
        bool isWyciwyg = false;
        rv = aURI->SchemeIs("wyciwyg", &isWyciwyg);   
        if ((isWyciwyg && NS_SUCCEEDED(rv)) || NS_FAILED(rv)) 
            return NS_ERROR_FAILURE;
    }

    bool bIsJavascript = false;
    if (NS_FAILED(aURI->SchemeIs("javascript", &bIsJavascript))) {
        bIsJavascript = false;
    }

    
    
    
    
    nsCOMPtr<nsIDOMElement> requestingElement;
    
    nsCOMPtr<nsPIDOMWindow> privateWin(do_QueryInterface(mScriptGlobal));
    if (privateWin)
        requestingElement = privateWin->GetFrameElementInternal();

    int16_t shouldLoad = nsIContentPolicy::ACCEPT;
    uint32_t contentType;
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

    
    nsCOMPtr<nsIPrincipal> loadingPrincipal = do_QueryInterface(aOwner);
    if (!loadingPrincipal && aReferrer) {
        nsCOMPtr<nsIScriptSecurityManager> secMan =
            do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = secMan->GetSimpleCodebasePrincipal(aReferrer,
                                                getter_AddRefs(loadingPrincipal));
    }

    rv = NS_CheckContentLoadPolicy(contentType,
                                   aURI,
                                   loadingPrincipal,
                                   context,
                                   EmptyCString(), 
                                   nullptr,         
                                   &shouldLoad);

    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
        if (NS_SUCCEEDED(rv) && shouldLoad == nsIContentPolicy::REJECT_TYPE) {
            return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
        }

        return NS_ERROR_CONTENT_BLOCKED;
    }

    nsCOMPtr<nsISupports> owner(aOwner);
    
    
    
    
    
    
    
    
    
    
    {
        bool inherits;
        
        if (aLoadType != LOAD_NORMAL_EXTERNAL && !owner &&
            (aFlags & INTERNAL_LOAD_FLAGS_INHERIT_OWNER) &&
            NS_SUCCEEDED(nsContentUtils::URIInheritsSecurityContext(aURI,
                                                                    &inherits)) &&
            inherits) {

            owner = GetInheritedPrincipal(true);
        }
    }

    
    
    {
        bool willInherit;
        
        
        
        
        
        rv = nsContentUtils::URIInheritsSecurityContext(aURI, &willInherit);
        if (NS_FAILED(rv) || willInherit || NS_IsAboutBlank(aURI)) {
            nsCOMPtr<nsIDocShellTreeItem> treeItem = this;
            do {
                nsCOMPtr<nsIDocShell> itemDocShell =
                    do_QueryInterface(treeItem);
                bool isUnsafe;
                if (itemDocShell &&
                    NS_SUCCEEDED(itemDocShell->GetChannelIsUnsafe(&isUnsafe)) &&
                    isUnsafe) {
                    return NS_ERROR_DOM_SECURITY_ERR;
                }

                nsCOMPtr<nsIDocShellTreeItem> parent;
                treeItem->GetSameTypeParent(getter_AddRefs(parent));
                parent.swap(treeItem);
            } while (treeItem);
        }
    }
    
    
    
    
    
    
    if (aWindowTarget && *aWindowTarget) {
        
        
        
        aFlags = aFlags & ~INTERNAL_LOAD_FLAGS_INHERIT_OWNER;
        
        
        
        
        nsCOMPtr<nsIDocShellTreeItem> targetItem;
        FindItemWithName(aWindowTarget, nullptr, this,
                         getter_AddRefs(targetItem));

        nsCOMPtr<nsIDocShell> targetDocShell = do_QueryInterface(targetItem);
        
        bool isNewWindow = false;
        if (!targetDocShell) {
            
            
            
            
            
            NS_ENSURE_TRUE(mContentViewer, NS_ERROR_FAILURE);
            nsIDocument* doc = mContentViewer->GetDocument();
            uint32_t sandboxFlags = 0;

            if (doc) {
                sandboxFlags = doc->GetSandboxFlags();
                if (sandboxFlags & SANDBOXED_NAVIGATION) {
                    return NS_ERROR_FAILURE;
                }
            }

            nsCOMPtr<nsPIDOMWindow> win =
                do_GetInterface(GetAsSupports(this));
            NS_ENSURE_TRUE(win, NS_ERROR_NOT_AVAILABLE);

            nsDependentString name(aWindowTarget);
            nsCOMPtr<nsIDOMWindow> newWin;
            nsAutoCString spec;
            if (aURI)
                aURI->GetSpec(spec);
            rv = win->OpenNoNavigate(NS_ConvertUTF8toUTF16(spec),
                                     name,          
                                     EmptyString(), 
                                     getter_AddRefs(newWin));

            
            
            
            nsCOMPtr<nsPIDOMWindow> piNewWin = do_QueryInterface(newWin);
            if (piNewWin) {
                nsCOMPtr<nsIDocument> newDoc =
                    do_QueryInterface(piNewWin->GetExtantDocument());
                if (!newDoc || newDoc->IsInitialDocument()) {
                    isNewWindow = true;
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
                                              nullptr,         
                                              aTypeHint,
                                              NullString(),    
                                              aPostData,
                                              aHeadersData,
                                              aLoadType,
                                              aSHEntry,
                                              aFirstParty,
                                              aDocShell,
                                              aRequest);
            if (rv == NS_ERROR_NO_CONTENT) {
                
                if (isNewWindow) {
                    
                    
                    
                    
                    
                    
                    
                    nsCOMPtr<nsIDOMWindow> domWin =
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

    NS_ENSURE_STATE(!HasUnloadedParent());

    rv = CheckLoadingPermissions();
    if (NS_FAILED(rv)) {
        return rv;
    }

    
    
    nsCOMPtr<nsIDocShellTreeItem> parent;
    GetParent(getter_AddRefs(parent));
    if (parent) {
      nsCOMPtr<nsIDocument> doc = do_GetInterface(parent);
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
        
        bool isChrome = false;
        if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome) {
            NS_WARNING("blocked external chrome: url -- use '-chrome' option");
            return NS_ERROR_FAILURE;
        }

        
        rv = CreateAboutBlankContentViewer(nullptr, nullptr);
        if (NS_FAILED(rv))
            return NS_ERROR_FAILURE;

        
        
        aLoadType = LOAD_NORMAL;
    }

    mAllowKeywordFixup =
      (aFlags & INTERNAL_LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) != 0;
    mURIResultedInDocument = false;  

    if (aLoadType == LOAD_NORMAL ||
        aLoadType == LOAD_STOP_CONTENT ||
        LOAD_TYPE_HAS_FLAGS(aLoadType, LOAD_FLAGS_REPLACE_HISTORY) ||
        aLoadType == LOAD_HISTORY ||
        aLoadType == LOAD_LINK) {

        
        
        
        nsAutoCString curBeforeHash, curHash, newBeforeHash, newHash;
        nsresult splitRv1, splitRv2;
        splitRv1 = mCurrentURI ?
            nsContentUtils::SplitURIAtHash(mCurrentURI,
                                           curBeforeHash, curHash) :
            NS_ERROR_FAILURE;
        splitRv2 = nsContentUtils::SplitURIAtHash(aURI, newBeforeHash, newHash);

        bool sameExceptHashes = NS_SUCCEEDED(splitRv1) &&
                                  NS_SUCCEEDED(splitRv2) &&
                                  curBeforeHash.Equals(newBeforeHash);

        bool historyNavBetweenSameDoc = false;
        if (mOSHE && aSHEntry) {
            

            mOSHE->SharesDocumentWith(aSHEntry, &historyNavBetweenSameDoc);

#ifdef DEBUG
            if (historyNavBetweenSameDoc) {
                nsCOMPtr<nsIInputStream> currentPostData;
                mOSHE->GetPostData(getter_AddRefs(currentPostData));
                NS_ASSERTION(currentPostData == aPostData,
                             "Different POST data for entries for the same page?");
            }
#endif
        }

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        bool doShortCircuitedLoad =
          (historyNavBetweenSameDoc && mOSHE != aSHEntry) ||
          (!aSHEntry && aPostData == nullptr &&
           sameExceptHashes && !newHash.IsEmpty());

        if (doShortCircuitedLoad) {
            
            
            
            
            
            
            
            
            
            if (aSHEntry && mDocumentRequest) {
                mDocumentRequest->Cancel(NS_BINDING_ABORTED);
            }

            
            nsCOMPtr<nsIURI> oldURI = mCurrentURI;

            
            nscoord cx = 0, cy = 0;
            GetCurScrollPos(ScrollOrientation_X, &cx);
            GetCurScrollPos(ScrollOrientation_Y, &cy);

            
            
            
            
            
            rv = ScrollToAnchor(curHash, newHash, aLoadType);
            NS_ENSURE_SUCCESS(rv, rv);

            
            
            
            
            AutoRestore<uint32_t> loadTypeResetter(mLoadType);

            
            
            
            if (JustStartedNetworkLoad() && (aLoadType & LOAD_CMD_NORMAL)) {
                mLoadType = LOAD_NORMAL_REPLACE;
            }
            else {
                mLoadType = aLoadType;
            }

            mURIResultedInDocument = true;

            




            SetHistoryEntry(&mLSHE, aSHEntry);

            



            nsCOMPtr<nsISupports> owner;
            if (mOSHE) {
                mOSHE->GetOwner(getter_AddRefs(owner));
            }
            
            
            
            
            
            
            
            
            OnNewURI(aURI, nullptr, owner, mLoadType, true, true, true);

            nsCOMPtr<nsIInputStream> postData;
            nsCOMPtr<nsISupports> cacheKey;

            if (mOSHE) {
                
                mOSHE->SetScrollPosition(cx, cy);
                
                
                
                
                
                
                if (aLoadType & LOAD_CMD_NORMAL) {
                    mOSHE->GetPostData(getter_AddRefs(postData));
                    mOSHE->GetCacheKey(getter_AddRefs(cacheKey));

                    
                    
                    
                    if (mLSHE)
                        mLSHE->AdoptBFCacheEntry(mOSHE);
                }
            }

            


            if (mLSHE) {
                SetHistoryEntry(&mOSHE, mLSHE);
                
                
                
                
                if (postData)
                    mOSHE->SetPostData(postData);

                
                
                if (cacheKey)
                    mOSHE->SetCacheKey(cacheKey);
            }

            


            if (mOSHE && (aLoadType == LOAD_HISTORY || aLoadType == LOAD_RELOAD_NORMAL))
            {
                nscoord bx, by;
                mOSHE->GetScrollPosition(&bx, &by);
                SetCurScrollPosEx(bx, by);
            }

            


            SetHistoryEntry(&mLSHE, nullptr);
            


            if (mSessionHistory) {
                int32_t index = -1;
                mSessionHistory->GetIndex(&index);
                nsCOMPtr<nsIHistoryEntry> hEntry;
                mSessionHistory->GetEntryAtIndex(index, false,
                                                 getter_AddRefs(hEntry));
                NS_ENSURE_TRUE(hEntry, NS_ERROR_FAILURE);
                nsCOMPtr<nsISHEntry> shEntry(do_QueryInterface(hEntry));
                if (shEntry)
                    shEntry->SetTitle(mTitle);
            }

            

            if (mUseGlobalHistory && !mInPrivateBrowsing) {
                nsCOMPtr<IHistory> history = services::GetHistoryService();
                if (history) {
                    history->SetURITitle(aURI, mTitle);
                }
                else if (mGlobalHistory) {
                    mGlobalHistory->SetPageTitle(aURI, mTitle);
                }
            }

            
            nsCOMPtr<nsIDocument> doc =
              do_GetInterface(GetAsSupports(this));
            NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
            doc->SetDocumentURI(aURI);

            SetDocCurrentStateObj(mOSHE);

            
            nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mScriptGlobal);
            if (window) {
                
                bool doHashchange = sameExceptHashes && !curHash.Equals(newHash);

                if (historyNavBetweenSameDoc || doHashchange) {
                  window->DispatchSyncPopState();
                }

                if (doHashchange) {
                  
                  
                  window->DispatchAsyncHashchange(oldURI, aURI);
                }
            }

            
            
            CopyFavicon(oldURI, aURI, mInPrivateBrowsing);

            return NS_OK;
        }
    }
    
    
    
    
    
    nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

    
    
    

    
    
    
    
    
    if (!bIsJavascript) {
        MaybeInitTiming();
    }
    bool timeBeforeUnload = aFileName.IsVoid();
    if (mTiming && timeBeforeUnload) {
      mTiming->NotifyBeforeUnload();
    }
    
    
    if (!bIsJavascript && aFileName.IsVoid() && mContentViewer) {
        bool okToUnload;
        rv = mContentViewer->PermitUnload(false, &okToUnload);

        if (NS_SUCCEEDED(rv) && !okToUnload) {
            
            
            return NS_OK;
        }
    }

    if (mTiming && timeBeforeUnload) {
      mTiming->NotifyUnloadAccepted(mCurrentURI);
    }

    
    
    
    
    
    
    bool savePresentation = CanSavePresentation(aLoadType, nullptr, nullptr);

    
    
    
    
    
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
        
        
        aSHEntry->GetDocshellID(&mHistoryID);

        
        
        
        
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
                    
                    mContentViewer->SetPreviousViewer(nullptr);
                    prevViewer->Destroy();
                }
            }
        }
        nsCOMPtr<nsISHEntry> oldEntry = mOSHE;
        bool restoring;
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
                   owner, aTypeHint, aFileName, aPostData, aHeadersData,
                   aFirstParty, aDocShell, getter_AddRefs(req),
                   (aFlags & INTERNAL_LOAD_FLAGS_FIRST_LOAD) != 0,
                   (aFlags & INTERNAL_LOAD_FLAGS_BYPASS_CLASSIFIER) != 0,
                   (aFlags & INTERNAL_LOAD_FLAGS_FORCE_ALLOW_COOKIES) != 0);
    if (req && aRequest)
        NS_ADDREF(*aRequest = req);

    if (NS_FAILED(rv)) {
        nsCOMPtr<nsIChannel> chan(do_QueryInterface(req));
        DisplayLoadError(rv, aURI, nullptr, chan);
    }

    return rv;
}

nsIPrincipal*
nsDocShell::GetInheritedPrincipal(bool aConsiderCurrentDocument)
{
    nsCOMPtr<nsIDocument> document;
    bool inheritedFromCurrent = false;

    if (aConsiderCurrentDocument && mContentViewer) {
        document = mContentViewer->GetDocument();
        inheritedFromCurrent = true;
    }

    if (!document) {
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        GetSameTypeParent(getter_AddRefs(parentItem));
        if (parentItem) {
            document = do_GetInterface(parentItem);
        }
    }

    if (!document) {
        if (!aConsiderCurrentDocument) {
            return nullptr;
        }

        
        
        EnsureContentViewer();  
                                

        if (!mContentViewer)
            return nullptr;
        document = mContentViewer->GetDocument();
    }

    
    if (document) {
        nsIPrincipal *docPrincipal = document->NodePrincipal();

        
        
        if (inheritedFromCurrent &&
            mItemType == typeContent &&
            nsContentUtils::IsSystemPrincipal(docPrincipal)) {
            return nullptr;
        }

        return docPrincipal;
    }

    return nullptr;
}

nsresult
nsDocShell::DoURILoad(nsIURI * aURI,
                      nsIURI * aReferrerURI,
                      bool aSendReferrer,
                      nsISupports * aOwner,
                      const char * aTypeHint,
                      const nsAString & aFileName,
                      nsIInputStream * aPostData,
                      nsIInputStream * aHeadersData,
                      bool aFirstParty,
                      nsIDocShell ** aDocShell,
                      nsIRequest ** aRequest,
                      bool aIsNewWindowTarget,
                      bool aBypassClassifier,
                      bool aForceAllowCookies)
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

    
    
    nsCOMPtr<nsIChannelPolicy> channelPolicy;
    if (IsFrame()) {
        
        nsCOMPtr<nsIContentSecurityPolicy> csp;
        nsCOMPtr<nsIDocShellTreeItem> parentItem;
        GetSameTypeParent(getter_AddRefs(parentItem));
        nsCOMPtr<nsIDocument> doc = do_GetInterface(parentItem);
        if (doc) {
            rv = doc->NodePrincipal()->GetCsp(getter_AddRefs(csp));
            NS_ENSURE_SUCCESS(rv, rv);
            if (csp) {
                channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
                channelPolicy->SetContentSecurityPolicy(csp);
                channelPolicy->SetLoadType(nsIContentPolicy::TYPE_SUBDOCUMENT);
            }
        }
    }

    
    nsCOMPtr<nsIChannel> channel;

    rv = NS_NewChannel(getter_AddRefs(channel),
                       aURI,
                       nullptr,
                       nullptr,
                       static_cast<nsIInterfaceRequestor *>(this),
                       loadFlags,
                       channelPolicy);
    if (NS_FAILED(rv)) {
        if (rv == NS_ERROR_UNKNOWN_PROTOCOL) {
            
            
            
            
            bool abort = false;
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
        
        appCacheChannel->SetInheritApplicationCache(false);

        
        
        if (GeckoProcessType_Default != XRE_GetProcessType()) {
            
            appCacheChannel->SetChooseApplicationCache(true);
        } else {
            appCacheChannel->SetChooseApplicationCache(
                NS_ShouldCheckAppCache(aURI, mInPrivateBrowsing));
        }
    }

    
    
    if (aRequest)
        NS_ADDREF(*aRequest = channel);

    channel->SetOriginalURI(aURI);
    if (aTypeHint && *aTypeHint) {
        channel->SetContentType(nsDependentCString(aTypeHint));
        mContentTypeHint = aTypeHint;
    } else {
        mContentTypeHint.Truncate();
    }

    if (!aFileName.IsVoid()) {
        rv = channel->SetContentDisposition(nsIChannel::DISPOSITION_ATTACHMENT);
        NS_ENSURE_SUCCESS(rv, rv);
        if (!aFileName.IsEmpty()) {
            rv = channel->SetContentDispositionFilename(aFileName);
            NS_ENSURE_SUCCESS(rv, rv);
        }
    }

    if (mLoadType == LOAD_RELOAD_ALLOW_MIXED_CONTENT) {
          rv = SetMixedContentChannel(channel);
          NS_ENSURE_SUCCESS(rv, rv);
    } else {
          rv = SetMixedContentChannel(nullptr);
          NS_ENSURE_SUCCESS(rv, rv);
    }

    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
    nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal(do_QueryInterface(channel));
    if (httpChannelInternal) {
      if (aForceAllowCookies) {
        httpChannelInternal->SetForceAllowThirdPartyCookie(true);
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
                    uint32_t loadFlags;
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

    nsCOMPtr<nsIPrincipal> ownerPrincipal;

    
    
    
    
    if (mSandboxFlags & SANDBOXED_ORIGIN) {
        ownerPrincipal = do_CreateInstance("@mozilla.org/nullprincipal;1");
    } else {
        
        ownerPrincipal = do_QueryInterface(aOwner);
    }

    nsContentUtils::SetUpChannelOwner(ownerPrincipal, channel, aURI, true,
                                      (mSandboxFlags & SANDBOXED_ORIGIN));

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
                true);
        }
    }

    if (Preferences::GetBool("dom.enable_performance", false)) {
        nsCOMPtr<nsITimedChannel> timedChannel(do_QueryInterface(channel));
        if (timedChannel) {
            timedChannel->SetTimingEnabled(true);
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
                      uint32_t toOffset,
                      uint32_t count,
                      uint32_t *writeCount)
{
    

    nsAutoCString *buf = static_cast<nsAutoCString *>(closure);
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

    uint32_t numRead;
    nsAutoCString headersString;
    nsresult rv = aHeadersData->ReadSegments(AppendSegmentToString,
                                             &headersString,
                                             UINT32_MAX,
                                             &numRead);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsAutoCString headerName;
    nsAutoCString headerValue;
    int32_t crlf;
    int32_t colon;

    
    
    
    

    static const char kWhitespace[] = "\b\t\r\n ";
    while (true) {
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

        
        
        

        rv = httpChannel->SetRequestHeader(headerName, headerValue, true);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    NS_NOTREACHED("oops");
    return NS_ERROR_UNEXPECTED;
}

nsresult nsDocShell::DoChannelLoad(nsIChannel * aChannel,
                                   nsIURILoader * aURILoader,
                                   bool aBypassClassifier)
{
    nsresult rv;
    
    nsLoadFlags loadFlags = 0;
    (void) aChannel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_DOCUMENT_URI |
                 nsIChannel::LOAD_CALL_CONTENT_SNIFFERS;

    
    switch (mLoadType) {
    case LOAD_HISTORY:
        {
            
            
            bool uriModified = false;
            if (mLSHE) {
                mLSHE->GetURIWasModified(&uriModified);
            }

            if (!uriModified)
                loadFlags |= nsIRequest::VALIDATE_NEVER;
        }
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
    case LOAD_RELOAD_ALLOW_MIXED_CONTENT:
    case LOAD_REPLACE_BYPASS_CACHE:
        loadFlags |= nsIRequest::LOAD_BYPASS_CACHE |
                     nsIRequest::LOAD_FRESH_CONNECTION;
        break;

    case LOAD_NORMAL:
    case LOAD_LINK:
        
        switch (Preferences::GetInt("browser.cache.check_doc_frequency", -1)) {
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
        break;
    }

    if (!aBypassClassifier) {
        loadFlags |= nsIChannel::LOAD_CLASSIFY_URI;
    }

    (void) aChannel->SetLoadFlags(loadFlags);

    rv = aURILoader->OpenURI(aChannel,
                             (mLoadType == LOAD_LINK),
                             this);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
nsDocShell::ScrollToAnchor(nsACString & aCurHash, nsACString & aNewHash,
                           uint32_t aLoadType)
{
    if (!mCurrentURI) {
        return NS_OK;
    }

    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    if (!shell) {
        
        
        return NS_OK;
    }

    
    
    
    
    if ((aCurHash.IsEmpty() || aLoadType != LOAD_HISTORY) &&
        aNewHash.IsEmpty()) {
        return NS_OK;
    }

    
    
    nsDependentCSubstring newHashName(aNewHash, 1);

    
    

    if (!newHashName.IsEmpty()) {
        
        
        bool scroll = aLoadType != LOAD_HISTORY &&
                        aLoadType != LOAD_RELOAD_NORMAL;

        char *str = ToNewCString(newHashName);
        if (!str) {
            return NS_ERROR_OUT_OF_MEMORY;
        }

        
        nsUnescape(str);

        
        
        

        
        
        
        
        
        nsresult rv = NS_ERROR_FAILURE;
        NS_ConvertUTF8toUTF16 uStr(str);
        if (!uStr.IsEmpty()) {
            rv = shell->GoToAnchor(NS_ConvertUTF8toUTF16(str), scroll);
        }
        nsMemory::Free(str);

        
        
        if (NS_FAILED(rv)) {
                
            
            NS_ENSURE_TRUE(mContentViewer, NS_ERROR_FAILURE);
            nsIDocument* doc = mContentViewer->GetDocument();
            NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
            const nsACString &aCharset = doc->GetDocumentCharacterSet();

            nsCOMPtr<nsITextToSubURI> textToSubURI =
                do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
            NS_ENSURE_SUCCESS(rv, rv);

            
            nsXPIDLString uStr;

            rv = textToSubURI->UnEscapeAndConvert(PromiseFlatCString(aCharset).get(),
                                                  PromiseFlatCString(newHashName).get(),
                                                  getter_Copies(uStr));
            NS_ENSURE_SUCCESS(rv, rv);

            
            
            
            
            
            
            
            
            shell->GoToAnchor(uStr, scroll && !uStr.IsEmpty());
        }
    }
    else {

        
        shell->GoToAnchor(EmptyString(), false);
        
        
        
        
        
        if (aLoadType == LOAD_HISTORY || aLoadType == LOAD_RELOAD_NORMAL)
            return NS_OK;
        
        
        
        SetCurScrollPosEx(0, 0);
    }

    return NS_OK;
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

bool
nsDocShell::OnNewURI(nsIURI * aURI, nsIChannel * aChannel, nsISupports* aOwner,
                     uint32_t aLoadType, bool aFireOnLocationChange,
                     bool aAddToGlobalHistory, bool aCloneSHChildren)
{
    NS_PRECONDITION(aURI, "uri is null");
    NS_PRECONDITION(!aChannel || !aOwner, "Shouldn't have both set");

#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsAutoCString spec;
        aURI->GetSpec(spec);

        nsAutoCString chanName;
        if (aChannel)
            aChannel->GetName(chanName);
        else
            chanName.AssignLiteral("<no channel>");

        PR_LOG(gDocShellLog, PR_LOG_DEBUG,
               ("nsDocShell[%p]::OnNewURI(\"%s\", [%s], 0x%x)\n", this, spec.get(),
                chanName.get(), aLoadType));
    }
#endif

    bool equalUri = false;

    
    uint32_t responseStatus = 0;
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

            
            
            nsresult rv = httpChannel->GetResponseStatus(&responseStatus);
            if (mLSHE && NS_SUCCEEDED(rv) && responseStatus >= 400) {
                mLSHE->AbandonBFCacheEntry();
            }
        }
    }

    
    bool updateGHistory = !(aLoadType == LOAD_BYPASS_HISTORY ||
                            aLoadType == LOAD_ERROR_PAGE ||
                            aLoadType & LOAD_CMD_HISTORY);

    
    bool updateSHistory = updateGHistory && (!(aLoadType & LOAD_CMD_RELOAD));

    


    nsCOMPtr<nsISHistory> rootSH = mSessionHistory;
    if (!rootSH) {
        
        GetRootSessionHistory(getter_AddRefs(rootSH));
        if (!rootSH) {
            updateSHistory = false;
            updateGHistory = false; 
        }
    }  

    
    if (mCurrentURI)
        aURI->Equals(mCurrentURI, &equalUri);

#ifdef DEBUG
    bool shAvailable = (rootSH != nullptr);

    
    

    PR_LOG(gDocShellLog, PR_LOG_DEBUG,
           ("  shAvailable=%i updateSHistory=%i updateGHistory=%i"
            " equalURI=%i\n",
            shAvailable, updateSHistory, updateGHistory, equalUri));

    if (shAvailable && mCurrentURI && !mOSHE && aLoadType != LOAD_ERROR_PAGE) {
        NS_ASSERTION(NS_IsAboutBlank(mCurrentURI), "no SHEntry for a non-transient viewer?");
    }
#endif

    
















    if (equalUri &&
        mOSHE &&
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
         aLoadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE ||
         aLoadType == LOAD_RELOAD_ALLOW_MIXED_CONTENT)) {
        NS_ASSERTION(!updateSHistory,
                     "We shouldn't be updating session history for forced"
                     " reloads!");
        
        nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(aChannel));
        nsCOMPtr<nsISupports>  cacheKey;
        
        if (cacheChannel)
            cacheChannel->GetCacheKey(getter_AddRefs(cacheKey));
        
        
        
        if (mLSHE)
            mLSHE->SetCacheKey(cacheKey);
        else if (mOSHE)
            mOSHE->SetCacheKey(cacheKey);

        
        ClearFrameHistory(mLSHE);
        ClearFrameHistory(mOSHE);
    }

    if (aLoadType == LOAD_RELOAD_NORMAL) {
        nsCOMPtr<nsISHEntry> currentSH;
        bool oshe = false;
        GetCurrentSHEntry(getter_AddRefs(currentSH), &oshe);
        bool dynamicallyAddedChild = false;
        if (currentSH) {
          currentSH->HasDynamicallyAddedChild(&dynamicallyAddedChild);
        }
        if (dynamicallyAddedChild) {
          ClearFrameHistory(currentSH);
        }
    }

    if (aLoadType == LOAD_REFRESH) {
        ClearFrameHistory(mLSHE);
        ClearFrameHistory(mOSHE);
    }

    if (updateSHistory) { 
        
        if (!mLSHE && (mItemType == typeContent) && mURIResultedInDocument) {
            



            (void) AddToSessionHistory(aURI, aChannel, aOwner, aCloneSHChildren,
                                       getter_AddRefs(mLSHE));
        }
    }

    
    
    if (updateGHistory &&
        aAddToGlobalHistory && 
        !ChannelIsPost(aChannel)) {
        nsCOMPtr<nsIURI> previousURI;
        uint32_t previousFlags = 0;

        if (aLoadType & LOAD_CMD_RELOAD) {
            
            previousURI = aURI;
        } else {
            ExtractLastVisit(aChannel, getter_AddRefs(previousURI),
                             &previousFlags);
        }

        
        
        nsCOMPtr<nsIURI> referrer;
        
        (void)NS_GetReferrerFromChannel(aChannel, getter_AddRefs(referrer));

        AddURIVisit(aURI, referrer, previousURI, previousFlags, responseStatus);
    }

    
    
    if (rootSH && (mLoadType & (LOAD_CMD_HISTORY | LOAD_CMD_RELOAD))) {
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

    
    uint32_t locationFlags = aCloneSHChildren?
                                 uint32_t(LOCATION_CHANGE_SAME_DOCUMENT) : 0;

    bool onLocationChangeNeeded = SetCurrentURI(aURI, aChannel,
                                                aFireOnLocationChange,
                                                locationFlags);
    
    SetupReferrerFromChannel(aChannel);
    return onLocationChangeNeeded;
}

bool
nsDocShell::OnLoadingSite(nsIChannel * aChannel, bool aFireOnLocationChange,
                          bool aAddToGlobalHistory)
{
    nsCOMPtr<nsIURI> uri;
    
    
    
    
    NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
    NS_ENSURE_TRUE(uri, false);

    
    return OnNewURI(uri, aChannel, nullptr, mLoadType, aFireOnLocationChange,
                    aAddToGlobalHistory, false);

}

void
nsDocShell::SetReferrerURI(nsIURI * aURI)
{
    mReferrerURI = aURI;        
}





NS_IMETHODIMP
nsDocShell::AddState(nsIVariant *aData, const nsAString& aTitle,
                     const nsAString& aURL, bool aReplace, JSContext* aCx)
{
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    nsresult rv;

    
    AutoRestore<uint32_t> loadTypeResetter(mLoadType);

    
    
    
    if (JustStartedNetworkLoad()) {
        aReplace = true;
    }

    nsCOMPtr<nsIDocument> document = do_GetInterface(GetAsSupports(this));
    NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

    
    nsCOMPtr<nsIStructuredCloneContainer> scContainer;

    
    
    
    
    {
        nsCOMPtr<nsIDocument> origDocument =
            do_GetInterface(GetAsSupports(this));
        if (!origDocument)
            return NS_ERROR_DOM_SECURITY_ERR;
        nsCOMPtr<nsIPrincipal> origPrincipal = origDocument->NodePrincipal();

        scContainer = new nsStructuredCloneContainer();
        JSContext *cx = aCx;
        if (!cx) {
            cx = nsContentUtils::GetContextFromDocument(document);
        }
        rv = scContainer->InitFromVariant(aData, cx);

        
        
        if (NS_FAILED(rv) && !aCx) {
            JS_ClearPendingException(aCx);
        }
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIDocument> newDocument =
            do_GetInterface(GetAsSupports(this));
        if (!newDocument)
            return NS_ERROR_DOM_SECURITY_ERR;
        nsCOMPtr<nsIPrincipal> newPrincipal = newDocument->NodePrincipal();

        bool principalsEqual = false;
        origPrincipal->Equals(newPrincipal, &principalsEqual);
        NS_ENSURE_TRUE(principalsEqual, NS_ERROR_DOM_SECURITY_ERR);
    }

    
    
    int32_t maxStateObjSize =
        Preferences::GetInt("browser.history.maxStateObjectSize", 0xA0000);
    if (maxStateObjSize < 0) {
        maxStateObjSize = 0;
    }

    uint64_t scSize;
    rv = scContainer->GetSerializedNBytes(&scSize);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(scSize <= (uint32_t)maxStateObjSize,
                   NS_ERROR_ILLEGAL_VALUE);

    
    bool equalURIs = true;
    nsCOMPtr<nsIURI> oldURI = mCurrentURI;
    nsCOMPtr<nsIURI> newURI;
    if (aURL.Length() == 0) {
        newURI = mCurrentURI;
    }
    else {
        

        nsIURI* docBaseURI = document->GetDocBaseURI();
        if (!docBaseURI)
            return NS_ERROR_FAILURE;

        nsAutoCString spec;
        docBaseURI->GetSpec(spec);

        nsAutoCString charset;
        rv = docBaseURI->GetOriginCharset(charset);
        NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

        rv = NS_NewURI(getter_AddRefs(newURI), aURL,
                       charset.get(), docBaseURI);

        
        if (NS_FAILED(rv)) {
            return NS_ERROR_DOM_SECURITY_ERR;
        }

        
        if (!nsContentUtils::URIIsLocalFile(newURI)) {
            
            
            
            
            
            
            
            

            nsCOMPtr<nsIScriptSecurityManager> secMan =
                do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
            NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

            
            
            
            nsAutoCString currentUserPass, newUserPass;
            NS_ENSURE_SUCCESS(mCurrentURI->GetUserPass(currentUserPass),
                              NS_ERROR_FAILURE);
            NS_ENSURE_SUCCESS(newURI->GetUserPass(newUserPass),
                              NS_ERROR_FAILURE);
            if (NS_FAILED(secMan->CheckSameOriginURI(mCurrentURI,
                                                     newURI, true)) ||
                !currentUserPass.Equals(newUserPass)) {

                return NS_ERROR_DOM_SECURITY_ERR;
            }
        }
        else {
            
            nsCOMPtr<nsIScriptObjectPrincipal> docScriptObj =
                do_QueryInterface(document);

            if (!docScriptObj) {
                return NS_ERROR_DOM_SECURITY_ERR;
            }

            nsCOMPtr<nsIPrincipal> principal = docScriptObj->GetPrincipal();

            if (!principal ||
                NS_FAILED(principal->CheckMayLoad(newURI, true, false))) {

                return NS_ERROR_DOM_SECURITY_ERR;
            }
        }

        if (mCurrentURI) {
            mCurrentURI->Equals(newURI, &equalURIs);
        }
        else {
            equalURIs = false;
        }

    } 

    
    
    
    
    NS_ENSURE_TRUE(mOSHE, NS_ERROR_FAILURE);
    nsCOMPtr<nsISHEntry> oldOSHE = mOSHE;

    mLoadType = LOAD_PUSHSTATE;

    nsCOMPtr<nsISHEntry> newSHEntry;
    if (!aReplace) {
        
        nscoord cx = 0, cy = 0;
        GetCurScrollPos(ScrollOrientation_X, &cx);
        GetCurScrollPos(ScrollOrientation_Y, &cy);
        mOSHE->SetScrollPosition(cx, cy);

        
        
        rv = AddToSessionHistory(newURI, nullptr, nullptr, true,
                                 getter_AddRefs(newSHEntry));
        NS_ENSURE_SUCCESS(rv, rv);

        NS_ENSURE_TRUE(newSHEntry, NS_ERROR_FAILURE);

        
        
        NS_ENSURE_SUCCESS(newSHEntry->AdoptBFCacheEntry(oldOSHE),
                          NS_ERROR_FAILURE);

        
        nsString title;
        mOSHE->GetTitle(getter_Copies(title));
        newSHEntry->SetTitle(title);

        
        
        mOSHE = newSHEntry;

    } else {
        newSHEntry = mOSHE;
        newSHEntry->SetURI(newURI);
    }

    
    
    newSHEntry->SetStateData(scContainer);
    newSHEntry->SetPostData(nullptr);

    
    
    
    
    bool sameExceptHashes = true, oldURIWasModified = false;
    newURI->EqualsExceptRef(mCurrentURI, &sameExceptHashes);
    oldOSHE->GetURIWasModified(&oldURIWasModified);
    newSHEntry->SetURIWasModified(!sameExceptHashes || oldURIWasModified);

    
    
    
    if (!aReplace) {
        nsCOMPtr<nsISHistory> rootSH;
        GetRootSessionHistory(getter_AddRefs(rootSH));
        NS_ENSURE_TRUE(rootSH, NS_ERROR_UNEXPECTED);

        nsCOMPtr<nsISHistoryInternal> internalSH =
            do_QueryInterface(rootSH);
        NS_ENSURE_TRUE(internalSH, NS_ERROR_UNEXPECTED);

        int32_t curIndex = -1;
        rv = rootSH->GetIndex(&curIndex);
        if (NS_SUCCEEDED(rv) && curIndex > -1) {
            internalSH->EvictOutOfRangeContentViewers(curIndex);
        }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!equalURIs) {
        SetCurrentURI(newURI, nullptr, true, LOCATION_CHANGE_SAME_DOCUMENT);
        document->SetDocumentURI(newURI);

        AddURIVisit(newURI, oldURI, oldURI, 0);

        
        
        if (mUseGlobalHistory && !mInPrivateBrowsing) {
            nsCOMPtr<IHistory> history = services::GetHistoryService();
            if (history) {
                history->SetURITitle(newURI, mTitle);
            }
            else if (mGlobalHistory) {
                mGlobalHistory->SetPageTitle(newURI, mTitle);
            }
        }

        
        
        CopyFavicon(oldURI, newURI, mInPrivateBrowsing);
    }
    else {
        FireDummyOnLocationChange();
    }
    document->SetStateObject(scContainer);

    return NS_OK;
}

bool
nsDocShell::ShouldAddToSessionHistory(nsIURI * aURI)
{
    
    
    
    
    nsresult rv;
    nsAutoCString buf, pref;

    rv = aURI->GetScheme(buf);
    if (NS_FAILED(rv))
        return false;

    if (buf.Equals("about")) {
        rv = aURI->GetPath(buf);
        if (NS_FAILED(rv))
            return false;

        if (buf.Equals("blank")) {
            return false;
        }
    }

    rv = Preferences::GetDefaultCString("browser.newtab.url", &pref);

    if (NS_FAILED(rv)) {
        return true;
    }

    rv = aURI->GetSpec(buf);
    NS_ENSURE_SUCCESS(rv, true);

    return !buf.Equals(pref);
}

nsresult
nsDocShell::AddToSessionHistory(nsIURI * aURI, nsIChannel * aChannel,
                                nsISupports* aOwner, bool aCloneChildren,
                                nsISHEntry ** aNewEntry)
{
    NS_PRECONDITION(aURI, "uri is null");
    NS_PRECONDITION(!aChannel || !aOwner, "Shouldn't have both set");

#if defined(PR_LOGGING) && defined(DEBUG)
    if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
        nsAutoCString spec;
        aURI->GetSpec(spec);

        nsAutoCString chanName;
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
    bool shouldPersist;

    shouldPersist = ShouldAddToSessionHistory(aURI);

    
    nsCOMPtr<nsIDocShellTreeItem> root;
    GetSameTypeRootTreeItem(getter_AddRefs(root));     
    




    if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY) &&
        root != static_cast<nsIDocShellTreeItem *>(this)) {
        
        entry = mOSHE;
        nsCOMPtr<nsISHContainer> shContainer(do_QueryInterface(entry));
        if (shContainer) {
            int32_t childCount = 0;
            shContainer->GetChildCount(&childCount);
            
            for (int32_t i = childCount - 1; i >= 0; i--) {
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
    nsCOMPtr<nsISupports> owner = aOwner;
    bool expired = false;
    bool discardLayoutState = false;
    nsCOMPtr<nsICachingChannel> cacheChannel;
    if (aChannel) {
        cacheChannel = do_QueryInterface(aChannel);

        


        if (cacheChannel) {
            cacheChannel->GetCacheKey(getter_AddRefs(cacheKey));
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
                  nullptr,            
                  cacheKey,          
                  mContentTypeHint,  
                  owner,             
                  mHistoryID,
                  mDynamicallyCreated);
    entry->SetReferrerURI(referrerURI);
    


    
    if (discardLayoutState) {
        entry->SetSaveLayoutStateFlag(false);
    }
    if (cacheChannel) {
        
        uint32_t expTime = 0;
        cacheChannel->GetCacheTokenExpirationTime(&expTime);
        uint32_t now = PRTimeToSeconds(PR_Now());
        if (expTime <=  now)
            expired = true;
    }
    if (expired)
        entry->SetExpirationStatus(true);


    if (root == static_cast<nsIDocShellTreeItem *>(this) && mSessionHistory) {
        
        
        if (aCloneChildren && mOSHE) {
            uint32_t cloneID;
            mOSHE->GetID(&cloneID);
            nsCOMPtr<nsISHEntry> newEntry;
            CloneAndReplace(mOSHE, this, cloneID, entry, true, getter_AddRefs(newEntry));
            NS_ASSERTION(entry == newEntry, "The new session history should be in the new entry");
        }

        
        if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {            
            
            int32_t  index = 0;   
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
            rv = DoAddChildSHEntry(entry, mChildOffset, aCloneChildren);
    }

    
    if (aNewEntry) {
        *aNewEntry = nullptr;
        if (NS_SUCCEEDED(rv)) {
            *aNewEntry = entry;
            NS_ADDREF(*aNewEntry);
        }
    }

    return rv;
}


NS_IMETHODIMP
nsDocShell::LoadHistoryEntry(nsISHEntry * aEntry, uint32_t aLoadType)
{
    if (!IsNavigationAllowed()) {
        return NS_OK;
    }
    
    nsCOMPtr<nsIURI> uri;
    nsCOMPtr<nsIInputStream> postData;
    nsCOMPtr<nsIURI> referrerURI;
    nsAutoCString contentType;
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
    bool isJS;
    nsresult rv = uri->SchemeIs("javascript", &isJS);
    if (NS_FAILED(rv) || isJS) {
        
        
        
        
        nsCOMPtr<nsIPrincipal> prin = do_QueryInterface(owner);
        
        
        
        rv = CreateAboutBlankContentViewer(prin, nullptr, aEntry != mOSHE);

        if (NS_FAILED(rv)) {
            
            
            
            return NS_OK;
        }

        if (!owner) {
            
            
            
            owner = do_CreateInstance("@mozilla.org/nullprincipal;1");
            NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);
        }
    }

    



    if ((aLoadType & LOAD_CMD_RELOAD) && postData) {
      bool repost;
      rv = ConfirmRepost(&repost);
      if (NS_FAILED(rv)) return rv;

      
      if (!repost)
        return NS_BINDING_ABORTED;
    }

    rv = InternalLoad(uri,
                      referrerURI,
                      owner,
                      INTERNAL_LOAD_FLAGS_NONE, 
                      nullptr,            
                      contentType.get(),  
                      NullString(),       
                      postData,           
                      nullptr,            
                      aLoadType,          
                      aEntry,             
                      true,
                      nullptr,            
                      nullptr);           
    return rv;
}

NS_IMETHODIMP nsDocShell::GetShouldSaveLayoutState(bool* aShould)
{
    *aShould = false;
    if (mOSHE) {
        
        
        mOSHE->GetSaveLayoutStateFlag(aShould);
    }

    return NS_OK;
}

NS_IMETHODIMP nsDocShell::PersistLayoutHistoryState()
{
    nsresult  rv = NS_OK;
    
    if (mOSHE) {
        nsCOMPtr<nsIPresShell> shell = GetPresShell();
        if (shell) {
            nsCOMPtr<nsILayoutHistoryState> layoutState;
            rv = shell->CaptureHistoryState(getter_AddRefs(layoutState));
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

    int32_t childCount;
    container->GetChildCount(&childCount);
    for (int32_t i = 0; i < childCount; i++) {
        nsCOMPtr<nsISHEntry> childEntry;
        container->GetChildAt(i, getter_AddRefs(childEntry));
        if (!childEntry) {
            
            
            
            aCallback(nullptr, nullptr, i, aData);
            continue;
        }

        nsDocShell *childShell = nullptr;
        if (aRootShell) {
            
            

            uint32_t childCount = aRootShell->mChildList.Length();
            for (uint32_t j = 0; j < childCount; ++j) {
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
    CloneAndReplaceData(uint32_t aCloneID, nsISHEntry *aReplaceEntry,
                        bool aCloneChildren, nsISHEntry *aDestTreeParent)
        : cloneID(aCloneID),
          cloneChildren(aCloneChildren),
          replaceEntry(aReplaceEntry),
          destTreeParent(aDestTreeParent) { }

    uint32_t              cloneID;
    bool                  cloneChildren;
    nsISHEntry           *replaceEntry;
    nsISHEntry           *destTreeParent;
    nsCOMPtr<nsISHEntry>  resultEntry;
};

 nsresult
nsDocShell::CloneAndReplaceChild(nsISHEntry *aEntry, nsDocShell *aShell,
                                 int32_t aEntryIndex, void *aData)
{
    nsCOMPtr<nsISHEntry> dest;

    CloneAndReplaceData *data = static_cast<CloneAndReplaceData*>(aData);
    uint32_t cloneID = data->cloneID;
    nsISHEntry *replaceEntry = data->replaceEntry;

    nsCOMPtr<nsISHContainer> container =
      do_QueryInterface(data->destTreeParent);
    if (!aEntry) {
        if (container) {
            container->AddChild(nullptr, aEntryIndex);
        }
        return NS_OK;
    }
    
    uint32_t srcID;
    aEntry->GetID(&srcID);

    nsresult rv = NS_OK;
    if (srcID == cloneID) {
        
        dest = replaceEntry;
    } else {
        
        rv = aEntry->Clone(getter_AddRefs(dest));
        NS_ENSURE_SUCCESS(rv, rv);
    }
    dest->SetIsSubFrame(true);

    if (srcID != cloneID || data->cloneChildren) {
        
        CloneAndReplaceData childData(cloneID, replaceEntry,
                                      data->cloneChildren, dest);
        rv = WalkHistoryEntries(aEntry, aShell,
                                CloneAndReplaceChild, &childData);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    if (srcID != cloneID && aShell) {
        aShell->SwapHistoryEntries(aEntry, dest);
    }

    if (container)
        container->AddChild(dest, aEntryIndex);

    data->resultEntry = dest;
    return rv;
}

 nsresult
nsDocShell::CloneAndReplace(nsISHEntry *aSrcEntry,
                                   nsDocShell *aSrcShell,
                                   uint32_t aCloneID,
                                   nsISHEntry *aReplaceEntry,
                                   bool aCloneChildren,
                                   nsISHEntry **aResultEntry)
{
    NS_ENSURE_ARG_POINTER(aResultEntry);
    NS_ENSURE_TRUE(aReplaceEntry, NS_ERROR_FAILURE);

    CloneAndReplaceData data(aCloneID, aReplaceEntry, aCloneChildren, nullptr);
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
                                 int32_t aEntryIndex, void *aData)
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
        
        
        
        

        uint32_t targetID, id;
        aEntry->GetID(&targetID);

        
        nsCOMPtr<nsISHEntry> entry;
        container->GetChildAt(aEntryIndex, getter_AddRefs(entry));
        if (entry && NS_SUCCEEDED(entry->GetID(&id)) && id == targetID) {
            destEntry.swap(entry);
        } else {
            int32_t childCount;
            container->GetChildCount(&childCount);
            for (int32_t i = 0; i < childCount; ++i) {
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
    nsISHEntry *result = nullptr;
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
            nsCOMPtr<nsIDocShellTreeItem> rootAsItem;
            GetSameTypeRootTreeItem(getter_AddRefs(rootAsItem));
            nsCOMPtr<nsIDocShell> rootShell = do_QueryInterface(rootAsItem);
            if (rootShell) { 
                SwapEntriesData data = { this, newRootEntry };
                nsIDocShell *rootIDocShell =
                    static_cast<nsIDocShell*>(rootShell);
                nsDocShell *rootDocShell = static_cast<nsDocShell*>
                                                      (rootIDocShell);

#ifdef DEBUG
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

bool 
nsDocShell::ShouldDiscardLayoutState(nsIHttpChannel * aChannel)
{    
    
    if (!aChannel)
        return false;

    
    nsCOMPtr<nsISupports> securityInfo;
    bool noStore = false, noCache = false;
    aChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
    aChannel->IsNoStoreResponse(&noStore);
    aChannel->IsNoCacheResponse(&noCache);

    return (noStore || (noCache && securityInfo));
}

NS_IMETHODIMP nsDocShell::GetEditor(nsIEditor * *aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  if (!mEditorData) {
    *aEditor = nullptr;
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


NS_IMETHODIMP nsDocShell::GetEditable(bool *aEditable)
{
  NS_ENSURE_ARG_POINTER(aEditable);
  *aEditable = mEditorData && mEditorData->GetEditable();
  return NS_OK;
}


NS_IMETHODIMP nsDocShell::GetHasEditingSession(bool *aHasEditingSession)
{
  NS_ENSURE_ARG_POINTER(aHasEditingSession);
  
  if (mEditorData)
  {
    nsCOMPtr<nsIEditingSession> editingSession;
    mEditorData->GetEditingSession(getter_AddRefs(editingSession));
    *aHasEditingSession = (editingSession.get() != nullptr);
  }
  else
  {
    *aHasEditingSession = false;
  }

  return NS_OK;
}

NS_IMETHODIMP nsDocShell::MakeEditable(bool inWaitForUriLoad)
{
  nsresult rv = EnsureEditorData();
  if (NS_FAILED(rv)) return rv;

  return mEditorData->MakeEditable(inWaitForUriLoad);
}

bool
nsDocShell::ChannelIsPost(nsIChannel* aChannel)
{
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
    if (!httpChannel) {
        return false;
    }

    nsAutoCString method;
    httpChannel->GetRequestMethod(method);
    return method.Equals("POST");
}

void
nsDocShell::ExtractLastVisit(nsIChannel* aChannel,
                             nsIURI** aURI,
                             uint32_t* aChannelRedirectFlags)
{
    nsCOMPtr<nsIPropertyBag2> props(do_QueryInterface(aChannel));
    if (!props) {
        return;
    }

    nsresult rv = props->GetPropertyAsInterface(
        NS_LITERAL_STRING("docshell.previousURI"),
        NS_GET_IID(nsIURI),
        reinterpret_cast<void**>(aURI)
    );

    if (NS_FAILED(rv)) {
        
        
        
        (void)NS_GetReferrerFromChannel(aChannel, aURI);
    }
    else {
      rv = props->GetPropertyAsUint32(
          NS_LITERAL_STRING("docshell.previousFlags"),
          aChannelRedirectFlags
      );

      NS_WARN_IF_FALSE(
          NS_SUCCEEDED(rv),
          "Could not fetch previous flags, URI will be treated like referrer"
      );
    }
}

void
nsDocShell::SaveLastVisit(nsIChannel* aChannel,
                          nsIURI* aURI,
                          uint32_t aChannelRedirectFlags)
{
    nsCOMPtr<nsIWritablePropertyBag2> props(do_QueryInterface(aChannel));
    if (!props || !aURI) {
        return;
    }

    props->SetPropertyAsInterface(NS_LITERAL_STRING("docshell.previousURI"),
                                  aURI);
    props->SetPropertyAsUint32(NS_LITERAL_STRING("docshell.previousFlags"),
                               aChannelRedirectFlags);
}

void
nsDocShell::AddURIVisit(nsIURI* aURI,
                        nsIURI* aReferrerURI,
                        nsIURI* aPreviousURI,
                        uint32_t aChannelRedirectFlags,
                        uint32_t aResponseStatus)
{
    MOZ_ASSERT(aURI, "Visited URI is null!");
    MOZ_ASSERT(mLoadType != LOAD_ERROR_PAGE &&
               mLoadType != LOAD_BYPASS_HISTORY,
               "Do not add error or bypass pages to global history");

    
    
    if (mItemType != typeContent || !mUseGlobalHistory || mInPrivateBrowsing) {
        return;
    }

    nsCOMPtr<IHistory> history = services::GetHistoryService();

    if (history) {
        uint32_t visitURIFlags = 0;

        if (!IsFrame()) {
            visitURIFlags |= IHistory::TOP_LEVEL;
        }

        if (aChannelRedirectFlags & nsIChannelEventSink::REDIRECT_TEMPORARY) {
            visitURIFlags |= IHistory::REDIRECT_TEMPORARY;
        }
        else if (aChannelRedirectFlags &
                 nsIChannelEventSink::REDIRECT_PERMANENT) {
            visitURIFlags |= IHistory::REDIRECT_PERMANENT;
        }

        if (aResponseStatus >= 300 && aResponseStatus < 400) {
            visitURIFlags |= IHistory::REDIRECT_SOURCE;
        }
        
        
        
        
        else if (aResponseStatus != 408 &&
                 ((aResponseStatus >= 400 && aResponseStatus <= 501) ||
                   aResponseStatus == 505)) {
            visitURIFlags |= IHistory::UNRECOVERABLE_ERROR;
        }

        (void)history->VisitURI(aURI, aPreviousURI, visitURIFlags);
    }
    else if (mGlobalHistory) {
        
        (void)mGlobalHistory->AddURI(aURI,
                                     !!aChannelRedirectFlags,
                                     !IsFrame(),
                                     aReferrerURI);
    }
}





NS_IMETHODIMP
nsDocShell::SetLoadType(uint32_t aLoadType)
{
    mLoadType = aLoadType;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLoadType(uint32_t * aLoadType)
{
    *aLoadType = mLoadType;
    return NS_OK;
}

nsresult
nsDocShell::ConfirmRepost(bool * aRepost)
{
  nsCOMPtr<nsIPrompt> prompter;
  CallGetInterface(this, static_cast<nsIPrompt**>(getter_AddRefs(prompter)));
  if (!prompter) {
      return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  if (!stringBundleService)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIStringBundle> appBundle;
  nsresult rv = stringBundleService->CreateBundle(kAppstringsBundleURL,
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
                                         formatStrings, ArrayLength(formatStrings),
                                         getter_Copies(msgString));
  }
  if (NS_FAILED(rv)) return rv;

  rv = appBundle->GetStringFromName(NS_LITERAL_STRING("resendButton.label").get(),
                                    getter_Copies(button0Title));
  if (NS_FAILED(rv)) return rv;

  int32_t buttonPressed;
  
  
  bool checkState = false;
  rv = prompter->
         ConfirmEx(nullptr, msgString.get(),
                   (nsIPrompt::BUTTON_POS_0 * nsIPrompt::BUTTON_TITLE_IS_STRING) +
                   (nsIPrompt::BUTTON_POS_1 * nsIPrompt::BUTTON_TITLE_CANCEL),
                   button0Title.get(), nullptr, nullptr, nullptr, &checkState, &buttonPressed);
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

    nsCOMPtr<nsIStringBundleService> stringBundleService =
      mozilla::services::GetStringBundleService();
    NS_ENSURE_TRUE(stringBundleService, NS_ERROR_FAILURE);

    NS_ENSURE_SUCCESS(stringBundleService->
                      CreateBundle(kAppstringsBundleURL,
                                   aStringBundle),
                      NS_ERROR_FAILURE);

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChildOffset(nsIDOMNode * aChild, nsIDOMNode * aParent,
                           int32_t * aOffset)
{
    NS_ENSURE_ARG_POINTER(aChild || aParent);

    nsCOMPtr<nsIDOMNodeList> childNodes;
    NS_ENSURE_SUCCESS(aParent->GetChildNodes(getter_AddRefs(childNodes)),
                      NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(childNodes, NS_ERROR_FAILURE);

    int32_t i = 0;

    for (; true; i++) {
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

nsIScrollableFrame *
nsDocShell::GetRootScrollFrame()
{
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    NS_ENSURE_TRUE(shell, nullptr);

    return shell->GetRootScrollFrameAsScrollableExternal();
}

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

    
    
    AutoRestore<bool> boolSetter(mInEnsureScriptEnv);
    mInEnsureScriptEnv = true;
#endif

    nsCOMPtr<nsIWebBrowserChrome> browserChrome(do_GetInterface(mTreeOwner));
    NS_ENSURE_TRUE(browserChrome, NS_ERROR_NOT_AVAILABLE);

    uint32_t chromeFlags;
    browserChrome->GetChromeFlags(&chromeFlags);

    bool isModalContentWindow =
        (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL) &&
        !(chromeFlags & nsIWebBrowserChrome::CHROME_OPENAS_CHROME);

    
    
    nsRefPtr<nsGlobalWindow> window =
        NS_NewScriptGlobalObject(mItemType == typeChrome, isModalContentWindow);
    MOZ_ASSERT(window);
    mScriptGlobal = window;

    window->SetDocShell(this);

    
    return mScriptGlobal->EnsureScriptEnvironment();
}


NS_IMETHODIMP
nsDocShell::EnsureEditorData()
{
    bool openDocHasDetachedEditor = mOSHE && mOSHE->HasDetachedEditor();
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

    
    nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(scriptGO);
    nsCOMPtr<nsPIDOMWindow> windowToSearch;
    nsFocusManager::GetFocusedDescendant(ourWindow, true, getter_AddRefs(windowToSearch));

    nsCOMPtr<nsIWebBrowserFindInFrames> findInFrames = do_QueryInterface(mFind);
    if (!findInFrames) return NS_ERROR_NO_INTERFACE;
    
    rv = findInFrames->SetRootSearchFrame(ourWindow);
    if (NS_FAILED(rv)) return rv;
    rv = findInFrames->SetCurrentSearchFrame(windowToSearch);
    if (NS_FAILED(rv)) return rv;
    
    return NS_OK;
}

bool
nsDocShell::IsFrame()
{
    nsCOMPtr<nsIDocShellTreeItem> parent;
    GetSameTypeParent(getter_AddRefs(parent));
    return !!parent;
}


NS_IMETHODIMP 
nsDocShell::IsBeingDestroyed(bool *aDoomed)
{
  NS_ENSURE_ARG(aDoomed);
  *aDoomed = mIsBeingDestroyed;
  return NS_OK;
}


NS_IMETHODIMP 
nsDocShell::GetIsExecutingOnLoadHandler(bool *aResult)
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
    : mDelay(0), mRepeat(false), mMetaRefresh(false)
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
        
        uint32_t delay = 0;
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
    mWeakPtr = nullptr;
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
    *aSink = nullptr;
    return NS_NOINTERFACE;
}

nsresult
nsDocShell::SetBaseUrlForWyciwyg(nsIContentViewer * aContentViewer)
{
    if (!aContentViewer)
        return NS_ERROR_FAILURE;

    nsCOMPtr<nsIURI> baseURI;
    nsresult rv = NS_ERROR_NOT_AVAILABLE;

    if (sURIFixup)
        rv = sURIFixup->CreateExposableURI(mCurrentURI,
                                           getter_AddRefs(baseURI));

    
    if (baseURI) {
        nsIDocument* document = aContentViewer->GetDocument();
        if (document) {
            rv = document->SetBaseURI(baseURI);
        }
    }
    return rv;
}





NS_IMETHODIMP
nsDocShell::GetAuthPrompt(uint32_t aPromptReason, const nsIID& iid,
                          void** aResult)
{
    
    bool priorityPrompt = (aPromptReason == PROMPT_PROXY);

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

        bool tmpbool;
        rv = Preferences::GetBool("browser.xul.error_pages.enabled", &tmpbool);
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
    CallGetInterface(this, aWindow);
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetTopWindow(nsIDOMWindow** aWindow)
{
    nsCOMPtr<nsIDOMWindow> win = do_GetInterface(GetAsSupports(this));
    if (win) {
        win->GetTop(aWindow);
    }
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetTopFrameElement(nsIDOMElement** aElement)
{
    *aElement = nullptr;
    nsCOMPtr<nsIDOMWindow> win = do_GetInterface(GetAsSupports(this));
    if (!win) {
        return NS_OK;
    }

    nsCOMPtr<nsIDOMWindow> top;
    win->GetScriptableTop(getter_AddRefs(top));
    NS_ENSURE_TRUE(top, NS_ERROR_FAILURE);

    
    
    return top->GetFrameElement(aElement);
}

NS_IMETHODIMP
nsDocShell::IsAppOfType(uint32_t aAppType, bool *aIsOfType)
{
    nsCOMPtr<nsIDocShell> shell = this;
    while (shell) {
        uint32_t type;
        shell->GetAppType(&type);
        if (type == aAppType) {
            *aIsOfType = true;
            return NS_OK;
        }
        nsCOMPtr<nsIDocShellTreeItem> item = do_QueryInterface(shell);
        nsCOMPtr<nsIDocShellTreeItem> parent;
        item->GetParent(getter_AddRefs(parent));
        shell = do_QueryInterface(parent);
    }

    *aIsOfType = false;
    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsContent(bool *aIsContent)
{
    *aIsContent = (mItemType == typeContent);
    return NS_OK;
}

bool
nsDocShell::IsOKToLoadURI(nsIURI* aURI)
{
    NS_PRECONDITION(aURI, "Must have a URI!");
    
    if (!mFiredUnloadEvent) {
        return true;
    }

    if (!mLoadingURI) {
        return false;
    }

    nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
    return
        secMan &&
        NS_SUCCEEDED(secMan->CheckSameOriginURI(aURI, mLoadingURI, false));
}




nsresult
nsDocShell::GetControllerForCommand(const char * inCommand,
                                    nsIController** outController)
{
  NS_ENSURE_ARG_POINTER(outController);
  *outController = nullptr;

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mScriptGlobal));
  if (window) {
      nsCOMPtr<nsPIWindowRoot> root = window->GetTopWindowRoot();
      if (root) {
          return root->GetControllerForCommand(inCommand, outController);
      }
  }

  return NS_ERROR_FAILURE;
}

nsresult
nsDocShell::IsCommandEnabled(const char * inCommand, bool* outEnabled)
{
  NS_ENSURE_ARG_POINTER(outEnabled);
  *outEnabled = false;

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
nsDocShell::CanCutSelection(bool* aResult)
{
  return IsCommandEnabled("cmd_cut", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopySelection(bool* aResult)
{
  return IsCommandEnabled("cmd_copy", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyLinkLocation(bool* aResult)
{
  return IsCommandEnabled("cmd_copyLink", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyImageLocation(bool* aResult)
{
  return IsCommandEnabled("cmd_copyImageLocation",
                          aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyImageContents(bool* aResult)
{
  return IsCommandEnabled("cmd_copyImageContents",
                          aResult);
}

NS_IMETHODIMP
nsDocShell::CanPaste(bool* aResult)
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
                   const nsAString& aFileName,
                   nsIInputStream* aPostDataStream,
                   nsIInputStream* aHeadersDataStream,
                   bool aIsTrusted);

  NS_IMETHOD Run() {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mHandler->mScriptGlobal));
    nsAutoPopupStatePusher popupStatePusher(window, mPopupState);

    nsCxPusher pusher;
    if (mIsTrusted || pusher.Push(mContent)) {
      mHandler->OnLinkClickSync(mContent, mURI,
                                mTargetSpec.get(), mFileName,
                                mPostDataStream, mHeadersDataStream,
                                nullptr, nullptr);
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsDocShell>     mHandler;
  nsCOMPtr<nsIURI>         mURI;
  nsString                 mTargetSpec;
  nsString                mFileName;
  nsCOMPtr<nsIInputStream> mPostDataStream;
  nsCOMPtr<nsIInputStream> mHeadersDataStream;
  nsCOMPtr<nsIContent>     mContent;
  PopupControlState        mPopupState;
  bool                     mIsTrusted;
};

OnLinkClickEvent::OnLinkClickEvent(nsDocShell* aHandler,
                                   nsIContent *aContent,
                                   nsIURI* aURI,
                                   const PRUnichar* aTargetSpec,
                                   const nsAString& aFileName,
                                   nsIInputStream* aPostDataStream,
                                   nsIInputStream* aHeadersDataStream,
                                   bool aIsTrusted)
  : mHandler(aHandler)
  , mURI(aURI)
  , mTargetSpec(aTargetSpec)
  , mFileName(aFileName)
  , mPostDataStream(aPostDataStream)
  , mHeadersDataStream(aHeadersDataStream)
  , mContent(aContent)
  , mIsTrusted(aIsTrusted)
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mHandler->mScriptGlobal));

  mPopupState = window->GetPopupControlState();
}



NS_IMETHODIMP
nsDocShell::OnLinkClick(nsIContent* aContent,
                        nsIURI* aURI,
                        const PRUnichar* aTargetSpec,
                        const nsAString& aFileName,
                        nsIInputStream* aPostDataStream,
                        nsIInputStream* aHeadersDataStream,
                        bool aIsTrusted)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");

  if (!IsOKToLoadURI(aURI)) {
    return NS_OK;
  }

  
  
  
  
  
  
  if (ShouldBlockLoadingForBackButton()) {
    return NS_OK;
  }

  if (aContent->IsEditable()) {
    return NS_OK;
  }

  nsresult rv = NS_ERROR_FAILURE;
  nsAutoString target;

  nsCOMPtr<nsIWebBrowserChrome3> browserChrome3 = do_GetInterface(mTreeOwner);
  if (browserChrome3) {
    nsCOMPtr<nsIDOMNode> linkNode = do_QueryInterface(aContent);
    nsAutoString oldTarget(aTargetSpec);
    rv = browserChrome3->OnBeforeLinkTraversal(oldTarget, aURI,
                                               linkNode, mIsAppTab, target);
  }
  
  if (NS_FAILED(rv))
    target = aTargetSpec;  

  nsCOMPtr<nsIRunnable> ev =
      new OnLinkClickEvent(this, aContent, aURI, target.get(), aFileName, 
                           aPostDataStream, aHeadersDataStream, aIsTrusted);
  return NS_DispatchToCurrentThread(ev);
}

NS_IMETHODIMP
nsDocShell::OnLinkClickSync(nsIContent *aContent,
                            nsIURI* aURI,
                            const PRUnichar* aTargetSpec,
                            const nsAString& aFileName,
                            nsIInputStream* aPostDataStream,
                            nsIInputStream* aHeadersDataStream,
                            nsIDocShell** aDocShell,
                            nsIRequest** aRequest)
{
  
  if (aDocShell) {
    *aDocShell = nullptr;
  }
  if (aRequest) {
    *aRequest = nullptr;
  }

  if (!IsOKToLoadURI(aURI)) {
    return NS_OK;
  }

  
  
  
  if (nsGkAtoms::form == aContent->Tag() && ShouldBlockLoadingForBackButton()) {
    return NS_OK;
  }

  if (aContent->IsEditable()) {
    return NS_OK;
  }

  {
    
    nsCOMPtr<nsIExternalProtocolService> extProtService =
        do_GetService(NS_EXTERNALPROTOCOLSERVICE_CONTRACTID);
    if (extProtService) {
      nsAutoCString scheme;
      aURI->GetScheme(scheme);
      if (!scheme.IsEmpty()) {
        
        
        bool isExposed;
        nsresult rv = extProtService->IsExposedProtocol(scheme.get(), &isExposed);
        if (NS_SUCCEEDED(rv) && !isExposed) {
          return extProtService->LoadURI(aURI, this); 
        }
      }
    }
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> refererDoc = aContent->OwnerDoc();
  NS_ENSURE_TRUE(refererDoc, NS_ERROR_UNEXPECTED);

  
  
  
  nsPIDOMWindow* refererInner = refererDoc->GetInnerWindow();
  NS_ENSURE_TRUE(refererInner, NS_ERROR_UNEXPECTED);
  nsCOMPtr<nsPIDOMWindow> outerWindow = do_QueryInterface(mScriptGlobal);
  if (!outerWindow || outerWindow->GetCurrentInnerWindow() != refererInner) {
      
      return NS_OK;
  }

  nsCOMPtr<nsIURI> referer = refererDoc->GetDocumentURI();

  
  

  nsAutoString target(aTargetSpec);

  
  nsAutoString typeHint;
  nsCOMPtr<nsIDOMHTMLAnchorElement> anchor(do_QueryInterface(aContent));
  if (anchor) {
    anchor->GetType(typeHint);
    NS_ConvertUTF16toUTF8 utf8Hint(typeHint);
    nsAutoCString type, dummy;
    NS_ParseContentType(utf8Hint, type, dummy);
    CopyUTF8toUTF16(type, typeHint);
  }

  
  
  
  nsCOMPtr<nsIURI> clonedURI;
  aURI->Clone(getter_AddRefs(clonedURI));
  if (!clonedURI) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = InternalLoad(clonedURI,                 
                             referer,                   
                             aContent->NodePrincipal(), 
                                                        
                             INTERNAL_LOAD_FLAGS_NONE,
                             target.get(),              
                             NS_LossyConvertUTF16toASCII(typeHint).get(),
                             aFileName,                 
                             aPostDataStream,           
                             aHeadersDataStream,        
                             LOAD_LINK,                 
                             nullptr,                   
                             true,                      
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

  
  nsAutoCString charset;
  rv = aURI->GetOriginCharset(charset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString spec;
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

bool
nsDocShell::ShouldBlockLoadingForBackButton()
{
  if (!(mLoadType & LOAD_CMD_HISTORY) ||
      nsEventStateManager::IsHandlingUserInput() ||
      !Preferences::GetBool("accessibility.blockjsredirection")) {
    return false;
  }

  bool canGoForward = false;
  GetCanGoForward(&canGoForward);
  return canGoForward;
}

bool 
nsDocShell::PluginsAllowedInCurrentDoc()
{
  bool pluginsAllowed = false;

  if (!mContentViewer) {
    return false;
  }
  
  nsIDocument* doc = mContentViewer->GetDocument();
  if (!doc) {
    return false;
  }
  
  doc->GetAllowPlugins(&pluginsAllowed);
  return pluginsAllowed;
}






NS_IMETHODIMP
nsDocShell::ReloadDocument(const char* aCharset,
                           int32_t aSource)
{

  
  nsCOMPtr<nsIContentViewer> cv;
  NS_ENSURE_SUCCESS(GetContentViewer(getter_AddRefs(cv)), NS_ERROR_FAILURE);
  if (cv)
  {
    nsCOMPtr<nsIMarkupDocumentViewer> muDV = do_QueryInterface(cv);  
    if (muDV)
    {
      int32_t hint;
      muDV->GetHintCharacterSetSource(&hint);
      if (aSource > hint)
      {
        nsCString charset(aCharset);
        muDV->SetHintCharacterSet(charset);
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
nsDocShell::GetPrintPreview(nsIWebBrowserPrint** aPrintPreview)
{
  *aPrintPreview = nullptr;
#if NS_PRINT_PREVIEW
  nsCOMPtr<nsIDocumentViewerPrint> print = do_QueryInterface(mContentViewer);
  if (!print || !print->IsInitializedForPrintPreview()) {
    Stop(nsIWebNavigation::STOP_ALL);
    nsCOMPtr<nsIPrincipal> principal =
      do_CreateInstance("@mozilla.org/nullprincipal;1");
    NS_ENSURE_STATE(principal);
    nsresult rv = CreateAboutBlankContentViewer(principal, nullptr);
    NS_ENSURE_SUCCESS(rv, rv);
    print = do_QueryInterface(mContentViewer);
    NS_ENSURE_STATE(print);
    print->InitializeForPrintPreview();
  }
  nsCOMPtr<nsIWebBrowserPrint> result = do_QueryInterface(print);
  result.forget(aPrintPreview);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}


#ifdef DEBUG
unsigned long nsDocShell::gNumberOfDocShells = 0;
#endif

NS_IMETHODIMP
nsDocShell::GetCanExecuteScripts(bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = false; 

  nsCOMPtr<nsIDocShell> docshell = this;
  nsCOMPtr<nsIDocShellTreeItem> globalObjTreeItem =
      do_QueryInterface(docshell);

  if (globalObjTreeItem)
  {
      nsCOMPtr<nsIDocShellTreeItem> treeItem(globalObjTreeItem);
      nsCOMPtr<nsIDocShellTreeItem> parentItem;
      bool firstPass = true;
      bool lookForParents = false;

      
      do
      {
          nsresult rv = docshell->GetAllowJavascript(aResult);
          if (NS_FAILED(rv)) return rv;
          if (!*aResult) {
              nsDocShell* realDocshell = static_cast<nsDocShell*>(docshell.get());
              if (realDocshell->mContentViewer) {
                  nsIDocument* doc = realDocshell->mContentViewer->GetDocument();
                  if (doc && doc->HasFlag(NODE_IS_EDITABLE) &&
                      realDocshell->mEditorData) {
                      nsCOMPtr<nsIEditingSession> editSession;
                      realDocshell->mEditorData->GetEditingSession(getter_AddRefs(editSession));
                      bool jsDisabled = false;
                      if (editSession &&
                          NS_SUCCEEDED(rv = editSession->GetJsAndPluginsDisabled(&jsDisabled))) {
                          if (firstPass) {
                              if (jsDisabled) {
                                  
                                  
                                  return NS_OK;
                              }
                              
                              
                              
                              
                              *aResult = true;
                              break;
                          } else if (lookForParents && jsDisabled) {
                              
                              
                              *aResult = true;
                              break;
                          }
                          
                          
                          
                          *aResult = true;
                          return NS_OK;
                      }
                      NS_WARNING("The editing session does not work?");
                      return NS_FAILED(rv) ? rv : NS_ERROR_FAILURE;
                  }
                  if (firstPass) {
                      
                      
                      
                      lookForParents = true;
                  } else {
                      
                      
                      
                      return NS_OK;
                  }
              }
          } else if (lookForParents) {
              
              
              
              *aResult = false;
              return NS_OK;
          }
          firstPass = false;

          treeItem->GetParent(getter_AddRefs(parentItem));
          treeItem.swap(parentItem);
          docshell = do_QueryInterface(treeItem);
#ifdef DEBUG
          if (treeItem && !docshell) {
            NS_ERROR("cannot get a docshell from a treeItem!");
          }
#endif 
      } while (treeItem && docshell);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsApp(uint32_t aOwnAppId)
{
    mOwnOrContainingAppId = aOwnAppId;
    if (aOwnAppId != nsIScriptSecurityManager::NO_APP_ID &&
        aOwnAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
        mFrameType = eFrameTypeApp;
    } else {
        mFrameType = eFrameTypeRegular;
    }

    return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsBrowserInsideApp(uint32_t aContainingAppId)
{
    mOwnOrContainingAppId = aContainingAppId;
    mFrameType = eFrameTypeBrowser;
    return NS_OK;
}

 NS_IMETHODIMP
nsDocShell::GetIsBrowserElement(bool* aIsBrowser)
{
    *aIsBrowser = (mFrameType == eFrameTypeBrowser);
    return NS_OK;
}

 NS_IMETHODIMP
nsDocShell::GetIsApp(bool* aIsApp)
{
    *aIsApp = (mFrameType == eFrameTypeApp);
    return NS_OK;
}

 NS_IMETHODIMP
nsDocShell::GetIsBrowserOrApp(bool* aIsBrowserOrApp)
{
    switch (mFrameType) {
        case eFrameTypeRegular:
            *aIsBrowserOrApp = false;
            break;
        case eFrameTypeBrowser:
        case eFrameTypeApp:
            *aIsBrowserOrApp = true;
            break;
    }

    return NS_OK;
}

nsDocShell::FrameType
nsDocShell::GetInheritedFrameType()
{
    if (mFrameType != eFrameTypeRegular) {
        return mFrameType;
    }

    nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
    GetSameTypeParent(getter_AddRefs(parentAsItem));

    nsCOMPtr<nsIDocShell> parent = do_QueryInterface(parentAsItem);
    if (!parent) {
        return eFrameTypeRegular;
    }

    return static_cast<nsDocShell*>(parent.get())->GetInheritedFrameType();
}

 NS_IMETHODIMP
nsDocShell::GetIsInBrowserElement(bool* aIsInBrowserElement)
{
    *aIsInBrowserElement = (GetInheritedFrameType() == eFrameTypeBrowser);
    return NS_OK;
}

 NS_IMETHODIMP
nsDocShell::GetIsInBrowserOrApp(bool* aIsInBrowserOrApp)
{
    switch (GetInheritedFrameType()) {
        case eFrameTypeRegular:
            *aIsInBrowserOrApp = false;
            break;
        case eFrameTypeBrowser:
        case eFrameTypeApp:
            *aIsInBrowserOrApp = true;
            break;
    }

    return NS_OK;
}

 NS_IMETHODIMP
nsDocShell::GetAppId(uint32_t* aAppId)
{
    if (mOwnOrContainingAppId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
        *aAppId = mOwnOrContainingAppId;
        return NS_OK;
    }

    nsCOMPtr<nsIDocShell> parent;
    GetSameTypeParentIgnoreBrowserAndAppBoundaries(getter_AddRefs(parent));

    if (!parent) {
        *aAppId = nsIScriptSecurityManager::NO_APP_ID;
        return NS_OK;
    }

    return parent->GetAppId(aAppId);
}

NS_IMETHODIMP
nsDocShell::GetAsyncPanZoomEnabled(bool* aOut)
{
    if (TabChild* tabChild = GetTabChildFrom(this)) {
        *aOut = tabChild->IsAsyncPanZoomEnabled();
        return NS_OK;
    }
    *aOut = false;
    return NS_OK;
}

bool
nsDocShell::HasUnloadedParent()
{
    nsCOMPtr<nsIDocShellTreeItem> currentTreeItem = this;
    while (currentTreeItem) {
        nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
        currentTreeItem->GetParent(getter_AddRefs(parentTreeItem));
        nsCOMPtr<nsIDocShell> parent = do_QueryInterface(parentTreeItem);
        if (parent) {
            bool inUnload = false;
            parent->GetIsInUnload(&inUnload);
            if (inUnload) {
                return true;
            }
        }
        currentTreeItem.swap(parentTreeItem);
    }
    return false;
}
