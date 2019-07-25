




































#include "FrameLayerBuilder.h"

#include "nsDisplayList.h"
#include "nsPresContext.h"
#include "nsLayoutUtils.h"
#include "Layers.h"
#include "BasicLayers.h"

#ifdef DEBUG
#include <stdio.h>
#endif

using namespace mozilla::layers;

namespace mozilla {

namespace {




class LayerManagerData {
public:
  LayerManagerData() :
    mInvalidateAllThebesContent(PR_FALSE),
    mInvalidateAllLayers(PR_FALSE)
  {
    mFramesWithLayers.Init();
  }

  


  nsTHashtable<nsPtrHashKey<nsIFrame> > mFramesWithLayers;
  PRPackedBool mInvalidateAllThebesContent;
  PRPackedBool mInvalidateAllLayers;
};

static void DestroyRegion(void* aPropertyValue)
{
  delete static_cast<nsRegion*>(aPropertyValue);
}












NS_DECLARE_FRAME_PROPERTY(ThebesLayerInvalidRegionProperty, DestroyRegion)





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 nsIFrame* aContainerFrame,
                 ContainerLayer* aContainerLayer) :
    mBuilder(aBuilder), mManager(aManager),
    mContainerFrame(aContainerFrame), mContainerLayer(aContainerLayer),
    mNextFreeRecycledThebesLayer(0), mNextFreeRecycledColorLayer(0),
    mInvalidateAllThebesContent(PR_FALSE)
  {
    CollectOldLayers();
  }

  void SetInvalidThebesContent(const nsIntRegion& aRegion)
  {
    mInvalidThebesContent = aRegion;
  }
  void SetInvalidateAllThebesContent()
  {
    mInvalidateAllThebesContent = PR_TRUE;
  }
  





  void ProcessDisplayItems(const nsDisplayList& aList,
                           const nsRect* aClipRect);
  





  void Finish();

protected:
  








  class ThebesLayerData {
  public:
    ThebesLayerData() :
      mActiveScrolledRoot(nsnull), mLayer(nsnull),
      mIsSolidColorInVisibleRegion(PR_FALSE) {}
    










    void Accumulate(const nsIntRect& aVisibleRect,
                    const nsIntRect* aOpaqueRect,
                    nscolor* aSolidColor);
    nsIFrame* GetActiveScrolledRoot() { return mActiveScrolledRoot; }

    




    nsIntRegion  mVisibleRegion;
    





    nsIntRegion  mVisibleAboveRegion;
    



    nsIntRegion  mOpaqueRegion;
    




    nsIFrame*    mActiveScrolledRoot;
    ThebesLayer* mLayer;
    



    nscolor      mSolidColor;
    


    PRPackedBool mIsSolidColorInVisibleRegion;
  };

  





  already_AddRefed<ThebesLayer> CreateOrRecycleThebesLayer(nsIFrame* aActiveScrolledRoot);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer();
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem, Layer* aNewLayer);
  




  void PopThebesLayerData();
  












  already_AddRefed<ThebesLayer> FindThebesLayerFor(const nsIntRect& aVisibleRect,
                                                   nsIFrame* aActiveScrolledRoot,
                                                   const nsIntRect* aOpaqueRect,
                                                   nscolor* aSolidColor);
  ThebesLayerData* GetTopThebesLayerData()
  {
    return mThebesLayerDataStack.IsEmpty() ? nsnull
        : mThebesLayerDataStack[mThebesLayerDataStack.Length() - 1].get();
  }

  nsDisplayListBuilder*            mBuilder;
  LayerManager*                    mManager;
  nsIFrame*                        mContainerFrame;
  ContainerLayer*                  mContainerLayer;
  



  nsIntRegion                      mInvalidThebesContent;
  nsAutoTArray<nsAutoPtr<ThebesLayerData>,1>  mThebesLayerDataStack;
  




  nsAutoTArray<nsRefPtr<Layer>,1>  mNewChildLayers;
  nsTArray<nsRefPtr<ThebesLayer> > mRecycledThebesLayers;
  nsTArray<nsRefPtr<ColorLayer> >  mRecycledColorLayers;
  PRUint32                         mNextFreeRecycledThebesLayer;
  PRUint32                         mNextFreeRecycledColorLayer;
  PRPackedBool                     mInvalidateAllThebesContent;
};









static PRUint8 gThebesDisplayItemLayerUserData;




static PRUint8 gColorLayerUserData;

} 

PRBool
FrameLayerBuilder::DisplayItemDataEntry::HasContainerLayer()
{
  for (PRUint32 i = 0; i < mData.Length(); ++i) {
    if (mData[i].mLayer->GetType() == Layer::TYPE_CONTAINER)
      return PR_TRUE;
  }
  return PR_FALSE;
}

 void
