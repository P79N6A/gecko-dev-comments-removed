






#include "base/basictypes.h"

#include "BasicLayers.h"
#include "gfx3DMatrix.h"
#include "LayerManagerOGL.h"
#ifdef MOZ_ENABLE_D3D9_LAYER
# include "LayerManagerD3D9.h"
#endif 
#include "mozilla/BrowserElementParent.h"
#include "mozilla/dom/TabParent.h"
#include "mozilla/layers/AsyncPanZoomController.h"
#include "mozilla/layers/CompositorParent.h"
#include "mozilla/layers/ShadowLayersParent.h"
#include "nsContentUtils.h"
#include "nsFrameLoader.h"
#include "nsIObserver.h"
#include "nsSubDocumentFrame.h"
#include "nsViewportFrame.h"
#include "RenderFrameParent.h"

typedef nsContentView::ViewConfig ViewConfig;
using namespace mozilla::dom;
using namespace mozilla::layers;

namespace mozilla {
namespace layout {

typedef FrameMetrics::ViewID ViewID;
typedef RenderFrameParent::ViewMap ViewMap;


struct ViewTransform {
  ViewTransform(nsIntPoint aTranslation = nsIntPoint(0, 0), float aXScale = 1, float aYScale = 1)
    : mTranslation(aTranslation)
    , mXScale(aXScale)
    , mYScale(aYScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::ScalingMatrix(mXScale, mYScale, 1) *
      gfx3DMatrix::Translation(mTranslation.x, mTranslation.y, 0);
  }

  nsIntPoint mTranslation;
  float mXScale;
  float mYScale;
};







static double GetXScale(const gfx3DMatrix& aTransform)
{
  return aTransform._11;
}
 
static double GetYScale(const gfx3DMatrix& aTransform)
{
  return aTransform._22;
}

static void Scale(gfx3DMatrix& aTransform, double aXScale, double aYScale)
{
  aTransform._11 *= aXScale;
  aTransform._22 *= aYScale;
}

static void ReverseTranslate(gfx3DMatrix& aTransform, const gfxPoint& aOffset)
{
  aTransform._41 -= aOffset.x;
  aTransform._42 -= aOffset.y;
}


static void ApplyTransform(nsRect& aRect,
                           gfx3DMatrix& aTransform,
                           nscoord auPerDevPixel)
{
  aRect.x = aRect.x * aTransform._11 + aTransform._41 * auPerDevPixel;
  aRect.y = aRect.y * aTransform._22 + aTransform._42 * auPerDevPixel;
  aRect.width = aRect.width * aTransform._11;
  aRect.height = aRect.height * aTransform._22;
}
 
static void
AssertInTopLevelChromeDoc(ContainerLayer* aContainer,
                          nsIFrame* aContainedFrame)
{
  NS_ASSERTION(
    (aContainer->Manager()->GetBackendType() != mozilla::layers::LAYERS_BASIC) ||
    (aContainedFrame->GetNearestWidget() ==
     static_cast<BasicLayerManager*>(aContainer->Manager())->GetRetainerWidget()),
    "Expected frame to be in top-level chrome document");
}


static nsContentView*
FindViewForId(const ViewMap& aMap, ViewID aId)
{
  ViewMap::const_iterator iter = aMap.find(aId);
  return iter != aMap.end() ? iter->second : NULL;
}

static const FrameMetrics*
GetFrameMetrics(Layer* aLayer)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  return container ? &container->GetFrameMetrics() : NULL;
}






static nsIntPoint
GetContentRectLayerOffset(nsIFrame* aContainerFrame, nsDisplayListBuilder* aBuilder)
{
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();

  
  
  
  
  nsPoint frameOffset = aBuilder->ToReferenceFrame(aContainerFrame) +
    (aContainerFrame->GetContentRect().TopLeft() - aContainerFrame->GetPosition());

  return frameOffset.ToNearestPixels(auPerDevPixel);
}






static ViewTransform
ComputeShadowTreeTransform(nsIFrame* aContainerFrame,
                           nsFrameLoader* aRootFrameLoader,
                           const FrameMetrics* aMetrics,
                           const ViewConfig& aConfig,
                           float aTempScaleX = 1.0,
                           float aTempScaleY = 1.0)
{
  
  
  
  
  
  
  
  
  
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntPoint scrollOffset =
    aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel);
  
