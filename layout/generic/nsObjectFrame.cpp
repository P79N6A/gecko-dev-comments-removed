







#include "mozilla/plugins/PluginMessageUtils.h"

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsWidgetsCID.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMDragEvent.h"
#include "nsPluginHost.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "prmem.h"
#include "nsGkAtoms.h"
#include "nsIAppShell.h"
#include "nsIDocument.h"
#include "nsINodeInfo.h"
#include "nsIURL.h"
#include "nsNetUtil.h"
#include "nsIPluginInstanceOwner.h"
#include "nsNPAPIPluginInstance.h"
#include "nsIPluginTagInfo.h"
#include "plstr.h"
#include "nsILinkHandler.h"
#include "nsIScrollPositionListener.h"
#include "nsITimer.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDocShellTreeOwner.h"
#include "nsDocShellCID.h"
#include "nsIWebBrowserChrome.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMHTMLObjectElement.h"
#include "nsIDOMHTMLEmbedElement.h"
#include "nsIDOMHTMLAppletElement.h"
#include "nsIDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsIDocumentEncoder.h"
#include "nsXPIDLString.h"
#include "nsIDOMRange.h"
#include "nsIPluginWidget.h"
#include "nsGUIEvent.h"
#include "nsRenderingContext.h"
#include "npapi.h"
#include "nsTransform2D.h"
#include "nsIImageLoadingContent.h"
#include "nsIObjectLoadingContent.h"
#include "nsPIDOMWindow.h"
#include "nsContentUtils.h"
#include "nsDisplayList.h"
#include "nsAttrName.h"
#include "nsDataHashtable.h"
#include "nsDOMClassInfo.h"
#include "nsFocusManager.h"
#include "nsLayoutUtils.h"
#include "nsFrameManager.h"
#include "nsComponentManagerUtils.h"
#include "nsIObserverService.h"
#include "nsIScrollableFrame.h"
#include "mozilla/Preferences.h"
#include "sampler.h"


#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsIXPCScriptable.h"
#include "nsIClassInfo.h"
#include "nsIDOMClientRect.h"

#include "nsObjectFrame.h"
#include "nsIObjectFrame.h"
#include "nsPluginNativeWindow.h"
#include "nsIPluginDocument.h"
#include "FrameLayerBuilder.h"

#include "nsThreadUtils.h"

#include "gfxContext.h"
#include "gfxPlatform.h"

#ifdef XP_WIN
#include "gfxWindowsNativeDrawing.h"
#include "gfxWindowsSurface.h"
#endif

#include "gfxImageSurface.h"
#include "gfxUtils.h"
#include "Layers.h"
#include "ReadbackLayer.h"


#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG 1 /* Allow logging in the release build */
#endif 
#include "prlog.h"

#include <errno.h>

#include "nsContentCID.h"
static NS_DEFINE_CID(kAppShellCID, NS_APPSHELL_CID);

#ifdef XP_MACOSX
#include "gfxQuartzNativeDrawing.h"
#include "nsPluginUtilsOSX.h"
#include "nsCoreAnimationSupport.h"
#endif

#ifdef MOZ_WIDGET_GTK2
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include "gfxXlibNativeRenderer.h"
#endif

#ifdef MOZ_X11
#include "mozilla/X11Util.h"
using mozilla::DefaultXDisplay;
#endif

#ifdef XP_WIN
#include <wtypes.h>
#include <winuser.h>
#endif

#ifdef XP_OS2
#define INCL_PM
#define INCL_GPI
#include <os2.h>
#include "gfxOS2Surface.h"
#endif

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
#include "GLContext.h"
#endif

#ifdef CreateEvent 
#undef CreateEvent
#endif

#ifdef PR_LOGGING 
static PRLogModuleInfo *nsObjectFrameLM = PR_NewLogModule("nsObjectFrame");
#endif 

#if defined(XP_MACOSX) && !defined(NP_NO_CARBON)

#define MAC_CARBON_PLUGINS







extern "C" {
  #if !defined(__QUICKDRAWAPI__)
  extern void SetRect(
    Rect * r,
    short  left,
    short  top,
    short  right,
    short  bottom)
    __attribute__((weak_import));
  #endif 

  #if !defined(__QDOFFSCREEN__)
  extern QDErr NewGWorldFromPtr(
    GWorldPtr *   offscreenGWorld,
    UInt32        PixelFormat,
    const Rect *  boundsRect,
    CTabHandle    cTable,                
    GDHandle      aGDevice,              
    GWorldFlags   flags,
    Ptr           newBuffer,
    SInt32        rowBytes)
    __attribute__((weak_import));
  extern void DisposeGWorld(GWorldPtr offscreenGWorld)
    __attribute__((weak_import));
  #endif 
}

#endif 

using namespace mozilla;
using namespace mozilla::plugins;
using namespace mozilla::layers;

class PluginBackgroundSink : public ReadbackSink {
public:
  PluginBackgroundSink(nsObjectFrame* aFrame, PRUint64 aStartSequenceNumber)
    : mLastSequenceNumber(aStartSequenceNumber), mFrame(aFrame) {}
  ~PluginBackgroundSink()
  {
    if (mFrame) {
      mFrame->mBackgroundSink = nsnull;
    }
  }

  virtual void SetUnknown(PRUint64 aSequenceNumber)
  {
    if (!AcceptUpdate(aSequenceNumber))
      return;
    mFrame->mInstanceOwner->SetBackgroundUnknown();
  }

  virtual already_AddRefed<gfxContext>
      BeginUpdate(const nsIntRect& aRect, PRUint64 aSequenceNumber)
  {
    if (!AcceptUpdate(aSequenceNumber))
      return nsnull;
    return mFrame->mInstanceOwner->BeginUpdateBackground(aRect);
  }

  virtual void EndUpdate(gfxContext* aContext, const nsIntRect& aRect)
  {
    return mFrame->mInstanceOwner->EndUpdateBackground(aContext, aRect);
  }

  void Destroy() { mFrame = nsnull; }

protected:
  bool AcceptUpdate(PRUint64 aSequenceNumber) {
    if (aSequenceNumber > mLastSequenceNumber && mFrame &&
        mFrame->mInstanceOwner) {
      mLastSequenceNumber = aSequenceNumber;
      return true;
    }
    return false;
  }