FrameLayerBuilder::InternalDestroyDisplayItemData(nsIFrame* aFrame,
                                                  void* aPropertyValue,
                                                  PRBool aRemoveFromFramesWithLayers)
{
  nsRefPtr<LayerManager> managerRef;
  nsTArray<DisplayItemData>* array =
    reinterpret_cast<nsTArray<DisplayItemData>*>(&aPropertyValue);
  NS_ASSERTION(!array->IsEmpty(), "Empty arrays should not be stored");

  if (aRemoveFromFramesWithLayers) {
    LayerManager* manager = array->ElementAt(0).mLayer->Manager();
    LayerManagerData* data = static_cast<LayerManagerData*>
      (manager->GetUserData());
    NS_ASSERTION(data, "Frame with layer should have been recorded");
    data->mFramesWithLayers.RemoveEntry(aFrame);
    if (data->mFramesWithLayers.Count() == 0) {
      delete data;
      manager->SetUserData(nsnull);
      
      
      
      managerRef = manager;
      NS_RELEASE(manager);
    }
  }

  array->~nsTArray<DisplayItemData>();
}

 void
FrameLayerBuilder::DestroyDisplayItemData(nsIFrame* aFrame,
                                          void* aPropertyValue)
{
  InternalDestroyDisplayItemData(aFrame, aPropertyValue, PR_TRUE);
}

void
FrameLayerBuilder::WillBeginRetainedLayerTransaction(LayerManager* aManager)
{
  mRetainingManager = aManager;
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData());
  if (data) {
    mInvalidateAllThebesContent = data->mInvalidateAllThebesContent;
    mInvalidateAllLayers = data->mInvalidateAllLayers;
  }
}





void
FrameLayerBuilder::RemoveThebesItemsForLayerSubtree(Layer* aLayer)
{
  ThebesLayer* thebes = aLayer->AsThebesLayer();
  if (thebes) {
    mThebesLayerItems.RemoveEntry(thebes);
    return;
  }

  for (Layer* child = aLayer->GetFirstChild(); child;
       child = child->GetNextSibling()) {
    RemoveThebesItemsForLayerSubtree(child);
  }
}

void
FrameLayerBuilder::DidEndTransaction(LayerManager* aManager)
{
  if (aManager != mRetainingManager) {
    Layer* root = aManager->GetRoot();
    if (root) {
      RemoveThebesItemsForLayerSubtree(root);
    }
  }
}

void
FrameLayerBuilder::WillEndTransaction(LayerManager* aManager)
{
  if (aManager != mRetainingManager)
    return;

  
  
  
  
  
  LayerManagerData* data = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData());
  if (data) {
    
    data->mFramesWithLayers.EnumerateEntries(UpdateDisplayItemDataForFrame, this);
  } else {
    data = new LayerManagerData();
    mRetainingManager->SetUserData(data);
    
    
    NS_ADDREF(mRetainingManager);
  }
  
  
  
  mNewDisplayItemData.EnumerateEntries(StoreNewDisplayItemData, data);
  data->mInvalidateAllThebesContent = PR_FALSE;
  data->mInvalidateAllLayers = PR_FALSE;

  NS_ASSERTION(data->mFramesWithLayers.Count() > 0,
               "Some frame must have a layer!");
}

 PLDHashOperator
FrameLayerBuilder::UpdateDisplayItemDataForFrame(nsPtrHashKey<nsIFrame>* aEntry,
                                                 void* aUserArg)
{
  FrameLayerBuilder* builder = static_cast<FrameLayerBuilder*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();
  FrameProperties props = f->Properties();
  DisplayItemDataEntry* newDisplayItems =
    builder->mNewDisplayItemData.GetEntry(f);
  if (!newDisplayItems) {
    
    PRBool found;
    void* prop = props.Remove(DisplayItemDataProperty(), &found);
    NS_ASSERTION(found, "How can the frame property be missing?");
    
    
    
    
    
    
    
    InternalDestroyDisplayItemData(f, prop, PR_FALSE);
    props.Delete(ThebesLayerInvalidRegionProperty());
    f->RemoveStateBits(NS_FRAME_HAS_CONTAINER_LAYER);
    return PL_DHASH_REMOVE;
  }

  if (!newDisplayItems->HasContainerLayer()) {
    props.Delete(ThebesLayerInvalidRegionProperty());
    f->RemoveStateBits(NS_FRAME_HAS_CONTAINER_LAYER);
  } else {
    NS_ASSERTION(f->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER,
                 "This bit should have been set by BuildContainerLayerFor");
  }

  
  
  nsRegion* invalidRegion = static_cast<nsRegion*>
    (props.Get(ThebesLayerInvalidRegionProperty()));
  if (invalidRegion) {
    invalidRegion->SetEmpty();
  }

  
  
  void* propValue = props.Remove(DisplayItemDataProperty());
  NS_ASSERTION(propValue, "mFramesWithLayers out of sync");
  PR_STATIC_ASSERT(sizeof(nsTArray<DisplayItemData>) == sizeof(void*));
  nsTArray<DisplayItemData>* array =
    reinterpret_cast<nsTArray<DisplayItemData>*>(&propValue);
  
  array->SwapElements(newDisplayItems->mData);
  props.Set(DisplayItemDataProperty(), propValue);
  
  builder->mNewDisplayItemData.RawRemoveEntry(newDisplayItems);
  return PL_DHASH_NEXT;
}

 PLDHashOperator
