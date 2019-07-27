




#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsRegion.h"
#include "nsIFrame.h"
#include "ImageLayers.h"
#include "DisplayItemClip.h"
#include "mozilla/layers/LayersTypes.h"
#include "LayerState.h"

class nsDisplayListBuilder;
class nsDisplayList;
class nsDisplayItem;
class gfxContext;
class nsDisplayItemGeometry;

namespace mozilla {
namespace layers {
class ContainerLayer;
class LayerManager;
class BasicLayerManager;
class PaintedLayer;
}

namespace gfx {
class Matrix4x4;
}

class FrameLayerBuilder;
class LayerManagerData;
class PaintedLayerData;
class ContainerState;

class RefCountedRegion {
private:
  ~RefCountedRegion() {}
public:
  NS_INLINE_DECL_REFCOUNTING(RefCountedRegion)

  RefCountedRegion() : mIsInfinite(false) {}
  nsRegion mRegion;
  bool mIsInfinite;
};

struct NewLayerEntry;

struct ContainerLayerParameters {
  ContainerLayerParameters()
    : mXScale(1)
    , mYScale(1)
    , mLayerContentsVisibleRect(nullptr)
    , mBackgroundColor(NS_RGBA(0,0,0,0))
    , mInTransformedSubtree(false)
    , mInActiveTransformedSubtree(false)
    , mDisableSubpixelAntialiasingInDescendants(false)
    , mInLowPrecisionDisplayPort(false)
  {}
  ContainerLayerParameters(float aXScale, float aYScale)
    : mXScale(aXScale)
    , mYScale(aYScale)
    , mLayerContentsVisibleRect(nullptr)
    , mBackgroundColor(NS_RGBA(0,0,0,0))
    , mInTransformedSubtree(false)
    , mInActiveTransformedSubtree(false)
    , mDisableSubpixelAntialiasingInDescendants(false)
    , mInLowPrecisionDisplayPort(false)
  {}
  ContainerLayerParameters(float aXScale, float aYScale,
                           const nsIntPoint& aOffset,
                           const ContainerLayerParameters& aParent)
    : mXScale(aXScale)
    , mYScale(aYScale)
    , mLayerContentsVisibleRect(nullptr)
    , mOffset(aOffset)
    , mBackgroundColor(aParent.mBackgroundColor)
    , mInTransformedSubtree(aParent.mInTransformedSubtree)
    , mInActiveTransformedSubtree(aParent.mInActiveTransformedSubtree)
    , mDisableSubpixelAntialiasingInDescendants(aParent.mDisableSubpixelAntialiasingInDescendants)
    , mInLowPrecisionDisplayPort(aParent.mInLowPrecisionDisplayPort)
  {}

  float mXScale, mYScale;

  LayoutDeviceToLayerScale2D Scale() const {
    return LayoutDeviceToLayerScale2D(mXScale, mYScale);
  }

  



  nsIntRect* mLayerContentsVisibleRect;

  


  nsIntPoint mOffset;

  LayerIntPoint Offset() const {
    return LayerIntPoint::FromUntyped(mOffset);
  }

  nscolor mBackgroundColor;
  bool mInTransformedSubtree;
  bool mInActiveTransformedSubtree;
  bool mDisableSubpixelAntialiasingInDescendants;
  bool mInLowPrecisionDisplayPort;
  



  bool AllowResidualTranslation()
  {
    
    
    
    return mInTransformedSubtree && !mInActiveTransformedSubtree;
  }
};







































class FrameLayerBuilder : public layers::LayerUserData {
public:
  typedef layers::ContainerLayer ContainerLayer;
  typedef layers::Layer Layer;
  typedef layers::PaintedLayer PaintedLayer;
  typedef layers::ImageLayer ImageLayer;
  typedef layers::LayerManager LayerManager;
  typedef layers::BasicLayerManager BasicLayerManager;
  typedef layers::EventRegions EventRegions;

  FrameLayerBuilder() :
    mRetainingManager(nullptr),
    mDetectedDOMModification(false),
    mInvalidateAllLayers(false),
    mInLayerTreeCompressionMode(false),
    mContainerLayerGeneration(0),
    mMaxContainerLayerGeneration(0)
  {
    MOZ_COUNT_CTOR(FrameLayerBuilder);
  }
  ~FrameLayerBuilder()
  {
    MOZ_COUNT_DTOR(FrameLayerBuilder);
  }

  static void Shutdown();

  void Init(nsDisplayListBuilder* aBuilder, LayerManager* aManager,
            PaintedLayerData* aLayerData = nullptr,
            ContainerState* aContainingContainerState = nullptr);

  



  void DidBeginRetainedLayerTransaction(LayerManager* aManager);

  


