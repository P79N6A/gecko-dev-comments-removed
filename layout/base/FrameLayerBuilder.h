




#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsRegion.h"
#include "nsIFrame.h"

class nsDisplayListBuilder;
class nsDisplayList;
class nsDisplayItem;
class gfxContext;
class nsRootPresContext;

namespace mozilla {
namespace layers {
class ContainerLayer;
class LayerManager;
class ThebesLayer;
}

class FrameLayerBuilder;

enum LayerState {
  LAYER_NONE,
  LAYER_INACTIVE,
  LAYER_ACTIVE,
  
  
  LAYER_ACTIVE_FORCE,
  
  LAYER_ACTIVE_EMPTY,
  
  LAYER_SVG_EFFECTS
};

class RefCountedRegion : public RefCounted<RefCountedRegion> {
public:
  RefCountedRegion() : mIsInfinite(false) {}
  nsRegion mRegion;
  bool mIsInfinite;
};








































class FrameLayerBuilder {
public:
  typedef layers::ContainerLayer ContainerLayer;
  typedef layers::Layer Layer;
  typedef layers::ThebesLayer ThebesLayer;
  typedef layers::LayerManager LayerManager;

  FrameLayerBuilder() :
    mRetainingManager(nullptr),
    mDetectedDOMModification(false),
    mInvalidateAllLayers(false),
    mContainerLayerGeneration(0),
    mMaxContainerLayerGeneration(0)
  {
    MOZ_COUNT_CTOR(FrameLayerBuilder);
    mNewDisplayItemData.Init();
    mThebesLayerItems.Init();
  }
  ~FrameLayerBuilder()
  {
    MOZ_COUNT_DTOR(FrameLayerBuilder);
  }

  static void Shutdown();

  void Init(nsDisplayListBuilder* aBuilder, LayerManager* aManager);

  



  void DidBeginRetainedLayerTransaction(LayerManager* aManager);

  




  void WillEndTransaction(LayerManager* aManager);

  




  void DidEndTransaction(LayerManager* aManager);

  struct ContainerParameters {
    ContainerParameters() :
      mXScale(1), mYScale(1),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
      mDisableSubpixelAntialiasingInDescendants(false)
    {}
    ContainerParameters(float aXScale, float aYScale) :
      mXScale(aXScale), mYScale(aYScale),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
      mDisableSubpixelAntialiasingInDescendants(false)
    {}
    ContainerParameters(float aXScale, float aYScale,
                        const ContainerParameters& aParent) :
      mXScale(aXScale), mYScale(aYScale),
      mInTransformedSubtree(aParent.mInTransformedSubtree),
      mInActiveTransformedSubtree(aParent.mInActiveTransformedSubtree),
      mDisableSubpixelAntialiasingInDescendants(aParent.mDisableSubpixelAntialiasingInDescendants)
    {}
    float mXScale, mYScale;
    bool mInTransformedSubtree;
    bool mInActiveTransformedSubtree;
    bool mDisableSubpixelAntialiasingInDescendants;
    



    bool AllowResidualTranslation()
    {
      
      
      
      return mInTransformedSubtree && !mInActiveTransformedSubtree;
    }
  };
  
















  already_AddRefed<ContainerLayer>
  BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsIFrame* aContainerFrame,
                         nsDisplayItem* aContainerItem,
                         const nsDisplayList& aChildren,
                         const ContainerParameters& aContainerParameters,
                         const gfx3DMatrix* aTransform);

  









  Layer* GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsDisplayItem* aItem);

  






  static void InvalidateThebesLayerContents(nsIFrame* aFrame,
                                            const nsRect& aRect);

  





  static void InvalidateThebesLayersInSubtree(nsIFrame* aFrame);

  



  static void InvalidateThebesLayersInSubtreeWithUntrustedFrameGeometry(nsIFrame* aFrame);

  



  static void InvalidateAllLayers(LayerManager* aManager);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef MOZ_DUMP_PAINTING
  



  static void DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile = stdout, bool aDumpHtml = false);
