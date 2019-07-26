




#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsRegion.h"
#include "nsIFrame.h"
#include "nsDisplayListInvalidation.h"
#include "LayerTreeInvalidation.h"
#include "ImageLayers.h"

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
class LayerManagerData;

enum LayerState {
  LAYER_NONE,
  LAYER_INACTIVE,
  LAYER_ACTIVE,
  
  
  LAYER_ACTIVE_FORCE,
  
  LAYER_ACTIVE_EMPTY,
  
  LAYER_SVG_EFFECTS
};

extern uint8_t gLayerManagerSecondary;

class LayerManagerSecondary : public layers::LayerUserData {
};

class RefCountedRegion : public RefCounted<RefCountedRegion> {
public:
  RefCountedRegion() : mIsInfinite(false) {}
  nsRegion mRegion;
  bool mIsInfinite;
};







































class FrameLayerBuilder : public layers::LayerUserData {
public:
  typedef layers::ContainerLayer ContainerLayer;
  typedef layers::Layer Layer;
  typedef layers::ThebesLayer ThebesLayer;
  typedef layers::ImageLayer ImageLayer;
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

  


  void WillEndTransaction();

  


  void DidEndTransaction();

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
                        const nsIntPoint& aOffset,
                        const ContainerParameters& aParent) :
      mXScale(aXScale), mYScale(aYScale),
      mOffset(aOffset),
      mInTransformedSubtree(aParent.mInTransformedSubtree),
      mInActiveTransformedSubtree(aParent.mInActiveTransformedSubtree),
      mDisableSubpixelAntialiasingInDescendants(aParent.mDisableSubpixelAntialiasingInDescendants)
    {}
    float mXScale, mYScale;

    


    nsIntPoint mOffset;

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
                         nsDisplayItem* aItem);

  



  static void InvalidateAllLayers(LayerManager* aManager);
  static void InvalidateAllLayersForFrame(nsIFrame *aFrame);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef MOZ_DUMP_PAINTING
  



  static void DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile = stdout, bool aDumpHtml = false);
#endif

  
  



  









  struct Clip;
  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           const Clip& aClip,
                           LayerState aLayerState,
                           const nsPoint& aTopLeft,
                           LayerManager* aManager = nullptr);

  






  void AddThebesDisplayItem(ThebesLayer* aLayer,
                            nsDisplayItem* aItem,
                            const Clip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState,
                            const nsPoint& aTopLeft);

  



  static void SetWidgetLayerManager(LayerManager* aManager)
  {
    LayerManagerSecondary* secondary = 
      static_cast<LayerManagerSecondary*>(aManager->GetUserData(&gLayerManagerSecondary));
    sWidgetManagerSecondary = !!secondary;
  }

  



  static const FramePropertyDescriptor* GetDescriptorForManager(LayerManager* aManager);

  



  static LayerManagerData* GetManagerData(nsIFrame* aFrame, LayerManager* aManager = nullptr);

  



  static void SetManagerData(nsIFrame* aFrame, LayerManagerData* aData);

  



  static void ClearManagerData(nsIFrame* aFrame);

  



  static void ClearManagerData(nsIFrame* aFrame, LayerManagerData* aData);

  




  Layer* GetOldLayerFor(nsDisplayItem* aItem, nsDisplayItemGeometry** aOldGeometry = nullptr, Clip** aOldClip = nullptr);

  static Layer* GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  




  LayerManager* GetInactiveLayerManagerFor(nsDisplayItem* aItem);

  




  nscolor FindOpaqueColorCovering(nsDisplayListBuilder* aBuilder,
                                  ThebesLayer* aLayer, const nsRect& aRect);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame);

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  






  static bool NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                 nsDisplayItem* aItem);

  








  static bool HasRetainedLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey, LayerManager* aManager);

  



  static bool HasRetainedDataFor(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  




  void SaveLastPaintOffset(ThebesLayer* aLayer);
  




  nsIntPoint GetLastPaintOffset(ThebesLayer* aLayer);

  






  static gfxSize GetThebesLayerScaleForFrame(nsIFrame* aFrame);

  





  void StoreOptimizedLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey, Layer* aImage);

  



  struct Clip {
    struct RoundedRect {
      nsRect mRect;
      
      nscoord mRadii[8];

      RoundedRect operator+(const nsPoint& aOffset) const {
        RoundedRect r = *this;
        r.mRect += aOffset;
        return r;
      }
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
                 uint32_t aBegin = 0, uint32_t aEnd = UINT32_MAX);

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

    
    
    void AddOffsetAndComputeDifference(const nsPoint& aPoint, const nsRect& aBounds,
                                       const Clip& aOther, const nsRect& aOtherBounds,
                                       nsRegion* aDifference);

    bool operator==(const Clip& aOther) const {
      return mHaveClipRect == aOther.mHaveClipRect &&
             (!mHaveClipRect || mClipRect.IsEqualInterior(aOther.mClipRect)) &&
             mRoundedClipRects == aOther.mRoundedClipRects;
    }
    bool operator!=(const Clip& aOther) const {
      return !(*this == aOther);
    }
  };
  
  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerSecondaryDataProperty,
                                               RemoveFrameFromLayerManager)