  void WillEndTransaction();

  


  void DidEndTransaction();

  enum {
    CONTAINER_NOT_CLIPPED_BY_ANCESTORS = 0x01,

    






    CONTAINER_ALLOW_PULL_BACKGROUND_COLOR = 0x02
  };
  



















  already_AddRefed<ContainerLayer>
  BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsIFrame* aContainerFrame,
                         nsDisplayItem* aContainerItem,
                         nsDisplayList* aChildren,
                         const ContainerLayerParameters& aContainerParameters,
                         const gfx::Matrix4x4* aTransform,
                         uint32_t aFlags = 0);

  









  Layer* GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                         nsDisplayItem* aItem);

  



  static void InvalidateAllLayers(LayerManager* aManager);
  static void InvalidateAllLayersForFrame(nsIFrame *aFrame);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  static void DrawPaintedLayer(PaintedLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              mozilla::layers::DrawRegionClip aClip,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

  



  static void DumpRetainedLayerTree(LayerManager* aManager, std::stringstream& aStream, bool aDumpHtml = false);

  







  static nsDisplayItemGeometry* GetMostRecentGeometry(nsDisplayItem* aItem);


  
  



  









  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           LayerState aLayerState,
                           const nsPoint& aTopLeft,
                           BasicLayerManager* aManager);

  






  void AddPaintedDisplayItem(PaintedLayerData* aLayer,
                            nsDisplayItem* aItem,
                            const DisplayItemClip& aClip,
                            ContainerState& aContainerState,
                            LayerState aLayerState,
                            const nsPoint& aTopLeft);

  




  Layer* GetOldLayerFor(nsDisplayItem* aItem, 
                        nsDisplayItemGeometry** aOldGeometry = nullptr, 
                        DisplayItemClip** aOldClip = nullptr);

  void ClearCachedGeometry(nsDisplayItem* aItem);

  static Layer* GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  static Layer* GetDebugSingleOldLayerForFrame(nsIFrame* aFrame);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame);

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  



  static bool HasRetainedDataFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  class DisplayItemData;
  typedef void (*DisplayItemDataCallback)(nsIFrame *aFrame, DisplayItemData* aItem);

  static void IterateRetainedDataFor(nsIFrame* aFrame, DisplayItemDataCallback aCallback);

  




  void SavePreviousDataForLayer(PaintedLayer* aLayer, uint32_t aClipCount);
  




  nsIntPoint GetLastPaintOffset(PaintedLayer* aLayer);

  






  static gfxSize GetPaintedLayerScaleForFrame(nsIFrame* aFrame);

  





  void StoreOptimizedLayerForFrame(nsDisplayItem* aItem, Layer* aLayer);
  
  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  












  


  class DisplayItemData final {
  public:
    friend class FrameLayerBuilder;

    uint32_t GetDisplayItemKey() { return mDisplayItemKey; }
    Layer* GetLayer() { return mLayer; }
    nsDisplayItemGeometry* GetGeometry() const { return mGeometry.get(); }
    void Invalidate() { mIsInvalid = true; }

  private:
    DisplayItemData(LayerManagerData* aParent,
                    uint32_t aKey,
                    Layer* aLayer,
                    nsIFrame* aFrame = nullptr);

    



    ~DisplayItemData();

    NS_INLINE_DECL_REFCOUNTING(DisplayItemData)


    



    void AddFrame(nsIFrame* aFrame);
    void RemoveFrame(nsIFrame* aFrame);
    const nsTArray<nsIFrame*>& GetFrameListChanges();

    







    void BeginUpdate(Layer* aLayer, LayerState aState,
                     uint32_t aContainerLayerGeneration, nsDisplayItem* aItem = nullptr);

    








    void EndUpdate(nsAutoPtr<nsDisplayItemGeometry> aGeometry);
    void EndUpdate();

    LayerManagerData* mParent;
    nsRefPtr<Layer> mLayer;
    nsRefPtr<Layer> mOptLayer;
    nsRefPtr<BasicLayerManager> mInactiveManager;
    nsAutoTArray<nsIFrame*, 1> mFrameList;
    nsAutoPtr<nsDisplayItemGeometry> mGeometry;
    DisplayItemClip mClip;
    uint32_t        mDisplayItemKey;
    uint32_t        mContainerLayerGeneration;
    LayerState      mLayerState;

    



    nsDisplayItem* mItem;
    nsAutoTArray<nsIFrame*, 1> mFrameListChanges;

    



    bool            mUsed;
    bool            mIsInvalid;
  };