  gfxPoint metricsScrollOffset = aMetrics->GetScrollOffsetInLayerPixels();
  nsIntPoint roundedMetricsScrollOffset =
    nsIntPoint(NS_lround(metricsScrollOffset.x), NS_lround(metricsScrollOffset.y));

  if (aRootFrameLoader->AsyncScrollEnabled() && !aMetrics->mDisplayPort.IsEmpty()) {
    
    
    
    
    nsIntPoint scrollCompensation(
      (scrollOffset.x / aTempScaleX - roundedMetricsScrollOffset.x) * aConfig.mXScale,
      (scrollOffset.y / aTempScaleY - roundedMetricsScrollOffset.y) * aConfig.mYScale);

    return ViewTransform(-scrollCompensation, aConfig.mXScale, aConfig.mYScale);
  } else {
    return ViewTransform(nsIntPoint(0, 0), 1, 1);
  }
}


static void
BuildListForLayer(Layer* aLayer,
                  nsFrameLoader* aRootFrameLoader,
                  const gfx3DMatrix& aTransform,
                  nsDisplayListBuilder* aBuilder,
                  nsDisplayList& aShadowTree,
                  nsIFrame* aSubdocFrame)
{
  const FrameMetrics* metrics = GetFrameMetrics(aLayer);

  gfx3DMatrix transform;

  if (metrics && metrics->IsScrollable()) {
    const ViewID scrollId = metrics->mScrollId;

    
    
    
    

    
    nsContentView* view =
      aRootFrameLoader->GetCurrentRemoteFrame()->GetContentView(scrollId);
    
    
    gfx3DMatrix applyTransform = ComputeShadowTreeTransform(
      aSubdocFrame, aRootFrameLoader, metrics, view->GetViewConfig(),
      1 / GetXScale(aTransform), 1 / GetYScale(aTransform));
    transform = applyTransform * aLayer->GetTransform() * aTransform;

    
    
    gfx3DMatrix tmpTransform = aTransform;
    Scale(tmpTransform, GetXScale(applyTransform), GetYScale(applyTransform));

    
    nsRect bounds;
    {
      nscoord auPerDevPixel = aSubdocFrame->PresContext()->AppUnitsPerDevPixel();
      gfx::Rect viewport = metrics->mViewport;
      bounds = nsIntRect(viewport.x, viewport.y,
                         viewport.width, viewport.height).ToAppUnits(auPerDevPixel);
      ApplyTransform(bounds, tmpTransform, auPerDevPixel);

    }

    aShadowTree.AppendToTop(
      new (aBuilder) nsDisplayRemoteShadow(aBuilder, aSubdocFrame, bounds, scrollId));

  } else {
    transform = aLayer->GetTransform() * aTransform;
  }

  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    BuildListForLayer(child, aRootFrameLoader, transform,
                      aBuilder, aShadowTree, aSubdocFrame);
  }
}


