




#include "MobileViewportManager.h"

#include "LayersLogging.h"
#include "nsViewManager.h"
#include "nsViewportInfo.h"

#define MVM_LOG(...)


NS_IMPL_ISUPPORTS(MobileViewportManager, nsIDOMEventListener, nsIObserver)

static const nsLiteralString DOM_META_ADDED = NS_LITERAL_STRING("DOMMetaAdded");
static const nsLiteralString FULL_ZOOM_CHANGE = NS_LITERAL_STRING("FullZoomChange");
static const nsLiteralCString BEFORE_FIRST_PAINT = NS_LITERAL_CSTRING("before-first-paint");

using namespace mozilla;
using namespace mozilla::layers;

MobileViewportManager::MobileViewportManager(nsIPresShell* aPresShell,
                                             nsIDocument* aDocument)
  : mDocument(aDocument)
  , mPresShell(aPresShell)
  , mIsFirstPaint(false)
{
  MOZ_ASSERT(mPresShell);
  MOZ_ASSERT(mDocument);

  MVM_LOG("%p: creating with presShell %p document %p\n", this, mPresShell, aDocument);

  if (nsCOMPtr<nsPIDOMWindow> window = mDocument->GetWindow()) {
    mEventTarget = window->GetChromeEventHandler();
  }
  if (mEventTarget) {
    mEventTarget->AddEventListener(DOM_META_ADDED, this, false);
    mEventTarget->AddEventListener(FULL_ZOOM_CHANGE, this, false);
  }

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  if (observerService) {
    observerService->AddObserver(this, BEFORE_FIRST_PAINT.Data(), false);
  }
}

MobileViewportManager::~MobileViewportManager()
{
}

void
MobileViewportManager::Destroy()
{
  MVM_LOG("%p: destroying\n", this);

  if (mEventTarget) {
    mEventTarget->RemoveEventListener(DOM_META_ADDED, this, false);
    mEventTarget->RemoveEventListener(FULL_ZOOM_CHANGE, this, false);
    mEventTarget = nullptr;
  }

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  if (observerService) {
    observerService->RemoveObserver(this, BEFORE_FIRST_PAINT.Data());
  }

  mDocument = nullptr;
  mPresShell = nullptr;
}

void
MobileViewportManager::RequestReflow()
{
  MVM_LOG("%p: got a reflow request\n", this);
  RefreshViewportSize(false);
}

NS_IMETHODIMP
MobileViewportManager::HandleEvent(nsIDOMEvent* event)
{
  nsAutoString type;
  event->GetType(type);

  if (type.Equals(DOM_META_ADDED)) {
    MVM_LOG("%p: got a dom-meta-added event\n", this);
    RefreshViewportSize(true);
  } else if (type.Equals(FULL_ZOOM_CHANGE)) {
    MVM_LOG("%p: got a full-zoom-change event\n", this);
    RefreshViewportSize(false);
  }
  return NS_OK;
}

NS_IMETHODIMP
MobileViewportManager::Observe(nsISupports* aSubject, const char* aTopic, const char16_t* aData)
{
  if (SameCOMIdentity(aSubject, mDocument) && BEFORE_FIRST_PAINT.EqualsASCII(aTopic)) {
    MVM_LOG("%p: got a before-first-paint event\n", this);
    mIsFirstPaint = true;
    RefreshViewportSize(false);
  }
  return NS_OK;
}

