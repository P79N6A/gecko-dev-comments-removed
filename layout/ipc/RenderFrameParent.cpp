







































#include "mozilla/layers/ShadowLayersParent.h"

#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "RenderFrameParent.h"

#include "gfx3DMatrix.h"
#include "nsFrameLoader.h"
#include "nsViewportFrame.h"
#include "nsSubDocumentFrame.h"

typedef nsContentView::ViewConfig ViewConfig;
using namespace mozilla::layers;

namespace mozilla {
namespace layout {

typedef FrameMetrics::ViewID ViewID;
typedef RenderFrameParent::ViewMap ViewMap;


struct ViewTransform {
  ViewTransform(nsIntPoint aTranslation, float aXScale, float aYScale)
    : mTranslation(aTranslation)
    , mXScale(aXScale)
    , mYScale(aYScale)
  {}

  operator gfx3DMatrix() const
  {
    return
      gfx3DMatrix::Scale(mXScale, mYScale, 1) *
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

static void Translate(gfx3DMatrix& aTransform, nsIntPoint aTranslate)
{
  aTransform._41 += aTranslate.x;
  aTransform._42 += aTranslate.y;
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
    (aContainer->Manager()->GetBackendType() != LayerManager::LAYERS_BASIC) ||
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

static void
AssertValidContainerOfShadowTree(ContainerLayer* aContainer,
                                 Layer* aShadowRoot)
{
  NS_ABORT_IF_FALSE(
    !aContainer || (aShadowRoot &&
                    aShadowRoot == aContainer->GetFirstChild() &&
                    nsnull == aShadowRoot->GetNextSibling()),
    "container of shadow tree may only be null or have 1 child that is the shadow root");
}

static const FrameMetrics*
GetFrameMetrics(Layer* aLayer)
{
  
  
  if (!aLayer->GetFirstChild())
    return NULL;

  ContainerLayer* container = static_cast<ContainerLayer*>(aLayer);
  return &container->GetFrameMetrics();
}

static nsIntPoint
GetRootFrameOffset(nsIFrame* aContainerFrame, nsDisplayListBuilder* aBuilder)
{
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();

  
  nsPoint frameOffset =
    (aBuilder->ToReferenceFrame(aContainerFrame->GetParent()) +
     aContainerFrame->GetContentRect().TopLeft());

  return frameOffset.ToNearestPixels(auPerDevPixel);
}






static ViewTransform
ComputeShadowTreeTransform(nsIFrame* aContainerFrame,
                           nsFrameLoader* aRootFrameLoader,
                           const FrameMetrics* aMetrics,
                           const ViewConfig& aConfig,
                           nsDisplayListBuilder* aBuilder,
                           float aInverseScaleX,
                           float aInverseScaleY)
{
  
  
  
  
  
  
  
  
  
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntPoint scrollOffset =
    aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel);
  nsIntPoint metricsScrollOffset = aMetrics->mViewportScrollOffset;

  if (aRootFrameLoader->AsyncScrollEnabled()) {
    nsIntPoint scrollCompensation(
      scrollOffset.x * aInverseScaleX - metricsScrollOffset.x * aConfig.mXScale,
      scrollOffset.y * aInverseScaleY - metricsScrollOffset.y * aConfig.mYScale);

    return ViewTransform(-scrollCompensation, aConfig.mXScale, aConfig.mYScale);
  } else {
    return ViewTransform(nsIntPoint(0, 0), 1, 1);
  }
}


static void
BuildListForLayer(Layer* aLayer,
                  nsFrameLoader* aRootFrameLoader,
                  gfx3DMatrix aTransform,
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
      aSubdocFrame, aRootFrameLoader, metrics, view->GetViewConfig(), aBuilder,
      1 / GetXScale(aTransform), 1 / GetYScale(aTransform));
    transform = applyTransform * aLayer->GetTransform() * aTransform;

    
    
    Scale(aTransform, GetXScale(applyTransform), GetYScale(applyTransform));

    
    nsRect bounds;
    {
      nscoord auPerDevPixel = aSubdocFrame->PresContext()->AppUnitsPerDevPixel();
      bounds = metrics->mViewport.ToAppUnits(auPerDevPixel);
      ApplyTransform(bounds, aTransform, auPerDevPixel);

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
                    float aXScale = 1, float aYScale = 1)
{
  ShadowLayer* shadow = aLayer->AsShadowLayer();
  shadow->SetShadowClipRect(aLayer->GetClipRect());
  shadow->SetShadowVisibleRegion(aLayer->GetVisibleRegion());

  const FrameMetrics* metrics = GetFrameMetrics(aLayer);

  gfx3DMatrix shadowTransform;

  if (metrics && metrics->IsScrollable()) {
    const ViewID scrollId = metrics->mScrollId;
    const nsContentView* view =
      aFrameLoader->GetCurrentRemoteFrame()->GetContentView(scrollId);
    NS_ABORT_IF_FALSE(view, "Array of views should be consistent with layer tree");

    ViewTransform viewTransform = ComputeShadowTreeTransform(
      aFrame, aFrameLoader, metrics, view->GetViewConfig(), aBuilder,
      1 / aXScale, 1 / aYScale
    );

    if (metrics->IsRootScrollable()) {
      viewTransform.mTranslation += GetRootFrameOffset(aFrame, aBuilder);
    }

    shadowTransform = gfx3DMatrix(viewTransform) * aLayer->GetTransform();

  } else {
    shadowTransform = aLayer->GetTransform();
  }

  shadow->SetShadowTransform(shadowTransform);
  aXScale *= GetXScale(shadowTransform);
  aYScale *= GetYScale(shadowTransform);

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    TransformShadowTree(aBuilder, aFrameLoader, aFrame, child, aXScale, aYScale);
  }
}

static Layer*
ShadowRootOf(ContainerLayer* aContainer)
{
  NS_ABORT_IF_FALSE(aContainer, "need a non-null container");

  Layer* shadowRoot = aContainer->GetFirstChild();
  NS_ABORT_IF_FALSE(!shadowRoot || nsnull == shadowRoot->GetNextSibling(),
                    "shadow root container may only have 0 or 1 children");
  return shadowRoot;
}





static PRBool
IsTempLayerManager(LayerManager* aManager)
{
  return (LayerManager::LAYERS_BASIC == aManager->GetBackendType() &&
          !static_cast<BasicLayerManager*>(aManager)->IsRetained());
}







static void
BuildViewMap(ViewMap& oldContentViews, ViewMap& newContentViews,
                   nsFrameLoader* aFrameLoader, Layer* aLayer,
                   float aXScale = 1, float aYScale = 1)
{
  if (!aLayer->GetFirstChild())
    return;

  ContainerLayer* container = static_cast<ContainerLayer*>(aLayer);
  const FrameMetrics metrics = container->GetFrameMetrics();
  const ViewID scrollId = metrics.mScrollId;

  nscoord auPerDevPixel = aFrameLoader->GetPrimaryFrameOfOwningContent()
                                      ->PresContext()->AppUnitsPerDevPixel();
  nsContentView* view = FindViewForId(oldContentViews, scrollId);
  if (view) {
    
    
    ViewConfig config = view->GetViewConfig();
    aXScale *= config.mXScale;
    aYScale *= config.mYScale;
    view->mOwnerContent = aFrameLoader->GetOwnerContent();
  } else {
    
    
    
    ViewConfig config;
    config.mScrollOffset = nsPoint(
      NSIntPixelsToAppUnits(metrics.mViewportScrollOffset.x, auPerDevPixel) * aXScale,
      NSIntPixelsToAppUnits(metrics.mViewportScrollOffset.y, auPerDevPixel) * aYScale);
    view = new nsContentView(aFrameLoader->GetOwnerContent(), scrollId, config);
  }

  newContentViews.insert(ViewMap::value_type(scrollId, view));

  for (Layer* child = aLayer->GetFirstChild();
       child; child = child->GetNextSibling()) {
    const gfx3DMatrix transform = aLayer->GetTransform();
    aXScale *= GetXScale(transform);
    aYScale *= GetYScale(transform);
    BuildViewMap(oldContentViews, newContentViews, aFrameLoader, child,
                 aXScale, aYScale);
  }
}

RenderFrameParent::RenderFrameParent(nsFrameLoader* aFrameLoader)
  : mFrameLoader(aFrameLoader)
{
  NS_ABORT_IF_FALSE(aFrameLoader, "Need a frameloader here");
  mContentViews.insert(ViewMap::value_type(
    FrameMetrics::ROOT_SCROLL_ID,
    new nsContentView(aFrameLoader->GetOwnerContent(), FrameMetrics::ROOT_SCROLL_ID)
  ));
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
}

nsContentView*
RenderFrameParent::GetContentView(ViewID aId)
{
  return FindViewForId(mContentViews, aId);
}

void
RenderFrameParent::ShadowLayersUpdated()
{
  mFrameLoader->SetCurrentRemoteFrame(this);

  
  
  
  BuildViewMap();

  nsIFrame* docFrame = mFrameLoader->GetPrimaryFrameOfOwningContent();
  if (!docFrame) {
    
    
    
    
    
    return;
  }

  
  
  
  
  
  
  
  
  
  nsRect rect = nsRect(nsPoint(0, 0), docFrame->GetRect().Size());
  docFrame->InvalidateWithFlags(rect, nsIFrame::INVALIDATE_NO_THEBES_LAYERS);
}

already_AddRefed<Layer>
RenderFrameParent::BuildLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              LayerManager* aManager,
                              const nsIntRect& aVisibleRect)
{
  NS_ABORT_IF_FALSE(aFrame,
                    "makes no sense to have a shadow tree without a frame");
  NS_ABORT_IF_FALSE(!mContainer ||
                    IsTempLayerManager(aManager) ||
                    mContainer->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  if (mContainer && mContainer->Manager() != aManager) {
    
    
    
    
    
    
    return nsnull;
  }

  Layer* containerShadowRoot = mContainer ? ShadowRootOf(mContainer) : nsnull;
  ContainerLayer* shadowRoot = GetRootLayer();
  NS_ABORT_IF_FALSE(!shadowRoot || shadowRoot->Manager() == aManager,
                    "retaining manager changed out from under us ... HELP!");

  if (mContainer && shadowRoot != containerShadowRoot) {
    
    
    if (containerShadowRoot) {
      mContainer->RemoveChild(containerShadowRoot);
    }
  }

  if (!shadowRoot) {
    
    mContainer = nsnull;
  } else if (shadowRoot != containerShadowRoot) {
    
    if (!mContainer) {
      mContainer = aManager->CreateContainerLayer();
    }
    NS_ABORT_IF_FALSE(!mContainer->GetFirstChild(),
                      "container of shadow tree shouldn't have a 'root' here");

    mContainer->InsertAfter(shadowRoot, nsnull);
  }

  if (mContainer) {
    AssertInTopLevelChromeDoc(mContainer, aFrame);
    TransformShadowTree(aBuilder, mFrameLoader, aFrame, shadowRoot);
    mContainer->SetClipRect(nsnull);
  }

  AssertValidContainerOfShadowTree(mContainer, shadowRoot);
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
RenderFrameParent::ActorDestroy(ActorDestroyReason why)
{
  if (mFrameLoader->GetCurrentRemoteFrame() == this) {
    
    
    
    
    
    
    mFrameLoader->SetCurrentRemoteFrame(nsnull);
  }
  mFrameLoader = nsnull;
}

PLayersParent*
RenderFrameParent::AllocPLayers()
{
  LayerManager* lm = GetLayerManager();
  switch (lm->GetBackendType()) {
  case LayerManager::LAYERS_BASIC: {
    BasicShadowLayerManager* bslm = static_cast<BasicShadowLayerManager*>(lm);
    return new ShadowLayersParent(bslm);
  }
  case LayerManager::LAYERS_OPENGL: {
    LayerManagerOGL* lmo = static_cast<LayerManagerOGL*>(lm);
    return new ShadowLayersParent(lmo);
  }
  default: {
    NS_WARNING("shadow layers no sprechen D3D backend yet");
    return nsnull;
  }
  }
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
  if (GetRootLayer()) {
    
    
    
    
    
    

    for (ViewMap::const_iterator iter = mContentViews.begin();
         iter != mContentViews.end();
         ++iter) {
      iter->second->mOwnerContent = NULL;
    }

    mozilla::layout::BuildViewMap(mContentViews, newContentViews, mFrameLoader, GetRootLayer());
  }

  
  
  if (!newContentViews.empty()) {
    mContentViews = newContentViews;
  }
}

LayerManager*
RenderFrameParent::GetLayerManager() const
{
  nsIDocument* doc = mFrameLoader->GetOwnerDoc();
  return doc->GetShell()->GetLayerManager();
}

ShadowLayersParent*
RenderFrameParent::GetShadowLayers() const
{
  const nsTArray<PLayersParent*>& shadowParents = ManagedPLayersParent();
  NS_ABORT_IF_FALSE(shadowParents.Length() <= 1,
                    "can only support at most 1 ShadowLayersParent");
  return (shadowParents.Length() == 1) ?
    static_cast<ShadowLayersParent*>(shadowParents[0]) : nsnull;
}

ContainerLayer*
RenderFrameParent::GetRootLayer() const
{
  ShadowLayersParent* shadowLayers = GetShadowLayers();
  return shadowLayers ? shadowLayers->GetRoot() : nsnull;
}

NS_IMETHODIMP
RenderFrameParent::BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                    nsSubDocumentFrame* aFrame,
                                    const nsRect& aDirtyRect,
                                    const nsDisplayListSet& aLists)
{
  
  
  nsDisplayList shadowTree;
  if (aBuilder->IsForEventDelivery()) {
    nsRect bounds = aFrame->EnsureInnerView()->GetBounds();
    ViewTransform offset =
      ViewTransform(GetRootFrameOffset(aFrame, aBuilder), 1, 1);
    BuildListForLayer(GetRootLayer(), mFrameLoader, offset,
                      aBuilder, shadowTree, aFrame);
  } else {
    shadowTree.AppendToTop(
      new (aBuilder) nsDisplayRemote(aBuilder, aFrame, this));
  }

  
  nsPoint offset = aFrame->GetOffsetToCrossDoc(aBuilder->ReferenceFrame());
  nsRect bounds = aFrame->EnsureInnerView()->GetBounds() + offset;

  return aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayClip(aBuilder, aFrame, &shadowTree,
                                 bounds));
}

}  
}  

already_AddRefed<Layer>
nsDisplayRemote::BuildLayer(nsDisplayListBuilder* aBuilder,
                            LayerManager* aManager)
{
  PRInt32 appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  nsIntRect visibleRect = GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
  nsRefPtr<Layer> layer = mRemoteFrame->BuildLayer(aBuilder, mFrame, aManager, visibleRect);
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
