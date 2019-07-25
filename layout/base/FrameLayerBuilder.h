




#ifndef FRAMELAYERBUILDER_H_
#define FRAMELAYERBUILDER_H_

#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsRegion.h"
#include "nsIFrame.h"
#include "Layers.h"
#include "nsDisplayListInvalidation.h"
#include "LayerTreeInvalidation.h"

class nsDisplayListBuilder;
class nsDisplayList;
class nsDisplayItem;
class gfxContext;
class nsRootPresContext;

namespace mozilla {

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

extern PRUint8 gLayerManagerLayerBuilder;
extern PRUint8 gLayerManagerSecondary;

class LayerManagerSecondary : public layers::LayerUserData {
};








































class FrameLayerBuilder : public layers::LayerUserData {
public:
  typedef layers::ContainerLayer ContainerLayer; 
  typedef layers::Layer Layer; 
  typedef layers::ThebesLayer ThebesLayer;
  typedef layers::LayerManager LayerManager;

  FrameLayerBuilder() :
    mRetainingManager(nsnull),
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

  void Init(nsDisplayListBuilder* aBuilder);

  



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
                         nsDisplayItem* aItem);

  



  static void InvalidateAllLayers(LayerManager* aManager);

  




  static Layer* GetDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey);
  
  



  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef MOZ_DUMP_PAINTING
  



  static void DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile = stdout);
#endif

  
  


  
  








  struct Clip;
  void AddLayerDisplayItem(Layer* aLayer,
                           nsDisplayItem* aItem,
                           const Clip& aClip,
                           LayerState aLayerState,
                           LayerManager* aManager = nsnull);

  





  void AddThebesDisplayItem(ThebesLayer* aLayer,
                            nsDisplayItem* aItem,
                            const Clip& aClip,
                            nsIFrame* aContainerLayerFrame,
                            LayerState aLayerState);

  



  static void SetWidgetLayerManager(LayerManager* aManager)
  {
    LayerManagerSecondary* secondary = 
      static_cast<LayerManagerSecondary*>(aManager->GetUserData(&gLayerManagerSecondary));
    sWidgetManagerSecondary = !!secondary;
  }

  



  static const FramePropertyDescriptor* GetDescriptorForManager(LayerManager* aManager);

  



  static LayerManagerData* GetManagerData(nsIFrame* aFrame, LayerManager* aManager = nsnull);

  



  static void SetManagerData(nsIFrame* aFrame, LayerManagerData* aData);

  



  static void ClearManagerData(nsIFrame* aFrame);

  



  static void ClearManagerData(nsIFrame* aFrame, LayerManagerData* aData);

  






  Layer* GetOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey, 
                        nsDisplayItemGeometry** aOldGeometry = nsnull, Clip** aOldClip = nsnull);

  static Layer* GetDebugOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

  




  LayerManager* GetInactiveLayerManagerFor(nsDisplayItem* aItem);

  




  nscolor FindOpaqueColorCovering(nsDisplayListBuilder* aBuilder,
                                  ThebesLayer* aLayer, const nsRect& aRect);

  



  static void DestroyDisplayItemDataFor(nsIFrame* aFrame);

  LayerManager* GetRetainingLayerManager() { return mRetainingManager; }

  






  static bool NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                 nsDisplayItem* aItem);

  








  static bool HasRetainedLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey, LayerManager* aManager);

  



  void SaveLastPaintOffset(ThebesLayer* aLayer);
  




  nsIntPoint GetLastPaintOffset(ThebesLayer* aLayer);

  






  static bool GetThebesLayerResolutionForFrame(nsIFrame* aFrame,
                                               double* aXRes, double* aYRes,
                                               gfxPoint* aPoint);

  



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
                 PRUint32 aBegin = 0, PRUint32 aEnd = PR_UINT32_MAX);

    void ApplyRectTo(gfxContext* aContext, PRInt32 A2D) const;
    
    
    
    void ApplyRoundedRectsTo(gfxContext* aContext, PRInt32 A2DPRInt32,
                             PRUint32 aBegin, PRUint32 aEnd) const;

    
    void DrawRoundedRectsTo(gfxContext* aContext, PRInt32 A2D,
                            PRUint32 aBegin, PRUint32 aEnd) const;
    
    void AddRoundedRectPathTo(gfxContext* aContext, PRInt32 A2D,
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
  
  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerDataProperty,
                                               RemoveFrameFromLayerManager)

  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(LayerManagerSecondaryDataProperty,
                                               RemoveFrameFromLayerManager)

