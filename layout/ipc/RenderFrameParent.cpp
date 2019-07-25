







































#include "mozilla/layers/ShadowLayersParent.h"

#include "BasicLayers.h"
#include "LayerManagerOGL.h"
#include "RenderFrameParent.h"

#include "gfx3DMatrix.h"
#include "nsFrameLoader.h"
#include "nsViewportFrame.h"

typedef nsFrameLoader::ViewportConfig ViewportConfig;
using namespace mozilla::layers;

namespace mozilla {
namespace layout {

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






static void
ComputeShadowTreeTransform(nsIFrame* aContainerFrame,
                           const FrameMetrics& aMetrics,
                           const ViewportConfig& aConfig,
                           nsDisplayListBuilder* aBuilder,
                           nsIntPoint* aShadowTranslation,
                           float* aShadowXScale,
                           float* aShadowYScale)
{
  nscoord auPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
  
  nsPoint frameOffset =
    (aBuilder->ToReferenceFrame(aContainerFrame->GetParent()) +
     aContainerFrame->GetContentRect().TopLeft());
  *aShadowTranslation = frameOffset.ToNearestPixels(auPerDevPixel);

  
  
  
  
  
  
  nsIntPoint scrollCompensation =
    (aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel));
  scrollCompensation.x -= aMetrics.mViewportScrollOffset.x * aConfig.mXScale;
  scrollCompensation.y -= aMetrics.mViewportScrollOffset.y * aConfig.mYScale;
  *aShadowTranslation -= scrollCompensation;

  *aShadowXScale = aConfig.mXScale;
  *aShadowYScale = aConfig.mYScale;
}

static void
UpdateShadowSubtree(Layer* aSubtreeRoot)
{
  ShadowLayer* shadow = aSubtreeRoot->AsShadowLayer();

  shadow->SetShadowClipRect(aSubtreeRoot->GetClipRect());
  shadow->SetShadowTransform(aSubtreeRoot->GetTransform());
  shadow->SetShadowVisibleRegion(aSubtreeRoot->GetVisibleRegion());

  for (Layer* child = aSubtreeRoot->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    UpdateShadowSubtree(child);
  }
}

static void
TransformShadowTreeTo(ContainerLayer* aRoot,
                      const nsIntRect& aVisibleRect,
                      const nsIntPoint& aTranslation,
                      float aXScale, float aYScale)
{
  UpdateShadowSubtree(aRoot);

  ShadowLayer* shadow = aRoot->AsShadowLayer();
  NS_ABORT_IF_FALSE(aRoot->GetTransform() == shadow->GetShadowTransform(),
                    "transforms should be the same now");
  NS_ABORT_IF_FALSE(aRoot->GetTransform().Is2D(),
                    "only 2D transforms expected currently");
  gfxMatrix shadowTransform;
  shadow->GetShadowTransform().Is2D(&shadowTransform);
  
  
  shadowTransform.Translate(gfxPoint(aTranslation.x, aTranslation.y));
  shadowTransform.Scale(aXScale, aYScale);
  shadow->SetShadowTransform(gfx3DMatrix::From2D(shadowTransform));
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

RenderFrameParent::RenderFrameParent(nsFrameLoader* aFrameLoader)
  : mFrameLoader(aFrameLoader)
{}

RenderFrameParent::~RenderFrameParent()
{}

void
RenderFrameParent::ShadowLayersUpdated()
{
  mFrameLoader->SetCurrentRemoteFrame(this);

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
    nsIntPoint shadowTranslation;
    float shadowXScale, shadowYScale;
    ComputeShadowTreeTransform(aFrame,
                               shadowRoot->GetFrameMetrics(),
                               mFrameLoader->GetViewportConfig(),
                               aBuilder,
                               &shadowTranslation,
                               &shadowXScale, &shadowYScale);
    TransformShadowTreeTo(shadowRoot, aVisibleRect,
                          shadowTranslation, shadowXScale, shadowYScale);
    mContainer->SetClipRect(nsnull);
  }

  AssertValidContainerOfShadowTree(mContainer, shadowRoot);
  return nsRefPtr<Layer>(mContainer).forget();
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