static void
TransformShadowTree(nsDisplayListBuilder* aBuilder, nsFrameLoader* aFrameLoader,
                    nsIFrame* aFrame, Layer* aLayer,
                    const ViewTransform& aTransform,
                    float aTempScaleDiffX = 1.0,
                    float aTempScaleDiffY = 1.0)
{
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowClipRect(aLayer->GetClipRect());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());
  shadow->SetShadowOpacity(aLayer->GetOpacity());

  const FrameMetrics* metrics = GetFrameMetrics(aLayer);

  gfx3DMatrix shadowTransform = aLayer->GetTransform();
  ViewTransform layerTransform = aTransform;

  if (metrics && metrics->IsScrollable()) {
    const ViewID scrollId = metrics->mScrollId;
    const nsContentView* view =
      aFrameLoader->GetCurrentRemoteFrame()->GetContentView(scrollId);
    NS_ABORT_IF_FALSE(view, "Array of views should be consistent with layer tree");
    const gfx3DMatrix& currentTransform = aLayer->GetTransform();

    const ViewConfig& config = view->GetViewConfig();
    
    
    aTempScaleDiffX *= GetXScale(shadowTransform) * config.mXScale;
    aTempScaleDiffY *= GetYScale(shadowTransform) * config.mYScale;
    ViewTransform viewTransform = ComputeShadowTreeTransform(
      aFrame, aFrameLoader, metrics, view->GetViewConfig(),
      aTempScaleDiffX, aTempScaleDiffY
    );

    
    shadowTransform = gfx3DMatrix(viewTransform) * currentTransform;

    layerTransform = viewTransform;
    if (metrics->IsRootScrollable()) {
      
      nsIntPoint offset = GetContentRectLayerOffset(aFrame, aBuilder);
      shadowTransform = shadowTransform *
          gfx3DMatrix::Translation(float(offset.x), float(offset.y), 0.0);
    }
  }

  if (aLayer->GetIsFixedPosition() &&
      !aLayer->GetParent()->GetIsFixedPosition()) {
    
    
    
    float offsetX = layerTransform.mTranslation.x / layerTransform.mXScale;
    float offsetY = layerTransform.mTranslation.y / layerTransform.mYScale;
    ReverseTranslate(shadowTransform, gfxPoint(offsetX, offsetY));
    const nsIntRect* clipRect = shadow->GetShadowClipRect();
    if (clipRect) {
      nsIntRect transformedClipRect(*clipRect);
      transformedClipRect.MoveBy(-offsetX, -offsetY);
      shadow->SetShadowClipRect(&transformedClipRect);
    }
  }

  
  
  
  if (ContainerLayer* c = aLayer->AsContainerLayer()) {
    shadowTransform.Scale(1.0f/c->GetPreXScale(),
                          1.0f/c->GetPreYScale(),
                          1);
  }
  shadowTransform.ScalePost(1.0f/aLayer->GetPostXScale(),
                            1.0f/aLayer->GetPostYScale(),
                            1);

  shadow->SetShadowTransform(shadowTransform);
  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    TransformShadowTree(aBuilder, aFrameLoader, aFrame, child, layerTransform,
                        aTempScaleDiffX, aTempScaleDiffY);
  }
}

static void
ClearContainer(ContainerLayer* aContainer)
{
  while (Layer* layer = aContainer->GetFirstChild()) {
    aContainer->RemoveChild(layer);
  }
}





inline static bool
IsTempLayerManager(LayerManager* aManager)
{
  return (mozilla::layers::LAYERS_BASIC == aManager->GetBackendType() &&
          !static_cast<BasicLayerManager*>(aManager)->IsRetained());
}







static void
BuildViewMap(ViewMap& oldContentViews, ViewMap& newContentViews,
             nsFrameLoader* aFrameLoader, Layer* aLayer,
             float aXScale = 1, float aYScale = 1,
             float aAccConfigXScale = 1, float aAccConfigYScale = 1)
{
  ContainerLayer* container = aLayer->AsContainerLayer();
  if (!container)
    return;
  const FrameMetrics metrics = container->GetFrameMetrics();
  const ViewID scrollId = metrics.mScrollId;
  const gfx3DMatrix transform = aLayer->GetTransform();
  aXScale *= GetXScale(transform);
  aYScale *= GetYScale(transform);

  if (metrics.IsScrollable()) {
    nscoord auPerDevPixel = aFrameLoader->GetPrimaryFrameOfOwningContent()
                                        ->PresContext()->AppUnitsPerDevPixel();
    nscoord auPerCSSPixel = auPerDevPixel * metrics.mDevPixelsPerCSSPixel;
    nsContentView* view = FindViewForId(oldContentViews, scrollId);
    if (view) {
      
      
      ViewConfig config = view->GetViewConfig();
      aXScale *= config.mXScale;
      aYScale *= config.mYScale;
      view->mFrameLoader = aFrameLoader;
      
      
      if (aAccConfigXScale != view->mParentScaleX ||
          aAccConfigYScale != view->mParentScaleY) {
        float xscroll = 0, yscroll = 0;
        view->GetScrollX(&xscroll);
        view->GetScrollY(&yscroll);
        xscroll = xscroll * (aAccConfigXScale / view->mParentScaleX);
        yscroll = yscroll * (aAccConfigYScale / view->mParentScaleY);
        view->ScrollTo(xscroll, yscroll);
        view->mParentScaleX = aAccConfigXScale;
        view->mParentScaleY = aAccConfigYScale;
      }
      
      aAccConfigXScale *= config.mXScale;
      aAccConfigYScale *= config.mYScale;
    } else {
      
      
      
      ViewConfig config;
      config.mScrollOffset = nsPoint(
        NSIntPixelsToAppUnits(metrics.mScrollOffset.x, auPerCSSPixel) * aXScale,
        NSIntPixelsToAppUnits(metrics.mScrollOffset.y, auPerCSSPixel) * aYScale);
      view = new nsContentView(aFrameLoader, scrollId, config);
      view->mParentScaleX = aAccConfigXScale;
      view->mParentScaleY = aAccConfigYScale;
    }

    view->mViewportSize = nsSize(
      NSIntPixelsToAppUnits(metrics.mViewport.width, auPerDevPixel) * aXScale,
      NSIntPixelsToAppUnits(metrics.mViewport.height, auPerDevPixel) * aYScale);
    view->mContentSize = nsSize(
      NSIntPixelsToAppUnits(metrics.mContentRect.width, auPerDevPixel) * aXScale,
      NSIntPixelsToAppUnits(metrics.mContentRect.height, auPerDevPixel) * aYScale);

    newContentViews[scrollId] = view;
  }

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    BuildViewMap(oldContentViews, newContentViews, aFrameLoader, child,
                 aXScale, aYScale, aAccConfigXScale, aAccConfigYScale);
  }
}