FrameLayerBuilder::StoreNewDisplayItemData(DisplayItemDataEntry* aEntry,
                                           void* aUserArg)
{
  LayerManagerData* data = static_cast<LayerManagerData*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();
  
  NS_ASSERTION(!data->mFramesWithLayers.GetEntry(f),
               "We shouldn't get here if we're already in mFramesWithLayers");
  data->mFramesWithLayers.PutEntry(f);
  NS_ASSERTION(!f->Properties().Get(DisplayItemDataProperty()),
               "mFramesWithLayers out of sync");

  void* propValue;
  nsTArray<DisplayItemData>* array =
    new (&propValue) nsTArray<DisplayItemData>();
  
  array->SwapElements(aEntry->mData);
  
  f->Properties().Set(DisplayItemDataProperty(), propValue);

  return PL_DHASH_REMOVE;
}

Layer*
FrameLayerBuilder::GetOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  
  
  if (!mRetainingManager || mInvalidateAllLayers)
    return nsnull;

  void* propValue = aFrame->Properties().Get(DisplayItemDataProperty());
  if (!propValue)
    return nsnull;

  nsTArray<DisplayItemData>* array =
    (reinterpret_cast<nsTArray<DisplayItemData>*>(&propValue));
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (layer->Manager() == mRetainingManager)
        return layer;
    }
  }
  return nsnull;
}








static void
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsIntRegion& aRegion)
{
  gfxMatrix transform;
  if (aLayer->GetTransform().Is2D(&transform)) {
    NS_ASSERTION(!transform.HasNonIntegerTranslation(),
                 "Matrix not just an integer translation?");
    
    
    
    nsIntRegion rgn = aRegion;
    rgn.MoveBy(-nsIntPoint(PRInt32(transform.x0), PRInt32(transform.y0)));
    aLayer->InvalidateRegion(rgn);
  } else {
    NS_ERROR("Only 2D transformations currently supported");
  }
}

already_AddRefed<ColorLayer>
ContainerState::CreateOrRecycleColorLayer()
{
  nsRefPtr<ColorLayer> layer;
  if (mNextFreeRecycledColorLayer < mRecycledColorLayers.Length()) {
    
    layer = mRecycledColorLayers[mNextFreeRecycledColorLayer];
    ++mNextFreeRecycledColorLayer;
    
    
    layer->SetClipRect(nsnull);
  } else {
    
    layer = mManager->CreateColorLayer();
    if (!layer)
      return nsnull;
    
    layer->SetUserData(&gColorLayerUserData);
  }
  return layer.forget();
}

already_AddRefed<ThebesLayer>
ContainerState::CreateOrRecycleThebesLayer(nsIFrame* aActiveScrolledRoot)
{
  
  nsRefPtr<ThebesLayer> layer;
  if (mNextFreeRecycledThebesLayer < mRecycledThebesLayers.Length()) {
    
    layer = mRecycledThebesLayers[mNextFreeRecycledThebesLayer];
    ++mNextFreeRecycledThebesLayer;
    
    
    layer->SetClipRect(nsnull);

    
    
    
    
    
    
    
    
    if (mInvalidateAllThebesContent) {
      nsIntRect invalidate = layer->GetValidRegion().GetBounds();
      layer->InvalidateRegion(invalidate);
    } else {
      InvalidatePostTransformRegion(layer, mInvalidThebesContent);
    }
    
    
    
    
  } else {
    
    layer = mManager->CreateThebesLayer();
    if (!layer)
      return nsnull;
    
    layer->SetUserData(&gThebesDisplayItemLayerUserData);
  }

  
  
  nsPoint offset = mBuilder->ToReferenceFrame(aActiveScrolledRoot);
  nsIntPoint pixOffset = offset.ToNearestPixels(
      aActiveScrolledRoot->PresContext()->AppUnitsPerDevPixel());
  gfxMatrix matrix;
  matrix.Translate(gfxPoint(pixOffset.x, pixOffset.y));
  layer->SetTransform(gfx3DMatrix::From2D(matrix));

  return layer.forget();
}






static PRUint32
AppUnitsPerDevPixel(nsDisplayItem* aItem)
{
  return aItem->GetUnderlyingFrame()->PresContext()->AppUnitsPerDevPixel();
}






