




































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

namespace mozilla {

namespace layers {
class Layer;
class ThebesLayer;
class LayerManager;
}































class FrameLayerBuilder {
public:
  typedef layers::Layer Layer; 
  typedef layers::ThebesLayer ThebesLayer;
  typedef layers::LayerManager LayerManager;

  FrameLayerBuilder() :
    mRetainingManager(nsnull),
    mInvalidateAllThebesContent(PR_FALSE),
    mInvalidateAllLayers(PR_FALSE)
  {
    mNewDisplayItemData.Init();
    mThebesLayerItems.Init();
  }

  



  void BeginUpdatingRetainedLayers(LayerManager* aManager);

  




  void DidEndTransaction(LayerManager* aManager);

  














  already_AddRefed<Layer> BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                                                 LayerManager* aManager,
                                                 nsIFrame* aContainerFrame,
                                                 nsDisplayItem* aContainerItem,
                                                 const nsDisplayList& aChildren);

  









  Layer* GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                         LayerManager* aManager,
                         nsDisplayItem* aItem);

  





  static void InvalidateThebesLayerContents(nsIFrame* aFrame,
                                            const nsRect& aRect);

  



  static void InvalidateAllThebesLayerContents(LayerManager* aManager);

  



  static void InvalidateAllLayers(LayerManager* aManager);

  



  static PRBool HasDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

  



  static void DrawThebesLayer(ThebesLayer* aLayer,
                              gfxContext* aContext,
                              const nsIntRegion& aRegionToDraw,
                              const nsIntRegion& aRegionToInvalidate,
                              void* aCallbackData);

#ifdef DEBUG
  


  static void DumpLayerTree(LayerManager* aManager);

  



  void DumpRetainedLayerTree();
#endif

  
  


  
  


  void AddLayerDisplayItem(Layer* aLayer, nsDisplayItem* aItem);

  





  void AddThebesDisplayItem(ThebesLayer* aLayer, nsDisplayItem* aItem,
                            const nsRect* aClipRect,
                            nsIFrame* aContainerLayerFrame);

  






  Layer* GetOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey);

protected:
  




  class DisplayItemData {
  public:
    DisplayItemData(Layer* aLayer, PRUint32 aKey)
      : mLayer(aLayer), mDisplayItemKey(aKey) {}

    nsRefPtr<Layer> mLayer;
    PRUint32        mDisplayItemKey;
  };

  static void InternalDestroyDisplayItemData(nsIFrame* aFrame,
                                             void* aPropertyValue,
                                             PRBool aRemoveFromFramesWithLayers);
  static void DestroyDisplayItemData(nsIFrame* aFrame, void* aPropertyValue);

  




  NS_DECLARE_FRAME_PROPERTY_WITH_FRAME_IN_DTOR(DisplayItemDataProperty,
                                               DestroyDisplayItemData)

  




  class DisplayItemDataEntry : public nsPtrHashKey<nsIFrame> {
  public:
    DisplayItemDataEntry(const nsIFrame *key) : nsPtrHashKey<nsIFrame>(key) {}
    DisplayItemDataEntry(const DisplayItemDataEntry &toCopy) :
      nsPtrHashKey<nsIFrame>(toCopy.mKey), mData(toCopy.mData)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    PRBool HasContainerLayer();

    nsTArray<DisplayItemData> mData;

    enum { ALLOW_MEMMOVE = PR_TRUE };
  };

  









  struct ClippedDisplayItem {
    ClippedDisplayItem(nsDisplayItem* aItem, const nsRect* aClipRect)
      : mItem(aItem), mHasClipRect(aClipRect != nsnull)
    {
      if (mHasClipRect) {
        mClipRect = *aClipRect;
      }
    }

    nsDisplayItem* mItem;
    nsRect         mClipRect;
    PRPackedBool   mHasClipRect;
  };

  



  class ThebesLayerItemsEntry : public nsPtrHashKey<ThebesLayer> {
  public:
    ThebesLayerItemsEntry(const ThebesLayer *key) : nsPtrHashKey<ThebesLayer>(key) {}
    ThebesLayerItemsEntry(const ThebesLayerItemsEntry &toCopy) :
      nsPtrHashKey<ThebesLayer>(toCopy.mKey), mItems(toCopy.mItems)
    {
      NS_ERROR("Should never be called, since we ALLOW_MEMMOVE");
    }

    nsTArray<ClippedDisplayItem> mItems;
    nsIFrame* mContainerLayerFrame;

    enum { ALLOW_MEMMOVE = PR_TRUE };
  };

  void RemoveThebesItemsForLayerSubtree(Layer* aLayer);

  static PLDHashOperator UpdateDisplayItemDataForFrame(nsPtrHashKey<nsIFrame>* aEntry,
                                                       void* aUserArg);
  static PLDHashOperator StoreNewDisplayItemData(DisplayItemDataEntry* aEntry,
                                                 void* aUserArg);

  



  LayerManager*                       mRetainingManager;
  



  nsTHashtable<DisplayItemDataEntry>  mNewDisplayItemData;
  



  nsTHashtable<ThebesLayerItemsEntry> mThebesLayerItems;
  



  PRPackedBool                        mInvalidateAllThebesContent;
  



  PRPackedBool                        mInvalidateAllLayers;
};

}

#endif 