static void
BuildBackgroundPatternFor(ContainerLayer* aContainer,
                          ContainerLayer* aShadowRoot,
                          const ViewConfig& aConfig,
                          const gfxRGBA& aColor,
                          LayerManager* aManager,
                          nsIFrame* aFrame)
{
  ShadowLayer* shadowRoot = aShadowRoot->AsShadowLayer();
  gfxMatrix t;
  if (!shadowRoot->GetShadowTransform().Is2D(&t)) {
    return;
  }

  
  
  nsIntRect contentBounds = shadowRoot->GetShadowVisibleRegion().GetBounds();
  gfxRect contentVis(contentBounds.x, contentBounds.y,
                     contentBounds.width, contentBounds.height);
  gfxRect localContentVis(t.Transform(contentVis));
  
  localContentVis.RoundIn();
  nsIntRect localIntContentVis(localContentVis.X(), localContentVis.Y(),
                               localContentVis.Width(), localContentVis.Height());

  
  nscoord auPerDevPixel = aFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect frameRect = aFrame->GetRect().ToOutsidePixels(auPerDevPixel);

  
  
  if (localIntContentVis.Contains(frameRect)) {
    return;
  }
  nsRefPtr<ColorLayer> layer = aManager->CreateColorLayer();
  layer->SetColor(aColor);

  
  
  nsIntRegion bgRgn(frameRect);
  bgRgn.Sub(bgRgn, localIntContentVis);
  bgRgn.MoveBy(-frameRect.TopLeft());
  layer->SetVisibleRegion(bgRgn);

  aContainer->InsertAfter(layer, nullptr);
}

already_AddRefed<LayerManager>
GetFrom(nsFrameLoader* aFrameLoader)
{
  nsIDocument* doc = aFrameLoader->GetOwnerDoc();
  return nsContentUtils::LayerManagerForDocument(doc);
}

class RemoteContentController : public GeckoContentController {
public:
  RemoteContentController(RenderFrameParent* aRenderFrame)
    : mUILoop(MessageLoop::current())
    , mRenderFrame(aRenderFrame)
  { }

  virtual void RequestContentRepaint(const FrameMetrics& aFrameMetrics) MOZ_OVERRIDE
  {
    
    
    mUILoop->PostTask(
      FROM_HERE,
      NewRunnableMethod(this, &RemoteContentController::DoRequestContentRepaint,
                        aFrameMetrics));
  }

  virtual void HandleDoubleTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleDoubleTap,
                          aPoint));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleDoubleTap(aPoint);
    }
  }

  virtual void HandleSingleTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleSingleTap,
                          aPoint));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleSingleTap(aPoint);
    }
  }

  virtual void HandleLongTap(const nsIntPoint& aPoint) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      
      
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this, &RemoteContentController::HandleLongTap,
                          aPoint));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->HandleLongTap(aPoint);
    }
  }

  void ClearRenderFrame() { mRenderFrame = nullptr; }

  virtual void SendAsyncScrollDOMEvent(const gfx::Rect& aContentRect,
                                       const gfx::Size& aContentSize) MOZ_OVERRIDE
  {
    if (MessageLoop::current() != mUILoop) {
      mUILoop->PostTask(
        FROM_HERE,
        NewRunnableMethod(this,
                          &RemoteContentController::SendAsyncScrollDOMEvent,
                          aContentRect, aContentSize));
      return;
    }
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      BrowserElementParent::DispatchAsyncScrollEvent(browser, aContentRect,
                                                     aContentSize);
    }
  }