static void
SetVisibleRectForLayer(Layer* aLayer, const nsIntRect& aRect)
{
  gfxMatrix transform;
  if (aLayer->GetTransform().Is2D(&transform)) {
    
    
    transform.Invert();
    gfxRect layerVisible = transform.TransformBounds(
        gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
    layerVisible.RoundOut();
    nsIntRect visibleRect;
    if (NS_FAILED(nsLayoutUtils::GfxRectToIntRect(layerVisible, &visibleRect))) {
      NS_ERROR("Visible rect transformed out of bounds");
    }
    aLayer->SetVisibleRegion(visibleRect);
  } else {
    NS_ERROR("Only 2D transformations currently supported");
  }
}

void
ContainerState::PopThebesLayerData()
{
  NS_ASSERTION(!mThebesLayerDataStack.IsEmpty(), "Can't pop");

  PRInt32 lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  if (lastIndex > 0) {
    
    
    
    ThebesLayerData* nextData = mThebesLayerDataStack[lastIndex - 1];
    nextData->mVisibleAboveRegion.Or(nextData->mVisibleAboveRegion,
                                     data->mVisibleAboveRegion);
    nextData->mVisibleAboveRegion.Or(nextData->mVisibleAboveRegion,
                                     data->mVisibleRegion);
  }

  Layer* layer;
  if (data->mIsSolidColorInVisibleRegion) {
    nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer();
    colorLayer->SetColor(data->mSolidColor);

    NS_ASSERTION(!mNewChildLayers.Contains(colorLayer), "Layer already in list???");
    nsTArray_base::index_type index = mNewChildLayers.IndexOf(data->mLayer);
    NS_ASSERTION(index != nsTArray_base::NoIndex, "Thebes layer not found?");
    mNewChildLayers.InsertElementAt(index + 1, colorLayer);

    
    colorLayer->SetTransform(data->mLayer->GetTransform());
    
    
    
    
    
    
    nsIntRect visibleRect = data->mVisibleRegion.GetBounds();
    colorLayer->SetClipRect(&visibleRect);

    
    
    data->mLayer->IntersectClipRect(nsIntRect());
    data->mLayer->SetVisibleRegion(nsIntRegion());

    layer = colorLayer;
  } else {
    layer = data->mLayer;
  }

  gfxMatrix transform;
  if (layer->GetTransform().Is2D(&transform)) {
    NS_ASSERTION(!transform.HasNonIntegerTranslation(),
                 "Matrix not just an integer translation?");
    
    
    nsIntRegion rgn = data->mVisibleRegion;
    rgn.MoveBy(-nsIntPoint(PRInt32(transform.x0), PRInt32(transform.y0)));
    layer->SetVisibleRegion(rgn);
  } else {
    NS_ERROR("Only 2D transformations currently supported");
  }

  nsIntRegion transparentRegion;
  transparentRegion.Sub(data->mVisibleRegion, data->mOpaqueRegion);
  layer->SetIsOpaqueContent(transparentRegion.IsEmpty());

  mThebesLayerDataStack.RemoveElementAt(lastIndex);
}

void
ContainerState::ThebesLayerData::Accumulate(const nsIntRect& aRect,
                                            const nsIntRect* aOpaqueRect,
                                            nscolor* aSolidColor)
{
  if (aSolidColor) {
    if (mVisibleRegion.IsEmpty()) {
      
      mSolidColor = *aSolidColor;
      mIsSolidColorInVisibleRegion = PR_TRUE;
    } else if (mIsSolidColorInVisibleRegion &&
               mVisibleRegion.IsEqual(nsIntRegion(aRect))) {
      
      mSolidColor = NS_ComposeColors(mSolidColor, *aSolidColor);
    } else {
      mIsSolidColorInVisibleRegion = PR_FALSE;
    }
  } else {
    mIsSolidColorInVisibleRegion = PR_FALSE;
  }

  mVisibleRegion.Or(mVisibleRegion, aRect);
  mVisibleRegion.SimplifyOutward(4);
  if (aOpaqueRect) {
    
    
    
    
    
    nsIntRegion tmp;
    tmp.Or(mOpaqueRegion, *aOpaqueRect);
    if (tmp.GetNumRects() <= 4) {
      mOpaqueRegion = tmp;
    }
  }
}

already_AddRefed<ThebesLayer>
ContainerState::FindThebesLayerFor(const nsIntRect& aVisibleRect,
                                   nsIFrame* aActiveScrolledRoot,
                                   const nsIntRect* aOpaqueRect,
                                   nscolor* aSolidColor)
{
  PRInt32 i;
  PRInt32 lowestUsableLayerWithScrolledRoot = -1;
  PRInt32 topmostLayerWithScrolledRoot = -1;
  for (i = mThebesLayerDataStack.Length() - 1; i >= 0; --i) {
    ThebesLayerData* data = mThebesLayerDataStack[i];
    if (data->mVisibleAboveRegion.Intersects(aVisibleRect)) {
      ++i;
      break;
    }
    if (data->mActiveScrolledRoot == aActiveScrolledRoot) {
      lowestUsableLayerWithScrolledRoot = i;
      if (topmostLayerWithScrolledRoot < 0) {
        topmostLayerWithScrolledRoot = i;
      }
    }
    if (data->mVisibleRegion.Intersects(aVisibleRect))
      break;
  }
  if (topmostLayerWithScrolledRoot < 0) {
    --i;
    for (; i >= 0; --i) {
      ThebesLayerData* data = mThebesLayerDataStack[i];
      if (data->mActiveScrolledRoot == aActiveScrolledRoot) {
        topmostLayerWithScrolledRoot = i;
        break;
      }
    }
  }

  if (topmostLayerWithScrolledRoot >= 0) {
    while (PRUint32(topmostLayerWithScrolledRoot + 1) < mThebesLayerDataStack.Length()) {
      PopThebesLayerData();
    }
  }

  nsRefPtr<ThebesLayer> layer;
  ThebesLayerData* thebesLayerData = nsnull;
  if (lowestUsableLayerWithScrolledRoot < 0) {
    layer = CreateOrRecycleThebesLayer(aActiveScrolledRoot);

    NS_ASSERTION(!mNewChildLayers.Contains(layer), "Layer already in list???");
    mNewChildLayers.AppendElement(layer);

    thebesLayerData = new ThebesLayerData();
    mThebesLayerDataStack.AppendElement(thebesLayerData);
    thebesLayerData->mLayer = layer;
    thebesLayerData->mActiveScrolledRoot = aActiveScrolledRoot;
  } else {
    thebesLayerData = mThebesLayerDataStack[lowestUsableLayerWithScrolledRoot];
    layer = thebesLayerData->mLayer;
  }

  thebesLayerData->Accumulate(aVisibleRect, aOpaqueRect, aSolidColor);
  return layer.forget();
}














void
ContainerState::ProcessDisplayItems(const nsDisplayList& aList,
                                    const nsRect* aClipRect)
{
  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    if (item->GetType() == nsDisplayItem::TYPE_CLIP) {
      nsDisplayClip* clipItem = static_cast<nsDisplayClip*>(item);
      nsRect clip = clipItem->GetClipRect();
      if (aClipRect) {
        clip.IntersectRect(clip, *aClipRect);
      }
      ProcessDisplayItems(*clipItem->GetList(), &clip);
      continue;
    }

    PRInt32 appUnitsPerDevPixel = AppUnitsPerDevPixel(item);
    nsIntRect itemVisibleRect =
      item->GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
    nsDisplayItem::LayerState layerState =
      item->GetLayerState(mBuilder, mManager);

    
    if (layerState == LAYER_ACTIVE) {
      
      
      
      
      if (itemVisibleRect.IsEmpty()) {
        InvalidateForLayerChange(item, nsnull);
        continue;
      }

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager);
      if (!ownLayer) {
        InvalidateForLayerChange(item, ownLayer);
        continue;
      }

      
      NS_ASSERTION(ownLayer->Manager() == mManager, "Wrong manager");
      NS_ASSERTION(ownLayer->GetUserData() != &gThebesDisplayItemLayerUserData,
                   "We shouldn't have a FrameLayerBuilder-managed layer here!");
      
      if (aClipRect) {
        ownLayer->IntersectClipRect(
            aClipRect->ToNearestPixels(appUnitsPerDevPixel));
      }
      ThebesLayerData* data = GetTopThebesLayerData();
      if (data) {
        data->mVisibleAboveRegion.Or(data->mVisibleAboveRegion, itemVisibleRect);
      }
      SetVisibleRectForLayer(ownLayer, itemVisibleRect);
      ContainerLayer* oldContainer = ownLayer->GetParent();
      if (oldContainer && oldContainer != mContainerLayer) {
        oldContainer->RemoveChild(ownLayer);
      }
      NS_ASSERTION(!mNewChildLayers.Contains(ownLayer),
                   "Layer already in list???");

      InvalidateForLayerChange(item, ownLayer);

      mNewChildLayers.AppendElement(ownLayer);
      mBuilder->LayerBuilder()->AddLayerDisplayItem(ownLayer, item);
    } else {
      nsIFrame* f = item->GetUnderlyingFrame();
      nsPoint offsetToActiveScrolledRoot;
      nsIFrame* activeScrolledRoot =
        nsLayoutUtils::GetActiveScrolledRootFor(f, mBuilder->ReferenceFrame(),
                                                &offsetToActiveScrolledRoot);
      NS_ASSERTION(offsetToActiveScrolledRoot == f->GetOffsetToCrossDoc(activeScrolledRoot),
                   "Wrong offset");
      if (item->IsFixedAndCoveringViewport(mBuilder)) {
        
        
        
        
        nsIFrame* viewportFrame =
          nsLayoutUtils::GetClosestFrameOfType(f, nsGkAtoms::viewportFrame);
        NS_ASSERTION(viewportFrame, "no viewport???");
        activeScrolledRoot =
          nsLayoutUtils::GetActiveScrolledRootFor(viewportFrame, mBuilder->ReferenceFrame(),
                                                  &offsetToActiveScrolledRoot);
      }

      nscolor uniformColor;
      PRBool isUniform = item->IsUniform(mBuilder, &uniformColor);
      PRBool isOpaque = item->IsOpaque(mBuilder);
      nsIntRect opaqueRect;
      if (isOpaque) {
        opaqueRect = item->GetBounds(mBuilder).ToNearestPixels(appUnitsPerDevPixel);
      }
      nsRefPtr<ThebesLayer> thebesLayer =
        FindThebesLayerFor(itemVisibleRect, activeScrolledRoot,
                           isOpaque ? &opaqueRect : nsnull,
                           isUniform ? &uniformColor : nsnull);

      InvalidateForLayerChange(item, thebesLayer);

      mBuilder->LayerBuilder()->
        AddThebesDisplayItem(thebesLayer, mBuilder,
                             item, aClipRect, mContainerFrame,
                             layerState);
    }
  }
}

