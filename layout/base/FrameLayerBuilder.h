




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
      mXScale(1), mYScale(1), mAncestorClipRect(nullptr),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
      mDisableSubpixelAntialiasingInDescendants(false)
    {}
    ContainerParameters(float aXScale, float aYScale) :
      mXScale(aXScale), mYScale(aYScale), mAncestorClipRect(nullptr),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false),
      mDisableSubpixelAntialiasingInDescendants(false)
    {}
    ContainerParameters(float aXScale, float aYScale,
                        const nsIntPoint& aOffset,
                        const ContainerParameters& aParent) :
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
  enum {
    CONTAINER_NOT_CLIPPED_BY_ANCESTORS = 0x01
  };
  
















  already_AddRefed<ContainerLayer>
  BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsIFrame* aContainerFrame,
                         nsDisplayItem* aContainerItem,
                         const nsDisplayList& aChildren,
                         const ContainerParameters& aContainerParameters,
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
                           LayerManager* aManager,
                           nsAutoPtr<nsDisplayItemGeometry> aGeometry);

  






  void AddThebesDisplayItem(ThebesLayer* aLayer,
                            nsDisplayItem* aItem,
                            const Clip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState,
                            const nsPoint& aTopLeft,
                            nsAutoPtr<nsDisplayItemGeometry> aGeometry);

  



  static const FramePropertyDescriptor* GetDescriptorForManager(LayerManager* aManager);

  




  Layer* GetOldLayerFor(nsDisplayItem* aItem, 
                        nsDisplayItemGeometry** aOldGeometry = nullptr, 
                        Clip** aOldClip = nullptr,
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

    
    
    nsRect ApplyNonRoundedIntersection(const nsRect& aRect) const;

    
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

  












  


  class DisplayItemData {
  public:
    friend class FrameLayerBuilder;

    uint32_t GetDisplayItemKey() { return mDisplayItemKey; }
    Layer* GetLayer() { return mLayer; }
    void Invalidate() { mIsInvalid = true; }
  protected:

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
    nsRefPtr<LayerManager> mInactiveManager;
    nsAutoTArray<nsIFrame*, 1> mFrameList;
    nsAutoPtr<nsDisplayItemGeometry> mGeometry;
    Clip            mClip;
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
    ClippedDisplayItem(nsDisplayItem* aItem, const Clip& aClip, uint32_t aGeneration)
      : mItem(aItem), mClip(aClip), mContainerLayerGeneration(aGeneration)
    {
    }

    ~ClippedDisplayItem();

    nsDisplayItem* mItem;

    




    nsRefPtr<LayerManager> mInactiveLayerManager;

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
  


  uint32_t                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  uint32_t                            mContainerLayerGeneration;
  uint32_t                            mMaxContainerLayerGeneration;
};

}

#endif 
