






#ifndef mozilla_layout_RenderFrameParent_h
#define mozilla_layout_RenderFrameParent_h

#include "mozilla/Attributes.h"
#include <map>

#include "mozilla/layout/PRenderFrameParent.h"
#include "nsDisplayList.h"
#include "RenderFrameUtils.h"

class nsFrameLoader;
class nsSubDocumentFrame;

namespace mozilla {

class InputEvent;

namespace layers {
class APZCTreeManager;
class TargetConfig;
class LayerTransactionParent;
struct TextureFactoryIdentifier;
struct ScrollableLayerGuid;
}

namespace layout {

class RemoteContentController;

class RenderFrameParent : public PRenderFrameParent
{
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::ContainerLayer ContainerLayer;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::TargetConfig TargetConfig;
  typedef mozilla::layers::LayerTransactionParent LayerTransactionParent;
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;
  typedef mozilla::layers::TextureFactoryIdentifier TextureFactoryIdentifier;
  typedef mozilla::layers::ScrollableLayerGuid ScrollableLayerGuid;
  typedef mozilla::layers::ZoomConstraints ZoomConstraints;
  typedef FrameMetrics::ViewID ViewID;

public:


  




  RenderFrameParent(nsFrameLoader* aFrameLoader,
                    ScrollingBehavior aScrollingBehavior,
                    TextureFactoryIdentifier* aTextureFactoryIdentifier,
                    uint64_t* aId, bool* aSuccess);
  virtual ~RenderFrameParent();

  void Destroy();

  void BuildDisplayList(nsDisplayListBuilder* aBuilder,
                        nsSubDocumentFrame* aFrame,
                        const nsRect& aDirtyRect,
                        const nsDisplayListSet& aLists);

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame,
                                     LayerManager* aManager,
                                     const nsIntRect& aVisibleRect,
                                     nsDisplayItem* aItem,
                                     const ContainerLayerParameters& aContainerParameters);

  void OwnerContentChanged(nsIContent* aContent);

  void SetBackgroundColor(nscolor aColor) { mBackgroundColor = gfxRGBA(aColor); };

  








  nsEventStatus NotifyInputEvent(WidgetInputEvent& aEvent,
                                 ScrollableLayerGuid* aOutTargetGuid);

  void ZoomToRect(uint32_t aPresShellId, ViewID aViewId, const CSSRect& aRect);

  void ContentReceivedTouch(const ScrollableLayerGuid& aGuid,
                            bool aPreventDefault);

  void UpdateZoomConstraints(uint32_t aPresShellId,
                             ViewID aViewId,
                             bool aIsRoot,
                             const ZoomConstraints& aConstraints);

  bool HitTest(const nsRect& aRect);

protected:
  void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual bool RecvNotifyCompositorTransaction() MOZ_OVERRIDE;

  virtual bool RecvUpdateHitRegion(const nsRegion& aRegion) MOZ_OVERRIDE;

private:
  void TriggerRepaint();
  void DispatchEventForPanZoomController(const InputEvent& aEvent);

  uint64_t GetLayerTreeId() const;

  
  
  
  uint64_t mLayersId;

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsRefPtr<ContainerLayer> mContainer;
  
  
  
  nsRefPtr<layers::APZCTreeManager> mApzcTreeManager;
  nsRefPtr<RemoteContentController> mContentController;

  layers::APZCTreeManager* GetApzcTreeManager();

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mFrameLoaderDestroyed;
  
  gfxRGBA mBackgroundColor;

  nsRegion mTouchRegion;
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

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  { return mozilla::LAYER_ACTIVE_FORCE; }

  virtual already_AddRefed<Layer>
  BuildLayer(nsDisplayListBuilder* aBuilder, LayerManager* aManager,
             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
               HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("Remote", TYPE_REMOTE)

private:
  RenderFrameParent* mRemoteFrame;
};


#endif  
