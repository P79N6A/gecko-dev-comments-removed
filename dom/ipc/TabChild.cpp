





#include "base/basictypes.h"

#include "TabChild.h"

#include "BasicLayers.h"
#include "Blob.h"
#include "ContentChild.h"
#include "IndexedDBChild.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/IntentionalCrash.h"
#include "mozilla/docshell/OfflineCacheUpdateChild.h"
#include "mozilla/dom/PContentChild.h"
#include "mozilla/dom/PContentDialogChild.h"
#include "mozilla/ipc/DocumentRendererChild.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/PLayersChild.h"
#include "mozilla/layout/RenderFrameChild.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/unused.h"
#include "mozIApplication.h"
#include "nsComponentManagerUtils.h"
#include "nsComponentManagerUtils.h"
#include "nsContentUtils.h"
#include "nsEmbedCID.h"
#include "nsEventListenerManager.h"
#include <algorithm>
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif
#include "mozilla/dom/Element.h"
#include "nsIAppsService.h"
#include "nsIBaseWindow.h"
#include "nsICachedFileDescriptorListener.h"
#include "nsIComponentManager.h"
#include "nsIDocumentInlines.h"
#include "nsIDOMClassInfo.h"
#include "nsIDOMElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDocShell.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIJSContextStack.h"
#include "nsIJSRuntimeService.h"
#include "nsISSLStatusProvider.h"
#include "nsIScriptContext.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptSecurityManager.h"
#include "nsISecureBrowserUI.h"
#include "nsIServiceManager.h"
#include "nsISupportsImpl.h"
#include "nsIURI.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsView.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserSetup.h"
#include "nsIWebProgress.h"
#include "nsIXPCSecurityManager.h"
#include "nsInterfaceHashtable.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsGlobalWindow.h"
#include "nsPresContext.h"
#include "nsPrintfCString.h"
#include "nsScriptLoader.h"
#include "nsSerializationHelper.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "PCOMContentPermissionRequestChild.h"
#include "PuppetWidget.h"
#include "StructuredCloneUtils.h"
#include "xpcpublic.h"
#include "nsViewportInfo.h"

#define BROWSER_ELEMENT_CHILD_SCRIPT \
    NS_LITERAL_STRING("chrome://global/content/BrowserElementChild.js")

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::dom::ipc;
using namespace mozilla::ipc;
using namespace mozilla::layers;
using namespace mozilla::layout;
using namespace mozilla::docshell;
using namespace mozilla::dom::indexedDB;
using namespace mozilla::widget;

NS_IMPL_ISUPPORTS1(ContentListener, nsIDOMEventListener)

static const nsIntSize kDefaultViewportSize(980, 480);

static const char CANCEL_DEFAULT_PAN_ZOOM[] = "cancel-default-pan-zoom";
static const char BROWSER_ZOOM_TO_RECT[] = "browser-zoom-to-rect";
static const char BEFORE_FIRST_PAINT[] = "before-first-paint";
static const char DETECT_SCROLLABLE_SUBFRAME[] = "detect-scrollable-subframe";

NS_IMETHODIMP
ContentListener::HandleEvent(nsIDOMEvent* aEvent)
{
  RemoteDOMEvent remoteEvent;
  remoteEvent.mEvent = do_QueryInterface(aEvent);
  NS_ENSURE_STATE(remoteEvent.mEvent);
  mTabChild->SendEvent(remoteEvent);
  return NS_OK;
}

class ContentDialogChild : public PContentDialogChild
{
public:
  virtual bool Recv__delete__(const InfallibleTArray<int>& aIntParams,
                              const InfallibleTArray<nsString>& aStringParams);
};

class TabChild::CachedFileDescriptorInfo
{
    struct PathOnlyComparatorHelper
    {
        bool Equals(const nsAutoPtr<CachedFileDescriptorInfo>& a,
                    const CachedFileDescriptorInfo& b) const
        {
            return a->mPath == b.mPath;
        }
    };

    struct PathAndCallbackComparatorHelper
    {
        bool Equals(const nsAutoPtr<CachedFileDescriptorInfo>& a,
                    const CachedFileDescriptorInfo& b) const
        {
            return a->mPath == b.mPath &&
                   a->mCallback == b.mCallback;
        }
    };

public:
    nsString mPath;
    FileDescriptor mFileDescriptor;
    nsCOMPtr<nsICachedFileDescriptorListener> mCallback;
    bool mCanceled;

    CachedFileDescriptorInfo(const nsAString& aPath)
      : mPath(aPath), mCanceled(false)
    { }

    CachedFileDescriptorInfo(const nsAString& aPath,
                             const FileDescriptor& aFileDescriptor)
      : mPath(aPath), mFileDescriptor(aFileDescriptor), mCanceled(false)
    { }

    CachedFileDescriptorInfo(const nsAString& aPath,
                             nsICachedFileDescriptorListener* aCallback)
      : mPath(aPath), mCallback(aCallback), mCanceled(false)
    { }

    PathOnlyComparatorHelper PathOnlyComparator() const
    {
        return PathOnlyComparatorHelper();
    }

    PathAndCallbackComparatorHelper PathAndCallbackComparator() const
    {
        return PathAndCallbackComparatorHelper();
    }

    void FireCallback() const
    {
        mCallback->OnCachedFileDescriptor(mPath, mFileDescriptor);
    }
};

class TabChild::CachedFileDescriptorCallbackRunnable : public nsRunnable
{
    typedef TabChild::CachedFileDescriptorInfo CachedFileDescriptorInfo;

    nsAutoPtr<CachedFileDescriptorInfo> mInfo;

public:
    CachedFileDescriptorCallbackRunnable(CachedFileDescriptorInfo* aInfo)
      : mInfo(aInfo)
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(aInfo);
        MOZ_ASSERT(!aInfo->mPath.IsEmpty());
        MOZ_ASSERT(aInfo->mCallback);
    }

    void Dispatch()
    {
        MOZ_ASSERT(NS_IsMainThread());

        nsresult rv = NS_DispatchToCurrentThread(this);
        NS_ENSURE_SUCCESS_VOID(rv);
    }

private:
    NS_IMETHOD Run()
    {
        MOZ_ASSERT(NS_IsMainThread());
        MOZ_ASSERT(mInfo);

        mInfo->FireCallback();
        return NS_OK;
    }
};

StaticRefPtr<TabChild> sPreallocatedTab;

 void
TabChild::PreloadSlowThings()
{
    MOZ_ASSERT(!sPreallocatedTab);

    nsRefPtr<TabChild> tab(new TabChild(TabContext(),  0));
    if (!NS_SUCCEEDED(tab->Init()) ||
        !tab->InitTabChildGlobal(DONT_LOAD_SCRIPTS)) {
        return;
    }
    
    tab->TryCacheLoadAndCompileScript(BROWSER_ELEMENT_CHILD_SCRIPT);
    
    tab->RecvLoadRemoteScript(
        NS_LITERAL_STRING("chrome://global/content/preload.js"));

    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(tab->mWebNav);
    if (nsIPresShell* presShell = docShell->GetPresShell()) {
        
        
        presShell->Initialize(0, 0);
        nsIDocument* doc = presShell->GetDocument();
        doc->FlushPendingNotifications(Flush_Layout);
        
        
        presShell->MakeZombie();
    }

    sPreallocatedTab = tab;
    ClearOnShutdown(&sPreallocatedTab);
}

 already_AddRefed<TabChild>
TabChild::Create(const TabContext &aContext, uint32_t aChromeFlags)
{
    if (sPreallocatedTab &&
        sPreallocatedTab->mChromeFlags == aChromeFlags &&
        aContext.IsBrowserOrApp()) {

        nsRefPtr<TabChild> child = sPreallocatedTab.get();
        sPreallocatedTab = nullptr;

        MOZ_ASSERT(!child->mTriedBrowserInit);

        child->SetTabContext(aContext);
        child->NotifyTabContextUpdated();
        return child.forget();
    }

    nsRefPtr<TabChild> iframe = new TabChild(aContext, aChromeFlags);
    return NS_SUCCEEDED(iframe->Init()) ? iframe.forget() : nullptr;
}


TabChild::TabChild(const TabContext& aContext, uint32_t aChromeFlags)
  : TabContext(aContext)
  , mRemoteFrame(nullptr)
  , mTabChildGlobal(nullptr)
  , mChromeFlags(aChromeFlags)
  , mOuterRect(0, 0, 0, 0)
  , mInnerSize(0, 0)
  , mActivePointerId(-1)
  , mTapHoldTimer(nullptr)
  , mOldViewportWidth(0.0f)
  , mLastBackgroundColor(NS_RGB(255, 255, 255))
  , mDidFakeShow(false)
  , mNotified(false)
  , mContentDocumentIsDisplayed(false)
  , mTriedBrowserInit(false)
  , mOrientation(eScreenOrientation_PortraitPrimary)
{
    printf("creating %d!\n", NS_IsMainThread());
}