void
ContainerState::InvalidateForLayerChange(nsDisplayItem* aItem, Layer* aNewLayer)
{
  nsIFrame* f = aItem->GetUnderlyingFrame();
  NS_ASSERTION(f, "Display items that render using Thebes must have a frame");
  PRUint32 key = aItem->GetPerFrameKey();
  NS_ASSERTION(key, "Display items that render using Thebes must have a key");
  Layer* oldLayer = mBuilder->LayerBuilder()->GetOldLayerFor(f, key);
  if (!oldLayer) {
    
    return;
  }
  if (aNewLayer != oldLayer) {
    
    
    
    
    
    nsRect bounds = aItem->GetBounds(mBuilder);
    PRInt32 appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
    nsIntRect r = bounds.ToOutsidePixels(appUnitsPerDevPixel);

    ThebesLayer* t = oldLayer->AsThebesLayer();
    if (t) {
      InvalidatePostTransformRegion(t, r);
    }
    if (aNewLayer) {
      ThebesLayer* newLayer = aNewLayer->AsThebesLayer();
      if (newLayer) {
        InvalidatePostTransformRegion(newLayer, r);
      }
    }

    mContainerFrame->InvalidateWithFlags(
        bounds - mBuilder->ToReferenceFrame(mContainerFrame),
        nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
        nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);
  }
}