  PRUint64 mLastSequenceNumber;
  nsObjectFrame* mFrame;
};

nsObjectFrame::nsObjectFrame(nsStyleContext* aContext)
  : nsObjectFrameSuper(aContext)
  , mReflowCallbackPosted(false)
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Created new nsObjectFrame %p\n", this));
}

nsObjectFrame::~nsObjectFrame()
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("nsObjectFrame %p deleted\n", this));
}

NS_QUERYFRAME_HEAD(nsObjectFrame)
  NS_QUERYFRAME_ENTRY(nsIObjectFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsObjectFrameSuper)

#ifdef ACCESSIBILITY
already_AddRefed<Accessible>
nsObjectFrame::CreateAccessible()
{
  nsAccessibilityService* accService = nsIPresShell::AccService();
  return accService ?
    accService->CreateHTMLObjectFrameAccessible(this, mContent,
                                                PresContext()->PresShell()) :
    nsnull;
}

#ifdef XP_WIN
NS_IMETHODIMP nsObjectFrame::GetPluginPort(HWND *aPort)
{
  *aPort = (HWND) mInstanceOwner->GetPluginPortFromWidget();
  return NS_OK;
}
#endif
#endif

NS_IMETHODIMP 
nsObjectFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  PR_LOG(nsObjectFrameLM, PR_LOG_DEBUG,
         ("Initializing nsObjectFrame %p for content %p\n", this, aContent));

  nsresult rv = nsObjectFrameSuper::Init(aContent, aParent, aPrevInFlow);

  return rv;
}

void
nsObjectFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mReflowCallbackPosted) {
    PresContext()->PresShell()->CancelReflowCallback(this);
  }

  
  nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(mContent));
  NS_ASSERTION(objContent, "Why not an object loading content?");
  objContent->DisconnectFrame();

  if (mBackgroundSink) {
    mBackgroundSink->Destroy();
  }

  if (mInstanceOwner) {
    mInstanceOwner->SetFrame(nsnull);
  }
  SetInstanceOwner(nsnull);

  nsObjectFrameSuper::DestroyFrom(aDestructRoot);
}

 void
nsObjectFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  if (HasView()) {
    nsIView* view = GetView();
    nsIViewManager* vm = view->GetViewManager();
    if (vm) {
      nsViewVisibility visibility = 
        IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow;
      vm->SetViewVisibility(view, visibility);
    }
  }

  nsObjectFrameSuper::DidSetStyleContext(aOldStyleContext);
}

nsIAtom*
nsObjectFrame::GetType() const
{
  return nsGkAtoms::objectFrame; 
}

#ifdef DEBUG
NS_IMETHODIMP
nsObjectFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ObjectFrame"), aResult);
}
#endif

nsresult
nsObjectFrame::PrepForDrawing(nsIWidget *aWidget)
{
  mWidget = aWidget;

  nsIView* view = GetView();
  NS_ASSERTION(view, "Object frames must have views");  
  if (!view) {
    return NS_ERROR_FAILURE;
  }

  nsIViewManager* viewMan = view->GetViewManager();
  
  
  viewMan->SetViewVisibility(view, nsViewVisibility_kHide);

  
  
  
  
  nsIView* parentWithView;
  nsPoint origin;
  nsRect r(0, 0, mRect.width, mRect.height);

  GetOffsetFromView(origin, &parentWithView);
  viewMan->ResizeView(view, r);
  viewMan->MoveViewTo(view, origin.x, origin.y);

  nsRootPresContext* rpc = PresContext()->GetRootPresContext();
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
    viewMan->InsertChild(view, mInnerView, nsnull, true);

    mWidget->SetParent(parentWidget);
    mWidget->Show(true);
    mWidget->Enable(true);

    
    
    
    
    
    
    nsAutoTArray<nsIWidget::Configuration,1> configuration;
    GetEmptyClipConfiguration(&configuration);
    NS_ASSERTION(configuration.Length() > 0, "Empty widget configuration array!");
    configuration[0].mBounds.width = mRect.width;
    configuration[0].mBounds.height = mRect.height;
    parentWidget->ConfigureChildren(configuration);

    nsRefPtr<nsDeviceContext> dx;
    viewMan->GetDeviceContext(*getter_AddRefs(dx));
    EVENT_CALLBACK eventHandler = mInnerView->AttachWidgetEventHandler(mWidget);
    mWidget->SetEventCallback(eventHandler, dx);

#ifdef XP_MACOSX
    
    
    
    if (parentWidget == GetNearestWidget()) {
      Invalidate(GetContentRectRelativeToSelf());
    }
#endif

    rpc->RegisterPluginForGeometryUpdates(this);
    rpc->RequestUpdatePluginGeometry(this);

    
    
    
    
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

#ifndef XP_MACOSX
    rpc->RegisterPluginForGeometryUpdates(this);
    rpc->RequestUpdatePluginGeometry(this);
#endif
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
nsObjectFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result = 0;

  if (!IsHidden(false)) {
    nsIAtom *atom = mContent->Tag();
    if (atom == nsGkAtoms::applet || atom == nsGkAtoms::embed) {
      result = nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH);
    }
  }

  DISPLAY_MIN_WIDTH(this, result);
  return result;
}

 nscoord
nsObjectFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  return nsObjectFrame::GetMinWidth(aRenderingContext);
}

