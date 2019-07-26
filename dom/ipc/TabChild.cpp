





#include "base/basictypes.h"

#include "TabChild.h"

#include "Layers.h"
#include "ContentChild.h"
#include "IndexedDBChild.h"
#include "mozilla/Preferences.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/IntentionalCrash.h"
#include "mozilla/docshell/OfflineCacheUpdateChild.h"
#include "mozilla/dom/PContentDialogChild.h"
#include "mozilla/ipc/DocumentRendererChild.h"
#include "mozilla/ipc/FileDescriptorUtils.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/CompositorChild.h"
#include "mozilla/layers/ImageBridgeChild.h"
#include "mozilla/layers/ShadowLayers.h"
#include "mozilla/layout/RenderFrameChild.h"
#include "mozilla/MouseEvents.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "mozilla/TextEvents.h"
#include "mozilla/TouchEvents.h"
#include "mozilla/unused.h"
#include "mozIApplication.h"
#include "nsContentUtils.h"
#include "nsCxPusher.h"
#include "nsEmbedCID.h"
#include "nsEventListenerManager.h"
#include <algorithm>
#ifdef MOZ_CRASHREPORTER
#include "nsExceptionHandler.h"
#endif
#include "mozilla/dom/Element.h"
#include "nsIBaseWindow.h"
#include "nsICachedFileDescriptorListener.h"
#include "nsIDialogParamBlock.h"
#include "nsIDocumentInlines.h"
#include "nsIDocShellTreeOwner.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowUtils.h"
#include "nsIDocShell.h"
#include "nsIURI.h"
#include "nsIURIFixup.h"
#include "nsCDefaultURIFixup.h"
#include "nsIWebBrowser.h"
#include "nsIWebBrowserFocus.h"
#include "nsIWebBrowserSetup.h"
#include "nsIWebProgress.h"
#include "nsIXULRuntime.h"
#include "nsInterfaceHashtable.h"
#include "nsPIDOMWindow.h"
#include "nsPIWindowRoot.h"
#include "nsLayoutUtils.h"
#include "nsPrintfCString.h"
#include "nsThreadUtils.h"
#include "nsWeakReference.h"
#include "PermissionMessageUtils.h"
#include "PCOMContentPermissionRequestChild.h"
#include "PuppetWidget.h"
#include "StructuredCloneUtils.h"
#include "nsViewportInfo.h"
#include "JavaScriptChild.h"
#include "APZCCallbackHelper.h"
#include "nsILoadContext.h"
#include "ipc/nsGUIEventIPC.h"
#include "mozilla/gfx/Matrix.h"

#ifdef DEBUG
#include "PCOMContentPermissionRequestChild.h"
#endif 

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
using namespace mozilla::jsipc;

NS_IMPL_ISUPPORTS1(ContentListener, nsIDOMEventListener)

static const CSSSize kDefaultViewportSize(980, 480);

static const char BROWSER_ZOOM_TO_RECT[] = "browser-zoom-to-rect";
static const char BEFORE_FIRST_PAINT[] = "before-first-paint";

static bool sCpowsEnabled = false;

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

    nsRefPtr<TabChild> tab(new TabChild(ContentChild::GetSingleton(),
                                        TabContext(),  0));
    if (!NS_SUCCEEDED(tab->Init()) ||
        !tab->InitTabChildGlobal(DONT_LOAD_SCRIPTS)) {
        return;
    }
    
    tab->TryCacheLoadAndCompileScript(BROWSER_ELEMENT_CHILD_SCRIPT, true);
    
    tab->RecvLoadRemoteScript(
        NS_LITERAL_STRING("chrome://global/content/preload.js"),
        true);

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
TabChild::Create(ContentChild* aManager, const TabContext &aContext, uint32_t aChromeFlags)
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

    nsRefPtr<TabChild> iframe = new TabChild(aManager,
                                             aContext, aChromeFlags);
    return NS_SUCCEEDED(iframe->Init()) ? iframe.forget() : nullptr;
}


