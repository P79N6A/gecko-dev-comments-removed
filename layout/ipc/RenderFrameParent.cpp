







































#include "mozilla/layers/ShadowLayersParent.h"

#include "BasicLayers.h"
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
SetTransformFor(ContainerLayer* aContainer, nsIFrame* aContainedFrame,
                const FrameMetrics& aMetrics, const ViewportConfig& aConfig,
                nsDisplayListBuilder* aBuilder)
{
  NS_ABORT_IF_FALSE(aContainer && aContainedFrame, "args must be nonnull");
  AssertInTopLevelChromeDoc(aContainer, aContainedFrame);

  nscoord auPerDevPixel = aContainedFrame->PresContext()->AppUnitsPerDevPixel();
  
  nsPoint frameOffset =
    (aBuilder->ToReferenceFrame(aContainedFrame->GetParent()) +
     aContainedFrame->GetContentRect().TopLeft());
  nsIntPoint translation = frameOffset.ToNearestPixels(auPerDevPixel);

  
  
  
  
  
  
  nsIntPoint scrollCompensation =
    (aConfig.mScrollOffset.ToNearestPixels(auPerDevPixel) -
     aMetrics.mViewportScrollOffset);
  translation -= scrollCompensation;

  gfxMatrix transform;
  transform.Translate(gfxPoint(translation.x, translation.y));
  transform.Scale(aConfig.mXScale, aConfig.mYScale);
  aContainer->SetTransform(gfx3DMatrix::From2D(transform));
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
    
    
    
    
    NS_WARNING("RenderFrameParent just received a layer update, but our <browser> doesn't have an nsIFrame so we can't invalidate for the update");
    return;
  }

  
  
  
  
  
  
  
  
  
  nsRect rect = nsRect(nsPoint(0, 0), docFrame->GetRect().Size());
  docFrame->InvalidateWithFlags(rect, nsIFrame::INVALIDATE_NO_THEBES_LAYERS);
}

already_AddRefed<Layer>
RenderFrameParent::BuildLayer(nsDisplayListBuilder* aBuilder,
                              nsIFrame* aFrame,
                              LayerManager* aManager)
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
    SetTransformFor(mContainer, aFrame,
                    shadowRoot->GetFrameMetrics(),
                    mFrameLoader->GetViewportConfig(),
                    aBuilder);
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
  if (LayerManager::LAYERS_BASIC != lm->GetBackendType()) {
    NS_WARNING("shadow layers no sprechen GL backend yet");
    return nsnull;
  }    

  BasicShadowLayerManager* bslm = static_cast<BasicShadowLayerManager*>(lm);
  ShadowLayersParent* slp = new ShadowLayersParent(bslm);
  bslm->SetForwarder(nsnull);   
  bslm->SetForwarder(slp);
  return slp;
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
  nsRefPtr<Layer> layer = mRemoteFrame->BuildLayer(aBuilder, mFrame, aManager);
  return layer.forget();
}