void
nsObjectFrame::GetDesiredSize(nsPresContext* aPresContext,
                              const nsHTMLReflowState& aReflowState,
                              nsHTMLReflowMetrics& aMetrics)
{
  
  aMetrics.width = 0;
  aMetrics.height = 0;

  if (IsHidden(false)) {
    return;
  }
  
  aMetrics.width = aReflowState.ComputedWidth();
  aMetrics.height = aReflowState.ComputedHeight();

  
  nsIAtom *atom = mContent->Tag();
  if (atom == nsGkAtoms::applet || atom == nsGkAtoms::embed) {
    if (aMetrics.width == NS_UNCONSTRAINEDSIZE) {
      aMetrics.width = clamped(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_WIDTH),
                               aReflowState.mComputedMinWidth,
                               aReflowState.mComputedMaxWidth);
    }
    if (aMetrics.height == NS_UNCONSTRAINEDSIZE) {
      aMetrics.height = clamped(nsPresContext::CSSPixelsToAppUnits(EMBED_DEF_HEIGHT),
                                aReflowState.mComputedMinHeight,
                                aReflowState.mComputedMaxHeight);
    }

#if defined (MOZ_WIDGET_GTK2)
    
    
    
    
    aMetrics.height = NS_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.height);
    aMetrics.width = NS_MIN(aPresContext->DevPixelsToAppUnits(PR_INT16_MAX), aMetrics.width);
#endif
  }

  
  
  
  if (aMetrics.width == NS_UNCONSTRAINEDSIZE) {
    aMetrics.width =
      (aReflowState.mComputedMinWidth != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.mComputedMinWidth : 0;
  }

  
  
  
  
  if (aMetrics.height == NS_UNCONSTRAINEDSIZE) {
    aMetrics.height =
      (aReflowState.mComputedMinHeight != NS_UNCONSTRAINEDSIZE) ?
        aReflowState.mComputedMinHeight : 0;
  }

  
  
  
  
  
}

NS_IMETHODIMP
nsObjectFrame::Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsObjectFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  
  GetDesiredSize(aPresContext, aReflowState, aMetrics);
  aMetrics.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aMetrics);

  
  
  
  if (!GetContent()->IsDoneAddingChildren()) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  
  if (aPresContext->Medium() == nsGkAtoms::print) {
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  nsRect r(0, 0, aMetrics.width, aMetrics.height);
  r.Deflate(aReflowState.mComputedBorderPadding);

  if (mInnerView) {
    nsIViewManager* vm = mInnerView->GetViewManager();
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
  return NS_OK;
}



bool
nsObjectFrame::ReflowFinished()
{
  mReflowCallbackPosted = false;
  CallSetWindow();
  return true;
}

void
nsObjectFrame::ReflowCallbackCanceled()
{
  mReflowCallbackPosted = false;
}

void
nsObjectFrame::FixupWindow(const nsSize& aSize)
{
  nsPresContext* presContext = PresContext();

  if (!mInstanceOwner)
    return;

  NPWindow *window;
  mInstanceOwner->GetWindow(window);

  NS_ENSURE_TRUE(window, );

#ifdef XP_MACOSX
  nsWeakFrame weakFrame(this);
  mInstanceOwner->FixUpPluginWindow(nsPluginInstanceOwner::ePluginPaintDisable);
  if (!weakFrame.IsAlive()) {
    return;
  }
#endif

  bool windowless = (window->type == NPWindowTypeDrawable);

  nsIntPoint origin = GetWindowOriginInPixels(windowless);

  window->x = origin.x;
  window->y = origin.y;

  window->width = presContext->AppUnitsToDevPixels(aSize.width);
  window->height = presContext->AppUnitsToDevPixels(aSize.height);

  
  
  
#ifdef XP_MACOSX
  window->clipRect.top = 0;
  window->clipRect.left = 0;
  window->clipRect.bottom = 0;
  window->clipRect.right = 0;
#else
  mInstanceOwner->UpdateWindowPositionAndClipRect(false);
#endif

  NotifyPluginReflowObservers();
}

nsresult
nsObjectFrame::CallSetWindow(bool aCheckIsHidden)
{
  NPWindow *win = nsnull;
 
  nsresult rv = NS_ERROR_FAILURE;
  nsRefPtr<nsNPAPIPluginInstance> pi;
  if (!mInstanceOwner ||
      NS_FAILED(rv = mInstanceOwner->GetInstance(getter_AddRefs(pi))) ||
      !pi ||
      NS_FAILED(rv = mInstanceOwner->GetWindow(win)) || 
      !win)
    return rv;

  nsPluginNativeWindow *window = (nsPluginNativeWindow *)win;
#ifdef XP_MACOSX
  nsWeakFrame weakFrame(this);
  mInstanceOwner->FixUpPluginWindow(nsPluginInstanceOwner::ePluginPaintDisable);
  if (!weakFrame.IsAlive()) {
    return NS_ERROR_NOT_AVAILABLE;
  }
#endif

  if (aCheckIsHidden && IsHidden())
    return NS_ERROR_FAILURE;

  
  window->window = mInstanceOwner->GetPluginPortFromWidget();

  
  
  nsPresContext* presContext = PresContext();
  nsRootPresContext* rootPC = presContext->GetRootPresContext();
  if (!rootPC)
    return NS_ERROR_FAILURE;
  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsIFrame* rootFrame = rootPC->PresShell()->FrameManager()->GetRootFrame();
  nsRect bounds = GetContentRectRelativeToSelf() + GetOffsetToCrossDoc(rootFrame);
  nsIntRect intBounds = bounds.ToNearestPixels(appUnitsPerDevPixel);
  window->x = intBounds.x;
  window->y = intBounds.y;
  window->width = intBounds.width;
  window->height = intBounds.height;

  
  
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
nsObjectFrame::SetInstanceOwner(nsPluginInstanceOwner* aOwner)
{
  mInstanceOwner = aOwner;
  if (!mInstanceOwner) {
    nsRootPresContext* rpc = PresContext()->GetRootPresContext();
    if (rpc) {
      if (mWidget) {
        if (mInnerView) {
          mInnerView->DetachWidgetEventHandler(mWidget);

          rpc->UnregisterPluginForGeometryUpdates(this);
          
          
          nsIWidget* parent = mWidget->GetParent();
          if (parent) {
            nsTArray<nsIWidget::Configuration> configurations;
            this->GetEmptyClipConfiguration(&configurations);
            parent->ConfigureChildren(configurations);

            mWidget->Show(false);
            mWidget->Enable(false);
            mWidget->SetParent(nsnull);
          }
        }
      } else {
#ifndef XP_MACOSX
        rpc->UnregisterPluginForGeometryUpdates(this);
#endif
      }
    }
  }
}

bool
nsObjectFrame::IsFocusable(PRInt32 *aTabIndex, bool aWithMouse)
{
  if (aTabIndex)
    *aTabIndex = -1;
  return nsObjectFrameSuper::IsFocusable(aTabIndex, aWithMouse);
}

bool
nsObjectFrame::IsHidden(bool aCheckVisibilityStyle) const
{
  if (aCheckVisibilityStyle) {
    if (!GetStyleVisibility()->IsVisibleOrCollapsed())
      return true;    
  }

  
  if (mContent->Tag() == nsGkAtoms::embed) {
    
    
    

    
    
    
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

nsIntPoint nsObjectFrame::GetWindowOriginInPixels(bool aWindowless)
{
  nsIView * parentWithView;
  nsPoint origin(0,0);

  GetOffsetFromView(origin, &parentWithView);

  
  
  if (aWindowless && parentWithView) {
    nsPoint offsetToWidget;
    parentWithView->GetNearestWidget(&offsetToWidget);
    origin += offsetToWidget;
  }
  origin += GetContentRectRelativeToSelf().TopLeft();

  return nsIntPoint(PresContext()->AppUnitsToDevPixels(origin.x),
                    PresContext()->AppUnitsToDevPixels(origin.y));
}

NS_IMETHODIMP
nsObjectFrame::DidReflow(nsPresContext*            aPresContext,
                         const nsHTMLReflowState*  aReflowState,
                         nsDidReflowStatus         aStatus)
{
  
  
  if (aStatus == NS_FRAME_REFLOW_FINISHED &&
      (GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsCOMPtr<nsIObjectLoadingContent> objContent(do_QueryInterface(mContent));
    NS_ASSERTION(objContent, "Why not an object loading content?");
    objContent->HasNewFrame(this);
  }

  nsresult rv = nsObjectFrameSuper::DidReflow(aPresContext, aReflowState, aStatus);

  
  
  if (aStatus != NS_FRAME_REFLOW_FINISHED) 
    return rv;

  if (HasView()) {
    nsIView* view = GetView();
    nsIViewManager* vm = view->GetViewManager();
    if (vm)
      vm->SetViewVisibility(view, IsHidden() ? nsViewVisibility_kHide : nsViewVisibility_kShow);
  }

  return rv;
}

 void
nsObjectFrame::PaintPrintPlugin(nsIFrame* aFrame, nsRenderingContext* aCtx,
                                const nsRect& aDirtyRect, nsPoint aPt)
{
  nsPoint pt = aPt + aFrame->GetContentRectRelativeToSelf().TopLeft();
  nsRenderingContext::AutoPushTranslation translate(aCtx, pt);
  
  static_cast<nsObjectFrame*>(aFrame)->PrintPlugin(*aCtx, aDirtyRect);
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);

  NS_DISPLAY_DECL_NAME("PluginReadback", TYPE_PLUGIN_READBACK)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  {
    return static_cast<nsObjectFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters)
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

bool
nsDisplayPluginReadback::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion,
                                           const nsRect& aAllowVisibleRegionExpansion)
{
  if (!nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion,
                                        aAllowVisibleRegionExpansion))
    return false;

  nsRect expand;
  bool snap;
  expand.IntersectRect(aAllowVisibleRegionExpansion, GetBounds(aBuilder, &snap));
  
  
  
  aVisibleRegion->Or(*aVisibleRegion, expand);
  return true;
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);

  NS_DISPLAY_DECL_NAME("PluginVideo", TYPE_PLUGIN_VIDEO)

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  {
    return static_cast<nsObjectFrame*>(mFrame)->BuildLayer(aBuilder, aManager, this);
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters)
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

bool
nsDisplayPluginVideo::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                           nsRegion* aVisibleRegion,
                                           const nsRect& aAllowVisibleRegionExpansion)
{
  return nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion,
                                          aAllowVisibleRegionExpansion);
}

#endif

nsRect
nsDisplayPlugin::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = false;
  return GetDisplayItemBounds(aBuilder, this, mFrame);
}

