







#include "nsPluginFrame.h"

#include "gfx2DGlue.h"
#include "gfxMatrix.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/BasicEvents.h"
#ifdef XP_WIN

#include "mozilla/plugins/PluginMessageUtils.h"
#endif

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsWidgetsCID.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsString.h"
#include "nsGkAtoms.h"
#include "nsIPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIDOMElement.h"
#include "nsRenderingContext.h"
#include "npapi.h"
#include "nsIObjectLoadingContent.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsFocusManager.h"
#include "nsLayoutUtils.h"
#include "nsFrameManager.h"
#include "nsIObserverService.h"
#include "GeckoProfiler.h"
#include <algorithm>

#include "nsIObjectFrame.h"
#include "nsPluginNativeWindow.h"
#include "FrameLayerBuilder.h"

#include "ImageLayers.h"
#include "nsPluginInstanceOwner.h"

#ifdef XP_WIN
#include "gfxWindowsNativeDrawing.h"
#include "gfxWindowsSurface.h"
#endif

#include "Layers.h"
#include "ReadbackLayer.h"
#include "ImageContainer.h"


#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"

#ifdef XP_MACOSX
#include "gfxQuartzNativeDrawing.h"
#include "mozilla/gfx/QuartzSupport.h"
#endif

#ifdef MOZ_X11
#include "mozilla/X11Util.h"
using mozilla::DefaultXDisplay;
#endif

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#include "GLContext.h"
#endif

#include "mozilla/dom/TabChild.h"

#ifdef CreateEvent 
#undef CreateEvent
#endif

#ifdef PR_LOGGING 
static PRLogModuleInfo *
GetObjectFrameLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("nsPluginFrame");
  return sLog;
}
#endif 

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::layers;

class PluginBackgroundSink : public ReadbackSink {
public:
  PluginBackgroundSink(nsPluginFrame* aFrame, uint64_t aStartSequenceNumber)
    : mLastSequenceNumber(aStartSequenceNumber), mFrame(aFrame) {}
  ~PluginBackgroundSink()
  {
    if (mFrame) {
      mFrame->mBackgroundSink = nullptr;
    }
  }

  virtual void SetUnknown(uint64_t aSequenceNumber)
  {
    if (!AcceptUpdate(aSequenceNumber))
      return;
    mFrame->mInstanceOwner->SetBackgroundUnknown();
  }

  virtual already_AddRefed<gfxContext>
      BeginUpdate(const nsIntRect& aRect, uint64_t aSequenceNumber)
  {
    if (!AcceptUpdate(aSequenceNumber))
      return nullptr;
    return mFrame->mInstanceOwner->BeginUpdateBackground(aRect);
  }

  virtual void EndUpdate(gfxContext* aContext, const nsIntRect& aRect)
  {
    return mFrame->mInstanceOwner->EndUpdateBackground(aContext, aRect);
  }

  void Destroy() { mFrame = nullptr; }

protected:
  bool AcceptUpdate(uint64_t aSequenceNumber) {
    if (aSequenceNumber > mLastSequenceNumber && mFrame &&
        mFrame->mInstanceOwner) {
      mLastSequenceNumber = aSequenceNumber;
      return true;
    }
    return false;
  }

  uint64_t mLastSequenceNumber;
  nsPluginFrame* mFrame;
};

nsPluginFrame::nsPluginFrame(nsStyleContext* aContext)
  : nsPluginFrameSuper(aContext)
  , mReflowCallbackPosted(false)
{
  PR_LOG(GetObjectFrameLog(), PR_LOG_DEBUG,
         ("Created new nsPluginFrame %p\n", this));
}

nsPluginFrame::~nsPluginFrame()
{
  PR_LOG(GetObjectFrameLog(), PR_LOG_DEBUG,
         ("nsPluginFrame %p deleted\n", this));
}

NS_QUERYFRAME_HEAD(nsPluginFrame)
  NS_QUERYFRAME_ENTRY(nsPluginFrame)
  NS_QUERYFRAME_ENTRY(nsIObjectFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsPluginFrameSuper)

#ifdef ACCESSIBILITY
a11y::AccType
nsPluginFrame::AccessibleType()
{
  return a11y::ePluginType;
}

#ifdef XP_WIN
NS_IMETHODIMP nsPluginFrame::GetPluginPort(HWND *aPort)
{
  *aPort = (HWND) mInstanceOwner->GetPluginPort();
  return NS_OK;
}
#endif
#endif

void
nsPluginFrame::Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow)
{
  PR_LOG(GetObjectFrameLog(), PR_LOG_DEBUG,
         ("Initializing nsPluginFrame %p for content %p\n", this, aContent));

  nsPluginFrameSuper::Init(aContent, aParent, aPrevInFlow);
}

void
nsPluginFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mReflowCallbackPosted) {
    PresContext()->PresShell()->CancelReflowCallback(this);
  }

  
  nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(mContent));
  NS_ASSERTION(objContent, "Why not an object loading content?");

  
  
  
  if (mInstanceOwner) {
    mInstanceOwner->SetFrame(nullptr);
  }
  objContent->HasNewFrame(nullptr);

  if (mBackgroundSink) {
    mBackgroundSink->Destroy();
  }

  nsPluginFrameSuper::DestroyFrom(aDestructRoot);
}

 void
nsPluginFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (HasView()) {
    nsView* view = GetView();
    nsViewManager* vm = view->GetViewManager();
    if (vm) {
      nsViewVisibility visibility = 
        IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow;
      vm->SetViewVisibility(view, visibility);
    }
  }

  nsPluginFrameSuper::DidSetStyleContext(aOldStyleContext);
}

nsIAtom*
nsPluginFrame::GetType() const
{
  return nsGkAtoms::objectFrame; 
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsPluginFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("PluginFrame"), aResult);
}
#endif