void
FrameLayerBuilder::AddThebesDisplayItem(ThebesLayer* aLayer,
                                        nsDisplayListBuilder* aBuilder,
                                        nsDisplayItem* aItem,
                                        const nsRect* aClipRect,
                                        nsIFrame* aContainerLayerFrame,
                                        LayerState aLayerState)
{
  nsRefPtr<BasicLayerManager> tempManager;
  if (aLayerState == LAYER_INACTIVE) {
    
    
    
    
    
    tempManager = new BasicLayerManager();
    tempManager->BeginTransaction();
    nsRefPtr<Layer> layer = aItem->BuildLayer(aBuilder, tempManager);
    if (!layer) {
      tempManager->EndTransaction(nsnull, nsnull);
      return;
    }
    PRInt32 appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
    nsIntRect itemVisibleRect =
      aItem->GetVisibleRect().ToNearestPixels(appUnitsPerDevPixel);
    SetVisibleRectForLayer(layer, itemVisibleRect);

    tempManager->SetRoot(layer);
    
    tempManager->EndTransaction(nsnull, nsnull);
  }

  AddLayerDisplayItem(aLayer, aItem);

  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerLayerFrame;
    NS_ASSERTION(aItem->GetUnderlyingFrame(), "Must have frame");
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem, aClipRect));
    cdi->mTempLayerManager = tempManager.forget();
  }
}

void
FrameLayerBuilder::AddLayerDisplayItem(Layer* aLayer,
                                       nsDisplayItem* aItem)
{
  if (aLayer->Manager() != mRetainingManager)
    return;

  nsIFrame* f = aItem->GetUnderlyingFrame();
  DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(f);
  if (entry) {
    entry->mData.AppendElement(DisplayItemData(aLayer, aItem->GetPerFrameKey()));
  }
}

void
ContainerState::CollectOldLayers()
{
  for (Layer* layer = mContainerLayer->GetFirstChild(); layer;
       layer = layer->GetNextSibling()) {
    void* data = layer->GetUserData();
    if (data == &gColorLayerUserData) {
      mRecycledColorLayers.AppendElement(static_cast<ColorLayer*>(layer));
    } else if (data == &gThebesDisplayItemLayerUserData) {
      NS_ASSERTION(layer->AsThebesLayer(), "Wrong layer type");
      mRecycledThebesLayers.AppendElement(static_cast<ThebesLayer*>(layer));
    }
  }
}

void
ContainerState::Finish()
{
  while (!mThebesLayerDataStack.IsEmpty()) {
    PopThebesLayerData();
  }

  for (PRUint32 i = 0; i <= mNewChildLayers.Length(); ++i) {
    
    
    Layer* layer;
    if (i < mNewChildLayers.Length()) {
      layer = mNewChildLayers[i];
      if (!layer->GetParent()) {
        
        
        Layer* prevChild = i == 0 ? nsnull : mNewChildLayers[i - 1];
        mContainerLayer->InsertAfter(layer, prevChild);
        continue;
      }
      NS_ASSERTION(layer->GetParent() == mContainerLayer,
                   "Layer shouldn't be the child of some other container");
    } else {
      layer = nsnull;
    }

    
    
    
    
    
    Layer* nextOldChild = i == 0 ? mContainerLayer->GetFirstChild() :
      mNewChildLayers[i - 1]->GetNextSibling();
    while (nextOldChild != layer) {
      Layer* tmp = nextOldChild;
      nextOldChild = nextOldChild->GetNextSibling();
      mContainerLayer->RemoveChild(tmp);
    }
    
    
  }
}