void
nsDisplayPlugin::Paint(nsDisplayListBuilder* aBuilder,
                       nsRenderingContext* aCtx)
{
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  bool snap;
  f->PaintPlugin(aBuilder, *aCtx, mVisibleRect, GetBounds(aBuilder, &snap));
}

bool
nsDisplayPlugin::ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion)
{
  bool snap;
  mVisibleRegion.And(*aVisibleRegion, GetBounds(aBuilder, &snap));
  return nsDisplayItem::ComputeVisibility(aBuilder, aVisibleRegion,
                                          aAllowVisibleRegionExpansion);
}

nsRegion
nsDisplayPlugin::GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                 bool* aSnap)
{
  *aSnap = false;
  nsRegion result;
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  if (!aBuilder->IsForPluginGeometry()) {
    nsIWidget* widget = f->GetWidget();
    if (widget) {
      nsTArray<nsIntRect> clip;
      widget->GetWindowClipRegion(&clip);
      nsTArray<nsIWidget::Configuration> configuration;
      GetWidgetConfiguration(aBuilder, &configuration);
      NS_ASSERTION(configuration.Length() == 1, "No configuration found");
      if (clip != configuration[0].mClipRegion) {
        
        
        
    	return result;
      }
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

void
nsDisplayPlugin::GetWidgetConfiguration(nsDisplayListBuilder* aBuilder,
                                        nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  nsObjectFrame* f = static_cast<nsObjectFrame*>(mFrame);
  nsPoint pluginOrigin = mFrame->GetContentRectRelativeToSelf().TopLeft() +
    ToReferenceFrame();
  f->ComputeWidgetGeometry(mVisibleRegion, pluginOrigin, aConfigurations);
}

void
nsObjectFrame::ComputeWidgetGeometry(const nsRegion& aRegion,
                                     const nsPoint& aPluginOrigin,
                                     nsTArray<nsIWidget::Configuration>* aConfigurations)
{
  if (!mWidget) {
#ifndef XP_MACOSX
    if (mInstanceOwner) {
      
      
      mInstanceOwner->UpdateWindowVisibility(!aRegion.IsEmpty());
    }
#endif
    return;
  }

  if (!mInnerView) {
    return;
  }

  nsPresContext* presContext = PresContext();
  nsRootPresContext* rootPC = presContext->GetRootPresContext();
  if (!rootPC)
    return;

  nsIWidget::Configuration* configuration = aConfigurations->AppendElement();
  if (!configuration)
    return;
  configuration->mChild = mWidget;

  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsIFrame* rootFrame = rootPC->PresShell()->FrameManager()->GetRootFrame();
  nsRect bounds = GetContentRectRelativeToSelf() + GetOffsetToCrossDoc(rootFrame);
  configuration->mBounds = bounds.ToNearestPixels(appUnitsPerDevPixel);

  
  
  
  mInnerView->CalcWidgetBounds(eWindowType_plugin);

  nsRegionRectIterator iter(aRegion);
  nsIntPoint pluginOrigin = aPluginOrigin.ToNearestPixels(appUnitsPerDevPixel);
  for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
    
    
    nsIntRect pixRect =
      r->ToNearestPixels(appUnitsPerDevPixel) - pluginOrigin;
    if (!pixRect.IsEmpty()) {
      configuration->mClipRegion.AppendElement(pixRect);
    }
  }
}

nsresult
nsObjectFrame::PluginEventNotifier::Run() {
  nsCOMPtr<nsIObserverService> obsSvc =
    mozilla::services::GetObserverService();
  obsSvc->NotifyObservers(nsnull, "plugin-changed-event", mEventType.get());
  return NS_OK;
}

void
nsObjectFrame::NotifyPluginReflowObservers()
{
  nsContentUtils::AddScriptRunner(new PluginEventNotifier(NS_LITERAL_STRING("reflow")));
}

void
nsObjectFrame::DidSetWidgetGeometry()
{
#if defined(XP_MACOSX)
  if (mInstanceOwner) {
    mInstanceOwner->FixUpPluginWindow(nsPluginInstanceOwner::ePluginPaintEnable);
  }
#endif
}

bool
nsObjectFrame::IsOpaque() const
{
#if defined(XP_MACOSX) || defined(MOZ_WIDGET_ANDROID)
  
  return false;
#else
  return !IsTransparentMode();
#endif
}

bool
nsObjectFrame::IsTransparentMode() const
{
#if defined(XP_MACOSX)
  
  return false;
#else
  if (!mInstanceOwner)
    return false;

  NPWindow *window = nsnull;
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

NS_IMETHODIMP
nsObjectFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  
  if (!IsVisibleOrCollapsedForPainting(aBuilder))
    return NS_OK;

  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsPresContext::nsPresContextType type = PresContext()->Type();

  
  if (type == nsPresContext::eContext_PrintPreview)
    return NS_OK;

  DO_GLOBAL_REFLOW_COUNT_DSP("nsObjectFrame");

#ifndef XP_MACOSX
  if (mWidget && aBuilder->IsInTransform()) {
    
    return NS_OK;
  }
#endif

  nsDisplayList replacedContent;

  if (aBuilder->IsForPainting() && mInstanceOwner && mInstanceOwner->UseAsyncRendering()) {
    NPWindow* window = nsnull;
    mInstanceOwner->GetWindow(window);
    bool isVisible = window && window->width > 0 && window->height > 0;
    if (isVisible && aBuilder->ShouldSyncDecodeImages()) {
  #ifndef XP_MACOSX
      mInstanceOwner->UpdateWindowVisibility(true);
  #endif
    }

    nsRefPtr<ImageContainer> container = GetImageContainer();
    if (container && container->HasCurrentImage() || !isVisible ||
        container->GetCurrentSize() != gfxIntSize(window->width, window->height)) {
      mInstanceOwner->NotifyPaintWaiter(aBuilder);
    }
  }

  
  if (type == nsPresContext::eContext_Print) {
    rv = replacedContent.AppendNewToTop(new (aBuilder)
        nsDisplayGeneric(aBuilder, this, PaintPrintPlugin, "PrintPlugin",
                         nsDisplayItem::TYPE_PRINT_PLUGIN));
  } else {
    
#if !MOZ_WIDGET_ANDROID
    if (aBuilder->IsPaintingToWindow() &&
        GetLayerState(aBuilder, nsnull) == LAYER_ACTIVE &&
        IsTransparentMode()) {
      rv = replacedContent.AppendNewToTop(new (aBuilder)
          nsDisplayPluginReadback(aBuilder, this));
      NS_ENSURE_SUCCESS(rv, rv);
    }
#endif

#if MOZ_WIDGET_ANDROID
    if (aBuilder->IsPaintingToWindow() &&
        GetLayerState(aBuilder, nsnull) == LAYER_ACTIVE) {

      nsTArray<nsNPAPIPluginInstance::VideoInfo*> videos;
      mInstanceOwner->GetVideos(videos);
      
      for (int i = 0; i < videos.Length(); i++) {
        rv = replacedContent.AppendNewToTop(new (aBuilder)
          nsDisplayPluginVideo(aBuilder, this, videos[i]));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
#endif

    rv = replacedContent.AppendNewToTop(new (aBuilder)
        nsDisplayPlugin(aBuilder, this));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  WrapReplacedContentForBorderRadius(aBuilder, &replacedContent, aLists);

  return NS_OK;
}

#ifdef XP_OS2
static void *
GetPSFromRC(nsRenderingContext& aRenderingContext)
{
  nsRefPtr<gfxASurface>
    surf = aRenderingContext.ThebesContext()->CurrentSurface();
  if (!surf || surf->CairoStatus())
    return nsnull;
  return (void *)(static_cast<gfxOS2Surface*>
                  (static_cast<gfxASurface*>(surf.get()))->GetPS());
}
#endif

void
nsObjectFrame::PrintPlugin(nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect)
{
  nsCOMPtr<nsIObjectLoadingContent> obj(do_QueryInterface(mContent));
  if (!obj)
    return;

  nsIFrame* frame = nsnull;
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
  window.window = nsnull;

  
  NPPrint npprint;
  npprint.mode = NP_EMBED;

  
  bool windowless = false;
  pi->IsWindowless(&windowless);
  window.type = windowless ? NPWindowTypeDrawable : NPWindowTypeWindow;

  window.clipRect.bottom = 0; window.clipRect.top = 0;
  window.clipRect.left = 0; window.clipRect.right = 0;
  

#ifdef MAC_CARBON_PLUGINS
  
  
  if (!::SetRect || !::NewGWorldFromPtr || !::DisposeGWorld) {
    NS_WARNING("Cannot print plugin -- required QuickDraw APIs are missing!");
    return;
  }

  nsSize contentSize = GetContentRectRelativeToSelf().Size();
  window.x = 0;
  window.y = 0;
  window.width = presContext->AppUnitsToDevPixels(contentSize.width);
  window.height = presContext->AppUnitsToDevPixels(contentSize.height);

  gfxContext *ctx = aRenderingContext.ThebesContext();
  if (!ctx)
    return;
  gfxContextAutoSaveRestore save(ctx);

  ctx->NewPath();

  gfxRect rect(window.x, window.y, window.width, window.height);

  ctx->Rectangle(rect);
  ctx->Clip();

  gfxQuartzNativeDrawing nativeDraw(ctx, rect);
  CGContextRef cgContext = nativeDraw.BeginNativeDrawing();
  if (!cgContext) {
    nativeDraw.EndNativeDrawing();
    return;
  }

  window.clipRect.right = window.width;
  window.clipRect.bottom = window.height;
  window.type = NPWindowTypeDrawable;

  ::Rect gwBounds;
  ::SetRect(&gwBounds, 0, 0, window.width, window.height);

  nsTArray<char> buffer(window.width * window.height * 4);
  CGColorSpaceRef cspace = ::CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
  if (!cspace) {
    nativeDraw.EndNativeDrawing();
    return;
  }
  CGContextRef cgBuffer =
    ::CGBitmapContextCreate(buffer.Elements(), 
                            window.width, window.height, 8, window.width * 4,
                            cspace, kCGBitmapByteOrder32Big | kCGImageAlphaPremultipliedFirst);
  ::CGColorSpaceRelease(cspace);
  if (!cgBuffer) {
    nativeDraw.EndNativeDrawing();
    return;
  }
  GWorldPtr gWorld;
  if (::NewGWorldFromPtr(&gWorld, k32ARGBPixelFormat, &gwBounds, NULL, NULL, 0,
                         buffer.Elements(), window.width * 4) != noErr) {
    ::CGContextRelease(cgBuffer);
    nativeDraw.EndNativeDrawing();
    return;
  }

  window.clipRect.right = window.width;
  window.clipRect.bottom = window.height;
  window.type = NPWindowTypeDrawable;
  
  
  
  
  
  
  
  
  window.window = &gWorld;
  npprint.print.embedPrint.platformPrint = gWorld;
  npprint.print.embedPrint.window = window;
  pi->Print(&npprint);

  ::CGContextTranslateCTM(cgContext, 0.0f, float(window.height));
  ::CGContextScaleCTM(cgContext, 1.0f, -1.0f);
  CGImageRef image = ::CGBitmapContextCreateImage(cgBuffer);
  if (!image) {
    ::CGContextRestoreGState(cgContext);
    ::CGContextRelease(cgBuffer);
    ::DisposeGWorld(gWorld);
    nativeDraw.EndNativeDrawing();
    return;
  }
  ::CGContextDrawImage(cgContext,
                       ::CGRectMake(0, 0, window.width, window.height),
                       image);
  ::CGImageRelease(image);
  ::CGContextRelease(cgBuffer);

  ::DisposeGWorld(gWorld);

  nativeDraw.EndNativeDrawing();
#elif defined(XP_UNIX)

  


  (void)window;
  (void)npprint;

#elif defined(XP_OS2)
  void *hps = GetPSFromRC(aRenderingContext);
  if (!hps)
    return;

  npprint.print.embedPrint.platformPrint = hps;
  npprint.print.embedPrint.window = window;
  
  pi->Print(&npprint);
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

  
  
  nsDidReflowStatus status = NS_FRAME_REFLOW_FINISHED; 
  frame->DidReflow(presContext,
                   nsnull, status);  
}

already_AddRefed<ImageContainer>
nsObjectFrame::GetImageContainer()
{
  nsRefPtr<ImageContainer> container = mImageContainer;

  if (container) {
    return container.forget();
  }

  container = mImageContainer = LayerManager::CreateImageContainer();

  return container.forget();
}

nsRect
nsObjectFrame::GetPaintedRect(nsDisplayPlugin* aItem)
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

void
nsObjectFrame::UpdateImageLayer(const gfxRect& aRect)
{
  if (!mInstanceOwner) {
    return;
  }

#ifdef XP_MACOSX
  if (!mInstanceOwner->UseAsyncRendering()) {
    mInstanceOwner->DoCocoaEventDrawRect(aRect, nsnull);
    
    
    
    
    mInstanceOwner->GetImageContainer();
  }
#endif
}

LayerState
nsObjectFrame::GetLayerState(nsDisplayListBuilder* aBuilder,
                             LayerManager* aManager)
{
  if (!mInstanceOwner)
    return LAYER_NONE;

#ifdef XP_MACOSX
  if (!mInstanceOwner->UseAsyncRendering() &&
      mInstanceOwner->IsRemoteDrawingCoreAnimation() &&
      mInstanceOwner->GetEventModel() == NPEventModelCocoa) {
    return LAYER_ACTIVE;
  }
#endif

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
nsObjectFrame::BuildLayer(nsDisplayListBuilder* aBuilder,
                          LayerManager* aManager,
                          nsDisplayItem* aItem)
{
  if (!mInstanceOwner)
    return nsnull;

  NPWindow* window = nsnull;
  mInstanceOwner->GetWindow(window);
  if (!window)
    return nsnull;

  if (window->width <= 0 || window->height <= 0)
    return nsnull;

  
  nsRefPtr<ImageContainer> container = mInstanceOwner->GetImageContainer();

  if (!container) {
    
    return nsnull;
  }

  gfxIntSize size;

#ifdef XP_MACOSX
  if (mInstanceOwner->GetDrawingModel() == NPDrawingModelCoreAnimation) {
    size = container->GetCurrentSize();
  } else
#endif
  {
    size = gfxIntSize(window->width, window->height);
  }

  nsRect area = GetContentRectRelativeToSelf() + aItem->ToReferenceFrame();
  gfxRect r = nsLayoutUtils::RectToGfxRect(area, PresContext()->AppUnitsPerDevPixel());
  
  r.Round();
  nsRefPtr<Layer> layer =
    (GetLayerBuilderForManager(aManager)->GetLeafLayerFor(aBuilder, aManager, aItem));

  if (aItem->GetType() == nsDisplayItem::TYPE_PLUGIN) {
    if (!layer) {
      mInstanceOwner->NotifyPaintWaiter(aBuilder);
      
      layer = aManager->CreateImageLayer();
      if (!layer)
        return nsnull;
    }

    NS_ASSERTION(layer->GetType() == Layer::TYPE_IMAGE, "Bad layer type");

    ImageLayer* imglayer = static_cast<ImageLayer*>(layer.get());
    UpdateImageLayer(r);

#ifdef XP_WIN
    imglayer->SetScaleToSize(size, ImageLayer::SCALE_STRETCH);
#endif
    imglayer->SetContainer(container);
    gfxPattern::GraphicsFilter filter =
      nsLayoutUtils::GetGraphicsFilterForFrame(this);
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
    if (!aManager->IsCompositingCheap()) {
      
      filter = gfxPattern::FILTER_NEAREST;
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
      return nsnull;

    if (!layer) {
      
      layer = aManager->CreateImageLayer();
      if (!layer)
        return nsnull;
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
    NS_ABORT_IF_FALSE(!IsOpaque(), "Opaque plugins don't use backgrounds");

    if (!layer) {
      layer = aManager->CreateReadbackLayer();
      if (!layer)
        return nsnull;
    }
    NS_ASSERTION(layer->GetType() == Layer::TYPE_READBACK, "Bad layer type");

    ReadbackLayer* readback = static_cast<ReadbackLayer*>(layer.get());
    if (readback->GetSize() != nsIntSize(size.width, size.height)) {
      
      
      readback->SetSink(nsnull);
      readback->SetSize(nsIntSize(size.width, size.height));

      if (mBackgroundSink) {
        
        
        
        
        mBackgroundSink->Destroy();
      }
      mBackgroundSink =
        new PluginBackgroundSink(this,
                                 readback->AllocateSequenceNumber());
      readback->SetSink(mBackgroundSink);
      
      
    }
  }

  
  gfxMatrix transform;
  transform.Translate(r.TopLeft());

  layer->SetTransform(gfx3DMatrix::From2D(transform));
  layer->SetVisibleRegion(nsIntRect(0, 0, size.width, size.height));
  return layer.forget();
}

void
nsObjectFrame::PaintPlugin(nsDisplayListBuilder* aBuilder,
                           nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, const nsRect& aPluginRect)
{
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
      PRInt32 appUnitsPerDevPixel = PresContext()->AppUnitsPerDevPixel();
      
      
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
      ctx->Translate(offset);

      gfxQuartzNativeDrawing nativeDrawing(ctx, nativeClipRect - offset);

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
#ifndef NP_NO_CARBON
      if (mInstanceOwner->GetEventModel() == NPEventModelCarbon &&
          !mInstanceOwner->SetPluginPortAndDetectChange()) {
        NS_WARNING("null plugin port during PaintPlugin");
        nativeDrawing.EndNativeDrawing();
        return;
      }

      
      
      
      
      
      
      
      
      NP_CGContext* windowContext = static_cast<NP_CGContext*>(window->window);
      if (mInstanceOwner->GetEventModel() == NPEventModelCarbon &&
          windowContext->context != cgContext) {
        windowContext->context = cgContext;
        cgPluginPortCopy->context = cgContext;
        mInstanceOwner->SetPluginPortChanged(true);
      }
#endif

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
      
      nsRenderingContext::AutoPushTranslation
        translate(&aRenderingContext, aPluginRect.TopLeft());

      
      gfxRect tmpRect(0, 0, 0, 0);
      mInstanceOwner->Paint(tmpRect, NULL);
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
      ctx->IdentityMatrix();
    }
    dirtyGfxRect.RoundOut();

    
    NPWindow *window;
    mInstanceOwner->GetWindow(window);

    if (window->type == NPWindowTypeDrawable) {
      
      nsPoint origin;

      gfxWindowsNativeDrawing nativeDraw(ctx, frameGfxRect);
      if (nativeDraw.IsDoublePass()) {
        
        
        
        NPEvent pluginEvent;
        pluginEvent.event = DoublePassRenderingEvent();
        pluginEvent.wParam = 0;
        pluginEvent.lParam = 0;
        if (pluginEvent.event)
          inst->HandleEvent(&pluginEvent, nsnull);
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
          inst->HandleEvent(&pluginEvent, nsnull);
        }

        inst->SetWindow(window);

        mInstanceOwner->Paint(dirty, hdc);
        nativeDraw.EndNativeDrawing();
      } while (nativeDraw.ShouldRenderAgain());
      nativeDraw.PaintToContext();
    }

    ctx->SetMatrix(currentMatrix);
  }
#elif defined(XP_OS2)
  nsRefPtr<nsNPAPIPluginInstance> inst;
  GetPluginInstance(getter_AddRefs(inst));
  if (inst) {
    
    NPWindow *window;
    mInstanceOwner->GetWindow(window);

    if (window->type == NPWindowTypeDrawable) {
      
      nsRenderingContext::AutoPushTranslation
        translate(&aRenderingContext, aPluginRect.TopLeft());

      
      bool doupdatewindow = false;
      
      nsIntPoint origin;

      







      gfxContext *ctx = aRenderingContext.ThebesContext();

      gfxMatrix ctxMatrix = ctx->CurrentMatrix();
      if (ctxMatrix.HasNonTranslation()) {
        
        
        

        
        
        

        return;
      }

      origin.x = NSToIntRound(float(ctxMatrix.GetTranslation().x));
      origin.y = NSToIntRound(float(ctxMatrix.GetTranslation().y));

      
      ctx->UpdateSurfaceClip();

      
      gfxFloat xoff, yoff;
      nsRefPtr<gfxASurface> surf = ctx->CurrentSurface(&xoff, &yoff);

      if (surf->CairoStatus() != 0) {
        NS_WARNING("Plugin is being asked to render to a surface that's in error!");
        return;
      }

      
      HPS hps = (HPS)GetPSFromRC(aRenderingContext);
      if (reinterpret_cast<HPS>(window->window) != hps) {
        window->window = reinterpret_cast<void*>(hps);
        doupdatewindow = true;
      }
      LONG lPSid = GpiSavePS(hps);
      RECTL rclViewport;
      if (GpiQueryDevice(hps) != NULLHANDLE) { 
        if (GpiQueryPageViewport(hps, &rclViewport)) {
          rclViewport.xLeft += (LONG)xoff;
          rclViewport.xRight += (LONG)xoff;
          rclViewport.yBottom += (LONG)yoff;
          rclViewport.yTop += (LONG)yoff;
          GpiSetPageViewport(hps, &rclViewport);
        }
      }

      if ((window->x != origin.x) || (window->y != origin.y)) {
        window->x = origin.x;
        window->y = origin.y;
        doupdatewindow = true;
      }

      
      if (doupdatewindow) {
        inst->SetWindow(window);        
      }

      mInstanceOwner->Paint(aDirtyRect, hps);
      if (lPSid >= 1) {
        GpiRestorePS(hps, lPSid);
      }
      surf->MarkDirty();
    }
  }
#endif
}

NS_IMETHODIMP
nsObjectFrame::HandleEvent(nsPresContext* aPresContext,
                           nsGUIEvent*     anEvent,
                           nsEventStatus*  anEventStatus)
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
      NS_IS_PLUGIN_EVENT(anEvent)) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    return rv;
  }

#ifdef XP_WIN
  rv = nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
  return rv;
#endif

#ifdef XP_MACOSX
  
  if ((anEvent->message == NS_MOUSE_ENTER || anEvent->message == NS_MOUSE_SCROLL) &&
      mInstanceOwner->GetEventModel() == NPEventModelCocoa) {
    *anEventStatus = mInstanceOwner->ProcessEvent(*anEvent);
    return rv;
  }
#endif

  if (anEvent->message == NS_DESTROY) {
#ifdef MAC_CARBON_PLUGINS
    mInstanceOwner->CancelTimer();
#endif
    return rv;
  }

  return nsObjectFrameSuper::HandleEvent(aPresContext, anEvent, anEventStatus);
}

#ifdef XP_MACOSX


NS_IMETHODIMP
nsObjectFrame::HandlePress(nsPresContext* aPresContext,
                           nsGUIEvent*    anEvent,
                           nsEventStatus* anEventStatus)
{
  nsIPresShell::SetCapturingContent(GetContent(), CAPTURE_IGNOREALLOWED);
  return nsObjectFrameSuper::HandlePress(aPresContext, anEvent, anEventStatus);
}
#endif

nsresult
nsObjectFrame::GetPluginInstance(nsNPAPIPluginInstance** aPluginInstance)
{
  *aPluginInstance = nsnull;

  if (!mInstanceOwner) {
    return NS_OK;
  }

  return mInstanceOwner->GetInstance(aPluginInstance);
}

NS_IMETHODIMP
nsObjectFrame::GetCursor(const nsPoint& aPoint, nsIFrame::Cursor& aCursor)
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

  return nsObjectFrameSuper::GetCursor(aPoint, aCursor);
}

void
nsObjectFrame::SetIsDocumentActive(bool aIsActive)
{
#ifndef XP_MACOSX
  if (mInstanceOwner) {
    mInstanceOwner->UpdateDocumentActiveState(aIsActive);
  }
#endif
}


nsIObjectFrame *
nsObjectFrame::GetNextObjectFrame(nsPresContext* aPresContext, nsIFrame* aRoot)
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

  return nsnull;
}

 void