nsresult
nsPluginFrame::PrepForDrawing(nsIWidget *aWidget)
{
  mWidget = aWidget;

  nsView* view = GetView();
  NS_ASSERTION(view, "Object frames must have views");  
  if (!view) {
    return NS_ERROR_FAILURE;
  }

  nsViewManager* viewMan = view->GetViewManager();
  
  
  viewMan->SetViewVisibility(view, nsViewVisibility_kHide);

  
  
  
  
  nsView* parentWithView;
  nsPoint origin;
  nsRect r(0, 0, mRect.width, mRect.height);

  GetOffsetFromView(origin, &parentWithView);
  viewMan->ResizeView(view, r);
  viewMan->MoveViewTo(view, origin.x, origin.y);

  nsPresContext* presContext = PresContext();
  nsRootPresContext* rpc = presContext->GetRootPresContext();
  if (!rpc) {
    return NS_ERROR_FAILURE;
  }

  if (mWidget) {
    
    nsIFrame* rootFrame = rpc->PresShell()->FrameManager()->GetRootFrame();
    nsIWidget* parentWidget = rootFrame->GetNearestWidget();
    if (!parentWidget || nsLayoutUtils::GetDisplayRootFrame(this) != rootFrame) {
      return NS_ERROR_FAILURE;
    }

    mInnerView = viewMan->CreateView(GetContentRectRelativeToSelf(), view);
    if (!mInnerView) {
      NS_ERROR("Could not create inner view");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    viewMan->InsertChild(view, mInnerView, nullptr, true);

    mWidget->SetParent(parentWidget);
    mWidget->Show(true);
    mWidget->Enable(true);

    
    
    
    
    
    nsAutoTArray<nsIWidget::Configuration,1> configurations;
    nsIWidget::Configuration* configuration = configurations.AppendElement();
    nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    configuration->mChild = mWidget;
    configuration->mBounds.width = NSAppUnitsToIntPixels(mRect.width, appUnitsPerDevPixel);
    configuration->mBounds.height = NSAppUnitsToIntPixels(mRect.height, appUnitsPerDevPixel);
    parentWidget->ConfigureChildren(configurations);

    nsRefPtr<nsDeviceContext> dx = viewMan->GetDeviceContext();
    mInnerView->AttachWidgetEventHandler(mWidget);

#ifdef XP_MACOSX
    
    
    
    if (parentWidget == GetNearestWidget()) {
      InvalidateFrame();
    }
#endif

    RegisterPluginForGeometryUpdates();

    
    
    
    
    for (nsIFrame* frame = this; frame; frame = frame->GetParent()) {
      nscolor bgcolor =
        frame->GetVisitedDependentColor(eCSSProperty_background_color);
      if (NS_GET_A(bgcolor) > 0) {  
        mWidget->SetBackgroundColor(bgcolor);
        break;
      }
    }
  } else {
    
    FixupWindow(GetContentRectRelativeToSelf().Size());
    RegisterPluginForGeometryUpdates();
  }

  if (!IsHidden()) {
    viewMan->SetViewVisibility(view, nsViewVisibility_kShow);
  }

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    accService->RecreateAccessible(PresContext()->PresShell(), mContent);
  }
#endif

  return NS_OK;
}

#define EMBED_DEF_WIDTH 240
#define EMBED_DEF_HEIGHT 200

 nscoord
nsPluginFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;

  if (!IsHidden(false)) {
    if (mContent->IsAnyOfHTMLElements(nsGkAtoms::applet,
                                      nsGkAtoms::embed)) {
      bool vertical = GetWritingMode().IsVertical();
      result = nsPresContext::CSSPixelsToAppUnits(
        vertical ? EMBED_DEF_HEIGHT : EMBED_DEF_WIDTH);
    }
  }

  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsPluginFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  return nsPluginFrame::GetMinISize(aRenderingContext);
}

void
nsPluginFrame::GetWidgetConfiguration(nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  if (!mWidget) {
    return;
  }

  if (!mWidget->GetParent()) {
    
    
    
    
    NS_ERROR("Plugin widgets registered for geometry updates should not be toplevel");
    return;
  }

  nsIWidget::Configuration* configuration = aConfigurations->AppendElement();
  configuration->mChild = mWidget;
  configuration->mBounds = mNextConfigurationBounds;
  configuration->mClipRegion = mNextConfigurationClipRegion;
#if defined(XP_WIN) || defined(MOZ_WIDGET_GTK)
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    configuration->mWindowID = (uintptr_t)mWidget->GetNativeData(NS_NATIVE_PLUGIN_PORT);
    configuration->mVisible = mWidget->IsVisible();
  }
#endif 
}

void
nsPluginFrame::GetDesiredSize(nsPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aMetrics)
{
  
  aMetrics.ClearSize();

  if (IsHidden(false)) {
    return;
  }

  aMetrics.Width() = aReflowState.ComputedWidth();
  aMetrics.Height() = aReflowState.ComputedHeight();

  
  if (mContent->IsAnyOfHTMLElements(nsGkAtoms::applet,
                                    nsGkAtoms::embed)) {
    if (aMetrics.Width() == NS_UNCONSTRAINEDSIZE) {
      aMetrics.Width() = clamped(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH),
                               aReflowState.ComputedMinWidth(),
                               aReflowState.ComputedMaxWidth());
    }
    if (aMetrics.Height() == NS_UNCONSTRAINEDSIZE) {
      aMetrics.Height() = clamped(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_HEIGHT),
                                aReflowState.ComputedMinHeight(),
                                aReflowState.ComputedMaxHeight());
    }

#if defined(MOZ_WIDGET_GTK)
    
    
    
    
    aMetrics.Height() = std::min(aPresContext->DevPixelsToAppUnits(INT16_MAX), aMetrics.Height());
    aMetrics.Width() = std::min(aPresContext->DevPixelsToAppUnits(INT16_MAX), aMetrics.Width());
