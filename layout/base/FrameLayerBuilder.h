




































#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsRegion.h"
#include "nsIFrame.h"
#include "Layers.h"

class nsDisplayListBuilder;
class nsDisplayList;
class nsDisplayItem;
class gfxContext;
class nsRootPresContext;

namespace mozilla {

enum LayerState {
  LAYER_NONE,
  LAYER_INACTIVE,
  LAYER_ACTIVE,
  
  
  LAYER_ACTIVE_FORCE,
  
  LAYER_ACTIVE_EMPTY
};








































class FrameLayerBuilder {
public:
  typedef layers::ContainerLayer ContainerLayer; 
  typedef layers::Layer Layer; 
  typedef layers::ThebesLayer ThebesLayer;
  typedef layers::LayerManager LayerManager;

  FrameLayerBuilder() :
    mRetainingManager(nsnull),
    mDetectedDOMModification(PR_FALSE),
    mInvalidateAllLayers(PR_FALSE)
  {
    mNewDisplayItemData.Init();
    mThebesLayerItems.Init();
  }

  void Init(nsDisplayListBuilder* aBuilder);

  



  void DidBeginRetainedLayerTransaction(LayerManager* aManager);

  




  void WillEndTransaction(LayerManager* aManager);

  




  void DidEndTransaction(LayerManager* aManager);

  struct ContainerParameters {
    ContainerParameters() :
      mXScale(1), mYScale(1),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false) {}
    ContainerParameters(float aXScale, float aYScale) :
      mXScale(aXScale), mYScale(aYScale),
      mInTransformedSubtree(false), mInActiveTransformedSubtree(false) {}
    ContainerParameters(float aXScale, float aYScale,
                        const ContainerParameters& aParent) :
      mXScale(aXScale), mYScale(aYScale),
      mInTransformedSubtree(aParent.mInTransformedSubtree),
      mInActiveTransformedSubtree(aParent.mInActiveTransformedSubtree) {}
    float mXScale, mYScale;
    bool mInTransformedSubtree;
    bool mInActiveTransformedSubtree;
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

  



  static void InvalidateAllLayers(LayerManager* aManager);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

  



  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef DEBUG
  



  void DumpRetainedLayerTree();
#endif

  
  


  
  


  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           LayerState aLayerState);

  





  struct Clip;
  void AddThebesDisplayItem(ThebesLayer* aLayer,
                            nsDisplayItem* aItem,
                            const Clip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState);

  






  Layer* GetOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

  




  nscolor FindOpaqueColorCovering(nsDisplayListBuilder* aBuilder,
                                  ThebesLayer* aLayer, const nsRect& aRect);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame)
  {
    aFrame->Properties().Delete(LayerManagerDataProperty());
  }

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  






  static bool NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                 nsDisplayItem* aItem);

  





  static bool HasRetainedLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

  



  void SaveLastPaintOffset(ThebesLayer* aLayer);
  




  nsIntPoint GetLastPaintOffset(ThebesLayer* aLayer);

  



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

    Clip() : mHaveClipRect(PR_FALSE) {}

    
    Clip(const Clip& aOther, nsDisplayItem* aClipItem);

    
    
    void ApplyTo(gfxContext* aContext, nsPresContext* aPresContext);

    
    
    
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
    DisplayItemData(Layer* aLayer, PRUint32 aKey, LayerState aLayerState)
      : mLayer(aLayer), mDisplayItemKey(aKey), mLayerState(aLayerState) {}

    nsRefPtr<Layer> mLayer;
    PRUint32        mDisplayItemKey;
    LayerState    mLayerState;
  };

  static void RemoveFrameFromLayerManager(nsIFrame* aFrame, void* aPropertyValue);

  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  




  class DisplayItemDataEntry : public nsPtrHashKey<nsIFrame> {
  public:
    DisplayItemDataEntry(const nsIFrame *key) : nsPtrHashKey<nsIFrame>(key) {}
    DisplayItemDataEntry(DisplayItemDataEntry &toCopy) :
      nsPtrHashKey<nsIFrame>(toCopy.mKey)
    {
      
      
      mData.SwapElements(toCopy.mData);
    }

    bool HasNonEmptyContainerLayer();

    nsAutoTArray<DisplayItemData, 1> mData;

    enum { ALLOW_MEMMOVE = false };
  };

  
  friend class LayerManagerData;

  






  static nsTArray<DisplayItemData>* GetDisplayItemDataArrayForFrame(nsIFrame *aFrame);

  





  static PLDHashOperator RemoveDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aClosure)
  {
    return UpdateDisplayItemDataForFrame(aEntry, nsnull);
  }

  









  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, const Clip& aClip)
      : mItem(aItem), mClip(aClip)
    {
    }

    nsDisplayItem* mItem;
    Clip mClip;
    bool mInactiveLayer;
  };

  



  class ThebesLayerItemsEntry : public nsPtrHashKey<ThebesLayer> {
  public:
    ThebesLayerItemsEntry(const ThebesLayer *key) :
        nsPtrHashKey<ThebesLayer>(key), mContainerLayerFrame(nsnull),
        mHasExplicitLastPaintOffset(PR_FALSE) {}
    ThebesLayerItemsEntry(const ThebesLayerItemsEntry &toCopy) :
      nsPtrHashKey<ThebesLayer>(toCopy.mKey), mItems(toCopy.mItems)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    nsTArray<ClippedDisplayItem> mItems;
    nsIFrame* mContainerLayerFrame;
    
    
    nsIntPoint mLastPaintOffset;
    bool mHasExplicitLastPaintOffset;

    enum { ALLOW_MEMMOVE = PR_TRUE };
  };

  void RemoveThebesItemsForLayerSubtree(Layer* aLayer);

  static PLDHashOperator UpdateDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aUserArg);
  static PLDHashOperator StoreNewDisplayItemData(DisplayItemDataEntry* aEntry,
                                                 void* aUserArg);

  




  bool CheckDOMModified();

  



  LayerManager*                       mRetainingManager;
  


  nsRootPresContext*                  mRootPresContext;
  



  nsTHashtable<DisplayItemDataEntry>  mNewDisplayItemData;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;
  


  PRUint32                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;
};

}

#endif 