protected:
  




  class DisplayItemData {
  public:
    DisplayItemData(Layer* aLayer, uint32_t aKey, LayerState aLayerState, uint32_t aGeneration);
    DisplayItemData()
      : mUsed(false)
    {}
    DisplayItemData(DisplayItemData &toCopy);
    ~DisplayItemData();

    NS_INLINE_DECL_REFCOUNTING(DisplayItemData)

    void AddFrame(nsIFrame* aFrame)
    {
      mFrameList.AppendElement(aFrame);
    }

    bool FrameListMatches(nsDisplayItem* aOther);

    nsRefPtr<Layer> mLayer;
    nsRefPtr<Layer> mOptLayer;
    nsRefPtr<LayerManager> mInactiveManager;
    nsAutoTArray<nsIFrame*, 2> mFrameList;
    nsAutoPtr<nsDisplayItemGeometry> mGeometry;
    Clip            mClip;
    uint32_t        mDisplayItemKey;
    uint32_t        mContainerLayerGeneration;
    LayerState      mLayerState;

    




    bool            mUsed;
  };

  static void RemoveFrameFromLayerManager(nsIFrame* aFrame, void* aPropertyValue);

  






  DisplayItemData* GetOldLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey);

  





  class DisplayItemDataEntry : public nsPtrHashKey<nsIFrame> {
  public:
    DisplayItemDataEntry(const nsIFrame *key)
      : nsPtrHashKey<nsIFrame>(key)
    { 
      MOZ_COUNT_CTOR(DisplayItemDataEntry); 
    }
    DisplayItemDataEntry(DisplayItemDataEntry &toCopy)
      : nsPtrHashKey<nsIFrame>(toCopy.mKey)
    {
      MOZ_COUNT_CTOR(DisplayItemDataEntry);
      
      
      mData.SwapElements(toCopy.mData);
      mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
    }
    ~DisplayItemDataEntry() { MOZ_COUNT_DTOR(DisplayItemDataEntry); }

    bool HasNonEmptyContainerLayer();

    nsAutoTArray<nsRefPtr<DisplayItemData>, 1> mData;
    uint32_t mContainerLayerGeneration;

    enum { ALLOW_MEMMOVE = false };
  };

  
  friend class LayerManagerData;

  



  void StoreDataForFrame(nsIFrame* aFrame, DisplayItemData* data);

  
  static void FlashPaint(gfxContext *aContext);

  






  nsTArray<nsRefPtr<DisplayItemData> >* GetDisplayItemDataArrayForFrame(nsIFrame *aFrame);

  



  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey, 
                                                       LayerManager* aManager);
  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey);
  static DisplayItemData* GetDisplayItemDataForManager(nsDisplayItem* aItem, LayerManager* aManager);
  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       uint32_t aDisplayItemKey, 
                                                       LayerManagerData* aData);

  




  static PLDHashOperator RemoveDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aClosure);

  









  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, const Clip& aClip, uint32_t aGeneration)
      : mItem(aItem), mClip(aClip), mContainerLayerGeneration(aGeneration)
    {
    }

    ~ClippedDisplayItem();

    nsDisplayItem* mItem;

    




    nsRefPtr<LayerManager> mInactiveLayer;

    Clip mClip;
    uint32_t mContainerLayerGeneration;
  };

  



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

  static PLDHashOperator ProcessRemovedDisplayItems(DisplayItemDataEntry* aEntry,
                                                    void* aUserArg);
protected:
  void RemoveThebesItemsAndOwnerDataForLayerSubtree(Layer* aLayer,
                                                    bool aRemoveThebesItems,
                                                    bool aRemoveOwnerData);

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
  


  nsRefPtr<nsRootPresContext>         mRootPresContext;

  


  nsDisplayListBuilder*               mDisplayListBuilder;
  



  nsTHashtable<DisplayItemDataEntry>  mNewDisplayItemData;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;
  


  uint32_t                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  uint32_t                            mContainerLayerGeneration;
  uint32_t                            mMaxContainerLayerGeneration;

  



  static bool                         sWidgetManagerSecondary;
};

}

#endif 