NS_IMETHODIMP
TabChild::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("DOMMetaAdded")) {
    
    
    HandlePossibleViewportChange();
  }

  return NS_OK;
}

NS_IMETHODIMP
TabChild::Observe(nsISupports *aSubject,
                  const char *aTopic,
                  const PRUnichar *aData)
{
  if (!strcmp(aTopic, CANCEL_DEFAULT_PAN_ZOOM)) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
    nsCOMPtr<nsITabChild> tabChild(GetTabChildFrom(docShell));
    if (tabChild == this) {
      mRemoteFrame->CancelDefaultPanZoom();
    }
  } else if (!strcmp(aTopic, BROWSER_ZOOM_TO_RECT)) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
    nsCOMPtr<nsITabChild> tabChild(GetTabChildFrom(docShell));
    if (tabChild == this) {
      gfxRect rect;
      sscanf(NS_ConvertUTF16toUTF8(aData).get(),
             "{\"x\":%lf,\"y\":%lf,\"w\":%lf,\"h\":%lf}",
             &rect.x, &rect.y, &rect.width, &rect.height);
      SendZoomToRect(rect);
    }
  } else if (!strcmp(aTopic, BEFORE_FIRST_PAINT)) {
    if (IsAsyncPanZoomEnabled()) {
      nsCOMPtr<nsIDocument> subject(do_QueryInterface(aSubject));
      nsCOMPtr<nsIDOMDocument> domDoc;
      mWebNav->GetDocument(getter_AddRefs(domDoc));
      nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));

      if (SameCOMIdentity(subject, doc)) {
        nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());

        mContentDocumentIsDisplayed = true;

        
        
        
        SetCSSViewport(kDefaultViewportSize.width, kDefaultViewportSize.height);

        
        
        
        mLastMetrics.mZoom = gfxSize(1.0, 1.0);
        mLastMetrics.mViewport =
            gfx::Rect(0, 0,
                      kDefaultViewportSize.width, kDefaultViewportSize.height);
        mLastMetrics.mCompositionBounds = nsIntRect(nsIntPoint(0, 0),
                                                    mInnerSize);
        mLastMetrics.mResolution =
          AsyncPanZoomController::CalculateResolution(mLastMetrics);
        mLastMetrics.mScrollOffset = gfx::Point(0, 0);
        utils->SetResolution(mLastMetrics.mResolution.width,
                             mLastMetrics.mResolution.height);

        HandlePossibleViewportChange();
      }
    }
  } else if (!strcmp(aTopic, DETECT_SCROLLABLE_SUBFRAME)) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
    nsCOMPtr<nsITabChild> tabChild(GetTabChildFrom(docShell));
    if (tabChild == this) {
      mRemoteFrame->DetectScrollableSubframe();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
TabChild::OnStateChange(nsIWebProgress* aWebProgress,
                        nsIRequest* aRequest,
                        uint32_t aStateFlags,
                        nsresult aStatus)
{
  NS_NOTREACHED("not implemented in TabChild");
  return NS_OK;
}

NS_IMETHODIMP
TabChild::OnProgressChange(nsIWebProgress* aWebProgress,
                           nsIRequest* aRequest,
                           int32_t aCurSelfProgress,
                           int32_t aMaxSelfProgress,
                           int32_t aCurTotalProgress,
                           int32_t aMaxTotalProgress)
{
  NS_NOTREACHED("not implemented in TabChild");
  return NS_OK;
}

NS_IMETHODIMP
TabChild::OnLocationChange(nsIWebProgress* aWebProgress,
                           nsIRequest* aRequest,
                           nsIURI *aLocation,
                           uint32_t aFlags)
{
  if (!IsAsyncPanZoomEnabled()) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindow> window;
  aWebProgress->GetDOMWindow(getter_AddRefs(window));
  if (!window) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMWindowUtils> utils(do_GetInterface(window));
  utils->SetIsFirstPaint(true);

  nsCOMPtr<nsIDOMDocument> progressDoc;
  window->GetDocument(getter_AddRefs(progressDoc));
  if (!progressDoc) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  mWebNav->GetDocument(getter_AddRefs(domDoc));
  if (!domDoc || !SameCOMIdentity(domDoc, progressDoc)) {
    return NS_OK;
  }

  nsCOMPtr<nsIURIFixup> urifixup(do_GetService(NS_URIFIXUP_CONTRACTID));
  if (!urifixup) {
    return NS_OK;
  }

  nsCOMPtr<nsIURI> exposableURI;
  urifixup->CreateExposableURI(aLocation, getter_AddRefs(exposableURI));
  if (!exposableURI) {
    return NS_OK;
  }

  if (!(aFlags & nsIWebProgressListener::LOCATION_CHANGE_SAME_DOCUMENT)) {
    mContentDocumentIsDisplayed = false;
  } else if (mLastURI != nullptr) {
    bool exposableEqualsLast, exposableEqualsNew;
    exposableURI->Equals(mLastURI.get(), &exposableEqualsLast);
    exposableURI->Equals(aLocation, &exposableEqualsNew);
    if (exposableEqualsLast && !exposableEqualsNew) {
      mContentDocumentIsDisplayed = false;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
TabChild::OnStatusChange(nsIWebProgress* aWebProgress,
                         nsIRequest* aRequest,
                         nsresult aStatus,
                         const PRUnichar* aMessage)
{
  NS_NOTREACHED("not implemented in TabChild");
  return NS_OK;
}

NS_IMETHODIMP
TabChild::OnSecurityChange(nsIWebProgress* aWebProgress,
                           nsIRequest* aRequest,
                           uint32_t aState)
{
  NS_NOTREACHED("not implemented in TabChild");
  return NS_OK;
}

void
TabChild::SetCSSViewport(float aWidth, float aHeight)
{
  mOldViewportWidth = aWidth;

  if (mContentDocumentIsDisplayed) {
    nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
    utils->SetCSSViewport(aWidth, aHeight);
  }
}

void
TabChild::HandlePossibleViewportChange()
{
  if (!IsAsyncPanZoomEnabled()) {
    return;
  }

  nsCOMPtr<nsIDOMDocument> domDoc;
  mWebNav->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDoc));

  nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());

  nsViewportInfo viewportInfo =
    nsContentUtils::GetViewportInfo(document, mInnerSize.width, mInnerSize.height);
  SendUpdateZoomConstraints(viewportInfo.IsZoomAllowed(),
                            viewportInfo.GetMinZoom(),
                            viewportInfo.GetMaxZoom());

  float screenW = mInnerSize.width;
  float screenH = mInnerSize.height;
  float viewportW = viewportInfo.GetWidth();
  float viewportH = viewportInfo.GetHeight();

  
  
  if (!screenW || !screenH) {
    return;
  }

  
  
  
  
  
  float oldBrowserWidth = mOldViewportWidth;
  mLastMetrics.mViewport.width = viewportW;
  mLastMetrics.mViewport.height = viewportH;
  if (!oldBrowserWidth) {
    oldBrowserWidth = kDefaultViewportSize.width;
  }
  SetCSSViewport(viewportW, viewportH);

  
  
  
  
  
  
  
  
  
  if (!mContentDocumentIsDisplayed) {
    return;
  }

  float minScale = 1.0f;

  nsCOMPtr<nsIDOMElement> htmlDOMElement = do_QueryInterface(document->GetHtmlElement());
  HTMLBodyElement* bodyDOMElement = document->GetBodyElement();

  int32_t htmlWidth = 0, htmlHeight = 0;
  if (htmlDOMElement) {
    htmlDOMElement->GetScrollWidth(&htmlWidth);
    htmlDOMElement->GetScrollHeight(&htmlHeight);
  }
  int32_t bodyWidth = 0, bodyHeight = 0;
  if (bodyDOMElement) {
    bodyWidth = bodyDOMElement->ScrollWidth();
    bodyHeight = bodyDOMElement->ScrollHeight();
  }

  float pageWidth, pageHeight;
  if (htmlDOMElement || bodyDOMElement) {
    pageWidth = std::max(htmlWidth, bodyWidth);
    pageHeight = std::max(htmlHeight, bodyHeight);
  } else {
    
    pageWidth = viewportW;
    pageHeight = viewportH;
  }
  NS_ENSURE_TRUE_VOID(pageWidth); 

  minScale = mInnerSize.width / pageWidth;
  minScale = clamped((double)minScale, viewportInfo.GetMinZoom(),
                     viewportInfo.GetMaxZoom());
  NS_ENSURE_TRUE_VOID(minScale); 

  viewportH = std::max(viewportH, screenH / minScale);
  SetCSSViewport(viewportW, viewportH);

  
  
  
  
  
  
  
  
  
  
  
  
  int32_t oldScreenWidth = mLastMetrics.mCompositionBounds.width;
  if (!oldScreenWidth) {
    oldScreenWidth = mInnerSize.width;
  }

  FrameMetrics metrics(mLastMetrics);
  metrics.mViewport = gfx::Rect(0.0f, 0.0f, viewportW, viewportH);
  metrics.mScrollableRect = gfx::Rect(0.0f, 0.0f, pageWidth, pageHeight);
  metrics.mCompositionBounds = nsIntRect(0, 0, mInnerSize.width, mInnerSize.height);

  
  
  bool isFirstPaint;
  nsresult rv = utils->GetIsFirstPaint(&isFirstPaint);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  if (NS_FAILED(rv) || isFirstPaint) {
    gfxSize intrinsicScale =
        AsyncPanZoomController::CalculateIntrinsicScale(metrics);
    
    
    
    if (viewportInfo.GetDefaultZoom() < 0.01f) {
      viewportInfo.SetDefaultZoom(intrinsicScale.width);
    }

    double defaultZoom = viewportInfo.GetDefaultZoom();
    MOZ_ASSERT(viewportInfo.GetMinZoom() <= defaultZoom &&
               defaultZoom <= viewportInfo.GetMaxZoom());
    
    
    metrics.mZoom = gfxSize(defaultZoom / intrinsicScale.width,
                            defaultZoom / intrinsicScale.height);
  }

  metrics.mDisplayPort = AsyncPanZoomController::CalculatePendingDisplayPort(
    
    
    
    metrics, gfx::Point(0.0f, 0.0f), gfx::Point(0.0f, 0.0f), 0.0);
  gfxSize resolution = AsyncPanZoomController::CalculateResolution(metrics);
  
  
  gfxFloat hysteresis =
    gfxFloat(oldBrowserWidth) / gfxFloat(oldScreenWidth);
  resolution.width *= hysteresis;
  resolution.height *= hysteresis;
  metrics.mResolution = resolution;
  utils->SetResolution(metrics.mResolution.width, metrics.mResolution.height);

  
  
  RecvUpdateFrame(metrics);
}

nsresult
TabChild::Init()
{
  nsCOMPtr<nsIWebBrowser> webBrowser = do_CreateInstance(NS_WEBBROWSER_CONTRACTID);
  if (!webBrowser) {
    NS_ERROR("Couldn't create a nsWebBrowser?");
    return NS_ERROR_FAILURE;
  }

  webBrowser->SetContainerWindow(this);
  mWebNav = do_QueryInterface(webBrowser);
  NS_ASSERTION(mWebNav, "nsWebBrowser doesn't implement nsIWebNavigation?");

  nsCOMPtr<nsIDocShellTreeItem> docShellItem(do_QueryInterface(mWebNav));
  docShellItem->SetItemType(nsIDocShellTreeItem::typeContentWrapper);
  
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebNav);
  if (!baseWindow) {
    NS_ERROR("mWebNav doesn't QI to nsIBaseWindow");
    return NS_ERROR_FAILURE;
  }

  mWidget = nsIWidget::CreatePuppetWidget(this);
  if (!mWidget) {
    NS_ERROR("couldn't create fake widget");
    return NS_ERROR_FAILURE;
  }
  mWidget->Create(
    nullptr, 0,              
    nsIntRect(nsIntPoint(0, 0), nsIntSize(0, 0)),
    nullptr,                 
    nullptr                  
  );

  baseWindow->InitWindow(0, mWidget, 0, 0, 0, 0);
  baseWindow->Create();

  NotifyTabContextUpdated();

  
  
  nsCOMPtr<nsIWebBrowserSetup> webBrowserSetup =
    do_QueryInterface(baseWindow);
  if (webBrowserSetup) {
    webBrowserSetup->SetProperty(nsIWebBrowserSetup::SETUP_ALLOW_DNS_PREFETCH,
                                 true);
  } else {
    NS_WARNING("baseWindow doesn't QI to nsIWebBrowserSetup, skipping "
               "DNS prefetching enable step.");
  }

  nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebNav);
  MOZ_ASSERT(docShell);
  nsCOMPtr<nsIWebProgress> webProgress = do_GetInterface(docShell);
  NS_ENSURE_TRUE(webProgress, NS_ERROR_FAILURE);
  webProgress->AddProgressListener(this, nsIWebProgress::NOTIFY_LOCATION);

  return NS_OK;
}