#endif

  
  



  


  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           LayerState aLayerState);

  


  void AddLayerDisplayItemForFrame(Layer* aLayer,
                                   nsIFrame* aFrame,
                                   uint32_t aDisplayItemKey,
                                   LayerState aLayerState);

  





  struct Clip;
  void AddThebesDisplayItem(ThebesLayer* aLayer,
                            nsDisplayItem* aItem,
                            const Clip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState);

  






  Layer* GetOldLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  




  Layer* GetOldLayerFor(nsDisplayItem* aItem);

  static Layer* GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);
  




  nscolor FindOpaqueColorCovering(nsDisplayListBuilder* aBuilder,
                                  ThebesLayer* aLayer, const nsRect& aRect);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame)
  {
    aFrame->Properties().Delete(LayerManagerDataProperty());
  }

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  






  static bool NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                 nsDisplayItem* aItem);

  





  static bool HasRetainedLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  



  void SaveLastPaintOffset(ThebesLayer* aLayer);
  




  nsIntPoint GetLastPaintOffset(ThebesLayer* aLayer);

  






  static gfxSize GetThebesLayerScaleForFrame(nsIFrame* aFrame);

  



  struct Clip {
    struct RoundedRect {
      nsRect mRect;
      
      nscoord mRadii[8];

      bool operator==(const RoundedRect& aOther) const {
        if (!mRect.IsEqualInterior(aOther.mRect)) {
          return false;
        }

        NS_FOR_CSS_HALF_CORNERS(corner) {
          if (mRadii[corner] != aOther.mRadii[corner]) {
            return false;
          }
        }
        return true;
      }
      bool operator!=(const RoundedRect& aOther) const {
        return !(*this == aOther);
      }
    };
    nsRect mClipRect;
    nsTArray<RoundedRect> mRoundedClipRects;
    bool mHaveClipRect;

    Clip() : mHaveClipRect(false) {}

    
    Clip(const Clip& aOther, nsDisplayItem* aClipItem);

    
    
    
    void ApplyTo(gfxContext* aContext, nsPresContext* aPresContext,
                 uint32_t aBegin = 0, uint32_t aEnd = PR_UINT32_MAX);

    void ApplyRectTo(gfxContext* aContext, int32_t A2D) const;
    
    
    
    void ApplyRoundedRectsTo(gfxContext* aContext, int32_t A2DPRInt32,
                             uint32_t aBegin, uint32_t aEnd) const;

    
    void DrawRoundedRectsTo(gfxContext* aContext, int32_t A2D,
                            uint32_t aBegin, uint32_t aEnd) const;
    
    void AddRoundedRectPathTo(gfxContext* aContext, int32_t A2D,
                              const RoundedRect &aRoundRect) const;

    
    
    
    nsRect ApproximateIntersect(const nsRect& aRect) const;

    
    
    
    
    bool IsRectClippedByRoundedCorner(const nsRect& aRect) const;

    
    nsRect NonRoundedIntersection() const;

    
    void RemoveRoundedCorners();

    bool operator==(const Clip& aOther) const {
      return mHaveClipRect == aOther.mHaveClipRect &&
             (!mHaveClipRect || mClipRect.IsEqualInterior(aOther.mClipRect)) &&
             mRoundedClipRects == aOther.mRoundedClipRects;
    }
    bool operator!=(const Clip& aOther) const {
      return !(*this == aOther);
    }
  };

protected:
  




  class DisplayItemData {
  public:
    DisplayItemData(Layer* aLayer, uint32_t aKey, LayerState aLayerState, uint32_t aGeneration);
    ~DisplayItemData();

    nsRefPtr<Layer> mLayer;
    uint32_t        mDisplayItemKey;
    uint32_t        mContainerLayerGeneration;
    LayerState      mLayerState;
  };

  static void RemoveFrameFromLayerManager(nsIFrame* aFrame, void* aPropertyValue);

  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  




  class DisplayItemDataEntry : public nsPtrHashKey<nsIFrame> {
  public:
    DisplayItemDataEntry(const nsIFrame *key)
      : nsPtrHashKey<nsIFrame>(key)
      , mIsSharingContainerLayer(false)
      {}
    DisplayItemDataEntry(DisplayItemDataEntry &toCopy)
      : nsPtrHashKey<nsIFrame>(toCopy.mKey)
      , mIsSharingContainerLayer(toCopy.mIsSharingContainerLayer)
    {
      
      
      mData.SwapElements(toCopy.mData);
      mInvalidRegion.swap(toCopy.mInvalidRegion);
      mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
    }

    bool HasNonEmptyContainerLayer();

    nsAutoTArray<DisplayItemData, 1> mData;
    nsRefPtr<RefCountedRegion> mInvalidRegion;
    uint32_t mContainerLayerGeneration;
    bool mIsSharingContainerLayer;

    enum { ALLOW_MEMMOVE = false };
  };

  
  friend class LayerManagerData;

  
  static void FlashPaint(gfxContext *aContext);

  






  static nsTArray<DisplayItemData>* GetDisplayItemDataArrayForFrame(nsIFrame *aFrame);

  





  static PLDHashOperator RemoveDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aClosure)
  {
    return UpdateDisplayItemDataForFrame(aEntry, nullptr);
  }

  









  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, const Clip& aClip, uint32_t aGeneration)
      : mItem(aItem), mClip(aClip), mContainerLayerGeneration(aGeneration)
    {
    }

    nsDisplayItem* mItem;
    Clip mClip;
    uint32_t mContainerLayerGeneration;
    bool mInactiveLayer;
  };

  



public:
  class ThebesLayerItemsEntry : public nsPtrHashKey<ThebesLayer> {
  public:
    ThebesLayerItemsEntry(const ThebesLayer *key) :
        nsPtrHashKey<ThebesLayer>(key), mContainerLayerFrame(nullptr),
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

protected:
  void RemoveThebesItemsForLayerSubtree(Layer* aLayer);

  static void SetAndClearInvalidRegion(DisplayItemDataEntry* aEntry);
  static PLDHashOperator UpdateDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aUserArg);
  static PLDHashOperator StoreNewDisplayItemData(DisplayItemDataEntry* aEntry,
                                                 void* aUserArg);
  static PLDHashOperator RestoreDisplayItemData(DisplayItemDataEntry* aEntry,
                                                void *aUserArg);

  static PLDHashOperator RestoreThebesLayerItemEntries(ThebesLayerItemsEntry* aEntry,
                                                       void *aUserArg);

  




  bool CheckDOMModified();

  



  LayerManager*                       mRetainingManager;
  


  nsRootPresContext*                  mRootPresContext;
  



  nsTHashtable<DisplayItemDataEntry>  mNewDisplayItemData;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;
  


  uint32_t                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  uint32_t                            mContainerLayerGeneration;
  uint32_t                            mMaxContainerLayerGeneration;
};

}

#endif 