already_AddRefed<Layer>
FrameLayerBuilder::BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                                          LayerManager* aManager,
                                          nsIFrame* aContainerFrame,
                                          nsDisplayItem* aContainerItem,
                                          const nsDisplayList& aChildren)
{
  FrameProperties props = aContainerFrame->Properties();
  PRUint32 containerDisplayItemKey =
    aContainerItem ? aContainerItem->GetPerFrameKey() : 0;
  NS_ASSERTION(aContainerFrame, "Container display items here should have a frame");
  NS_ASSERTION(!aContainerItem ||
               aContainerItem->GetUnderlyingFrame() == aContainerFrame,
               "Container display item must match given frame");

  nsRefPtr<ContainerLayer> containerLayer;
  if (aManager == mRetainingManager) {
    Layer* oldLayer = GetOldLayerFor(aContainerFrame, containerDisplayItemKey);
    if (oldLayer) {
      NS_ASSERTION(oldLayer->Manager() == aManager, "Wrong manager");
      if (oldLayer->GetUserData() == &gThebesDisplayItemLayerUserData) {
        
        
        
      } else {
        NS_ASSERTION(oldLayer->GetType() == Layer::TYPE_CONTAINER,
                     "Wrong layer type");
        containerLayer = static_cast<ContainerLayer*>(oldLayer);
        
        containerLayer->SetClipRect(nsnull);
      }
    }
  }
  if (!containerLayer) {
    
    containerLayer = aManager->CreateContainerLayer();
    if (!containerLayer)
      return nsnull;
  }

  ContainerState state(aBuilder, aManager, aContainerFrame, containerLayer);

  if (aManager == mRetainingManager) {
    DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(aContainerFrame);
    if (entry) {
      entry->mData.AppendElement(
          DisplayItemData(containerLayer, containerDisplayItemKey));
    }

    if (mInvalidateAllThebesContent) {
      state.SetInvalidateAllThebesContent();
    }

    nsRegion* invalidThebesContent(static_cast<nsRegion*>
      (props.Get(ThebesLayerInvalidRegionProperty())));
    if (invalidThebesContent) {
      nsPoint offset = aBuilder->ToReferenceFrame(aContainerFrame);
      invalidThebesContent->MoveBy(offset);
      state.SetInvalidThebesContent(invalidThebesContent->
        ToOutsidePixels(aContainerFrame->PresContext()->AppUnitsPerDevPixel()));
      invalidThebesContent->MoveBy(-offset);
    } else {
      
      props.Set(ThebesLayerInvalidRegionProperty(), new nsRegion());
    }
    aContainerFrame->AddStateBits(NS_FRAME_HAS_CONTAINER_LAYER);
  }

  state.ProcessDisplayItems(aChildren, nsnull);
  state.Finish();

  containerLayer->SetIsOpaqueContent(aChildren.IsOpaque());
  nsRefPtr<Layer> layer = containerLayer.forget();
  return layer.forget();
}

Layer*
FrameLayerBuilder::GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   nsDisplayItem* aItem)
{
  if (aManager != mRetainingManager)
    return nsnull;

  nsIFrame* f = aItem->GetUnderlyingFrame();
  NS_ASSERTION(f, "Can only call GetLeafLayerFor on items that have a frame");
  Layer* layer = GetOldLayerFor(f, aItem->GetPerFrameKey());
  if (!layer)
    return nsnull;
  if (layer->GetUserData() == &gThebesDisplayItemLayerUserData) {
    
    
    
    return nsnull;
  }
  
  layer->SetClipRect(nsnull);
  return layer;
}

 void
FrameLayerBuilder::InvalidateThebesLayerContents(nsIFrame* aFrame,
                                                 const nsRect& aRect)
{
  nsRegion* invalidThebesContent = static_cast<nsRegion*>
    (aFrame->Properties().Get(ThebesLayerInvalidRegionProperty()));
  if (!invalidThebesContent)
    return;
  invalidThebesContent->Or(*invalidThebesContent, aRect);
  invalidThebesContent->SimplifyOutward(20);
}

 void
FrameLayerBuilder::InvalidateAllThebesLayerContents(LayerManager* aManager)
{
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData());
  if (data) {
    data->mInvalidateAllThebesContent = PR_TRUE;
  }
}

 void
FrameLayerBuilder::InvalidateAllLayers(LayerManager* aManager)
{
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData());
  if (data) {
    data->mInvalidateAllLayers = PR_TRUE;
  }
}


PRBool
FrameLayerBuilder::HasDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  void* propValue = aFrame->Properties().Get(DisplayItemDataProperty());
  if (!propValue)
    return PR_FALSE;

  nsTArray<DisplayItemData>* array =
    (reinterpret_cast<nsTArray<DisplayItemData>*>(&propValue));
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      void* layerUserData = array->ElementAt(i).mLayer->GetUserData();
      if (layerUserData != &gColorLayerUserData &&
          layerUserData != &gThebesDisplayItemLayerUserData)
        return PR_TRUE;
    }
  }
  return PR_FALSE;
}

 void