#endif
  }

  
  
  
  if (aMetrics.Width() == NS_UNCONSTRAINEDSIZE) {
    aMetrics.Width() =
      (aReflowState.ComputedMinWidth() != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.ComputedMinWidth() : 0;
  }

  
  
  
  
  if (aMetrics.Height() == NS_UNCONSTRAINEDSIZE) {
    aMetrics.Height() =
      (aReflowState.ComputedMinHeight() != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.ComputedMinHeight() : 0;
  }

  
  
  
  
  
}

void
nsPluginFrame::Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsPluginFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  
  GetDesiredSize(aPresContext, aReflowState, aMetrics);
  aMetrics.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (!GetContent()->IsDoneAddingChildren()) {
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  
  if (aPresContext->Medium() == nsGkAtoms::print) {
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  nsRect r(0, 0, aMetrics.Width(), aMetrics.Height());
  r.Deflate(aReflowState.ComputedPhysicalBorderPadding());

  if (mInnerView) {
    nsViewManager* vm = mInnerView->GetViewManager();
    vm->MoveViewTo(mInnerView, r.x, r.y);
    vm->ResizeView(mInnerView, nsRect(nsPoint(0, 0), r.Size()), true);
  }

  FixupWindow(r.Size());
  if (!mReflowCallbackPosted) {
    mReflowCallbackPosted = true;
    aPresContext->PresShell()->PostReflowCallback(this);
  }

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
}



bool
nsPluginFrame::ReflowFinished()
{
  mReflowCallbackPosted = false;
  CallSetWindow();
  return true;
}

void
nsPluginFrame::ReflowCallbackCanceled()
{
  mReflowCallbackPosted = false;
}

void
nsPluginFrame::FixupWindow(const nsSize& aSize)
{
  nsPresContext* presContext = PresContext();

  if (!mInstanceOwner)
    return;

  NPWindow *window;
  mInstanceOwner->GetWindow(window);

  NS_ENSURE_TRUE_VOID(window);

  bool windowless = (window->type == NPWindowTypeDrawable);

  nsIntPoint origin = GetWindowOriginInPixels(windowless);

  
  double scaleFactor = 1.0;
  if (NS_FAILED(mInstanceOwner->GetContentsScaleFactor(&scaleFactor))) {
    scaleFactor = 1.0;
  }
  int intScaleFactor = ceil(scaleFactor);
  window->x = origin.x / intScaleFactor;
  window->y = origin.y / intScaleFactor;
  window->width = presContext->AppUnitsToDevPixels(aSize.width) / intScaleFactor;
  window->height = presContext->AppUnitsToDevPixels(aSize.height) / intScaleFactor;

#ifndef XP_MACOSX
  mInstanceOwner->UpdateWindowPositionAndClipRect(false);
#endif

  NotifyPluginReflowObservers();
}

nsresult
nsPluginFrame::CallSetWindow(bool aCheckIsHidden)
{
  NPWindow *win = nullptr;
 
  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<nsNPAPIPluginInstance> pi;
  if (!mInstanceOwner ||
      NS_FAILED(rv = mInstanceOwner->GetInstance(getter_AddRefs(pi))) ||
      !pi ||
      NS_FAILED(rv = mInstanceOwner->GetWindow(win)) || 
      !win)
    return rv;

  nsPluginNativeWindow *window = (nsPluginNativeWindow *)win;

  if (aCheckIsHidden && IsHidden())
    return NS_ERROR_FAILURE;

  
#ifdef XP_MACOSX
  mInstanceOwner->FixUpPluginWindow(nsPluginInstanceOwner::ePluginPaintEnable);
#endif
  window->window = mInstanceOwner->GetPluginPort();

  
  
  nsPresContext* presContext = PresContext();
  nsRootPresContext* rootPC = presContext->GetRootPresContext();
  if (!rootPC)
    return NS_ERROR_FAILURE;
  int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsIFrame* rootFrame = rootPC->PresShell()->FrameManager()->GetRootFrame();
  nsRect bounds = GetContentRectRelativeToSelf() + GetOffsetToCrossDoc(rootFrame);
  nsIntRect intBounds = bounds.ToNearestPixels(appUnitsPerDevPixel);

  
  
  LayoutDeviceIntPoint intOffset = GetRemoteTabChromeOffset();
  intBounds.x += intOffset.x;
  intBounds.y += intOffset.y;

  
  double scaleFactor = 1.0;
  if (NS_FAILED(mInstanceOwner->GetContentsScaleFactor(&scaleFactor))) {
    scaleFactor = 1.0;
  }
  size_t intScaleFactor = ceil(scaleFactor);
  window->x = intBounds.x / intScaleFactor;
  window->y = intBounds.y / intScaleFactor;
  window->width = intBounds.width / intScaleFactor;
  window->height = intBounds.height / intScaleFactor;

  
  
  nsRefPtr<nsPluginInstanceOwner> instanceOwnerRef(mInstanceOwner);

  
  
  
  if (mInstanceOwner->UseAsyncRendering()) {
    rv = pi->AsyncSetWindow(window);
  }
  else {
    rv = window->CallSetWindow(pi);
  }

  instanceOwnerRef->ReleasePluginPort(window->window);

  return rv;
}

void
nsPluginFrame::RegisterPluginForGeometryUpdates()
{
  nsRootPresContext* rpc = PresContext()->GetRootPresContext();
  NS_ASSERTION(rpc, "We should have a root pres context!");
  if (mRootPresContextRegisteredWith == rpc || !rpc) {
    
    
    return;
  }
  if (mRootPresContextRegisteredWith && mRootPresContextRegisteredWith != rpc) {
    
    
    UnregisterPluginForGeometryUpdates();
  }
  mRootPresContextRegisteredWith = rpc;
  mRootPresContextRegisteredWith->RegisterPluginForGeometryUpdates(mContent);
}

void
nsPluginFrame::UnregisterPluginForGeometryUpdates()
{
  if (!mRootPresContextRegisteredWith) {
    
    return;
  }
  mRootPresContextRegisteredWith->UnregisterPluginForGeometryUpdates(mContent);
  mRootPresContextRegisteredWith = nullptr;
}

void
nsPluginFrame::SetInstanceOwner(nsPluginInstanceOwner* aOwner)
{
  
  
  
  
  mInstanceOwner = aOwner;
  if (mInstanceOwner) {
    return;
  }
  UnregisterPluginForGeometryUpdates();
  if (mWidget && mInnerView) {
    mInnerView->DetachWidgetEventHandler(mWidget);
    
    
    nsIWidget* parent = mWidget->GetParent();
    if (parent) {
      nsTArray<nsIWidget::Configuration> configurations;
      nsIWidget::Configuration* configuration = configurations.AppendElement();
      configuration->mChild = mWidget;
      parent->ConfigureChildren(configurations);

      mWidget->Show(false);
      mWidget->Enable(false);
      mWidget->SetParent(nullptr);
    }
  }
}

bool
nsPluginFrame::IsFocusable(int32_t *aTabIndex, bool aWithMouse)
{
  if (aTabIndex)
    *aTabIndex = -1;
  return nsPluginFrameSuper::IsFocusable(aTabIndex, aWithMouse);
}

bool
nsPluginFrame::IsHidden(bool aCheckVisibilityStyle) const
{
  if (aCheckVisibilityStyle) {
    if (!StyleVisibility()->IsVisibleOrCollapsed())
      return true;    
  }

  
  if (mContent->IsHTMLElement(nsGkAtoms::embed)) {
    
    
    

    
    
    
    nsAutoString hidden;
    if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::hidden, hidden) &&
       (hidden.IsEmpty() ||
        (!hidden.LowerCaseEqualsLiteral("false") &&
         !hidden.LowerCaseEqualsLiteral("no") &&
         !hidden.LowerCaseEqualsLiteral("off")))) {
      return true;
    }
  }

  return false;
}

mozilla::LayoutDeviceIntPoint
nsPluginFrame::GetRemoteTabChromeOffset()
{
  LayoutDeviceIntPoint offset;
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    nsCOMPtr<nsIDOMWindow> window = do_QueryInterface(GetContent()->OwnerDoc()->GetWindow());
    if (window) {
      nsCOMPtr<nsIDOMWindow> topWindow;
      window->GetTop(getter_AddRefs(topWindow));
      if (topWindow) {
        dom::TabChild* tc = dom::TabChild::GetFrom(topWindow);
        if (tc) {
          LayoutDeviceIntPoint chromeOffset;
          tc->SendGetTabOffset(&chromeOffset);
          offset -= chromeOffset;
        }
      }
    }
  }
  return offset;
}

