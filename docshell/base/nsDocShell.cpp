





#include "nsDocShell.h"

#include <algorithm>

#include "mozilla/ArrayUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Casting.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/TabChild.h"
#include "mozilla/dom/ProfileTimelineMarkerBinding.h"
#include "mozilla/dom/ToJSValue.h"
#include "mozilla/EventStateManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/StartupTimeline.h"
#include "mozilla/Telemetry.h"
#include "mozilla/unused.h"
#include "mozilla/VisualEventTracer.h"
#include "URIUtils.h"

#include "nsIContent.h"
#include "nsIContentInlines.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsIDOMElement.h"

#include "nsIDOMStorage.h"
#include "nsIContentViewer.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsCURILoader.h"
#include "nsDocShellCID.h"
#include "nsDOMCID.h"
#include "nsNetUtil.h"
#include "mozilla/net/ReferrerPolicy.h"
#include "nsRect.h"
#include "prenv.h"
#include "nsIDOMWindow.h"
#include "nsIGlobalObject.h"
#include "nsIWebBrowserChrome.h"
#include "nsPoint.h"
#include "nsIObserverService.h"
#include "nsIPrompt.h"
#include "nsIAuthPrompt.h"
#include "nsIAuthPrompt2.h"
#include "nsIChannelEventSink.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIServiceWorkerManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsIScriptObjectPrincipal.h"
#include "nsIScrollableFrame.h"
#include "nsContentPolicyUtils.h" 
#include "nsISeekableStream.h"
#include "nsAutoPtr.h"
#include "nsIWritablePropertyBag2.h"
#include "nsIAppShell.h"
#include "nsWidgetsCID.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsIScriptChannel.h"
#include "nsITimedChannel.h"
#include "nsIPrivacyTransitionObserver.h"
#include "nsIReflowObserver.h"
#include "nsIScrollObserver.h"
#include "nsIDocShellTreeItem.h"
#include "nsIChannel.h"
#include "IHistory.h"
#include "nsViewSourceHandler.h"
#include "nsWhitespaceTokenizer.h"




#include "nsIHttpChannelInternal.h"
#include "nsPILoadGroupInternal.h"


#include "nsDocShellLoadInfo.h"
#include "nsCDefaultURIFixup.h"
#include "nsDocShellEnumerator.h"
#include "nsSHistory.h"
#include "nsDocShellEditorData.h"
#include "GeckoProfiler.h"


#include "nsError.h"
#include "nsEscape.h"


#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIWebProgress.h"
#include "nsILayoutHistoryState.h"
#include "nsITimer.h"
#include "nsISHistoryInternal.h"
#include "nsIPrincipal.h"
#include "nsNullPrincipal.h"
#include "nsISHEntry.h"
#include "nsIWindowWatcher.h"
#include "nsIPromptFactory.h"
#include "nsITransportSecurityInfo.h"
#include "nsINode.h"
#include "nsINSSErrorsService.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIApplicationCacheContainer.h"
#include "nsStreamUtils.h"
#include "nsIController.h"
#include "nsPICommandUpdater.h"
#include "nsIDOMHTMLAnchorElement.h"
#include "nsIWebBrowserChrome3.h"
#include "nsITabChild.h"
#include "nsISiteSecurityService.h"
#include "nsStructuredCloneContainer.h"
#include "nsIStructuredCloneContainer.h"
#ifdef MOZ_PLACES
#include "nsIFaviconService.h"
#include "mozIAsyncFavicons.h"
#endif
#include "nsINetworkPredictor.h"


#include "nsIEditingSession.h"

#include "nsPIDOMWindow.h"
#include "nsGlobalWindow.h"
#include "nsPIWindowRoot.h"
#include "nsICachingChannel.h"
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

#include "nsIFrame.h"
#include "nsSubDocumentFrame.h"


#include "nsIWebBrowserChromeFocus.h"

#if NS_PRINT_PREVIEW
#include "nsIDocumentViewerPrint.h"
#include "nsIWebBrowserPrint.h"
#endif

#include "nsContentUtils.h"
#include "nsIContentSecurityPolicy.h"
#include "nsILoadInfo.h"
#include "nsSandboxFlags.h"
#include "nsXULAppAPI.h"
#include "nsDOMNavigationTiming.h"
#include "nsISecurityUITelemetry.h"
#include "nsIAppsService.h"
#include "nsDSURIContentListener.h"
#include "nsDocShellLoadTypes.h"
#include "nsDocShellTransferableHooks.h"
#include "nsICommandManager.h"
#include "nsIDOMNode.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIHttpChannel.h"
#include "nsISHContainer.h"
#include "nsISHistory.h"
#include "nsISecureBrowserUI.h"
#include "nsIStringBundle.h"
#include "nsISupportsArray.h"
#include "nsIURIFixup.h"
#include "nsIURILoader.h"
#include "nsIWebBrowserFind.h"
#include "nsIWidget.h"
#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/URLSearchParams.h"
#include "nsPerformance.h"

#ifdef MOZ_TOOLKIT_SEARCH
#include "nsIBrowserSearchService.h"
#endif

static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#if defined(DEBUG_bryner) || defined(DEBUG_chb)

#define DEBUG_PAGE_CACHE
#endif

#ifdef XP_WIN
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h> 
#endif

using namespace mozilla;
using namespace mozilla::dom;


static bool gAddedPreferencesVarCache = false;

bool nsDocShell::sUseErrorPages = false;


static int32_t gNumberOfDocumentsLoading = 0;


static int32_t gDocShellCount = 0;


static uint32_t gNumberOfPrivateDocShells = 0;


nsIURIFixup* nsDocShell::sURIFixup = 0;




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
FavorPerformanceHint(bool aPerfOverStarvation)
{
  nsCOMPtr<nsIAppShell> appShell = do_GetService(kAppShellCID);
  if (appShell) {
    appShell->FavorPerformanceHint(
      aPerfOverStarvation,
      Preferences::GetUint("docshell.event_starvation_delay_hint",
                           NS_EVENT_STARVATION_DELAY_HINT));
  }
}





#define PREF_PINGS_ENABLED           "browser.send_pings"
#define PREF_PINGS_MAX_PER_LINK      "browser.send_pings.max_per_link"
#define PREF_PINGS_REQUIRE_SAME_HOST "browser.send_pings.require_same_host"

















static bool
PingsEnabled(int32_t* aMaxPerLink, bool* aRequireSameHost)
{
  bool allow = Preferences::GetBool(PREF_PINGS_ENABLED, false);

  *aMaxPerLink = 1;
  *aRequireSameHost = true;

  if (allow) {
    Preferences::GetInt(PREF_PINGS_MAX_PER_LINK, aMaxPerLink);
    Preferences::GetBool(PREF_PINGS_REQUIRE_SAME_HOST, aRequireSameHost);
  }

  return allow;
}

static bool
CheckPingURI(nsIURI* aURI, nsIContent* aContent)
{
  if (!aURI) {
    return false;
  }

  
  nsCOMPtr<nsIScriptSecurityManager> ssmgr =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
  NS_ENSURE_TRUE(ssmgr, false);

  nsresult rv = ssmgr->CheckLoadURIWithPrincipal(
    aContent->NodePrincipal(), aURI, nsIScriptSecurityManager::STANDARD);
  if (NS_FAILED(rv)) {
    return false;
  }

  
  bool match;
  if ((NS_FAILED(aURI->SchemeIs("http", &match)) || !match) &&
      (NS_FAILED(aURI->SchemeIs("https", &match)) || !match)) {
    return false;
  }

  
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_PING,
                                 aURI,
                                 aContent->NodePrincipal(),
                                 aContent,
                                 EmptyCString(), 
                                 nullptr, 
                                 &shouldLoad);
  return NS_SUCCEEDED(rv) && NS_CP_ACCEPTED(shouldLoad);
}

typedef void (*ForEachPingCallback)(void* closure, nsIContent* content,
                                    nsIURI* uri, nsIIOService* ios);

static bool
IsElementAnchor(nsIContent* aContent)
{
  
  
  return aContent->IsAnyOfHTMLElements(nsGkAtoms::a, nsGkAtoms::area);
}

static void
ForEachPing(nsIContent* aContent, ForEachPingCallback aCallback, void* aClosure)
{
  
  
  
  

  
  
  if (!IsElementAnchor(aContent)) {
    return;
  }

  nsCOMPtr<nsIAtom> pingAtom = do_GetAtom("ping");
  if (!pingAtom) {
    return;
  }

  nsAutoString value;
  aContent->GetAttr(kNameSpaceID_None, pingAtom, value);
  if (value.IsEmpty()) {
    return;
  }

  nsCOMPtr<nsIIOService> ios = do_GetIOService();
  if (!ios) {
    return;
  }

  nsIDocument* doc = aContent->OwnerDoc();

  nsWhitespaceTokenizer tokenizer(value);

  while (tokenizer.hasMoreTokens()) {
    nsCOMPtr<nsIURI> uri, baseURI = aContent->GetBaseURI();
    ios->NewURI(NS_ConvertUTF16toUTF8(tokenizer.nextToken()),
                doc->GetDocumentCharacterSet().get(),
                baseURI, getter_AddRefs(uri));
    if (CheckPingURI(uri, aContent)) {
      aCallback(aClosure, aContent, uri, ios);
    }
  }
}




#define PING_TIMEOUT 10000

static void
OnPingTimeout(nsITimer* aTimer, void* aClosure)
{
  nsILoadGroup* loadGroup = static_cast<nsILoadGroup*>(aClosure);
  if (loadGroup) {
    loadGroup->Cancel(NS_ERROR_ABORT);
  }
}


static bool
IsSameHost(nsIURI* aUri1, nsIURI* aUri2)
{
  nsAutoCString host1, host2;
  aUri1->GetAsciiHost(host1);
  aUri2->GetAsciiHost(host2);
  return host1.Equals(host2);
}

class nsPingListener final
  : public nsIStreamListener
  , public nsIInterfaceRequestor
  , public nsIChannelEventSink
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  nsPingListener(bool aRequireSameHost, nsIContent* aContent,
                 nsILoadGroup* aLoadGroup)
    : mRequireSameHost(aRequireSameHost)
    , mContent(aContent)
    , mLoadGroup(aLoadGroup)
  {
  }

  nsresult StartTimeout();

  void SetInterceptController(nsINetworkInterceptController* aInterceptController)
  {
    mInterceptController = aInterceptController;
  }

private:
  ~nsPingListener();

  bool mRequireSameHost;
  nsCOMPtr<nsIContent> mContent;
  nsCOMPtr<nsILoadGroup> mLoadGroup;
  nsCOMPtr<nsITimer> mTimer;
  nsCOMPtr<nsINetworkInterceptController> mInterceptController;
};

NS_IMPL_ISUPPORTS(nsPingListener, nsIStreamListener, nsIRequestObserver,
                  nsIInterfaceRequestor, nsIChannelEventSink)

nsPingListener::~nsPingListener()
{
  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
}

nsresult
nsPingListener::StartTimeout()
{
  nsCOMPtr<nsITimer> timer = do_CreateInstance(NS_TIMER_CONTRACTID);

  if (timer) {
    nsresult rv = timer->InitWithFuncCallback(OnPingTimeout, mLoadGroup,
                                              PING_TIMEOUT,
                                              nsITimer::TYPE_ONE_SHOT);
    if (NS_SUCCEEDED(rv)) {
      mTimer = timer;
      return NS_OK;
    }
  }

  return NS_ERROR_OUT_OF_MEMORY;
}

NS_IMETHODIMP
nsPingListener::OnStartRequest(nsIRequest* aRequest, nsISupports* aContext)
{
  return NS_OK;
}

NS_IMETHODIMP
nsPingListener::OnDataAvailable(nsIRequest* aRequest, nsISupports* aContext,
                                nsIInputStream* aStream, uint64_t aOffset,
                                uint32_t aCount)
{
  uint32_t result;
  return aStream->ReadSegments(NS_DiscardSegment, nullptr, aCount, &result);
}

NS_IMETHODIMP
nsPingListener::OnStopRequest(nsIRequest* aRequest, nsISupports* aContext,
                              nsresult aStatus)
{
  mLoadGroup = nullptr;

  if (mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsPingListener::GetInterface(const nsIID& aIID, void** aResult)
{
  if (aIID.Equals(NS_GET_IID(nsIChannelEventSink))) {
    nsCOMPtr<nsIChannelEventSink> copy(this);
    *aResult = copy.forget().take();
    return NS_OK;
  }

  if (aIID.Equals(NS_GET_IID(nsINetworkInterceptController)) &&
      mInterceptController) {
    nsCOMPtr<nsINetworkInterceptController> copy(mInterceptController);
    *aResult = copy.forget().take();
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsPingListener::AsyncOnChannelRedirect(nsIChannel* aOldChan,
                                       nsIChannel* aNewChan,
                                       uint32_t aFlags,
                                       nsIAsyncVerifyRedirectCallback* aCallback)
{
  nsCOMPtr<nsIURI> newURI;
  aNewChan->GetURI(getter_AddRefs(newURI));

  if (!CheckPingURI(newURI, mContent)) {
    return NS_ERROR_ABORT;
  }

  if (!mRequireSameHost) {
    aCallback->OnRedirectVerifyCallback(NS_OK);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIURI> oldURI;
  aOldChan->GetURI(getter_AddRefs(oldURI));
  NS_ENSURE_STATE(oldURI && newURI);

  if (!IsSameHost(oldURI, newURI)) {
    return NS_ERROR_ABORT;
  }

  aCallback->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}

struct MOZ_STACK_CLASS SendPingInfo
{
  int32_t numPings;
  int32_t maxPings;
  bool requireSameHost;
  nsIURI* target;
  nsIURI* referrer;
  nsIDocShell* docShell;
  uint32_t referrerPolicy;
};

static void
SendPing(void* aClosure, nsIContent* aContent, nsIURI* aURI,
         nsIIOService* aIOService)
{
  SendPingInfo* info = static_cast<SendPingInfo*>(aClosure);
  if (info->maxPings > -1 && info->numPings >= info->maxPings) {
    return;
  }

  if (info->requireSameHost) {
    
    
    if (!IsSameHost(aURI, info->referrer)) {
      return;
    }
  }

  nsIDocument* doc = aContent->OwnerDoc();

  nsCOMPtr<nsIChannel> chan;
  NS_NewChannel(getter_AddRefs(chan),
                aURI,
                doc,
                nsILoadInfo::SEC_NORMAL,
                nsIContentPolicy::TYPE_PING,
                nullptr, 
                nullptr, 
                nsIRequest::LOAD_NORMAL, 
                aIOService);

  if (!chan) {
    return;
  }

  
  chan->SetLoadFlags(nsIRequest::INHIBIT_CACHING);

  nsCOMPtr<nsIHttpChannel> httpChan = do_QueryInterface(chan);
  if (!httpChan) {
    return;
  }

  
  nsCOMPtr<nsIHttpChannelInternal> httpInternal = do_QueryInterface(httpChan);
  if (httpInternal) {
    httpInternal->SetDocumentURI(doc->GetDocumentURI());
  }

  httpChan->SetRequestMethod(NS_LITERAL_CSTRING("POST"));

  
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept"),
                             EmptyCString(), false);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-language"),
                             EmptyCString(), false);
  httpChan->SetRequestHeader(NS_LITERAL_CSTRING("accept-encoding"),
                             EmptyCString(), false);

  
  nsAutoCString pingTo;
  if (NS_SUCCEEDED(info->target->GetSpec(pingTo))) {
    httpChan->SetRequestHeader(NS_LITERAL_CSTRING("Ping-To"), pingTo, false);
  }

  nsCOMPtr<nsIScriptSecurityManager> sm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);

  if (sm && info->referrer) {
    bool referrerIsSecure;
    uint32_t flags = nsIProtocolHandler::URI_SAFE_TO_LOAD_IN_SECURE_CONTEXT;
    nsresult rv = NS_URIChainHasFlags(info->referrer, flags, &referrerIsSecure);

    
    referrerIsSecure = NS_FAILED(rv) || referrerIsSecure;

    bool sameOrigin =
      NS_SUCCEEDED(sm->CheckSameOriginURI(info->referrer, aURI, false));

    
    
    
    
    if (sameOrigin || !referrerIsSecure) {
      nsAutoCString pingFrom;
      if (NS_SUCCEEDED(info->referrer->GetSpec(pingFrom))) {
        httpChan->SetRequestHeader(NS_LITERAL_CSTRING("Ping-From"), pingFrom,
                                   false);
      }
    }

    
    
    
    if (!sameOrigin && !referrerIsSecure) {
      httpChan->SetReferrerWithPolicy(info->referrer, info->referrerPolicy);
    }
  }

  nsCOMPtr<nsIUploadChannel2> uploadChan = do_QueryInterface(httpChan);
  if (!uploadChan) {
    return;
  }

  NS_NAMED_LITERAL_CSTRING(uploadData, "PING");

  nsCOMPtr<nsIInputStream> uploadStream;
  NS_NewPostDataStream(getter_AddRefs(uploadStream), false, uploadData);
  if (!uploadStream) {
    return;
  }

  uploadChan->ExplicitSetUploadStream(uploadStream,
                                      NS_LITERAL_CSTRING("text/ping"),
                                      uploadData.Length(),
                                      NS_LITERAL_CSTRING("POST"), false);

  
  
  nsCOMPtr<nsILoadGroup> loadGroup = do_CreateInstance(NS_LOADGROUP_CONTRACTID);
  if (!loadGroup) {
    return;
  }
  chan->SetLoadGroup(loadGroup);

  
  
  
  nsPingListener* pingListener =
    new nsPingListener(info->requireSameHost, aContent, loadGroup);

  nsCOMPtr<nsINetworkInterceptController> interceptController = do_QueryInterface(info->docShell);
  pingListener->SetInterceptController(interceptController);
  nsCOMPtr<nsIStreamListener> listener(pingListener);

  
  nsCOMPtr<nsIInterfaceRequestor> callbacks = do_QueryInterface(listener);
  NS_ASSERTION(callbacks, "oops");
  loadGroup->SetNotificationCallbacks(callbacks);

  chan->AsyncOpen(listener, nullptr);

  
  
  
  info->numPings++;

  
  if (NS_FAILED(pingListener->StartTimeout())) {
    
    
    chan->Cancel(NS_ERROR_ABORT);
  }
}


static void
DispatchPings(nsIDocShell* aDocShell,
              nsIContent* aContent,
              nsIURI* aTarget,
              nsIURI* aReferrer,
              uint32_t aReferrerPolicy)
{
  SendPingInfo info;

  if (!PingsEnabled(&info.maxPings, &info.requireSameHost)) {
    return;
  }
  if (info.maxPings == 0) {
    return;
  }

  info.numPings = 0;
  info.target = aTarget;
  info.referrer = aReferrer;
  info.referrerPolicy = aReferrerPolicy;
  info.docShell = aDocShell;

  ForEachPing(aContent, SendPing, &info);
}

static nsDOMPerformanceNavigationType
ConvertLoadTypeToNavigationType(uint32_t aLoadType)
{
  
  if (aLoadType == 0) {
    aLoadType = LOAD_NORMAL;
  }

  auto result = dom::PerformanceNavigation::TYPE_RESERVED;
  switch (aLoadType) {
    case LOAD_NORMAL:
    case LOAD_NORMAL_EXTERNAL:
    case LOAD_NORMAL_BYPASS_CACHE:
    case LOAD_NORMAL_BYPASS_PROXY:
    case LOAD_NORMAL_BYPASS_PROXY_AND_CACHE:
    case LOAD_NORMAL_REPLACE:
    case LOAD_NORMAL_ALLOW_MIXED_CONTENT:
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

static nsISHEntry* GetRootSHEntry(nsISHEntry* aEntry);

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
  if (!gNumberOfPrivateDocShells) {
    if (XRE_GetProcessType() == GeckoProcessType_Content) {
      dom::ContentChild* cc = dom::ContentChild::GetSingleton();
      cc->SendPrivateDocShellsExist(false);
      return;
    }

    nsCOMPtr<nsIObserverService> obsvc = services::GetObserverService();
    if (obsvc) {
      obsvc->NotifyObservers(nullptr, "last-pb-context-exited", nullptr);
    }
  }
}





static uint64_t gDocshellIDCounter = 0;

nsDocShell::nsDocShell()
  : nsDocLoader()
  , mDefaultScrollbarPref(Scrollbar_Auto, Scrollbar_Auto)
  , mTreeOwner(nullptr)
  , mChromeEventHandler(nullptr)
  , mCharsetReloadState(eCharsetReloadInit)
  , mChildOffset(0)
  , mBusyFlags(BUSY_FLAGS_NONE)
  , mAppType(nsIDocShell::APP_TYPE_UNKNOWN)
  , mLoadType(0)
  , mMarginWidth(-1)
  , mMarginHeight(-1)
  , mItemType(typeContent)
  , mPreviousTransIndex(-1)
  , mLoadedTransIndex(-1)
  , mSandboxFlags(0)
  , mFullscreenAllowed(CHECK_ATTRIBUTES)
  , mCreated(false)
  , mAllowSubframes(true)
  , mAllowPlugins(true)
  , mAllowJavascript(true)
  , mAllowMetaRedirects(true)
  , mAllowImages(true)
  , mAllowMedia(true)
  , mAllowDNSPrefetch(true)
  , mAllowWindowControl(true)
  , mAllowContentRetargeting(true)
  , mAllowContentRetargetingOnChildren(true)
  , mCreatingDocument(false)
  , mUseErrorPages(false)
  , mObserveErrorPages(true)
  , mAllowAuth(true)
  , mAllowKeywordFixup(false)
  , mIsOffScreenBrowser(false)
  , mIsActive(true)
  , mIsPrerendered(false)
  , mIsAppTab(false)
  , mUseGlobalHistory(false)
  , mInPrivateBrowsing(false)
  , mUseRemoteTabs(false)
  , mDeviceSizeIsPageSize(false)
  , mWindowDraggingAllowed(false)
  , mCanExecuteScripts(false)
  , mFiredUnloadEvent(false)
  , mEODForCurrentDocument(false)
  , mURIResultedInDocument(false)
  , mIsBeingDestroyed(false)
  , mIsExecutingOnLoadHandler(false)
  , mIsPrintingOrPP(false)
  , mSavingOldViewer(false)
#ifdef DEBUG
  , mInEnsureScriptEnv(false)
#endif
  , mAffectPrivateSessionLifetime(true)
  , mInvisible(false)
  , mHasLoadedNonBlankURI(false)
  , mDefaultLoadFlags(nsIRequest::LOAD_NORMAL)
  , mBlankTiming(false)
  , mFrameType(eFrameTypeRegular)
  , mOwnOrContainingAppId(nsIScriptSecurityManager::UNKNOWN_APP_ID)
  , mParentCharsetSource(0)
  , mJSRunToCompletionDepth(0)
{
  mHistoryID = ++gDocshellIDCounter;
  if (gDocShellCount++ == 0) {
    NS_ASSERTION(sURIFixup == nullptr,
                 "Huh, sURIFixup not null in first nsDocShell ctor!");

    CallGetService(NS_URIFIXUP_CONTRACTID, &sURIFixup);
  }

#ifdef PR_LOGGING
#ifdef DEBUG
  if (!gDocShellLog) {
    gDocShellLog = PR_NewLogModule("nsDocShell");
  }
#endif
  if (!gDocShellLeakLog) {
    gDocShellLeakLog = PR_NewLogModule("nsDocShellLeak");
  }
  if (gDocShellLeakLog) {
    PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p created\n", this));
  }
#endif

#ifdef DEBUG
  
  ++gNumberOfDocShells;
  if (!PR_GetEnv("MOZ_QUIET")) {
    printf_stderr("++DOCSHELL %p == %ld [pid = %d] [id = %llu]\n",
                  (void*)this,
                  gNumberOfDocShells,
                  getpid(),
                  AssertedCast<unsigned long long>(mHistoryID));
  }
#endif
}

nsDocShell::~nsDocShell()
{
  MOZ_ASSERT(!mProfileTimelineRecording);

  Destroy();

  nsCOMPtr<nsISHistoryInternal> shPrivate(do_QueryInterface(mSessionHistory));
  if (shPrivate) {
    shPrivate->SetRootDocShell(nullptr);
  }

  if (--gDocShellCount == 0) {
    NS_IF_RELEASE(sURIFixup);
  }

#ifdef PR_LOGGING
  if (gDocShellLeakLog) {
    PR_LOG(gDocShellLeakLog, PR_LOG_DEBUG, ("DOCSHELL %p destroyed\n", this));
  }
#endif

#ifdef DEBUG
  
  --gNumberOfDocShells;
  if (!PR_GetEnv("MOZ_QUIET")) {
    printf_stderr("--DOCSHELL %p == %ld [pid = %d] [id = %llu]\n",
                  (void*)this,
                  gNumberOfDocShells,
                  getpid(),
                  AssertedCast<unsigned long long>(mHistoryID));
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

  
  
  nsCOMPtr<nsIInterfaceRequestor> proxy =
    new InterfaceRequestorProxy(static_cast<nsIInterfaceRequestor*>(this));
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
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    shell = do_QueryObject(iter.GetNext());
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
  NS_INTERFACE_MAP_ENTRY(nsIWebNavigation)
  NS_INTERFACE_MAP_ENTRY(nsIBaseWindow)
  NS_INTERFACE_MAP_ENTRY(nsIScrollable)
  NS_INTERFACE_MAP_ENTRY(nsITextScroll)
  NS_INTERFACE_MAP_ENTRY(nsIDocCharset)
  NS_INTERFACE_MAP_ENTRY(nsIRefreshURI)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
  NS_INTERFACE_MAP_ENTRY(nsIContentViewerContainer)
  NS_INTERFACE_MAP_ENTRY(nsIWebPageDescriptor)
  NS_INTERFACE_MAP_ENTRY(nsIAuthPromptProvider)
  NS_INTERFACE_MAP_ENTRY(nsILoadContext)
  NS_INTERFACE_MAP_ENTRY(nsIWebShellServices)
  NS_INTERFACE_MAP_ENTRY(nsILinkHandler)
  NS_INTERFACE_MAP_ENTRY(nsIClipboardCommands)
  NS_INTERFACE_MAP_ENTRY(nsIDOMStorageManager)
  NS_INTERFACE_MAP_ENTRY(nsINetworkInterceptController)
NS_INTERFACE_MAP_END_INHERITING(nsDocLoader)




NS_IMETHODIMP
nsDocShell::GetInterface(const nsIID& aIID, void** aSink)
{
  NS_PRECONDITION(aSink, "null out param");

  *aSink = nullptr;

  if (aIID.Equals(NS_GET_IID(nsICommandManager))) {
    NS_ENSURE_SUCCESS(EnsureCommandHandler(), NS_ERROR_FAILURE);
    *aSink = mCommandManager;
  } else if (aIID.Equals(NS_GET_IID(nsIURIContentListener))) {
    *aSink = mContentListener;
  } else if ((aIID.Equals(NS_GET_IID(nsIScriptGlobalObject)) ||
              aIID.Equals(NS_GET_IID(nsIGlobalObject)) ||
              aIID.Equals(NS_GET_IID(nsPIDOMWindow)) ||
              aIID.Equals(NS_GET_IID(nsIDOMWindow)) ||
              aIID.Equals(NS_GET_IID(nsIDOMWindowInternal))) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
    return mScriptGlobal->QueryInterface(aIID, aSink);
  } else if (aIID.Equals(NS_GET_IID(nsIDOMDocument)) &&
             NS_SUCCEEDED(EnsureContentViewer())) {
    mContentViewer->GetDOMDocument((nsIDOMDocument**)aSink);
    return *aSink ? NS_OK : NS_NOINTERFACE;
  } else if (aIID.Equals(NS_GET_IID(nsIDocument)) &&
             NS_SUCCEEDED(EnsureContentViewer())) {
    nsCOMPtr<nsIDocument> doc = mContentViewer->GetDocument();
    doc.forget(aSink);
    return *aSink ? NS_OK : NS_NOINTERFACE;
  } else if (aIID.Equals(NS_GET_IID(nsIApplicationCacheContainer))) {
    *aSink = nullptr;

    

    nsCOMPtr<nsIContentViewer> contentViewer;
    GetContentViewer(getter_AddRefs(contentViewer));
    if (!contentViewer) {
      return NS_ERROR_NO_INTERFACE;
    }

    nsCOMPtr<nsIDOMDocument> domDoc;
    contentViewer->GetDOMDocument(getter_AddRefs(domDoc));
    NS_ASSERTION(domDoc, "Should have a document.");
    if (!domDoc) {
      return NS_ERROR_NO_INTERFACE;
    }

#if defined(PR_LOGGING) && defined(DEBUG)
    PR_LOG(gDocShellLog, PR_LOG_DEBUG,
           ("nsDocShell[%p]: returning app cache container %p",
            this, domDoc.get()));
#endif
    return domDoc->QueryInterface(aIID, aSink);
  } else if (aIID.Equals(NS_GET_IID(nsIPrompt)) &&
             NS_SUCCEEDED(EnsureScriptEnvironment())) {
    nsresult rv;
    nsCOMPtr<nsIWindowWatcher> wwatch =
      do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsIPrompt* prompt;
    rv = wwatch->GetNewPrompter(mScriptGlobal, &prompt);
    NS_ENSURE_SUCCESS(rv, rv);

    *aSink = prompt;
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIAuthPrompt)) ||
             aIID.Equals(NS_GET_IID(nsIAuthPrompt2))) {
    return NS_SUCCEEDED(GetAuthPrompt(PROMPT_NORMAL, aIID, aSink)) ?
      NS_OK : NS_NOINTERFACE;
  } else if (aIID.Equals(NS_GET_IID(nsISHistory))) {
    nsCOMPtr<nsISHistory> shistory;
    nsresult rv = GetSessionHistory(getter_AddRefs(shistory));
    if (NS_SUCCEEDED(rv) && shistory) {
      *aSink = shistory;
      NS_ADDREF((nsISupports*)*aSink);
      return NS_OK;
    }
    return NS_NOINTERFACE;
  } else if (aIID.Equals(NS_GET_IID(nsIWebBrowserFind))) {
    nsresult rv = EnsureFind();
    if (NS_FAILED(rv)) {
      return rv;
    }

    *aSink = mFind;
    NS_ADDREF((nsISupports*)*aSink);
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsIEditingSession)) &&
             NS_SUCCEEDED(EnsureEditorData())) {
    nsCOMPtr<nsIEditingSession> editingSession;
    mEditorData->GetEditingSession(getter_AddRefs(editingSession));
    if (editingSession) {
      *aSink = editingSession;
      NS_ADDREF((nsISupports*)*aSink);
      return NS_OK;
    }

    return NS_NOINTERFACE;
  } else if (aIID.Equals(NS_GET_IID(nsIClipboardDragDropHookList)) &&
             NS_SUCCEEDED(EnsureTransferableHookData())) {
    *aSink = mTransferableHookData;
    NS_ADDREF((nsISupports*)*aSink);
    return NS_OK;
  } else if (aIID.Equals(NS_GET_IID(nsISelectionDisplay))) {
    nsIPresShell* shell = GetPresShell();
    if (shell) {
      return shell->QueryInterface(aIID, aSink);
    }
  } else if (aIID.Equals(NS_GET_IID(nsIDocShellTreeOwner))) {
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    nsresult rv = GetTreeOwner(getter_AddRefs(treeOwner));
    if (NS_SUCCEEDED(rv) && treeOwner) {
      return treeOwner->QueryInterface(aIID, aSink);
    }
  } else if (aIID.Equals(NS_GET_IID(nsITabChild))) {
    nsCOMPtr<nsIDocShellTreeOwner> treeOwner;
    nsresult rv = GetTreeOwner(getter_AddRefs(treeOwner));
    if (NS_SUCCEEDED(rv) && treeOwner) {
      nsCOMPtr<nsIInterfaceRequestor> ir = do_QueryInterface(treeOwner);
      if (ir) {
        return ir->GetInterface(aIID, aSink);
      }
    }
  } else if (aIID.Equals(NS_GET_IID(nsIContentFrameMessageManager))) {
    nsCOMPtr<nsITabChild> tabChild =
      do_GetInterface(static_cast<nsIDocShell*>(this));
    nsCOMPtr<nsIContentFrameMessageManager> mm;
    if (tabChild) {
      tabChild->GetMessageManager(getter_AddRefs(mm));
    } else {
      nsCOMPtr<nsPIDOMWindow> win = GetWindow();
      if (win) {
        mm = do_QueryInterface(win->GetParentTarget());
      }
    }
    *aSink = mm.get();
  } else {
    return nsDocLoader::GetInterface(aIID, aSink);
  }

  NS_IF_ADDREF(((nsISupports*)*aSink));
  return *aSink ? NS_OK : NS_NOINTERFACE;
}

uint32_t
nsDocShell::ConvertDocShellLoadInfoToLoadType(
    nsDocShellInfoLoadType aDocShellLoadType)
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
    case nsIDocShellLoadInfo::loadNormalAllowMixedContent:
      loadType = LOAD_NORMAL_ALLOW_MIXED_CONTENT;
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
    case nsIDocShellLoadInfo::loadReloadMixedContent:
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
    case LOAD_NORMAL_ALLOW_MIXED_CONTENT:
      docShellLoadType = nsIDocShellLoadInfo::loadNormalAllowMixedContent;
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
      docShellLoadType = nsIDocShellLoadInfo::loadReloadMixedContent;
      break;
    default:
      NS_NOTREACHED("Unexpected load type value");
  }

  return docShellLoadType;
}