FrameLayerBuilder::DrawThebesLayer(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw,
                                   const nsIntRegion& aRegionToInvalidate,
                                   void* aCallbackData)
{
  nsDisplayListBuilder* builder = static_cast<nsDisplayListBuilder*>
    (aCallbackData);
  nsTArray<ClippedDisplayItem> items;
  nsIFrame* containerLayerFrame;
  {
    ThebesLayerItemsEntry* entry =
      builder->LayerBuilder()->mThebesLayerItems.GetEntry(aLayer);
    NS_ASSERTION(entry, "We shouldn't be drawing into a layer with no items!");
    items.SwapElements(entry->mItems);
    containerLayerFrame = entry->mContainerLayerFrame;
    
    
    
  }

  gfxMatrix transform;
  if (!aLayer->GetTransform().Is2D(&transform)) {
    NS_ERROR("non-2D transform in our Thebes layer!");
    return;
  }
  NS_ASSERTION(!transform.HasNonIntegerTranslation(),
               "Matrix not just an integer translation?");
  
  
  gfxContextMatrixAutoSaveRestore saveMatrix(aContext); 
  aContext->Translate(-gfxPoint(transform.x0, transform.y0));
  nsIntPoint offset(PRInt32(transform.x0), PRInt32(transform.y0));

  nsPresContext* presContext = containerLayerFrame->PresContext();
  nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsRect r = (aRegionToInvalidate.GetBounds() + offset).
    ToAppUnits(appUnitsPerDevPixel);
  containerLayerFrame->InvalidateWithFlags(r,
      nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
      nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);

  
  
  
  
  
  
  
  nsRefPtr<nsIRenderingContext> rc;
  nsPresContext* lastPresContext = nsnull;
  nsRect currentClip;
  PRBool setClipRect = PR_FALSE;

  PRUint32 i;
  
  
  
  
  
  nsRegion visible = aRegionToDraw.ToAppUnits(appUnitsPerDevPixel);
  visible.MoveBy(NSIntPixelsToAppUnits(offset.x, appUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(offset.y, appUnitsPerDevPixel));

  for (i = items.Length(); i > 0; --i) {
    ClippedDisplayItem* cdi = &items[i - 1];

    presContext = cdi->mItem->GetUnderlyingFrame()->PresContext();
    if (presContext->AppUnitsPerDevPixel() != appUnitsPerDevPixel) {
      
      nsRegion tmp(cdi->mItem->GetBounds(builder));
      cdi->mItem->RecomputeVisibility(builder, &tmp);
      continue;
    }

    if (!cdi->mHasClipRect || cdi->mClipRect.Contains(visible.GetBounds())) {
      cdi->mItem->RecomputeVisibility(builder, &visible);
      continue;
    }

    
    
    nsRegion clipped;
    clipped.And(visible, cdi->mClipRect);
    nsRegion finalClipped = clipped;
    cdi->mItem->RecomputeVisibility(builder, &finalClipped);
    nsRegion removed;
    removed.Sub(clipped, finalClipped);
    nsRegion newVisible;
    newVisible.Sub(visible, removed);
    
    if (newVisible.GetNumRects() <= 15) {
      visible = newVisible;
    }
  }

  for (i = 0; i < items.Length(); ++i) {
    ClippedDisplayItem* cdi = &items[i];

    if (cdi->mItem->GetVisibleRect().IsEmpty())
      continue;

    presContext = cdi->mItem->GetUnderlyingFrame()->PresContext();
    
    
    if (setClipRect != cdi->mHasClipRect ||
        (cdi->mHasClipRect && cdi->mClipRect != currentClip)) {
      if (setClipRect) {
        aContext->Restore();
      }
      setClipRect = cdi->mHasClipRect;
      if (setClipRect) {
        currentClip = cdi->mClipRect;
        aContext->Save();
        aContext->NewPath();
        gfxRect clip(currentClip.x, currentClip.y, currentClip.width, currentClip.height);
        clip.ScaleInverse(presContext->AppUnitsPerDevPixel());
        aContext->Rectangle(clip, PR_TRUE);
        aContext->Clip();
      }
    }

    if (cdi->mTempLayerManager) {
      
      
      cdi->mTempLayerManager->BeginTransactionWithTarget(aContext);
      cdi->mTempLayerManager->EndTransaction(DrawThebesLayer, builder);
    } else {
      if (presContext != lastPresContext) {
        
        
        nsresult rv =
          presContext->DeviceContext()->CreateRenderingContextInstance(*getter_AddRefs(rc));
        if (NS_FAILED(rv))
          break;
        rc->Init(presContext->DeviceContext(), aContext);
        lastPresContext = presContext;
      }
      cdi->mItem->Paint(builder, rc);
    }
  }

  if (setClipRect) {
    aContext->Restore();
  }
}

#ifdef DEBUG
void
FrameLayerBuilder::DumpRetainedLayerTree()
{
  if (mRetainingManager) {
    mRetainingManager->Dump(stderr);
  }
}
#endif

} 