nsIntPoint
nsPluginFrame::GetWindowOriginInPixels(bool aWindowless)
{
  nsView * parentWithView;
  nsPoint origin(0,0);

  GetOffsetFromView(origin, &parentWithView);

  
  
  if (aWindowless && parentWithView) {
    nsPoint offsetToWidget;
    parentWithView->GetNearestWidget(&offsetToWidget);
    origin += offsetToWidget;
  }
  origin += GetContentRectRelativeToSelf().TopLeft();

  nsIntPoint pt(PresContext()->AppUnitsToDevPixels(origin.x),
                PresContext()->AppUnitsToDevPixels(origin.y));

  
  
  
  
  if (aWindowless) {
    mozilla::LayoutDeviceIntPoint lpt = GetRemoteTabChromeOffset();
    pt += nsIntPoint(lpt.x, lpt.y);
  }

  return pt;
}

void
nsPluginFrame::DidReflow(nsPresContext*            aPresContext,
                         const nsHTMLReflowState*  aReflowState,
                         nsDidReflowStatus         aStatus)
{
  
  
  if (aStatus == nsDidReflowStatus::FINISHED &&
      (GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(mContent));
    NS_ASSERTION(objContent, "Why not an object loading content?");
    objContent->HasNewFrame(this);
  }

  nsPluginFrameSuper::DidReflow(aPresContext, aReflowState, aStatus);

  
  
  if (aStatus != nsDidReflowStatus::FINISHED)
    return;

  if (HasView()) {
    nsView* view = GetView();
    nsViewManager* vm = view->GetViewManager();
    if (vm)
      vm->SetViewVisibility(view, IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow);
  }
}

 void
nsPluginFrame::PaintPrintPlugin(nsIFrame* aFrame, nsRenderingContext* aCtx,
                                const nsRect& aDirtyRect, nsPoint aPt)
{
  gfxContext* ctx = aCtx->ThebesContext();

  
  nsPoint pt = aPt + aFrame->GetContentRectRelativeToSelf().TopLeft();
  gfxPoint devPixelPt =
    nsLayoutUtils::PointToGfxPoint(pt, aFrame->PresContext()->AppUnitsPerDevPixel());

  gfxContextMatrixAutoSaveRestore autoSR(ctx);
  ctx->SetMatrix(ctx->CurrentMatrix().Translate(devPixelPt));

  

  static_cast<nsPluginFrame*>(aFrame)->PrintPlugin(*aCtx, aDirtyRect);
}







class nsDisplayPluginReadback : public nsDisplayItem {
public:
  nsDisplayPluginReadback(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayPluginReadback);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayPluginReadback() {
    MOZ_COUNT_DTOR(nsDisplayPluginReadback);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) override;

  NS_DISPLAY_DECL_NAME("PluginReadback", TYPE_PLUGIN_READBACK)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) override
  {
    return static_cast<nsPluginFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this, aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) override
  {
    return LAYER_ACTIVE;
  }
};

static nsRect
GetDisplayItemBounds(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem, nsIFrame* aFrame)
{
  
  return aFrame->GetContentRectRelativeToSelf() + aItem->ToReferenceFrame();
}

nsRect
nsDisplayPluginReadback::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = false;
  return GetDisplayItemBounds(aBuilder, this, mFrame);
}

#ifdef MOZ_WIDGET_ANDROID

class nsDisplayPluginVideo : public nsDisplayItem {
public:
  nsDisplayPluginVideo(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame, nsNPAPIPluginInstance::VideoInfo* aVideoInfo)
    : nsDisplayItem(aBuilder, aFrame), mVideoInfo(aVideoInfo)
  {
    MOZ_COUNT_CTOR(nsDisplayPluginVideo);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayPluginVideo() {
    MOZ_COUNT_DTOR(nsDisplayPluginVideo);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) override;

  NS_DISPLAY_DECL_NAME("PluginVideo", TYPE_PLUGIN_VIDEO)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) override
  {
    return static_cast<nsPluginFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this, aContainerParameters);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) override
  {
    return LAYER_ACTIVE;
  }

  nsNPAPIPluginInstance::VideoInfo* VideoInfo() { return mVideoInfo; }

private:
  nsNPAPIPluginInstance::VideoInfo* mVideoInfo;
};

nsRect
nsDisplayPluginVideo::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = false;
  return GetDisplayItemBounds(aBuilder, this, mFrame);
}

#endif

nsRect
nsDisplayPlugin::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = true;
  return GetDisplayItemBounds(aBuilder, this, mFrame);
}

void
nsDisplayPlugin::Paint(nsDisplayListBuilder* aBuilder,
                       nsRenderingContext* aCtx)
{
  nsPluginFrame* f = static_cast<nsPluginFrame*>(mFrame);
  bool snap;
  f->PaintPlugin(aBuilder, *aCtx, mVisibleRect, GetBounds(aBuilder, &snap));
}

bool
nsDisplayPlugin::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion)
{
  if (aBuilder->IsForPluginGeometry()) {
    nsPluginFrame* f = static_cast<nsPluginFrame*>(mFrame);
    if (!aBuilder->IsInTransform() || f->IsPaintedByGecko()) {
      
      
      nsRect rAncestor = nsLayoutUtils::TransformFrameRectToAncestor(f,
          f->GetContentRectRelativeToSelf(), ReferenceFrame());
      nscoord appUnitsPerDevPixel =
        ReferenceFrame()->PresContext()->AppUnitsPerDevPixel();
      f->mNextConfigurationBounds = rAncestor.ToNearestPixels(appUnitsPerDevPixel);

      nsRegion visibleRegion;
      visibleRegion.And(*aVisibleRegion, GetClippedBounds(aBuilder));
      
      visibleRegion.MoveBy(-ToReferenceFrame());

      f->mNextConfigurationClipRegion.Clear();
      nsRegionRectIterator iter(visibleRegion);
      for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
        nsRect rAncestor =
          nsLayoutUtils::TransformFrameRectToAncestor(f, *r, ReferenceFrame());
        nsIntRect rPixels = rAncestor.ToNearestPixels(appUnitsPerDevPixel)
            - f->mNextConfigurationBounds.TopLeft();
        if (!rPixels.IsEmpty()) {
          f->mNextConfigurationClipRegion.AppendElement(rPixels);
        }
      }
    }

    if (f->mInnerView) {
      
      
      
      f->mInnerView->CalcWidgetBounds(eWindowType_plugin);
    }
  }

  return nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion);
}