NS_IMETHODIMP
nsDocShell::LoadURI(nsIURI* aURI,
                    nsIDocShellLoadInfo* aLoadInfo,
                    uint32_t aLoadFlags,
                    bool aFirstParty)
{
  NS_PRECONDITION(aLoadInfo || (aLoadFlags & EXTRA_LOAD_FLAGS) == 0,
                  "Unexpected flags");
  NS_PRECONDITION((aLoadFlags & 0xf) == 0, "Should not have these flags set");

  
  
  
  
  
  
  if (!IsNavigationAllowed(true, false)) {
    return NS_OK; 
  }

  if (DoAppRedirectIfNeeded(aURI, aLoadInfo, aFirstParty)) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> referrer;
  nsCOMPtr<nsIInputStream> postStream;
  nsCOMPtr<nsIInputStream> headersStream;
  nsCOMPtr<nsISupports> owner;
  bool inheritOwner = false;
  bool ownerIsExplicit = false;
  bool sendReferrer = true;
  uint32_t referrerPolicy = mozilla::net::RP_Default;
  bool isSrcdoc = false;
  nsCOMPtr<nsISHEntry> shEntry;
  nsXPIDLString target;
  nsAutoString srcdoc;
  nsCOMPtr<nsIDocShell> sourceDocShell;
  nsCOMPtr<nsIURI> baseURI;

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
    aLoadInfo->GetReferrerPolicy(&referrerPolicy);
    aLoadInfo->GetIsSrcdocLoad(&isSrcdoc);
    aLoadInfo->GetSrcdocData(srcdoc);
    aLoadInfo->GetSourceDocShell(getter_AddRefs(sourceDocShell));
    aLoadInfo->GetBaseURI(getter_AddRefs(baseURI));
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

    if (parentDS && parentDS != static_cast<nsIDocShell*>(this)) {
      








      
      parentDS->GetLoadType(&parentLoadType);

      
      nsCOMPtr<nsISHEntry> currentSH;
      bool oshe = false;
      parentDS->GetCurrentSHEntry(getter_AddRefs(currentSH), &oshe);
      bool dynamicallyAddedChild = mDynamicallyCreated;
      if (!dynamicallyAddedChild && !oshe && currentSH) {
        currentSH->HasDynamicallyAddedChild(&dynamicallyAddedChild);
      }
      if (!dynamicallyAddedChild) {
        
        
        parentDS->GetChildSHEntry(mChildOffset, getter_AddRefs(shEntry));
      }

      
      
      if (!mCurrentURI) {
        
        
        if (shEntry && (parentLoadType == LOAD_NORMAL ||
                        parentLoadType == LOAD_LINK   ||
                        parentLoadType == LOAD_NORMAL_EXTERNAL)) {
          
          
          
          
          
          
          bool inOnLoadHandler = false;
          parentDS->GetIsExecutingOnLoadHandler(&inOnLoadHandler);
          if (inOnLoadHandler) {
            loadType = LOAD_NORMAL_REPLACE;
            shEntry = nullptr;
          }
        } else if (parentLoadType == LOAD_REFRESH) {
          
          
          shEntry = nullptr;
        } else if ((parentLoadType == LOAD_BYPASS_HISTORY) ||
                   (shEntry &&
                    ((parentLoadType & LOAD_CMD_HISTORY) ||
                     (parentLoadType == LOAD_RELOAD_NORMAL) ||
                     (parentLoadType == LOAD_RELOAD_CHARSET_CHANGE)))) {
          
          
          
          
          loadType = parentLoadType;
        } else if (parentLoadType == LOAD_ERROR_PAGE) {
          
          
          
          loadType = LOAD_BYPASS_HISTORY;
        } else if ((parentLoadType == LOAD_RELOAD_BYPASS_CACHE) ||
                   (parentLoadType == LOAD_RELOAD_BYPASS_PROXY) ||
                   (parentLoadType == LOAD_RELOAD_BYPASS_PROXY_AND_CACHE)) {
          
          
          loadType = parentLoadType;
        }
      } else {
        
        
        
        
        
        
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
    else {
      
      
      
      bool inOnLoadHandler = false;
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

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (owner && mItemType != typeChrome) {
    nsCOMPtr<nsIPrincipal> ownerPrincipal = do_QueryInterface(owner);
    if (nsContentUtils::IsSystemOrExpandedPrincipal(ownerPrincipal)) {
      if (ownerIsExplicit) {
        return NS_ERROR_DOM_SECURITY_ERR;
      }
      owner = nullptr;
      inheritOwner = true;
    }
  }
  if (!owner && !inheritOwner && !ownerIsExplicit) {
    
    inheritOwner = nsContentUtils::IsCallerChrome();
  }

  if (aLoadFlags & LOAD_FLAGS_DISALLOW_INHERIT_OWNER) {
    inheritOwner = false;
    owner = nsNullPrincipal::Create();
  }

  uint32_t flags = 0;

  if (inheritOwner) {
    flags |= INTERNAL_LOAD_FLAGS_INHERIT_OWNER;
  }

  if (!sendReferrer) {
    flags |= INTERNAL_LOAD_FLAGS_DONT_SEND_REFERRER;
  }

  if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) {
    flags |= INTERNAL_LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
  }

  if (aLoadFlags & LOAD_FLAGS_FIRST_LOAD) {
    flags |= INTERNAL_LOAD_FLAGS_FIRST_LOAD;
  }

  if (aLoadFlags & LOAD_FLAGS_BYPASS_CLASSIFIER) {
    flags |= INTERNAL_LOAD_FLAGS_BYPASS_CLASSIFIER;
  }

  if (aLoadFlags & LOAD_FLAGS_FORCE_ALLOW_COOKIES) {
    flags |= INTERNAL_LOAD_FLAGS_FORCE_ALLOW_COOKIES;
  }

  if (isSrcdoc) {
    flags |= INTERNAL_LOAD_FLAGS_IS_SRCDOC;
  }

  return InternalLoad(aURI,
                      referrer,
                      referrerPolicy,
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
                      srcdoc,
                      sourceDocShell,
                      baseURI,
                      nullptr,  
                      nullptr); 
}

NS_IMETHODIMP
nsDocShell::LoadStream(nsIInputStream* aStream, nsIURI* aURI,
                       const nsACString& aContentType,
                       const nsACString& aContentCharset,
                       nsIDocShellLoadInfo* aLoadInfo)
{
  NS_ENSURE_ARG(aStream);

  mAllowKeywordFixup = false;

  
  
  
  nsCOMPtr<nsIURI> uri = aURI;
  if (!uri) {
    
    nsresult rv = NS_OK;
    uri = do_CreateInstance(NS_SIMPLEURI_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
    
    
    rv = uri->SetSpec(NS_LITERAL_CSTRING("internal:load-stream"));
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  uint32_t loadType = LOAD_NORMAL;
  if (aLoadInfo) {
    nsDocShellInfoLoadType lt = nsIDocShellLoadInfo::loadNormal;
    (void)aLoadInfo->GetLoadType(&lt);
    
    loadType = ConvertDocShellLoadInfoToLoadType(lt);
  }

  NS_ENSURE_SUCCESS(Stop(nsIWebNavigation::STOP_NETWORK), NS_ERROR_FAILURE);

  mLoadType = loadType;

  nsCOMPtr<nsISupports> owner;
  aLoadInfo->GetOwner(getter_AddRefs(owner));
  nsCOMPtr<nsIPrincipal> requestingPrincipal = do_QueryInterface(owner);
  if (!requestingPrincipal) {
    requestingPrincipal = nsContentUtils::GetSystemPrincipal();
  }

  
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = NS_NewInputStreamChannel(getter_AddRefs(channel),
                                         uri,
                                         aStream,
                                         requestingPrincipal,
                                         nsILoadInfo::SEC_NORMAL,
                                         nsIContentPolicy::TYPE_OTHER,
                                         aContentType,
                                         aContentCharset);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

  nsCOMPtr<nsIURILoader> uriLoader(do_GetService(NS_URI_LOADER_CONTRACTID));
  NS_ENSURE_TRUE(uriLoader, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(DoChannelLoad(channel, uriLoader, false),
                    NS_ERROR_FAILURE);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::CreateLoadInfo(nsIDocShellLoadInfo** aLoadInfo)
{
  nsDocShellLoadInfo* loadInfo = new nsDocShellLoadInfo();
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

void
nsDocShell::MaybeInitTiming()
{
  if (mTiming && !mBlankTiming) {
    return;
  }

  if (mScriptGlobal && mBlankTiming) {
    nsPIDOMWindow* innerWin = mScriptGlobal->GetCurrentInnerWindow();
    if (innerWin && innerWin->GetPerformance()) {
      mTiming = innerWin->GetPerformance()->GetDOMTiming();
      mBlankTiming = false;
    }
  }

  if (!mTiming) {
    mTiming = new nsDOMNavigationTiming();
  }

  mTiming->NotifyNavigationStart();
}










 bool
nsDocShell::ValidateOrigin(nsIDocShellTreeItem* aOriginTreeItem,
                           nsIDocShellTreeItem* aTargetTreeItem)
{
  
  
  if (nsContentUtils::GetCurrentJSContext() &&
      nsContentUtils::IsCallerChrome()) {
    return true;
  }

  MOZ_ASSERT(aOriginTreeItem && aTargetTreeItem, "need two docshells");

  
  nsCOMPtr<nsIDocument> originDocument = aOriginTreeItem->GetDocument();
  NS_ENSURE_TRUE(originDocument, false);

  
  nsCOMPtr<nsIDocument> targetDocument = aTargetTreeItem->GetDocument();
  NS_ENSURE_TRUE(targetDocument, false);

  bool equal;
  nsresult rv = originDocument->NodePrincipal()->Equals(
    targetDocument->NodePrincipal(), &equal);
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
  if (NS_SUCCEEDED(rv) && originURI) {
    innerOriginURI = NS_GetInnermostURI(originURI);
  }

  rv = targetDocument->NodePrincipal()->GetURI(getter_AddRefs(targetURI));
  if (NS_SUCCEEDED(rv) && targetURI) {
    innerTargetURI = NS_GetInnermostURI(targetURI);
  }

  return innerOriginURI && innerTargetURI &&
         NS_SUCCEEDED(innerOriginURI->SchemeIs("file", &originIsFile)) &&
         NS_SUCCEEDED(innerTargetURI->SchemeIs("file", &targetIsFile)) &&
         originIsFile && targetIsFile;
}

nsresult
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
nsDocShell::GetPresContext(nsPresContext** aPresContext)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  *aPresContext = nullptr;

  if (!mContentViewer) {
    return NS_OK;
  }

  return mContentViewer->GetPresContext(aPresContext);
}

NS_IMETHODIMP_(nsIPresShell*)
nsDocShell::GetPresShell()
{
  nsRefPtr<nsPresContext> presContext;
  (void)GetPresContext(getter_AddRefs(presContext));
  return presContext ? presContext->GetPresShell() : nullptr;
}

NS_IMETHODIMP
nsDocShell::GetEldestPresShell(nsIPresShell** aPresShell)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(aPresShell);
  *aPresShell = nullptr;

  nsRefPtr<nsPresContext> presContext;
  (void)GetEldestPresContext(getter_AddRefs(presContext));

  if (presContext) {
    NS_IF_ADDREF(*aPresShell = presContext->GetPresShell());
  }

  return rv;
}

NS_IMETHODIMP
nsDocShell::GetContentViewer(nsIContentViewer** aContentViewer)
{
  NS_ENSURE_ARG_POINTER(aContentViewer);

  *aContentViewer = mContentViewer;
  NS_IF_ADDREF(*aContentViewer);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetChromeEventHandler(nsIDOMEventTarget* aChromeEventHandler)
{
  
  nsCOMPtr<EventTarget> handler = do_QueryInterface(aChromeEventHandler);
  mChromeEventHandler = handler.get();

  if (mScriptGlobal) {
    mScriptGlobal->SetChromeEventHandler(mChromeEventHandler);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChromeEventHandler(nsIDOMEventTarget** aChromeEventHandler)
{
  NS_ENSURE_ARG_POINTER(aChromeEventHandler);
  nsCOMPtr<EventTarget> handler = mChromeEventHandler;
  handler.forget(aChromeEventHandler);
  return NS_OK;
}


NS_IMETHODIMP
nsDocShell::SetCurrentURI(nsIURI* aURI)
{
  
  
  SetCurrentURI(aURI, nullptr, true, 0);
  return NS_OK;
}

bool
nsDocShell::SetCurrentURI(nsIURI* aURI, nsIRequest* aRequest,
                          bool aFireOnLocationChange, uint32_t aLocationFlags)
{
#ifdef PR_LOGGING
  if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
    nsAutoCString spec;
    if (aURI) {
      aURI->GetSpec(spec);
    }
    PR_LogPrint("DOCSHELL %p SetCurrentURI %s\n", this, spec.get());
  }
#endif

  
  
  if (mLoadType == LOAD_ERROR_PAGE) {
    return false;
  }

  mCurrentURI = NS_TryToMakeImmutable(aURI);

  if (!NS_IsAboutBlank(mCurrentURI)) {
    mHasLoadedNonBlankURI = true;
  }

  bool isRoot = false;  
  bool isSubFrame = false;  

  nsCOMPtr<nsIDocShellTreeItem> root;

  GetSameTypeRootTreeItem(getter_AddRefs(root));
  if (root.get() == static_cast<nsIDocShellTreeItem*>(this)) {
    
    isRoot = true;
  }
  if (mLSHE) {
    mLSHE->GetIsSubFrame(&isSubFrame);
  }

  
  
  if (!mURLSearchParams) {
    mURLSearchParams = new URLSearchParams();
  }

  nsAutoCString search;

  nsCOMPtr<nsIURL> url(do_QueryInterface(mCurrentURI));
  if (url) {
    nsresult rv = url->GetQuery(search);
    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to get the query from a nsIURL.");
    }
  }

  mURLSearchParams->ParseInput(search, nullptr);

  if (!isSubFrame && !isRoot) {
    




    return false;
  }

  if (aFireOnLocationChange) {
    FireOnLocationChange(this, aRequest, aURI, aLocationFlags);
  }
  return !aFireOnLocationChange;
}

NS_IMETHODIMP
nsDocShell::GetCharset(nsACString& aCharset)
{
  aCharset.Truncate();

  nsIPresShell* presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);
  nsIDocument* doc = presShell->GetDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
  aCharset = doc->GetDocumentCharacterSet();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GatherCharsetMenuTelemetry()
{
  nsCOMPtr<nsIContentViewer> viewer;
  GetContentViewer(getter_AddRefs(viewer));
  if (!viewer) {
    return NS_OK;
  }

  nsIDocument* doc = viewer->GetDocument();
  if (!doc || doc->WillIgnoreCharsetOverride()) {
    return NS_OK;
  }

  Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_USED, true);

  bool isFileURL = false;
  nsIURI* url = doc->GetOriginalURI();
  if (url) {
    url->SchemeIs("file", &isFileURL);
  }

  int32_t charsetSource = doc->GetDocumentCharacterSetSource();
  switch (charsetSource) {
    case kCharsetFromTopLevelDomain:
      
      Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 7);
      break;
    case kCharsetFromFallback:
    case kCharsetFromDocTypeDefault:
    case kCharsetFromCache:
    case kCharsetFromParentFrame:
    case kCharsetFromHintPrevDoc:
      
      if (isFileURL) {
        Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 0);
      } else {
        Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 1);
      }
      break;
    case kCharsetFromAutoDetection:
      
      if (isFileURL) {
        Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 2);
      } else {
        Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 3);
      }
      break;
    case kCharsetFromMetaPrescan:
    case kCharsetFromMetaTag:
    case kCharsetFromChannel:
      
      Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 4);
      break;
    case kCharsetFromParentForced:
    case kCharsetFromUserForced:
      
      Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 5);
      break;
    case kCharsetFromIrreversibleAutoDetection:
    case kCharsetFromOtherComponent:
    case kCharsetFromByteOrderMark:
    case kCharsetUninitialized:
    default:
      
      Telemetry::Accumulate(Telemetry::CHARSET_OVERRIDE_SITUATION, 6);
      break;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetCharset(const nsACString& aCharset)
{
  
  return SetForcedCharset(aCharset);
}