TabChild::TabChild(ContentChild* aManager, const TabContext& aContext, uint32_t aChromeFlags)
  : TabContext(aContext)
  , mRemoteFrame(nullptr)
  , mManager(aManager)
  , mTabChildGlobal(nullptr)
  , mChromeFlags(aChromeFlags)
  , mOuterRect(0, 0, 0, 0)
  , mInnerSize(0, 0)
  , mActivePointerId(-1)
  , mTapHoldTimer(nullptr)
  , mAppPackageFileDescriptorRecved(false)
  , mOldViewportWidth(0.0f)
  , mLastBackgroundColor(NS_RGB(255, 255, 255))
  , mDidFakeShow(false)
  , mNotified(false)
  , mContentDocumentIsDisplayed(false)
  , mTriedBrowserInit(false)
  , mOrientation(eScreenOrientation_PortraitPrimary)
  , mUpdateHitRegion(false)
  , mContextMenuHandled(false)
  , mWaitingTouchListeners(false)
{
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
                  const char16_t *aData)
{
  if (!strcmp(aTopic, BROWSER_ZOOM_TO_RECT)) {
    nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aSubject));
    nsCOMPtr<nsITabChild> tabChild(TabChild::GetFrom(docShell));
    if (tabChild == this) {
      nsCOMPtr<nsIDocument> doc(GetDocument());
      uint32_t presShellId;
      ViewID viewId;
      if (APZCCallbackHelper::GetScrollIdentifiers(doc->GetDocumentElement(),
                                                    &presShellId, &viewId)) {
        CSSRect rect;
        sscanf(NS_ConvertUTF16toUTF8(aData).get(),
               "{\"x\":%f,\"y\":%f,\"w\":%f,\"h\":%f}",
               &rect.x, &rect.y, &rect.width, &rect.height);
        SendZoomToRect(presShellId, viewId, rect);
      }
    }
  } else if (!strcmp(aTopic, BEFORE_FIRST_PAINT)) {
    if (IsAsyncPanZoomEnabled()) {
      nsCOMPtr<nsIDocument> subject(do_QueryInterface(aSubject));
      nsCOMPtr<nsIDocument> doc(GetDocument());

      if (SameCOMIdentity(subject, doc)) {
        nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());

        mContentDocumentIsDisplayed = true;

        
        
        
        SetCSSViewport(kDefaultViewportSize);

        
        
        
        mLastRootMetrics.mViewport = CSSRect(CSSPoint(), kDefaultViewportSize);
        mLastRootMetrics.mCompositionBounds = ScreenIntRect(ScreenIntPoint(), mInnerSize);
        mLastRootMetrics.mZoom = mLastRootMetrics.CalculateIntrinsicScale();
        mLastRootMetrics.mDevPixelsPerCSSPixel = mWidget->GetDefaultScale();
        
        
        mLastRootMetrics.mCumulativeResolution =
          mLastRootMetrics.mZoom / mLastRootMetrics.mDevPixelsPerCSSPixel * ScreenToLayerScale(1);
        
        
        mLastRootMetrics.mResolution = mLastRootMetrics.mCumulativeResolution / LayoutDeviceToParentLayerScale(1);
        mLastRootMetrics.mScrollOffset = CSSPoint(0, 0);

        utils->SetResolution(mLastRootMetrics.mResolution.scale,
                             mLastRootMetrics.mResolution.scale);

        HandlePossibleViewportChange();
      }
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
                         const char16_t* aMessage)
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
TabChild::SetCSSViewport(const CSSSize& aSize)
{
  mOldViewportWidth = aSize.width;

  if (mContentDocumentIsDisplayed) {
    nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
    utils->SetCSSViewport(aSize.width, aSize.height);
  }
}