nsRegion
nsDisplayPlugin::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                 bool* aSnap)
{
  *aSnap = false;
  nsRegion result;
  nsPluginFrame* f = static_cast<nsPluginFrame*>(mFrame);
  if (!aBuilder->IsForPluginGeometry()) {
    nsIWidget* widget = f->GetWidget();
    if (widget) {
      
      
      
      
      
      
      
      
  	  return result;
    }
  }

  if (f->IsOpaque()) {
    nsRect bounds = GetBounds(aBuilder, aSnap);
    if (aBuilder->IsForPluginGeometry() ||
        (f->GetPaintedRect(this) + ToReferenceFrame()).Contains(bounds)) {
      
      result = bounds;
    }
  }

  return result;
}

nsresult
nsPluginFrame::PluginEventNotifier::Run() {
  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  obsSvc->NotifyObservers(nullptr, "plugin-changed-event", mEventType.get());
  return NS_OK;
}

void
nsPluginFrame::NotifyPluginReflowObservers()
{
  nsContentUtils::AddScriptRunner(new PluginEventNotifier(NS_LITERAL_STRING("reflow")));
}

void
nsPluginFrame::DidSetWidgetGeometry()
{
#if defined(XP_MACOSX)
  if (mInstanceOwner) {
    mInstanceOwner->FixUpPluginWindow(nsPluginInstanceOwner::ePluginPaintEnable);
  }
#else
  if (!mWidget && mInstanceOwner) {
    
    
    
    
    
    mInstanceOwner->UpdateWindowVisibility(
      nsLayoutUtils::IsPopup(nsLayoutUtils::GetDisplayRootFrame(this)) ||
      !mNextConfigurationBounds.IsEmpty());
  }
#endif
}

bool
nsPluginFrame::IsOpaque() const
{
#if defined(XP_MACOSX)
  return false;
#elif defined(MOZ_WIDGET_ANDROID)
  
  return false;
#else
  return !IsTransparentMode();
#endif
}

bool
nsPluginFrame::IsTransparentMode() const
{
#if defined(XP_MACOSX)
  return false;
#else
  if (!mInstanceOwner)
    return false;

  NPWindow *window = nullptr;
  mInstanceOwner->GetWindow(window);
  if (!window) {
    return false;
  }

  if (window->type != NPWindowTypeDrawable)
    return false;

  nsresult rv;
  nsRefPtr<nsNPAPIPluginInstance> pi;
  rv = mInstanceOwner->GetInstance(getter_AddRefs(pi));
  if (NS_FAILED(rv) || !pi)
    return false;

  bool transparent = false;
  pi->IsTransparent(&transparent);
  return transparent;
#endif
}

void
nsPluginFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  
  if (!IsVisibleOrCollapsedForPainting(aBuilder))
    return;

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  nsPresContext::nsPresContextType type = PresContext()->Type();

  
  if (type == nsPresContext::eContext_PrintPreview)
    return;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsPluginFrame");

#ifndef XP_MACOSX
  if (mWidget && aBuilder->IsInTransform()) {
    
    return;
  }
#endif

  if (aBuilder->IsForPainting() && mInstanceOwner) {
#ifdef XP_MACOSX
    mInstanceOwner->ResolutionMayHaveChanged();
    mInstanceOwner->WindowFocusMayHaveChanged();
#endif
    if (mInstanceOwner->UseAsyncRendering()) {
      NPWindow* window = nullptr;
      mInstanceOwner->GetWindow(window);
      bool isVisible = window && window->width > 0 && window->height > 0;
      if (isVisible && aBuilder->ShouldSyncDecodeImages()) {
#ifndef XP_MACOSX
        mInstanceOwner->UpdateWindowVisibility(true);
#endif
      }

      mInstanceOwner->NotifyPaintWaiter(aBuilder);
    }
  }

  DisplayListClipState::AutoClipContainingBlockDescendantsToContentBox
    clip(aBuilder, this);

  
  if (type == nsPresContext::eContext_Print) {
    aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayGeneric(aBuilder, this, PaintPrintPlugin, "PrintPlugin",
                       nsDisplayItem::TYPE_PRINT_PLUGIN));
  } else {
    LayerState state = GetLayerState(aBuilder, nullptr);
    if (state == LAYER_INACTIVE &&
        nsDisplayItem::ForceActiveLayers()) {
      state = LAYER_ACTIVE;
    }
    
#if !MOZ_WIDGET_ANDROID
    if (aBuilder->IsPaintingToWindow() &&
        state == LAYER_ACTIVE &&
        IsTransparentMode()) {
      aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayPluginReadback(aBuilder, this));
    }
#endif

#if MOZ_WIDGET_ANDROID
    if (aBuilder->IsPaintingToWindow() &&
        state == LAYER_ACTIVE) {

      nsTArray<nsNPAPIPluginInstance::VideoInfo*> videos;
      mInstanceOwner->GetVideos(videos);

      for (uint32_t i = 0; i < videos.Length(); i++) {
        aLists.Content()->AppendNewToTop(new (aBuilder)
          nsDisplayPluginVideo(aBuilder, this, videos[i]));
      }
    }
#endif

    aLists.Content()->AppendNewToTop(new (aBuilder)
      nsDisplayPlugin(aBuilder, this));
  }
}

void
nsPluginFrame::PrintPlugin(nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  nsCOMPtr<nsIObjectLoadingContent> obj(do_QueryInterface(mContent));
  if (!obj)
    return;

  nsIFrame* frame = nullptr;
  obj->GetPrintFrame(&frame);
  if (!frame)
    return;

  nsPresContext* presContext = PresContext();
  
  
  nsIObjectFrame* objectFrame = do_QueryFrame(frame);
  if (!objectFrame)
    objectFrame = GetNextObjectFrame(presContext,frame);
  if (!objectFrame)
    return;

  
  nsRefPtr<nsNPAPIPluginInstance> pi;
  if (NS_FAILED(objectFrame->GetPluginInstance(getter_AddRefs(pi))) || !pi)
    return;

  
  NPWindow window;
  window.window = nullptr;

  
  NPPrint npprint;
  npprint.mode = NP_EMBED;

  
  bool windowless = false;
  pi->IsWindowless(&windowless);
  window.type = windowless ? NPWindowTypeDrawable : NPWindowTypeWindow;

  window.clipRect.bottom = 0; window.clipRect.top = 0;
  window.clipRect.left = 0; window.clipRect.right = 0;


#if defined(XP_UNIX) || defined(XP_MACOSX)
  
  (void)window;
  (void)npprint;
#elif defined(XP_WIN)

  














  
  nsSize contentSize = GetContentRectRelativeToSelf().Size();
  window.x = 0;
  window.y = 0;
  window.width = presContext->AppUnitsToDevPixels(contentSize.width);
  window.height = presContext->AppUnitsToDevPixels(contentSize.height);

  gfxContext *ctx = aRenderingContext.ThebesContext();

  ctx->Save();

  
  ctx->NewPath();
  gfxRect r(window.x, window.y, window.width, window.height);
  ctx->Rectangle(r);
  ctx->Clip();

  gfxWindowsNativeDrawing nativeDraw(ctx, r);
  do {
    HDC dc = nativeDraw.BeginNativeDrawing();
    if (!dc)
      return;

    
    npprint.print.embedPrint.platformPrint = dc;
    npprint.print.embedPrint.window = window;
    
    pi->Print(&npprint);

    nativeDraw.EndNativeDrawing();
  } while (nativeDraw.ShouldRenderAgain());
  nativeDraw.PaintToContext();

  ctx->Restore();
#endif

  
  
  nsDidReflowStatus status = nsDidReflowStatus::FINISHED; 
  frame->DidReflow(presContext,
                   nullptr, status);  
}