private:
  void DoRequestContentRepaint(const FrameMetrics& aFrameMetrics)
  {
    if (mRenderFrame) {
      TabParent* browser = static_cast<TabParent*>(mRenderFrame->Manager());
      browser->UpdateFrame(aFrameMetrics);
    }
  }

  MessageLoop* mUILoop;
  RenderFrameParent* mRenderFrame;
};

RenderFrameParent::RenderFrameParent(nsFrameLoader* aFrameLoader,
                                     ScrollingBehavior aScrollingBehavior,
                                     LayersBackend* aBackendType,
                                     int* aMaxTextureSize,
                                     uint64_t* aId)
  : mLayersId(0)
  , mFrameLoader(aFrameLoader)
  , mFrameLoaderDestroyed(false)
  , mBackgroundColor(gfxRGBA(1, 1, 1))
{
  mContentViews[FrameMetrics::ROOT_SCROLL_ID] =
    new nsContentView(aFrameLoader, FrameMetrics::ROOT_SCROLL_ID);

  *aBackendType = mozilla::layers::LAYERS_NONE;
  *aMaxTextureSize = 0;
  *aId = 0;

  nsRefPtr<LayerManager> lm = GetFrom(mFrameLoader);
  
  if (lm) {
    *aBackendType = lm->GetBackendType();
    *aMaxTextureSize = lm->GetMaxTextureSize();
  }

  if (CompositorParent::CompositorLoop()) {
    
    
    *aId = mLayersId = CompositorParent::AllocateLayerTreeId();
    if (aScrollingBehavior == ASYNC_PAN_ZOOM) {
      mContentController = new RemoteContentController(this);
      mPanZoomController = new AsyncPanZoomController(
        mContentController, AsyncPanZoomController::USE_GESTURE_DETECTOR);
      CompositorParent::SetPanZoomControllerForLayerTree(mLayersId,
                                                         mPanZoomController);
    }
  }
}

RenderFrameParent::~RenderFrameParent()
{}

void
RenderFrameParent::Destroy()
{
  size_t numChildren = ManagedPLayersParent().Length();
  NS_ABORT_IF_FALSE(0 == numChildren || 1 == numChildren,
                    "render frame must only have 0 or 1 layer manager");

  if (numChildren) {
    ShadowLayersParent* layers =
      static_cast<ShadowLayersParent*>(ManagedPLayersParent()[0]);
    layers->Destroy();
  }

  mFrameLoaderDestroyed = true;
}

nsContentView*
RenderFrameParent::GetContentView(ViewID aId)
{
  return FindViewForId(mContentViews, aId);
}

void
RenderFrameParent::ContentViewScaleChanged(nsContentView* aView)
{
  
  
  BuildViewMap();
}

void
RenderFrameParent::ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                       const TargetConfig& aTargetConfig,
                                       bool isFirstPaint)
{
  
  
  
  BuildViewMap();

  TriggerRepaint();
}

