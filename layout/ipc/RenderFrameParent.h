







































#ifndef mozilla_layout_RenderFrameParent_h
#define mozilla_layout_RenderFrameParent_h

#include "mozilla/layout/PRenderFrameParent.h"

#include "nsDisplayList.h"

class nsFrameLoader;

namespace mozilla {
namespace layers {
class ContainerLayer;
class Layer;
class LayerManager;
class ShadowLayersParent;
}

namespace layout {

class RenderFrameParent : public PRenderFrameParent
{
  typedef mozilla::layers::ContainerLayer ContainerLayer;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::ShadowLayersParent ShadowLayersParent;

public:
  RenderFrameParent(nsFrameLoader* aFrameLoader);
  virtual ~RenderFrameParent();

  void ShadowLayersUpdated();

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame,
                                     LayerManager* aManager);

protected:
  NS_OVERRIDE void ActorDestroy(ActorDestroyReason why);

  NS_OVERRIDE virtual PLayersParent* AllocPLayers();
  NS_OVERRIDE virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  LayerManager* GetLayerManager() const;
  ShadowLayersParent* GetShadowLayers() const;
  Layer* GetRootLayer() const;

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsRefPtr<ContainerLayer> mContainer;
};

} 
} 






class nsDisplayRemote : public nsDisplayItem
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;

public:
  nsDisplayRemote(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                  RenderFrameParent* aRemoteFrame)
    : nsDisplayItem(aBuilder, aFrame)
    , mRemoteFrame(aRemoteFrame)
  {}

  NS_OVERRIDE
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager)
  { return mozilla::LAYER_ACTIVE; }  

  NS_OVERRIDE
  virtual already_AddRefed<Layer>
  BuildLayer(nsDisplayListBuilder* aBuilder, LayerManager* aManager);

  NS_DISPLAY_DECL_NAME("Remote", TYPE_REMOTE)

private:
  RenderFrameParent* mRemoteFrame;
};

#endif  