nsRect
nsPluginFrame::GetPaintedRect(nsDisplayPlugin* aItem)
{
  if (!mInstanceOwner)
    return nsRect();
  nsRect r = GetContentRectRelativeToSelf();
  if (!mInstanceOwner->UseAsyncRendering())
    return r;

  nsIntSize size = mInstanceOwner->GetCurrentImageSize();
  nsPresContext* pc = PresContext();
  r.IntersectRect(r, nsRect(0, 0, pc->DevPixelsToAppUnits(size.width),
                                  pc->DevPixelsToAppUnits(size.height)));
  return r;
}

LayerState
nsPluginFrame::GetLayerState(nsDisplayListBuilder* aBuilder,
                             LayerManager* aManager)
{
  if (!mInstanceOwner)
    return LAYER_NONE;

#ifdef MOZ_WIDGET_ANDROID
  
  if (AndroidBridge::Bridge()->GetAPIVersion() >= 11)
    return LAYER_ACTIVE;
#endif

  if (!mInstanceOwner->UseAsyncRendering()) {
    return LAYER_NONE;
  }

  return LAYER_ACTIVE;
}

already_AddRefed<Layer>
nsPluginFrame::BuildLayer(nsDisplayListBuilder* aBuilder,
                          LayerManager* aManager,
                          nsDisplayItem* aItem,
                          const ContainerLayerParameters& aContainerParameters)
{
  if (!mInstanceOwner)
    return nullptr;

  NPWindow* window = nullptr;
  mInstanceOwner->GetWindow(window);
  if (!window)
    return nullptr;

  if (window->width <= 0 || window->height <= 0)
    return nullptr;

  
  double scaleFactor = 1.0;
  if (NS_FAILED(mInstanceOwner->GetContentsScaleFactor(&scaleFactor))) {
    scaleFactor = 1.0;
  }
  int intScaleFactor = ceil(scaleFactor);
  IntSize size(window->width * intScaleFactor, window->height * intScaleFactor);

  nsRect area = GetContentRectRelativeToSelf() + aItem->ToReferenceFrame();
  gfxRect r = nsLayoutUtils::RectToGfxRect(area, PresContext()->AppUnitsPerDevPixel());
  
  r.Round();
  nsRefPtr<Layer> layer =
    (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, aItem));

  if (aItem->GetType() == nsDisplayItem::TYPE_PLUGIN) {
    
    nsRefPtr<ImageContainer> container = mInstanceOwner->GetImageContainer();
    if (!container) {
      
      return nullptr;
    }

    if (!layer) {
      mInstanceOwner->NotifyPaintWaiter(aBuilder);
      
      layer = aManager->CreateImageLayer();
      if (!layer)
        return nullptr;
    }

    NS_ASSERTION(layer->GetType() == Layer::TYPE_IMAGE, "Bad layer type");
    ImageLayer* imglayer = static_cast<ImageLayer*>(layer.get());
#ifdef XP_MACOSX
    if (!mInstanceOwner->UseAsyncRendering()) {
      mInstanceOwner->DoCocoaEventDrawRect(r, nullptr);
    }
#endif

    imglayer->SetScaleToSize(size, ScaleMode::STRETCH);
    imglayer->SetContainer(container);
    GraphicsFilter filter =
      nsLayoutUtils::GetGraphicsFilterForFrame(this);
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    if (!aManager->IsCompositingCheap()) {
      
      filter = GraphicsFilter::FILTER_NEAREST;
    }
#endif
    imglayer->SetFilter(filter);

    layer->SetContentFlags(IsOpaque() ? Layer::CONTENT_OPAQUE : 0);
#ifdef MOZ_WIDGET_ANDROID
  } else if (aItem->GetType() == nsDisplayItem::TYPE_PLUGIN_VIDEO) {
    nsDisplayPluginVideo* videoItem = reinterpret_cast<nsDisplayPluginVideo*>(aItem);
    nsNPAPIPluginInstance::VideoInfo* videoInfo = videoItem->VideoInfo();

    nsRefPtr<ImageContainer> container = mInstanceOwner->GetImageContainerForVideo(videoInfo);
    if (!container)
      return nullptr;

    if (!layer) {
      
      layer = aManager->CreateImageLayer();
      if (!layer)
        return nullptr;
    }

    ImageLayer* imglayer = static_cast<ImageLayer*>(layer.get());
    imglayer->SetContainer(container);

    layer->SetContentFlags(IsOpaque() ? Layer::CONTENT_OPAQUE : 0);

    
    r.MoveBy(videoInfo->mDimensions.TopLeft());
    size.width = videoInfo->mDimensions.width;
    size.height = videoInfo->mDimensions.height;
#endif
  } else {
    NS_ASSERTION(aItem->GetType() == nsDisplayItem::TYPE_PLUGIN_READBACK,
                 "Unknown item type");
    MOZ_ASSERT(!IsOpaque(), "Opaque plugins don't use backgrounds");

    if (!layer) {
      layer = aManager->CreateReadbackLayer();
      if (!layer)
        return nullptr;
    }
    NS_ASSERTION(layer->GetType() == Layer::TYPE_READBACK, "Bad layer type");

    ReadbackLayer* readback = static_cast<ReadbackLayer*>(layer.get());
    if (readback->GetSize() != size) {
      
      
      readback->SetSink(nullptr);
      readback->SetSize(size);

      if (mBackgroundSink) {
        
        
        
        
        mBackgroundSink->Destroy();
      }
      mBackgroundSink =
        new PluginBackgroundSink(this,
                                 readback->AllocateSequenceNumber());
      readback->SetSink(mBackgroundSink);
      
      
    }
  }

  
  gfxPoint p = r.TopLeft() + aContainerParameters.mOffset;
  Matrix transform = Matrix::Translation(p.x, p.y);

  layer->SetBaseTransform(Matrix4x4::From2D(transform));
  return layer.forget();
}