protected:
  




  class DisplayItemData {
  public:
    DisplayItemData(Layer* aLayer, PRUint32 aKey, LayerState aLayerState, PRUint32 aGeneration)
      : mLayer(aLayer)
      , mDisplayItemKey(aKey)
      , mContainerLayerGeneration(aGeneration)
      , mLayerState(aLayerState)
      , mUsed(false)
    {}
    
    DisplayItemData()
      : mUsed(false)
    {}
    DisplayItemData(DisplayItemData &toCopy)
    {
      
      
      mLayer = toCopy.mLayer;
      mInactiveManager = toCopy.mInactiveManager;
      mGeometry = toCopy.mGeometry;
      mDisplayItemKey = toCopy.mDisplayItemKey;
      mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
      mLayerState = toCopy.mLayerState;
      mUsed = toCopy.mUsed;
    }

    nsRefPtr<Layer> mLayer;
    nsRefPtr<LayerManager> mInactiveManager;
    nsAutoPtr<nsDisplayItemGeometry> mGeometry;
    Clip            mClip;
    PRUint32        mDisplayItemKey;
    PRUint32        mContainerLayerGeneration;
    LayerState      mLayerState;

    




    bool            mUsed;
  };
  
  
  friend class LayerManagerData;

  static void RemoveFrameFromLayerManager(nsIFrame* aFrame, void* aPropertyValue);

  




  class DisplayItemDataEntry : public nsPtrHashKey<nsIFrame> {
  public:
    DisplayItemDataEntry(const nsIFrame *key) : nsPtrHashKey<nsIFrame>(key) { MOZ_COUNT_CTOR(DisplayItemDataEntry); }
    DisplayItemDataEntry(DisplayItemDataEntry &toCopy) :
      nsPtrHashKey<nsIFrame>(toCopy.mKey)
    {
      MOZ_COUNT_CTOR(DisplayItemDataEntry);
      
      
      mData.SwapElements(toCopy.mData);
      mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
    }
    ~DisplayItemDataEntry() { MOZ_COUNT_DTOR(DisplayItemDataEntry); }

    bool HasNonEmptyContainerLayer();

    nsAutoTArray<DisplayItemData, 1> mData;
    PRUint32 mContainerLayerGeneration;

    enum { ALLOW_MEMMOVE = false };
  };

  



  void StoreDataForFrame(nsIFrame* aFrame, DisplayItemData& data);

  
  static void FlashPaint(gfxContext *aContext);

  






  nsTArray<DisplayItemData>* GetDisplayItemDataArrayForFrame(nsIFrame *aFrame);

  



  static DisplayItemData* GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                       PRUint32 aDisplayItemKey, 
                                                       LayerManager* aManager);
  static DisplayItemData* GetDisplayItemDataForManager(nsDisplayItem* aItem, LayerManager* aManager);

  





  static PLDHashOperator RemoveDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                       void* aClosure);

  









  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, const Clip& aClip, PRUint32 aGeneration)
      : mItem(aItem), mClip(aClip), mContainerLayerGeneration(aGeneration)
    {
    }

    ~ClippedDisplayItem();

    nsDisplayItem* mItem;

    




    nsRefPtr<LayerManager> mInactiveLayer;

    Clip mClip;
    PRUint32 mContainerLayerGeneration;
  };

  



public:
  class ThebesLayerItemsEntry : public nsPtrHashKey<ThebesLayer> {
  public:
    ThebesLayerItemsEntry(const ThebesLayer *key) :
        nsPtrHashKey<ThebesLayer>(key), mContainerLayerFrame(nsnull),
        mHasExplicitLastPaintOffset(false), mCommonClipCount(0) {}
    ThebesLayerItemsEntry(const ThebesLayerItemsEntry &toCopy) :
      nsPtrHashKey<ThebesLayer>(toCopy.mKey), mItems(toCopy.mItems)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    nsTArray<ClippedDisplayItem> mItems;
    nsIFrame* mContainerLayerFrame;
    
    
    nsIntPoint mLastPaintOffset;
    PRUint32 mContainerLayerGeneration;
    bool mHasExplicitLastPaintOffset;
    



    PRUint32 mCommonClipCount;

    enum { ALLOW_MEMMOVE = true };
  };

  



  ThebesLayerItemsEntry* GetThebesLayerItemsEntry(ThebesLayer* aLayer)
  {
    return mThebesLayerItems.GetEntry(aLayer);
  }

  static PLDHashOperator ProcessRemovedDisplayItems(DisplayItemDataEntry* aEntry,
                                                    void* aUserArg);
protected:
  void RemoveThebesItemsForLayerSubtree(Layer* aLayer);

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

  


  nsDisplayListBuilder*               mDisplayListBuilder;
  



  nsTHashtable<DisplayItemDataEntry>  mNewDisplayItemData;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;
  


  PRUint32                            mInitialDOMGeneration;
  



  bool                                mDetectedDOMModification;
  



  bool                                mInvalidateAllLayers;

  PRUint32                            mContainerLayerGeneration;
  PRUint32                            mMaxContainerLayerGeneration;

  



  static bool                         sWidgetManagerSecondary;
};

static inline FrameLayerBuilder *GetLayerBuilderForManager(layers::LayerManager* aManager)
{
  return static_cast<FrameLayerBuilder*>(aManager->GetUserData(&gLayerManagerLayerBuilder));
}

}

#endif 