nsObjectFrame::BeginSwapDocShells(nsIContent* aContent, void*)
{
  NS_PRECONDITION(aContent, "");

  
  
  nsIObjectFrame* obj = do_QueryFrame(aContent->GetPrimaryFrame());
  if (!obj)
    return;

  nsObjectFrame* objectFrame = static_cast<nsObjectFrame*>(obj);
  NS_ASSERTION(!objectFrame->mWidget || objectFrame->mWidget->GetParent(),
               "Plugin windows must not be toplevel");
  nsRootPresContext* rootPC = objectFrame->PresContext()->GetRootPresContext();
  NS_ASSERTION(rootPC, "unable to unregister the plugin frame");
  rootPC->UnregisterPluginForGeometryUpdates(objectFrame);
}

 void
nsObjectFrame::EndSwapDocShells(nsIContent* aContent, void*)
{
  NS_PRECONDITION(aContent, "");

  
  
  nsIObjectFrame* obj = do_QueryFrame(aContent->GetPrimaryFrame());
  if (!obj)
    return;

  nsObjectFrame* objectFrame = static_cast<nsObjectFrame*>(obj);
  nsRootPresContext* rootPC = objectFrame->PresContext()->GetRootPresContext();
  NS_ASSERTION(rootPC, "unable to register the plugin frame");
  nsIWidget* widget = objectFrame->mWidget;
  if (widget) {
    
    nsIWidget* parent =
      rootPC->PresShell()->GetRootFrame()->GetNearestWidget();
    widget->SetParent(parent);
    objectFrame->CallSetWindow();

    
    rootPC->RegisterPluginForGeometryUpdates(objectFrame);
    rootPC->RequestUpdatePluginGeometry(objectFrame);
  }
}

nsIFrame*
NS_NewObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsObjectFrame(aContext);
}

bool
nsObjectFrame::PaintedByGecko()
{
#ifdef XP_MACOSX
  return true;
#else
  return !mWidget;
#endif
}

NS_IMPL_FRAMEARENA_HELPERS(nsObjectFrame)