void
nsPluginFrame::PaintPlugin(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, const nsRect& aPluginRect)
{
#if defined(XP_MACOSX)
  DrawTarget& aDrawTarget = *aRenderingContext.GetDrawTarget();
#endif

#if defined(MOZ_WIDGET_ANDROID)
  if (mInstanceOwner) {
    gfxRect frameGfxRect =
      PresContext()->AppUnitsToGfxUnits(aPluginRect);
    gfxRect dirtyGfxRect =
      PresContext()->AppUnitsToGfxUnits(aDirtyRect);

    gfxContext* ctx = aRenderingContext.ThebesContext();

    mInstanceOwner->Paint(ctx, frameGfxRect, dirtyGfxRect);
    return;
  }
#endif

  
#if defined(XP_MACOSX)
  
  if (mInstanceOwner) {
    if (mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreGraphics ||
        mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreAnimation ||
        mInstanceOwner->GetDrawingModel() == 
                                  NPDrawingModelInvalidatingCoreAnimation) {
      int32_t appUnitsPerDevPixel = PresContext()->AppUnitsPerDevPixel();
      
      
      nsIntRect contentPixels = aPluginRect.ToNearestPixels(appUnitsPerDevPixel);
      nsIntRect dirtyPixels = aDirtyRect.ToOutsidePixels(appUnitsPerDevPixel);
      nsIntRect clipPixels;
      clipPixels.IntersectRect(contentPixels, dirtyPixels);

      
      if (clipPixels.IsEmpty())
        return;

      gfxRect nativeClipRect(clipPixels.x, clipPixels.y,
                             clipPixels.width, clipPixels.height);
      gfxContext* ctx = aRenderingContext.ThebesContext();

      gfxContextAutoSaveRestore save(ctx);
      ctx->NewPath();
      ctx->Rectangle(nativeClipRect);
      ctx->Clip();
      gfxPoint offset(contentPixels.x, contentPixels.y);
      ctx->SetMatrix(
        ctx->CurrentMatrix().Translate(offset));

      gfxQuartzNativeDrawing nativeDrawing(aDrawTarget,
                                           ToRect(nativeClipRect - offset));

      CGContextRef cgContext = nativeDrawing.BeginNativeDrawing();
      if (!cgContext) {
        NS_WARNING("null CGContextRef during PaintPlugin");
        return;
      }

      nsRefPtr<nsNPAPIPluginInstance> inst;
      GetPluginInstance(getter_AddRefs(inst));
      if (!inst) {
        NS_WARNING("null plugin instance during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      NPWindow* window;
      mInstanceOwner->GetWindow(window);
      if (!window) {
        NS_WARNING("null plugin window during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }
      NP_CGContext* cgPluginPortCopy =
                static_cast<NP_CGContext*>(mInstanceOwner->GetPluginPortCopy());
      if (!cgPluginPortCopy) {
        NS_WARNING("null plugin port copy during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }

      mInstanceOwner->BeginCGPaint();
      if (mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreAnimation ||
          mInstanceOwner->GetDrawingModel() == 
                                   NPDrawingModelInvalidatingCoreAnimation) {
        
        mInstanceOwner->RenderCoreAnimation(cgContext, window->width, window->height);
      } else {
        mInstanceOwner->Paint(nativeClipRect - offset, cgContext);
      }
      mInstanceOwner->EndCGPaint();

      nativeDrawing.EndNativeDrawing();
    } else {
      gfxContext* ctx = aRenderingContext.ThebesContext();

      
      gfxPoint devPixelPt =
        nsLayoutUtils::PointToGfxPoint(aPluginRect.TopLeft(),
                                       PresContext()->AppUnitsPerDevPixel());

      gfxContextMatrixAutoSaveRestore autoSR(ctx);
      ctx->SetMatrix(ctx->CurrentMatrix().Translate(devPixelPt));

      

      
      gfxRect tmpRect(0, 0, 0, 0);
      mInstanceOwner->Paint(tmpRect, nullptr);
    }
  }
#elif defined(MOZ_X11)
  if (mInstanceOwner) {
    NPWindow *window;
    mInstanceOwner->GetWindow(window);
    if (window->type == NPWindowTypeDrawable) {
      gfxRect frameGfxRect =
        PresContext()->AppUnitsToGfxUnits(aPluginRect);
      gfxRect dirtyGfxRect =
        PresContext()->AppUnitsToGfxUnits(aDirtyRect);
      gfxContext* ctx = aRenderingContext.ThebesContext();

      mInstanceOwner->Paint(ctx, frameGfxRect, dirtyGfxRect);
    }
  }
#elif defined(XP_WIN)
  nsRefPtr<nsNPAPIPluginInstance> inst;
  GetPluginInstance(getter_AddRefs(inst));
  if (inst) {
    gfxRect frameGfxRect =
      PresContext()->AppUnitsToGfxUnits(aPluginRect);
    gfxRect dirtyGfxRect =
      PresContext()->AppUnitsToGfxUnits(aDirtyRect);
    gfxContext *ctx = aRenderingContext.ThebesContext();
    gfxMatrix currentMatrix = ctx->CurrentMatrix();

    if (ctx->UserToDevicePixelSnapped(frameGfxRect, false)) {
      dirtyGfxRect = ctx->UserToDevice(dirtyGfxRect);
      ctx->SetMatrix(gfxMatrix());
    }
    dirtyGfxRect.RoundOut();

    
    NPWindow *window;
    mInstanceOwner->GetWindow(window);

    if (window->type == NPWindowTypeDrawable) {
      
      nsPoint origin;

      gfxWindowsNativeDrawing nativeDraw(ctx, frameGfxRect);
      if (nativeDraw.IsDoublePass()) {
        
        
        
        NPEvent pluginEvent;
        pluginEvent.event = plugins::DoublePassRenderingEvent();
        pluginEvent.wParam = 0;
        pluginEvent.lParam = 0;
        if (pluginEvent.event)
          inst->HandleEvent(&pluginEvent, nullptr);
      }
      do {
        HDC hdc = nativeDraw.BeginNativeDrawing();
        if (!hdc)
          return;

        RECT dest;
        nativeDraw.TransformToNativeRect(frameGfxRect, dest);
        RECT dirty;
        nativeDraw.TransformToNativeRect(dirtyGfxRect, dirty);

        window->window = hdc;
        window->x = dest.left;
        window->y = dest.top;
        window->clipRect.left = 0;
        window->clipRect.top = 0;
        
        window->clipRect.right = window->width;
        window->clipRect.bottom = window->height;

        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        

        nsIntPoint origin = GetWindowOriginInPixels(true);
        nsIntRect winlessRect = nsIntRect(origin, nsIntSize(window->width, window->height));

        if (!mWindowlessRect.IsEqualEdges(winlessRect)) {
          mWindowlessRect = winlessRect;

          WINDOWPOS winpos;
          memset(&winpos, 0, sizeof(winpos));
          winpos.x = mWindowlessRect.x;
          winpos.y = mWindowlessRect.y;
          winpos.cx = mWindowlessRect.width;
          winpos.cy = mWindowlessRect.height;

          
          NPEvent pluginEvent;
          pluginEvent.event = WM_WINDOWPOSCHANGED;
          pluginEvent.wParam = 0;
          pluginEvent.lParam = (LPARAM)&winpos;
          inst->HandleEvent(&pluginEvent, nullptr);
        }

        inst->SetWindow(window);

        mInstanceOwner->Paint(dirty, hdc);
        nativeDraw.EndNativeDrawing();
      } while (nativeDraw.ShouldRenderAgain());
      nativeDraw.PaintToContext();
    }

    ctx->SetMatrix(currentMatrix);
  }
#endif
}

nsresult
nsPluginFrame::HandleEvent(nsPresContext* aPresContext,
                           WidgetGUIEvent* anEvent,
                           nsEventStatus* anEventStatus)
{
  NS_ENSURE_ARG_POINTER(anEvent);
  NS_ENSURE_ARG_POINTER(anEventStatus);
  nsresult rv = NS_OK;

  if (!mInstanceOwner)
    return NS_ERROR_NULL_POINTER;

  mInstanceOwner->ConsiderNewEventloopNestingLevel();

  if (anEvent->message == NS_PLUGIN_ACTIVATE) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    nsCOMPtr<nsIDOMElement> elem = do_QueryInterface(GetContent());
    if (fm && elem)
      return fm->SetFocus(elem, 0);
  }
  else if (anEvent->message == NS_PLUGIN_FOCUS) {
    nsIFocusManager* fm = nsFocusManager::GetFocusManager();
    if (fm)
      return fm->FocusPlugin(GetContent());
  }

  if (mInstanceOwner->SendNativeEvents() &&
      anEvent->IsNativeEventDelivererForPlugin()) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    
    
    return rv;
  }

#ifdef XP_WIN
  rv = nsPluginFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
  return rv;
#endif

#ifdef XP_MACOSX
  
  if ((anEvent->message == NS_MOUSE_ENTER ||
       anEvent->message == NS_WHEEL_WHEEL) &&
      mInstanceOwner->GetEventModel() == NPEventModelCocoa) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    
    
    return rv;
  }

  
  
  
  
  if (anEvent->message == NS_MOUSE_BUTTON_DOWN) {
    nsIPresShell::SetCapturingContent(GetContent(), CAPTURE_IGNOREALLOWED);
  }
#endif

  rv = nsPluginFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);

  
  

#ifdef XP_MACOSX
  if (anEvent->message == NS_MOUSE_BUTTON_UP) {
    nsIPresShell::SetCapturingContent(nullptr, 0);
  }
#endif

  return rv;
}

nsresult
nsPluginFrame::GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance)
{
  *aPluginInstance = nullptr;

  if (!mInstanceOwner) {
    return NS_OK;
  }

  return mInstanceOwner->GetInstance(aPluginInstance);
}

nsresult
nsPluginFrame::GetCursor(const nsPoint& aPoint, nsIFrame::Cursor& aCursor)
{
  if (!mInstanceOwner) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<nsNPAPIPluginInstance> inst;
  mInstanceOwner->GetInstance(getter_AddRefs(inst));
  if (!inst) {
    return NS_ERROR_FAILURE;
  }

  bool useDOMCursor = static_cast<nsNPAPIPluginInstance*>(inst.get())->UsesDOMForCursor();
  if (!useDOMCursor) {
    return NS_ERROR_FAILURE;
  }

  return nsPluginFrameSuper::GetCursor(aPoint, aCursor);
}

void
nsPluginFrame::SetIsDocumentActive(bool aIsActive)
{
  if (mInstanceOwner) {
    mInstanceOwner->UpdateDocumentActiveState(aIsActive);
  }
}


nsIObjectFrame *
nsPluginFrame::GetNextObjectFrame(nsPresContext* aPresContext, nsIFrame* aRoot)
{
  nsIFrame* child = aRoot->GetFirstPrincipalChild();

  while (child) {
    nsIObjectFrame* outFrame = do_QueryFrame(child);
    if (outFrame) {
      nsRefPtr<nsNPAPIPluginInstance> pi;
      outFrame->GetPluginInstance(getter_AddRefs(pi));  
      if (pi)
        return outFrame;
    }

    outFrame = GetNextObjectFrame(aPresContext, child);
    if (outFrame)
      return outFrame;
    child = child->GetNextSibling();
  }

  return nullptr;
}

 void
nsPluginFrame::BeginSwapDocShells(nsISupports* aSupports, void*)
{
  NS_PRECONDITION(aSupports, "");
  nsCOMPtr<nsIContent> content(do_QueryInterface(aSupports));
  if (!content) {
    return;
  }

  
  
  nsIObjectFrame* obj = do_QueryFrame(content->GetPrimaryFrame());
  if (!obj)
    return;

  nsPluginFrame* objectFrame = static_cast<nsPluginFrame*>(obj);
  NS_ASSERTION(!objectFrame->mWidget || objectFrame->mWidget->GetParent(),
               "Plugin windows must not be toplevel");
  objectFrame->UnregisterPluginForGeometryUpdates();
}

 void
nsPluginFrame::EndSwapDocShells(nsISupports* aSupports, void*)
{
  NS_PRECONDITION(aSupports, "");
  nsCOMPtr<nsIContent> content(do_QueryInterface(aSupports));
  if (!content) {
    return;
  }

  
  
  nsIObjectFrame* obj = do_QueryFrame(content->GetPrimaryFrame());
  if (!obj)
    return;

  nsPluginFrame* objectFrame = static_cast<nsPluginFrame*>(obj);
  nsRootPresContext* rootPC = objectFrame->PresContext()->GetRootPresContext();
  NS_ASSERTION(rootPC, "unable to register the plugin frame");
  nsIWidget* widget = objectFrame->mWidget;
  if (widget) {
    
    nsIWidget* parent =
      rootPC->PresShell()->GetRootFrame()->GetNearestWidget();
    widget->SetParent(parent);
    nsWeakFrame weakFrame(objectFrame);
    objectFrame->CallSetWindow();
    if (!weakFrame.IsAlive()) {
      return;
    }
  }

  objectFrame->RegisterPluginForGeometryUpdates();
}

nsIFrame*
NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPluginFrame(aContext);
}

bool
nsPluginFrame::IsPaintedByGecko() const
{
#ifdef XP_MACOSX
  return true;
#else
  return !mWidget;
#endif
}

NS_IMPL_FRAMEARENA_HELPERS(nsPluginFrame)