static CSSSize
GetPageSize(nsCOMPtr<nsIDocument> aDocument, const CSSSize& aViewport)
{
  nsCOMPtr<Element> htmlDOMElement = aDocument->GetHtmlElement();
  HTMLBodyElement* bodyDOMElement = aDocument->GetBodyElement();

  if (!htmlDOMElement && !bodyDOMElement) {
    
    return aViewport;
  }

  int32_t htmlWidth = 0, htmlHeight = 0;
  if (htmlDOMElement) {
    htmlWidth = htmlDOMElement->ScrollWidth();
    htmlHeight = htmlDOMElement->ScrollHeight();
  }
  int32_t bodyWidth = 0, bodyHeight = 0;
  if (bodyDOMElement) {
    bodyWidth = bodyDOMElement->ScrollWidth();
    bodyHeight = bodyDOMElement->ScrollHeight();
  }
  return CSSSize(std::max(htmlWidth, bodyWidth),
                 std::max(htmlHeight, bodyHeight));
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

  nsViewportInfo viewportInfo = nsContentUtils::GetViewportInfo(document, mInnerSize);
  uint32_t presShellId;
  ViewID viewId;
  if (APZCCallbackHelper::GetScrollIdentifiers(document->GetDocumentElement(),
                                               &presShellId, &viewId)) {
    ZoomConstraints constraints(
      viewportInfo.IsZoomAllowed(),
      viewportInfo.GetMinZoom(),
      viewportInfo.GetMaxZoom());
    SendUpdateZoomConstraints(presShellId,
                              viewId,
                               true,
                              constraints);
  }


  float screenW = mInnerSize.width;
  float screenH = mInnerSize.height;
  CSSSize viewport(viewportInfo.GetSize());

  
  
  if (!screenW || !screenH) {
    return;
  }

  float oldBrowserWidth = mOldViewportWidth;
  mLastRootMetrics.mViewport.SizeTo(viewport);
  if (!oldBrowserWidth) {
    oldBrowserWidth = kDefaultViewportSize.width;
  }
  SetCSSViewport(viewport);

  
  
  
  
  
  
  
  
  
  if (!mContentDocumentIsDisplayed) {
    return;
  }

  float oldScreenWidth = mLastRootMetrics.mCompositionBounds.width;
  if (!oldScreenWidth) {
    oldScreenWidth = mInnerSize.width;
  }

  FrameMetrics metrics(mLastRootMetrics);
  metrics.mViewport = CSSRect(CSSPoint(), viewport);
  metrics.mCompositionBounds = ScreenIntRect(ScreenIntPoint(), mInnerSize);

  
  
  
  
  
  
  
  
  
  
  
  
  float oldIntrinsicScale = oldScreenWidth / oldBrowserWidth;
  metrics.mZoom.scale *= metrics.CalculateIntrinsicScale().scale / oldIntrinsicScale;

  
  
  bool isFirstPaint;
  nsresult rv = utils->GetIsFirstPaint(&isFirstPaint);
  if (NS_FAILED(rv) || isFirstPaint) {
    
    
    
    if (viewportInfo.GetDefaultZoom().scale < 0.01f) {
      viewportInfo.SetDefaultZoom(metrics.CalculateIntrinsicScale());
    }

    CSSToScreenScale defaultZoom = viewportInfo.GetDefaultZoom();
    MOZ_ASSERT(viewportInfo.GetMinZoom() <= defaultZoom &&
               defaultZoom <= viewportInfo.GetMaxZoom());
    metrics.mZoom = defaultZoom;

    metrics.mScrollId = viewId;
  }

  metrics.mDisplayPort = AsyncPanZoomController::CalculatePendingDisplayPort(
    
    
    
    metrics, ScreenPoint(0.0f, 0.0f), 0.0);
  metrics.mCumulativeResolution = metrics.mZoom / metrics.mDevPixelsPerCSSPixel * ScreenToLayerScale(1);
  
  
  metrics.mResolution = metrics.mCumulativeResolution / LayoutDeviceToParentLayerScale(1);
  utils->SetResolution(metrics.mResolution.scale, metrics.mResolution.scale);

  CSSSize scrollPort = metrics.CalculateCompositedRectInCssPixels().Size();
  utils->SetScrollPositionClampingScrollPortSize(scrollPort.width, scrollPort.height);

  
  
  

  CSSSize pageSize = GetPageSize(document, viewport);
  if (!pageSize.width) {
    
    return;
  }
  metrics.mScrollableRect = CSSRect(CSSPoint(), pageSize);

  
  
  ProcessUpdateFrame(metrics);
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

  docShell->SetAffectPrivateSessionLifetime(
      mChromeFlags & nsIWebBrowserChrome::CHROME_PRIVATE_LIFETIME);
  nsCOMPtr<nsILoadContext> loadContext = do_GetInterface(mWebNav);
  MOZ_ASSERT(loadContext);
  loadContext->SetPrivateBrowsing(
      mChromeFlags & nsIWebBrowserChrome::CHROME_PRIVATE_WINDOW);

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
  NS_INTERFACE_MAP_ENTRY(nsITooltipListener)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(TabChild)
NS_IMPL_RELEASE(TabChild)

NS_IMETHODIMP
TabChild::SetStatus(uint32_t aStatusType, const char16_t* aStatus)
{
  return SetStatusWithContext(aStatusType,
      aStatus ? static_cast<const nsString &>(nsDependentString(aStatus))
              : EmptyString(),
      nullptr);
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
  
  
  if (mRemoteFrame)
    SendSetStatus(aStatusType, nsString(aStatusText));
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
  NS_WARNING("TabChild::SetFocus not supported in TabChild");

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
TabChild::GetTitle(char16_t** aTitle)
{
  NS_NOTREACHED("TabChild::GetTitle not supported in TabChild");

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
TabChild::SetTitle(const char16_t* aTitle)
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
  NS_WARNING("TabChild::Blur not supported in TabChild");

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
      new TabChild(ContentChild::GetSingleton(),
                    *this,  0);
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

already_AddRefed<nsIDOMWindowUtils>
TabChild::GetDOMWindowUtils()
{
  nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNav);
  nsCOMPtr<nsIDOMWindowUtils> utils = do_GetInterface(window);
  return utils.forget();
}

already_AddRefed<nsIDocument>
TabChild::GetDocument()
{
  nsCOMPtr<nsIDOMDocument> domDoc;
  mWebNav->GetDocument(getter_AddRefs(domDoc));
  nsCOMPtr<nsIDocument> doc(do_QueryInterface(domDoc));
  return doc.forget();
}

static nsInterfaceHashtable<nsPtrHashKey<PContentDialogChild>, nsIDialogParamBlock>* gActiveDialogs;

NS_IMETHODIMP
TabChild::OpenDialog(uint32_t aType, const nsACString& aName,
                     const nsACString& aFeatures,
                     nsIDialogParamBlock* aArguments,
                     nsIDOMElement* aFrameElement)
{
  if (!gActiveDialogs) {
    gActiveDialogs = new nsInterfaceHashtable<nsPtrHashKey<PContentDialogChild>, nsIDialogParamBlock>;
  }
  InfallibleTArray<int32_t> intParams;
  InfallibleTArray<nsString> stringParams;
  ParamsToArrays(aArguments, intParams, stringParams);
  PContentDialogChild* dialog =
    SendPContentDialogConstructor(aType, nsCString(aName),
                                  nsCString(aFeatures), intParams, stringParams);
  gActiveDialogs->Put(dialog, aArguments);
  nsIThread *thread = NS_GetCurrentThread();
  while (gActiveDialogs && gActiveDialogs->GetWeak(dialog)) {
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
  if (gActiveDialogs && gActiveDialogs->Get(this, getter_AddRefs(params))) {
    TabChild::ArraysToParams(aIntParams, aStringParams, params);
    gActiveDialogs->Remove(this);
    if (gActiveDialogs->Count() == 0) {
      delete gActiveDialogs;
      gActiveDialogs = nullptr;
    }
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

#ifdef DEBUG
PContentPermissionRequestChild*
TabChild:: SendPContentPermissionRequestConstructor(PContentPermissionRequestChild* aActor,
                                                    const nsCString& aType,
                                                    const nsCString& aAccess,
                                                    const IPC::Principal& aPrincipal)
{
  PCOMContentPermissionRequestChild* child = static_cast<PCOMContentPermissionRequestChild*>(aActor);
  PContentPermissionRequestChild* request = PBrowserChild::SendPContentPermissionRequestConstructor(aActor, aType, aAccess, aPrincipal);
  child->mIPCOpen = true;
  return request;
}
#endif 

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
    mGlobal = nullptr;

    if (mTabChildGlobal) {
      nsEventListenerManager* elm = mTabChildGlobal->GetExistingListenerManager();
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
    SetProcessNameToAppName();

    nsresult rv = mWebNav->LoadURI(NS_ConvertUTF8toUTF16(uri).get(),
                                   nsIWebNavigation::LOAD_FLAGS_ALLOW_THIRD_PARTY_FIXUP |
                                   nsIWebNavigation::LOAD_FLAGS_DISALLOW_INHERIT_OWNER,
                                   nullptr, nullptr, nullptr);
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
    MOZ_ASSERT(!mAppPackageFileDescriptorRecved);

    mAppPackageFileDescriptorRecved = true;

    
    

    
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
        
        
        if (!mAppPackageFileDescriptorRecved) {
          mCachedFileDescriptorInfos.AppendElement(
              new CachedFileDescriptorInfo(aPath, aCallback));
        }
        return false;
    }

    nsAutoPtr<CachedFileDescriptorInfo>& info =
        mCachedFileDescriptorInfos[index];

    MOZ_ASSERT(info);
    MOZ_ASSERT(info->mPath == aPath);

    
    
    
    if (info->mCanceled) {
        
        
        mCachedFileDescriptorInfos.InsertElementAt(index,
            new CachedFileDescriptorInfo(aPath, aCallback));
        return false;
    }

    MOZ_ASSERT(!info->mCallback);
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

    if (mAppPackageFileDescriptorRecved) {
      
      return;
    }

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
    mInnerSize = ScreenIntSize::FromUnknownSize(
      gfx::IntSize(size.width, size.height));
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
    AutoSafeJSContext cx;
    JS::Rooted<JS::Value> json(cx, JSVAL_NULL);
    StructuredCloneData cloneData;
    JSAutoStructuredCloneBuffer buffer;
    if (JS_ParseJSON(cx,
                      static_cast<const jschar*>(NS_ConvertUTF8toUTF16(aJSONData).get()),
                      aJSONData.Length(),
                      &json)) {
        WriteStructuredClone(cx, json, buffer, cloneData.mClosure);
        cloneData.mData = buffer.data();
        cloneData.mDataLength = buffer.nbytes();
    }

    nsCOMPtr<nsIXPConnectJSObjectHolder> kungFuDeathGrip(GetGlobal());
    
    
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mTabChildGlobal->mMessageManager.get());
    mm->ReceiveMessage(static_cast<EventTarget*>(mTabChildGlobal),
                       aMessageName, false, &cloneData, nullptr, nullptr, nullptr);
}

bool
TabChild::RecvUpdateFrame(const FrameMetrics& aFrameMetrics)
{
  MOZ_ASSERT(aFrameMetrics.mScrollId != FrameMetrics::NULL_SCROLL_ID);

  if (aFrameMetrics.mIsRoot) {
    nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
    if (APZCCallbackHelper::HasValidPresShellId(utils, aFrameMetrics)) {
      return ProcessUpdateFrame(aFrameMetrics);
    }
  } else {
    
    
    nsCOMPtr<nsIContent> content = nsLayoutUtils::FindContentFor(
                                      aFrameMetrics.mScrollId);
    if (content) {
      FrameMetrics newSubFrameMetrics(aFrameMetrics);
      APZCCallbackHelper::UpdateSubFrame(content, newSubFrameMetrics);
      return true;
    }
  }

  
  
  
  return ProcessUpdateFrame(mLastRootMetrics);
}

bool
TabChild::ProcessUpdateFrame(const FrameMetrics& aFrameMetrics)
  {
    if (!mGlobal || !mTabChildGlobal) {
        return true;
    }

    nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());

    FrameMetrics newMetrics = aFrameMetrics;
    APZCCallbackHelper::UpdateRootFrame(utils, newMetrics);

    CSSRect cssCompositedRect = newMetrics.CalculateCompositedRectInCssPixels();
    
    
    
    nsCString data;
    data.AppendPrintf("{ \"x\" : %d", NS_lround(newMetrics.mScrollOffset.x));
    data.AppendPrintf(", \"y\" : %d", NS_lround(newMetrics.mScrollOffset.y));
    data.AppendLiteral(", \"viewport\" : ");
        data.AppendLiteral("{ \"width\" : ");
        data.AppendFloat(newMetrics.mViewport.width);
        data.AppendLiteral(", \"height\" : ");
        data.AppendFloat(newMetrics.mViewport.height);
        data.AppendLiteral(" }");
    data.AppendLiteral(", \"displayPort\" : ");
        data.AppendLiteral("{ \"x\" : ");
        data.AppendFloat(newMetrics.mDisplayPort.x);
        data.AppendLiteral(", \"y\" : ");
        data.AppendFloat(newMetrics.mDisplayPort.y);
        data.AppendLiteral(", \"width\" : ");
        data.AppendFloat(newMetrics.mDisplayPort.width);
        data.AppendLiteral(", \"height\" : ");
        data.AppendFloat(newMetrics.mDisplayPort.height);
        data.AppendLiteral(" }");
    data.AppendLiteral(", \"compositionBounds\" : ");
        data.AppendPrintf("{ \"x\" : %d", newMetrics.mCompositionBounds.x);
        data.AppendPrintf(", \"y\" : %d", newMetrics.mCompositionBounds.y);
        data.AppendPrintf(", \"width\" : %d", newMetrics.mCompositionBounds.width);
        data.AppendPrintf(", \"height\" : %d", newMetrics.mCompositionBounds.height);
        data.AppendLiteral(" }");
    data.AppendLiteral(", \"cssPageRect\" : ");
        data.AppendLiteral("{ \"x\" : ");
        data.AppendFloat(newMetrics.mScrollableRect.x);
        data.AppendLiteral(", \"y\" : ");
        data.AppendFloat(newMetrics.mScrollableRect.y);
        data.AppendLiteral(", \"width\" : ");
        data.AppendFloat(newMetrics.mScrollableRect.width);
        data.AppendLiteral(", \"height\" : ");
        data.AppendFloat(newMetrics.mScrollableRect.height);
        data.AppendLiteral(" }");
    data.AppendLiteral(", \"cssCompositedRect\" : ");
        data.AppendLiteral("{ \"width\" : ");
        data.AppendFloat(cssCompositedRect.width);
        data.AppendLiteral(", \"height\" : ");
        data.AppendFloat(cssCompositedRect.height);
        data.AppendLiteral(" }");
    data.AppendLiteral(" }");

    DispatchMessageManagerMessage(NS_LITERAL_STRING("Viewport:Change"), data);

    mLastRootMetrics = newMetrics;

    return true;
}

bool
TabChild::RecvHandleDoubleTap(const CSSIntPoint& aPoint)
{
    if (!mGlobal || !mTabChildGlobal) {
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
TabChild::RecvHandleSingleTap(const CSSIntPoint& aPoint)
{
  if (!mGlobal || !mTabChildGlobal) {
    return true;
  }

  LayoutDevicePoint currentPoint = CSSPoint(aPoint) * mWidget->GetDefaultScale();;

  int time = 0;
  DispatchSynthesizedMouseEvent(NS_MOUSE_MOVE, time, currentPoint);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_DOWN, time, currentPoint);
  DispatchSynthesizedMouseEvent(NS_MOUSE_BUTTON_UP, time, currentPoint);

  return true;
}

bool
TabChild::RecvHandleLongTap(const CSSIntPoint& aPoint)
{
  if (!mGlobal || !mTabChildGlobal) {
    return true;
  }

  mContextMenuHandled =
      DispatchMouseEvent(NS_LITERAL_STRING("contextmenu"), aPoint, 2, 1, 0, false,
                         nsIDOMMouseEvent::MOZ_SOURCE_TOUCH);

  return true;
}

bool
TabChild::RecvHandleLongTapUp(const CSSIntPoint& aPoint)
{
  if (mContextMenuHandled) {
    mContextMenuHandled = false;
    return true;
  }

  RecvHandleSingleTap(aPoint);
  return true;
}

bool
TabChild::RecvNotifyTransformBegin(const ViewID& aViewId)
{
  nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aViewId);
  if (sf) {
    nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(sf);
    if (scrollbarOwner) {
      scrollbarOwner->ScrollbarActivityStarted();
    }
  }
  return true;
}

bool
TabChild::RecvNotifyTransformEnd(const ViewID& aViewId)
{
  nsIScrollableFrame* sf = nsLayoutUtils::FindScrollableFrameFor(aViewId);
  if (sf) {
    nsIScrollbarOwner* scrollbarOwner = do_QueryFrame(sf);
    if (scrollbarOwner) {
      scrollbarOwner->ScrollbarActivityStopped();
    }
  }
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
  DispatchMouseEvent(aType, CSSPoint(aX, aY), aButton, aClickCount, aModifiers,
                     aIgnoreRootScrollFrame, nsIDOMMouseEvent::MOZ_SOURCE_UNKNOWN);
  return true;
}

bool
TabChild::RecvRealMouseEvent(const WidgetMouseEvent& event)
{
  WidgetMouseEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

bool
TabChild::RecvMouseWheelEvent(const WidgetWheelEvent& event)
{
  WidgetWheelEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

void
TabChild::DispatchSynthesizedMouseEvent(uint32_t aMsg, uint64_t aTime,
                                        const LayoutDevicePoint& aRefPoint)
{
  MOZ_ASSERT(aMsg == NS_MOUSE_MOVE || aMsg == NS_MOUSE_BUTTON_DOWN ||
             aMsg == NS_MOUSE_BUTTON_UP);

  WidgetMouseEvent event(true, aMsg, nullptr,
                         WidgetMouseEvent::eReal, WidgetMouseEvent::eNormal);
  event.refPoint = LayoutDeviceIntPoint(aRefPoint.x, aRefPoint.y);
  event.time = aTime;
  event.button = WidgetMouseEvent::eLeftButton;
  event.inputSource = nsIDOMMouseEvent::MOZ_SOURCE_TOUCH;
  if (aMsg != NS_MOUSE_MOVE) {
    event.clickCount = 1;
  }

  DispatchWidgetEvent(event);
}

static Touch*
GetTouchForIdentifier(const WidgetTouchEvent& aEvent, int32_t aId)
{
  for (uint32_t i = 0; i < aEvent.touches.Length(); ++i) {
    Touch* touch = static_cast<Touch*>(aEvent.touches[i].get());
    if (touch->mIdentifier == aId) {
      return touch;
    }
  }
  return nullptr;
}

void
TabChild::UpdateTapState(const WidgetTouchEvent& aEvent, nsEventStatus aStatus)
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

  if (aEvent.touches.Length() == 0) {
    return;
  }

  bool currentlyTrackingTouch = (mActivePointerId >= 0);
  if (aEvent.message == NS_TOUCH_START) {
    if (currentlyTrackingTouch || aEvent.touches.Length() > 1) {
      
      
      
      return;
    }
    if (aStatus == nsEventStatus_eConsumeNoDefault ||
        nsIPresShell::gPreventMouseEvents ||
        aEvent.mFlags.mMultipleActionsPrevented) {
      return;
    }

    Touch* touch = aEvent.touches[0];
    mGestureDownPoint = LayoutDevicePoint(touch->mRefPoint.x, touch->mRefPoint.y);
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
  Touch* trackedTouch = GetTouchForIdentifier(aEvent, mActivePointerId);
  if (!trackedTouch) {
    return;
  }

  LayoutDevicePoint currentPoint = LayoutDevicePoint(trackedTouch->mRefPoint.x, trackedTouch->mRefPoint.y);
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
  double scale;
  GetDefaultScale(&scale);
  if (scale < 0) {
    scale = 1;
  }

  MOZ_ASSERT(mTapHoldTimer && mActivePointerId >= 0);
  bool defaultPrevented = DispatchMouseEvent(NS_LITERAL_STRING("contextmenu"),
                                             mGestureDownPoint / CSSToLayoutDeviceScale(scale),
                                             2 ,
                                             1 ,
                                             0 ,
                                             false ,
                                             nsIDOMMouseEvent::MOZ_SOURCE_TOUCH);

  
  
  if (defaultPrevented) {
    CancelTapTracking();
  } else if (mTapHoldTimer) {
    mTapHoldTimer->Cancel();
    mTapHoldTimer = nullptr;
  }
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
TabChild::RecvRealTouchEvent(const WidgetTouchEvent& aEvent,
                             const ScrollableLayerGuid& aGuid)
{
  WidgetTouchEvent localEvent(aEvent);
  nsEventStatus status = DispatchWidgetEvent(localEvent);

  if (!IsAsyncPanZoomEnabled()) {
    UpdateTapState(localEvent, status);
    return true;
  }

  nsCOMPtr<nsPIDOMWindow> outerWindow = do_GetInterface(mWebNav);
  nsCOMPtr<nsPIDOMWindow> innerWindow = outerWindow->GetCurrentInnerWindow();

  if (!innerWindow || !innerWindow->HasTouchEventListeners()) {
    SendContentReceivedTouch(aGuid, false);
    return true;
  }

  bool isTouchPrevented = nsIPresShell::gPreventMouseEvents ||
                          localEvent.mFlags.mMultipleActionsPrevented;
  switch (aEvent.message) {
  case NS_TOUCH_START: {
    if (isTouchPrevented) {
      SendContentReceivedTouch(aGuid, isTouchPrevented);
    } else {
      mWaitingTouchListeners = true;
    }
    break;
  }

  case NS_TOUCH_MOVE:
  case NS_TOUCH_END:
  case NS_TOUCH_CANCEL: {
    if (mWaitingTouchListeners) {
      SendContentReceivedTouch(aGuid, isTouchPrevented);
      mWaitingTouchListeners = false;
    }
    break;
  }

  default:
    NS_WARNING("Unknown touch event type");
  }

  return true;
}

bool
TabChild::RecvRealTouchMoveEvent(const WidgetTouchEvent& aEvent,
                                 const ScrollableLayerGuid& aGuid)
{
  return RecvRealTouchEvent(aEvent, aGuid);
}

bool
TabChild::RecvRealKeyEvent(const WidgetKeyboardEvent& event)
{
  WidgetKeyboardEvent localEvent(event);
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
TabChild::RecvCompositionEvent(const WidgetCompositionEvent& event)
{
  WidgetCompositionEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

bool
TabChild::RecvTextEvent(const WidgetTextEvent& event)
{
  WidgetTextEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  IPC::ParamTraits<WidgetTextEvent>::Free(event);
  return true;
}

bool
TabChild::RecvSelectionEvent(const WidgetSelectionEvent& event)
{
  WidgetSelectionEvent localEvent(event);
  DispatchWidgetEvent(localEvent);
  return true;
}

nsEventStatus
TabChild::DispatchWidgetEvent(WidgetGUIEvent& event)
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
TabChild::AllocPDocumentRendererChild(const nsRect& documentRect,
                                      const mozilla::gfx::Matrix& transform,
                                      const nsString& bgcolor,
                                      const uint32_t& renderFlags,
                                      const bool& flushLayout,
                                      const nsIntSize& renderSize)
{
    return new DocumentRendererChild();
}

bool
TabChild::DeallocPDocumentRendererChild(PDocumentRendererChild* actor)
{
    delete actor;
    return true;
}

bool
TabChild::RecvPDocumentRendererConstructor(PDocumentRendererChild* actor,
                                           const nsRect& documentRect,
                                           const mozilla::gfx::Matrix& transform,
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
TabChild::AllocPContentDialogChild(const uint32_t&,
                                   const nsCString&,
                                   const nsCString&,
                                   const InfallibleTArray<int>&,
                                   const InfallibleTArray<nsString>&)
{
  return new ContentDialogChild();
}

bool
TabChild::DeallocPContentDialogChild(PContentDialogChild* aDialog)
{
  delete aDialog;
  return true;
}

PContentPermissionRequestChild*
TabChild::AllocPContentPermissionRequestChild(const nsCString& aType, const nsCString& aAccess, const IPC::Principal&)
{
  NS_RUNTIMEABORT("unused");
  return nullptr;
}

bool
TabChild::DeallocPContentPermissionRequestChild(PContentPermissionRequestChild* actor)
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
  nsCOMPtr<EventTarget> chromeHandler =
    do_QueryInterface(window->GetChromeEventHandler());
  NS_ENSURE_TRUE(chromeHandler, true);
  nsRefPtr<ContentListener> listener = new ContentListener(this);
  chromeHandler->AddEventListener(aType, listener, capture);
  return true;
}

POfflineCacheUpdateChild*
TabChild::AllocPOfflineCacheUpdateChild(const URIParams& manifestURI,
                                        const URIParams& documentURI,
                                        const bool& stickDocument)
{
  NS_RUNTIMEABORT("unused");
  return nullptr;
}

bool
TabChild::DeallocPOfflineCacheUpdateChild(POfflineCacheUpdateChild* actor)
{
  OfflineCacheUpdateChild* offlineCacheUpdate = static_cast<OfflineCacheUpdateChild*>(actor);
  delete offlineCacheUpdate;
  return true;
}

bool
TabChild::RecvLoadRemoteScript(const nsString& aURL, const bool& aRunInGlobalScope)
{
  if (!mGlobal && !InitTabChildGlobal())
    
    
    return true;

  LoadFrameScriptInternal(aURL, aRunInGlobalScope);
  return true;
}

bool
TabChild::RecvAsyncMessage(const nsString& aMessage,
                           const ClonedMessageData& aData,
                           const InfallibleTArray<CpowEntry>& aCpows,
                           const IPC::Principal& aPrincipal)
{
  if (mTabChildGlobal) {
    nsCOMPtr<nsIXPConnectJSObjectHolder> kungFuDeathGrip(GetGlobal());
    StructuredCloneData cloneData = UnpackClonedMessageDataForChild(aData);
    nsRefPtr<nsFrameMessageManager> mm =
      static_cast<nsFrameMessageManager*>(mTabChildGlobal->mMessageManager.get());
    CpowIdHolder cpows(static_cast<ContentChild*>(Manager())->GetCPOWManager(), aCpows);
    mm->ReceiveMessage(static_cast<EventTarget*>(mTabChildGlobal),
                       aMessage, false, &cloneData, &cpows, aPrincipal, nullptr);
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
    NS_NewDOMEvent(getter_AddRefs(event), mTabChildGlobal, nullptr, nullptr);
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
    mozilla::services::GetObserverService();

  observerService->RemoveObserver(this, BROWSER_ZOOM_TO_RECT);
  observerService->RemoveObserver(this, BEFORE_FIRST_PAINT);

  const InfallibleTArray<PIndexedDBChild*>& idbActors =
    ManagedPIndexedDBChild();
  for (uint32_t i = 0; i < idbActors.Length(); ++i) {
    static_cast<IndexedDBChild*>(idbActors[i])->Disconnect();
  }

  
  DestroyWindow();

  return Send__delete__(this);
}

bool
TabChild::RecvSetUpdateHitRegion(const bool& aEnabled)
{
    mUpdateHitRegion = aEnabled;
    return true;
}

PRenderFrameChild*
TabChild::AllocPRenderFrameChild()
{
    return new RenderFrameChild();
}

bool
TabChild::DeallocPRenderFrameChild(PRenderFrameChild* aFrame)
{
    delete aFrame;
    return true;
}

bool
TabChild::InitTabChildGlobal(FrameScriptLoading aScriptLoading)
{
  if (!mGlobal && !mTabChildGlobal) {
    nsCOMPtr<nsPIDOMWindow> window = do_GetInterface(mWebNav);
    NS_ENSURE_TRUE(window, false);
    nsCOMPtr<EventTarget> chromeHandler =
      do_QueryInterface(window->GetChromeEventHandler());
    NS_ENSURE_TRUE(chromeHandler, false);

    nsRefPtr<TabChildGlobal> scope = new TabChildGlobal(this);
    mTabChildGlobal = scope;

    nsISupports* scopeSupports = NS_ISUPPORTS_CAST(EventTarget*, scope);

    NS_NAMED_LITERAL_CSTRING(globalId, "outOfProcessTabChildGlobal");
    NS_ENSURE_TRUE(InitTabChildGlobalInternal(scopeSupports, globalId), false);

    scope->Init();

    nsCOMPtr<nsPIWindowRoot> root = do_QueryInterface(chromeHandler);
    NS_ENSURE_TRUE(root, false);
    root->SetParentTarget(scope);

    chromeHandler->AddEventListener(NS_LITERAL_STRING("DOMMetaAdded"), this, false);
  }

  if (aScriptLoading != DONT_LOAD_SCRIPTS && !mTriedBrowserInit) {
    mTriedBrowserInit = true;
    
    
    if (IsBrowserOrApp()) {
      RecvLoadRemoteScript(BROWSER_ELEMENT_CHILD_SCRIPT, true);
    }
  }

  return true;
}

bool
TabChild::InitRenderingState()
{
    static_cast<PuppetWidget*>(mWidget.get())->InitIMEState();

    uint64_t id;
    bool success;
    RenderFrameChild* remoteFrame =
        static_cast<RenderFrameChild*>(SendPRenderFrameConstructor());
    if (!remoteFrame) {
        NS_WARNING("failed to construct RenderFrame");
        return false;
    }
    SendInitRenderFrame(remoteFrame, &mScrolling, &mTextureFactoryIdentifier, &id, &success);
    if (!success) {
        NS_WARNING("failed to construct RenderFrame");
        PRenderFrameChild::Send__delete__(remoteFrame);
        return false;
    }

    PLayerTransactionChild* shadowManager = nullptr;
    if (id != 0) {
        
        
        PCompositorChild* compositorChild = CompositorChild::Get();
        if (!compositorChild) {
          NS_WARNING("failed to get CompositorChild instance");
          return false;
        }
        nsTArray<LayersBackend> backends;
        backends.AppendElement(mTextureFactoryIdentifier.mParentBackend);
        bool success;
        shadowManager =
            compositorChild->SendPLayerTransactionConstructor(backends,
                                                              id, &mTextureFactoryIdentifier, &success);
        if (!success) {
          NS_WARNING("failed to properly allocate layer transaction");
          return false;
        }
    } else {
        
        shadowManager = remoteFrame->SendPLayerTransactionConstructor();
    }

    if (!shadowManager) {
      NS_WARNING("failed to construct LayersChild");
      
      PRenderFrameChild::Send__delete__(remoteFrame);
      return false;
    }

    ShadowLayerForwarder* lf =
        mWidget->GetLayerManager(shadowManager, mTextureFactoryIdentifier.mParentBackend)
               ->AsShadowForwarder();
    NS_ABORT_IF_FALSE(lf && lf->HasShadowManager(),
                      "PuppetWidget should have shadow manager");
    lf->IdentifyTextureHost(mTextureFactoryIdentifier);
    ImageBridgeChild::IdentifyCompositorTextureHost(mTextureFactoryIdentifier);

    mRemoteFrame = remoteFrame;

    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();

    if (observerService) {
        observerService->AddObserver(this,
                                     BROWSER_ZOOM_TO_RECT,
                                     false);
        observerService->AddObserver(this,
                                     BEFORE_FIRST_PAINT,
                                     false);
    }

    
    sCpowsEnabled = BrowserTabsRemote();
    if (Preferences::GetBool("dom.ipc.cpows.force-enabled", false))
      sCpowsEnabled = true;

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
TabChild::GetDefaultScale(double* aScale)
{
    *aScale = -1.0;
    if (!mRemoteFrame) {
        return;
    }

    SendGetDefaultScale(aScale);
}

void
TabChild::NotifyPainted()
{
    
    
    
    if (UseDirectCompositor() &&
        (!mNotified || mTextureFactoryIdentifier.mParentBackend == LayersBackend::LAYERS_BASIC)) {
        mRemoteFrame->SendNotifyCompositorTransaction();
        mNotified = true;
    }
}

bool
TabChild::IsAsyncPanZoomEnabled()
{
    return mScrolling == ASYNC_PAN_ZOOM;
}

bool
TabChild::DispatchMouseEvent(const nsString&       aType,
                             const CSSPoint&       aPoint,
                             const int32_t&        aButton,
                             const int32_t&        aClickCount,
                             const int32_t&        aModifiers,
                             const bool&           aIgnoreRootScrollFrame,
                             const unsigned short& aInputSourceArg)
{
  nsCOMPtr<nsIDOMWindowUtils> utils(GetDOMWindowUtils());
  NS_ENSURE_TRUE(utils, true);
  
  bool defaultPrevented = false;
  utils->SendMouseEvent(aType, aPoint.x, aPoint.y, aButton, aClickCount, aModifiers,
                        aIgnoreRootScrollFrame, 0, aInputSourceArg, false, 4, &defaultPrevented);
  return defaultPrevented;
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

void
TabChild::UpdateHitRegion(const nsRegion& aRegion)
{
    mRemoteFrame->SendUpdateHitRegion(aRegion);
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

void
TabChild::SendRequestFocus(bool aCanFocus)
{
  PBrowserChild::SendRequestFocus(aCanFocus);
}

PIndexedDBChild*
TabChild::AllocPIndexedDBChild(
                            const nsCString& aGroup,
                            const nsCString& aASCIIOrigin, bool* )
{
  NS_NOTREACHED("Should never get here!");
  return nullptr;
}

bool
TabChild::DeallocPIndexedDBChild(PIndexedDBChild* aActor)
{
  delete aActor;
  return true;
}

bool
TabChild::DoSendBlockingMessage(JSContext* aCx,
                                const nsAString& aMessage,
                                const StructuredCloneData& aData,
                                JS::Handle<JSObject *> aCpows,
                                nsIPrincipal* aPrincipal,
                                InfallibleTArray<nsString>* aJSONRetVal,
                                bool aIsSync)
{
  ContentChild* cc = Manager();
  ClonedMessageData data;
  if (!BuildClonedMessageDataForChild(cc, aData, data)) {
    return false;
  }
  InfallibleTArray<CpowEntry> cpows;
  if (sCpowsEnabled) {
    if (!cc->GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
      return false;
    }
  }
  if (aIsSync) {
    return SendSyncMessage(nsString(aMessage), data, cpows, aPrincipal,
                           aJSONRetVal);
  }

  return CallRpcMessage(nsString(aMessage), data, cpows, aPrincipal,
                        aJSONRetVal);
}

bool
TabChild::DoSendAsyncMessage(JSContext* aCx,
                             const nsAString& aMessage,
                             const StructuredCloneData& aData,
                             JS::Handle<JSObject *> aCpows,
                             nsIPrincipal* aPrincipal)
{
  ContentChild* cc = Manager();
  ClonedMessageData data;
  if (!BuildClonedMessageDataForChild(cc, aData, data)) {
    return false;
  }
  InfallibleTArray<CpowEntry> cpows;
  if (sCpowsEnabled) {
    if (!cc->GetCPOWManager()->Wrap(aCx, aCpows, &cpows)) {
      return false;
    }
  }
  return SendAsyncMessage(nsString(aMessage), data, cpows,
                          aPrincipal);
}

TabChild*
TabChild::GetFrom(nsIPresShell* aPresShell)
{
  nsIDocument* doc = aPresShell->GetDocument();
  if (!doc) {
      return nullptr;
  }
  nsCOMPtr<nsIDocShell> docShell(doc->GetDocShell());
  return GetFrom(docShell);
}

NS_IMETHODIMP
TabChild::OnShowTooltip(int32_t aXCoords, int32_t aYCoords, const char16_t *aTipText)
{
    nsString str(aTipText);
    SendShowTooltip(aXCoords, aYCoords, str);
    return NS_OK;
}

NS_IMETHODIMP
TabChild::OnHideTooltip()
{
    SendHideTooltip();
    return NS_OK;
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
                                              MM_CHILD);
}

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(TabChildGlobal, nsDOMEventTargetHelper,
                                     mMessageManager)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(TabChildGlobal)
  NS_INTERFACE_MAP_ENTRY(nsIMessageListenerManager)
  NS_INTERFACE_MAP_ENTRY(nsIMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsISyncMessageSender)
  NS_INTERFACE_MAP_ENTRY(nsIContentFrameMessageManager)
  NS_INTERFACE_MAP_ENTRY(nsIScriptObjectPrincipal)
  NS_INTERFACE_MAP_ENTRY(nsIGlobalObject)
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
  return nsContentUtils::GetSafeJSContext();
}

nsIPrincipal*
TabChildGlobal::GetPrincipal()
{
  if (!mTabChild)
    return nullptr;
  return mTabChild->GetPrincipal();
}

JSObject*
TabChildGlobal::GetGlobalJSObject()
{
  NS_ENSURE_TRUE(mTabChild, nullptr);
  nsCOMPtr<nsIXPConnectJSObjectHolder> ref = mTabChild->GetGlobal();
  NS_ENSURE_TRUE(ref, nullptr);
  return ref->GetJSObject();
}