void
TabChild::NotifyTabContextUpdated()
{
    nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mWebNav);
    MOZ_ASSERT(docShell);

    if (docShell) {
        
        
        if (IsBrowserElement()) {
          docShell->SetIsBrowserInsideApp(BrowserOwnerAppId());
        } else {
          docShell->SetIsApp(OwnAppId());
        }
    }
}

NS_INTERFACE_MAP_BEGIN(TabChild)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIWebBrowserChrome)
  NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome)
  NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChrome2)
  NS_INTERFACE_MAP_ENTRY(nsIEmbeddingSiteWindow)
  NS_INTERFACE_MAP_ENTRY(nsIWebBrowserChromeFocus)
  NS_INTERFACE_MAP_ENTRY(nsIInterfaceRequestor)
  NS_INTERFACE_MAP_ENTRY(nsIWindowProvider)
  NS_INTERFACE_MAP_ENTRY(nsIDOMEventListener)
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
  NS_INTERFACE_MAP_ENTRY(nsITabChild)
  NS_INTERFACE_MAP_ENTRY(nsIDialogCreator)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
  NS_INTERFACE_MAP_ENTRY(nsSupportsWeakReference)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(TabChild)
NS_IMPL_RELEASE(TabChild)

NS_IMETHODIMP
TabChild::SetStatus(uint32_t aStatusType, const PRUnichar* aStatus)
{
  
  return NS_OK;
}

