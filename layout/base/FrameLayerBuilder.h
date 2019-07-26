




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
class ThebesLayer;
}

class FrameLayerBuilder;
class LayerManagerData;
class ThebesLayerData;

enum LayerState {
  LAYER_NONE,
  LAYER_INACTIVE,
  LAYER_ACTIVE,
  
  
  LAYER_ACTIVE_FORCE,
  
  LAYER_ACTIVE_EMPTY,
  
  LAYER_SVG_EFFECTS
};

class RefCountedRegion {
private:
  ~RefCountedRegion() {}
public:
  NS_INLINE_DECL_REFCOUNTING(RefCountedRegion)

  RefCountedRegion() : mIsInfinite(false) {}
  nsRegion mRegion;
  bool mIsInfinite;
};

struct ContainerLayerParameters {
  ContainerLayerParameters() :
    mXScale(1), mYScale(1), mAncestorClipRect(nullptr),
    mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
    mDisableSubpixelAntialiasingInDescendants(false)
  {}
  ContainerLayerParameters(float aXScale, float aYScale) :
    mXScale(aXScale), mYScale(aYScale), mAncestorClipRect(nullptr),
    mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
    mDisableSubpixelAntialiasingInDescendants(false)
  {}
  ContainerLayerParameters(float aXScale, float aYScale,
                           const nsIntPoint& aOffset,
                           const ContainerLayerParameters& aParent) :
    mXScale(aXScale), mYScale(aYScale), mAncestorClipRect(nullptr),
    mOffset(aOffset),
    mInTransformedSubtree(aParent.mInTransformedSubtree),
    mInActiveTransformedSubtree(aParent.mInActiveTransformedSubtree),
    mDisableSubpixelAntialiasingInDescendants(aParent.mDisableSubpixelAntialiasingInDescendants)
  {}
  float mXScale, mYScale;
  



  const nsIntRect* mAncestorClipRect;
  


  nsIntPoint mOffset;

  bool mInTransformedSubtree;
  bool mInActiveTransformedSubtree;
  bool mDisableSubpixelAntialiasingInDescendants;
  



  bool AllowResidualTranslation()
  {
    
    
    
    return mInTransformedSubtree && !mInActiveTransformedSubtree;
  }
};







































class FrameLayerBuilder : public layers::LayerUserData {
public:
  typedef layers::ContainerLayer ContainerLayer;
  typedef layers::Layer Layer;
  typedef layers::ThebesLayer ThebesLayer;
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
            ThebesLayerData* aLayerData = nullptr);

  



  void DidBeginRetainedLayerTransaction(LayerManager* aManager);

  


  void WillEndTransaction();

  


  void DidEndTransaction();

  enum {
    CONTAINER_NOT_CLIPPED_BY_ANCESTORS = 0x01
  };
  
















  already_AddRefed<ContainerLayer>
  BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsIFrame* aContainerFrame,
                         nsDisplayItem* aContainerItem,
                         const nsDisplayList& aChildren,
                         const ContainerLayerParameters& aContainerParameters,
                         const gfx3DMatrix* aTransform,
                         uint32_t aFlags = 0);

  









  Layer* GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                         nsDisplayItem* aItem);

  



  static void InvalidateAllLayers(LayerManager* aManager);
  static void InvalidateAllLayersForFrame(nsIFrame *aFrame);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              mozilla::layers::DrawRegionClip aClip,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef MOZ_DUMP_PAINTING
  



  static void DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile = stdout, bool aDumpHtml = false);