CSSToScreenScale
MobileViewportManager::UpdateResolution(const nsViewportInfo& aViewportInfo,
                                        const ScreenIntSize& aDisplaySize,
                                        const CSSSize& aViewport,
                                        const Maybe<float>& aDisplayWidthChangeRatio)
{
  CSSToLayoutDeviceScale cssToDev((float)nsPresContext::AppUnitsPerCSSPixel()
    / mPresShell->GetPresContext()->AppUnitsPerDevPixel());
  LayoutDeviceToLayerScale res(nsLayoutUtils::GetResolution(mPresShell));

#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID) || defined(MOZ_WIDGET_UIKIT)
  if (mIsFirstPaint) {
    CSSToScreenScale defaultZoom = aViewportInfo.GetDefaultZoom();
    MVM_LOG("%p: default zoom from viewport is %f\n", this, defaultZoom.scale);
    
    
    
    if (defaultZoom.scale < 0.01f) {
      defaultZoom = MaxScaleRatio(ScreenSize(aDisplaySize), aViewport);
      MVM_LOG("%p: Intrinsic computed zoom is %f\n", this, defaultZoom.scale);
    }
    MOZ_ASSERT(aViewportInfo.GetMinZoom() <= defaultZoom &&
      defaultZoom <= aViewportInfo.GetMaxZoom());

    CSSToParentLayerScale zoom = ViewTargetAs<ParentLayerPixel>(defaultZoom,
      PixelCastJustification::ScreenIsParentLayerForRoot);

    LayoutDeviceToLayerScale resolution = zoom / cssToDev * ParentLayerToLayerScale(1);
    MVM_LOG("%p: setting resolution %f\n", this, resolution.scale);
    nsLayoutUtils::SetResolutionAndScaleTo(mPresShell, resolution.scale);

    return defaultZoom;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (aDisplayWidthChangeRatio) {
    float cssViewportChangeRatio = (mMobileViewportSize.width == 0)
       ? 1.0f : aViewport.width / mMobileViewportSize.width;
    LayoutDeviceToLayerScale newRes(res.scale * aDisplayWidthChangeRatio.value()
      / cssViewportChangeRatio);
    MVM_LOG("%p: Old resolution was %f, changed by %f/%f to %f\n", this, res.scale,
      aDisplayWidthChangeRatio.value(), cssViewportChangeRatio, newRes.scale);
    nsLayoutUtils::SetResolutionAndScaleTo(mPresShell, newRes.scale);
    res = newRes;
  }
#endif

  return ViewTargetAs<ScreenPixel>(cssToDev * res / ParentLayerToLayerScale(1),
    PixelCastJustification::ScreenIsParentLayerForRoot);
}

void
MobileViewportManager::UpdateSPCSPS(const ScreenIntSize& aDisplaySize,
                                    const CSSToScreenScale& aZoom)
{
  ScreenSize compositionSize(aDisplaySize);
  ScreenMargin scrollbars =
    CSSMargin::FromAppUnits(
      nsLayoutUtils::ScrollbarAreaToExcludeFromCompositionBoundsFor(
        mPresShell->GetRootScrollFrame()))
    * CSSToScreenScale(1.0f); 
                              
  compositionSize.width -= scrollbars.LeftRight();
  compositionSize.height -= scrollbars.TopBottom();
  CSSSize compSize = compositionSize / aZoom;
  MVM_LOG("%p: Setting SPCSPS %s\n", this, Stringify(compSize).c_str());
  nsLayoutUtils::SetScrollPositionClampingScrollPortSize(mPresShell, compSize);
}

void
MobileViewportManager::UpdateDisplayPortMargins()
{
  if (nsIScrollableFrame* root = mPresShell->GetRootScrollFrameAsScrollable()) {
    nsLayoutUtils::CalculateAndSetDisplayPortMargins(root,
      nsLayoutUtils::RepaintMode::DoNotRepaint);
  }
}

void
MobileViewportManager::RefreshViewportSize(bool aForceAdjustResolution)
{
  
  
  
  
  
  
  
  
  
  
  
  
  

  Maybe<float> displayWidthChangeRatio;
  LayoutDeviceIntSize newDisplaySize;
  if (nsLayoutUtils::GetContentViewerSize(mPresShell->GetPresContext(), newDisplaySize)) {
    
    if (mDisplaySize.width > 0) {
      if (aForceAdjustResolution || mDisplaySize.width != newDisplaySize.width) {
        displayWidthChangeRatio = Some((float)newDisplaySize.width / (float)mDisplaySize.width);
      }
    } else if (aForceAdjustResolution) {
      displayWidthChangeRatio = Some(1.0f);
    }

    MVM_LOG("%p: Display width change ratio is %f\n", this, displayWidthChangeRatio.valueOr(0.0f));
    mDisplaySize = newDisplaySize;
  }

  MVM_LOG("%p: Computing CSS viewport using %d,%d\n", this,
    mDisplaySize.width, mDisplaySize.height);
  if (mDisplaySize.width == 0 || mDisplaySize.height == 0) {
    
    return;
  }

  ScreenIntSize displaySize = ViewAs<ScreenPixel>(
    mDisplaySize, PixelCastJustification::LayoutDeviceIsScreenForBounds);
  nsViewportInfo viewportInfo = nsContentUtils::GetViewportInfo(
    mDocument, displaySize);

  CSSSize viewport = viewportInfo.GetSize();
  MVM_LOG("%p: Computed CSS viewport %s\n", this, Stringify(viewport).c_str());

  if (!mIsFirstPaint && mMobileViewportSize == viewport) {
    
    return;
  }

  
  
  MVM_LOG("%p: Updating properties because %d || %d\n", this,
    mIsFirstPaint, mMobileViewportSize != viewport);

  CSSToScreenScale zoom = UpdateResolution(viewportInfo, displaySize, viewport,
    displayWidthChangeRatio);
  MVM_LOG("%p: New zoom is %f\n", this, zoom.scale);
  UpdateSPCSPS(displaySize, zoom);
  UpdateDisplayPortMargins();

  
  mIsFirstPaint = false;
  mMobileViewportSize = viewport;

  
  mPresShell->ResizeReflowIgnoreOverride(
    nsPresContext::CSSPixelsToAppUnits(viewport.width),
    nsPresContext::CSSPixelsToAppUnits(viewport.height));
}