protected:

  friend class LayerManagerData;

  static void RemoveFrameFromLayerManager(nsIFrame* aFrame, void* aPropertyValue);

  






  DisplayItemData* GetOldLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  



  DisplayItemData* StoreDataForFrame(nsDisplayItem* aItem, Layer* aLayer, LayerState aState);
  void StoreDataForFrame(nsIFrame* aFrame,
                         uint32_t aDisplayItemKey,
                         Layer* aLayer,
                         LayerState aState);

  
  static void FlashPaint(gfxContext *aContext);

  






  DisplayItemData* GetDisplayItemData(nsIFrame *aFrame, uint32_t aKey);

  



  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey, 
                                                       LayerManager* aManager);
  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey);
  static DisplayItemData* GetDisplayItemDataForManager(nsDisplayItem* aItem, LayerManager* aManager);
  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey, 
                                                       LayerManagerData* aData);

  static PLDHashOperator DumpDisplayItemDataForFrame(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                                     void* aClosure);
  








  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, uint32_t aGeneration)
      : mItem(aItem), mContainerLayerGeneration(aGeneration)
    {
    }

    ~ClippedDisplayItem();

    nsDisplayItem* mItem;

    




    nsRefPtr<LayerManager> mInactiveLayerManager;

    uint32_t mContainerLayerGeneration;

  };

  static void RecomputeVisibilityForItems(nsTArray<ClippedDisplayItem>& aItems,
                                          nsDisplayListBuilder* aBuilder,
                                          const nsIntRegion& aRegionToDraw,
                                          const nsIntPoint& aOffset,
                                          int32_t aAppUnitsPerDevPixel,
                                          float aXScale,
                                          float aYScale);

  void PaintItems(nsTArray<ClippedDisplayItem>& aItems,
                  const nsIntRect& aRect,
                  gfxContext* aContext,
                  nsRenderingContext* aRC,
                  nsDisplayListBuilder* aBuilder,
                  nsPresContext* aPresContext,
                  const nsIntPoint& aOffset,
                  float aXScale, float aYScale,
                  int32_t aCommonClipCount);

  



public:
  class PaintedLayerItemsEntry : public nsPtrHashKey<PaintedLayer> {
  public:
    explicit PaintedLayerItemsEntry(const PaintedLayer *key)
      : nsPtrHashKey<PaintedLayer>(key)
      , mContainerLayerFrame(nullptr)
      , mLastCommonClipCount(0)
      , mContainerLayerGeneration(0)
      , mHasExplicitLastPaintOffset(false)
      , mCommonClipCount(0)
    {}
    PaintedLayerItemsEntry(const PaintedLayerItemsEntry &toCopy) :
      nsPtrHashKey<PaintedLayer>(toCopy.mKey), mItems(toCopy.mItems)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    nsTArray<ClippedDisplayItem> mItems;
    nsIFrame* mContainerLayerFrame;
    
    
    nsIntPoint mLastPaintOffset;
    uint32_t mLastCommonClipCount;

    uint32_t mContainerLayerGeneration;
    bool mHasExplicitLastPaintOffset;
    



    uint32_t mCommonClipCount;

    enum { ALLOW_MEMMOVE = true };
  };

  



  PaintedLayerItemsEntry* GetPaintedLayerItemsEntry(PaintedLayer* aLayer)
  {
    return mPaintedLayerItems.GetEntry(aLayer);
  }

  PaintedLayerData* GetContainingPaintedLayerData()
  {
    return mContainingPaintedLayer;
  }

  bool IsBuildingRetainedLayers()
  {
    return !mContainingPaintedLayer && mRetainingManager;
  }

  ContainerState* GetContainingContainerState()
  {
    return mContainingContainerState;
  }

  



  void SetLayerTreeCompressionMode() { mInLayerTreeCompressionMode = true; }
  bool CheckInLayerTreeCompressionMode();

  void ComputeGeometryChangeForItem(DisplayItemData* aData);

protected:
  static PLDHashOperator ProcessRemovedDisplayItems(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                                    void* aUserArg);
  static PLDHashOperator RestoreDisplayItemData(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                                void *aUserArg);

  static PLDHashOperator RestorePaintedLayerItemEntries(PaintedLayerItemsEntry* aEntry,
                                                       void *aUserArg);

  




  bool CheckDOMModified();

  



  LayerManager*                       mRetainingManager;
  


  nsRefPtr<nsRootPresContext>         mRootPresContext;

  


  nsDisplayListBuilder*               mDisplayListBuilder;
  



  nsTHashtable<PaintedLayerItemsEntry> mPaintedLayerItems;

  



  PaintedLayerData*                   mContainingPaintedLayer;

  ContainerState*                     mContainingContainerState;

  


  uint32_t                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  bool                                mInLayerTreeCompressionMode;

  uint32_t                            mContainerLayerGeneration;
  uint32_t                            mMaxContainerLayerGeneration;
};

}

#endif 