already_AddRefed<Layer>
RenderFrameParent::BuildLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              LayerManager* aManager,
                              const nsIntRect& aVisibleRect,
                              nsDisplayItem* aItem,
                              const ContainerParameters& aContainerParameters)
{
  NS_ABORT_IF_FALSE(aFrame,
                    "makes no sense to have a shadow tree without a frame");
  NS_ABORT_IF_FALSE(!mContainer ||
                    IsTempLayerManager(aManager) ||
                    mContainer->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  if (IsTempLayerManager(aManager) ||
      (mContainer && mContainer->Manager() != aManager)) {
    
    
    
    
    
    
    NS_WARNING("Remote iframe not rendered");
    return nullptr;
  }

  uint64_t id = GetLayerTreeId();
  if (0 != id) {
    MOZ_ASSERT(!GetRootLayer());

    nsRefPtr<Layer> layer =
      (aManager->GetLayerBuilder()->GetLeafLayerFor(aBuilder, aItem));
    if (!layer) {
      layer = aManager->CreateRefLayer();
    }
    if (!layer) {
      
      
      return nullptr;
    }
    static_cast<RefLayer*>(layer.get())->SetReferentId(id);
    layer->SetVisibleRegion(aVisibleRect);
    nsIntPoint offset = GetContentRectLayerOffset(aFrame, aBuilder);
    
    
    
    MOZ_ASSERT(aContainerParameters.mOffset == nsIntPoint());
    gfx3DMatrix m =
      gfx3DMatrix::Translation(offset.x, offset.y, 0.0);
    
    
    m.Scale(aContainerParameters.mXScale, aContainerParameters.mYScale, 1.0);
    layer->SetBaseTransform(m);

    return layer.forget();
  }

  if (mContainer) {
    ClearContainer(mContainer);
    mContainer->SetPreScale(1.0f, 1.0f);
    mContainer->SetPostScale(1.0f, 1.0f);
    mContainer->SetInheritedScale(1.0f, 1.0f);
  }

  ContainerLayer* shadowRoot = GetRootLayer();
  if (!shadowRoot) {
    mContainer = nullptr;
    return nullptr;
  }

  NS_ABORT_IF_FALSE(!shadowRoot || shadowRoot->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  
  if (!mContainer) {
    mContainer = aManager->CreateContainerLayer();
  }
  NS_ABORT_IF_FALSE(!mContainer->GetFirstChild(),
                    "container of shadow tree shouldn't have a 'root' here");

  mContainer->InsertAfter(shadowRoot, nullptr);

  AssertInTopLevelChromeDoc(mContainer, aFrame);
  ViewTransform transform;
  TransformShadowTree(aBuilder, mFrameLoader, aFrame, shadowRoot, transform);
  mContainer->SetClipRect(nullptr);

  if (mFrameLoader->AsyncScrollEnabled()) {
    const nsContentView* view = GetContentView(FrameMetrics::ROOT_SCROLL_ID);
    BuildBackgroundPatternFor(mContainer,
                              shadowRoot,
                              view->GetViewConfig(),
                              mBackgroundColor,
                              aManager, aFrame);
  }
  mContainer->SetVisibleRegion(aVisibleRect);

  return nsRefPtr<Layer>(mContainer).forget();
}

void
RenderFrameParent::OwnerContentChanged(nsIContent* aContent)
{
  NS_ABORT_IF_FALSE(mFrameLoader->GetOwnerContent() == aContent,
                    "Don't build new map if owner is same!");
  BuildViewMap();
}

void
RenderFrameParent::NotifyInputEvent(const nsInputEvent& aEvent,
                                    nsInputEvent* aOutEvent)
{
  if (mPanZoomController) {
    mPanZoomController->ReceiveInputEvent(aEvent, aOutEvent);
  }
}

void
RenderFrameParent::NotifyDimensionsChanged(int width, int height)
{
  if (mPanZoomController) {
    mPanZoomController->UpdateCompositionBounds(
      nsIntRect(0, 0, width, height));
  }
}

void
RenderFrameParent::ActorDestroy(ActorDestroyReason why)
{
  if (mLayersId != 0) {
    CompositorParent::DeallocateLayerTreeId(mLayersId);
    if (mContentController) {
      
      
      mContentController->ClearRenderFrame();
      mPanZoomController->Destroy();
    }
  }

  if (mFrameLoader && mFrameLoader->GetCurrentRemoteFrame() == this) {
    
    
    
    
    
    
    mFrameLoader->SetCurrentRemoteFrame(nullptr);
  }
  mFrameLoader = nullptr;
}

bool
RenderFrameParent::RecvNotifyCompositorTransaction()
{
  TriggerRepaint();
  return true;
}

bool
RenderFrameParent::RecvCancelDefaultPanZoom()
{
  if (mPanZoomController) {
    mPanZoomController->CancelDefaultPanZoom();
  }
  return true;
}

bool
RenderFrameParent::RecvDetectScrollableSubframe()
{
  if (mPanZoomController) {
    mPanZoomController->DetectScrollableSubframe();
  }
  return true;
}

PLayersParent*
RenderFrameParent::AllocPLayers()
{
  if (!mFrameLoader || mFrameLoaderDestroyed) {
    return nullptr;
  }
  nsRefPtr<LayerManager> lm = GetFrom(mFrameLoader);
  return new ShadowLayersParent(lm->AsShadowManager(), this, 0);
}

bool
RenderFrameParent::DeallocPLayers(PLayersParent* aLayers)
{
  delete aLayers;
  return true;
}

void
RenderFrameParent::BuildViewMap()
{
  ViewMap newContentViews;
  
  if (GetRootLayer() && mFrameLoader->GetPrimaryFrameOfOwningContent()) {
    
    
    
    
    
    

    for (ViewMap::const_iterator iter = mContentViews.begin();
         iter != mContentViews.end();
         ++iter) {
      iter->second->mFrameLoader = NULL;
    }

    mozilla::layout::BuildViewMap(mContentViews, newContentViews, mFrameLoader, GetRootLayer());
  }

  
  
  
  
  if (newContentViews.empty()) {
    newContentViews[FrameMetrics::ROOT_SCROLL_ID] =
      FindViewForId(mContentViews, FrameMetrics::ROOT_SCROLL_ID);
  }

  mContentViews = newContentViews;
}

void
RenderFrameParent::TriggerRepaint()
{
  mFrameLoader->SetCurrentRemoteFrame(this);

  nsIFrame* docFrame = mFrameLoader->GetPrimaryFrameOfOwningContent();
  if (!docFrame) {
    
    
    
    
    
    return;
  }

  docFrame->SchedulePaint();
}

ShadowLayersParent*
RenderFrameParent::GetShadowLayers() const
{
  const InfallibleTArray<PLayersParent*>& shadowParents = ManagedPLayersParent();
  NS_ABORT_IF_FALSE(shadowParents.Length() <= 1,
                    "can only support at most 1 ShadowLayersParent");
  return (shadowParents.Length() == 1) ?
    static_cast<ShadowLayersParent*>(shadowParents[0]) : nullptr;
}

uint64_t
RenderFrameParent::GetLayerTreeId() const
{
  return mLayersId;
}

ContainerLayer*
RenderFrameParent::GetRootLayer() const
{
  ShadowLayersParent* shadowLayers = GetShadowLayers();
  return shadowLayers ? shadowLayers->GetRoot() : nullptr;
}

void
RenderFrameParent::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                    nsSubDocumentFrame* aFrame,
                                    const nsRect& aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  
  nsDisplayList shadowTree;
  ContainerLayer* container = GetRootLayer();
  if (aBuilder->IsForEventDelivery() && container) {
    ViewTransform offset =
      ViewTransform(GetContentRectLayerOffset(aFrame, aBuilder), 1, 1);
    BuildListForLayer(container, mFrameLoader, offset,
                      aBuilder, shadowTree, aFrame);
  } else {
    shadowTree.AppendToTop(
      new (aBuilder) nsDisplayRemote(aBuilder, aFrame, this));
  }

  
  nsPoint offset = aBuilder->ToReferenceFrame(aFrame);
  nsRect bounds = aFrame->EnsureInnerView()->GetBounds() + offset;

  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayClip(aBuilder, aFrame, &shadowTree,
                                 bounds));
}