#endif

  
  



  









  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           const DisplayItemClip& aClip,
                           LayerState aLayerState,
                           const nsPoint& aTopLeft,
                           BasicLayerManager* aManager,
                           nsAutoPtr<nsDisplayItemGeometry> aGeometry);

  






  void AddThebesDisplayItem(ThebesLayerData* aLayer,
                            nsDisplayItem* aItem,
                            const DisplayItemClip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState,
                            const nsPoint& aTopLeft,
                            nsAutoPtr<nsDisplayItemGeometry> aGeometry);

  



  static const FramePropertyDescriptor* GetDescriptorForManager(LayerManager* aManager);

  




  Layer* GetOldLayerFor(nsDisplayItem* aItem, 
                        nsDisplayItemGeometry** aOldGeometry = nullptr, 
                        DisplayItemClip** aOldClip = nullptr,
                        nsTArray<nsIFrame*>* aChangedFrames = nullptr,
                        bool *aIsInvalid = nullptr);

  static Layer* GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame);

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  



  static bool HasRetainedDataFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  class DisplayItemData;
  typedef void (*DisplayItemDataCallback)(nsIFrame *aFrame, DisplayItemData* aItem);

  static void IterateRetainedDataFor(nsIFrame* aFrame, DisplayItemDataCallback aCallback);

  




  void SaveLastPaintOffset(ThebesLayer* aLayer);
  




  nsIntPoint GetLastPaintOffset(ThebesLayer* aLayer);

  






  static gfxSize GetThebesLayerScaleForFrame(nsIFrame* aFrame);

  





  void StoreOptimizedLayerForFrame(nsDisplayItem* aItem, Layer* aLayer);
  
  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  












  


  class DisplayItemData MOZ_FINAL {
  public:
    friend class FrameLayerBuilder;

    uint32_t GetDisplayItemKey() { return mDisplayItemKey; }
    Layer* GetLayer() { return mLayer; }
    void Invalidate() { mIsInvalid = true; }

  private:
    DisplayItemData(LayerManagerData* aParent, uint32_t aKey, Layer* aLayer, LayerState aLayerState, uint32_t aGeneration);
    DisplayItemData(DisplayItemData &toCopy);

    



    ~DisplayItemData();

    NS_INLINE_DECL_REFCOUNTING(DisplayItemData)


    



    void AddFrame(nsIFrame* aFrame);
    void RemoveFrame(nsIFrame* aFrame);
    void GetFrameListChanges(nsDisplayItem* aOther, nsTArray<nsIFrame*>& aOut);

    






    void UpdateContents(Layer* aLayer, LayerState aState,
                        uint32_t aContainerLayerGeneration, nsDisplayItem* aItem = nullptr);

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
  class ThebesLayerItemsEntry : public nsPtrHashKey<ThebesLayer> {
  public:
    ThebesLayerItemsEntry(const ThebesLayer *key) :
        nsPtrHashKey<ThebesLayer>(key), mContainerLayerFrame(nullptr),
        mContainerLayerGeneration(0),
        mHasExplicitLastPaintOffset(false), mCommonClipCount(0) {}
    ThebesLayerItemsEntry(const ThebesLayerItemsEntry &toCopy) :
      nsPtrHashKey<ThebesLayer>(toCopy.mKey), mItems(toCopy.mItems)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    nsTArray<ClippedDisplayItem> mItems;
    nsIFrame* mContainerLayerFrame;
    
    
    nsIntPoint mLastPaintOffset;
    uint32_t mContainerLayerGeneration;
    bool mHasExplicitLastPaintOffset;
    



    uint32_t mCommonClipCount;

    enum { ALLOW_MEMMOVE = true };
  };

  



  ThebesLayerItemsEntry* GetThebesLayerItemsEntry(ThebesLayer* aLayer)
  {
    return mThebesLayerItems.GetEntry(aLayer);
  }

  ThebesLayerData* GetContainingThebesLayerData()
  {
    return mContainingThebesLayer;
  }

  



  void SetLayerTreeCompressionMode() { mInLayerTreeCompressionMode = true; }
  bool CheckInLayerTreeCompressionMode();

protected:
  void RemoveThebesItemsAndOwnerDataForLayerSubtree(Layer* aLayer,
                                                    bool aRemoveThebesItems,
                                                    bool aRemoveOwnerData);

  static PLDHashOperator ProcessRemovedDisplayItems(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                                    void* aUserArg);
  static PLDHashOperator RestoreDisplayItemData(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                                void *aUserArg);

  static PLDHashOperator RestoreThebesLayerItemEntries(ThebesLayerItemsEntry* aEntry,
                                                       void *aUserArg);

  




  bool CheckDOMModified();

  



  LayerManager*                       mRetainingManager;
  


  nsRefPtr<nsRootPresContext>         mRootPresContext;

  


  nsDisplayListBuilder*               mDisplayListBuilder;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;

  



  ThebesLayerData*                    mContainingThebesLayer;

  


  uint32_t                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  bool                                mInLayerTreeCompressionMode;

  uint32_t                            mContainerLayerGeneration;
  uint32_t                            mMaxContainerLayerGeneration;
};

}

#endif 
