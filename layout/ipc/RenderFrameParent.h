






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
class AsyncPanZoomController;
class GestureEventListener;
class TargetConfig;
class ShadowLayersParent;
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
  typedef mozilla::layers::ShadowLayersParent ShadowLayersParent;
  typedef mozilla::FrameLayerBuilder::ContainerParameters ContainerParameters;
  typedef FrameMetrics::ViewID ViewID;

public:
  typedef std::map<ViewID, nsRefPtr<nsContentView> > ViewMap;

  




  RenderFrameParent(nsFrameLoader* aFrameLoader,
                    ScrollingBehavior aScrollingBehavior,
                    mozilla::layers::LayersBackend* aBackendType,
                    int* aMaxTextureSize,
                    uint64_t* aId);
  virtual ~RenderFrameParent();

  void Destroy();

  



  nsContentView* GetContentView(ViewID aId = FrameMetrics::ROOT_SCROLL_ID);

  void ContentViewScaleChanged(nsContentView* aView);

  virtual void ShadowLayersUpdated(ShadowLayersParent* aLayerTree,
                                   const TargetConfig& aTargetConfig,
                                   bool isFirstPaint) MOZ_OVERRIDE;

  void BuildDisplayList(nsDisplayListBuilder* aBuilder,
                        nsSubDocumentFrame* aFrame,
                        const nsRect& aDirtyRect,
                        const nsDisplayListSet& aLists);

  already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                     nsIFrame* aFrame,
                                     LayerManager* aManager,
                                     const nsIntRect& aVisibleRect,
                                     nsDisplayItem* aItem,
                                     const ContainerParameters& aContainerParameters);

  void OwnerContentChanged(nsIContent* aContent);

  void SetBackgroundColor(nscolor aColor) { mBackgroundColor = gfxRGBA(aColor); };

  void NotifyInputEvent(const nsInputEvent& aEvent);

  void ApplyZoomCompensationToEvent(nsInputEvent* aEvent);

  void NotifyDimensionsChanged(int width, int height);

  void ZoomToRect(const gfxRect& aRect);

  void ContentReceivedTouch(bool aPreventDefault);

  void UpdateZoomConstraints(bool aAllowZoom, float aMinZoom, float aMaxZoom);

protected:
  void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  virtual bool RecvNotifyCompositorTransaction() MOZ_OVERRIDE;

  virtual bool RecvCancelDefaultPanZoom() MOZ_OVERRIDE;
  virtual bool RecvDetectScrollableSubframe() MOZ_OVERRIDE;

  virtual PLayersParent* AllocPLayers() MOZ_OVERRIDE;
  virtual bool DeallocPLayers(PLayersParent* aLayers) MOZ_OVERRIDE;

private:
  void BuildViewMap();
  void TriggerRepaint();
  void DispatchEventForPanZoomController(const InputEvent& aEvent);

  ShadowLayersParent* GetShadowLayers() const;
  uint64_t GetLayerTreeId() const;
  ContainerLayer* GetRootLayer() const;

  
  
  
  uint64_t mLayersId;

  nsRefPtr<nsFrameLoader> mFrameLoader;
  nsRefPtr<ContainerLayer> mContainer;
  
  
  
  nsRefPtr<layers::AsyncPanZoomController> mPanZoomController;
  nsRefPtr<RemoteContentController> mContentController;

  
  
  ViewMap mContentViews;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool mFrameLoaderDestroyed;
  
  gfxRGBA mBackgroundColor;
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
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE
  { return mozilla::LAYER_ACTIVE_FORCE; }

  virtual already_AddRefed<Layer>
  BuildLayer(nsDisplayListBuilder* aBuilder, LayerManager* aManager,
             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;

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