void
RenderFrameParent::ZoomToRect(const gfxRect& aRect)
{
  if (mPanZoomController) {
    mPanZoomController->ZoomToRect(aRect);
  }
}

void
RenderFrameParent::ContentReceivedTouch(bool aPreventDefault)
{
  if (mPanZoomController) {
    mPanZoomController->ContentReceivedTouch(aPreventDefault);
  }
}

void
RenderFrameParent::UpdateZoomConstraints(bool aAllowZoom, float aMinZoom, float aMaxZoom)
{
  if (mPanZoomController) {
    mPanZoomController->UpdateZoomConstraints(aAllowZoom, aMinZoom, aMaxZoom);
  }
}

}  
}  

already_AddRefed<Layer>
nsDisplayRemote::BuildLayer(nsDisplayListBuilder* aBuilder,
                            LayerManager* aManager,
                            const ContainerParameters& aContainerParameters)
{
  int32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect visibleRect = GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
  visibleRect += aContainerParameters.mOffset;
  nsRefPtr<Layer> layer = mRemoteFrame->BuildLayer(aBuilder, mFrame, aManager, visibleRect, this, aContainerParameters);
  return layer.forget();
}


void
nsDisplayRemoteShadow::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                         HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  
  
  
  
  
  if (aState->mShadows) {
    aState->mShadows->AppendElement(mId);
  }
}
