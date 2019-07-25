







































#ifndef mozilla_layout_RenderFrameParent_h
#define mozilla_layout_RenderFrameParent_h

#include "mozilla/layout/PRenderFrameParent.h"

#include "nsDisplayList.h"
#include "Layers.h"

class nsFrameLoader;
class nsSubDocumentFrame;

namespace mozilla {

namespace layers {
class ShadowLayersParent;
}

namespace layout {

class RenderFrameParent : public PRenderFrameParent
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ContainerLayer ContainerLayer;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::ShadowLayersParent ShadowLayersParent;

public:
  RenderFrameParent(nsFrameLoader* aFrameLoader);
  virtual ~RenderFrameParent();

  void Destroy();

  void ShadowLayersUpdated();

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder* aBuilder,
                              nsSubDocumentFrame* aFrame,
                              const nsRect& aDirtyRect,
                              const nsDisplayListSet& aLists);

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame,
                                     LayerManager* aManager,
                                     const nsIntRect& aVisibleRect);

protected:
  NS_OVERRIDE void ActorDestroy(ActorDestroyReason why);

  NS_OVERRIDE virtual PLayersParent* AllocPLayers();
  NS_OVERRIDE virtual bool DeallocPLayers(PLayersParent* aLayers);

private:
  LayerManager* GetLayerManager() const;
  ShadowLayersParent* GetShadowLayers() const;
  ContainerLayer* GetRootLayer() const;

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








class nsDisplayRemoteShadow : public nsDisplayItem
{
  typedef mozilla::layout::RenderFrameParent RenderFrameParent;
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;

public:
  nsDisplayRemoteShadow(nsDisplayListBuilder* aBuilder,
                        nsIFrame* aFrame,
                        nsRect aRect,
                        ViewID aId)
    : nsDisplayItem(aBuilder, aFrame)
    , mRect(aRect)
    , mId(aId)
  {}

  NS_OVERRIDE nsRect GetBounds(nsDisplayListBuilder* aBuilder)
  {
    return mRect;
  }

  virtual PRUint32 GetPerFrameKey()
  {
    NS_ABORT();
    return 0;
  }

  NS_OVERRIDE void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                           HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);

  NS_DISPLAY_DECL_NAME("Remote-Shadow", TYPE_REMOTE_SHADOW)

private:
  nsRect mRect;
  ViewID mId;
};

#endif  
