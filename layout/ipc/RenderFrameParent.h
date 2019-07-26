






#ifndef mozilla_layout_RenderFrameParent_h
#define mozilla_layout_RenderFrameParent_h

#include "mozilla/Attributes.h"
#include <map>

#include "mozilla/layout/PRenderFrameParent.h"
#include "mozilla/layers/ShadowLayersManager.h"
#include "nsDisplayList.h"
#include "RenderFrameUtils.h"

class nsContentView;
class nsFrameLoader;
class nsSubDocumentFrame;

namespace mozilla {

class InputEvent;

namespace layers {
class APZCTreeManager;
class GestureEventListener;
class TargetConfig;
class LayerTransactionParent;
struct TextureFactoryIdentifier;
struct ScrollableLayerGuid;
}

namespace layout {

class RemoteContentController;

class RenderFrameParent : public PRenderFrameParent,
                          public mozilla::layers::ShadowLayersManager
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
  typedef std::map<ViewID, nsRefPtr<nsContentView> > ViewMap;


  




  RenderFrameParent(nsFrameLoader* aFrameLoader,
                    ScrollingBehavior aScrollingBehavior,
                    TextureFactoryIdentifier* aTextureFactoryIdentifier,
                    uint64_t* aId, bool* aSuccess);
  virtual ~RenderFrameParent();

  void Destroy();

  



  nsContentView* GetContentView(ViewID aId);
  nsContentView* GetRootContentView();

  void ContentViewScaleChanged(nsContentView* aView);

  virtual void ShadowLayersUpdated(LayerTransactionParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool aIsFirstPaint,
                                   bool aScheduleComposite,
                                   uint32_t aPaintSequenceNumber) MOZ_OVERRIDE;

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

  virtual PLayerTransactionParent* AllocPLayerTransactionParent() MOZ_OVERRIDE;
  virtual bool DeallocPLayerTransactionParent(PLayerTransactionParent* aLayers) MOZ_OVERRIDE;

private:
  void BuildViewMap();
  void TriggerRepaint();
  void DispatchEventForPanZoomController(const InputEvent& aEvent);

  LayerTransactionParent* GetShadowLayers() const;
  uint64_t GetLayerTreeId() const;
  Layer* GetRootLayer() const;

  
  
  
  uint64_t mLayersId;

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsRefPtr<ContainerLayer> mContainer;
  
  
  
  nsRefPtr<layers::APZCTreeManager> mApzcTreeManager;
  nsRefPtr<RemoteContentController> mContentController;

  layers::APZCTreeManager* GetApzcTreeManager();

  
  
  ViewMap mContentViews;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
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

  nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = false;
    return mRect;
  }

  virtual uint32_t GetPerFrameKey() MOZ_OVERRIDE
  {
    NS_ABORT();
    return 0;
  }

  void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
               HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("Remote-Shadow", TYPE_REMOTE_SHADOW)

private:
  nsRect mRect;
  ViewID mId;
};

#endif  