NS_IMETHODIMP
TabChild::GetWebBrowser(nsIWebBrowser** aWebBrowser)
{
  NS_NOTREACHED("TabChild::GetWebBrowser not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::SetWebBrowser(nsIWebBrowser* aWebBrowser)
{
  NS_NOTREACHED("TabChild::SetWebBrowser not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::GetChromeFlags(uint32_t* aChromeFlags)
{
  *aChromeFlags = mChromeFlags;
  return NS_OK;
}

NS_IMETHODIMP
TabChild::SetChromeFlags(uint32_t aChromeFlags)
{
  NS_NOTREACHED("trying to SetChromeFlags from content process?");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::DestroyBrowserWindow()
{
  NS_NOTREACHED("TabChild::SetWebBrowser not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::SizeBrowserTo(int32_t aCX, int32_t aCY)
{
  NS_NOTREACHED("TabChild::SizeBrowserTo not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::ShowAsModal()
{
  NS_NOTREACHED("TabChild::ShowAsModal not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::IsWindowModal(bool* aRetVal)
{
  *aRetVal = false;
  return NS_OK;
}

NS_IMETHODIMP
TabChild::ExitModalEventLoop(nsresult aStatus)
{
  NS_NOTREACHED("TabChild::ExitModalEventLoop not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::SetStatusWithContext(uint32_t aStatusType,
                                    const nsAString& aStatusText,
                                    nsISupports* aStatusContext)
{
  
  return NS_OK;
}

NS_IMETHODIMP
TabChild::SetDimensions(uint32_t aFlags, int32_t aX, int32_t aY,
                             int32_t aCx, int32_t aCy)
{
  NS_NOTREACHED("TabChild::SetDimensions not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::GetDimensions(uint32_t aFlags, int32_t* aX,
                             int32_t* aY, int32_t* aCx, int32_t* aCy)
{
  if (aX) {
    *aX = mOuterRect.x;
  }
  if (aY) {
    *aY = mOuterRect.y;
  }
  if (aCx) {
    *aCx = mOuterRect.width;
  }
  if (aCy) {
    *aCy = mOuterRect.height;
  }

  return NS_OK;
}

NS_IMETHODIMP
TabChild::SetFocus()
{
  NS_NOTREACHED("TabChild::SetFocus not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::GetVisibility(bool* aVisibility)
{
  *aVisibility = true;
  return NS_OK;
}

NS_IMETHODIMP
TabChild::SetVisibility(bool aVisibility)
{
  
  return NS_OK;
}

NS_IMETHODIMP
TabChild::GetTitle(PRUnichar** aTitle)
{
  NS_NOTREACHED("TabChild::GetTitle not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::SetTitle(const PRUnichar* aTitle)
{
  
  return NS_OK;
}

NS_IMETHODIMP
TabChild::GetSiteWindow(void** aSiteWindow)
{
  NS_NOTREACHED("TabChild::GetSiteWindow not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::Blur()
{
  NS_NOTREACHED("TabChild::Blur not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::FocusNextElement()
{
  SendMoveFocus(true);
  return NS_OK;
}

NS_IMETHODIMP
TabChild::FocusPrevElement()
{
  SendMoveFocus(false);
  return NS_OK;
}

NS_IMETHODIMP
TabChild::GetInterface(const nsIID & aIID, void **aSink)
{
    
    
    return QueryInterface(aIID, aSink);
}

NS_IMETHODIMP
TabChild::ProvideWindow(nsIDOMWindow* aParent, uint32_t aChromeFlags,
                        bool aCalledFromJS,
                        bool aPositionSpecified, bool aSizeSpecified,
                        nsIURI* aURI, const nsAString& aName,
                        const nsACString& aFeatures, bool* aWindowIsNew,
                        nsIDOMWindow** aReturn)
{
    *aReturn = nullptr;

    
    
    
    nsCOMPtr<nsIDocShell> docshell = do_GetInterface(aParent);
    if (docshell && docshell->GetIsInBrowserOrApp() &&
        !(aChromeFlags & (nsIWebBrowserChrome::CHROME_MODAL |
                          nsIWebBrowserChrome::CHROME_OPENAS_DIALOG |
                          nsIWebBrowserChrome::CHROME_OPENAS_CHROME))) {

      
      
      
      return BrowserFrameProvideWindow(aParent, aURI, aName, aFeatures,
                                       aWindowIsNew, aReturn);
    }

    
    PBrowserChild* newChild;
    if (!CallCreateWindow(&newChild)) {
        return NS_ERROR_NOT_AVAILABLE;
    }

    *aWindowIsNew = true;
    nsCOMPtr<nsIDOMWindow> win =
        do_GetInterface(static_cast<TabChild*>(newChild)->mWebNav);
    win.forget(aReturn);
    return NS_OK;
}

nsresult
TabChild::BrowserFrameProvideWindow(nsIDOMWindow* aOpener,
                                    nsIURI* aURI,
                                    const nsAString& aName,
                                    const nsACString& aFeatures,
                                    bool* aWindowIsNew,
                                    nsIDOMWindow** aReturn)
{
  *aReturn = nullptr;

  nsRefPtr<TabChild> newChild =
      new TabChild( *this,  0);
  if (!NS_SUCCEEDED(newChild->Init())) {
      return NS_ERROR_ABORT;
  }

  
  
  
  
  PopupIPCTabContext context;
  context.openerChild() = this;
  context.isBrowserElement() = IsBrowserElement();

  unused << Manager()->SendPBrowserConstructor(
      
      nsRefPtr<TabChild>(newChild).forget().get(),
      IPCTabContext(context, mScrolling),  0);

  nsAutoCString spec;
  if (aURI) {
    aURI->GetSpec(spec);
  }

  NS_ConvertUTF8toUTF16 url(spec);
  nsString name(aName);
  NS_ConvertUTF8toUTF16 features(aFeatures);
  newChild->SendBrowserFrameOpenWindow(this, url, name,
                                       features, aWindowIsNew);
  if (!*aWindowIsNew) {
    PBrowserChild::Send__delete__(newChild);
    return NS_ERROR_ABORT;
  }

  
  
  newChild->DoFakeShow();

  nsCOMPtr<nsIDOMWindow> win = do_GetInterface(newChild->mWebNav);
  win.forget(aReturn);
  return NS_OK;
}

static nsInterfaceHashtable<nsPtrHashKey<PContentDialogChild>, nsIDialogParamBlock> gActiveDialogs;

NS_IMETHODIMP
TabChild::OpenDialog(uint32_t aType, const nsACString& aName,
                     const nsACString& aFeatures,
                     nsIDialogParamBlock* aArguments,
                     nsIDOMElement* aFrameElement)
{
  if (!gActiveDialogs.IsInitialized()) {
    gActiveDialogs.Init();
  }
  InfallibleTArray<int32_t> intParams;
  InfallibleTArray<nsString> stringParams;
  ParamsToArrays(aArguments, intParams, stringParams);
  PContentDialogChild* dialog =
    SendPContentDialogConstructor(aType, nsCString(aName),
                                  nsCString(aFeatures), intParams, stringParams);
  gActiveDialogs.Put(dialog, aArguments);
  nsIThread *thread = NS_GetCurrentThread();
  while (gActiveDialogs.GetWeak(dialog)) {
    if (!NS_ProcessNextEvent(thread)) {
      break;
    }
  }
  return NS_OK;
}

bool
ContentDialogChild::Recv__delete__(const InfallibleTArray<int>& aIntParams,
                                   const InfallibleTArray<nsString>& aStringParams)
{
  nsCOMPtr<nsIDialogParamBlock> params;
  if (gActiveDialogs.Get(this, getter_AddRefs(params))) {
    TabChild::ArraysToParams(aIntParams, aStringParams, params);
    gActiveDialogs.Remove(this);
  }
  return true;
}

void
TabChild::ParamsToArrays(nsIDialogParamBlock* aParams,
                         InfallibleTArray<int>& aIntParams,
                         InfallibleTArray<nsString>& aStringParams)
{
  if (aParams) {
    for (int32_t i = 0; i < 8; ++i) {
      int32_t val = 0;
      aParams->GetInt(i, &val);
      aIntParams.AppendElement(val);
    }
    int32_t j = 0;
    nsXPIDLString strVal;
    while (NS_SUCCEEDED(aParams->GetString(j, getter_Copies(strVal)))) {
      aStringParams.AppendElement(strVal);
      ++j;
    }
  }
}

void
TabChild::ArraysToParams(const InfallibleTArray<int>& aIntParams,
                         const InfallibleTArray<nsString>& aStringParams,
                         nsIDialogParamBlock* aParams)
{
  if (aParams) {
    for (int32_t i = 0; uint32_t(i) < aIntParams.Length(); ++i) {
      aParams->SetInt(i, aIntParams[i]);
    }
    for (int32_t j = 0; uint32_t(j) < aStringParams.Length(); ++j) {
      aParams->SetString(j, aStringParams[j].get());
    }
  }
}

void
TabChild::DestroyWindow()
{
    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebNav);
    if (baseWindow)
        baseWindow->Destroy();

    
    
    
    if (mWidget) {
        mWidget->Destroy();
    }

    if (mRemoteFrame) {
        mRemoteFrame->Destroy();
        mRemoteFrame = nullptr;
    }
}

bool
TabChild::UseDirectCompositor()
{
    return !!CompositorChild::Get();
}

void
TabChild::ActorDestroy(ActorDestroyReason why)
{
  if (mTabChildGlobal) {
    
    
    static_cast<nsFrameMessageManager*>
      (mTabChildGlobal->mMessageManager.get())->Disconnect();
    mTabChildGlobal->mMessageManager = nullptr;
  }
}

TabChild::~TabChild()
{
    DestroyWindow();

    nsCOMPtr<nsIWebBrowser> webBrowser = do_QueryInterface(mWebNav);
    if (webBrowser) {
      webBrowser->SetContainerWindow(nullptr);
    }
    if (mCx) {
      DestroyCx();
    }
    
    if (mTabChildGlobal) {
      nsEventListenerManager* elm = mTabChildGlobal->GetListenerManager(false);
      if (elm) {
        elm->Disconnect();
      }
      mTabChildGlobal->mTabChild = nullptr;
    }
}

void
TabChild::SetProcessNameToAppName()
{
  nsCOMPtr<mozIApplication> app = GetOwnApp();
  if (!app) {
    return;
  }

  nsAutoString appName;
  nsresult rv = app->GetName(appName);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to retrieve app name");
    return;
  }

  ContentChild::GetSingleton()->SetProcessName(appName);
}

bool
TabChild::IsRootContentDocument()
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    return !HasAppOwnerApp();
}

bool
TabChild::RecvLoadURL(const nsCString& uri)
{
    printf("loading %s, %d\n", uri.get(), NS_IsMainThread());
    SetProcessNameToAppName();

    nsresult rv = mWebNav->LoadURI(NS_ConvertUTF8toUTF16(uri).get(),
                                   nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP |
                                   nsIWebNavigation::LOAD_FLAGS_DISALLOW_INHERIT_OWNER,
                                   NULL, NULL, NULL);
    if (NS_FAILED(rv)) {
        NS_WARNING("mWebNav->LoadURI failed. Eating exception, what else can I do?");
    }

#ifdef MOZ_CRASHREPORTER
    CrashReporter::AnnotateCrashReport(NS_LITERAL_CSTRING("URL"), uri);
#endif

    return true;
}

bool
TabChild::RecvCacheFileDescriptor(const nsString& aPath,
                                  const FileDescriptor& aFileDescriptor)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!aPath.IsEmpty());

    
    

    
    const CachedFileDescriptorInfo search(aPath);
    uint32_t index =
        mCachedFileDescriptorInfos.IndexOf(search, 0,
                                           search.PathOnlyComparator());
    if (index == mCachedFileDescriptorInfos.NoIndex) {
        
        
        mCachedFileDescriptorInfos.AppendElement(
            new CachedFileDescriptorInfo(aPath, aFileDescriptor));
        return true;
    }

    nsAutoPtr<CachedFileDescriptorInfo>& info =
        mCachedFileDescriptorInfos[index];

    MOZ_ASSERT(info);
    MOZ_ASSERT(info->mPath == aPath);
    MOZ_ASSERT(!info->mFileDescriptor.IsValid());
    MOZ_ASSERT(info->mCallback);

    
    
    if (info->mCanceled) {
        
        if (aFileDescriptor.IsValid()) {
            nsRefPtr<CloseFileRunnable> runnable =
                new CloseFileRunnable(aFileDescriptor);
            runnable->Dispatch();
        }
    } else {
        
        info->mFileDescriptor = aFileDescriptor;

        
        
        info->FireCallback();
    }

    mCachedFileDescriptorInfos.RemoveElementAt(index);
    return true;
}

bool
TabChild::GetCachedFileDescriptor(const nsAString& aPath,
                                  nsICachedFileDescriptorListener* aCallback)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!aPath.IsEmpty());
    MOZ_ASSERT(aCallback);

    
    
    const CachedFileDescriptorInfo search(aPath);
    uint32_t index =
        mCachedFileDescriptorInfos.IndexOf(search, 0,
                                           search.PathOnlyComparator());
    if (index == mCachedFileDescriptorInfos.NoIndex) {
        
        
        mCachedFileDescriptorInfos.AppendElement(
            new CachedFileDescriptorInfo(aPath, aCallback));
        return false;
    }

    nsAutoPtr<CachedFileDescriptorInfo>& info =
        mCachedFileDescriptorInfos[index];

    MOZ_ASSERT(info);
    MOZ_ASSERT(info->mPath == aPath);
    MOZ_ASSERT(!info->mCallback);
    MOZ_ASSERT(!info->mCanceled);

    info->mCallback = aCallback;

    nsRefPtr<CachedFileDescriptorCallbackRunnable> runnable =
        new CachedFileDescriptorCallbackRunnable(info.forget());
    runnable->Dispatch();

    mCachedFileDescriptorInfos.RemoveElementAt(index);
    return true;
}

void
TabChild::CancelCachedFileDescriptorCallback(
                                     const nsAString& aPath,
                                     nsICachedFileDescriptorListener* aCallback)
{
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(!aPath.IsEmpty());
    MOZ_ASSERT(aCallback);

    const CachedFileDescriptorInfo search(aPath, aCallback);
    uint32_t index =
        mCachedFileDescriptorInfos.IndexOf(search, 0,
                                           search.PathAndCallbackComparator());
    if (index == mCachedFileDescriptorInfos.NoIndex) {
        
        return;
    }

    nsAutoPtr<CachedFileDescriptorInfo>& info =
        mCachedFileDescriptorInfos[index];

    MOZ_ASSERT(info);
    MOZ_ASSERT(info->mPath == aPath);
    MOZ_ASSERT(!info->mFileDescriptor.IsValid());
    MOZ_ASSERT(info->mCallback == aCallback);
    MOZ_ASSERT(!info->mCanceled);

    
    info->mCanceled = true;
}

void
TabChild::DoFakeShow()
{
  RecvShow(nsIntSize(0, 0));
  mDidFakeShow = true;
}

bool
TabChild::RecvShow(const nsIntSize& size)
{

    if (mDidFakeShow) {
        return true;
    }

    printf("[TabChild] SHOW (w,h)= (%d, %d)\n", size.width, size.height);

    nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(mWebNav);
    if (!baseWindow) {
        NS_ERROR("mWebNav doesn't QI to nsIBaseWindow");
        return false;
    }

    if (!InitRenderingState()) {
        
        
        
        
        return true;
    }

    baseWindow->SetVisibility(true);

    return InitTabChildGlobal();
}

bool
TabChild::RecvUpdateDimensions(const nsRect& rect, const nsIntSize& size, const ScreenOrientation& orientation)
{
    if (!mRemoteFrame) {
        return true;
    }

    mOuterRect.x = rect.x;
    mOuterRect.y = rect.y;
    mOuterRect.width = rect.width;
    mOuterRect.height = rect.height;

    mOrientation = orientation;
    mInnerSize = size;
    mWidget->Resize(0, 0, size.width, size.height,
                    true);

    nsCOMPtr<nsIBaseWindow> baseWin = do_QueryInterface(mWebNav);
    baseWin->SetPositionAndSize(0, 0, size.width, size.height,
                                true);

    HandlePossibleViewportChange();

    return true;
}

void
TabChild::DispatchMessageManagerMessage(const nsAString& aMessageName,
                                        const nsACString& aJSONData)
{
    JSAutoRequest ar(mCx);
    jsval json = JSVAL_NULL;
    StructuredCloneData cloneData;
    JSAutoStructuredCloneBuffer buffer;
    if (JS_ParseJSON(mCx,
                      static_cast<const jschar*>(NS_ConvertUTF8toUTF16(aJSONData).get()),
                      aJSONData.Length(),
                      &json)) {
        WriteStructuredClone(mCx, json, buffer, cloneData.mClosure);
        cloneData.mData = buffer.data();
        cloneData.mDataLength = buffer.nbytes();
    }

    nsFrameScriptCx cx(static_cast<nsIWebBrowserChrome*>(this), this);
    
    
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mTabChildGlobal->mMessageManager.get());
    mm->ReceiveMessage(static_cast<nsIDOMEventTarget*>(mTabChildGlobal),
                       aMessageName, false, &cloneData, nullptr, nullptr);
}

static void
ScrollWindowTo(nsIDOMWindow* aWindow, const mozilla::gfx::Point& aPoint)
{
    nsGlobalWindow* window = static_cast<nsGlobalWindow*>(aWindow);
    nsIScrollableFrame* sf = window->GetScrollFrame();

    if (sf) {
        sf->ScrollToCSSPixelsApproximate(aPoint);
    }
}

bool
TabChild::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
    if (!mCx || !mTabChildGlobal) {
        return true;
    }

    gfx::Rect cssCompositedRect =
      AsyncPanZoomController::CalculateCompositedRectInCssPixels(aFrameMetrics);
    
    
    nsCString data;
    data += nsPrintfCString("{ \"x\" : %d", NS_lround(aFrameMetrics.mScrollOffset.x));
    data += nsPrintfCString(", \"y\" : %d", NS_lround(aFrameMetrics.mScrollOffset.y));
    data += nsPrintfCString(", \"viewport\" : ");
        data += nsPrintfCString("{ \"width\" : %f", aFrameMetrics.mViewport.width);
        data += nsPrintfCString(", \"height\" : %f", aFrameMetrics.mViewport.height);
        data += nsPrintfCString(" }");
    data += nsPrintfCString(", \"displayPort\" : ");
        data += nsPrintfCString("{ \"x\" : %f", aFrameMetrics.mDisplayPort.x);
        data += nsPrintfCString(", \"y\" : %f", aFrameMetrics.mDisplayPort.y);
        data += nsPrintfCString(", \"width\" : %f", aFrameMetrics.mDisplayPort.width);
        data += nsPrintfCString(", \"height\" : %f", aFrameMetrics.mDisplayPort.height);
        data += nsPrintfCString(" }");
    data += nsPrintfCString(", \"compositionBounds\" : ");
        data += nsPrintfCString("{ \"x\" : %d", aFrameMetrics.mCompositionBounds.x);
        data += nsPrintfCString(", \"y\" : %d", aFrameMetrics.mCompositionBounds.y);
        data += nsPrintfCString(", \"width\" : %d", aFrameMetrics.mCompositionBounds.width);
        data += nsPrintfCString(", \"height\" : %d", aFrameMetrics.mCompositionBounds.height);
        data += nsPrintfCString(" }");
    data += nsPrintfCString(", \"cssPageRect\" : ");
        data += nsPrintfCString("{ \"x\" : %f", aFrameMetrics.mScrollableRect.x);
        data += nsPrintfCString(", \"y\" : %f", aFrameMetrics.mScrollableRect.y);
        data += nsPrintfCString(", \"width\" : %f", aFrameMetrics.mScrollableRect.width);
        data += nsPrintfCString(", \"height\" : %f", aFrameMetrics.mScrollableRect.height);
        data += nsPrintfCString(" }");
    data += nsPrintfCString(", \"cssCompositedRect\" : ");
            data += nsPrintfCString("{ \"width\" : %f", cssCompositedRect.width);
            data += nsPrintfCString(", \"height\" : %f", cssCompositedRect.height);
            data += nsPrintfCString(" }");
    data += nsPrintfCString(" }");

    DispatchMessageManagerMessage(NS_LITERAL_STRING("Viewport:Change"), data);

    nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
    nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mWebNav);

    utils->SetScrollPositionClampingScrollPortSize(
      cssCompositedRect.width, cssCompositedRect.height);
    ScrollWindowTo(window, aFrameMetrics.mScrollOffset);
    gfxSize resolution = AsyncPanZoomController::CalculateResolution(
      aFrameMetrics);
    utils->SetResolution(resolution.width, resolution.height);

    nsCOMPtr<nsIDOMDocument> domDoc;
    nsCOMPtr<nsIDOMElement> docElement;
    mWebNav->GetDocument(getter_AddRefs(domDoc));
    if (domDoc) {
      domDoc->GetDocumentElement(getter_AddRefs(docElement));
      if (docElement) {
        utils->SetDisplayPortForElement(
          aFrameMetrics.mDisplayPort.x, aFrameMetrics.mDisplayPort.y,
          aFrameMetrics.mDisplayPort.width, aFrameMetrics.mDisplayPort.height,
          docElement);
      }
    }

    mLastMetrics = aFrameMetrics;

    return true;
}

bool
TabChild::RecvHandleDoubleTap(const nsIntPoint& aPoint)
{
    if (!mCx || !mTabChildGlobal) {
        return true;
    }

    nsCString data;
    data += nsPrintfCString("{ \"x\" : %d", aPoint.x);
    data += nsPrintfCString(", \"y\" : %d", aPoint.y);
    data += nsPrintfCString(" }");

    DispatchMessageManagerMessage(NS_LITERAL_STRING("Gesture:DoubleTap"), data);

    return true;
}

bool
TabChild::RecvHandleSingleTap(const nsIntPoint& aPoint)
{
  if (!mCx || !mTabChildGlobal) {
    return true;
  }

  RecvMouseEvent(NS_LITERAL_STRING("mousemove"), aPoint.x, aPoint.y, 0, 1, 0, false);
  RecvMouseEvent(NS_LITERAL_STRING("mousedown"), aPoint.x, aPoint.y, 0, 1, 0, false);
  RecvMouseEvent(NS_LITERAL_STRING("mouseup"), aPoint.x, aPoint.y, 0, 1, 0, false);

  return true;
}

bool
TabChild::RecvHandleLongTap(const nsIntPoint& aPoint)
{
  if (!mCx || !mTabChildGlobal) {
    return true;
  }

  RecvMouseEvent(NS_LITERAL_STRING("contextmenu"), aPoint.x, aPoint.y,
                 2 ,
                 1 ,
                 0 ,
                 false );

  return true;
}

bool
TabChild::RecvActivate()
{
  nsCOMPtr<nsIWebBrowserFocus> browser = do_QueryInterface(mWebNav);
  browser->Activate();
  return true;
}

bool TabChild::RecvDeactivate()
{
  nsCOMPtr<nsIWebBrowserFocus> browser = do_QueryInterface(mWebNav);
  browser->Deactivate();
  return true;
}

bool
TabChild::RecvMouseEvent(const nsString& aType,
                         const float&    aX,
                         const float&    aY,
                         const int32_t&  aButton,
                         const int32_t&  aClickCount,
                         const int32_t&  aModifiers,
                         const bool&     aIgnoreRootScrollFrame)
{
  nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
  NS_ENSURE_TRUE(utils, true);
  bool ignored = false;
  utils->SendMouseEvent(aType, aX, aY, aButton, aClickCount, aModifiers,
                        aIgnoreRootScrollFrame, 0, 0, &ignored);
  return true;
}

bool
TabChild::RecvRealMouseEvent(const nsMouseEvent& event)
{
  nsMouseEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

bool
TabChild::RecvMouseWheelEvent(const WheelEvent& event)
{
  WheelEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

void
TabChild::DispatchSynthesizedMouseEvent(uint32_t aMsg, uint64_t aTime,
                                        const nsIntPoint& aRefPoint)
{
  MOZ_ASSERT(aMsg == NS_MOUSE_MOVE || aMsg == NS_MOUSE_BUTTON_DOWN ||
             aMsg == NS_MOUSE_BUTTON_UP);

  nsMouseEvent event(true, aMsg, NULL,
      nsMouseEvent::eReal, nsMouseEvent::eNormal);
  event.refPoint = aRefPoint;
  event.time = aTime;
  event.button = nsMouseEvent::eLeftButton;
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  if (aMsg != NS_MOUSE_MOVE) {
    event.clickCount = 1;
  }

  DispatchWidgetEvent(event);
}

static nsDOMTouch*
GetTouchForIdentifier(const nsTouchEvent& aEvent, int32_t aId)
{
  for (uint32_t i = 0; i < aEvent.touches.Length(); ++i) {
    nsDOMTouch* touch = static_cast<nsDOMTouch*>(aEvent.touches[i].get());
    if (touch->mIdentifier == aId) {
      return touch;
    }
  }
  return nullptr;
}

void
TabChild::UpdateTapState(const nsTouchEvent& aEvent, nsEventStatus aStatus)
{
  static bool sHavePrefs;
  static bool sClickHoldContextMenusEnabled;
  static nsIntSize sDragThreshold;
  static int32_t sContextMenuDelayMs;
  if (!sHavePrefs) {
    sHavePrefs = true;
    Preferences::AddBoolVarCache(&sClickHoldContextMenusEnabled,
                                 "ui.click_hold_context_menus", true);
    Preferences::AddIntVarCache(&sDragThreshold.width,
                                "ui.dragThresholdX", 25);
    Preferences::AddIntVarCache(&sDragThreshold.height,
                                "ui.dragThresholdY", 25);
    Preferences::AddIntVarCache(&sContextMenuDelayMs,
                                "ui.click_hold_context_menus.delay", 500);
  }

  bool currentlyTrackingTouch = (mActivePointerId >= 0);
  if (aEvent.message == NS_TOUCH_START) {
    if (currentlyTrackingTouch || aEvent.touches.Length() > 1) {
      
      
      
      return;
    }
    if (aStatus == nsEventStatus_eConsumeNoDefault ||
        nsIPresShell::gPreventMouseEvents) {
      return;
    }

    nsDOMTouch* touch = static_cast<nsDOMTouch*>(aEvent.touches[0].get());
    mGestureDownPoint = touch->mRefPoint;
    mActivePointerId = touch->mIdentifier;
    if (sClickHoldContextMenusEnabled) {
      MOZ_ASSERT(!mTapHoldTimer);
      mTapHoldTimer = NewRunnableMethod(this,
                                        &TabChild::FireContextMenuEvent);
      MessageLoop::current()->PostDelayedTask(FROM_HERE, mTapHoldTimer,
                                              sContextMenuDelayMs);
    }
    return;
  }

  
  
  if (!currentlyTrackingTouch) {
    return;
  }
  nsDOMTouch* trackedTouch = GetTouchForIdentifier(aEvent, mActivePointerId);
  if (!trackedTouch) {
    return;
  }

  nsIntPoint currentPoint = trackedTouch->mRefPoint;
  int64_t time = aEvent.time;
  switch (aEvent.message) {
  case NS_TOUCH_MOVE:
    if (abs(currentPoint.x - mGestureDownPoint.x) > sDragThreshold.width ||
        abs(currentPoint.y - mGestureDownPoint.y) > sDragThreshold.height) {
      CancelTapTracking();
    }
    return;

  case NS_TOUCH_END:
    if (!nsIPresShell::gPreventMouseEvents) {
      DispatchSynthesizedMouseEvent(NS_MOUSE_MOVE, time, currentPoint);
      DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_DOWN, time, currentPoint);
      DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_UP, time, currentPoint);
    }
    
  case NS_TOUCH_CANCEL:
    CancelTapTracking();
    return;

  default:
    NS_WARNING("Unknown touch event type");
  }
}

void
TabChild::FireContextMenuEvent()
{
  MOZ_ASSERT(mTapHoldTimer && mActivePointerId >= 0);
  RecvHandleLongTap(mGestureDownPoint);
  CancelTapTracking();
}

void
TabChild::CancelTapTracking()
{
  mActivePointerId = -1;
  if (mTapHoldTimer) {
    mTapHoldTimer->Cancel();
  }
  mTapHoldTimer = nullptr;
}

bool
TabChild::RecvRealTouchEvent(const nsTouchEvent& aEvent)
{
  nsTouchEvent localEvent(aEvent);
  nsEventStatus status = DispatchWidgetEvent(localEvent);

  if (IsAsyncPanZoomEnabled()) {
    nsCOMPtr<nsPIDOMWindow> outerWindow = do_GetInterface(mWebNav);
    nsCOMPtr<nsPIDOMWindow> innerWindow = outerWindow->GetCurrentInnerWindow();

    if (innerWindow && innerWindow->HasTouchEventListeners()) {
      SendContentReceivedTouch(nsIPresShell::gPreventMouseEvents);
    }
  } else {
    UpdateTapState(aEvent, status);
  }

  return true;
}

bool
TabChild::RecvRealTouchMoveEvent(const nsTouchEvent& aEvent)
{
  return RecvRealTouchEvent(aEvent);
}

bool
TabChild::RecvRealKeyEvent(const nsKeyEvent& event)
{
  nsKeyEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

bool
TabChild::RecvKeyEvent(const nsString& aType,
                       const int32_t& aKeyCode,
                       const int32_t& aCharCode,
                       const int32_t& aModifiers,
                       const bool& aPreventDefault)
{
  nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
  NS_ENSURE_TRUE(utils, true);
  bool ignored = false;
  utils->SendKeyEvent(aType, aKeyCode, aCharCode,
                      aModifiers, aPreventDefault, &ignored);
  return true;
}

bool
TabChild::RecvCompositionEvent(const nsCompositionEvent& event)
{
  nsCompositionEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

bool
TabChild::RecvTextEvent(const nsTextEvent& event)
{
  nsTextEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  IPC::ParamTraits<nsTextEvent>::Free(event);
  return true;
}

bool
TabChild::RecvSelectionEvent(const nsSelectionEvent& event)
{
  nsSelectionEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

nsEventStatus
TabChild::DispatchWidgetEvent(nsGUIEvent& event)
{
  if (!mWidget)
    return nsEventStatus_eConsumeNoDefault;

  nsEventStatus status;
  event.widget = mWidget;
  NS_ENSURE_SUCCESS(mWidget->DispatchEvent(&event, status),
                    nsEventStatus_eConsumeNoDefault);
  return status;
}

PDocumentRendererChild*
TabChild::AllocPDocumentRenderer(const nsRect& documentRect,
                                 const gfxMatrix& transform,
                                 const nsString& bgcolor,
                                 const uint32_t& renderFlags,
                                 const bool& flushLayout,
                                 const nsIntSize& renderSize)
{
    return new DocumentRendererChild();
}

bool
TabChild::DeallocPDocumentRenderer(PDocumentRendererChild* actor)
{
    delete actor;
    return true;
}

bool
TabChild::RecvPDocumentRendererConstructor(PDocumentRendererChild* actor,
                                           const nsRect& documentRect,
                                           const gfxMatrix& transform,
                                           const nsString& bgcolor,
                                           const uint32_t& renderFlags,
                                           const bool& flushLayout,
                                           const nsIntSize& renderSize)
{
    DocumentRendererChild *render = static_cast<DocumentRendererChild *>(actor);

    nsCOMPtr<nsIWebBrowser> browser = do_QueryInterface(mWebNav);
    if (!browser)
        return true; 
    nsCOMPtr<nsIDOMWindow> window;
    if (NS_FAILED(browser->GetContentDOMWindow(getter_AddRefs(window))) ||
        !window)
    {
        return true; 
    }

    nsCString data;
    bool ret = render->RenderDocument(window,
                                      documentRect, transform,
                                      bgcolor,
                                      renderFlags, flushLayout,
                                      renderSize, data);
    if (!ret)
        return true; 

    return PDocumentRendererChild::Send__delete__(actor, renderSize, data);
}

PContentDialogChild*
TabChild::AllocPContentDialog(const uint32_t&,
                              const nsCString&,
                              const nsCString&,
                              const InfallibleTArray<int>&,
                              const InfallibleTArray<nsString>&)
{
  return new ContentDialogChild();
}

bool
TabChild::DeallocPContentDialog(PContentDialogChild* aDialog)
{
  delete aDialog;
  return true;
}

PContentPermissionRequestChild*
TabChild::AllocPContentPermissionRequest(const nsCString& aType, const nsCString& aAccess, const IPC::Principal&)
{
  NS_RUNTIMEABORT("unused");
  return nullptr;
}

bool
TabChild::DeallocPContentPermissionRequest(PContentPermissionRequestChild* actor)
{
    PCOMContentPermissionRequestChild* child =
        static_cast<PCOMContentPermissionRequestChild*>(actor);
#ifdef DEBUG
    child->mIPCOpen = false;
#endif 
    child->IPDLRelease();
    return true;
}

bool
TabChild::RecvActivateFrameEvent(const nsString& aType, const bool& capture)
{
  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNav);
  NS_ENSURE_TRUE(window, true);
  nsCOMPtr<nsIDOMEventTarget> chromeHandler =
    do_QueryInterface(window->GetChromeEventHandler());
  NS_ENSURE_TRUE(chromeHandler, true);
  nsRefPtr<ContentListener> listener = new ContentListener(this);
  NS_ENSURE_TRUE(listener, true);
  chromeHandler->AddEventListener(aType, listener, capture);
  return true;
}

POfflineCacheUpdateChild*
TabChild::AllocPOfflineCacheUpdate(const URIParams& manifestURI,
                                   const URIParams& documentURI,
                                   const bool& stickDocument)
{
  NS_RUNTIMEABORT("unused");
  return nullptr;
}

bool
TabChild::DeallocPOfflineCacheUpdate(POfflineCacheUpdateChild* actor)
{
  OfflineCacheUpdateChild* offlineCacheUpdate = static_cast<OfflineCacheUpdateChild*>(actor);
  delete offlineCacheUpdate;
  return true;
}

bool
TabChild::RecvLoadRemoteScript(const nsString& aURL)
{
  if (!mCx && !InitTabChildGlobal())
    
    
    return true;

  LoadFrameScriptInternal(aURL);
  return true;
}

bool
TabChild::RecvAsyncMessage(const nsString& aMessage,
                           const ClonedMessageData& aData)
{
  if (mTabChildGlobal) {
    nsFrameScriptCx cx(static_cast<nsIWebBrowserChrome*>(this), this);
    StructuredCloneData cloneData = UnpackClonedMessageDataForChild(aData);
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mTabChildGlobal->mMessageManager.get());
    mm->ReceiveMessage(static_cast<nsIDOMEventTarget*>(mTabChildGlobal),
                       aMessage, false, &cloneData, nullptr, nullptr);
  }
  return true;
}

class UnloadScriptEvent : public nsRunnable
{
public:
  UnloadScriptEvent(TabChild* aTabChild, TabChildGlobal* aTabChildGlobal)
    : mTabChild(aTabChild), mTabChildGlobal(aTabChildGlobal)
  { }

  NS_IMETHOD Run()
  {
    nsCOMPtr<nsIDOMEvent> event;
    NS_NewDOMEvent(getter_AddRefs(event), nullptr, nullptr);
    if (event) {
      event->InitEvent(NS_LITERAL_STRING("unload"), false, false);
      event->SetTrusted(true);

      bool dummy;
      mTabChildGlobal->DispatchEvent(event, &dummy);
    }

    return NS_OK;
  }

  nsRefPtr<TabChild> mTabChild;
  TabChildGlobal* mTabChildGlobal;
};

bool
TabChild::RecvDestroy()
{
  if (mTabChildGlobal) {
    
    nsContentUtils::AddScriptRunner(
      new UnloadScriptEvent(this, mTabChildGlobal)
    );
  }

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

  observerService->RemoveObserver(this, CANCEL_DEFAULT_PAN_ZOOM);
  observerService->RemoveObserver(this, BROWSER_ZOOM_TO_RECT);
  observerService->RemoveObserver(this, BEFORE_FIRST_PAINT);
  observerService->RemoveObserver(this, DETECT_SCROLLABLE_SUBFRAME);

  const InfallibleTArray<PIndexedDBChild*>& idbActors =
    ManagedPIndexedDBChild();
  for (uint32_t i = 0; i < idbActors.Length(); ++i) {
    static_cast<IndexedDBChild*>(idbActors[i])->Disconnect();
  }

  
  DestroyWindow();

  return Send__delete__(this);
}

 bool
TabChild::RecvSetAppType(const nsString& aAppType)
{
  MOZ_ASSERT_IF(!aAppType.IsEmpty(), HasOwnApp());
  mAppType = aAppType;
  return true;
}

PRenderFrameChild*
TabChild::AllocPRenderFrame(ScrollingBehavior* aScrolling,
                            LayersBackend* aBackend,
                            int32_t* aMaxTextureSize,
                            uint64_t* aLayersId)
{
    return new RenderFrameChild();
}

bool
TabChild::DeallocPRenderFrame(PRenderFrameChild* aFrame)
{
    delete aFrame;
    return true;
}

bool
TabChild::InitTabChildGlobal(FrameScriptLoading aScriptLoading)
{
  if (!mCx && !mTabChildGlobal) {
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNav);
    NS_ENSURE_TRUE(window, false);
    nsCOMPtr<nsIDOMEventTarget> chromeHandler =
      do_QueryInterface(window->GetChromeEventHandler());
    NS_ENSURE_TRUE(chromeHandler, false);

    nsRefPtr<TabChildGlobal> scope = new TabChildGlobal(this);
    NS_ENSURE_TRUE(scope, false);

    mTabChildGlobal = scope;

    nsISupports* scopeSupports = NS_ISUPPORTS_CAST(nsIDOMEventTarget*, scope);

    NS_ENSURE_TRUE(InitTabChildGlobalInternal(scopeSupports), false); 

    scope->Init();

    nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(chromeHandler);
    NS_ENSURE_TRUE(root, false);
    root->SetParentTarget(scope);

    chromeHandler->AddEventListener(NS_LITERAL_STRING("DOMMetaAdded"), this, false);
  }

  if (aScriptLoading != DONT_LOAD_SCRIPTS && !mTriedBrowserInit) {
    mTriedBrowserInit = true;
    
    
    if (IsBrowserOrApp()) {
      RecvLoadRemoteScript(BROWSER_ELEMENT_CHILD_SCRIPT);
    }
  }

  return true;
}

bool
TabChild::InitRenderingState()
{
    static_cast<PuppetWidget*>(mWidget.get())->InitIMEState();

    LayersBackend be;
    uint64_t id;
    int32_t maxTextureSize;
    RenderFrameChild* remoteFrame =
        static_cast<RenderFrameChild*>(SendPRenderFrameConstructor(
                                           &mScrolling, &be, &maxTextureSize, &id));
    if (!remoteFrame) {
      NS_WARNING("failed to construct RenderFrame");
      return false;
    }

    PLayersChild* shadowManager = nullptr;
    if (id != 0) {
        
        
        shadowManager =
            CompositorChild::Get()->SendPLayersConstructor(be, id,
                                                           &be,
                                                           &maxTextureSize);
    } else {
        
        shadowManager = remoteFrame->SendPLayersConstructor();
    }

    if (!shadowManager) {
      NS_WARNING("failed to construct LayersChild");
      
      PRenderFrameChild::Send__delete__(remoteFrame);
      return false;
    }

    ShadowLayerForwarder* lf =
        mWidget->GetLayerManager(shadowManager, be)->AsShadowForwarder();
    NS_ABORT_IF_FALSE(lf && lf->HasShadowManager(),
                      "PuppetWidget should have shadow manager");
    lf->SetParentBackendType(be);
    lf->SetMaxTextureSize(maxTextureSize);

    mRemoteFrame = remoteFrame;

    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(NS_OBSERVERSERVICE_CONTRACTID);

    if (observerService) {
        observerService->AddObserver(this,
                                     CANCEL_DEFAULT_PAN_ZOOM,
                                     false);
        observerService->AddObserver(this,
                                     BROWSER_ZOOM_TO_RECT,
                                     false);
        observerService->AddObserver(this,
                                     BEFORE_FIRST_PAINT,
                                     false);
        observerService->AddObserver(this,
                                     DETECT_SCROLLABLE_SUBFRAME,
                                     false);
    }

    return true;
}

void
TabChild::SetBackgroundColor(const nscolor& aColor)
{
  if (mLastBackgroundColor != aColor) {
    mLastBackgroundColor = aColor;
    SendSetBackgroundColor(mLastBackgroundColor);
  }
}

void
TabChild::GetDPI(float* aDPI)
{
    *aDPI = -1.0;
    if (!mRemoteFrame) {
        return;
    }

    SendGetDPI(aDPI);
}

void
TabChild::NotifyPainted()
{
    if (UseDirectCompositor() && !mNotified) {
        mRemoteFrame->SendNotifyCompositorTransaction();
        mNotified = true;
    }
}

bool
TabChild::IsAsyncPanZoomEnabled()
{
    return mScrolling == ASYNC_PAN_ZOOM;
}

void
TabChild::MakeVisible()
{
    if (mWidget) {
        mWidget->Show(true);
    }
}

void
TabChild::MakeHidden()
{
    if (mWidget) {
        mWidget->Show(false);
    }
}

NS_IMETHODIMP
TabChild::GetMessageManager(nsIContentFrameMessageManager** aResult)
{
  if (mTabChildGlobal) {
    NS_ADDREF(*aResult = mTabChildGlobal);
    return NS_OK;
  }
  *aResult = nullptr;
  return NS_ERROR_FAILURE;
}

PIndexedDBChild*
TabChild::AllocPIndexedDB(const nsCString& aASCIIOrigin, bool* )
{
  NS_NOTREACHED("Should never get here!");
  return NULL;
}

bool
TabChild::DeallocPIndexedDB(PIndexedDBChild* aActor)
{
  delete aActor;
  return true;
}

bool
TabChild::DoSendSyncMessage(const nsAString& aMessage,
                            const StructuredCloneData& aData,
                            InfallibleTArray<nsString>* aJSONRetVal)
{
  ContentChild* cc = static_cast<ContentChild*>(Manager());
  ClonedMessageData data;
  if (!BuildClonedMessageDataForChild(cc, aData, data)) {
    return false;
  }
  return SendSyncMessage(nsString(aMessage), data, aJSONRetVal);
}

bool
TabChild::DoSendAsyncMessage(const nsAString& aMessage,
                             const StructuredCloneData& aData)
{
  ContentChild* cc = static_cast<ContentChild*>(Manager());
  ClonedMessageData data;
  if (!BuildClonedMessageDataForChild(cc, aData, data)) {
    return false;
  }
  return SendAsyncMessage(nsString(aMessage), data);
}


TabChildGlobal::TabChildGlobal(TabChild* aTabChild)
: mTabChild(aTabChild)
{
}

void
TabChildGlobal::Init()
{
  NS_ASSERTION(!mMessageManager, "Re-initializing?!?");
  mMessageManager = new nsFrameMessageManager(mTabChild,
                                              nullptr,
                                              mTabChild->GetJSContext(),
                                              MM_CHILD);
}

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(TabChildGlobal,
                                                nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(TabChildGlobal,
                                                  nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mMessageManager)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TabChildGlobal)
  NS_INTERFACE_MAP_ENTRY(nsIMessageListenerManager)
  NS_INTERFACE_MAP_ENTRY(nsIMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsISyncMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsIContentFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIScriptContextPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ContentFrameMessageManager)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(TabChildGlobal, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(TabChildGlobal, nsDOMEventTargetHelper)




NS_IMETHODIMP_(bool)
TabChildGlobal::MarkForCC()
{
  return mMessageManager ? mMessageManager->MarkForCC() : false;
}

NS_IMETHODIMP
TabChildGlobal::GetContent(nsIDOMWindow** aContent)
{
  *aContent = nullptr;
  if (!mTabChild)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMWindow> window = do_GetInterface(mTabChild->WebNavigation());
  window.swap(*aContent);
  return NS_OK;
}

NS_IMETHODIMP
TabChildGlobal::PrivateNoteIntentionalCrash()
{
    mozilla::NoteIntentionalCrash("tab");
    return NS_OK;
}

NS_IMETHODIMP
TabChildGlobal::GetDocShell(nsIDocShell** aDocShell)
{
  *aDocShell = nullptr;
  if (!mTabChild)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDocShell> docShell = do_GetInterface(mTabChild->WebNavigation());
  docShell.swap(*aDocShell);
  return NS_OK;
}

NS_IMETHODIMP
TabChildGlobal::Btoa(const nsAString& aBinaryData,
                     nsAString& aAsciiBase64String)
{
  return nsContentUtils::Btoa(aBinaryData, aAsciiBase64String);
}

NS_IMETHODIMP
TabChildGlobal::Atob(const nsAString& aAsciiString,
                     nsAString& aBinaryData)
{
  return nsContentUtils::Atob(aAsciiString, aBinaryData);
}

JSContext*
TabChildGlobal::GetJSContextForEventHandlers()
{
  if (!mTabChild)
    return nullptr;
  return mTabChild->GetJSContext();
}

nsIPrincipal* 
TabChildGlobal::GetPrincipal()
{
  if (!mTabChild)
    return nullptr;
  return mTabChild->GetPrincipal();
}