NS_IMETHODIMP
nsDocShell::SetForcedCharset(const nsACString& aCharset)
{
  if (aCharset.IsEmpty()) {
    mForcedCharset.Truncate();
    return NS_OK;
  }
  nsAutoCString encoding;
  if (!EncodingUtils::FindEncodingForLabel(aCharset, encoding)) {
    
    return NS_ERROR_INVALID_ARG;
  }
  if (!EncodingUtils::IsAsciiCompatible(encoding)) {
    
    return NS_ERROR_INVALID_ARG;
  }
  mForcedCharset = encoding;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetForcedCharset(nsACString& aResult)
{
  aResult = mForcedCharset;
  return NS_OK;
}

void
nsDocShell::SetParentCharset(const nsACString& aCharset,
                             int32_t aCharsetSource,
                             nsIPrincipal* aPrincipal)
{
  mParentCharset = aCharset;
  mParentCharsetSource = aCharsetSource;
  mParentCharsetPrincipal = aPrincipal;
}

void
nsDocShell::GetParentCharset(nsACString& aCharset,
                             int32_t* aCharsetSource,
                             nsIPrincipal** aPrincipal)
{
  aCharset = mParentCharset;
  *aCharsetSource = mParentCharsetSource;
  NS_IF_ADDREF(*aPrincipal = mParentCharsetPrincipal);
}

NS_IMETHODIMP
nsDocShell::GetChannelIsUnsafe(bool* aUnsafe)
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
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasMixedActiveContentLoaded = doc && doc->GetHasMixedActiveContentLoaded();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedActiveContentBlocked(bool* aHasMixedActiveContentBlocked)
{
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasMixedActiveContentBlocked =
    doc && doc->GetHasMixedActiveContentBlocked();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedDisplayContentLoaded(bool* aHasMixedDisplayContentLoaded)
{
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasMixedDisplayContentLoaded =
    doc && doc->GetHasMixedDisplayContentLoaded();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasMixedDisplayContentBlocked(
  bool* aHasMixedDisplayContentBlocked)
{
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasMixedDisplayContentBlocked =
    doc && doc->GetHasMixedDisplayContentBlocked();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasTrackingContentBlocked(bool* aHasTrackingContentBlocked)
{
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasTrackingContentBlocked = doc && doc->GetHasTrackingContentBlocked();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasTrackingContentLoaded(bool* aHasTrackingContentLoaded)
{
  nsCOMPtr<nsIDocument> doc(GetDocument());
  *aHasTrackingContentLoaded = doc && doc->GetHasTrackingContentLoaded();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowPlugins(bool* aAllowPlugins)
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
nsDocShell::GetAllowJavascript(bool* aAllowJavascript)
{
  NS_ENSURE_ARG_POINTER(aAllowJavascript);

  *aAllowJavascript = mAllowJavascript;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowJavascript(bool aAllowJavascript)
{
  mAllowJavascript = aAllowJavascript;
  RecomputeCanExecuteScripts();
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
    NS_LITERAL_CSTRING("Internal API Used"),
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

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsILoadContext> shell = do_QueryObject(iter.GetNext());
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
nsDocShell::GetHasLoadedNonBlankURI(bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = mHasLoadedNonBlankURI;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetUseRemoteTabs(bool* aUseRemoteTabs)
{
  NS_ENSURE_ARG_POINTER(aUseRemoteTabs);

  *aUseRemoteTabs = mUseRemoteTabs;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetRemoteTabs(bool aUseRemoteTabs)
{
#ifdef MOZ_CRASHREPORTER
  if (aUseRemoteTabs) {
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("DOMIPCEnabled"),
                                       NS_LITERAL_CSTRING("1"));
  }
#endif

  mUseRemoteTabs = aUseRemoteTabs;
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

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> shell = do_QueryObject(iter.GetNext());
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
nsDocShell::AddWeakPrivacyTransitionObserver(
    nsIPrivacyTransitionObserver* aObserver)
{
  nsWeakPtr weakObs = do_GetWeakReference(aObserver);
  if (!weakObs) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  return mPrivacyObservers.AppendElement(weakObs) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::AddWeakReflowObserver(nsIReflowObserver* aObserver)
{
  nsWeakPtr weakObs = do_GetWeakReference(aObserver);
  if (!weakObs) {
    return NS_ERROR_FAILURE;
  }
  return mReflowObservers.AppendElement(weakObs) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::RemoveWeakReflowObserver(nsIReflowObserver* aObserver)
{
  nsWeakPtr obs = do_GetWeakReference(aObserver);
  return mReflowObservers.RemoveElement(obs) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::NotifyReflowObservers(bool aInterruptible,
                                  DOMHighResTimeStamp aStart,
                                  DOMHighResTimeStamp aEnd)
{
  nsTObserverArray<nsWeakPtr>::ForwardIterator iter(mReflowObservers);
  while (iter.HasMore()) {
    nsWeakPtr ref = iter.GetNext();
    nsCOMPtr<nsIReflowObserver> obs = do_QueryReferent(ref);
    if (!obs) {
      mReflowObservers.RemoveElement(ref);
    } else if (aInterruptible) {
      obs->ReflowInterruptible(aStart, aEnd);
    } else {
      obs->Reflow(aStart, aEnd);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowMetaRedirects(bool* aReturn)
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

NS_IMETHODIMP
nsDocShell::SetAllowMetaRedirects(bool aValue)
{
  mAllowMetaRedirects = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowSubframes(bool* aAllowSubframes)
{
  NS_ENSURE_ARG_POINTER(aAllowSubframes);

  *aAllowSubframes = mAllowSubframes;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowSubframes(bool aAllowSubframes)
{
  mAllowSubframes = aAllowSubframes;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowImages(bool* aAllowImages)
{
  NS_ENSURE_ARG_POINTER(aAllowImages);

  *aAllowImages = mAllowImages;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowImages(bool aAllowImages)
{
  mAllowImages = aAllowImages;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowMedia(bool* aAllowMedia)
{
  *aAllowMedia = mAllowMedia;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowMedia(bool aAllowMedia)
{
  mAllowMedia = aAllowMedia;

  
  if (mScriptGlobal) {
    nsPIDOMWindow* innerWin = mScriptGlobal->GetCurrentInnerWindow();
    if (innerWin) {
      if (aAllowMedia) {
        innerWin->UnmuteAudioContexts();
      } else {
        innerWin->MuteAudioContexts();
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowDNSPrefetch(bool* aAllowDNSPrefetch)
{
  *aAllowDNSPrefetch = mAllowDNSPrefetch;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowDNSPrefetch(bool aAllowDNSPrefetch)
{
  mAllowDNSPrefetch = aAllowDNSPrefetch;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowWindowControl(bool* aAllowWindowControl)
{
  *aAllowWindowControl = mAllowWindowControl;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowWindowControl(bool aAllowWindowControl)
{
  mAllowWindowControl = aAllowWindowControl;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowContentRetargeting(bool* aAllowContentRetargeting)
{
  *aAllowContentRetargeting = mAllowContentRetargeting;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowContentRetargeting(bool aAllowContentRetargeting)
{
  mAllowContentRetargetingOnChildren = aAllowContentRetargeting;
  mAllowContentRetargeting = aAllowContentRetargeting;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowContentRetargetingOnChildren(bool* aAllowContentRetargetingOnChildren)
{
  *aAllowContentRetargetingOnChildren = mAllowContentRetargetingOnChildren;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetAllowContentRetargetingOnChildren(bool aAllowContentRetargetingOnChildren)
{
  mAllowContentRetargetingOnChildren = aAllowContentRetargetingOnChildren;
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

  
  
  
  
  nsCOMPtr<nsPIDOMWindow> win = GetWindow();
  if (!win) {
    return NS_OK;
  }
  nsCOMPtr<Element> frameElement = win->GetFrameElementInternal();
  if (frameElement &&
      frameElement->IsHTMLElement(nsGkAtoms::iframe) &&
      !frameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::allowfullscreen) &&
      !frameElement->HasAttr(kNameSpaceID_None, nsGkAtoms::mozallowfullscreen)) {
    return NS_OK;
  }

  
  
  
  nsRefPtr<nsDocShell> parent = GetParentDocshell();
  if (!parent) {
    *aFullscreenAllowed = true;
    return NS_OK;
  }

  
  
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
nsDocShell::GetMayEnableCharacterEncodingMenu(
    bool* aMayEnableCharacterEncodingMenu)
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
nsDocShell::GetDocShellEnumerator(int32_t aItemType, int32_t aDirection,
                                  nsISimpleEnumerator** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  nsRefPtr<nsDocShellEnumerator> docShellEnum;
  if (aDirection == ENUMERATE_FORWARDS) {
    docShellEnum = new nsDocShellForwardsEnumerator;
  } else {
    docShellEnum = new nsDocShellBackwardsEnumerator;
  }

  if (!docShellEnum) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsresult rv = docShellEnum->SetEnumDocShellType(aItemType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = docShellEnum->SetEnumerationRootItem((nsIDocShellTreeItem*)this);
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = docShellEnum->First();
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = docShellEnum->QueryInterface(NS_GET_IID(nsISimpleEnumerator),
                                    (void**)aResult);

  return rv;
}

NS_IMETHODIMP
nsDocShell::GetAppType(uint32_t* aAppType)
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
nsDocShell::GetAllowAuth(bool* aAllowAuth)
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
nsDocShell::GetZoom(float* aZoom)
{
  NS_ENSURE_ARG_POINTER(aZoom);
  *aZoom = 1.0f;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetZoom(float aZoom)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetMarginWidth(int32_t* aWidth)
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
nsDocShell::GetMarginHeight(int32_t* aHeight)
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
nsDocShell::GetBusyFlags(uint32_t* aBusyFlags)
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
    if (aForward) {
      *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusNextElement());
    } else {
      *aTookFocus = NS_SUCCEEDED(chromeFocus->FocusPrevElement());
    }
  } else {
    *aTookFocus = false;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSecurityUI(nsISecureBrowserUI** aSecurityUI)
{
  NS_IF_ADDREF(*aSecurityUI = mSecurityUI);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetSecurityUI(nsISecureBrowserUI* aSecurityUI)
{
  mSecurityUI = aSecurityUI;
  mSecurityUI->SetDocShell(this);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetUseErrorPages(bool* aUseErrorPages)
{
  *aUseErrorPages = UseErrorPages();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetUseErrorPages(bool aUseErrorPages)
{
  
  if (mObserveErrorPages) {
    mObserveErrorPages = false;
  }
  mUseErrorPages = aUseErrorPages;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPreviousTransIndex(int32_t* aPreviousTransIndex)
{
  *aPreviousTransIndex = mPreviousTransIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLoadedTransIndex(int32_t* aLoadedTransIndex)
{
  *aLoadedTransIndex = mLoadedTransIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::HistoryPurged(int32_t aNumEntries)
{
  
  
  
  
  
  mPreviousTransIndex = std::max(-1, mPreviousTransIndex - aNumEntries);
  mLoadedTransIndex = std::max(0, mLoadedTransIndex - aNumEntries);

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> shell = do_QueryObject(iter.GetNext());
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

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> shell = do_QueryObject(iter.GetNext());
    if (shell) {
      static_cast<nsDocShell*>(shell.get())->HistoryTransactionRemoved(aIndex);
    }
  }

  return NS_OK;
}

unsigned long nsDocShell::gProfileTimelineRecordingsCount = 0;

NS_IMETHODIMP
nsDocShell::SetRecordProfileTimelineMarkers(bool aValue)
{
  bool currentValue = nsIDocShell::GetRecordProfileTimelineMarkers();
  if (currentValue != aValue) {
    if (aValue) {
      ++gProfileTimelineRecordingsCount;
      UseEntryScriptProfiling();
      mProfileTimelineRecording = true;
    } else {
      --gProfileTimelineRecordingsCount;
      UnuseEntryScriptProfiling();
      mProfileTimelineRecording = false;
      ClearProfileTimelineMarkers();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRecordProfileTimelineMarkers(bool* aValue)
{
  *aValue = mProfileTimelineRecording;
  return NS_OK;
}

nsresult
nsDocShell::PopProfileTimelineMarkers(
    JSContext* aCx,
    JS::MutableHandle<JS::Value> aProfileTimelineMarkers)
{
  
  
  
  
  
  
  

  nsTArray<mozilla::dom::ProfileTimelineMarker> profileTimelineMarkers;
  SequenceRooter<mozilla::dom::ProfileTimelineMarker> rooter(
    aCx, &profileTimelineMarkers);

  
  
  
  nsTArray<UniquePtr<TimelineMarker>> keptMarkers;

  for (uint32_t i = 0; i < mProfileTimelineMarkers.Length(); ++i) {
    UniquePtr<TimelineMarker>& startPayload = mProfileTimelineMarkers[i];
    const char* startMarkerName = startPayload->GetName();

    bool hasSeenPaintedLayer = false;
    bool isPaint = strcmp(startMarkerName, "Paint") == 0;

    
    
    dom::Sequence<dom::ProfileTimelineLayerRect> layerRectangles;

    if (startPayload->GetMetaData() == TRACING_INTERVAL_START) {
      bool hasSeenEnd = false;

      
      
      
      uint32_t markerDepth = 0;

      
      
      
      for (uint32_t j = i + 1; j < mProfileTimelineMarkers.Length(); ++j) {
        UniquePtr<TimelineMarker>& endPayload = mProfileTimelineMarkers[j];
        const char* endMarkerName = endPayload->GetName();

        
        if (isPaint && strcmp(endMarkerName, "Layer") == 0) {
          hasSeenPaintedLayer = true;
          endPayload->AddLayerRectangles(layerRectangles);
        }

        if (!startPayload->Equals(*endPayload)) {
          continue;
        }

        
        if (endPayload->GetMetaData() == TRACING_INTERVAL_START) {
          ++markerDepth;
        } else if (endPayload->GetMetaData() == TRACING_INTERVAL_END) {
          if (markerDepth > 0) {
            --markerDepth;
          } else {
            
            if (!isPaint || (isPaint && hasSeenPaintedLayer)) {
              mozilla::dom::ProfileTimelineMarker* marker =
                profileTimelineMarkers.AppendElement();

              marker->mName = NS_ConvertUTF8toUTF16(startPayload->GetName());
              marker->mStart = startPayload->GetTime();
              marker->mEnd = endPayload->GetTime();
              marker->mStack = startPayload->GetStack();
              if (isPaint) {
                marker->mRectangles.Construct(layerRectangles);
              }
              startPayload->AddDetails(*marker);
              endPayload->AddDetails(*marker);
            }

            
            hasSeenEnd = true;

            break;
          }
        }
      }

      
      if (!hasSeenEnd) {
        keptMarkers.AppendElement(Move(mProfileTimelineMarkers[i]));
        mProfileTimelineMarkers.RemoveElementAt(i);
        --i;
      }
    }
  }

  mProfileTimelineMarkers.SwapElements(keptMarkers);

  if (!ToJSValue(aCx, profileTimelineMarkers, aProfileTimelineMarkers)) {
    JS_ClearPendingException(aCx);
    return NS_ERROR_UNEXPECTED;
  }

  return NS_OK;
}

nsresult
nsDocShell::Now(DOMHighResTimeStamp* aWhen)
{
  bool ignore;
  *aWhen =
    (TimeStamp::Now() - TimeStamp::ProcessCreation(ignore)).ToMilliseconds();
  return NS_OK;
}

void
nsDocShell::AddProfileTimelineMarker(const char* aName,
                                     TracingMetadata aMetaData)
{
  if (mProfileTimelineRecording) {
    TimelineMarker* marker = new TimelineMarker(this, aName, aMetaData);
    mProfileTimelineMarkers.AppendElement(marker);
  }
}

void
nsDocShell::AddProfileTimelineMarker(UniquePtr<TimelineMarker>&& aMarker)
{
  if (mProfileTimelineRecording) {
    mProfileTimelineMarkers.AppendElement(Move(aMarker));
  }
}

NS_IMETHODIMP
nsDocShell::SetWindowDraggingAllowed(bool aValue)
{
  nsRefPtr<nsDocShell> parent = GetParentDocshell();
  if (!aValue && mItemType == typeChrome && !parent) {
    
    
    return NS_ERROR_FAILURE;
  }
  mWindowDraggingAllowed = aValue;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetWindowDraggingAllowed(bool* aValue)
{
  
  
  
  nsRefPtr<nsDocShell> parent = GetParentDocshell();
  if (mItemType == typeChrome && !parent) {
    
    *aValue = true;
  } else {
    *aValue = mWindowDraggingAllowed;
  }
  return NS_OK;
}

void
nsDocShell::ClearProfileTimelineMarkers()
{
  mProfileTimelineMarkers.Clear();
}

nsIDOMStorageManager*
nsDocShell::TopSessionStorageManager()
{
  nsresult rv;

  nsCOMPtr<nsIDocShellTreeItem> topItem;
  rv = GetSameTypeRootTreeItem(getter_AddRefs(topItem));
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  if (!topItem) {
    return nullptr;
  }

  nsDocShell* topDocShell = static_cast<nsDocShell*>(topItem.get());
  if (topDocShell != this) {
    return topDocShell->TopSessionStorageManager();
  }

  if (!mSessionStorageManager) {
    mSessionStorageManager =
      do_CreateInstance("@mozilla.org/dom/sessionStorage-manager;1");
  }

  return mSessionStorageManager;
}

NS_IMETHODIMP
nsDocShell::GetSessionStorageForPrincipal(nsIPrincipal* aPrincipal,
                                          const nsAString& aDocumentURI,
                                          bool aCreate,
                                          nsIDOMStorage** aStorage)
{
  nsCOMPtr<nsIDOMStorageManager> manager = TopSessionStorageManager();
  if (!manager) {
    return NS_ERROR_UNEXPECTED;
  }

  nsCOMPtr<nsIDOMWindow> domWin = do_GetInterface(GetAsSupports(this));

  if (aCreate) {
    return manager->CreateStorage(domWin, aPrincipal, aDocumentURI,
                                  mInPrivateBrowsing, aStorage);
  }

  return manager->GetStorage(domWin, aPrincipal, mInPrivateBrowsing, aStorage);
}

nsresult
nsDocShell::AddSessionStorage(nsIPrincipal* aPrincipal, nsIDOMStorage* aStorage)
{
  nsRefPtr<DOMStorage> storage = static_cast<DOMStorage*>(aStorage);
  if (!storage) {
    return NS_ERROR_UNEXPECTED;
  }

  nsIPrincipal* storagePrincipal = storage->GetPrincipal();
  if (storagePrincipal != aPrincipal) {
    NS_ERROR("Wanting to add a sessionStorage for different principal");
    return NS_ERROR_DOM_SECURITY_ERR;
  }

  nsCOMPtr<nsIDOMStorageManager> manager = TopSessionStorageManager();
  if (!manager) {
    return NS_ERROR_UNEXPECTED;
  }

  return manager->CloneStorage(aStorage);
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
nsDocShell::AddWeakScrollObserver(nsIScrollObserver* aObserver)
{
  nsWeakPtr weakObs = do_GetWeakReference(aObserver);
  if (!weakObs) {
    return NS_ERROR_FAILURE;
  }
  return mScrollObservers.AppendElement(weakObs) ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::RemoveWeakScrollObserver(nsIScrollObserver* aObserver)
{
  nsWeakPtr obs = do_GetWeakReference(aObserver);
  return mScrollObservers.RemoveElement(obs) ? NS_OK : NS_ERROR_FAILURE;
}

void
nsDocShell::NotifyAsyncPanZoomStarted()
{
  nsTObserverArray<nsWeakPtr>::ForwardIterator iter(mScrollObservers);
  while (iter.HasMore()) {
    nsWeakPtr ref = iter.GetNext();
    nsCOMPtr<nsIScrollObserver> obs = do_QueryReferent(ref);
    if (obs) {
      obs->AsyncPanZoomStarted();
    } else {
      mScrollObservers.RemoveElement(ref);
    }
  }

  
  for (uint32_t i = 0; i < mChildList.Length(); ++i) {
    nsCOMPtr<nsIDocShell> kid = do_QueryInterface(ChildAt(i));
    if (kid) {
      nsDocShell* docShell = static_cast<nsDocShell*>(kid.get());
      docShell->NotifyAsyncPanZoomStarted();
    }
  }
}

void
nsDocShell::NotifyAsyncPanZoomStopped()
{
  nsTObserverArray<nsWeakPtr>::ForwardIterator iter(mScrollObservers);
  while (iter.HasMore()) {
    nsWeakPtr ref = iter.GetNext();
    nsCOMPtr<nsIScrollObserver> obs = do_QueryReferent(ref);
    if (obs) {
      obs->AsyncPanZoomStopped();
    } else {
      mScrollObservers.RemoveElement(ref);
    }
  }

  
  for (uint32_t i = 0; i < mChildList.Length(); ++i) {
    nsCOMPtr<nsIDocShell> kid = do_QueryInterface(ChildAt(i));
    if (kid) {
      nsDocShell* docShell = static_cast<nsDocShell*>(kid.get());
      docShell->NotifyAsyncPanZoomStopped();
    }
  }
}

NS_IMETHODIMP
nsDocShell::NotifyScrollObservers()
{
  nsTObserverArray<nsWeakPtr>::ForwardIterator iter(mScrollObservers);
  while (iter.HasMore()) {
    nsWeakPtr ref = iter.GetNext();
    nsCOMPtr<nsIScrollObserver> obs = do_QueryReferent(ref);
    if (obs) {
      obs->ScrollPositionChanged();
    } else {
      mScrollObservers.RemoveElement(ref);
    }
  }
  return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetName(nsAString& aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetName(const nsAString& aName)
{
  mName = aName;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::NameEquals(const char16_t* aName, bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aName);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = mName.Equals(aName);
  return NS_OK;
}

 int32_t
nsDocShell::ItemType()
{
  return mItemType;
}

NS_IMETHODIMP
nsDocShell::GetItemType(int32_t* aItemType)
{
  NS_ENSURE_ARG_POINTER(aItemType);

  *aItemType = ItemType();
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
    presContext->UpdateIsChrome();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParent(nsIDocShellTreeItem** aParent)
{
  if (!mParent) {
    *aParent = nullptr;
  } else {
    CallQueryInterface(mParent, aParent);
  }
  
  
  return NS_OK;
}

already_AddRefed<nsDocShell>
nsDocShell::GetParentDocshell()
{
  nsCOMPtr<nsIDocShell> docshell = do_QueryInterface(GetAsSupports(mParent));
  return docshell.forget().downcast<nsDocShell>();
}

void
nsDocShell::RecomputeCanExecuteScripts()
{
  bool old = mCanExecuteScripts;
  nsRefPtr<nsDocShell> parent = GetParentDocshell();

  
  
  
  
  
  
  
  
  if (!mTreeOwner) {
    mCanExecuteScripts = mCanExecuteScripts && mAllowJavascript;
    
  } else if (!mAllowJavascript) {
    mCanExecuteScripts = false;
    
  } else if (parent) {
    mCanExecuteScripts = parent->mCanExecuteScripts;
    
    
  } else {
    mCanExecuteScripts = true;
  }

  
  
  
  if (mScriptGlobal && mScriptGlobal->GetGlobalJSObject()) {
    xpc::Scriptability& scriptability =
      xpc::Scriptability::Get(mScriptGlobal->GetGlobalJSObject());
    scriptability.SetDocShellAllowsScript(mCanExecuteScripts);
  }

  
  
  if (old != mCanExecuteScripts) {
    nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
    while (iter.HasMore()) {
      static_cast<nsDocShell*>(iter.GetNext())->RecomputeCanExecuteScripts();
    }
  }
}

nsresult
nsDocShell::SetDocLoaderParent(nsDocLoader* aParent)
{
  bool wasFrame = IsFrame();

  nsDocLoader::SetDocLoaderParent(aParent);

  nsCOMPtr<nsISupportsPriority> priorityGroup = do_QueryInterface(mLoadGroup);
  if (wasFrame != IsFrame() && priorityGroup) {
    priorityGroup->AdjustPriority(wasFrame ? -1 : 1);
  }

  
  nsISupports* parent = GetAsSupports(aParent);

  
  
  bool value;
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(parent));
  if (parentAsDocShell) {
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowPlugins(&value))) {
      SetAllowPlugins(value);
    }
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowJavascript(&value))) {
      SetAllowJavascript(value);
    }
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowMetaRedirects(&value))) {
      SetAllowMetaRedirects(value);
    }
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowSubframes(&value))) {
      SetAllowSubframes(value);
    }
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowImages(&value))) {
      SetAllowImages(value);
    }
    SetAllowMedia(parentAsDocShell->GetAllowMedia());
    if (NS_SUCCEEDED(parentAsDocShell->GetAllowWindowControl(&value))) {
      SetAllowWindowControl(value);
    }
    SetAllowContentRetargeting(parentAsDocShell->GetAllowContentRetargetingOnChildren());
    if (NS_SUCCEEDED(parentAsDocShell->GetIsActive(&value))) {
      SetIsActive(value);
    }
    if (parentAsDocShell->GetIsPrerendered()) {
      SetIsPrerendered(true);
    }
    if (NS_FAILED(parentAsDocShell->GetAllowDNSPrefetch(&value))) {
      value = false;
    }
    SetAllowDNSPrefetch(value);
    value = parentAsDocShell->GetAffectPrivateSessionLifetime();
    SetAffectPrivateSessionLifetime(value);
    uint32_t flags;
    if (NS_SUCCEEDED(parentAsDocShell->GetDefaultLoadFlags(&flags))) {
      SetDefaultLoadFlags(flags);
    }
  }

  nsCOMPtr<nsILoadContext> parentAsLoadContext(do_QueryInterface(parent));
  if (parentAsLoadContext &&
      NS_SUCCEEDED(parentAsLoadContext->GetUsePrivateBrowsing(&value))) {
    SetPrivateBrowsing(value);
  }

  nsCOMPtr<nsIURIContentListener> parentURIListener(do_GetInterface(parent));
  if (parentURIListener) {
    mContentListener->SetParentContentListener(parentURIListener);
  }

  
  RecomputeCanExecuteScripts();

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSameTypeParent(nsIDocShellTreeItem** aParent)
{
  NS_ENSURE_ARG_POINTER(aParent);
  *aParent = nullptr;

  if (nsIDocShell::GetIsBrowserOrApp()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDocShellTreeItem> parent =
    do_QueryInterface(GetAsSupports(mParent));
  if (!parent) {
    return NS_OK;
  }

  if (parent->ItemType() == mItemType) {
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
  if (!parent) {
    return NS_OK;
  }

  if (parent->ItemType() == mItemType) {
    nsCOMPtr<nsIDocShell> parentDS = do_QueryInterface(parent);
    parentDS.forget(aParent);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRootTreeItem(nsIDocShellTreeItem** aRootTreeItem)
{
  NS_ENSURE_ARG_POINTER(aRootTreeItem);

  nsRefPtr<nsDocShell> root = this;
  nsRefPtr<nsDocShell> parent = root->GetParentDocshell();
  while (parent) {
    root = parent;
    parent = root->GetParentDocshell();
  }

  root.forget(aRootTreeItem);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetSameTypeRootTreeItem(nsIDocShellTreeItem** aRootTreeItem)
{
  NS_ENSURE_ARG_POINTER(aRootTreeItem);
  *aRootTreeItem = static_cast<nsIDocShellTreeItem*>(this);

  nsCOMPtr<nsIDocShellTreeItem> parent;
  NS_ENSURE_SUCCESS(GetSameTypeParent(getter_AddRefs(parent)),
                    NS_ERROR_FAILURE);
  while (parent) {
    *aRootTreeItem = parent;
    NS_ENSURE_SUCCESS(
      (*aRootTreeItem)->GetSameTypeParent(getter_AddRefs(parent)),
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

  nsCOMPtr<nsIDOMWindow> targetWindow = aTargetItem->GetWindow();
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
ItemIsActive(nsIDocShellTreeItem* aItem)
{
  nsCOMPtr<nsIDOMWindow> window = aItem->GetWindow();

  if (window) {
    bool isClosed;

    if (NS_SUCCEEDED(window->GetClosed(&isClosed)) && !isClosed) {
      return true;
    }
  }

  return false;
}

NS_IMETHODIMP
nsDocShell::FindItemWithName(const char16_t* aName,
                             nsISupports* aRequestor,
                             nsIDocShellTreeItem* aOriginalRequestor,
                             nsIDocShellTreeItem** aResult)
{
  NS_ENSURE_ARG(aName);
  NS_ENSURE_ARG_POINTER(aResult);

  
  *aResult = nullptr;

  if (!*aName) {
    return NS_OK;
  }

  if (aRequestor) {
    
    
    return DoFindItemWithName(aName, aRequestor, aOriginalRequestor, aResult);
  } else {
    
    
    

    nsCOMPtr<nsIDocShellTreeItem> foundItem;
    nsDependentString name(aName);
    if (name.LowerCaseEqualsLiteral("_self")) {
      foundItem = this;
    } else if (name.LowerCaseEqualsLiteral("_blank")) {
      
      
      return NS_OK;
    } else if (name.LowerCaseEqualsLiteral("_parent")) {
      GetSameTypeParent(getter_AddRefs(foundItem));
      if (!foundItem) {
        foundItem = this;
      }
    } else if (name.LowerCaseEqualsLiteral("_top")) {
      GetSameTypeRootTreeItem(getter_AddRefs(foundItem));
      NS_ASSERTION(foundItem, "Must have this; worst case it's us!");
    }
    
    
    else if (name.LowerCaseEqualsLiteral("_content") ||
             name.EqualsLiteral("_main")) {
      
      
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
    } else {
      
      DoFindItemWithName(aName, aRequestor, aOriginalRequestor,
                         getter_AddRefs(foundItem));
    }

    if (foundItem && !CanAccessItem(foundItem, aOriginalRequestor)) {
      foundItem = nullptr;
    }

    
    
    if (foundItem) {
      foundItem.swap(*aResult);
    }
    return NS_OK;
  }
}

nsresult
nsDocShell::DoFindItemWithName(const char16_t* aName,
                               nsISupports* aRequestor,
                               nsIDocShellTreeItem* aOriginalRequestor,
                               nsIDocShellTreeItem** aResult)
{
  
  if (mName.Equals(aName) && ItemIsActive(this) &&
      CanAccessItem(this, aOriginalRequestor)) {
    NS_ADDREF(*aResult = this);
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> reqAsTreeItem(do_QueryInterface(aRequestor));

  
  
#ifdef DEBUG
  nsresult rv =
#endif
  FindChildWithName(aName, true, true, reqAsTreeItem, aOriginalRequestor,
                    aResult);
  NS_ASSERTION(NS_SUCCEEDED(rv),
               "FindChildWithName should not be failing here.");
  if (*aResult) {
    return NS_OK;
  }

  
  
  
  
  
  nsCOMPtr<nsIDocShellTreeItem> parentAsTreeItem =
    do_QueryInterface(GetAsSupports(mParent));
  if (parentAsTreeItem) {
    if (parentAsTreeItem == reqAsTreeItem) {
      return NS_OK;
    }

    if (parentAsTreeItem->ItemType() == mItemType) {
      return parentAsTreeItem->FindItemWithName(
        aName,
        static_cast<nsIDocShellTreeItem*>(this),
        aOriginalRequestor,
        aResult);
    }
  }

  
  

  
  nsCOMPtr<nsIDocShellTreeOwner> reqAsTreeOwner(do_QueryInterface(aRequestor));
  if (mTreeOwner && mTreeOwner != reqAsTreeOwner) {
    return mTreeOwner->FindItemWithName(aName, this, aOriginalRequestor,
                                        aResult);
  }

  return NS_OK;
}

bool
nsDocShell::IsSandboxedFrom(nsIDocShell* aTargetDocShell)
{
  
  if (!aTargetDocShell) {
    return false;
  }

  
  if (aTargetDocShell == this) {
    return false;
  }

  
  
  uint32_t sandboxFlags = mSandboxFlags;
  if (mContentViewer) {
    nsCOMPtr<nsIDocument> doc = mContentViewer->GetDocument();
    if (doc) {
      sandboxFlags = doc->GetSandboxFlags();
    }
  }

  
  if (!sandboxFlags) {
    return false;
  }

  
  nsCOMPtr<nsIDocShellTreeItem> ancestorOfTarget;
  aTargetDocShell->GetSameTypeParent(getter_AddRefs(ancestorOfTarget));
  if (ancestorOfTarget) {
    do {
      
      if (ancestorOfTarget == this) {
        return false;
      }
      nsCOMPtr<nsIDocShellTreeItem> tempTreeItem;
      ancestorOfTarget->GetSameTypeParent(getter_AddRefs(tempTreeItem));
      tempTreeItem.swap(ancestorOfTarget);
    } while (ancestorOfTarget);

    
    return true;
  }

  
  
  nsCOMPtr<nsIDocShell> permittedNavigator;
  aTargetDocShell->GetOnePermittedSandboxedNavigator(
    getter_AddRefs(permittedNavigator));
  if (permittedNavigator == this) {
    return false;
  }

  
  
  if (!(sandboxFlags & SANDBOXED_TOPLEVEL_NAVIGATION)) {
    nsCOMPtr<nsIDocShellTreeItem> rootTreeItem;
    GetSameTypeRootTreeItem(getter_AddRefs(rootTreeItem));
    if (SameCOMIdentity(aTargetDocShell, rootTreeItem)) {
      return false;
    }
  }

  
  return true;
}

NS_IMETHODIMP
nsDocShell::GetTreeOwner(nsIDocShellTreeOwner** aTreeOwner)
{
  NS_ENSURE_ARG_POINTER(aTreeOwner);

  *aTreeOwner = mTreeOwner;
  NS_IF_ADDREF(*aTreeOwner);
  return NS_OK;
}

#ifdef DEBUG_DOCSHELL_FOCUS
static void
PrintDocTree(nsIDocShellTreeItem* aParentNode, int aLevel)
{
  for (int32_t i = 0; i < aLevel; i++) {
    printf("  ");
  }

  int32_t childWebshellCount;
  aParentNode->GetChildCount(&childWebshellCount);
  nsCOMPtr<nsIDocShell> parentAsDocShell(do_QueryInterface(aParentNode));
  int32_t type = aParentNode->ItemType();
  nsCOMPtr<nsIPresShell> presShell = parentAsDocShell->GetPresShell();
  nsRefPtr<nsPresContext> presContext;
  parentAsDocShell->GetPresContext(getter_AddRefs(presContext));
  nsIDocument* doc = presShell->GetDocument();

  nsCOMPtr<nsIDOMWindow> domwin(doc->GetWindow());

  nsCOMPtr<nsIWidget> widget;
  nsViewManager* vm = presShell->GetViewManager();
  if (vm) {
    vm->GetWidget(getter_AddRefs(widget));
  }
  dom::Element* rootElement = doc->GetRootElement();

  printf("DS %p  Ty %s  Doc %p DW %p EM %p CN %p\n",
         (void*)parentAsDocShell.get(),
         type == nsIDocShellTreeItem::typeChrome ? "Chr" : "Con",
         (void*)doc, (void*)domwin.get(),
         (void*)presContext->EventStateManager(), (void*)rootElement);

  if (childWebshellCount > 0) {
    for (int32_t i = 0; i < childWebshellCount; i++) {
      nsCOMPtr<nsIDocShellTreeItem> child;
      aParentNode->GetChildAt(i, getter_AddRefs(child));
      PrintDocTree(child, aLevel + 1);
    }
  }
}

static void
PrintDocTree(nsIDocShellTreeItem* aParentNode)
{
  NS_ASSERTION(aParentNode, "Pointer is null!");

  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  aParentNode->GetParent(getter_AddRefs(parentItem));
  while (parentItem) {
    nsCOMPtr<nsIDocShellTreeItem> tmp;
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
nsDocShell::SetTreeOwner(nsIDocShellTreeOwner* aTreeOwner)
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
      nsCOMPtr<nsIWebProgressListener> oldListener =
        do_QueryInterface(mTreeOwner);
      nsCOMPtr<nsIWebProgressListener> newListener =
        do_QueryInterface(aTreeOwner);

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

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShellTreeItem> child = do_QueryObject(iter.GetNext());
    NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);

    if (child->ItemType() == mItemType) {
      child->SetTreeOwner(aTreeOwner);
    }
  }

  
  
  
  
  
  
  
  
  RecomputeCanExecuteScripts();

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
nsDocShell::GetChildCount(int32_t* aChildCount)
{
  NS_ENSURE_ARG_POINTER(aChildCount);
  *aChildCount = mChildList.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::AddChild(nsIDocShellTreeItem* aChild)
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

  nsCOMPtr<nsIDocShell> childDocShell = do_QueryInterface(aChild);
  bool dynamic = false;
  childDocShell->GetCreatedDynamically(&dynamic);
  if (!dynamic) {
    nsCOMPtr<nsISHEntry> currentSH;
    bool oshe = false;
    GetCurrentSHEntry(getter_AddRefs(currentSH), &oshe);
    if (currentSH) {
      currentSH->HasDynamicallyAddedChild(&dynamic);
    }
  }
  childDocShell->SetChildOffset(dynamic ? -1 : mChildList.Length() - 1);

  
  if (mUseGlobalHistory) {
    childDocShell->SetUseGlobalHistory(true);
  }

  if (aChild->ItemType() != mItemType) {
    return NS_OK;
  }

  aChild->SetTreeOwner(mTreeOwner);

  nsCOMPtr<nsIDocShell> childAsDocShell(do_QueryInterface(aChild));
  if (!childAsDocShell) {
    return NS_OK;
  }

  

  
  
  
  
  
  
  

  
  if (mItemType == nsIDocShellTreeItem::typeChrome) {
    return NS_OK;
  }

  
  if (!mContentViewer) {
    return NS_OK;
  }
  nsIDocument* doc = mContentViewer->GetDocument();
  if (!doc) {
    return NS_OK;
  }

  bool isWyciwyg = false;

  if (mCurrentURI) {
    
    mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);
  }

  if (!isWyciwyg) {
    
    
    
    

    const nsACString& parentCS = doc->GetDocumentCharacterSet();
    int32_t charsetSource = doc->GetDocumentCharacterSetSource();
    
    childAsDocShell->SetParentCharset(parentCS,
                                      charsetSource,
                                      doc->NodePrincipal());
  }

  
  

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::RemoveChild(nsIDocShellTreeItem* aChild)
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
nsDocShell::GetChildAt(int32_t aIndex, nsIDocShellTreeItem** aChild)
{
  NS_ENSURE_ARG_POINTER(aChild);

#ifdef DEBUG
  if (aIndex < 0) {
    NS_WARNING("Negative index passed to GetChildAt");
  } else if (static_cast<uint32_t>(aIndex) >= mChildList.Length()) {
    NS_WARNING("Too large an index passed to GetChildAt");
  }
#endif

  nsIDocumentLoader* child = ChildAt(aIndex);
  NS_ENSURE_TRUE(child, NS_ERROR_UNEXPECTED);

  return CallQueryInterface(child, aChild);
}

NS_IMETHODIMP
nsDocShell::FindChildWithName(const char16_t* aName,
                              bool aRecurse, bool aSameType,
                              nsIDocShellTreeItem* aRequestor,
                              nsIDocShellTreeItem* aOriginalRequestor,
                              nsIDocShellTreeItem** aResult)
{
  NS_ENSURE_ARG(aName);
  NS_ENSURE_ARG_POINTER(aResult);

  
  *aResult = nullptr;

  if (!*aName) {
    return NS_OK;
  }

  nsXPIDLString childName;
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShellTreeItem> child = do_QueryObject(iter.GetNext());
    NS_ENSURE_TRUE(child, NS_ERROR_FAILURE);
    int32_t childType = child->ItemType();

    if (aSameType && (childType != mItemType)) {
      continue;
    }

    bool childNameEquals = false;
    child->NameEquals(aName, &childNameEquals);
    if (childNameEquals && ItemIsActive(child) &&
        CanAccessItem(child, aOriginalRequestor)) {
      child.swap(*aResult);
      break;
    }

    
    if (childType != mItemType) {
      continue;
    }

    
    if (aRecurse && (aRequestor != child)) {
      
#ifdef DEBUG
      nsresult rv =
#endif
      child->FindChildWithName(aName, true, aSameType,
                               static_cast<nsIDocShellTreeItem*>(this),
                               aOriginalRequestor, aResult);
      NS_ASSERTION(NS_SUCCEEDED(rv), "FindChildWithName should not fail here");
      if (*aResult) {
        
        return NS_OK;
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChildSHEntry(int32_t aChildOffset, nsISHEntry** aResult)
{
  nsresult rv = NS_OK;

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  
  

  if (mLSHE) {
    




    bool parentExpired = false;
    mLSHE->GetExpirationStatus(&parentExpired);

    


    uint32_t loadType = nsIDocShellLoadInfo::loadHistory;
    mLSHE->GetLoadType(&loadType);
    
    
    if (loadType == nsIDocShellLoadInfo::loadReloadBypassCache ||
        loadType == nsIDocShellLoadInfo::loadReloadBypassProxy ||
        loadType == nsIDocShellLoadInfo::loadReloadBypassProxyAndCache ||
        loadType == nsIDocShellLoadInfo::loadRefresh) {
      return rv;
    }

    


    if (parentExpired && (loadType == nsIDocShellLoadInfo::loadReloadNormal)) {
      
      *aResult = nullptr;
      return rv;
    }

    nsCOMPtr<nsISHContainer> container(do_QueryInterface(mLSHE));
    if (container) {
      
      rv = container->GetChildAt(aChildOffset, aResult);
      if (*aResult) {
        (*aResult)->SetLoadType(loadType);
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsDocShell::AddChildSHEntry(nsISHEntry* aCloneRef, nsISHEntry* aNewEntry,
                            int32_t aChildOffset, uint32_t aLoadType,
                            bool aCloneChildren)
{
  nsresult rv = NS_OK;

  if (mLSHE && aLoadType != LOAD_PUSHSTATE) {
    


    nsCOMPtr<nsISHContainer> container(do_QueryInterface(mLSHE, &rv));
    if (container) {
      if (NS_FAILED(container->ReplaceChild(aNewEntry))) {
        rv = container->AddChild(aNewEntry, aChildOffset);
      }
    }
  } else if (!aCloneRef) {
    
    nsCOMPtr<nsISHContainer> container(do_QueryInterface(mOSHE, &rv));
    if (container) {
      rv = container->AddChild(aNewEntry, aChildOffset);
    }
  } else {
    rv = AddChildSHEntryInternal(aCloneRef, aNewEntry, aChildOffset,
                                 aLoadType, aCloneChildren);
  }
  return rv;
}

nsresult
nsDocShell::AddChildSHEntryInternal(nsISHEntry* aCloneRef,
                                    nsISHEntry* aNewEntry,
                                    int32_t aChildOffset,
                                    uint32_t aLoadType,
                                    bool aCloneChildren)
{
  nsresult rv = NS_OK;
  if (mSessionHistory) {
    






    int32_t index = -1;
    nsCOMPtr<nsISHEntry> currentHE;
    mSessionHistory->GetIndex(&index);
    if (index < 0) {
      return NS_ERROR_FAILURE;
    }

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
        nsCOMPtr<nsISHistoryInternal> shPrivate =
          do_QueryInterface(mSessionHistory);
        NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
        rv = shPrivate->AddEntry(nextEntry, true);
      }
    }
  } else {
    
    nsCOMPtr<nsIDocShell> parent =
      do_QueryInterface(GetAsSupports(mParent), &rv);
    if (parent) {
      rv = static_cast<nsDocShell*>(parent.get())->AddChildSHEntryInternal(
        aCloneRef, aNewEntry, aChildOffset, aLoadType, aCloneChildren);
    }
  }
  return rv;
}

nsresult
nsDocShell::AddChildSHEntryToParent(nsISHEntry* aNewEntry, int32_t aChildOffset,
                                    bool aCloneChildren)
{
  






  
  
  nsCOMPtr<nsISHistory> rootSH;
  GetRootSessionHistory(getter_AddRefs(rootSH));
  if (rootSH) {
    rootSH->GetIndex(&mPreviousTransIndex);
  }

  nsresult rv;
  nsCOMPtr<nsIDocShell> parent = do_QueryInterface(GetAsSupports(mParent), &rv);
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
nsDocShell::GetUseGlobalHistory(bool* aUseGlobalHistory)
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
    nsCOMPtr<nsIWebNavigation> rootAsWebnav = do_QueryInterface(root);
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

nsIScriptGlobalObject*
nsDocShell::GetScriptGlobalObject()
{
  NS_ENSURE_SUCCESS(EnsureScriptEnvironment(), nullptr);
  return mScriptGlobal;
}

nsIDocument*
nsDocShell::GetDocument()
{
  NS_ENSURE_SUCCESS(EnsureContentViewer(), nullptr);
  return mContentViewer->GetDocument();
}

nsPIDOMWindow*
nsDocShell::GetWindow()
{
  NS_ENSURE_SUCCESS(EnsureScriptEnvironment(), nullptr);
  return mScriptGlobal;
}

NS_IMETHODIMP
nsDocShell::SetDeviceSizeIsPageSize(bool aValue)
{
  if (mDeviceSizeIsPageSize != aValue) {
    mDeviceSizeIsPageSize = aValue;
    nsRefPtr<nsPresContext> presContext;
    GetPresContext(getter_AddRefs(presContext));
    if (presContext) {
      presContext->MediaFeatureValuesChanged(nsRestyleHint(0));
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDeviceSizeIsPageSize(bool* aValue)
{
  *aValue = mDeviceSizeIsPageSize;
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
    DisplayLoadError(NS_ERROR_DOCUMENT_IS_PRINTMODE, nullptr, nullptr, nullptr);
  }

  return mIsPrintingOrPP;
}

bool
nsDocShell::IsNavigationAllowed(bool aDisplayPrintErrorDialog,
                                bool aCheckIfUnloadFired)
{
  bool isAllowed = !IsPrintingOrPP(aDisplayPrintErrorDialog) &&
                   (!aCheckIfUnloadFired || !mFiredUnloadEvent);
  if (!isAllowed) {
    return false;
  }
  if (!mContentViewer) {
    return true;
  }
  bool firingBeforeUnload;
  mContentViewer->GetBeforeUnloadFiring(&firingBeforeUnload);
  return !firingBeforeUnload;
}





NS_IMETHODIMP
nsDocShell::GetCanGoBack(bool* aCanGoBack)
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
nsDocShell::GetCanGoForward(bool* aCanGoForward)
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

NS_IMETHODIMP
nsDocShell::GotoIndex(int32_t aIndex)
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
nsDocShell::LoadURI(const char16_t* aURI,
                    uint32_t aLoadFlags,
                    nsIURI* aReferringURI,
                    nsIInputStream* aPostStream,
                    nsIInputStream* aHeaderStream)
{
  return LoadURIWithOptions(aURI, aLoadFlags, aReferringURI,
                            mozilla::net::RP_Default, aPostStream,
                            aHeaderStream, nullptr);
}

NS_IMETHODIMP
nsDocShell::LoadURIWithOptions(const char16_t* aURI,
                               uint32_t aLoadFlags,
                               nsIURI* aReferringURI,
                               uint32_t aReferrerPolicy,
                               nsIInputStream* aPostStream,
                               nsIInputStream* aHeaderStream,
                               nsIURI* aBaseURI)
{
  NS_ASSERTION((aLoadFlags & 0xf) == 0, "Unexpected flags");

  if (!IsNavigationAllowed()) {
    return NS_OK; 
  }
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIInputStream> postStream(aPostStream);
  nsresult rv = NS_OK;

  
  
  

  NS_ConvertUTF16toUTF8 uriString(aURI);
  
  uriString.Trim(" ");
  
  uriString.StripChars("\r\n");
  NS_ENSURE_TRUE(!uriString.IsEmpty(), NS_ERROR_FAILURE);

  rv = NS_NewURI(getter_AddRefs(uri), uriString);
  if (uri) {
    aLoadFlags &= ~LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP;
  }

  nsCOMPtr<nsIURIFixupInfo> fixupInfo;
  if (sURIFixup) {
    
    
    
    
    uint32_t fixupFlags = 0;
    if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) {
      fixupFlags |= nsIURIFixup::FIXUP_FLAG_ALLOW_KEYWORD_LOOKUP;
    }
    if (aLoadFlags & LOAD_FLAGS_FIXUP_SCHEME_TYPOS) {
      fixupFlags |= nsIURIFixup::FIXUP_FLAG_FIX_SCHEME_TYPOS;
    }
    nsCOMPtr<nsIInputStream> fixupStream;
    rv = sURIFixup->GetFixupURIInfo(uriString, fixupFlags,
                                    getter_AddRefs(fixupStream),
                                    getter_AddRefs(fixupInfo));

    if (NS_SUCCEEDED(rv)) {
      fixupInfo->GetPreferredURI(getter_AddRefs(uri));
      fixupInfo->SetConsumer(GetAsSupports(this));
    }

    if (fixupStream) {
      
      
      
      postStream = fixupStream;
    }

    if (aLoadFlags & LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP) {
      nsCOMPtr<nsIObserverService> serv = services::GetObserverService();
      if (serv) {
        serv->NotifyObservers(fixupInfo, "keyword-uri-fixup", aURI);
      }
    }
  }
  
  

  if (NS_ERROR_MALFORMED_URI == rv) {
    DisplayLoadError(rv, uri, aURI, nullptr);
  }

  if (NS_FAILED(rv) || !uri) {
    return NS_ERROR_FAILURE;
  }

  PopupControlState popupState;
  if (aLoadFlags & LOAD_FLAGS_ALLOW_POPUPS) {
    popupState = openAllowed;
    aLoadFlags &= ~LOAD_FLAGS_ALLOW_POPUPS;
  } else {
    popupState = openOverridden;
  }
  nsAutoPopupStatePusher statePusher(popupState);

  
  
  
  uint32_t extraFlags = (aLoadFlags & EXTRA_LOAD_FLAGS);
  aLoadFlags &= ~EXTRA_LOAD_FLAGS;

  nsCOMPtr<nsIDocShellLoadInfo> loadInfo;
  rv = CreateLoadInfo(getter_AddRefs(loadInfo));
  if (NS_FAILED(rv)) {
    return rv;
  }

  



  uint32_t loadType;
  if (aLoadFlags & LOAD_FLAGS_ALLOW_MIXED_CONTENT) {
    loadType = MAKE_LOAD_TYPE(LOAD_NORMAL_ALLOW_MIXED_CONTENT, aLoadFlags);
  } else {
    loadType = MAKE_LOAD_TYPE(LOAD_NORMAL, aLoadFlags);
  }

  loadInfo->SetLoadType(ConvertLoadTypeToDocShellLoadInfo(loadType));
  loadInfo->SetPostDataStream(postStream);
  loadInfo->SetReferrer(aReferringURI);
  loadInfo->SetReferrerPolicy(aReferrerPolicy);
  loadInfo->SetHeadersStream(aHeaderStream);
  loadInfo->SetBaseURI(aBaseURI);

  if (fixupInfo) {
    nsAutoString searchProvider, keyword;
    fixupInfo->GetKeywordProviderName(searchProvider);
    fixupInfo->GetKeywordAsSent(keyword);
    MaybeNotifyKeywordSearchLoading(searchProvider, keyword);
  }

  rv = LoadURI(uri, loadInfo, extraFlags, true);

  
  
  mOriginalUriString = uriString;

  return rv;
}

NS_IMETHODIMP
nsDocShell::DisplayLoadError(nsresult aError, nsIURI* aURI,
                             const char16_t* aURL,
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
    nsCOMPtr<nsINestedURI> nestedURI = do_QueryInterface(aURI);
    while (nestedURI) {
      nsCOMPtr<nsIURI> tempURI;
      nsresult rv2;
      rv2 = nestedURI->GetInnerURI(getter_AddRefs(tempURI));
      if (NS_SUCCEEDED(rv2) && tempURI) {
        tempURI->GetScheme(scheme);
        formatStrs[0].AppendLiteral(", ");
        AppendASCIItoUTF16(scheme, formatStrs[0]);
      }
      nestedURI = do_QueryInterface(tempURI);
    }
    formatStrCount = 1;
    error.AssignLiteral("unknownProtocolFound");
  } else if (NS_ERROR_FILE_NOT_FOUND == aError) {
    NS_ENSURE_ARG_POINTER(aURI);
    error.AssignLiteral("fileNotFound");
  } else if (NS_ERROR_UNKNOWN_HOST == aError) {
    NS_ENSURE_ARG_POINTER(aURI);
    
    nsAutoCString host;
    nsCOMPtr<nsIURI> innermostURI = NS_GetInnermostURI(aURI);
    innermostURI->GetHost(host);
    CopyUTF8toUTF16(host, formatStrs[0]);
    formatStrCount = 1;
    error.AssignLiteral("dnsNotFound");
  } else if (NS_ERROR_CONNECTION_REFUSED == aError) {
    NS_ENSURE_ARG_POINTER(aURI);
    addHostPort = true;
    error.AssignLiteral("connectionFailure");
  } else if (NS_ERROR_NET_INTERRUPT == aError) {
    NS_ENSURE_ARG_POINTER(aURI);
    addHostPort = true;
    error.AssignLiteral("netInterrupt");
  } else if (NS_ERROR_NET_TIMEOUT == aError) {
    NS_ENSURE_ARG_POINTER(aURI);
    
    nsAutoCString host;
    aURI->GetHost(host);
    CopyUTF8toUTF16(host, formatStrs[0]);
    formatStrCount = 1;
    error.AssignLiteral("netTimeout");
  } else if (NS_ERROR_CSP_FRAME_ANCESTOR_VIOLATION == aError ||
             NS_ERROR_CSP_FORM_ACTION_VIOLATION == aError) {
    
    cssClass.AssignLiteral("neterror");
    error.AssignLiteral("cspBlocked");
  } else if (NS_ERROR_GET_MODULE(aError) == NS_ERROR_MODULE_SECURITY) {
    nsCOMPtr<nsINSSErrorsService> nsserr =
      do_GetService(NS_NSS_ERRORS_SERVICE_CONTRACTID);

    uint32_t errorClass;
    if (!nsserr || NS_FAILED(nsserr->GetErrorClass(aError, &errorClass))) {
      errorClass = nsINSSErrorsService::ERROR_CLASS_SSL_PROTOCOL;
    }

    nsCOMPtr<nsISupports> securityInfo;
    nsCOMPtr<nsITransportSecurityInfo> tsi;
    if (aFailedChannel) {
      aFailedChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
    }
    tsi = do_QueryInterface(securityInfo);
    if (tsi) {
      uint32_t securityState;
      tsi->GetSecurityState(&securityState);
      if (securityState & nsIWebProgressListener::STATE_USES_SSL_3) {
        error.AssignLiteral("sslv3Used");
        addHostPort = true;
      } else {
        
        tsi->GetErrorMessage(getter_Copies(messageStr));
      }
    } else {
      
      if (nsserr) {
        nsserr->GetErrorMessage(aError, messageStr);
      }
    }
    if (!messageStr.IsEmpty()) {
      if (errorClass == nsINSSErrorsService::ERROR_CLASS_BAD_CERT) {
        error.AssignLiteral("nssBadCert");

        
        
        
        uint32_t flags =
          mInPrivateBrowsing ? nsISocketProvider::NO_PERMANENT_STORAGE : 0;
        bool isStsHost = false;
        bool isPinnedHost = false;
        if (XRE_GetProcessType() == GeckoProcessType_Default) {
          nsCOMPtr<nsISiteSecurityService> sss =
            do_GetService(NS_SSSERVICE_CONTRACTID, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = sss->IsSecureURI(nsISiteSecurityService::HEADER_HSTS, aURI,
                                flags, &isStsHost);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = sss->IsSecureURI(nsISiteSecurityService::HEADER_HPKP, aURI,
                                flags, &isPinnedHost);
          NS_ENSURE_SUCCESS(rv, rv);
        } else {
          mozilla::dom::ContentChild* cc =
            mozilla::dom::ContentChild::GetSingleton();
          mozilla::ipc::URIParams uri;
          SerializeURI(aURI, uri);
          cc->SendIsSecureURI(nsISiteSecurityService::HEADER_HSTS, uri, flags,
                              &isStsHost);
          cc->SendIsSecureURI(nsISiteSecurityService::HEADER_HPKP, uri, flags,
                              &isPinnedHost);
        }

        if (Preferences::GetBool(
              "browser.xul.error_pages.expert_bad_cert", false)) {
          cssClass.AssignLiteral("expertBadCert");
        }

        
        
        
        
        if (isStsHost || isPinnedHost) {
          cssClass.AssignLiteral("badStsCert");
        }

        uint32_t bucketId;
        if (isStsHost) {
          
          
          bucketId = nsISecurityUITelemetry::WARNING_BAD_CERT_TOP_STS;
        } else {
          bucketId = nsISecurityUITelemetry::WARNING_BAD_CERT_TOP;
        }

        
        nsAdoptingCString alternateErrorPage =
          Preferences::GetCString("security.alternate_certificate_error_page");
        if (alternateErrorPage) {
          errorPage.Assign(alternateErrorPage);
        }

        if (!IsFrame() && errorPage.EqualsIgnoreCase("certerror")) {
          Telemetry::Accumulate(mozilla::Telemetry::SECURITY_UI, bucketId);
        }

      } else {
        error.AssignLiteral("nssFailure2");
      }
    }
  } else if (NS_ERROR_PHISHING_URI == aError ||
             NS_ERROR_MALWARE_URI == aError) {
    nsAutoCString host;
    aURI->GetHost(host);
    CopyUTF8toUTF16(host, formatStrs[0]);
    formatStrCount = 1;

    
    
    nsAdoptingCString alternateErrorPage =
      Preferences::GetCString("urlclassifier.alternate_error_page");
    if (alternateErrorPage) {
      errorPage.Assign(alternateErrorPage);
    }

    uint32_t bucketId;
    if (NS_ERROR_PHISHING_URI == aError) {
      error.AssignLiteral("phishingBlocked");
      bucketId = IsFrame() ? nsISecurityUITelemetry::WARNING_PHISHING_PAGE_FRAME
                           : nsISecurityUITelemetry::WARNING_PHISHING_PAGE_TOP;
    } else {
      error.AssignLiteral("malwareBlocked");
      bucketId = IsFrame() ? nsISecurityUITelemetry::WARNING_MALWARE_PAGE_FRAME
                           : nsISecurityUITelemetry::WARNING_MALWARE_PAGE_TOP;
    }

    if (errorPage.EqualsIgnoreCase("blocked"))
      Telemetry::Accumulate(Telemetry::SECURITY_UI, bucketId);

    cssClass.AssignLiteral("blacklist");
  } else if (NS_ERROR_CONTENT_CRASHED == aError) {
    errorPage.AssignLiteral("tabcrashed");
    error.AssignLiteral("tabcrashed");

    nsCOMPtr<EventTarget> handler = mChromeEventHandler;
    if (handler) {
      nsCOMPtr<Element> element = do_QueryInterface(handler);
      element->GetAttribute(NS_LITERAL_STRING("crashedPageTitle"), messageStr);
    }

    
    
    
    if (messageStr.IsEmpty()) {
      messageStr.AssignLiteral(MOZ_UTF16(" "));
    }
  } else {
    
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
        error.AssignLiteral("remoteXUL");
        break;
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
    
  } else {
    if (addHostPort) {
      
      nsAutoCString hostport;
      if (aURI) {
        aURI->GetHostPort(hostport);
      } else {
        hostport.Assign('?');
      }
      CopyUTF8toUTF16(hostport, formatStrs[formatStrCount++]);
    }

    nsAutoCString spec;
    rv = NS_ERROR_NOT_AVAILABLE;
    if (aURI) {
      
      
      bool isFileURI = false;
      rv = aURI->SchemeIs("file", &isFileURI);
      if (NS_SUCCEEDED(rv) && isFileURI) {
        aURI->GetPath(spec);
      } else {
        aURI->GetSpec(spec);
      }

      nsAutoCString charset;
      
      aURI->GetOriginCharset(charset);
      nsCOMPtr<nsITextToSubURI> textToSubURI(
        do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv)) {
        rv = textToSubURI->UnEscapeURIForUI(charset, spec,
                                            formatStrs[formatStrCount]);
      }
    } else {
      spec.Assign('?');
    }
    if (NS_FAILED(rv)) {
      CopyUTF8toUTF16(spec, formatStrs[formatStrCount]);
    }
    rv = NS_OK;
    ++formatStrCount;

    const char16_t* strs[kMaxFormatStrArgs];
    for (uint32_t i = 0; i < formatStrCount; i++) {
      strs[i] = formatStrs[i].get();
    }
    nsXPIDLString str;
    rv = stringBundle->FormatStringFromName(error.get(), strs, formatStrCount,
                                            getter_Copies(str));
    NS_ENSURE_SUCCESS(rv, rv);
    messageStr.Assign(str.get());
  }

  
  NS_ENSURE_FALSE(messageStr.IsEmpty(), NS_ERROR_FAILURE);

  if (NS_ERROR_NET_INTERRUPT == aError || NS_ERROR_NET_RESET == aError) {
    bool isSecureURI = false;
    rv = aURI->SchemeIs("https", &isSecureURI);
    if (NS_SUCCEEDED(rv) && isSecureURI) {
      
      error.AssignLiteral("nssFailure2");
    }
  }

  if (UseErrorPages()) {
    
    LoadErrorPage(aURI, aURL, errorPage.get(), error.get(),
                  messageStr.get(), cssClass.get(), aFailedChannel);
  } else {
    
    
    
    if (mScriptGlobal) {
      unused << mScriptGlobal->GetDoc();
    }

    
    prompter->Alert(nullptr, messageStr.get());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::LoadErrorPage(nsIURI* aURI, const char16_t* aURL,
                          const char* aErrorPage,
                          const char16_t* aErrorType,
                          const char16_t* aDescription,
                          const char* aCSSClass,
                          nsIChannel* aFailedChannel)
{
#if defined(PR_LOGGING) && defined(DEBUG)
  if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
    nsAutoCString spec;
    aURI->GetSpec(spec);

    nsAutoCString chanName;
    if (aFailedChannel) {
      aFailedChannel->GetName(chanName);
    } else {
      chanName.AssignLiteral("<no channel>");
    }

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
  if (aURI) {
    nsresult rv = aURI->GetSpec(url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aURI->GetOriginCharset(charset);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (aURL) {
    CopyUTF16toUTF8(aURL, url);
  } else {
    return NS_ERROR_INVALID_POINTER;
  }

  

#undef SAFE_ESCAPE
#define SAFE_ESCAPE(cstring, escArg1, escArg2)                                 \
  {                                                                            \
    char* s = nsEscape(escArg1, escArg2);                                      \
    if (!s)                                                                    \
      return NS_ERROR_OUT_OF_MEMORY;                                           \
    cstring.Adopt(s);                                                          \
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
    errorPageUrl.AppendLiteral("&s=");
    errorPageUrl.AppendASCII(escapedCSSClass.get());
  }
  errorPageUrl.AppendLiteral("&c=");
  errorPageUrl.AppendASCII(escapedCharset.get());

  nsAutoCString frameType(FrameTypeToString(mFrameType));
  errorPageUrl.AppendLiteral("&f=");
  errorPageUrl.AppendASCII(frameType.get());

  
  nsString manifestURL;
  nsresult rv = GetAppManifestURL(manifestURL);
  if (manifestURL.Length() > 0) {
    nsCString manifestParam;
    SAFE_ESCAPE(manifestParam,
                NS_ConvertUTF16toUTF8(manifestURL).get(),
                url_Path);
    errorPageUrl.AppendLiteral("&m=");
    errorPageUrl.AppendASCII(manifestParam.get());
  }

  
  
  errorPageUrl.AppendLiteral("&d=");
  errorPageUrl.AppendASCII(escapedDescription.get());

  nsCOMPtr<nsIURI> errorPageURI;
  rv = NS_NewURI(getter_AddRefs(errorPageURI), errorPageUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  return InternalLoad(errorPageURI, nullptr, mozilla::net::RP_Default,
                      nullptr, INTERNAL_LOAD_FLAGS_INHERIT_OWNER, nullptr,
                      nullptr, NullString(), nullptr, nullptr, LOAD_ERROR_PAGE,
                      nullptr, true, NullString(), this, nullptr, nullptr,
                      nullptr);
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

  if (!canReload) {
    return NS_OK;
  }

  
  if (mOSHE) {
    rv = LoadHistoryEntry(mOSHE, loadType);
  } else if (mLSHE) { 
    rv = LoadHistoryEntry(mLSHE, loadType);
  } else {
    nsCOMPtr<nsIDocument> doc(GetDocument());

    
    uint32_t flags = INTERNAL_LOAD_FLAGS_NONE;
    nsAutoString srcdoc;
    nsIPrincipal* principal = nullptr;
    nsAutoString contentTypeHint;
    nsCOMPtr<nsIURI> baseURI;
    if (doc) {
      principal = doc->NodePrincipal();
      doc->GetContentType(contentTypeHint);

      if (doc->IsSrcdocDocument()) {
        doc->GetSrcdocData(srcdoc);
        flags |= INTERNAL_LOAD_FLAGS_IS_SRCDOC;
        baseURI = doc->GetBaseURI();
      }
    }
    rv = InternalLoad(mCurrentURI,
                      mReferrerURI,
                      mReferrerPolicy,
                      principal,
                      flags,
                      nullptr,         
                      NS_LossyConvertUTF16toASCII(contentTypeHint).get(),
                      NullString(),    
                      nullptr,         
                      nullptr,         
                      loadType,        
                      nullptr,         
                      true,
                      srcdoc,          
                      this,            
                      baseURI,
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
    
    if (mContentViewer) {
      nsCOMPtr<nsIContentViewer> cv = mContentViewer;
      cv->Stop();
    }
  }

  if (nsIWebNavigation::STOP_NETWORK & aStopFlags) {
    
    
    if (mRefreshURIList) {
      SuspendRefreshURIs();
      mSavedRefreshURIList.swap(mRefreshURIList);
      mRefreshURIList = nullptr;
    }

    
    
    
    Stop();
  }

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIWebNavigation> shellAsNav(do_QueryObject(iter.GetNext()));
    if (shellAsNav) {
      shellAsNav->Stop(aStopFlags);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDocument(nsIDOMDocument** aDocument)
{
  NS_ENSURE_ARG_POINTER(aDocument);
  NS_ENSURE_SUCCESS(EnsureContentViewer(), NS_ERROR_FAILURE);

  return mContentViewer->GetDOMDocument(aDocument);
}

NS_IMETHODIMP
nsDocShell::GetCurrentURI(nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  if (mCurrentURI) {
    return NS_EnsureSafeToReturn(mCurrentURI, aURI);
  }

  *aURI = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetReferringURI(nsIURI** aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);

  *aURI = mReferrerURI;
  NS_IF_ADDREF(*aURI);

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetSessionHistory(nsISHistory* aSessionHistory)
{
  NS_ENSURE_TRUE(aSessionHistory, NS_ERROR_FAILURE);
  
  

  nsCOMPtr<nsIDocShellTreeItem> root;
  



  GetSameTypeRootTreeItem(getter_AddRefs(root));
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);
  if (root.get() == static_cast<nsIDocShellTreeItem*>(this)) {
    mSessionHistory = aSessionHistory;
    nsCOMPtr<nsISHistoryInternal> shPrivate =
      do_QueryInterface(mSessionHistory);
    NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
    shPrivate->SetRootDocShell(this);
    return NS_OK;
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::GetSessionHistory(nsISHistory** aSessionHistory)
{
  NS_ENSURE_ARG_POINTER(aSessionHistory);
  *aSessionHistory = mSessionHistory;
  NS_IF_ADDREF(*aSessionHistory);
  return NS_OK;
}




NS_IMETHODIMP
nsDocShell::LoadPage(nsISupports* aPageDescriptor, uint32_t aDisplayType)
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
    if (NS_FAILED(rv)) {
      return rv;
    }

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
nsDocShell::GetCurrentDescriptor(nsISupports** aPageDescriptor)
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
nsDocShell::InitWindow(nativeWindow aParentNativeWindow,
                       nsIWidget* aParentWidget, int32_t aX, int32_t aY,
                       int32_t aWidth, int32_t aHeight)
{
  SetParentWidget(aParentWidget);
  SetPositionAndSize(aX, aY, aWidth, aHeight, false);

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

  if (gValidateOrigin == 0xffffffff) {
    
    gValidateOrigin =
      Preferences::GetBool("browser.frame.validate_origin", true);
  }

  
  mUseErrorPages =
    Preferences::GetBool("browser.xul.error_pages.enabled", mUseErrorPages);

  if (!gAddedPreferencesVarCache) {
    Preferences::AddBoolVarCache(&sUseErrorPages,
                                 "browser.xul.error_pages.enabled",
                                 mUseErrorPages);
    gAddedPreferencesVarCache = true;
  }

  mDeviceSizeIsPageSize =
    Preferences::GetBool("docshell.device_size_is_page_size",
                         mDeviceSizeIsPageSize);

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

  
  SetRecordProfileTimelineMarkers(false);

  
  if (mObserveErrorPages) {
    mObserveErrorPages = false;
  }

  
  
  mLoadingURI = nullptr;

  
  (void)FirePageHideNotification(true);

  
  
  if (mOSHE) {
    mOSHE->SetEditorData(nullptr);
  }
  if (mLSHE) {
    mLSHE->SetEditorData(nullptr);
  }

  
  
  if (mContentListener) {
    mContentListener->DropDocShellReference();
    mContentListener->SetParentContentListener(nullptr);
    
    
    
    
    
  }

  
  Stop(nsIWebNavigation::STOP_ALL);

  mEditorData = nullptr;

  mTransferableHookData = nullptr;

  
  
  
  PersistLayoutHistoryState();

  
  nsCOMPtr<nsIDocShellTreeItem> docShellParentAsItem =
    do_QueryInterface(GetAsSupports(mParent));
  if (docShellParentAsItem) {
    docShellParentAsItem->RemoveChild(this);
  }

  if (mContentViewer) {
    mContentViewer->Close(nullptr);
    mContentViewer->Destroy();
    mContentViewer = nullptr;
  }

  nsDocLoader::Destroy();

  mParentWidget = nullptr;
  mCurrentURI = nullptr;

  if (mURLSearchParams) {
    mURLSearchParams->RemoveObservers();
    mURLSearchParams = nullptr;
  }

  if (mScriptGlobal) {
    mScriptGlobal->DetachFromDocShell();
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

  mOnePermittedSandboxedNavigator = nullptr;

  
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
nsDocShell::GetUnscaledDevicePixelsPerCSSPixel(double* aScale)
{
  if (mParentWidget) {
    *aScale = mParentWidget->GetDefaultScale().scale;
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
nsDocShell::SetPosition(int32_t aX, int32_t aY)
{
  mBounds.x = aX;
  mBounds.y = aY;

  if (mContentViewer) {
    NS_ENSURE_SUCCESS(mContentViewer->Move(aX, aY), NS_ERROR_FAILURE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPosition(int32_t* aX, int32_t* aY)
{
  return GetPositionAndSize(aX, aY, nullptr, nullptr);
}

NS_IMETHODIMP
nsDocShell::SetSize(int32_t aWidth, int32_t aHeight, bool aRepaint)
{
  int32_t x = 0, y = 0;
  GetPosition(&x, &y);
  return SetPositionAndSize(x, y, aWidth, aHeight, aRepaint);
}

NS_IMETHODIMP
nsDocShell::GetSize(int32_t* aWidth, int32_t* aHeight)
{
  return GetPositionAndSize(nullptr, nullptr, aWidth, aHeight);
}

NS_IMETHODIMP
nsDocShell::SetPositionAndSize(int32_t aX, int32_t aY, int32_t aWidth,
                               int32_t aHeight, bool aFRepaint)
{
  mBounds.x = aX;
  mBounds.y = aY;
  mBounds.width = aWidth;
  mBounds.height = aHeight;

  
  nsCOMPtr<nsIContentViewer> viewer = mContentViewer;
  if (viewer) {
    
    NS_ENSURE_SUCCESS(viewer->SetBounds(mBounds), NS_ERROR_FAILURE);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPositionAndSize(int32_t* aX, int32_t* aY, int32_t* aWidth,
                               int32_t* aHeight)
{
  if (mParentWidget) {
    
    nsIntRect r;
    mParentWidget->GetClientBounds(r);
    SetPositionAndSize(mBounds.x, mBounds.y, r.width, r.height, false);
  }

  
  
  if (aWidth || aHeight) {
    
    
    nsCOMPtr<nsIDocument> doc(do_GetInterface(GetAsSupports(mParent)));
    if (doc) {
      doc->FlushPendingNotifications(Flush_Layout);
    }
  }

  DoGetPositionAndSize(aX, aY, aWidth, aHeight);
  return NS_OK;
}

void
nsDocShell::DoGetPositionAndSize(int32_t* aX, int32_t* aY, int32_t* aWidth,
                                 int32_t* aHeight)
{
  if (aX) {
    *aX = mBounds.x;
  }
  if (aY) {
    *aY = mBounds.y;
  }
  if (aWidth) {
    *aWidth = mBounds.width;
  }
  if (aHeight) {
    *aHeight = mBounds.height;
  }
}

NS_IMETHODIMP
nsDocShell::Repaint(bool aForce)
{
  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

  nsViewManager* viewManager = presShell->GetViewManager();
  NS_ENSURE_TRUE(viewManager, NS_ERROR_FAILURE);

  viewManager->InvalidateAllViews();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParentWidget(nsIWidget** aParentWidget)
{
  NS_ENSURE_ARG_POINTER(aParentWidget);

  *aParentWidget = mParentWidget;
  NS_IF_ADDREF(*aParentWidget);

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetParentWidget(nsIWidget* aParentWidget)
{
  mParentWidget = aParentWidget;

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetParentNativeWindow(nativeWindow* aParentNativeWindow)
{
  NS_ENSURE_ARG_POINTER(aParentNativeWindow);

  if (mParentWidget) {
    *aParentNativeWindow = mParentWidget->GetNativeData(NS_NATIVE_WIDGET);
  } else {
    *aParentNativeWindow = nullptr;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetParentNativeWindow(nativeWindow aParentNativeWindow)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetNativeHandle(nsAString& aNativeHandle)
{
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsDocShell::GetVisibility(bool* aVisibility)
{
  NS_ENSURE_ARG_POINTER(aVisibility);

  *aVisibility = false;

  if (!mContentViewer) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> presShell = GetPresShell();
  if (!presShell) {
    return NS_OK;
  }

  
  nsViewManager* vm = presShell->GetViewManager();
  NS_ENSURE_TRUE(vm, NS_ERROR_FAILURE);

  
  nsView* view = vm->GetRootView(); 
  NS_ENSURE_TRUE(view, NS_ERROR_FAILURE);

  
  if (view->GetVisibility() == nsViewVisibility_kHide) {
    return NS_OK;
  }

  
  
  

  nsRefPtr<nsDocShell> docShell = this;
  nsRefPtr<nsDocShell> parentItem = docShell->GetParentDocshell();
  while (parentItem) {
    presShell = docShell->GetPresShell();

    nsCOMPtr<nsIPresShell> pPresShell = parentItem->GetPresShell();

    
    if (!pPresShell) {
      NS_NOTREACHED("parent docshell has null pres shell");
      return NS_OK;
    }

    vm = presShell->GetViewManager();
    if (vm) {
      view = vm->GetRootView();
    }

    if (view) {
      view = view->GetParent(); 
      if (view) {
        view = view->GetParent(); 
      }
    }

    nsIFrame* frame = view ? view->GetFrame() : nullptr;
    bool isDocShellOffScreen = false;
    docShell->GetIsOffScreenBrowser(&isDocShellOffScreen);
    if (frame &&
        !frame->IsVisibleConsideringAncestors(
          nsIFrame::VISIBILITY_CROSS_CHROME_CONTENT_BOUNDARY) &&
        !isDocShellOffScreen) {
      return NS_OK;
    }

    docShell = parentItem;
    parentItem = docShell->GetParentDocshell();
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
nsDocShell::GetIsOffScreenBrowser(bool* aIsOffScreen)
{
  *aIsOffScreen = mIsOffScreenBrowser;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsActive(bool aIsActive)
{
  
  if (mItemType == nsIDocShellTreeItem::typeChrome) {
    return NS_ERROR_INVALID_ARG;
  }

  
  mIsActive = aIsActive;

  
  nsCOMPtr<nsIPresShell> pshell = GetPresShell();
  if (pshell) {
    pshell->SetIsActive(aIsActive);
  }

  
  if (mScriptGlobal) {
    mScriptGlobal->SetIsBackground(!aIsActive);
    if (nsCOMPtr<nsIDocument> doc = mScriptGlobal->GetExtantDoc()) {
      doc->PostVisibilityUpdateEvent();
    }
  }

  
  
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> docshell = do_QueryObject(iter.GetNext());
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
nsDocShell::GetIsActive(bool* aIsActive)
{
  *aIsActive = mIsActive;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsPrerendered(bool aPrerendered)
{
  MOZ_ASSERT(!aPrerendered || !mIsPrerendered,
             "SetIsPrerendered(true) called on already prerendered docshell");
  mIsPrerendered = aPrerendered;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsPrerendered(bool* aIsPrerendered)
{
  *aIsPrerendered = mIsPrerendered;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetIsAppTab(bool aIsAppTab)
{
  mIsAppTab = aIsAppTab;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsAppTab(bool* aIsAppTab)
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
nsDocShell::GetSandboxFlags(uint32_t* aSandboxFlags)
{
  *aSandboxFlags = mSandboxFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetOnePermittedSandboxedNavigator(nsIDocShell* aSandboxedNavigator)
{
  if (mOnePermittedSandboxedNavigator) {
    NS_ERROR("One Permitted Sandboxed Navigator should only be set once.");
    return NS_OK;
  }

  mOnePermittedSandboxedNavigator = do_GetWeakReference(aSandboxedNavigator);
  NS_ASSERTION(mOnePermittedSandboxedNavigator,
               "One Permitted Sandboxed Navigator must support weak references.");

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetOnePermittedSandboxedNavigator(nsIDocShell** aSandboxedNavigator)
{
  NS_ENSURE_ARG_POINTER(aSandboxedNavigator);
  nsCOMPtr<nsIDocShell> permittedNavigator =
    do_QueryReferent(mOnePermittedSandboxedNavigator);
  NS_IF_ADDREF(*aSandboxedNavigator = permittedNavigator);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetDefaultLoadFlags(uint32_t aDefaultLoadFlags)
{
  mDefaultLoadFlags = aDefaultLoadFlags;

  
  if (mLoadGroup) {
    mLoadGroup->SetDefaultLoadFlags(aDefaultLoadFlags);
  } else {
    NS_WARNING("nsDocShell::SetDefaultLoadFlags has no loadGroup to propagate the flags to");
  }

  
  
  
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> docshell = do_QueryObject(iter.GetNext());
    if (!docshell) {
      continue;
    }
    docshell->SetDefaultLoadFlags(aDefaultLoadFlags);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetDefaultLoadFlags(uint32_t* aDefaultLoadFlags)
{
  *aDefaultLoadFlags = mDefaultLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetMixedContentChannel(nsIChannel* aMixedContentChannel)
{
#ifdef DEBUG
  
  if (aMixedContentChannel) {
    
    nsCOMPtr<nsIDocShellTreeItem> root;
    GetSameTypeRootTreeItem(getter_AddRefs(root));
    NS_WARN_IF_FALSE(root.get() == static_cast<nsIDocShellTreeItem*>(this),
                     "Setting mMixedContentChannel on a docshell that is not "
                     "the root docshell");
  }
#endif
  mMixedContentChannel = aMixedContentChannel;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetFailedChannel(nsIChannel** aFailedChannel)
{
  NS_ENSURE_ARG_POINTER(aFailedChannel);
  nsIDocument* doc = GetDocument();
  if (!doc) {
    *aFailedChannel = nullptr;
    return NS_OK;
  }
  NS_IF_ADDREF(*aFailedChannel = doc->GetFailedChannel());
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetMixedContentChannel(nsIChannel** aMixedContentChannel)
{
  NS_ENSURE_ARG_POINTER(aMixedContentChannel);
  NS_IF_ADDREF(*aMixedContentChannel = mMixedContentChannel);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAllowMixedContentAndConnectionData(bool* aRootHasSecureConnection,
                                                  bool* aAllowMixedContent,
                                                  bool* aIsRootDocShell)
{
  *aRootHasSecureConnection = true;
  *aAllowMixedContent = false;
  *aIsRootDocShell = false;

  nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
  GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
  NS_ASSERTION(sameTypeRoot,
               "No document shell root tree item from document shell tree item!");
  *aIsRootDocShell =
    sameTypeRoot.get() == static_cast<nsIDocShellTreeItem*>(this);

  
  nsCOMPtr<nsIDocument> rootDoc = sameTypeRoot->GetDocument();
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
    *aAllowMixedContent =
      mixedChannel && (mixedChannel == rootDoc->GetChannel());
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetVisibility(bool aVisibility)
{
  
  nsCOMPtr<nsIContentViewer> cv = mContentViewer;
  if (!cv) {
    return NS_OK;
  }
  if (aVisibility) {
    cv->Show();
  } else {
    cv->Hide();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetEnabled(bool* aEnabled)
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
nsDocShell::GetMainWidget(nsIWidget** aMainWidget)
{
  
  return GetParentWidget(aMainWidget);
}

NS_IMETHODIMP
nsDocShell::GetTitle(char16_t** aTitle)
{
  NS_ENSURE_ARG_POINTER(aTitle);

  *aTitle = ToNewUnicode(mTitle);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetTitle(const char16_t* aTitle)
{
  
  mTitle = aTitle;

  nsCOMPtr<nsIDocShellTreeItem> parent;
  GetSameTypeParent(getter_AddRefs(parent));

  
  
  if (!parent) {
    nsCOMPtr<nsIBaseWindow> treeOwnerAsWin(do_QueryInterface(mTreeOwner));
    if (treeOwnerAsWin) {
      treeOwnerAsWin->SetTitle(aTitle);
    }
  }

  if (mCurrentURI && mLoadType != LOAD_ERROR_PAGE && mUseGlobalHistory &&
      !mInPrivateBrowsing) {
    nsCOMPtr<IHistory> history = services::GetHistoryService();
    if (history) {
      history->SetURITitle(mCurrentURI, mTitle);
    } else if (mGlobalHistory) {
      mGlobalHistory->SetPageTitle(mCurrentURI, nsString(mTitle));
    }
  }

  
  if (mOSHE && mLoadType != LOAD_BYPASS_HISTORY &&
      mLoadType != LOAD_ERROR_PAGE) {
    mOSHE->SetTitle(mTitle);
  }

  return NS_OK;
}

nsresult
nsDocShell::GetCurScrollPos(int32_t aScrollOrientation, int32_t* aCurPos)
{
  NS_ENSURE_ARG_POINTER(aCurPos);

  nsIScrollableFrame* sf = GetRootScrollFrame();
  NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

  nsPoint pt = sf->GetScrollPosition();

  switch (aScrollOrientation) {
    case ScrollOrientation_X:
      *aCurPos = pt.x;
      return NS_OK;

    case ScrollOrientation_Y:
      *aCurPos = pt.y;
      return NS_OK;

    default:
      NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
  }
}

nsresult
nsDocShell::SetCurScrollPosEx(int32_t aCurHorizontalPos,
                              int32_t aCurVerticalPos)
{
  nsIScrollableFrame* sf = GetRootScrollFrame();
  NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

  sf->ScrollTo(nsPoint(aCurHorizontalPos, aCurVerticalPos),
               nsIScrollableFrame::INSTANT);
  return NS_OK;
}





NS_IMETHODIMP
nsDocShell::GetDefaultScrollbarPreferences(int32_t aScrollOrientation,
                                           int32_t* aScrollbarPref)
{
  NS_ENSURE_ARG_POINTER(aScrollbarPref);
  switch (aScrollOrientation) {
    case ScrollOrientation_X:
      *aScrollbarPref = mDefaultScrollbarPref.x;
      return NS_OK;

    case ScrollOrientation_Y:
      *aScrollbarPref = mDefaultScrollbarPref.y;
      return NS_OK;

    default:
      NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::SetDefaultScrollbarPreferences(int32_t aScrollOrientation,
                                           int32_t aScrollbarPref)
{
  switch (aScrollOrientation) {
    case ScrollOrientation_X:
      mDefaultScrollbarPref.x = aScrollbarPref;
      return NS_OK;

    case ScrollOrientation_Y:
      mDefaultScrollbarPref.y = aScrollbarPref;
      return NS_OK;

    default:
      NS_ENSURE_TRUE(false, NS_ERROR_INVALID_ARG);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDocShell::GetScrollbarVisibility(bool* aVerticalVisible,
                                   bool* aHorizontalVisible)
{
  nsIScrollableFrame* sf = GetRootScrollFrame();
  NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

  uint32_t scrollbarVisibility = sf->GetScrollbarVisibility();
  if (aVerticalVisible) {
    *aVerticalVisible =
      (scrollbarVisibility & nsIScrollableFrame::VERTICAL) != 0;
  }
  if (aHorizontalVisible) {
    *aHorizontalVisible =
      (scrollbarVisibility & nsIScrollableFrame::HORIZONTAL) != 0;
  }

  return NS_OK;
}





NS_IMETHODIMP
nsDocShell::ScrollByLines(int32_t aNumLines)
{
  nsIScrollableFrame* sf = GetRootScrollFrame();
  NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

  sf->ScrollBy(nsIntPoint(0, aNumLines), nsIScrollableFrame::LINES,
               nsIScrollableFrame::SMOOTH);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ScrollByPages(int32_t aNumPages)
{
  nsIScrollableFrame* sf = GetRootScrollFrame();
  NS_ENSURE_TRUE(sf, NS_ERROR_FAILURE);

  sf->ScrollBy(nsIntPoint(0, aNumPages), nsIScrollableFrame::PAGES,
               nsIScrollableFrame::SMOOTH);
  return NS_OK;
}





NS_IMETHODIMP
nsDocShell::RefreshURI(nsIURI* aURI, int32_t aDelay, bool aRepeat,
                       bool aMetaRefresh)
{
  NS_ENSURE_ARG(aURI);

  





  bool allowRedirects = true;
  GetAllowMetaRedirects(&allowRedirects);
  if (!allowRedirects) {
    return NS_OK;
  }

  
  
  bool sameURI;
  nsresult rv = aURI->Equals(mCurrentURI, &sameURI);
  if (NS_FAILED(rv)) {
    sameURI = false;
  }
  if (!RefreshAttempted(this, aURI, aDelay, sameURI)) {
    return NS_OK;
  }

  nsRefreshTimer* refreshTimer = new nsRefreshTimer();
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
  } else {
    
    
    nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_TRUE(timer, NS_ERROR_FAILURE);

    mRefreshURIList->AppendElement(timer);  
    timer->InitWithCallback(refreshTimer, aDelay, nsITimer::TYPE_ONE_SHOT);
  }
  return NS_OK;
}

nsresult
nsDocShell::ForceRefreshURIFromTimer(nsIURI* aURI,
                                     int32_t aDelay,
                                     bool aMetaRefresh,
                                     nsITimer* aTimer)
{
  NS_PRECONDITION(aTimer, "Must have a timer here");

  
  if (mRefreshURIList) {
    uint32_t n = 0;
    mRefreshURIList->Count(&n);

    for (uint32_t i = 0; i < n; ++i) {
      nsCOMPtr<nsITimer> timer = do_QueryElementAt(mRefreshURIList, i);
      if (timer == aTimer) {
        mRefreshURIList->RemoveElementAt(i);
        break;
      }
    }
  }

  return ForceRefreshURI(aURI, aDelay, aMetaRefresh);
}

bool
nsDocShell::DoAppRedirectIfNeeded(nsIURI* aURI,
                                  nsIDocShellLoadInfo* aLoadInfo,
                                  bool aFirstParty)
{
  uint32_t appId;
  nsresult rv = GetAppId(&appId);
  if (NS_FAILED(rv)) {
    return false;
  }

  if (appId != nsIScriptSecurityManager::NO_APP_ID &&
      appId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService(APPS_SERVICE_CONTRACTID);
    NS_ASSERTION(appsService, "No AppsService available");
    nsCOMPtr<nsIURI> redirect;
    rv = appsService->GetRedirect(appId, aURI, getter_AddRefs(redirect));
    if (NS_SUCCEEDED(rv) && redirect) {
      rv = LoadURI(redirect, aLoadInfo, nsIWebNavigation::LOAD_FLAGS_NONE,
                   aFirstParty);
      if (NS_SUCCEEDED(rv)) {
        return true;
      }
    }
  }

  return false;
}

NS_IMETHODIMP
nsDocShell::ForceRefreshURI(nsIURI* aURI, int32_t aDelay, bool aMetaRefresh)
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
  } else {
    loadInfo->SetLoadType(nsIDocShellLoadInfo::loadRefresh);
  }

  



  LoadURI(aURI, loadInfo, nsIWebNavigation::LOAD_FLAGS_NONE, true);

  return NS_OK;
}

nsresult
nsDocShell::SetupRefreshURIFromHeader(nsIURI* aBaseURI,
                                      nsIPrincipal* aPrincipal,
                                      const nsACString& aHeader)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  
  MOZ_ASSERT(aPrincipal);

  nsAutoCString uriAttrib;
  int32_t seconds = 0;
  bool specifiesSeconds = false;

  nsACString::const_iterator iter, tokenStart, doneIterating;

  aHeader.BeginReading(iter);
  aHeader.EndReading(doneIterating);

  
  while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter)) {
    ++iter;
  }

  tokenStart = iter;

  
  if (iter != doneIterating && (*iter == '-' || *iter == '+')) {
    ++iter;
  }

  
  while (iter != doneIterating && (*iter >= '0' && *iter <= '9')) {
    seconds = seconds * 10 + (*iter - '0');
    specifiesSeconds = true;
    ++iter;
  }

  if (iter != doneIterating) {
    
    if (*tokenStart == '-') {
      seconds = -seconds;
    }

    
    nsACString::const_iterator iterAfterDigit = iter;
    while (iter != doneIterating && !(*iter == ';' || *iter == ',')) {
      if (specifiesSeconds) {
        
        
        
        if (iter == iterAfterDigit &&
            !nsCRT::IsAsciiSpace(*iter) && *iter != '.') {
          
          
          
          
          return NS_ERROR_FAILURE;
        } else if (nsCRT::IsAsciiSpace(*iter)) {
          
          
          
          ++iter;
          break;
        }
      }
      ++iter;
    }

    
    while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter)) {
      ++iter;
    }

    
    if (iter != doneIterating && (*iter == ';' || *iter == ',')) {
      ++iter;
    }

    
    while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter)) {
      ++iter;
    }
  }

  
  tokenStart = iter;

  
  if (iter != doneIterating && (*iter == 'u' || *iter == 'U')) {
    ++iter;
    if (iter != doneIterating && (*iter == 'r' || *iter == 'R')) {
      ++iter;
      if (iter != doneIterating && (*iter == 'l' || *iter == 'L')) {
        ++iter;

        
        while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter)) {
          ++iter;
        }

        if (iter != doneIterating && *iter == '=') {
          ++iter;

          
          while (iter != doneIterating && nsCRT::IsAsciiSpace(*iter)) {
            ++iter;
          }

          
          tokenStart = iter;
        }
      }
    }
  }

  

  bool isQuotedURI = false;
  if (tokenStart != doneIterating &&
      (*tokenStart == '"' || *tokenStart == '\'')) {
    isQuotedURI = true;
    ++tokenStart;
  }

  
  iter = tokenStart;

  

  
  while (iter != doneIterating) {
    if (isQuotedURI && (*iter == '"' || *iter == '\'')) {
      break;
    }
    ++iter;
  }

  
  if (iter != tokenStart && isQuotedURI) {
    --iter;
    if (!(*iter == '"' || *iter == '\'')) {
      ++iter;
    }
  }

  
  

  nsresult rv = NS_OK;

  nsCOMPtr<nsIURI> uri;
  bool specifiesURI = false;
  if (tokenStart == iter) {
    uri = aBaseURI;
  } else {
    uriAttrib = Substring(tokenStart, iter);
    
    rv = NS_NewURI(getter_AddRefs(uri), uriAttrib, nullptr, aBaseURI);
    specifiesURI = true;
  }

  
  if (!specifiesSeconds && !specifiesURI) {
    
    
    return NS_ERROR_FAILURE;
  }

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIScriptSecurityManager> securityManager(
      do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv)) {
      rv = securityManager->CheckLoadURIWithPrincipal(
        aPrincipal, uri,
        nsIScriptSecurityManager::LOAD_IS_AUTOMATIC_DOCUMENT_REPLACEMENT);

      if (NS_SUCCEEDED(rv)) {
        bool isjs = true;
        rv = NS_URIChainHasFlags(
          uri, nsIProtocolHandler::URI_OPENING_EXECUTES_SCRIPT, &isjs);
        NS_ENSURE_SUCCESS(rv, rv);

        if (isjs) {
          return NS_ERROR_FAILURE;
        }
      }

      if (NS_SUCCEEDED(rv)) {
        
        
        if (seconds < 0) {
          return NS_ERROR_FAILURE;
        }

        rv = RefreshURI(uri, seconds * 1000, false, true);
      }
    }
  }
  return rv;
}

NS_IMETHODIMP
nsDocShell::SetupRefreshURI(nsIChannel* aChannel)
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
      rv = secMan->GetChannelResultPrincipal(aChannel,
                                             getter_AddRefs(principal));
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
  if (!aTimerList) {
    return;
  }

  uint32_t n = 0;
  aTimerList->Count(&n);

  while (n) {
    nsCOMPtr<nsITimer> timer(do_QueryElementAt(aTimerList, --n));

    aTimerList->RemoveElementAt(n);  

    if (timer) {
      timer->Cancel();
    }
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
nsDocShell::GetRefreshPending(bool* aResult)
{
  if (!mRefreshURIList) {
    *aResult = false;
    return NS_OK;
  }

  uint32_t count;
  nsresult rv = mRefreshURIList->Count(&count);
  if (NS_SUCCEEDED(rv)) {
    *aResult = (count != 0);
  }
  return rv;
}

NS_IMETHODIMP
nsDocShell::SuspendRefreshURIs()
{
  if (mRefreshURIList) {
    uint32_t n = 0;
    mRefreshURIList->Count(&n);

    for (uint32_t i = 0; i < n; ++i) {
      nsCOMPtr<nsITimer> timer = do_QueryElementAt(mRefreshURIList, i);
      if (!timer) {
        continue; 
      }

      
      nsCOMPtr<nsITimerCallback> callback;
      timer->GetCallback(getter_AddRefs(callback));

      timer->Cancel();

      nsCOMPtr<nsITimerCallback> rt = do_QueryInterface(callback);
      NS_ASSERTION(rt,
                   "RefreshURIList timer callbacks should only be RefreshTimer objects");

      mRefreshURIList->ReplaceElementAt(rt, i);
    }
  }

  
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> shell = do_QueryObject(iter.GetNext());
    if (shell) {
      shell->SuspendRefreshURIs();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::ResumeRefreshURIs()
{
  RefreshURIFromQueue();

  
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> shell = do_QueryObject(iter.GetNext());
    if (shell) {
      shell->ResumeRefreshURIs();
    }
  }

  return NS_OK;
}

nsresult
nsDocShell::RefreshURIFromQueue()
{
  if (!mRefreshURIList) {
    return NS_OK;
  }
  uint32_t n = 0;
  mRefreshURIList->Count(&n);

  while (n) {
    nsCOMPtr<nsISupports> element;
    mRefreshURIList->GetElementAt(--n, getter_AddRefs(element));
    nsCOMPtr<nsITimerCallback> refreshInfo(do_QueryInterface(element));

    if (refreshInfo) {
      
      
      
      uint32_t delay =
        static_cast<nsRefreshTimer*>(
          static_cast<nsITimerCallback*>(refreshInfo))->GetDelay();
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
nsDocShell::Embed(nsIContentViewer* aContentViewer,
                  const char* aCommand, nsISupports* aExtraInfo)
{
  
  
  PersistLayoutHistoryState();

  nsresult rv = SetupNewViewer(aContentViewer);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  if (mCurrentURI &&
      (mLoadType & LOAD_CMD_HISTORY ||
       mLoadType == LOAD_RELOAD_NORMAL ||
       mLoadType == LOAD_RELOAD_CHARSET_CHANGE)) {
    bool isWyciwyg = false;
    
    rv = mCurrentURI->SchemeIs("wyciwyg", &isWyciwyg);
    if (isWyciwyg && NS_SUCCEEDED(rv)) {
      SetBaseUrlForWyciwyg(aContentViewer);
    }
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

  if (!updateHistory) {
    SetLayoutHistoryState(nullptr);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsDocShell::SetIsPrinting(bool aIsPrinting)
{
  mIsPrintingOrPP = aIsPrinting;
  return NS_OK;
}





NS_IMETHODIMP
nsDocShell::OnProgressChange(nsIWebProgress* aProgress,
                             nsIRequest* aRequest,
                             int32_t aCurSelfProgress,
                             int32_t aMaxSelfProgress,
                             int32_t aCurTotalProgress,
                             int32_t aMaxTotalProgress)
{
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnStateChange(nsIWebProgress* aProgress, nsIRequest* aRequest,
                          uint32_t aStateFlags, nsresult aStatus)
{
  if ((~aStateFlags & (STATE_START | STATE_IS_NETWORK)) == 0) {
    
    nsCOMPtr<nsIChannel> channel(do_QueryInterface(aRequest));
    nsCOMPtr<nsIURI> uri;
    channel->GetURI(getter_AddRefs(uri));
    nsAutoCString aURI;
    uri->GetAsciiSpec(aURI);

    nsCOMPtr<nsIWyciwygChannel> wcwgChannel(do_QueryInterface(aRequest));
    nsCOMPtr<nsIWebProgress> webProgress =
      do_QueryInterface(GetAsSupports(this));

    
    if (this == aProgress && !wcwgChannel) {
      MaybeInitTiming();
      mTiming->NotifyFetchStart(uri,
                                ConvertLoadTypeToNavigationType(mLoadType));
    }

    
    if (wcwgChannel && !mLSHE && (mItemType == typeContent) &&
        aProgress == webProgress.get()) {
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
          
          
          
          
          
          nsCOMPtr<nsIDocShell> parent = do_QueryInterface(parentAsItem);
          if (parent) {
            bool oshe = false;
            nsCOMPtr<nsISHEntry> entry;
            parent->GetCurrentSHEntry(getter_AddRefs(entry), &oshe);
            static_cast<nsDocShell*>(parent.get())->ClearFrameHistory(entry);
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
  } else if ((~aStateFlags & (STATE_TRANSFERRING | STATE_IS_DOCUMENT)) == 0) {
    
    mBusyFlags = BUSY_FLAGS_BUSY | BUSY_FLAGS_PAGE_LOADING;
  } else if ((aStateFlags & STATE_STOP) && (aStateFlags & STATE_IS_NETWORK)) {
    
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
nsDocShell::OnLocationChange(nsIWebProgress* aProgress, nsIRequest* aRequest,
                             nsIURI* aURI, uint32_t aFlags)
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
  if (!(aStateFlags & STATE_IS_DOCUMENT)) {
    return;  
  }

  nsCOMPtr<nsIURI> oldURI, newURI;
  aOldChannel->GetURI(getter_AddRefs(oldURI));
  aNewChannel->GetURI(getter_AddRefs(newURI));
  if (!oldURI || !newURI) {
    return;
  }

  if (DoAppRedirectIfNeeded(newURI, nullptr, false)) {
    return;
  }

  
  
  
  
  
  

  
  nsCOMPtr<nsIURI> previousURI;
  uint32_t previousFlags = 0;
  ExtractLastVisit(aOldChannel, getter_AddRefs(previousURI), &previousFlags);

  if (aRedirectFlags & nsIChannelEventSink::REDIRECT_INTERNAL ||
      ChannelIsPost(aOldChannel)) {
    
    
    
    
    
    
    SaveLastVisit(aNewChannel, previousURI, previousFlags);
  } else {
    nsCOMPtr<nsIURI> referrer;
    
    (void)NS_GetReferrerFromChannel(aOldChannel, getter_AddRefs(referrer));

    
    uint32_t responseStatus = 0;
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(aOldChannel);
    if (httpChannel) {
      (void)httpChannel->GetResponseStatus(&responseStatus);
    }

    
    AddURIVisit(oldURI, referrer, previousURI, previousFlags, responseStatus);

    
    
    SaveLastVisit(aNewChannel, oldURI, aRedirectFlags);
  }

  
  nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
    do_QueryInterface(aNewChannel);
  if (appCacheChannel) {
    if (GeckoProcessType_Default != XRE_GetProcessType()) {
      
      appCacheChannel->SetChooseApplicationCache(true);
    } else {
      nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);

      if (secMan) {
        nsCOMPtr<nsIPrincipal> principal;
        secMan->GetDocShellCodebasePrincipal(newURI, this,
                                             getter_AddRefs(principal));
        appCacheChannel->SetChooseApplicationCache(
          NS_ShouldCheckAppCache(principal, mInPrivateBrowsing));
      }
    }
  }

  if (!(aRedirectFlags & nsIChannelEventSink::REDIRECT_INTERNAL) &&
      mLoadType & (LOAD_CMD_RELOAD | LOAD_CMD_HISTORY)) {
    mLoadType = LOAD_NORMAL_REPLACE;
    SetHistoryEntry(&mLSHE, nullptr);
  }
}

NS_IMETHODIMP
nsDocShell::OnStatusChange(nsIWebProgress* aWebProgress,
                           nsIRequest* aRequest,
                           nsresult aStatus, const char16_t* aMessage)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::OnSecurityChange(nsIWebProgress* aWebProgress,
                             nsIRequest* aRequest, uint32_t aState)
{
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

nsresult
nsDocShell::EndPageLoad(nsIWebProgress* aProgress,
                        nsIChannel* aChannel, nsresult aStatus)
{
  if (!aChannel) {
    return NS_ERROR_NULL_POINTER;
  }

  MOZ_EVENT_TRACER_DONE(this, "docshell::pageload");

  nsCOMPtr<nsIURI> url;
  nsresult rv = aChannel->GetURI(getter_AddRefs(url));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsITimedChannel> timingChannel = do_QueryInterface(aChannel);
  if (timingChannel) {
    TimeStamp channelCreationTime;
    rv = timingChannel->GetChannelCreation(&channelCreationTime);
    if (NS_SUCCEEDED(rv) && !channelCreationTime.IsNull()) {
      Telemetry::AccumulateTimeDelta(Telemetry::TOTAL_CONTENT_PAGE_LOAD_TIME,
                                     channelCreationTime);
      nsCOMPtr<nsPILoadGroupInternal> internalLoadGroup =
        do_QueryInterface(mLoadGroup);
      if (internalLoadGroup) {
        internalLoadGroup->OnEndPageLoad(aChannel);
      }
    }
  }

  
  mTiming = nullptr;

  
  if (eCharsetReloadRequested == mCharsetReloadState) {
    mCharsetReloadState = eCharsetReloadStopOrigional;
  } else {
    mCharsetReloadState = eCharsetReloadInit;
  }

  
  
  
  nsCOMPtr<nsISHEntry> loadingSHE = mLSHE;

  
  
  
  
  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

  
  
  if (!mEODForCurrentDocument && mContentViewer) {
    mIsExecutingOnLoadHandler = true;
    mContentViewer->LoadComplete(aStatus);
    mIsExecutingOnLoadHandler = false;

    mEODForCurrentDocument = true;

    
    
    
    if (--gNumberOfDocumentsLoading == 0) {
      
      FavorPerformanceHint(false);
    }
  }
  




  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
  if (!httpChannel) {
    
    GetHttpChannel(aChannel, getter_AddRefs(httpChannel));
  }

  if (httpChannel) {
    
    bool discardLayoutState = ShouldDiscardLayoutState(httpChannel);
    if (mLSHE && discardLayoutState && (mLoadType & LOAD_CMD_NORMAL) &&
        (mLoadType != LOAD_BYPASS_HISTORY) && (mLoadType != LOAD_ERROR_PAGE)) {
      mLSHE->SetSaveLayoutStateFlag(false);
    }
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

    
    
    
    
    if (isTopFrame == false && aStatus == NS_ERROR_TRACKING_URI) {
      
      nsCOMPtr<nsIDOMElement> frameElement;
      nsPIDOMWindow* thisWindow = GetWindow();
      if (!thisWindow) {
        return NS_OK;
      }

      thisWindow->GetFrameElement(getter_AddRefs(frameElement));
      if (!frameElement) {
        return NS_OK;
      }

      
      nsCOMPtr<nsIDocShellTreeItem> parentItem;
      GetSameTypeParent(getter_AddRefs(parentItem));
      if (!parentItem) {
        return NS_OK;
      }

      nsCOMPtr<nsIDocument> parentDoc;
      parentDoc = parentItem->GetDocument();
      if (!parentDoc) {
        return NS_OK;
      }

      nsCOMPtr<nsIContent> cont = do_QueryInterface(frameElement);
      parentDoc->AddBlockedTrackingNode(cont);

      return NS_OK;
    }

    if (sURIFixup) {
      
      
      
      nsCOMPtr<nsIURI> newURI;
      nsCOMPtr<nsIInputStream> newPostData;

      nsAutoCString oldSpec;
      url->GetSpec(oldSpec);

      
      
      
      nsAutoString keywordProviderName, keywordAsSent;
      if (aStatus == NS_ERROR_UNKNOWN_HOST && mAllowKeywordFixup) {
        bool keywordsEnabled = Preferences::GetBool("keyword.enabled", false);

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
          nsCOMPtr<nsIURIFixupInfo> info;
          
          if (!mOriginalUriString.IsEmpty()) {
            sURIFixup->KeywordToURI(mOriginalUriString,
                                    getter_AddRefs(newPostData),
                                    getter_AddRefs(info));
          } else {
            
            
            
            
            
            
            
            
            
            
            bool isACE;
            nsAutoCString utf8Host;
            nsCOMPtr<nsIIDNService> idnSrv =
              do_GetService(NS_IDNSERVICE_CONTRACTID);
            if (idnSrv &&
                NS_SUCCEEDED(idnSrv->IsACE(host, &isACE)) && isACE &&
                NS_SUCCEEDED(idnSrv->ConvertACEtoUTF8(host, utf8Host))) {
              sURIFixup->KeywordToURI(utf8Host,
                                      getter_AddRefs(newPostData),
                                      getter_AddRefs(info));
            } else {
              sURIFixup->KeywordToURI(host,
                                      getter_AddRefs(newPostData),
                                      getter_AddRefs(info));
            }
          }

          info->GetPreferredURI(getter_AddRefs(newURI));
          if (newURI) {
            info->GetKeywordAsSent(keywordAsSent);
            info->GetKeywordProviderName(keywordProviderName);
          }
        } 
      }

      
      
      
      
      if (aStatus == NS_ERROR_UNKNOWN_HOST ||
          aStatus == NS_ERROR_NET_RESET) {
        bool doCreateAlternate = true;

        
        

        if (mLoadType != LOAD_NORMAL || !isTopFrame) {
          doCreateAlternate = false;
        } else {
          
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
          newPostData = nullptr;
          keywordProviderName.Truncate();
          keywordAsSent.Truncate();
          sURIFixup->CreateFixupURI(oldSpec,
                                    nsIURIFixup::FIXUP_FLAGS_MAKE_ALTERNATE_URI,
                                    getter_AddRefs(newPostData),
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

          
          
          MaybeNotifyKeywordSearchLoading(keywordProviderName, keywordAsSent);

          return LoadURI(newSpecW.get(),  
                         LOAD_FLAGS_NONE, 
                         nullptr,         
                         newPostData,     
                         nullptr);        
        }
      }
    }

    
    

    
    if ((aStatus == NS_ERROR_UNKNOWN_HOST ||
         aStatus == NS_ERROR_CONNECTION_REFUSED ||
         aStatus == NS_ERROR_UNKNOWN_PROXY_HOST ||
         aStatus == NS_ERROR_PROXY_CONNECTION_REFUSED) &&
        (isTopFrame || UseErrorPages())) {
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
    } else if (aStatus == NS_ERROR_DOCUMENT_NOT_CACHED) {
      
      
      
      if (!(mLoadType & LOAD_CMD_HISTORY)) {
        aStatus = NS_ERROR_OFFLINE;
      }
      DisplayLoadError(aStatus, url, nullptr, aChannel);
    }
  }
  else if (url && NS_SUCCEEDED(aStatus)) {
    
    mozilla::net::PredictorLearnRedirect(url, aChannel, this);
  }

  return NS_OK;
}





nsresult
nsDocShell::EnsureContentViewer()
{
  if (mContentViewer) {
    return NS_OK;
  }
  if (mIsBeingDestroyed) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> baseURI;
  nsIPrincipal* principal = GetInheritedPrincipal(false);
  nsCOMPtr<nsIDocShellTreeItem> parentItem;
  GetSameTypeParent(getter_AddRefs(parentItem));
  if (parentItem) {
    nsCOMPtr<nsPIDOMWindow> domWin = GetWindow();
    if (domWin) {
      nsCOMPtr<Element> parentElement = domWin->GetFrameElementInternal();
      if (parentElement) {
        baseURI = parentElement->GetBaseURI();
      }
    }
  }

  nsresult rv = CreateAboutBlankContentViewer(principal, baseURI);

  NS_ENSURE_STATE(mContentViewer);

  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIDocument> doc(GetDocument());
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

  


  NS_ASSERTION(!mCreatingDocument,
               "infinite(?) loop creating document averted");
  if (mCreatingDocument) {
    return NS_ERROR_FAILURE;
  }

  AutoRestore<bool> creatingDocument(mCreatingDocument);
  mCreatingDocument = true;

  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

  
  
  bool hadTiming = mTiming;
  MaybeInitTiming();
  if (mContentViewer) {
    
    
    
    

    
    
    mTiming->NotifyBeforeUnload();

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

    
    
    
    
    
    
    (void)FirePageHideNotification(!mSavingOldViewer);
  }

  
  
  
  
  mFiredUnloadEvent = false;

  nsCOMPtr<nsIDocumentLoaderFactory> docFactory =
    nsContentUtils::FindInternalContentViewer(NS_LITERAL_CSTRING("text/html"));

  if (docFactory) {
    nsCOMPtr<nsIPrincipal> principal;
    if (mSandboxFlags & SANDBOXED_ORIGIN) {
      principal = nsNullPrincipal::CreateWithInheritedAttributes(aPrincipal);
      NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);
    } else {
      principal = aPrincipal;
    }
    
    docFactory->CreateBlankDocument(mLoadGroup, principal,
                                    getter_AddRefs(blankDoc));
    if (blankDoc) {
      
      
      blankDoc->SetBaseURI(aBaseURI);

      blankDoc->SetContainer(this);

      
      
      blankDoc->SetSandboxFlags(mSandboxFlags);

      
      docFactory->CreateInstanceForDocument(NS_ISUPPORTS_CAST(nsIDocShell*, this),
                                            blankDoc, "view",
                                            getter_AddRefs(viewer));

      
      if (viewer) {
        viewer->SetContainer(this);
        rv = Embed(viewer, "", 0);
        NS_ENSURE_SUCCESS(rv, rv);

        SetCurrentURI(blankDoc->GetDocumentURI(), nullptr, true, 0);
        rv = mIsBeingDestroyed ? NS_ERROR_NOT_AVAILABLE : NS_OK;
      }
    }
  }

  
  SetHistoryEntry(&mOSHE, nullptr);

  
  
  if (!hadTiming) {
    mTiming = nullptr;
    mBlankTiming = true;
  }

  return rv;
}

NS_IMETHODIMP
nsDocShell::CreateAboutBlankContentViewer(nsIPrincipal* aPrincipal)
{
  return CreateAboutBlankContentViewer(aPrincipal, nullptr);
}

bool
nsDocShell::CanSavePresentation(uint32_t aLoadType,
                                nsIRequest* aNewRequest,
                                nsIDocument* aNewDocument)
{
  if (!mOSHE) {
    return false;  
  }

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
      aLoadType != LOAD_ERROR_PAGE) {
    return false;
  }

  
  
  bool canSaveState;
  mOSHE->GetSaveLayoutStateFlag(&canSaveState);
  if (!canSaveState) {
    return false;
  }

  
  if (!mScriptGlobal || mScriptGlobal->IsLoading()) {
    return false;
  }

  if (mScriptGlobal->WouldReuseInnerWindow(aNewDocument)) {
    return false;
  }

  
  
  if (nsSHistory::GetMaxTotalViewers() == 0) {
    return false;
  }

  
  
  bool cacheFrames =
    Preferences::GetBool("browser.sessionhistory.cache_subframes", false);
  if (!cacheFrames) {
    nsCOMPtr<nsIDocShellTreeItem> root;
    GetSameTypeParent(getter_AddRefs(root));
    if (root && root != this) {
      return false;  
    }
  }

  
  nsCOMPtr<nsIDocument> doc = mScriptGlobal->GetExtantDoc();
  return doc && doc->CanSavePresentation(aNewRequest);
}

void
nsDocShell::ReattachEditorToWindow(nsISHEntry* aSHEntry)
{
  NS_ASSERTION(!mEditorData,
               "Why reattach an editor when we already have one?");
  NS_ASSERTION(aSHEntry && aSHEntry->HasDetachedEditor(),
               "Reattaching when there's not a detached editor.");

  if (mEditorData || !aSHEntry) {
    return;
  }

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
    
    if (mOSHE) {
      mOSHE->SetEditorData(mEditorData.forget());
    } else {
      mEditorData = nullptr;
    }
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

  if (!mScriptGlobal) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsISupports> windowState = mScriptGlobal->SaveWindowState();
  NS_ENSURE_TRUE(windowState, NS_ERROR_FAILURE);

#ifdef DEBUG_PAGE_CACHE
  nsCOMPtr<nsIURI> uri;
  mOSHE->GetURI(getter_AddRefs(uri));
  nsAutoCString spec;
  if (uri) {
    uri->GetSpec(spec);
  }
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
  if (mDocShell && NS_FAILED(mDocShell->RestoreFromHistory())) {
    NS_WARNING("RestoreFromHistory failed");
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::BeginRestore(nsIContentViewer* aContentViewer, bool aTop)
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
    nsIChannel* channel = doc->GetChannel();
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
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> child = do_QueryObject(iter.GetNext());
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
  
  

  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> child = do_QueryObject(iter.GetNext());
    if (child) {
      child->FinishRestore();
    }
  }

  if (mOSHE && mOSHE->HasDetachedEditor()) {
    ReattachEditorToWindow(mOSHE);
  }

  nsCOMPtr<nsIDocument> doc = GetDocument();
  if (doc) {
    
    
    

    nsIChannel* channel = doc->GetChannel();
    if (channel) {
      mIsRestoringDocument = true;
      mLoadGroup->RemoveRequest(channel, nullptr, NS_OK);
      mIsRestoringDocument = false;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetRestoringDocument(bool* aRestoring)
{
  *aRestoring = mIsRestoringDocument;
  return NS_OK;
}

nsresult
nsDocShell::RestorePresentation(nsISHEntry* aSHEntry, bool* aRestoring)
{
  NS_ASSERTION(mLoadType & LOAD_CMD_HISTORY,
               "RestorePresentation should only be called for history loads");

  nsCOMPtr<nsIContentViewer> viewer;
  aSHEntry->GetContentViewer(getter_AddRefs(viewer));

#ifdef DEBUG_PAGE_CACHE
  nsCOMPtr<nsIURI> uri;
  aSHEntry->GetURI(getter_AddRefs(uri));

  nsAutoCString spec;
  if (uri) {
    uri->GetSpec(spec);
  }
#endif

  *aRestoring = false;

  if (!viewer) {
#ifdef DEBUG_PAGE_CACHE
    printf("no saved presentation for uri: %s\n", spec.get());
#endif
    return NS_OK;
  }

  
  
  
  
  

  nsCOMPtr<nsIDocShell> container;
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

namespace {
class MOZ_STACK_CLASS PresentationEventForgetter
{
public:
  explicit PresentationEventForgetter(
      nsRevocableEventPtr<nsDocShell::RestorePresentationEvent>&
        aRestorePresentationEvent)
    : mRestorePresentationEvent(aRestorePresentationEvent)
    , mEvent(aRestorePresentationEvent.get())
  {
  }

  ~PresentationEventForgetter()
  {
    Forget();
  }

  void Forget()
  {
    if (mRestorePresentationEvent.get() == mEvent) {
      mRestorePresentationEvent.Forget();
      mEvent = nullptr;
    }
  }

private:
  nsRevocableEventPtr<nsDocShell::RestorePresentationEvent>&
    mRestorePresentationEvent;
  nsRefPtr<nsDocShell::RestorePresentationEvent> mEvent;
};
}

nsresult
nsDocShell::RestoreFromHistory()
{
  MOZ_ASSERT(mRestorePresentationEvent.IsPending());
  PresentationEventForgetter forgetter(mRestorePresentationEvent);

  
  if (!mLSHE) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIContentViewer> viewer;
  mLSHE->GetContentViewer(getter_AddRefs(viewer));
  if (!viewer) {
    return NS_ERROR_FAILURE;
  }

  if (mSavingOldViewer) {
    
    
    
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    viewer->GetDOMDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    nsIRequest* request = nullptr;
    if (doc) {
      request = doc->GetChannel();
    }
    mSavingOldViewer = CanSavePresentation(mLoadType, request, doc);
  }

  nsCOMPtr<nsIContentViewer> oldCv(mContentViewer);
  nsCOMPtr<nsIContentViewer> newCv(viewer);
  int32_t minFontSize = 0;
  float textZoom = 1.0f;
  float pageZoom = 1.0f;
  bool styleDisabled = false;
  if (oldCv && newCv) {
    oldCv->GetMinFontSize(&minFontSize);
    oldCv->GetTextZoom(&textZoom);
    oldCv->GetFullZoom(&pageZoom);
    oldCv->GetAuthorStyleDisabled(&styleDisabled);
  }

  
  
  nsCOMPtr<nsISHEntry> origLSHE = mLSHE;

  
  
  mLoadingURI = nullptr;

  
  FirePageHideNotification(!mSavingOldViewer);

  
  
  if (mLSHE != origLSHE) {
    return NS_OK;
  }

  
  
  
  
  

  nsRefPtr<RestorePresentationEvent> currentPresentationRestoration =
    mRestorePresentationEvent.get();
  Stop();
  
  
  NS_ENSURE_STATE(currentPresentationRestoration ==
                  mRestorePresentationEvent.get());
  BeginRestore(viewer, true);
  NS_ENSURE_STATE(currentPresentationRestoration ==
                  mRestorePresentationEvent.get());
  forgetter.Forget();

  
  
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

  
  
  

  nsView* rootViewSibling = nullptr;
  nsView* rootViewParent = nullptr;
  nsIntRect newBounds(0, 0, 0, 0);

  nsCOMPtr<nsIPresShell> oldPresShell = GetPresShell();
  if (oldPresShell) {
    nsViewManager* vm = oldPresShell->GetViewManager();
    if (vm) {
      nsView* oldRootView = vm->GetRootView();

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
    nsIFrame* frame = rootViewSibling->GetFrame();
    sibling =
      frame ? frame->PresContext()->PresShell()->GetDocument() : nullptr;
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

  
  
  if (++gNumberOfDocumentsLoading == 1) {
    FavorPerformanceHint(true);
  }

  if (oldCv && newCv) {
    newCv->SetMinFontSize(minFontSize);
    newCv->SetTextZoom(textZoom);
    newCv->SetFullZoom(pageZoom);
    newCv->SetAuthorStyleDisabled(styleDisabled);
  }

  nsCOMPtr<nsIDocument> document = do_QueryInterface(domDoc);
  uint32_t parentSuspendCount = 0;
  if (document) {
    nsRefPtr<nsDocShell> parent = GetParentDocshell();
    if (parent) {
      nsCOMPtr<nsIDocument> d = parent->GetDocument();
      if (d) {
        if (d->EventHandlingSuppressed()) {
          document->SuppressEventHandling(nsIDocument::eEvents,
                                          d->EventHandlingSuppressed());
        }

        
        if (d->AnimationsPaused()) {
          document->SuppressEventHandling(nsIDocument::eAnimationsOnly,
                                          d->AnimationsPaused());
        }

        nsCOMPtr<nsPIDOMWindow> parentWindow = d->GetWindow();
        if (parentWindow) {
          parentSuspendCount = parentWindow->TimeoutSuspendCount();
        }
      }
    }

    
    
    
    
    
    nsCOMPtr<nsIURI> uri;
    origLSHE->GetURI(getter_AddRefs(uri));
    SetCurrentURI(uri, document->GetChannel(), true, 0);
  }

  
  
  
  nsCOMPtr<nsPIDOMWindow> privWin = GetWindow();
  NS_ASSERTION(privWin, "could not get nsPIDOMWindow interface");

  rv = privWin->RestoreWindowState(windowState);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  document->NotifyPossibleTitleChange(false);

  
  for (i = 0; i < childShells.Count(); ++i) {
    nsIDocShellTreeItem* childItem = childShells.ObjectAt(i);
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

    bool allowMedia = childShell->GetAllowMedia();

    bool allowDNSPrefetch;
    childShell->GetAllowDNSPrefetch(&allowDNSPrefetch);

    bool allowContentRetargeting = childShell->GetAllowContentRetargeting();
    bool allowContentRetargetingOnChildren = childShell->GetAllowContentRetargetingOnChildren();

    uint32_t defaultLoadFlags;
    childShell->GetDefaultLoadFlags(&defaultLoadFlags);

    
    
    
    
    AddChild(childItem);

    childShell->SetAllowPlugins(allowPlugins);
    childShell->SetAllowJavascript(allowJavascript);
    childShell->SetAllowMetaRedirects(allowRedirects);
    childShell->SetAllowSubframes(allowSubframes);
    childShell->SetAllowImages(allowImages);
    childShell->SetAllowMedia(allowMedia);
    childShell->SetAllowDNSPrefetch(allowDNSPrefetch);
    childShell->SetAllowContentRetargeting(allowContentRetargeting);
    childShell->SetAllowContentRetargetingOnChildren(allowContentRetargetingOnChildren);
    childShell->SetDefaultLoadFlags(defaultLoadFlags);

    rv = childShell->BeginRestore(nullptr, false);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIPresShell> shell = GetPresShell();

  
  
  

  
  
  
  

  
  
  
  if (shell && shell->GetPresContext()->DeviceContext()->CheckDPIChange()) {
    shell->BackingScaleFactorChanged();
  }

  nsViewManager* newVM = shell ? shell->GetViewManager() : nullptr;
  nsView* newRootView = newVM ? newVM->GetRootView() : nullptr;

  
  if (container) {
    nsSubDocumentFrame* subDocFrame =
      do_QueryFrame(container->GetPrimaryFrame());
    rootViewParent = subDocFrame ? subDocFrame->EnsureInnerView() : nullptr;
  }
  if (sibling &&
      sibling->GetShell() &&
      sibling->GetShell()->GetViewManager()) {
    rootViewSibling = sibling->GetShell()->GetViewManager()->GetRootView();
  } else {
    rootViewSibling = nullptr;
  }
  if (rootViewParent && newRootView &&
      newRootView->GetParent() != rootViewParent) {
    nsViewManager* parentVM = rootViewParent->GetViewManager();
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

  
  
  nsTObserverArray<nsDocLoader*>::ForwardIterator iter(mChildList);
  while (iter.HasMore()) {
    nsCOMPtr<nsIDocShell> child = do_QueryObject(iter.GetNext());
    if (child) {
      child->ResumeRefreshURIs();
    }
  }

  
  
  

  
  
  
  
  
  
  
  

  if (newRootView) {
    if (!newBounds.IsEmpty() && !newBounds.IsEqualEdges(oldBounds)) {
#ifdef DEBUG_PAGE_CACHE
      printf("resize widget(%d, %d, %d, %d)\n", newBounds.x,
             newBounds.y, newBounds.width, newBounds.height);
#endif
      mContentViewer->SetBounds(newBounds);
    } else {
      nsIScrollableFrame* rootScrollFrame =
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
  }

  return privWin->FireDelayedDOMEvents();
}

nsresult
nsDocShell::CreateContentViewer(const nsACString& aContentType,
                                nsIRequest* aRequest,
                                nsIStreamListener** aContentHandler)
{
  *aContentHandler = nullptr;

  
  

  NS_ASSERTION(mLoadGroup, "Someone ignored return from Init()?");

  
  nsCOMPtr<nsIContentViewer> viewer;
  nsresult rv = NewContentViewerObj(aContentType, aRequest, mLoadGroup,
                                    aContentHandler, getter_AddRefs(viewer));

  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  

  if (mSavingOldViewer) {
    
    
    
    
    nsCOMPtr<nsIDOMDocument> domDoc;
    viewer->GetDOMDocument(getter_AddRefs(domDoc));
    nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc);
    mSavingOldViewer = CanSavePresentation(mLoadType, aRequest, doc);
  }

  NS_ASSERTION(!mLoadingURI, "Re-entering unload?");

  nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(aRequest);
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

    nsIDocument* doc = viewer->GetDocument();
    if (doc) {
      doc->SetFailedChannel(failedChannel);
    }

    
    nsCOMPtr<nsIURI> failedURI;
    if (failedChannel) {
      NS_GetFinalChannelURI(failedChannel, getter_AddRefs(failedURI));
    }

    if (!failedURI) {
      failedURI = mFailedURI;
    }
    if (!failedURI) {
      
      NS_NewURI(getter_AddRefs(failedURI), "about:blank");
    }

    
    
    MOZ_ASSERT(failedURI, "We don't have a URI for history APIs.");

    mFailedChannel = nullptr;
    mFailedURI = nullptr;

    
    if (failedURI) {
      bool errorOnLocationChangeNeeded = OnNewURI(
        failedURI, failedChannel, nullptr, mLoadType, false, false, false);

      if (errorOnLocationChangeNeeded) {
        FireOnLocationChange(this, failedChannel, failedURI,
                             LOCATION_CHANGE_ERROR_PAGE);
      }
    }

    
    
    if (mSessionHistory && !mLSHE) {
      int32_t idx;
      mSessionHistory->GetRequestedIndex(&idx);
      if (idx == -1) {
        mSessionHistory->GetIndex(&idx);
      }
      mSessionHistory->GetEntryAtIndex(idx, false, getter_AddRefs(mLSHE));
    }

    mLoadType = LOAD_ERROR_PAGE;
  }

  bool onLocationChangeNeeded = OnLoadingSite(aOpenedChannel, false);

  
  nsCOMPtr<nsILoadGroup> currentLoadGroup;
  NS_ENSURE_SUCCESS(
    aOpenedChannel->GetLoadGroup(getter_AddRefs(currentLoadGroup)),
    NS_ERROR_FAILURE);

  if (currentLoadGroup != mLoadGroup) {
    nsLoadFlags loadFlags = 0;

    
    
    
    
    
    





    aOpenedChannel->SetLoadGroup(mLoadGroup);

    
    aOpenedChannel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_DOCUMENT_URI;

    aOpenedChannel->SetLoadFlags(loadFlags);

    mLoadGroup->AddRequest(aRequest, nullptr);
    if (currentLoadGroup)
      currentLoadGroup->RemoveRequest(aRequest, nullptr, NS_BINDING_RETARGETED);

    
    
    aOpenedChannel->SetNotificationCallbacks(this);
  }

  NS_ENSURE_SUCCESS(Embed(viewer, "", nullptr), NS_ERROR_FAILURE);

  mSavedRefreshURIList = nullptr;
  mSavingOldViewer = false;
  mEODForCurrentDocument = false;

  
  
  nsCOMPtr<nsIMultiPartChannel> multiPartChannel(do_QueryInterface(aRequest));
  if (multiPartChannel) {
    nsCOMPtr<nsIPresShell> shell = GetPresShell();
    if (NS_SUCCEEDED(rv) && shell) {
      nsIDocument* doc = shell->GetDocument();
      if (doc) {
        uint32_t partID;
        multiPartChannel->GetPartID(&partID);
        doc->SetPartID(partID);
      }
    }
  }

  
  
  
  if (++gNumberOfDocumentsLoading == 1) {
    
    
    
    FavorPerformanceHint(true);
  }

  if (onLocationChangeNeeded) {
    FireOnLocationChange(this, aRequest, mCurrentURI, 0);
  }

  return NS_OK;
}

nsresult
nsDocShell::NewContentViewerObj(const nsACString& aContentType,
                                nsIRequest* aRequest, nsILoadGroup* aLoadGroup,
                                nsIStreamListener** aContentHandler,
                                nsIContentViewer** aViewer)
{
  nsCOMPtr<nsIChannel> aOpenedChannel = do_QueryInterface(aRequest);

  nsCOMPtr<nsIDocumentLoaderFactory> docLoaderFactory =
    nsContentUtils::FindInternalContentViewer(aContentType);
  if (!docLoaderFactory) {
    return NS_ERROR_FAILURE;
  }

  
  
  nsresult rv = docLoaderFactory->CreateInstance("view",
                                                 aOpenedChannel,
                                                 aLoadGroup, aContentType,
                                                 this,
                                                 nullptr,
                                                 aContentHandler,
                                                 aViewer);
  NS_ENSURE_SUCCESS(rv, rv);

  (*aViewer)->SetContainer(this);
  return NS_OK;
}

nsresult
nsDocShell::SetupNewViewer(nsIContentViewer* aNewViewer)
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

  nsAutoCString forceCharset;
  nsAutoCString hintCharset;
  int32_t hintCharsetSource;
  int32_t minFontSize;
  float textZoom;
  float pageZoom;
  bool styleDisabled;
  
  nsCOMPtr<nsIContentViewer> newCv;

  if (mContentViewer || parent) {
    nsCOMPtr<nsIContentViewer> oldCv;
    if (mContentViewer) {
      
      
      
      oldCv = mContentViewer;

      
      

      if (mSavingOldViewer && NS_FAILED(CaptureState())) {
        if (mOSHE) {
          mOSHE->SyncPresentationState();
        }
        mSavingOldViewer = false;
      }
    } else {
      
      parent->GetContentViewer(getter_AddRefs(oldCv));
    }

    if (oldCv) {
      newCv = aNewViewer;
      if (newCv) {
        NS_ENSURE_SUCCESS(oldCv->GetForceCharacterSet(forceCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetHintCharacterSet(hintCharset),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetHintCharacterSetSource(&hintCharsetSource),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetMinFontSize(&minFontSize),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetTextZoom(&textZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetFullZoom(&pageZoom),
                          NS_ERROR_FAILURE);
        NS_ENSURE_SUCCESS(oldCv->GetAuthorStyleDisabled(&styleDisabled),
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
    NS_WARNING("ContentViewer Initialization failed");
    return NS_ERROR_FAILURE;
  }

  
  
  if (newCv) {
    NS_ENSURE_SUCCESS(newCv->SetForceCharacterSet(forceCharset),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetHintCharacterSet(hintCharset),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetHintCharacterSetSource(hintCharsetSource),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetMinFontSize(minFontSize),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetTextZoom(textZoom),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetFullZoom(pageZoom),
                      NS_ERROR_FAILURE);
    NS_ENSURE_SUCCESS(newCv->SetAuthorStyleDisabled(styleDisabled),
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
nsDocShell::SetDocCurrentStateObj(nsISHEntry* aShEntry)
{
  NS_ENSURE_STATE(mContentViewer);
  nsCOMPtr<nsIDocument> document = GetDocument();
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

  nsCOMPtr<nsIStructuredCloneContainer> scContainer;
  if (aShEntry) {
    nsresult rv = aShEntry->GetStateData(getter_AddRefs(scContainer));
    NS_ENSURE_SUCCESS(rv, rv);

    
  }

  
  
  document->SetStateObject(scContainer);

  return NS_OK;
}

nsresult
nsDocShell::CheckLoadingPermissions()
{
  
  
  
  
  
  
  
  nsresult rv = NS_OK;

  if (!gValidateOrigin || !IsFrame()) {
    
    

    return rv;
  }

  
  
  
  if (!nsContentUtils::GetCurrentJSContext()) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> item(this);
  do {
    nsCOMPtr<nsIScriptGlobalObject> sgo = do_GetInterface(item);
    nsCOMPtr<nsIScriptObjectPrincipal> sop(do_QueryInterface(sgo));

    nsIPrincipal* p;
    if (!sop || !(p = sop->GetPrincipal())) {
      return NS_ERROR_UNEXPECTED;
    }

    if (nsContentUtils::SubjectPrincipal()->Subsumes(p)) {
      
      return NS_OK;
    }

    nsCOMPtr<nsIDocShellTreeItem> tmp;
    item->GetSameTypeParent(getter_AddRefs(tmp));
    item.swap(tmp);
  } while (item);

  return NS_ERROR_DOM_PROP_ACCESS_DENIED;
}




namespace {

#ifdef MOZ_PLACES


class nsCopyFaviconCallback final : public nsIFaviconDataCallback
{
public:
  NS_DECL_ISUPPORTS

  nsCopyFaviconCallback(nsIURI* aNewURI, bool aInPrivateBrowsing)
    : mNewURI(aNewURI)
    , mInPrivateBrowsing(aInPrivateBrowsing)
  {
  }

  NS_IMETHODIMP
  OnComplete(nsIURI* aFaviconURI, uint32_t aDataLen,
             const uint8_t* aData, const nsACString& aMimeType) override
  {
    
    if (!aFaviconURI) {
      return NS_OK;
    }

    NS_ASSERTION(aDataLen == 0,
                 "We weren't expecting the callback to deliver data.");
    nsCOMPtr<mozIAsyncFavicons> favSvc =
      do_GetService("@mozilla.org/browser/favicon-service;1");
    NS_ENSURE_STATE(favSvc);

    return favSvc->SetAndFetchFaviconForPage(
      mNewURI, aFaviconURI, false,
      mInPrivateBrowsing ? nsIFaviconService::FAVICON_LOAD_PRIVATE :
                           nsIFaviconService::FAVICON_LOAD_NON_PRIVATE,
      nullptr);
  }

private:
  ~nsCopyFaviconCallback() {}

  nsCOMPtr<nsIURI> mNewURI;
  bool mInPrivateBrowsing;
};

NS_IMPL_ISUPPORTS(nsCopyFaviconCallback, nsIFaviconDataCallback)
#endif


void
CopyFavicon(nsIURI* aOldURI, nsIURI* aNewURI, bool aInPrivateBrowsing)
{
#ifdef MOZ_PLACES
  nsCOMPtr<mozIAsyncFavicons> favSvc =
    do_GetService("@mozilla.org/browser/favicon-service;1");
  if (favSvc) {
    nsCOMPtr<nsIFaviconDataCallback> callback =
      new nsCopyFaviconCallback(aNewURI, aInPrivateBrowsing);
    favSvc->GetFaviconURLForPage(aOldURI, callback);
  }
#endif
}

} 

class InternalLoadEvent : public nsRunnable
{
public:
  InternalLoadEvent(nsDocShell* aDocShell, nsIURI* aURI,
                    nsIURI* aReferrer, uint32_t aReferrerPolicy,
                    nsISupports* aOwner, uint32_t aFlags,
                    const char* aTypeHint, nsIInputStream* aPostData,
                    nsIInputStream* aHeadersData, uint32_t aLoadType,
                    nsISHEntry* aSHEntry, bool aFirstParty,
                    const nsAString& aSrcdoc, nsIDocShell* aSourceDocShell,
                    nsIURI* aBaseURI)
    : mSrcdoc(aSrcdoc)
    , mDocShell(aDocShell)
    , mURI(aURI)
    , mReferrer(aReferrer)
    , mReferrerPolicy(aReferrerPolicy)
    , mOwner(aOwner)
    , mPostData(aPostData)
    , mHeadersData(aHeadersData)
    , mSHEntry(aSHEntry)
    , mFlags(aFlags)
    , mLoadType(aLoadType)
    , mFirstParty(aFirstParty)
    , mSourceDocShell(aSourceDocShell)
    , mBaseURI(aBaseURI)
  {
    
    if (aTypeHint) {
      mTypeHint = aTypeHint;
    }
  }

  NS_IMETHOD Run()
  {
    return mDocShell->InternalLoad(mURI, mReferrer,
                                   mReferrerPolicy,
                                   mOwner, mFlags,
                                   nullptr, mTypeHint.get(),
                                   NullString(), mPostData, mHeadersData,
                                   mLoadType, mSHEntry, mFirstParty,
                                   mSrcdoc, mSourceDocShell, mBaseURI,
                                   nullptr, nullptr);
  }

private:
  
  nsXPIDLString mWindowTarget;
  nsXPIDLCString mTypeHint;
  nsString mSrcdoc;

  nsRefPtr<nsDocShell> mDocShell;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIURI> mReferrer;
  uint32_t mReferrerPolicy;
  nsCOMPtr<nsISupports> mOwner;
  nsCOMPtr<nsIInputStream> mPostData;
  nsCOMPtr<nsIInputStream> mHeadersData;
  nsCOMPtr<nsISHEntry> mSHEntry;
  uint32_t mFlags;
  uint32_t mLoadType;
  bool mFirstParty;
  nsCOMPtr<nsIDocShell> mSourceDocShell;
  nsCOMPtr<nsIURI> mBaseURI;
};











bool
nsDocShell::JustStartedNetworkLoad()
{
  return mDocumentRequest && mDocumentRequest != GetCurrentDocChannel();
}

nsresult
nsDocShell::CreatePrincipalFromReferrer(nsIURI* aReferrer,
                                        nsIPrincipal** aResult)
{
  nsresult rv;
  nsCOMPtr<nsIScriptSecurityManager> secMan =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  uint32_t appId;
  rv = GetAppId(&appId);
  NS_ENSURE_SUCCESS(rv, rv);
  bool isInBrowserElement;
  rv = GetIsInBrowserElement(&isInBrowserElement);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = secMan->GetAppCodebasePrincipal(aReferrer,
                                       appId,
                                       isInBrowserElement,
                                       aResult);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::InternalLoad(nsIURI* aURI,
                         nsIURI* aReferrer,
                         uint32_t aReferrerPolicy,
                         nsISupports* aOwner,
                         uint32_t aFlags,
                         const char16_t* aWindowTarget,
                         const char* aTypeHint,
                         const nsAString& aFileName,
                         nsIInputStream* aPostData,
                         nsIInputStream* aHeadersData,
                         uint32_t aLoadType,
                         nsISHEntry* aSHEntry,
                         bool aFirstParty,
                         const nsAString& aSrcdoc,
                         nsIDocShell* aSourceDocShell,
                         nsIURI* aBaseURI,
                         nsIDocShell** aDocShell,
                         nsIRequest** aRequest)
{
  nsresult rv = NS_OK;
  mOriginalUriString.Truncate();

#ifdef PR_LOGGING
  if (gDocShellLeakLog && PR_LOG_TEST(gDocShellLeakLog, PR_LOG_DEBUG)) {
    nsAutoCString spec;
    if (aURI) {
      aURI->GetSpec(spec);
    }
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
    if ((isWyciwyg && NS_SUCCEEDED(rv)) || NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }
  }

  bool isJavaScript = false;
  if (NS_FAILED(aURI->SchemeIs("javascript", &isJavaScript))) {
    isJavaScript = false;
  }

  
  
  
  
  nsCOMPtr<Element> requestingElement;
  
  if (mScriptGlobal) {
    requestingElement = mScriptGlobal->GetFrameElementInternal();
  }

  nsRefPtr<nsGlobalWindow> MMADeathGrip = mScriptGlobal;

  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  uint32_t contentType;
  bool isNewDocShell = false;
  bool isTargetTopLevelDocShell = false;
  nsCOMPtr<nsIDocShell> targetDocShell;
  if (aWindowTarget && *aWindowTarget) {
    
    nsCOMPtr<nsIDocShellTreeItem> targetItem;
    rv = FindItemWithName(aWindowTarget, nullptr, this,
                          getter_AddRefs(targetItem));
    NS_ENSURE_SUCCESS(rv, rv);

    targetDocShell = do_QueryInterface(targetItem);
    
    
    isNewDocShell = !targetDocShell;

    
    
    
    if (targetDocShell) {
      nsCOMPtr<nsIDocShellTreeItem> sameTypeRoot;
      targetDocShell->GetSameTypeRootTreeItem(getter_AddRefs(sameTypeRoot));
      NS_ASSERTION(sameTypeRoot,
                   "No document shell root tree item from targetDocShell!");
      nsCOMPtr<nsIDocShell> rootShell = do_QueryInterface(sameTypeRoot);
      NS_ASSERTION(rootShell,
                   "No root docshell from document shell root tree item.");

      if (targetDocShell == rootShell) {
        isTargetTopLevelDocShell = true;
      }
    }
  }
  if (IsFrame() && !isNewDocShell && !isTargetTopLevelDocShell) {
    NS_ASSERTION(requestingElement, "A frame but no DOM element!?");
    contentType = nsIContentPolicy::TYPE_SUBDOCUMENT;
  } else {
    contentType = nsIContentPolicy::TYPE_DOCUMENT;
  }

  nsISupports* context = requestingElement;
  if (!context) {
    context = ToSupports(mScriptGlobal);
  }

  
  nsCOMPtr<nsIPrincipal> loadingPrincipal = do_QueryInterface(aOwner);
  if (!loadingPrincipal && aReferrer) {
    rv =
      CreatePrincipalFromReferrer(aReferrer, getter_AddRefs(loadingPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
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
        nsCOMPtr<nsIDocShell> itemDocShell = do_QueryInterface(treeItem);
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

    bool isNewWindow = false;
    if (!targetDocShell) {
      
      
      
      NS_ENSURE_TRUE(mContentViewer, NS_ERROR_FAILURE);
      nsIDocument* doc = mContentViewer->GetDocument();
      uint32_t sandboxFlags = 0;

      if (doc) {
        sandboxFlags = doc->GetSandboxFlags();
        if (sandboxFlags & SANDBOXED_AUXILIARY_NAVIGATION) {
          return NS_ERROR_DOM_INVALID_ACCESS_ERR;
        }
      }

      nsCOMPtr<nsPIDOMWindow> win = GetWindow();
      NS_ENSURE_TRUE(win, NS_ERROR_NOT_AVAILABLE);

      nsDependentString name(aWindowTarget);
      nsCOMPtr<nsIDOMWindow> newWin;
      nsAutoCString spec;
      if (aURI) {
        aURI->GetSpec(spec);
      }
      nsAutoString features;
      if (mInPrivateBrowsing) {
        features.AssignLiteral("private");
      }
      rv = win->OpenNoNavigate(NS_ConvertUTF8toUTF16(spec),
                               name,  
                               features,
                               getter_AddRefs(newWin));

      
      
      
      nsCOMPtr<nsPIDOMWindow> piNewWin = do_QueryInterface(newWin);
      if (piNewWin) {
        nsCOMPtr<nsIDocument> newDoc = piNewWin->GetExtantDoc();
        if (!newDoc || newDoc->IsInitialDocument()) {
          isNewWindow = true;
          aFlags |= INTERNAL_LOAD_FLAGS_FIRST_LOAD;

          
          if (aFlags & INTERNAL_LOAD_FLAGS_NO_OPENER) {
            piNewWin->SetOpenerWindow(nullptr, false);
          }
        }
      }

      nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(newWin);
      targetDocShell = do_QueryInterface(webNav);
    }

    
    
    
    
    if (NS_SUCCEEDED(rv) && targetDocShell) {
      rv = targetDocShell->InternalLoad(aURI,
                                        aReferrer,
                                        aReferrerPolicy,
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
                                        aSrcdoc,
                                        aSourceDocShell,
                                        aBaseURI,
                                        aDocShell,
                                        aRequest);
      if (rv == NS_ERROR_NO_CONTENT) {
        
        if (isNewWindow) {
          
          
          
          
          
          
          
          nsCOMPtr<nsIDOMWindow> domWin = targetDocShell->GetWindow();
          if (domWin) {
            domWin->Close();
          }
        }
        
        
        
        
        
        
        rv = NS_OK;
      } else if (isNewWindow) {
        
        
        
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

  if (mFiredUnloadEvent) {
    if (IsOKToLoadURI(aURI)) {
      NS_PRECONDITION(!aWindowTarget || !*aWindowTarget,
                      "Shouldn't have a window target here!");

      
      
      
      if (LOAD_TYPE_HAS_FLAGS(aLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {
        mLoadType = LOAD_NORMAL_REPLACE;
      }

      
      nsCOMPtr<nsIRunnable> ev =
        new InternalLoadEvent(this, aURI, aReferrer,
                              aReferrerPolicy, aOwner, aFlags,
                              aTypeHint, aPostData, aHeadersData,
                              aLoadType, aSHEntry, aFirstParty, aSrcdoc,
                              aSourceDocShell, aBaseURI);
      return NS_DispatchToCurrentThread(ev);
    }

    
    return NS_OK;
  }

  
  
  if (aSourceDocShell && aSourceDocShell->IsSandboxedFrom(this)) {
    return NS_ERROR_DOM_INVALID_ACCESS_ERR;
  }

  
  
  nsCOMPtr<nsIDocShellTreeItem> parent = GetParentDocshell();
  if (parent) {
    nsCOMPtr<nsIDocument> doc = do_GetInterface(parent);
    if (doc) {
      doc->TryCancelFrameLoaderInitialization(this);
    }
  }

  
  if (aLoadType == LOAD_NORMAL_EXTERNAL) {
    
    bool isChrome = false;
    if (NS_SUCCEEDED(aURI->SchemeIs("chrome", &isChrome)) && isChrome) {
      NS_WARNING("blocked external chrome: url -- use '--chrome' option");
      return NS_ERROR_FAILURE;
    }

    
    rv = CreateAboutBlankContentViewer(nullptr, nullptr);
    if (NS_FAILED(rv)) {
      return NS_ERROR_FAILURE;
    }

    
    
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
    nsCOMPtr<nsIURI> currentURI = mCurrentURI;
    
    
    
    nsAutoCString curBeforeHash, curHash, newBeforeHash, newHash;
    nsresult splitRv1, splitRv2;
    splitRv1 = currentURI ?
      nsContentUtils::SplitURIAtHash(currentURI, curBeforeHash, curHash) :
      NS_ERROR_FAILURE;
    splitRv2 = nsContentUtils::SplitURIAtHash(aURI, newBeforeHash, newHash);

    bool sameExceptHashes = NS_SUCCEEDED(splitRv1) &&
                            NS_SUCCEEDED(splitRv2) &&
                            curBeforeHash.Equals(newBeforeHash);

    if (!sameExceptHashes && sURIFixup && currentURI &&
        NS_SUCCEEDED(splitRv2)) {
      
      nsCOMPtr<nsIURI> currentExposableURI;
      rv = sURIFixup->CreateExposableURI(currentURI,
                                         getter_AddRefs(currentExposableURI));
      NS_ENSURE_SUCCESS(rv, rv);
      splitRv1 = nsContentUtils::SplitURIAtHash(currentExposableURI,
                                                curBeforeHash, curHash);
      sameExceptHashes =
        NS_SUCCEEDED(splitRv1) && curBeforeHash.Equals(newBeforeHash);
    }

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
      (!aSHEntry && !aPostData &&
       sameExceptHashes && !newHash.IsEmpty());

    if (doShortCircuitedLoad) {
      
      nscoord cx = 0, cy = 0;
      GetCurScrollPos(ScrollOrientation_X, &cx);
      GetCurScrollPos(ScrollOrientation_Y, &cy);

      
      
      
      
      AutoRestore<uint32_t> loadTypeResetter(mLoadType);

      
      
      
      if (JustStartedNetworkLoad() && (aLoadType & LOAD_CMD_NORMAL)) {
        mLoadType = LOAD_NORMAL_REPLACE;
      } else {
        mLoadType = aLoadType;
      }

      mURIResultedInDocument = true;

      nsCOMPtr<nsISHEntry> oldLSHE = mLSHE;

      




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

          
          
          
          if (mLSHE) {
            mLSHE->AdoptBFCacheEntry(mOSHE);
          }
        }
      }

      


      if (mLSHE) {
        SetHistoryEntry(&mOSHE, mLSHE);
        
        
        
        
        if (postData) {
          mOSHE->SetPostData(postData);
        }

        
        
        if (cacheKey) {
          mOSHE->SetCacheKey(cacheKey);
        }
      }

      


      SetHistoryEntry(&mLSHE, oldLSHE);
      


      if (mSessionHistory) {
        int32_t index = -1;
        mSessionHistory->GetIndex(&index);
        nsCOMPtr<nsISHEntry> shEntry;
        mSessionHistory->GetEntryAtIndex(index, false, getter_AddRefs(shEntry));
        NS_ENSURE_TRUE(shEntry, NS_ERROR_FAILURE);
        shEntry->SetTitle(mTitle);
      }

      

      if (mUseGlobalHistory && !mInPrivateBrowsing) {
        nsCOMPtr<IHistory> history = services::GetHistoryService();
        if (history) {
          history->SetURITitle(aURI, mTitle);
        } else if (mGlobalHistory) {
          mGlobalHistory->SetPageTitle(aURI, mTitle);
        }
      }

      
      nsCOMPtr<nsIDocument> doc = GetDocument();
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
      doc->SetDocumentURI(aURI);

      SetDocCurrentStateObj(mOSHE);

      
      
      CopyFavicon(currentURI, aURI, mInPrivateBrowsing);

      nsRefPtr<nsGlobalWindow> win = mScriptGlobal ?
        mScriptGlobal->GetCurrentInnerWindowInternal() : nullptr;

      
      
      
      
      
      rv = ScrollToAnchor(curHash, newHash, aLoadType);
      NS_ENSURE_SUCCESS(rv, rv);

      


      if (mOSHE && (aLoadType == LOAD_HISTORY ||
                    aLoadType == LOAD_RELOAD_NORMAL)) {
        nscoord bx, by;
        mOSHE->GetScrollPosition(&bx, &by);
        SetCurScrollPosEx(bx, by);
      }

      
      
      
      
      
      if (win) {
        
        bool doHashchange = sameExceptHashes && !curHash.Equals(newHash);

        if (historyNavBetweenSameDoc || doHashchange) {
          win->DispatchSyncPopState();
        }

        if (doHashchange) {
          
          
          win->DispatchAsyncHashchange(currentURI, aURI);
        }
      }

      return NS_OK;
    }
  }

  
  
  nsCOMPtr<nsIWebBrowserChrome3> browserChrome3 = do_GetInterface(mTreeOwner);
  if (browserChrome3) {
    bool shouldLoad;
    rv = browserChrome3->ShouldLoadURI(this, aURI, aReferrer, &shouldLoad);
    if (NS_SUCCEEDED(rv) && !shouldLoad) {
      return NS_OK;
    }
  }

  
  
  
  
  nsCOMPtr<nsIDocShell> kungFuDeathGrip(this);

  
  
  

  
  
  
  
  
  if (!isJavaScript) {
    MaybeInitTiming();
  }
  bool timeBeforeUnload = aFileName.IsVoid();
  if (mTiming && timeBeforeUnload) {
    mTiming->NotifyBeforeUnload();
  }
  
  
  if (!isJavaScript && aFileName.IsVoid() && mContentViewer) {
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

  
  
  
  
  
  if (!isJavaScript && aFileName.IsVoid()) {
    
    
    
    
    
    
    

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

    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  mLoadType = aLoadType;

  
  
  
  if (mLoadType != LOAD_ERROR_PAGE) {
    SetHistoryEntry(&mLSHE, aSHEntry);
  }

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
    if (restoring) {
      return rv;
    }

    
    
    
    if (NS_FAILED(rv)) {
      if (oldEntry) {
        oldEntry->SyncPresentationState();
      }

      aSHEntry->SyncPresentationState();
    }
  }

  nsAutoString srcdoc;
  if (aFlags & INTERNAL_LOAD_FLAGS_IS_SRCDOC) {
    srcdoc = aSrcdoc;
  } else {
    srcdoc = NullString();
  }

  net::PredictorLearn(aURI, nullptr,
                      nsINetworkPredictor::LEARN_LOAD_TOPLEVEL, this);
  net::PredictorPredict(aURI, nullptr,
                        nsINetworkPredictor::PREDICT_LOAD, this, nullptr);

  nsCOMPtr<nsIRequest> req;
  rv = DoURILoad(aURI, aReferrer,
                 !(aFlags & INTERNAL_LOAD_FLAGS_DONT_SEND_REFERRER),
                 aReferrerPolicy,
                 owner, aTypeHint, aFileName, aPostData, aHeadersData,
                 aFirstParty, aDocShell, getter_AddRefs(req),
                 (aFlags & INTERNAL_LOAD_FLAGS_FIRST_LOAD) != 0,
                 (aFlags & INTERNAL_LOAD_FLAGS_BYPASS_CLASSIFIER) != 0,
                 (aFlags & INTERNAL_LOAD_FLAGS_FORCE_ALLOW_COOKIES) != 0,
                 srcdoc, aBaseURI, contentType);
  if (req && aRequest) {
    NS_ADDREF(*aRequest = req);
  }

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
      document = parentItem->GetDocument();
    }
  }

  if (!document) {
    if (!aConsiderCurrentDocument) {
      return nullptr;
    }

    
    
    EnsureContentViewer();
    if (!mContentViewer) {
      return nullptr;
    }
    document = mContentViewer->GetDocument();
  }

  
  if (document) {
    nsIPrincipal* docPrincipal = document->NodePrincipal();

    
    
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
nsDocShell::DoURILoad(nsIURI* aURI,
                      nsIURI* aReferrerURI,
                      bool aSendReferrer,
                      uint32_t aReferrerPolicy,
                      nsISupports* aOwner,
                      const char* aTypeHint,
                      const nsAString& aFileName,
                      nsIInputStream* aPostData,
                      nsIInputStream* aHeadersData,
                      bool aFirstParty,
                      nsIDocShell** aDocShell,
                      nsIRequest** aRequest,
                      bool aIsNewWindowTarget,
                      bool aBypassClassifier,
                      bool aForceAllowCookies,
                      const nsAString& aSrcdoc,
                      nsIURI* aBaseURI,
                      nsContentPolicyType aContentPolicyType)
{
#ifdef MOZ_VISUAL_EVENT_TRACER
  nsAutoCString urlSpec;
  aURI->GetAsciiSpec(urlSpec);
  MOZ_EVENT_TRACER_NAME_OBJECT(this, urlSpec.BeginReading());
  MOZ_EVENT_TRACER_EXEC(this, "docshell::pageload");
#endif

  nsresult rv;
  nsCOMPtr<nsIURILoader> uriLoader;

  uriLoader = do_GetService(NS_URI_LOADER_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsLoadFlags loadFlags = mDefaultLoadFlags;
  if (aFirstParty) {
    
    loadFlags |= nsIChannel::LOAD_INITIAL_DOCUMENT_URI;
  }

  if (mLoadType == LOAD_ERROR_PAGE) {
    
    loadFlags |= nsIChannel::LOAD_BACKGROUND;
  }

  if (IsFrame()) {
    
    
    
    
    
    nsCOMPtr<nsIURI> tempURI = aURI;
    nsCOMPtr<nsINestedURI> nestedURI = do_QueryInterface(tempURI);
    while (nestedURI) {
      
      
      bool isViewSource = false;
      rv = tempURI->SchemeIs("view-source", &isViewSource);
      if (NS_FAILED(rv) || isViewSource) {
        return NS_ERROR_UNKNOWN_PROTOCOL;
      }
      nestedURI->GetInnerURI(getter_AddRefs(tempURI));
      nestedURI = do_QueryInterface(tempURI);
    }
  }

  
  nsCOMPtr<nsIChannel> channel;

  bool isSrcdoc = !aSrcdoc.IsVoid();

  nsCOMPtr<nsINode> requestingNode;
  if (mScriptGlobal) {
    requestingNode = mScriptGlobal->GetFrameElementInternal();
    if (!requestingNode) {
      requestingNode = mScriptGlobal->GetExtantDoc();
    }
  }

  bool isSandBoxed = mSandboxFlags & SANDBOXED_ORIGIN;
  
  bool inherit = false;

  nsCOMPtr<nsIPrincipal> triggeringPrincipal = do_QueryInterface(aOwner);
  if (triggeringPrincipal) {
    inherit = nsContentUtils::ChannelShouldInheritPrincipal(
      triggeringPrincipal,
      aURI,
      true, 
      isSrcdoc);
  } else if (!triggeringPrincipal && aReferrerURI) {
    rv = CreatePrincipalFromReferrer(aReferrerURI,
                                     getter_AddRefs(triggeringPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    triggeringPrincipal = nsContentUtils::GetSystemPrincipal();
  }

  nsSecurityFlags securityFlags = nsILoadInfo::SEC_NORMAL;
  if (inherit) {
    securityFlags |= nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL;
  }
  if (isSandBoxed) {
    securityFlags |= nsILoadInfo::SEC_SANDBOXED;
  }

  if (!isSrcdoc) {
    nsCOMPtr<nsILoadInfo> loadInfo =
      new LoadInfo(requestingNode ? requestingNode->NodePrincipal() :
                                    triggeringPrincipal.get(),
                   triggeringPrincipal,
                   requestingNode,
                   securityFlags,
                   aContentPolicyType,
                   aBaseURI);
    rv = NS_NewChannelInternal(getter_AddRefs(channel),
                               aURI,
                               loadInfo,
                               nullptr,   
                               static_cast<nsIInterfaceRequestor*>(this),
                               loadFlags);

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
  } else {
    nsAutoCString scheme;
    rv = aURI->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    bool isViewSource;
    aURI->SchemeIs("view-source", &isViewSource);

    if (isViewSource) {
      nsViewSourceHandler* vsh = nsViewSourceHandler::GetInstance();
      NS_ENSURE_TRUE(vsh, NS_ERROR_FAILURE);

      rv = vsh->NewSrcdocChannel(aURI, aSrcdoc, getter_AddRefs(channel));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<nsILoadInfo> loadInfo =
        new LoadInfo(requestingNode ? requestingNode->NodePrincipal() :
                                      triggeringPrincipal.get(),
                     triggeringPrincipal,
                     requestingNode,
                     securityFlags,
                     aContentPolicyType,
                     aBaseURI);
      channel->SetLoadInfo(loadInfo);
    } else {
      rv = NS_NewInputStreamChannelInternal(getter_AddRefs(channel),
                                            aURI,
                                            aSrcdoc,
                                            NS_LITERAL_CSTRING("text/html"),
                                            requestingNode,
                                            requestingNode ?
                                              requestingNode->NodePrincipal() :
                                              triggeringPrincipal.get(),
                                            triggeringPrincipal,
                                            securityFlags,
                                            aContentPolicyType,
                                            true,
                                            aBaseURI);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
    do_QueryInterface(channel);
  if (appCacheChannel) {
    
    appCacheChannel->SetInheritApplicationCache(false);

    
    
    if (GeckoProcessType_Default != XRE_GetProcessType()) {
      
      appCacheChannel->SetChooseApplicationCache(true);
    } else {
      nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);

      if (secMan) {
        nsCOMPtr<nsIPrincipal> principal;
        secMan->GetDocShellCodebasePrincipal(aURI, this,
                                             getter_AddRefs(principal));
        appCacheChannel->SetChooseApplicationCache(
          NS_ShouldCheckAppCache(principal, mInPrivateBrowsing));
      }
    }
  }

  
  
  if (aRequest) {
    NS_ADDREF(*aRequest = channel);
  }

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

  if (mLoadType == LOAD_NORMAL_ALLOW_MIXED_CONTENT ||
      mLoadType == LOAD_RELOAD_ALLOW_MIXED_CONTENT) {
    rv = SetMixedContentChannel(channel);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mMixedContentChannel) {
    








    rv = nsContentUtils::CheckSameOrigin(mMixedContentChannel, channel);
    if (NS_FAILED(rv) || NS_FAILED(SetMixedContentChannel(channel))) {
      SetMixedContentChannel(nullptr);
    }
  }

  
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  nsCOMPtr<nsIHttpChannelInternal> httpChannelInternal(
    do_QueryInterface(channel));
  if (httpChannelInternal) {
    if (aForceAllowCookies) {
      httpChannelInternal->SetThirdPartyFlags(
        nsIHttpChannelInternal::THIRD_PARTY_FORCE_ALLOW);
    }
    if (aFirstParty) {
      httpChannelInternal->SetDocumentURI(aURI);
    } else {
      httpChannelInternal->SetDocumentURI(aReferrerURI);
    }
  }

  nsCOMPtr<nsIWritablePropertyBag2> props(do_QueryInterface(channel));
  if (props) {
    
    
    props->SetPropertyAsInterface(NS_LITERAL_STRING("docshell.internalReferrer"),
                                  aReferrerURI);
  }

  
  
  
  
  if (httpChannel) {
    nsCOMPtr<nsICachingChannel> cacheChannel(do_QueryInterface(httpChannel));
    
    nsCOMPtr<nsISupports> cacheKey;
    if (mLSHE) {
      mLSHE->GetCacheKey(getter_AddRefs(cacheKey));
    } else if (mOSHE) {  
      mOSHE->GetCacheKey(getter_AddRefs(cacheKey));
    }

    
    
    if (aPostData) {
      
      
      
      nsCOMPtr<nsISeekableStream> postDataSeekable =
        do_QueryInterface(aPostData);
      if (postDataSeekable) {
        rv = postDataSeekable->Seek(nsISeekableStream::NS_SEEK_SET, 0);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(httpChannel));
      NS_ASSERTION(uploadChannel, "http must support nsIUploadChannel");

      
      uploadChannel->SetUploadStream(aPostData, EmptyCString(), -1);
      




      if (cacheChannel && cacheKey) {
        if (mLoadType == LOAD_HISTORY ||
            mLoadType == LOAD_RELOAD_CHARSET_CHANGE) {
          cacheChannel->SetCacheKey(cacheKey);
          uint32_t loadFlags;
          if (NS_SUCCEEDED(channel->GetLoadFlags(&loadFlags))) {
            channel->SetLoadFlags(
              loadFlags | nsICachingChannel::LOAD_ONLY_FROM_CACHE);
          }
        } else if (mLoadType == LOAD_RELOAD_NORMAL) {
          cacheChannel->SetCacheKey(cacheKey);
        }
      }
    } else {
      





      if (mLoadType == LOAD_HISTORY ||
          mLoadType == LOAD_RELOAD_NORMAL ||
          mLoadType == LOAD_RELOAD_CHARSET_CHANGE) {
        if (cacheChannel && cacheKey) {
          cacheChannel->SetCacheKey(cacheKey);
        }
      }
    }
    if (aHeadersData) {
      rv = AddHeadersToChannel(aHeadersData, httpChannel);
    }
    
    if (aReferrerURI && aSendReferrer) {
      
      httpChannel->SetReferrerWithPolicy(aReferrerURI, aReferrerPolicy);
    }
  }

  nsCOMPtr<nsIScriptChannel> scriptChannel = do_QueryInterface(channel);
  if (scriptChannel) {
    
    scriptChannel->SetExecutionPolicy(nsIScriptChannel::EXECUTE_NORMAL);
  }

  if (aIsNewWindowTarget) {
    nsCOMPtr<nsIWritablePropertyBag2> props = do_QueryInterface(channel);
    if (props) {
      props->SetPropertyAsBool(NS_LITERAL_STRING("docshell.newWindowTarget"),
                               true);
    }
  }

  nsCOMPtr<nsITimedChannel> timedChannel(do_QueryInterface(channel));
  if (timedChannel) {
    timedChannel->SetTimingEnabled(true);
    if (IsFrame()) {
      timedChannel->SetInitiatorType(NS_LITERAL_STRING("subdocument"));
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
AppendSegmentToString(nsIInputStream* aIn,
                      void* aClosure,
                      const char* aFromRawSegment,
                      uint32_t aToOffset,
                      uint32_t aCount,
                      uint32_t* aWriteCount)
{
  

  nsAutoCString* buf = static_cast<nsAutoCString*>(aClosure);
  buf->Append(aFromRawSegment, aCount);

  
  *aWriteCount = aCount;
  return NS_OK;
}

nsresult
nsDocShell::AddHeadersToChannel(nsIInputStream* aHeadersData,
                                nsIChannel* aGenericChannel)
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
    if (crlf == kNotFound) {
      return NS_OK;
    }

    const nsCSubstring& oneHeader = StringHead(headersString, crlf);

    colon = oneHeader.FindChar(':');
    if (colon == kNotFound) {
      return NS_ERROR_UNEXPECTED;
    }

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

nsresult
nsDocShell::DoChannelLoad(nsIChannel* aChannel,
                          nsIURILoader* aURILoader,
                          bool aBypassClassifier)
{
  nsresult rv;
  
  nsLoadFlags loadFlags = 0;
  (void)aChannel->GetLoadFlags(&loadFlags);
  loadFlags |= nsIChannel::LOAD_DOCUMENT_URI |
               nsIChannel::LOAD_CALL_CONTENT_SNIFFERS;

  
  switch (mLoadType) {
    case LOAD_HISTORY: {
      
      
      bool uriModified = false;
      if (mLSHE) {
        mLSHE->GetURIWasModified(&uriModified);
      }

      if (!uriModified) {
        loadFlags |= nsIRequest::VALIDATE_NEVER;
      }
      break;
    }

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
    case LOAD_NORMAL_ALLOW_MIXED_CONTENT:
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

  (void)aChannel->SetLoadFlags(loadFlags);

  uint32_t openFlags = 0;
  if (mLoadType == LOAD_LINK) {
    openFlags |= nsIURILoader::IS_CONTENT_PREFERRED;
  }
  if (!mAllowContentRetargeting) {
    openFlags |= nsIURILoader::DONT_RETARGET;
  }
  rv = aURILoader->OpenURI(aChannel, openFlags, this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsDocShell::ScrollToAnchor(nsACString& aCurHash, nsACString& aNewHash,
                           uint32_t aLoadType)
{
  if (!mCurrentURI) {
    return NS_OK;
  }

  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  if (!shell) {
    
    
    return NS_OK;
  }

  nsIScrollableFrame* rootScroll = shell->GetRootScrollFrameAsScrollable();
  if (rootScroll) {
    rootScroll->ClearDidHistoryRestore();
  }

  
  
  
  
  if ((aCurHash.IsEmpty() || aLoadType != LOAD_HISTORY) &&
      aNewHash.IsEmpty()) {
    return NS_OK;
  }

  
  
  nsDependentCSubstring newHashName(aNewHash, 1);

  
  

  if (!newHashName.IsEmpty()) {
    
    
    bool scroll = aLoadType != LOAD_HISTORY &&
                  aLoadType != LOAD_RELOAD_NORMAL;

    char* str = ToNewCString(newHashName);
    if (!str) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    nsUnescape(str);

    
    
    

    
    
    
    
    
    nsresult rv = NS_ERROR_FAILURE;
    NS_ConvertUTF8toUTF16 uStr(str);
    if (!uStr.IsEmpty()) {
      rv = shell->GoToAnchor(NS_ConvertUTF8toUTF16(str), scroll,
                             nsIPresShell::SCROLL_SMOOTH_AUTO);
    }
    free(str);

    
    
    if (NS_FAILED(rv)) {
      
      NS_ENSURE_TRUE(mContentViewer, NS_ERROR_FAILURE);
      nsIDocument* doc = mContentViewer->GetDocument();
      NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);
      const nsACString& aCharset = doc->GetDocumentCharacterSet();

      nsCOMPtr<nsITextToSubURI> textToSubURI =
        do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      
      nsXPIDLString uStr;

      rv = textToSubURI->UnEscapeAndConvert(PromiseFlatCString(aCharset).get(),
                                            PromiseFlatCString(newHashName).get(),
                                            getter_Copies(uStr));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      
      
      
      
      
      
      shell->GoToAnchor(uStr, scroll && !uStr.IsEmpty(),
                        nsIPresShell::SCROLL_SMOOTH_AUTO);
    }
  } else {
    
    shell->GoToAnchor(EmptyString(), false);

    
    
    
    
    if (aLoadType == LOAD_HISTORY || aLoadType == LOAD_RELOAD_NORMAL) {
      return NS_OK;
    }
    
    
    
    SetCurScrollPosEx(0, 0);
  }

  return NS_OK;
}

void
nsDocShell::SetupReferrerFromChannel(nsIChannel* aChannel)
{
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
  if (httpChannel) {
    nsCOMPtr<nsIURI> referrer;
    nsresult rv = httpChannel->GetReferrer(getter_AddRefs(referrer));
    if (NS_SUCCEEDED(rv)) {
      SetReferrerURI(referrer);
    }
    uint32_t referrerPolicy;
    rv = httpChannel->GetReferrerPolicy(&referrerPolicy);
    if (NS_SUCCEEDED(rv)) {
      SetReferrerPolicy(referrerPolicy);
    }
  }
}

bool
nsDocShell::OnNewURI(nsIURI* aURI, nsIChannel* aChannel, nsISupports* aOwner,
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
    if (aChannel) {
      aChannel->GetName(chanName);
    } else {
      chanName.AssignLiteral("<no channel>");
    }

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

    
    if (!httpChannel) {
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

  
  if (mCurrentURI) {
    aURI->Equals(mCurrentURI, &equalUri);
  }

#ifdef DEBUG
  bool shAvailable = (rootSH != nullptr);

  
  

  PR_LOG(gDocShellLog, PR_LOG_DEBUG,
         ("  shAvailable=%i updateSHistory=%i updateGHistory=%i"
          " equalURI=%i\n",
          shAvailable, updateSHistory, updateGHistory, equalUri));

  if (shAvailable && mCurrentURI && !mOSHE && aLoadType != LOAD_ERROR_PAGE) {
    NS_ASSERTION(NS_IsAboutBlank(mCurrentURI),
                 "no SHEntry for a non-transient viewer?");
  }
#endif

  
















  if (equalUri &&
      mOSHE &&
      (mLoadType == LOAD_NORMAL ||
       mLoadType == LOAD_LINK ||
       mLoadType == LOAD_STOP_CONTENT) &&
      !inputStream) {
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
    nsCOMPtr<nsISupports> cacheKey;
    
    if (cacheChannel) {
      cacheChannel->GetCacheKey(getter_AddRefs(cacheKey));
    }
    
    
    
    if (mLSHE) {
      mLSHE->SetCacheKey(cacheKey);
    } else if (mOSHE) {
      mOSHE->SetCacheKey(cacheKey);
    }

    
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
      



      (void)AddToSessionHistory(aURI, aChannel, aOwner, aCloneSHChildren,
                                getter_AddRefs(mLSHE));
    }
  }

  
  
  if (updateGHistory && aAddToGlobalHistory && !ChannelIsPost(aChannel)) {
    nsCOMPtr<nsIURI> previousURI;
    uint32_t previousFlags = 0;

    if (aLoadType & LOAD_CMD_RELOAD) {
      
      previousURI = aURI;
    } else {
      ExtractLastVisit(aChannel, getter_AddRefs(previousURI), &previousFlags);
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

  
  uint32_t locationFlags =
    aCloneSHChildren ? uint32_t(LOCATION_CHANGE_SAME_DOCUMENT) : 0;

  bool onLocationChangeNeeded = SetCurrentURI(aURI, aChannel,
                                              aFireOnLocationChange,
                                              locationFlags);
  
  SetupReferrerFromChannel(aChannel);
  return onLocationChangeNeeded;
}

bool
nsDocShell::OnLoadingSite(nsIChannel* aChannel, bool aFireOnLocationChange,
                          bool aAddToGlobalHistory)
{
  nsCOMPtr<nsIURI> uri;
  
  
  
  
  NS_GetFinalChannelURI(aChannel, getter_AddRefs(uri));
  NS_ENSURE_TRUE(uri, false);

  
  return OnNewURI(uri, aChannel, nullptr, mLoadType, aFireOnLocationChange,
                  aAddToGlobalHistory, false);
}

void
nsDocShell::SetReferrerURI(nsIURI* aURI)
{
  mReferrerURI = aURI;  
}

void
nsDocShell::SetReferrerPolicy(uint32_t aReferrerPolicy)
{
  mReferrerPolicy = aReferrerPolicy;
}





NS_IMETHODIMP
nsDocShell::AddState(JS::Handle<JS::Value> aData, const nsAString& aTitle,
                     const nsAString& aURL, bool aReplace, JSContext* aCx)
{
  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsresult rv;

  
  AutoRestore<uint32_t> loadTypeResetter(mLoadType);

  
  
  
  if (JustStartedNetworkLoad()) {
    aReplace = true;
  }

  nsCOMPtr<nsIDocument> document = GetDocument();
  NS_ENSURE_TRUE(document, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIStructuredCloneContainer> scContainer;

  
  
  
  
  {
    nsCOMPtr<nsIDocument> origDocument = GetDocument();
    if (!origDocument) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
    nsCOMPtr<nsIPrincipal> origPrincipal = origDocument->NodePrincipal();

    scContainer = new nsStructuredCloneContainer();
    rv = scContainer->InitFromJSVal(aData);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDocument> newDocument = GetDocument();
    if (!newDocument) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }
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

  NS_ENSURE_TRUE(scSize <= (uint32_t)maxStateObjSize, NS_ERROR_ILLEGAL_VALUE);

  
  bool equalURIs = true;
  nsCOMPtr<nsIURI> currentURI;
  if (sURIFixup && mCurrentURI) {
    rv = sURIFixup->CreateExposableURI(mCurrentURI, getter_AddRefs(currentURI));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    currentURI = mCurrentURI;
  }
  nsCOMPtr<nsIURI> oldURI = currentURI;
  nsCOMPtr<nsIURI> newURI;
  if (aURL.Length() == 0) {
    newURI = currentURI;
  } else {
    

    nsIURI* docBaseURI = document->GetDocBaseURI();
    if (!docBaseURI) {
      return NS_ERROR_FAILURE;
    }

    nsAutoCString spec;
    docBaseURI->GetSpec(spec);

    nsAutoCString charset;
    rv = docBaseURI->GetOriginCharset(charset);
    NS_ENSURE_SUCCESS(rv, NS_ERROR_FAILURE);

    rv = NS_NewURI(getter_AddRefs(newURI), aURL, charset.get(), docBaseURI);

    
    if (NS_FAILED(rv)) {
      return NS_ERROR_DOM_SECURITY_ERR;
    }

    
    if (!nsContentUtils::URIIsLocalFile(newURI)) {
      
      
      
      
      
      
      
      

      nsCOMPtr<nsIScriptSecurityManager> secMan =
        do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID);
      NS_ENSURE_TRUE(secMan, NS_ERROR_FAILURE);

      
      
      
      nsAutoCString currentUserPass, newUserPass;
      NS_ENSURE_SUCCESS(currentURI->GetUserPass(currentUserPass),
                        NS_ERROR_FAILURE);
      NS_ENSURE_SUCCESS(newURI->GetUserPass(newUserPass), NS_ERROR_FAILURE);
      if (NS_FAILED(secMan->CheckSameOriginURI(currentURI, newURI, true)) ||
          !currentUserPass.Equals(newUserPass)) {
        return NS_ERROR_DOM_SECURITY_ERR;
      }
    } else {
      
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

    if (currentURI) {
      currentURI->Equals(newURI, &equalURIs);
    } else {
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

    
    
    NS_ENSURE_SUCCESS(newSHEntry->AdoptBFCacheEntry(oldOSHE), NS_ERROR_FAILURE);

    
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
  newURI->EqualsExceptRef(currentURI, &sameExceptHashes);
  oldOSHE->GetURIWasModified(&oldURIWasModified);
  newSHEntry->SetURIWasModified(!sameExceptHashes || oldURIWasModified);

  
  
  
  
  
  nsCOMPtr<nsISHistory> rootSH;
  GetRootSessionHistory(getter_AddRefs(rootSH));
  NS_ENSURE_TRUE(rootSH, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsISHistoryInternal> internalSH = do_QueryInterface(rootSH);
  NS_ENSURE_TRUE(internalSH, NS_ERROR_UNEXPECTED);

  if (!aReplace) {
    int32_t curIndex = -1;
    rv = rootSH->GetIndex(&curIndex);
    if (NS_SUCCEEDED(rv) && curIndex > -1) {
      internalSH->EvictOutOfRangeContentViewers(curIndex);
    }
  } else {
    nsCOMPtr<nsISHEntry> rootSHEntry = GetRootSHEntry(newSHEntry);

    int32_t index = -1;
    rv = rootSH->GetIndexOfEntry(rootSHEntry, &index);
    if (NS_SUCCEEDED(rv) && index > -1) {
      internalSH->ReplaceEntry(index, rootSHEntry);
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
      } else if (mGlobalHistory) {
        mGlobalHistory->SetPageTitle(newURI, mTitle);
      }
    }

    
    
    CopyFavicon(oldURI, newURI, mInPrivateBrowsing);
  } else {
    FireDummyOnLocationChange();
  }
  document->SetStateObject(scContainer);

  return NS_OK;
}

bool
nsDocShell::ShouldAddToSessionHistory(nsIURI* aURI)
{
  
  
  
  
  nsresult rv;
  nsAutoCString buf, pref;

  rv = aURI->GetScheme(buf);
  if (NS_FAILED(rv)) {
    return false;
  }

  if (buf.EqualsLiteral("about")) {
    rv = aURI->GetPath(buf);
    if (NS_FAILED(rv)) {
      return false;
    }

    if (buf.EqualsLiteral("blank")) {
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
nsDocShell::AddToSessionHistory(nsIURI* aURI, nsIChannel* aChannel,
                                nsISupports* aOwner, bool aCloneChildren,
                                nsISHEntry** aNewEntry)
{
  NS_PRECONDITION(aURI, "uri is null");
  NS_PRECONDITION(!aChannel || !aOwner, "Shouldn't have both set");

#if defined(PR_LOGGING) && defined(DEBUG)
  if (PR_LOG_TEST(gDocShellLog, PR_LOG_DEBUG)) {
    nsAutoCString spec;
    aURI->GetSpec(spec);

    nsAutoCString chanName;
    if (aChannel) {
      aChannel->GetName(chanName);
    } else {
      chanName.AssignLiteral("<no channel>");
    }

    PR_LOG(gDocShellLog, PR_LOG_DEBUG,
           ("nsDocShell[%p]::AddToSessionHistory(\"%s\", [%s])\n",
            this, spec.get(), chanName.get()));
  }
#endif

  nsresult rv = NS_OK;
  nsCOMPtr<nsISHEntry> entry;
  bool shouldPersist;

  shouldPersist = ShouldAddToSessionHistory(aURI);

  
  nsCOMPtr<nsIDocShellTreeItem> root;
  GetSameTypeRootTreeItem(getter_AddRefs(root));
  




  if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY) &&
      root != static_cast<nsIDocShellTreeItem*>(this)) {
    
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
      entry->AbandonBFCacheEntry();
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
  uint32_t referrerPolicy = mozilla::net::RP_Default;
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
      httpChannel->GetReferrerPolicy(&referrerPolicy);

      discardLayoutState = ShouldDiscardLayoutState(httpChannel);
    }
    aChannel->GetOwner(getter_AddRefs(owner));
    if (!owner) {
      nsCOMPtr<nsILoadInfo> loadInfo;
      aChannel->GetLoadInfo(getter_AddRefs(loadInfo));
      if (loadInfo) {
        
        if (loadInfo->GetLoadingSandboxed()) {
          owner = nsNullPrincipal::CreateWithInheritedAttributes(
            loadInfo->LoadingPrincipal());
          NS_ENSURE_TRUE(owner, NS_ERROR_FAILURE);
        } else if (loadInfo->GetForceInheritPrincipal()) {
          owner = loadInfo->TriggeringPrincipal();
        }
      }
    }
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
  entry->SetReferrerPolicy(referrerPolicy);
  nsCOMPtr<nsIInputStreamChannel> inStrmChan = do_QueryInterface(aChannel);
  if (inStrmChan) {
    bool isSrcdocChannel;
    inStrmChan->GetIsSrcdocChannel(&isSrcdocChannel);
    if (isSrcdocChannel) {
      nsAutoString srcdoc;
      inStrmChan->GetSrcdocData(srcdoc);
      entry->SetSrcdocData(srcdoc);
      nsCOMPtr<nsILoadInfo> loadInfo;
      aChannel->GetLoadInfo(getter_AddRefs(loadInfo));
      nsCOMPtr<nsIURI> baseURI;
      loadInfo->GetBaseURI(getter_AddRefs(baseURI));
      entry->SetBaseURI(baseURI);
    }
  }
  



  if (discardLayoutState) {
    entry->SetSaveLayoutStateFlag(false);
  }
  if (cacheChannel) {
    
    uint32_t expTime = 0;
    cacheChannel->GetCacheTokenExpirationTime(&expTime);
    uint32_t now = PRTimeToSeconds(PR_Now());
    if (expTime <= now) {
      expired = true;
    }
  }
  if (expired) {
    entry->SetExpirationStatus(true);
  }

  if (root == static_cast<nsIDocShellTreeItem*>(this) && mSessionHistory) {
    
    
    if (aCloneChildren && mOSHE) {
      uint32_t cloneID;
      mOSHE->GetID(&cloneID);
      nsCOMPtr<nsISHEntry> newEntry;
      CloneAndReplace(mOSHE, this, cloneID, entry, true,
                      getter_AddRefs(newEntry));
      NS_ASSERTION(entry == newEntry,
                   "The new session history should be in the new entry");
    }

    
    if (LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {
      
      int32_t index = 0;
      mSessionHistory->GetIndex(&index);
      nsCOMPtr<nsISHistoryInternal> shPrivate =
        do_QueryInterface(mSessionHistory);
      
      if (shPrivate) {
        rv = shPrivate->ReplaceEntry(index, entry);
      }
    } else {
      
      nsCOMPtr<nsISHistoryInternal> shPrivate =
        do_QueryInterface(mSessionHistory);
      NS_ENSURE_TRUE(shPrivate, NS_ERROR_FAILURE);
      mSessionHistory->GetIndex(&mPreviousTransIndex);
      rv = shPrivate->AddEntry(entry, shouldPersist);
      mSessionHistory->GetIndex(&mLoadedTransIndex);
#ifdef DEBUG_PAGE_CACHE
      printf("Previous index: %d, Loaded index: %d\n\n",
             mPreviousTransIndex, mLoadedTransIndex);
#endif
    }
  } else {
    
    if (!mOSHE || !LOAD_TYPE_HAS_FLAGS(mLoadType, LOAD_FLAGS_REPLACE_HISTORY)) {
      rv = AddChildSHEntryToParent(entry, mChildOffset, aCloneChildren);
    }
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

nsresult
nsDocShell::LoadHistoryEntry(nsISHEntry* aEntry, uint32_t aLoadType)
{
  if (!IsNavigationAllowed()) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<nsIInputStream> postData;
  nsCOMPtr<nsIURI> referrerURI;
  uint32_t referrerPolicy;
  nsAutoCString contentType;
  nsCOMPtr<nsISupports> owner;

  NS_ENSURE_TRUE(aEntry, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(aEntry->GetURI(getter_AddRefs(uri)), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(aEntry->GetReferrerURI(getter_AddRefs(referrerURI)),
                    NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(aEntry->GetReferrerPolicy(&referrerPolicy),
                    NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(aEntry->GetPostData(getter_AddRefs(postData)),
                    NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(aEntry->GetContentType(contentType), NS_ERROR_FAILURE);
  NS_ENSURE_SUCCESS(aEntry->GetOwner(getter_AddRefs(owner)), NS_ERROR_FAILURE);

  
  
  
  
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
      
      
      
      owner = nsNullPrincipal::Create();
      NS_ENSURE_TRUE(owner, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  



  if ((aLoadType & LOAD_CMD_RELOAD) && postData) {
    bool repost;
    rv = ConfirmRepost(&repost);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    if (!repost) {
      return NS_BINDING_ABORTED;
    }
  }

  
  uint32_t flags = INTERNAL_LOAD_FLAGS_NONE;

  nsAutoString srcdoc;
  bool isSrcdoc;
  nsCOMPtr<nsIURI> baseURI;
  aEntry->GetIsSrcdocEntry(&isSrcdoc);
  if (isSrcdoc) {
    aEntry->GetSrcdocData(srcdoc);
    aEntry->GetBaseURI(getter_AddRefs(baseURI));
    flags |= INTERNAL_LOAD_FLAGS_IS_SRCDOC;
  } else {
    srcdoc = NullString();
  }

  
  
  
  
  rv = InternalLoad(uri,
                    referrerURI,
                    referrerPolicy,
                    owner,
                    flags,
                    nullptr,            
                    contentType.get(),  
                    NullString(),       
                    postData,           
                    nullptr,            
                    aLoadType,          
                    aEntry,             
                    true,
                    srcdoc,
                    nullptr,            
                    baseURI,
                    nullptr,            
                    nullptr);           
  return rv;
}

NS_IMETHODIMP
nsDocShell::GetShouldSaveLayoutState(bool* aShould)
{
  *aShould = false;
  if (mOSHE) {
    
    
    mOSHE->GetSaveLayoutStateFlag(aShould);
  }

  return NS_OK;
}

nsresult
nsDocShell::PersistLayoutHistoryState()
{
  nsresult rv = NS_OK;

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
nsDocShell::WalkHistoryEntries(nsISHEntry* aRootEntry,
                               nsDocShell* aRootShell,
                               WalkHistoryEntriesFunc aCallback,
                               void* aData)
{
  NS_ENSURE_TRUE(aRootEntry, NS_ERROR_FAILURE);

  nsCOMPtr<nsISHContainer> container(do_QueryInterface(aRootEntry));
  if (!container) {
    return NS_ERROR_FAILURE;
  }

  int32_t childCount;
  container->GetChildCount(&childCount);
  for (int32_t i = 0; i < childCount; i++) {
    nsCOMPtr<nsISHEntry> childEntry;
    container->GetChildAt(i, getter_AddRefs(childEntry));
    if (!childEntry) {
      
      
      
      aCallback(nullptr, nullptr, i, aData);
      continue;
    }

    nsDocShell* childShell = nullptr;
    if (aRootShell) {
      
      
      nsTObserverArray<nsDocLoader*>::ForwardIterator iter(aRootShell->mChildList);
      while (iter.HasMore()) {
        nsDocShell* child = static_cast<nsDocShell*>(iter.GetNext());

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


struct MOZ_STACK_CLASS CloneAndReplaceData
{
  CloneAndReplaceData(uint32_t aCloneID, nsISHEntry* aReplaceEntry,
                      bool aCloneChildren, nsISHEntry* aDestTreeParent)
    : cloneID(aCloneID)
    , cloneChildren(aCloneChildren)
    , replaceEntry(aReplaceEntry)
    , destTreeParent(aDestTreeParent)
  {
  }

  uint32_t cloneID;
  bool cloneChildren;
  nsISHEntry* replaceEntry;
  nsISHEntry* destTreeParent;
  nsCOMPtr<nsISHEntry> resultEntry;
};

 nsresult
nsDocShell::CloneAndReplaceChild(nsISHEntry* aEntry, nsDocShell* aShell,
                                 int32_t aEntryIndex, void* aData)
{
  nsCOMPtr<nsISHEntry> dest;

  CloneAndReplaceData* data = static_cast<CloneAndReplaceData*>(aData);
  uint32_t cloneID = data->cloneID;
  nsISHEntry* replaceEntry = data->replaceEntry;

  nsCOMPtr<nsISHContainer> container = do_QueryInterface(data->destTreeParent);
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

  if (container) {
    container->AddChild(dest, aEntryIndex);
  }

  data->resultEntry = dest;
  return rv;
}

 nsresult
nsDocShell::CloneAndReplace(nsISHEntry* aSrcEntry,
                            nsDocShell* aSrcShell,
                            uint32_t aCloneID,
                            nsISHEntry* aReplaceEntry,
                            bool aCloneChildren,
                            nsISHEntry** aResultEntry)
{
  NS_ENSURE_ARG_POINTER(aResultEntry);
  NS_ENSURE_TRUE(aReplaceEntry, NS_ERROR_FAILURE);

  CloneAndReplaceData data(aCloneID, aReplaceEntry, aCloneChildren, nullptr);
  nsresult rv = CloneAndReplaceChild(aSrcEntry, aSrcShell, 0, &data);

  data.resultEntry.swap(*aResultEntry);
  return rv;
}

void
nsDocShell::SwapHistoryEntries(nsISHEntry* aOldEntry, nsISHEntry* aNewEntry)
{
  if (aOldEntry == mOSHE) {
    mOSHE = aNewEntry;
  }

  if (aOldEntry == mLSHE) {
    mLSHE = aNewEntry;
  }
}

struct SwapEntriesData
{
  nsDocShell* ignoreShell;     
  nsISHEntry* destTreeRoot;    
  nsISHEntry* destTreeParent;  
                               
};

nsresult
nsDocShell::SetChildHistoryEntry(nsISHEntry* aEntry, nsDocShell* aShell,
                                 int32_t aEntryIndex, void* aData)
{
  SwapEntriesData* data = static_cast<SwapEntriesData*>(aData);
  nsDocShell* ignoreShell = data->ignoreShell;

  if (!aShell || aShell == ignoreShell) {
    return NS_OK;
  }

  nsISHEntry* destTreeRoot = data->destTreeRoot;

  nsCOMPtr<nsISHEntry> destEntry;
  nsCOMPtr<nsISHContainer> container = do_QueryInterface(data->destTreeParent);

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
        if (!entry) {
          continue;
        }

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
  return WalkHistoryEntries(aEntry, aShell, SetChildHistoryEntry, &childData);
}

static nsISHEntry*
GetRootSHEntry(nsISHEntry* aEntry)
{
  nsCOMPtr<nsISHEntry> rootEntry = aEntry;
  nsISHEntry* result = nullptr;
  while (rootEntry) {
    result = rootEntry;
    result->GetParent(getter_AddRefs(rootEntry));
  }

  return result;
}

void
nsDocShell::SetHistoryEntry(nsCOMPtr<nsISHEntry>* aPtr, nsISHEntry* aEntry)
{
  
  
  
  
  
  

  nsISHEntry* newRootEntry = GetRootSHEntry(aEntry);
  if (newRootEntry) {
    
    

    
    
    nsCOMPtr<nsISHEntry> oldRootEntry = GetRootSHEntry(*aPtr);
    if (oldRootEntry) {
      nsCOMPtr<nsIDocShellTreeItem> rootAsItem;
      GetSameTypeRootTreeItem(getter_AddRefs(rootAsItem));
      nsCOMPtr<nsIDocShell> rootShell = do_QueryInterface(rootAsItem);
      if (rootShell) { 
        SwapEntriesData data = { this, newRootEntry };
        nsIDocShell* rootIDocShell = static_cast<nsIDocShell*>(rootShell);
        nsDocShell* rootDocShell = static_cast<nsDocShell*>(rootIDocShell);

#ifdef DEBUG
        nsresult rv =
#endif
        SetChildHistoryEntry(oldRootEntry, rootDocShell, 0, &data);
        NS_ASSERTION(NS_SUCCEEDED(rv), "SetChildHistoryEntry failed");
      }
    }
  }

  *aPtr = aEntry;
}

nsresult
nsDocShell::GetRootSessionHistory(nsISHistory** aReturn)
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
nsDocShell::GetHttpChannel(nsIChannel* aChannel, nsIHttpChannel** aReturn)
{
  NS_ENSURE_ARG_POINTER(aReturn);
  if (!aChannel) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIMultiPartChannel> multiPartChannel(do_QueryInterface(aChannel));
  if (multiPartChannel) {
    nsCOMPtr<nsIChannel> baseChannel;
    multiPartChannel->GetBaseChannel(getter_AddRefs(baseChannel));
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(baseChannel));
    *aReturn = httpChannel;
    NS_IF_ADDREF(*aReturn);
  }
  return NS_OK;
}

bool
nsDocShell::ShouldDiscardLayoutState(nsIHttpChannel* aChannel)
{
  
  if (!aChannel) {
    return false;
  }

  
  nsCOMPtr<nsISupports> securityInfo;
  bool noStore = false, noCache = false;
  aChannel->GetSecurityInfo(getter_AddRefs(securityInfo));
  aChannel->IsNoStoreResponse(&noStore);
  aChannel->IsNoCacheResponse(&noCache);

  return (noStore || (noCache && securityInfo));
}

NS_IMETHODIMP
nsDocShell::GetEditor(nsIEditor** aEditor)
{
  NS_ENSURE_ARG_POINTER(aEditor);

  if (!mEditorData) {
    *aEditor = nullptr;
    return NS_OK;
  }

  return mEditorData->GetEditor(aEditor);
}

NS_IMETHODIMP
nsDocShell::SetEditor(nsIEditor* aEditor)
{
  nsresult rv = EnsureEditorData();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return mEditorData->SetEditor(aEditor);
}

NS_IMETHODIMP
nsDocShell::GetEditable(bool* aEditable)
{
  NS_ENSURE_ARG_POINTER(aEditable);
  *aEditable = mEditorData && mEditorData->GetEditable();
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetHasEditingSession(bool* aHasEditingSession)
{
  NS_ENSURE_ARG_POINTER(aHasEditingSession);

  if (mEditorData) {
    nsCOMPtr<nsIEditingSession> editingSession;
    mEditorData->GetEditingSession(getter_AddRefs(editingSession));
    *aHasEditingSession = (editingSession.get() != nullptr);
  } else {
    *aHasEditingSession = false;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::MakeEditable(bool aInWaitForUriLoad)
{
  nsresult rv = EnsureEditorData();
  if (NS_FAILED(rv)) {
    return rv;
  }

  return mEditorData->MakeEditable(aInWaitForUriLoad);
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
  return method.EqualsLiteral("POST");
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
    reinterpret_cast<void**>(aURI));

  if (NS_FAILED(rv)) {
    
    
    
    (void)NS_GetReferrerFromChannel(aChannel, aURI);
  } else {
    rv = props->GetPropertyAsUint32(NS_LITERAL_STRING("docshell.previousFlags"),
                                    aChannelRedirectFlags);

    NS_WARN_IF_FALSE(
      NS_SUCCEEDED(rv),
      "Could not fetch previous flags, URI will be treated like referrer");
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
    } else if (aChannelRedirectFlags & nsIChannelEventSink::REDIRECT_PERMANENT) {
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
  } else if (mGlobalHistory) {
    
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
nsDocShell::GetLoadType(uint32_t* aLoadType)
{
  *aLoadType = mLoadType;
  return NS_OK;
}

nsresult
nsDocShell::ConfirmRepost(bool* aRepost)
{
  nsCOMPtr<nsIPrompt> prompter;
  CallGetInterface(this, static_cast<nsIPrompt**>(getter_AddRefs(prompter)));
  if (!prompter) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  if (!stringBundleService) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIStringBundle> appBundle;
  nsresult rv = stringBundleService->CreateBundle(kAppstringsBundleURL,
                                                  getter_AddRefs(appBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringBundle> brandBundle;
  rv = stringBundleService->CreateBundle(kBrandBundleURL,
                                         getter_AddRefs(brandBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(prompter && brandBundle && appBundle,
               "Unable to set up repost prompter.");

  nsXPIDLString brandName;
  rv = brandBundle->GetStringFromName(MOZ_UTF16("brandShortName"),
                                      getter_Copies(brandName));

  nsXPIDLString msgString, button0Title;
  if (NS_FAILED(rv)) { 
    rv = appBundle->GetStringFromName(MOZ_UTF16("confirmRepostPrompt"),
                                      getter_Copies(msgString));
  } else {
    
    
    
    const char16_t* formatStrings[] = { brandName.get() };
    rv = appBundle->FormatStringFromName(MOZ_UTF16("confirmRepostPrompt"),
                                         formatStrings,
                                         ArrayLength(formatStrings),
                                         getter_Copies(msgString));
  }
  if (NS_FAILED(rv)) {
    return rv;
  }

  rv = appBundle->GetStringFromName(MOZ_UTF16("resendButton.label"),
                                    getter_Copies(button0Title));
  if (NS_FAILED(rv)) {
    return rv;
  }

  int32_t buttonPressed;
  
  
  bool checkState = false;
  rv = prompter->ConfirmEx(
    nullptr, msgString.get(),
    (nsIPrompt::BUTTON_POS_0 * nsIPrompt::BUTTON_TITLE_IS_STRING) +
      (nsIPrompt::BUTTON_POS_1 * nsIPrompt::BUTTON_TITLE_CANCEL),
    button0Title.get(), nullptr, nullptr, nullptr, &checkState, &buttonPressed);
  if (NS_FAILED(rv)) {
    return rv;
  }

  *aRepost = (buttonPressed == 0);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetPromptAndStringBundle(nsIPrompt** aPrompt,
                                     nsIStringBundle** aStringBundle)
{
  NS_ENSURE_SUCCESS(GetInterface(NS_GET_IID(nsIPrompt), (void**)aPrompt),
                    NS_ERROR_FAILURE);

  nsCOMPtr<nsIStringBundleService> stringBundleService =
    mozilla::services::GetStringBundleService();
  NS_ENSURE_TRUE(stringBundleService, NS_ERROR_FAILURE);

  NS_ENSURE_SUCCESS(
    stringBundleService->CreateBundle(kAppstringsBundleURL, aStringBundle),
    NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetChildOffset(nsIDOMNode* aChild, nsIDOMNode* aParent,
                           int32_t* aOffset)
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

nsIScrollableFrame*
nsDocShell::GetRootScrollFrame()
{
  nsCOMPtr<nsIPresShell> shell = GetPresShell();
  NS_ENSURE_TRUE(shell, nullptr);

  return shell->GetRootScrollFrameAsScrollableExternal();
}

NS_IMETHODIMP
nsDocShell::EnsureScriptEnvironment()
{
  if (mScriptGlobal) {
    return NS_OK;
  }

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
    (mItemType == typeContent) &&
    (chromeFlags & nsIWebBrowserChrome::CHROME_MODAL_CONTENT_WINDOW);
  
  
  
  if (isModalContentWindow) {
    nsCOMPtr<nsIDocShellTreeItem> primaryItem;
    nsresult rv =
      mTreeOwner->GetPrimaryContentShell(getter_AddRefs(primaryItem));
    NS_ENSURE_SUCCESS(rv, rv);
    isModalContentWindow = (primaryItem == this);
  }

  
  
  mScriptGlobal =
    NS_NewScriptGlobalObject(mItemType == typeChrome, isModalContentWindow);
  MOZ_ASSERT(mScriptGlobal);

  mScriptGlobal->SetDocShell(this);

  
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
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::EnsureFind()
{
  nsresult rv;
  if (!mFind) {
    mFind = do_CreateInstance("@mozilla.org/embedcomp/find;1", &rv);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  
  
  

  nsIScriptGlobalObject* scriptGO = GetScriptGlobalObject();
  NS_ENSURE_TRUE(scriptGO, NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsPIDOMWindow> ourWindow = do_QueryInterface(scriptGO);
  nsCOMPtr<nsPIDOMWindow> windowToSearch;
  nsFocusManager::GetFocusedDescendant(ourWindow, true,
                                       getter_AddRefs(windowToSearch));

  nsCOMPtr<nsIWebBrowserFindInFrames> findInFrames = do_QueryInterface(mFind);
  if (!findInFrames) {
    return NS_ERROR_NO_INTERFACE;
  }

  rv = findInFrames->SetRootSearchFrame(ourWindow);
  if (NS_FAILED(rv)) {
    return rv;
  }
  rv = findInFrames->SetCurrentSearchFrame(windowToSearch);
  if (NS_FAILED(rv)) {
    return rv;
  }

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
nsDocShell::IsBeingDestroyed(bool* aDoomed)
{
  NS_ENSURE_ARG(aDoomed);
  *aDoomed = mIsBeingDestroyed;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsExecutingOnLoadHandler(bool* aResult)
{
  NS_ENSURE_ARG(aResult);
  *aResult = mIsExecutingOnLoadHandler;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetLayoutHistoryState(nsILayoutHistoryState** aLayoutHistoryState)
{
  if (mOSHE) {
    mOSHE->GetLayoutHistoryState(aLayoutHistoryState);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::SetLayoutHistoryState(nsILayoutHistoryState* aLayoutHistoryState)
{
  if (mOSHE) {
    mOSHE->SetLayoutHistoryState(aLayoutHistoryState);
  }
  return NS_OK;
}





nsRefreshTimer::nsRefreshTimer()
  : mDelay(0), mRepeat(false), mMetaRefresh(false)
{
}

nsRefreshTimer::~nsRefreshTimer()
{
}





NS_IMPL_ADDREF(nsRefreshTimer)
NS_IMPL_RELEASE(nsRefreshTimer)

NS_INTERFACE_MAP_BEGIN(nsRefreshTimer)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
NS_INTERFACE_MAP_END_THREADSAFE




NS_IMETHODIMP
nsRefreshTimer::Notify(nsITimer* aTimer)
{
  NS_ASSERTION(mDocShell, "DocShell is somehow null");

  if (mDocShell && aTimer) {
    
    uint32_t delay = 0;
    aTimer->GetDelay(&delay);
    mDocShell->ForceRefreshURIFromTimer(mURI, delay, mMetaRefresh, aTimer);
  }
  return NS_OK;
}




nsDocShell::InterfaceRequestorProxy::InterfaceRequestorProxy(
    nsIInterfaceRequestor* aRequestor)
{
  if (aRequestor) {
    mWeakPtr = do_GetWeakReference(aRequestor);
  }
}

nsDocShell::InterfaceRequestorProxy::~InterfaceRequestorProxy()
{
  mWeakPtr = nullptr;
}

NS_IMPL_ISUPPORTS(nsDocShell::InterfaceRequestorProxy, nsIInterfaceRequestor)

NS_IMETHODIMP
nsDocShell::InterfaceRequestorProxy::GetInterface(const nsIID& aIID,
                                                  void** aSink)
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
nsDocShell::SetBaseUrlForWyciwyg(nsIContentViewer* aContentViewer)
{
  if (!aContentViewer) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIURI> baseURI;
  nsresult rv = NS_ERROR_NOT_AVAILABLE;

  if (sURIFixup)
    rv = sURIFixup->CreateExposableURI(mCurrentURI, getter_AddRefs(baseURI));

  
  if (baseURI) {
    nsIDocument* document = aContentViewer->GetDocument();
    if (document) {
      rv = document->SetBaseURI(baseURI);
    }
  }
  return rv;
}





NS_IMETHODIMP
nsDocShell::GetAuthPrompt(uint32_t aPromptReason, const nsIID& aIID,
                          void** aResult)
{
  
  bool priorityPrompt = (aPromptReason == PROMPT_PROXY);

  if (!mAllowAuth && !priorityPrompt) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsresult rv;
  nsCOMPtr<nsIPromptFactory> wwatch =
    do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EnsureScriptEnvironment();
  NS_ENSURE_SUCCESS(rv, rv);

  
  

  return wwatch->GetPrompt(mScriptGlobal, aIID,
                           reinterpret_cast<void**>(aResult));
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
  nsCOMPtr<nsIDOMWindow> win = GetWindow();
  if (win) {
    win->GetTop(aWindow);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetTopFrameElement(nsIDOMElement** aElement)
{
  *aElement = nullptr;
  nsCOMPtr<nsIDOMWindow> win = GetWindow();
  if (!win) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindow> top;
  win->GetScriptableTop(getter_AddRefs(top));
  NS_ENSURE_TRUE(top, NS_ERROR_FAILURE);

  nsCOMPtr<nsPIDOMWindow> piTop = do_QueryInterface(top);
  NS_ENSURE_TRUE(piTop, NS_ERROR_FAILURE);

  
  
  
  nsCOMPtr<nsIDOMElement> elt =
    do_QueryInterface(piTop->GetFrameElementInternal());
  elt.forget(aElement);
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetNestedFrameId(uint64_t* aId)
{
  *aId = 0;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::IsAppOfType(uint32_t aAppType, bool* aIsOfType)
{
  nsRefPtr<nsDocShell> shell = this;
  while (shell) {
    uint32_t type;
    shell->GetAppType(&type);
    if (type == aAppType) {
      *aIsOfType = true;
      return NS_OK;
    }
    shell = shell->GetParentDocshell();
  }

  *aIsOfType = false;
  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetIsContent(bool* aIsContent)
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
  return secMan &&
         NS_SUCCEEDED(secMan->CheckSameOriginURI(aURI, mLoadingURI, false));
}




nsresult
nsDocShell::GetControllerForCommand(const char* aCommand,
                                    nsIController** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nullptr;

  NS_ENSURE_TRUE(mScriptGlobal, NS_ERROR_FAILURE);

  nsCOMPtr<nsPIWindowRoot> root = mScriptGlobal->GetTopWindowRoot();
  NS_ENSURE_TRUE(root, NS_ERROR_FAILURE);

  return root->GetControllerForCommand(aCommand, aResult);
}

NS_IMETHODIMP
nsDocShell::IsCommandEnabled(const char* aCommand, bool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = false;

  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand(aCommand, getter_AddRefs(controller));
  if (controller) {
    rv = controller->IsCommandEnabled(aCommand, aResult);
  }

  return rv;
}

NS_IMETHODIMP
nsDocShell::DoCommand(const char* aCommand)
{
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIController> controller;
  rv = GetControllerForCommand(aCommand, getter_AddRefs(controller));
  if (controller) {
    rv = controller->DoCommand(aCommand);
  }

  return rv;
}

nsresult
nsDocShell::EnsureCommandHandler()
{
  if (!mCommandManager) {
    nsCOMPtr<nsPICommandUpdater> commandUpdater =
      do_CreateInstance("@mozilla.org/embedcomp/command-manager;1");
    if (!commandUpdater) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIDOMWindow> domWindow = GetWindow();
    nsresult rv = commandUpdater->Init(domWindow);
    if (NS_SUCCEEDED(rv)) {
      mCommandManager = do_QueryInterface(commandUpdater);
    }
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
  return IsCommandEnabled("cmd_copyImageLocation", aResult);
}

NS_IMETHODIMP
nsDocShell::CanCopyImageContents(bool* aResult)
{
  return IsCommandEnabled("cmd_copyImageContents", aResult);
}

NS_IMETHODIMP
nsDocShell::CanPaste(bool* aResult)
{
  return IsCommandEnabled("cmd_paste", aResult);
}

NS_IMETHODIMP
nsDocShell::CutSelection(void)
{
  return DoCommand("cmd_cut");
}

NS_IMETHODIMP
nsDocShell::CopySelection(void)
{
  return DoCommand("cmd_copy");
}

NS_IMETHODIMP
nsDocShell::CopyLinkLocation(void)
{
  return DoCommand("cmd_copyLink");
}

NS_IMETHODIMP
nsDocShell::CopyImageLocation(void)
{
  return DoCommand("cmd_copyImageLocation");
}

NS_IMETHODIMP
nsDocShell::CopyImageContents(void)
{
  return DoCommand("cmd_copyImageContents");
}

NS_IMETHODIMP
nsDocShell::Paste(void)
{
  return DoCommand("cmd_paste");
}

NS_IMETHODIMP
nsDocShell::SelectAll(void)
{
  return DoCommand("cmd_selectAll");
}







NS_IMETHODIMP
nsDocShell::SelectNone(void)
{
  return DoCommand("cmd_selectNone");
}





class OnLinkClickEvent : public nsRunnable
{
public:
  OnLinkClickEvent(nsDocShell* aHandler, nsIContent* aContent,
                   nsIURI* aURI,
                   const char16_t* aTargetSpec,
                   const nsAString& aFileName,
                   nsIInputStream* aPostDataStream,
                   nsIInputStream* aHeadersDataStream,
                   bool aIsTrusted);

  NS_IMETHOD Run()
  {
    nsAutoPopupStatePusher popupStatePusher(mPopupState);

    AutoJSAPI jsapi;
    if (mIsTrusted || jsapi.Init(mContent->OwnerDoc()->GetScopeObject())) {
      mHandler->OnLinkClickSync(mContent, mURI,
                                mTargetSpec.get(), mFileName,
                                mPostDataStream, mHeadersDataStream,
                                nullptr, nullptr);
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsDocShell> mHandler;
  nsCOMPtr<nsIURI> mURI;
  nsString mTargetSpec;
  nsString mFileName;
  nsCOMPtr<nsIInputStream> mPostDataStream;
  nsCOMPtr<nsIInputStream> mHeadersDataStream;
  nsCOMPtr<nsIContent> mContent;
  PopupControlState mPopupState;
  bool mIsTrusted;
};

OnLinkClickEvent::OnLinkClickEvent(nsDocShell* aHandler,
                                   nsIContent* aContent,
                                   nsIURI* aURI,
                                   const char16_t* aTargetSpec,
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
  , mPopupState(mHandler->mScriptGlobal->GetPopupControlState())
  , mIsTrusted(aIsTrusted)
{
}



NS_IMETHODIMP
nsDocShell::OnLinkClick(nsIContent* aContent,
                        nsIURI* aURI,
                        const char16_t* aTargetSpec,
                        const nsAString& aFileName,
                        nsIInputStream* aPostDataStream,
                        nsIInputStream* aHeadersDataStream,
                        bool aIsTrusted)
{
  NS_ASSERTION(NS_IsMainThread(), "wrong thread");

  if (!IsNavigationAllowed() || !IsOKToLoadURI(aURI)) {
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

  if (NS_FAILED(rv)) {
    target = aTargetSpec;
  }

  nsCOMPtr<nsIRunnable> ev =
    new OnLinkClickEvent(this, aContent, aURI, target.get(), aFileName,
                         aPostDataStream, aHeadersDataStream, aIsTrusted);
  return NS_DispatchToCurrentThread(ev);
}

NS_IMETHODIMP
nsDocShell::OnLinkClickSync(nsIContent* aContent,
                            nsIURI* aURI,
                            const char16_t* aTargetSpec,
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

  if (!IsNavigationAllowed() || !IsOKToLoadURI(aURI)) {
    return NS_OK;
  }

  
  
  
  if (aContent->IsHTMLElement(nsGkAtoms::form) &&
      ShouldBlockLoadingForBackButton()) {
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
        nsresult rv =
          extProtService->IsExposedProtocol(scheme.get(), &isExposed);
        if (NS_SUCCEEDED(rv) && !isExposed) {
          return extProtService->LoadURI(aURI, this);
        }
      }
    }
  }

  uint32_t flags = INTERNAL_LOAD_FLAGS_NONE;
  if (IsElementAnchor(aContent)) {
    MOZ_ASSERT(aContent->IsHTMLElement());
    nsAutoString referrer;
    aContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rel, referrer);
    nsWhitespaceTokenizerTemplate<nsContentUtils::IsHTMLWhitespace> tok(referrer);
    while (tok.hasMoreTokens()) {
      if (tok.nextToken().LowerCaseEqualsLiteral("noreferrer")) {
        flags |= INTERNAL_LOAD_FLAGS_DONT_SEND_REFERRER |
                 INTERNAL_LOAD_FLAGS_NO_OPENER;
        break;
      }
    }
  }

  
  
  
  
  
  nsCOMPtr<nsIDocument> refererDoc = aContent->OwnerDoc();
  NS_ENSURE_TRUE(refererDoc, NS_ERROR_UNEXPECTED);

  
  
  
  nsPIDOMWindow* refererInner = refererDoc->GetInnerWindow();
  NS_ENSURE_TRUE(refererInner, NS_ERROR_UNEXPECTED);
  if (!mScriptGlobal ||
      mScriptGlobal->GetCurrentInnerWindow() != refererInner) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIURI> referer = refererDoc->GetDocumentURI();
  uint32_t refererPolicy = refererDoc->GetReferrerPolicy();

  
  

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
                             refererPolicy,             
                             aContent->NodePrincipal(), 
                                                        
                             flags,
                             target.get(),              
                             NS_LossyConvertUTF16toASCII(typeHint).get(),
                             aFileName,                 
                             aPostDataStream,           
                             aHeadersDataStream,        
                             LOAD_LINK,                 
                             nullptr,                   
                             true,                      
                             NullString(),              
                             this,                      
                             nullptr,                   
                             aDocShell,                 
                             aRequest);                 
  if (NS_SUCCEEDED(rv)) {
    DispatchPings(this, aContent, aURI, referer, refererPolicy);
  }
  return rv;
}

NS_IMETHODIMP
nsDocShell::OnOverLink(nsIContent* aContent,
                       nsIURI* aURI,
                       const char16_t* aTargetSpec)
{
  if (aContent->IsEditable()) {
    return NS_OK;
  }

  nsCOMPtr<nsIWebBrowserChrome2> browserChrome2 = do_GetInterface(mTreeOwner);
  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIWebBrowserChrome> browserChrome;
  if (!browserChrome2) {
    browserChrome = do_GetInterface(mTreeOwner);
    if (!browserChrome) {
      return rv;
    }
  }

  nsCOMPtr<nsITextToSubURI> textToSubURI =
    do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsAutoCString charset;
  rv = aURI->GetOriginCharset(charset);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoCString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString uStr;
  rv = textToSubURI->UnEscapeURIForUI(charset, spec, uStr);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::net::PredictorPredict(aURI, mCurrentURI,
                                 nsINetworkPredictor::PREDICT_LINK,
                                 this, nullptr);

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

  if (browserChrome) {
    rv = browserChrome->SetStatus(nsIWebBrowserChrome::STATUS_LINK,
                                  EmptyString().get());
  }
  return rv;
}

bool
nsDocShell::ShouldBlockLoadingForBackButton()
{
  if (!(mLoadType & LOAD_CMD_HISTORY) ||
      EventStateManager::IsHandlingUserInput() ||
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
nsDocShell::ReloadDocument(const char* aCharset, int32_t aSource)
{
  
  nsCOMPtr<nsIContentViewer> cv;
  NS_ENSURE_SUCCESS(GetContentViewer(getter_AddRefs(cv)), NS_ERROR_FAILURE);
  if (cv) {
    int32_t hint;
    cv->GetHintCharacterSetSource(&hint);
    if (aSource > hint) {
      nsCString charset(aCharset);
      cv->SetHintCharacterSet(charset);
      cv->SetHintCharacterSetSource(aSource);
      if (eCharsetReloadRequested != mCharsetReloadState) {
        mCharsetReloadState = eCharsetReloadRequested;
        return Reload(LOAD_FLAGS_CHARSET_CHANGE);
      }
    }
  }
  
  return NS_ERROR_DOCSHELL_REQUEST_REJECTED;
}

NS_IMETHODIMP
nsDocShell::StopDocumentLoad(void)
{
  if (eCharsetReloadRequested != mCharsetReloadState) {
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
    nsCOMPtr<nsIPrincipal> principal = nsNullPrincipal::Create();
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
nsDocShell::GetCanExecuteScripts(bool* aResult)
{
  *aResult = mCanExecuteScripts;
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
nsDocShell::GetAppManifestURL(nsAString& aAppManifestURL)
{
  uint32_t appId;
  GetAppId(&appId);

  if (appId != nsIScriptSecurityManager::NO_APP_ID &&
      appId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService(APPS_SERVICE_CONTRACTID);
    NS_ASSERTION(appsService, "No AppsService available");
    appsService->GetManifestURLByLocalId(appId, aAppManifestURL);
  } else {
    aAppManifestURL.SetLength(0);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsDocShell::GetAsyncPanZoomEnabled(bool* aOut)
{
  *aOut = Preferences::GetBool("layers.async-pan-zoom.enabled", false);
  return NS_OK;
}

bool
nsDocShell::HasUnloadedParent()
{
  nsRefPtr<nsDocShell> parent = GetParentDocshell();
  while (parent) {
    bool inUnload = false;
    parent->GetIsInUnload(&inUnload);
    if (inUnload) {
      return true;
    }
    parent = parent->GetParentDocshell();
  }
  return false;
}

bool
nsDocShell::IsInvisible()
{
  return mInvisible;
}

void
nsDocShell::SetInvisible(bool aInvisible)
{
  mInvisible = aInvisible;
}

void
nsDocShell::SetOpener(nsITabParent* aOpener)
{
  mOpener = do_GetWeakReference(aOpener);
}

nsITabParent*
nsDocShell::GetOpener()
{
  nsCOMPtr<nsITabParent> opener(do_QueryReferent(mOpener));
  return opener;
}

URLSearchParams*
nsDocShell::GetURLSearchParams()
{
  return mURLSearchParams;
}

void
nsDocShell::NotifyJSRunToCompletionStart()
{
  bool timelineOn = nsIDocShell::GetRecordProfileTimelineMarkers();

  
  if (timelineOn && mJSRunToCompletionDepth == 0) {
    AddProfileTimelineMarker("Javascript", TRACING_INTERVAL_START);
  }
  mJSRunToCompletionDepth++;
}

void
nsDocShell::NotifyJSRunToCompletionStop()
{
  bool timelineOn = nsIDocShell::GetRecordProfileTimelineMarkers();

  
  mJSRunToCompletionDepth--;
  if (timelineOn && mJSRunToCompletionDepth == 0) {
    AddProfileTimelineMarker("Javascript", TRACING_INTERVAL_END);
  }
}

void
nsDocShell::MaybeNotifyKeywordSearchLoading(const nsString& aProvider,
                                            const nsString& aKeyword)
{
  if (aProvider.IsEmpty()) {
    return;
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    dom::ContentChild* contentChild = dom::ContentChild::GetSingleton();
    if (contentChild) {
      contentChild->SendNotifyKeywordSearchLoading(aProvider, aKeyword);
    }
    return;
  }

#ifdef MOZ_TOOLKIT_SEARCH
  nsCOMPtr<nsIBrowserSearchService> searchSvc =
    do_GetService("@mozilla.org/browser/search-service;1");
  if (searchSvc) {
    nsCOMPtr<nsISearchEngine> searchEngine;
    searchSvc->GetEngineByName(aProvider, getter_AddRefs(searchEngine));
    if (searchEngine) {
      nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
      if (obsSvc) {
        
        
        obsSvc->NotifyObservers(searchEngine, "keyword-search", aKeyword.get());
      }
    }
  }
#endif
}

NS_IMETHODIMP
nsDocShell::ShouldPrepareForIntercept(nsIURI* aURI, bool aIsNavigate, bool* aShouldIntercept)
{
  *aShouldIntercept = false;
  if (mSandboxFlags) {
    
    return NS_OK;
  }

  nsCOMPtr<nsIServiceWorkerManager> swm = mozilla::services::GetServiceWorkerManager();
  if (!swm) {
    return NS_OK;
  }

  if (aIsNavigate) {
    return swm->IsAvailableForURI(aURI, aShouldIntercept);
  }

  nsCOMPtr<nsIDocument> doc = GetDocument();
  if (!doc) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return swm->IsControlled(doc, aShouldIntercept);
}

NS_IMETHODIMP
nsDocShell::ChannelIntercepted(nsIInterceptedChannel* aChannel)
{
  nsCOMPtr<nsIServiceWorkerManager> swm = mozilla::services::GetServiceWorkerManager();
  if (!swm) {
    aChannel->Cancel();
    return NS_OK;
  }

  bool isNavigation = false;
  nsresult rv = aChannel->GetIsNavigation(&isNavigation);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc;

  if (!isNavigation) {
    doc = GetDocument();
    if (!doc) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  bool isReload = mLoadType & LOAD_CMD_RELOAD;
  return swm->DispatchFetchEvent(doc, aChannel, isReload);
}

NS_IMETHODIMP
nsDocShell::SetPaymentRequestId(const nsAString& aPaymentRequestId)
{
  mPaymentRequestId = aPaymentRequestId;
  return NS_OK;
}

nsString
nsDocShell::GetInheritedPaymentRequestId()
{
  if (!mPaymentRequestId.IsEmpty()) {
    return mPaymentRequestId;
  }

  nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
  GetSameTypeParent(getter_AddRefs(parentAsItem));

  nsCOMPtr<nsIDocShell> parent = do_QueryInterface(parentAsItem);
  if (!parent) {
    return mPaymentRequestId;
  }
  return static_cast<nsDocShell*>(parent.get())->GetInheritedPaymentRequestId();
}

NS_IMETHODIMP
nsDocShell::GetPaymentRequestId(nsAString& aPaymentRequestId)
{
  aPaymentRequestId = GetInheritedPaymentRequestId();
  return NS_OK;
}
