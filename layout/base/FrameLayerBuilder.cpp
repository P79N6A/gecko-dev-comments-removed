




































#include "FrameLayerBuilder.h"

#include "nsDisplayList.h"
#include "nsPresContext.h"
#include "nsLayoutUtils.h"
#include "Layers.h"
#include "BasicLayers.h"
#include "nsSubDocumentFrame.h"
#include "nsCSSRendering.h"
#include "nsCSSFrameConstructor.h"
#include "gfxUtils.h"
#include "nsImageFrame.h"
#include "nsRenderingContext.h"

#ifdef DEBUG
#include <stdio.h>
#endif

using namespace mozilla::layers;

namespace mozilla {

namespace {




class LayerManagerData : public LayerUserData {
public:
  LayerManagerData(LayerManager *aManager) :
    mInvalidateAllLayers(PR_FALSE),
    mLayerManager(aManager)
  {
    MOZ_COUNT_CTOR(LayerManagerData);
    mFramesWithLayers.Init();
  }
  ~LayerManagerData() {
    
    
    mFramesWithLayers.EnumerateEntries(
        FrameLayerBuilder::RemoveDisplayItemDataForFrame, nsnull);
    MOZ_COUNT_DTOR(LayerManagerData);
  }

  


  nsTHashtable<nsPtrHashKey<nsIFrame> > mFramesWithLayers;
  PRPackedBool mInvalidateAllLayers;
  
  nsRefPtr<LayerManager> mLayerManager;
};

static void DestroyRegion(void* aPropertyValue)
{
  delete static_cast<nsRegion*>(aPropertyValue);
}















NS_DECLARE_FRAME_PROPERTY(ThebesLayerInvalidRegionProperty, DestroyRegion)

static void DestroyPoint(void* aPropertyValue)
{
  delete static_cast<nsPoint*>(aPropertyValue);
}










NS_DECLARE_FRAME_PROPERTY(ThebesLayerLastPaintOffsetProperty, DestroyPoint)





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 nsIFrame* aContainerFrame,
                 ContainerLayer* aContainerLayer) :
    mBuilder(aBuilder), mManager(aManager),
    mContainerFrame(aContainerFrame), mContainerLayer(aContainerLayer),
    mNextFreeRecycledThebesLayer(0), mNextFreeRecycledColorLayer(0),
    mNextFreeRecycledImageLayer(0), mInvalidateAllThebesContent(PR_FALSE)
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
                           FrameLayerBuilder::Clip& aClip);
  







  void Finish(PRUint32 *aTextContentFlags);

  nsRect GetChildrenBounds() { return mBounds; }

protected:
  








  class ThebesLayerData {
  public:
    ThebesLayerData() :
      mActiveScrolledRoot(nsnull), mLayer(nsnull),
      mIsSolidColorInVisibleRegion(PR_FALSE),
      mNeedComponentAlpha(PR_FALSE),
      mForceTransparentSurface(PR_FALSE),
      mImage(nsnull) {}
    












    void Accumulate(nsDisplayListBuilder* aBuilder,
                    nsDisplayItem* aItem,
                    const nsIntRect& aVisibleRect,
                    const nsIntRect& aDrawRect,
                    const FrameLayerBuilder::Clip& aClip);
    nsIFrame* GetActiveScrolledRoot() { return mActiveScrolledRoot; }

    




    nsRefPtr<ImageContainer> CanOptimizeImageLayer(LayerManager* aManager);

    




    nsIntRegion  mVisibleRegion;
    





    nsIntRegion  mVisibleAboveRegion;
    




    nsIntRegion  mDrawRegion;
    







    nsIntRegion  mDrawAboveRegion;
    



    nsIntRegion  mOpaqueRegion;
    




    nsIFrame*    mActiveScrolledRoot;
    ThebesLayer* mLayer;
    



    nscolor      mSolidColor;
    


    PRPackedBool mIsSolidColorInVisibleRegion;
    



    PRPackedBool mNeedComponentAlpha;
    





    PRPackedBool mForceTransparentSurface;

    



    nsDisplayImage* mImage;
    


    FrameLayerBuilder::Clip mImageClip;
  };

  





  already_AddRefed<ThebesLayer> CreateOrRecycleThebesLayer(nsIFrame* aActiveScrolledRoot);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer();
  



  already_AddRefed<ImageLayer> CreateOrRecycleImageLayer();
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem, Layer* aNewLayer);
  





  nscolor FindOpaqueBackgroundColorFor(PRInt32 aThebesLayerIndex);
  




  void PopThebesLayerData();
  
















  already_AddRefed<ThebesLayer> FindThebesLayerFor(nsDisplayItem* aItem,
                                                   const nsIntRect& aVisibleRect,
                                                   const nsIntRect& aDrawRect,
                                                   const FrameLayerBuilder::Clip& aClip,
                                                   nsIFrame* aActiveScrolledRoot);
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
  nsRect                           mBounds;
  nsAutoTArray<nsAutoPtr<ThebesLayerData>,1>  mThebesLayerDataStack;
  




  typedef nsAutoTArray<nsRefPtr<Layer>,1> AutoLayersArray;
  AutoLayersArray                  mNewChildLayers;
  nsTArray<nsRefPtr<ThebesLayer> > mRecycledThebesLayers;
  nsTArray<nsRefPtr<ColorLayer> >  mRecycledColorLayers;
  nsTArray<nsRefPtr<ImageLayer> >  mRecycledImageLayers;
  PRUint32                         mNextFreeRecycledThebesLayer;
  PRUint32                         mNextFreeRecycledColorLayer;
  PRUint32                         mNextFreeRecycledImageLayer;
  PRPackedBool                     mInvalidateAllThebesContent;
};

class ThebesDisplayItemLayerUserData : public LayerUserData
{
public:
  ThebesDisplayItemLayerUserData() :
    mForcedBackgroundColor(NS_RGBA(0,0,0,0)) {}

  



  nscolor mForcedBackgroundColor;
};










PRUint8 gThebesDisplayItemLayerUserData;





PRUint8 gColorLayerUserData;





PRUint8 gImageLayerUserData;





PRUint8 gLayerManagerUserData;

} 

void
FrameLayerBuilder::Init(nsDisplayListBuilder* aBuilder)
{
  mRootPresContext = aBuilder->ReferenceFrame()->PresContext()->GetRootPresContext();
  if (mRootPresContext) {
    mInitialDOMGeneration = mRootPresContext->GetDOMGeneration();
  }
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
      (manager->GetUserData(&gLayerManagerUserData));
    NS_ASSERTION(data, "Frame with layer should have been recorded");
    data->mFramesWithLayers.RemoveEntry(aFrame);
    if (data->mFramesWithLayers.Count() == 0) {
      
      
      
      managerRef = manager;
      manager->RemoveUserData(&gLayerManagerUserData);
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
FrameLayerBuilder::DidBeginRetainedLayerTransaction(LayerManager* aManager)
{
  mRetainingManager = aManager;
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));
  if (data) {
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
    (mRetainingManager->GetUserData(&gLayerManagerUserData));
  if (data) {
    
    data->mFramesWithLayers.EnumerateEntries(UpdateDisplayItemDataForFrame, this);
  } else {
    data = new LayerManagerData(mRetainingManager);
    mRetainingManager->SetUserData(&gLayerManagerUserData, data);
  }
  
  
  
  mNewDisplayItemData.EnumerateEntries(StoreNewDisplayItemData, data);
  data->mInvalidateAllLayers = PR_FALSE;

  NS_ASSERTION(data->mFramesWithLayers.Count() > 0,
               "Some frame must have a layer!");
}

static void
SetHasContainerLayer(nsIFrame* aFrame, nsPoint aOffsetToRoot)
{
  aFrame->AddStateBits(NS_FRAME_HAS_CONTAINER_LAYER);
  for (nsIFrame* f = aFrame;
       f && !(f->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT);
       f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    f->AddStateBits(NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT);
  }

  FrameProperties props = aFrame->Properties();
  nsPoint* lastPaintOffset = static_cast<nsPoint*>
    (props.Get(ThebesLayerLastPaintOffsetProperty()));
  if (lastPaintOffset) {
    *lastPaintOffset = aOffsetToRoot;
  } else {
    props.Set(ThebesLayerLastPaintOffsetProperty(), new nsPoint(aOffsetToRoot));
  }
}

static void
SetNoContainerLayer(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  props.Delete(ThebesLayerInvalidRegionProperty());
  props.Delete(ThebesLayerLastPaintOffsetProperty());
  aFrame->RemoveStateBits(NS_FRAME_HAS_CONTAINER_LAYER);
}

 PLDHashOperator
FrameLayerBuilder::UpdateDisplayItemDataForFrame(nsPtrHashKey<nsIFrame>* aEntry,
                                                 void* aUserArg)
{
  FrameLayerBuilder* builder = static_cast<FrameLayerBuilder*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();
  FrameProperties props = f->Properties();
  DisplayItemDataEntry* newDisplayItems =
    builder ? builder->mNewDisplayItemData.GetEntry(f) : nsnull;
  if (!newDisplayItems) {
    
    PRBool found;
    void* prop = props.Remove(DisplayItemDataProperty(), &found);
    NS_ASSERTION(found, "How can the frame property be missing?");
    
    
    
    
    
    
    
    InternalDestroyDisplayItemData(f, prop, PR_FALSE);
    SetNoContainerLayer(f);
    return PL_DHASH_REMOVE;
  }

  if (newDisplayItems->HasContainerLayer()) {
    
    
    
    
    nsRegion* invalidRegion = static_cast<nsRegion*>
      (props.Get(ThebesLayerInvalidRegionProperty()));
    if (invalidRegion) {
      invalidRegion->SetEmpty();
    } else {
      props.Set(ThebesLayerInvalidRegionProperty(), new nsRegion());
    }
  } else {
    SetNoContainerLayer(f);
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
  FrameProperties props = f->Properties();
  
  NS_ASSERTION(!data->mFramesWithLayers.GetEntry(f),
               "We shouldn't get here if we're already in mFramesWithLayers");
  data->mFramesWithLayers.PutEntry(f);
  NS_ASSERTION(!props.Get(DisplayItemDataProperty()),
               "mFramesWithLayers out of sync");

  void* propValue;
  nsTArray<DisplayItemData>* array =
    new (&propValue) nsTArray<DisplayItemData>();
  
  array->SwapElements(aEntry->mData);
  
  props.Set(DisplayItemDataProperty(), propValue);

  if (f->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER) {
    props.Set(ThebesLayerInvalidRegionProperty(), new nsRegion());
  }
  return PL_DHASH_REMOVE;
}

PRBool
FrameLayerBuilder::HasRetainedLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  void* propValue = aFrame->Properties().Get(DisplayItemDataProperty());
  if (!propValue)
    return PR_FALSE;

  nsTArray<DisplayItemData>* array =
    (reinterpret_cast<nsTArray<DisplayItemData>*>(&propValue));
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (layer->Manager()->GetUserData(&gLayerManagerUserData)) {
        
        return PR_TRUE;
      }
    }
  }
  return PR_FALSE;
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
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsIntRegion& aRegion,
                              const gfx3DMatrix& aTransform)
{
  gfxMatrix transform;
  if (aTransform.Is2D(&transform)) {
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
    
    layer->SetUserData(&gColorLayerUserData, nsnull);
  }
  return layer.forget();
}

already_AddRefed<ImageLayer>
ContainerState::CreateOrRecycleImageLayer()
{
  nsRefPtr<ImageLayer> layer;
  if (mNextFreeRecycledImageLayer < mRecycledImageLayers.Length()) {
    
    layer = mRecycledImageLayers[mNextFreeRecycledImageLayer];
    ++mNextFreeRecycledImageLayer;
    
    
    layer->SetClipRect(nsnull);
  } else {
    
    layer = mManager->CreateImageLayer();
    if (!layer)
      return nsnull;
    
    layer->SetUserData(&gImageLayerUserData, nsnull);
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
      InvalidatePostTransformRegion(layer, mInvalidThebesContent,
                                    layer->GetTransform());
    }
    
    
    
  } else {
    
    layer = mManager->CreateThebesLayer();
    if (!layer)
      return nsnull;
    
    layer->SetUserData(&gThebesDisplayItemLayerUserData,
        new ThebesDisplayItemLayerUserData());
  }

  mBuilder->LayerBuilder()->SaveLastPaintTransform(layer, layer->GetTransform());

  
  
  nsPoint offset = mBuilder->ToReferenceFrame(aActiveScrolledRoot);
  nsIntPoint pixOffset = offset.ToNearestPixels(
      aActiveScrolledRoot->PresContext()->AppUnitsPerDevPixel());
  gfxMatrix matrix;
  matrix.Translate(gfxPoint(pixOffset.x, pixOffset.y));
  layer->SetTransform(gfx3DMatrix::From2D(matrix));

  return layer.forget();
}






static PRInt32
AppUnitsPerDevPixel(nsDisplayItem* aItem)
{
  
  
  
  
  if (aItem->GetType() == nsDisplayItem::TYPE_ZOOM) {
    return static_cast<nsDisplayZoom*>(aItem)->GetParentAppUnitsPerDevPixel();
  }
  return aItem->GetUnderlyingFrame()->PresContext()->AppUnitsPerDevPixel();
}









static void
RestrictVisibleRegionForLayer(Layer* aLayer, const nsIntRect& aItemVisible)
{
  gfxMatrix transform;
  if (!aLayer->GetTransform().Is2D(&transform))
    return;

  
  
  gfxMatrix inverse = transform;
  inverse.Invert();
  gfxRect itemVisible(aItemVisible.x, aItemVisible.y, aItemVisible.width, aItemVisible.height);
  gfxRect layerVisible = inverse.TransformBounds(itemVisible);
  layerVisible.RoundOut();

  nsIntRect visibleRect;
  if (!gfxUtils::GfxRectToIntRect(layerVisible, &visibleRect))
    return;

  nsIntRegion rgn = aLayer->GetVisibleRegion();
  if (!visibleRect.Contains(rgn.GetBounds())) {
    rgn.And(rgn, visibleRect);
    aLayer->SetVisibleRegion(rgn);
  }
}

nscolor
ContainerState::FindOpaqueBackgroundColorFor(PRInt32 aThebesLayerIndex)
{
  ThebesLayerData* target = mThebesLayerDataStack[aThebesLayerIndex];
  for (PRInt32 i = aThebesLayerIndex - 1; i >= 0; --i) {
    ThebesLayerData* candidate = mThebesLayerDataStack[i];
    nsIntRegion visibleAboveIntersection;
    visibleAboveIntersection.And(candidate->mVisibleAboveRegion, target->mVisibleRegion);
    if (!visibleAboveIntersection.IsEmpty()) {
      
      
      break;
    }

    nsIntRegion intersection;
    intersection.And(candidate->mVisibleRegion, target->mVisibleRegion);
    if (intersection.IsEmpty()) {
      
      continue;
    }
 
    
    
    nsPresContext* presContext = mContainerFrame->PresContext();
    nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    nsRect rect =
      target->mVisibleRegion.GetBounds().ToAppUnits(appUnitsPerDevPixel);
    return mBuilder->LayerBuilder()->
      FindOpaqueColorCovering(mBuilder, candidate->mLayer, rect);
  }
  return NS_RGBA(0,0,0,0);
}

nsRefPtr<ImageContainer>
ContainerState::ThebesLayerData::CanOptimizeImageLayer(LayerManager* aManager)
{
  if (!mImage || !mImageClip.mRoundedClipRects.IsEmpty()) {
    return nsnull;
  }

  return mImage->GetContainer(aManager);
}

void
ContainerState::PopThebesLayerData()
{
  NS_ASSERTION(!mThebesLayerDataStack.IsEmpty(), "Can't pop");

  PRInt32 lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer(mManager); 

  if (data->mIsSolidColorInVisibleRegion || imageContainer) {
    NS_ASSERTION(!(data->mIsSolidColorInVisibleRegion && imageContainer),
                 "Can't be a solid color as well as an image!");
    if (imageContainer) {
      nsRefPtr<ImageLayer> imageLayer = CreateOrRecycleImageLayer();
      imageLayer->SetContainer(imageContainer);
      data->mImage->ConfigureLayer(imageLayer);
      NS_ASSERTION(data->mImageClip.mRoundedClipRects.IsEmpty(),
                   "How did we get rounded clip rects here?");
      if (data->mImageClip.mHaveClipRect) {
        nsPresContext* presContext = mContainerFrame->PresContext();
        nscoord appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
        nsIntRect clip = data->mImageClip.mClipRect.ToNearestPixels(appUnitsPerDevPixel);
        imageLayer->IntersectClipRect(
          data->mImageClip.mClipRect.ToNearestPixels(appUnitsPerDevPixel));
      }
      layer = imageLayer;
    } else {
      nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer();
      colorLayer->SetIsFixedPosition(data->mLayer->GetIsFixedPosition());
      colorLayer->SetColor(data->mSolidColor);

      
      colorLayer->SetTransform(data->mLayer->GetTransform());
      
      
      
      
      
      
      
      nsIntRect visibleRect = data->mVisibleRegion.GetBounds();
      colorLayer->SetClipRect(&visibleRect);

      layer = colorLayer;
    }

    NS_ASSERTION(!mNewChildLayers.Contains(layer), "Layer already in list???");
    AutoLayersArray::index_type index = mNewChildLayers.IndexOf(data->mLayer);
    NS_ASSERTION(index != AutoLayersArray::NoIndex, "Thebes layer not found?");
    mNewChildLayers.InsertElementAt(index + 1, layer);

    
    
    data->mLayer->IntersectClipRect(nsIntRect());
    data->mLayer->SetVisibleRegion(nsIntRegion());
  } else {
    layer = data->mLayer;
  }

  gfxMatrix transform;
  if (!layer->GetTransform().Is2D(&transform)) {
    NS_ERROR("Only 2D transformations currently supported");
  }
  
  
  if (!imageContainer) {
    NS_ASSERTION(!transform.HasNonIntegerTranslation(),
                 "Matrix not just an integer translation?");
    
    
    nsIntRegion rgn = data->mVisibleRegion;
    rgn.MoveBy(-nsIntPoint(PRInt32(transform.x0), PRInt32(transform.y0)));
    layer->SetVisibleRegion(rgn);
  }

  nsIntRegion transparentRegion;
  transparentRegion.Sub(data->mVisibleRegion, data->mOpaqueRegion);
  PRBool isOpaque = transparentRegion.IsEmpty();
  
  
  
  if (layer == data->mLayer) {
    nscolor backgroundColor = NS_RGBA(0,0,0,0);
    if (!isOpaque) {
      backgroundColor = FindOpaqueBackgroundColorFor(lastIndex);
      if (NS_GET_A(backgroundColor) == 255) {
        isOpaque = PR_TRUE;
      }
    }

    
    ThebesDisplayItemLayerUserData* userData =
      static_cast<ThebesDisplayItemLayerUserData*>
        (data->mLayer->GetUserData(&gThebesDisplayItemLayerUserData));
    NS_ASSERTION(userData, "where did our user data go?");
    if (userData->mForcedBackgroundColor != backgroundColor) {
      
      
      data->mLayer->InvalidateRegion(data->mLayer->GetValidRegion());
    }
    userData->mForcedBackgroundColor = backgroundColor;
  }
  PRUint32 flags;
  if (isOpaque && !data->mForceTransparentSurface) {
    flags = Layer::CONTENT_OPAQUE;
  } else if (data->mNeedComponentAlpha) {
    flags = Layer::CONTENT_COMPONENT_ALPHA;
  } else {
    flags = 0;
  }
  layer->SetContentFlags(flags);

  if (lastIndex > 0) {
    
    
    
    ThebesLayerData* nextData = mThebesLayerDataStack[lastIndex - 1];
    nextData->mVisibleAboveRegion.Or(nextData->mVisibleAboveRegion,
                                     data->mVisibleAboveRegion);
    nextData->mVisibleAboveRegion.Or(nextData->mVisibleAboveRegion,
                                     data->mVisibleRegion);
    nextData->mDrawAboveRegion.Or(nextData->mDrawAboveRegion,
                                     data->mDrawAboveRegion);
    nextData->mDrawAboveRegion.Or(nextData->mDrawAboveRegion,
                                     data->mDrawRegion);
  }

  mThebesLayerDataStack.RemoveElementAt(lastIndex);
}

static PRBool
SuppressComponentAlpha(nsDisplayListBuilder* aBuilder,
                       nsDisplayItem* aItem,
                       const nsRect& aComponentAlphaBounds)
{
  const nsRegion* windowTransparentRegion = aBuilder->GetFinalTransparentRegion();
  if (!windowTransparentRegion || windowTransparentRegion->IsEmpty())
    return PR_FALSE;

  
  
  nsIFrame* f = aItem->GetUnderlyingFrame();
  nsIFrame* ref = aBuilder->ReferenceFrame();
  if (f->PresContext() != ref->PresContext())
    return PR_FALSE;

  for (nsIFrame* t = f; t; t = t->GetParent()) {
    if (t->IsTransformed())
      return PR_FALSE;
  }

  return windowTransparentRegion->Intersects(aComponentAlphaBounds);
}

static PRBool
WindowHasTransparency(nsDisplayListBuilder* aBuilder)
{
  const nsRegion* windowTransparentRegion = aBuilder->GetFinalTransparentRegion();
  return windowTransparentRegion && !windowTransparentRegion->IsEmpty();
}

void
ContainerState::ThebesLayerData::Accumulate(nsDisplayListBuilder* aBuilder,
                                            nsDisplayItem* aItem,
                                            const nsIntRect& aVisibleRect,
                                            const nsIntRect& aDrawRect,
                                            const FrameLayerBuilder::Clip& aClip)
{
  nscolor uniformColor;
  PRBool isUniform = aItem->IsUniform(aBuilder, &uniformColor);
  
  
  
  if (!isUniform || NS_GET_A(uniformColor) > 0) {
    if (isUniform &&
        aItem->GetBounds(aBuilder).ToInsidePixels(AppUnitsPerDevPixel(aItem)).Contains(aVisibleRect)) {
      if (mVisibleRegion.IsEmpty()) {
        
        mSolidColor = uniformColor;
        mIsSolidColorInVisibleRegion = PR_TRUE;
      } else if (mIsSolidColorInVisibleRegion &&
                 mVisibleRegion.IsEqual(nsIntRegion(aVisibleRect))) {
        
        mSolidColor = NS_ComposeColors(mSolidColor, uniformColor);
      } else {
        mIsSolidColorInVisibleRegion = PR_FALSE;
      }
    } else {
      mIsSolidColorInVisibleRegion = PR_FALSE;
    }

    mVisibleRegion.Or(mVisibleRegion, aVisibleRect);
    mVisibleRegion.SimplifyOutward(4);
    mDrawRegion.Or(mDrawRegion, aDrawRect);
    mDrawRegion.SimplifyOutward(4);
  }

  


  if (aItem->GetType() == nsDisplayItem::TYPE_IMAGE && mVisibleRegion.IsEmpty()) {
    mImage = static_cast<nsDisplayImage*>(aItem);
    mImageClip = aClip;
  } else {
    mImage = nsnull;
  }
  
  PRBool forceTransparentSurface = PR_FALSE;
  nsRegion opaque = aItem->GetOpaqueRegion(aBuilder, &forceTransparentSurface);
  if (!opaque.IsEmpty()) {
    nsRegionRectIterator iter(opaque);
    nscoord appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
    for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
      
      
      
      
      
      nsIntRect rect = aClip.ApproximateIntersect(*r).ToInsidePixels(appUnitsPerDevPixel);
      nsIntRegion tmp;
      tmp.Or(mOpaqueRegion, rect);
       
       
       
       if (tmp.GetNumRects() <= 4 ||
           (WindowHasTransparency(aBuilder) &&
            aItem->GetUnderlyingFrame()->PresContext()->IsChrome())) {
        mOpaqueRegion = tmp;
      }
    }
  }
  nsRect componentAlpha = aItem->GetComponentAlphaBounds(aBuilder);
  componentAlpha.IntersectRect(componentAlpha, aItem->GetVisibleRect());
  if (!componentAlpha.IsEmpty()) {
    nscoord appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
    if (!mOpaqueRegion.Contains(componentAlpha.ToOutsidePixels(appUnitsPerDevPixel))) {
      if (SuppressComponentAlpha(aBuilder, aItem, componentAlpha)) {
        aItem->DisableComponentAlpha();
      } else {
        mNeedComponentAlpha = PR_TRUE;
      }
    }
  }
  mForceTransparentSurface = mForceTransparentSurface || forceTransparentSurface;
}

already_AddRefed<ThebesLayer>
ContainerState::FindThebesLayerFor(nsDisplayItem* aItem,
                                   const nsIntRect& aVisibleRect,
                                   const nsIntRect& aDrawRect,
                                   const FrameLayerBuilder::Clip& aClip,
                                   nsIFrame* aActiveScrolledRoot)
{
  PRInt32 i;
  PRInt32 lowestUsableLayerWithScrolledRoot = -1;
  PRInt32 topmostLayerWithScrolledRoot = -1;
  for (i = mThebesLayerDataStack.Length() - 1; i >= 0; --i) {
    ThebesLayerData* data = mThebesLayerDataStack[i];
    if (data->mDrawAboveRegion.Intersects(aVisibleRect)) {
      ++i;
      break;
    }
    if (data->mActiveScrolledRoot == aActiveScrolledRoot) {
      lowestUsableLayerWithScrolledRoot = i;
      if (topmostLayerWithScrolledRoot < 0) {
        topmostLayerWithScrolledRoot = i;
      }
    }
    if (data->mDrawRegion.Intersects(aVisibleRect))
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

  thebesLayerData->Accumulate(mBuilder, aItem, aVisibleRect, aDrawRect, aClip);
  return layer.forget();
}

static void
PaintInactiveLayer(nsDisplayListBuilder* aBuilder,
                   nsDisplayItem* aItem,
                   gfxContext* aContext)
{
  
  
  nsRefPtr<BasicLayerManager> tempManager = new BasicLayerManager();
  tempManager->BeginTransactionWithTarget(aContext);
  nsRefPtr<Layer> layer = aItem->BuildLayer(aBuilder, tempManager);
  if (!layer) {
    tempManager->EndTransaction(nsnull, nsnull);
    return;
  }
  PRInt32 appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
  nsIntRect itemVisibleRect =
    aItem->GetVisibleRect().ToOutsidePixels(appUnitsPerDevPixel);
  RestrictVisibleRegionForLayer(layer, itemVisibleRect);

  tempManager->SetRoot(layer);
  aBuilder->LayerBuilder()->WillEndTransaction(tempManager);
  tempManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer, aBuilder);
  aBuilder->LayerBuilder()->DidEndTransaction(tempManager);
}
















void
ContainerState::ProcessDisplayItems(const nsDisplayList& aList,
                                    FrameLayerBuilder::Clip& aClip)
{
  PRInt32 appUnitsPerDevPixel =
    mContainerFrame->PresContext()->AppUnitsPerDevPixel();

  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    nsDisplayItem::Type type = item->GetType();
    if (type == nsDisplayItem::TYPE_CLIP ||
        type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
      FrameLayerBuilder::Clip childClip(aClip, item);
      ProcessDisplayItems(*item->GetList(), childClip);
      continue;
    }

    NS_ASSERTION(appUnitsPerDevPixel == AppUnitsPerDevPixel(item),
      "items in a container layer should all have the same app units per dev pixel");

    nsIntRect itemVisibleRect =
      item->GetVisibleRect().ToOutsidePixels(appUnitsPerDevPixel);
    nsRect itemContent = item->GetBounds(mBuilder);
    if (aClip.mHaveClipRect) {
      itemContent.IntersectRect(aClip.mClipRect, itemContent);
    }
    mBounds.UnionRect(mBounds, itemContent);
    nsIntRect itemDrawRect = itemContent.ToOutsidePixels(appUnitsPerDevPixel);
    nsDisplayItem::LayerState layerState =
      item->GetLayerState(mBuilder, mManager);

    nsIFrame* activeScrolledRoot =
      nsLayoutUtils::GetActiveScrolledRootFor(item, mBuilder);

    
    if (layerState == LAYER_ACTIVE_FORCE ||
        layerState == LAYER_ACTIVE_EMPTY ||
        layerState == LAYER_ACTIVE && (aClip.mRoundedClipRects.IsEmpty() ||
        
        
        !aClip.IsRectClippedByRoundedCorner(item->GetVisibleRect()))) {

      
      
      NS_ASSERTION(layerState != LAYER_ACTIVE_EMPTY ||
                   itemVisibleRect.IsEmpty(),
                   "State is LAYER_ACTIVE_EMPTY but visible rect is not.");

      
      
      
      
      if (itemVisibleRect.IsEmpty() && layerState != LAYER_ACTIVE_EMPTY) {
        InvalidateForLayerChange(item, nsnull);
        continue;
      }

      aClip.RemoveRoundedCorners();

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager);
      if (!ownLayer) {
        InvalidateForLayerChange(item, ownLayer);
        continue;
      }

      ownLayer->SetIsFixedPosition(!nsLayoutUtils::ScrolledByViewportScrolling(
                                      activeScrolledRoot, mBuilder));

      
      NS_ASSERTION(ownLayer->Manager() == mManager, "Wrong manager");
      NS_ASSERTION(!ownLayer->HasUserData(&gLayerManagerUserData),
                   "We shouldn't have a FrameLayerBuilder-managed layer here!");
      
      if (aClip.mHaveClipRect) {
        ownLayer->IntersectClipRect(
            aClip.mClipRect.ToNearestPixels(appUnitsPerDevPixel));
      }
      ThebesLayerData* data = GetTopThebesLayerData();
      if (data) {
        data->mVisibleAboveRegion.Or(data->mVisibleAboveRegion, itemVisibleRect);
        
        
        
        
        data->mDrawAboveRegion.Or(data->mDrawAboveRegion, itemDrawRect);
      }
      RestrictVisibleRegionForLayer(ownLayer, itemVisibleRect);
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
      nsRefPtr<ThebesLayer> thebesLayer =
        FindThebesLayerFor(item, itemVisibleRect, itemDrawRect, aClip,
                           activeScrolledRoot);

      thebesLayer->SetIsFixedPosition(!nsLayoutUtils::ScrolledByViewportScrolling(
                                         activeScrolledRoot, mBuilder));

      InvalidateForLayerChange(item, thebesLayer);

      mBuilder->LayerBuilder()->
        AddThebesDisplayItem(thebesLayer, item, aClip, mContainerFrame,
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
      InvalidatePostTransformRegion(t, r,
              mBuilder->LayerBuilder()->GetLastPaintTransform(t));
    }
    if (aNewLayer) {
      ThebesLayer* newLayer = aNewLayer->AsThebesLayer();
      if (newLayer) {
        InvalidatePostTransformRegion(newLayer, r, newLayer->GetTransform());
      }
    }

    NS_ASSERTION(appUnitsPerDevPixel ==
                   mContainerFrame->PresContext()->AppUnitsPerDevPixel(),
                 "app units per dev pixel should be constant in a container");
    mContainerFrame->InvalidateWithFlags(
        bounds - mBuilder->ToReferenceFrame(mContainerFrame),
        nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
        nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);
  }
}

PRBool
FrameLayerBuilder::NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                    nsDisplayItem* aItem)
{
  return !aItem->ShouldFixToViewport(aBuilder) ||
      !HasRetainedLayerFor(aItem->GetUnderlyingFrame(), aItem->GetPerFrameKey());
}

void
FrameLayerBuilder::AddThebesDisplayItem(ThebesLayer* aLayer,
                                        nsDisplayItem* aItem,
                                        const Clip& aClip,
                                        nsIFrame* aContainerLayerFrame,
                                        LayerState aLayerState)
{
  AddLayerDisplayItem(aLayer, aItem);

  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerLayerFrame;
    NS_ASSERTION(aItem->GetUnderlyingFrame(), "Must have frame");
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem, aClip));
    cdi->mInactiveLayer = aLayerState != LAYER_NONE;
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

const gfx3DMatrix&
FrameLayerBuilder::GetLastPaintTransform(ThebesLayer* aLayer)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry && entry->mHasExplicitLastPaintTransform)
    return entry->mLastPaintTransform;
  return aLayer->GetTransform();
}

void
FrameLayerBuilder::SaveLastPaintTransform(ThebesLayer* aLayer,
                                          const gfx3DMatrix& aMatrix)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mLastPaintTransform = aMatrix;
    entry->mHasExplicitLastPaintTransform = PR_TRUE;
  }
}

nscolor
FrameLayerBuilder::FindOpaqueColorCovering(nsDisplayListBuilder* aBuilder,
                                           ThebesLayer* aLayer,
                                           const nsRect& aRect)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.GetEntry(aLayer);
  NS_ASSERTION(entry, "Must know about this layer!");
  for (PRInt32 i = entry->mItems.Length() - 1; i >= 0; --i) {
    nsDisplayItem* item = entry->mItems[i].mItem;
    const nsRect& visible = item->GetVisibleRect();
    if (!visible.Intersects(aRect))
      continue;

    nscolor color;
    if (visible.Contains(aRect) && item->IsUniform(aBuilder, &color) &&
        NS_GET_A(color) == 255)
      return color;
    break;
  }
  return NS_RGBA(0,0,0,0);
}

void
ContainerState::CollectOldLayers()
{
  for (Layer* layer = mContainerLayer->GetFirstChild(); layer;
       layer = layer->GetNextSibling()) {
    if (layer->HasUserData(&gColorLayerUserData)) {
      mRecycledColorLayers.AppendElement(static_cast<ColorLayer*>(layer));
    } else if (layer->HasUserData(&gImageLayerUserData)) {
      mRecycledImageLayers.AppendElement(static_cast<ImageLayer*>(layer));
    } else if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
      NS_ASSERTION(layer->AsThebesLayer(), "Wrong layer type");
      mRecycledThebesLayers.AppendElement(static_cast<ThebesLayer*>(layer));
    }
  }
}

void
ContainerState::Finish(PRUint32* aTextContentFlags)
{
  while (!mThebesLayerDataStack.IsEmpty()) {
    PopThebesLayerData();
  }

  PRUint32 textContentFlags = 0;

  for (PRUint32 i = 0; i <= mNewChildLayers.Length(); ++i) {
    
    
    Layer* layer;
    if (i < mNewChildLayers.Length()) {
      layer = mNewChildLayers[i];
      if (!layer->GetVisibleRegion().IsEmpty()) {
        textContentFlags |= layer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA;
      }
      if (!layer->GetParent()) {
        
        
        Layer* prevChild = i == 0 ? nsnull : mNewChildLayers[i - 1].get();
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

  *aTextContentFlags = textContentFlags;
}

already_AddRefed<ContainerLayer>
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
      if (oldLayer->HasUserData(&gThebesDisplayItemLayerUserData)) {
        
        
        
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
  nscoord appUnitsPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();

  if (aManager == mRetainingManager) {
    DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(aContainerFrame);
    if (entry) {
      entry->mData.AppendElement(
          DisplayItemData(containerLayer, containerDisplayItemKey));
    }

    nsPoint* offsetAtLastPaint = static_cast<nsPoint*>
      (props.Get(ThebesLayerLastPaintOffsetProperty()));
    nsPoint currentOffset = aBuilder->ToReferenceFrame(aContainerFrame);

    nsRegion* invalidThebesContent(static_cast<nsRegion*>
      (props.Get(ThebesLayerInvalidRegionProperty())));
    if (invalidThebesContent) {
      nsPoint offset = offsetAtLastPaint ? *offsetAtLastPaint : currentOffset;
      invalidThebesContent->MoveBy(offset);
      state.SetInvalidThebesContent(invalidThebesContent->
        ToOutsidePixels(appUnitsPerDevPixel));
      
      
      
      
      invalidThebesContent->MoveBy(-offset);
    } else {
      
      
      state.SetInvalidateAllThebesContent();
    }
    SetHasContainerLayer(aContainerFrame, currentOffset);
  }

  Clip clip;
  state.ProcessDisplayItems(aChildren, clip);

  
  
  
  PRUint32 flags;
  state.Finish(&flags);

  nsRect bounds = state.GetChildrenBounds();
  NS_ASSERTION(bounds.IsEqualInterior(aChildren.GetBounds(aBuilder)), "Wrong bounds");
  nsIntRect pixBounds = bounds.ToOutsidePixels(appUnitsPerDevPixel);
  containerLayer->SetVisibleRegion(pixBounds);
  
  
  if (aChildren.IsOpaque() && !aChildren.NeedsTransparentSurface() &&
      bounds.Contains(pixBounds.ToAppUnits(appUnitsPerDevPixel))) {
    
    flags = Layer::CONTENT_OPAQUE;
  }
  containerLayer->SetContentFlags(flags);

  return containerLayer.forget();
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
  if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
    
    
    
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




static PRBool
InternalInvalidateThebesLayersInSubtree(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT))
    return PR_FALSE;

  PRBool foundContainerLayer = PR_FALSE;
  if (aFrame->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER) {
    
    
    aFrame->Properties().Delete(ThebesLayerInvalidRegionProperty());
    foundContainerLayer = PR_TRUE;
  }

  PRInt32 listIndex = 0;
  nsIAtom* childList = nsnull;
  do {
    nsIFrame* child = aFrame->GetFirstChild(childList);
    if (!child && !childList) {
      nsSubDocumentFrame* subdocumentFrame = do_QueryFrame(aFrame);
      if (subdocumentFrame) {
        
        child = subdocumentFrame->GetSubdocumentRootFrame();
      }
    }
    while (child) {
      if (InternalInvalidateThebesLayersInSubtree(child)) {
        foundContainerLayer = PR_TRUE;
      }
      child = child->GetNextSibling();
    }
    childList = aFrame->GetAdditionalChildListName(listIndex++);
  } while (childList);

  if (!foundContainerLayer) {
    aFrame->RemoveStateBits(NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT);
  }
  return foundContainerLayer;
}

 void
FrameLayerBuilder::InvalidateThebesLayersInSubtree(nsIFrame* aFrame)
{
  InternalInvalidateThebesLayersInSubtree(aFrame);
}

 void
FrameLayerBuilder::InvalidateAllLayers(LayerManager* aManager)
{
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));
  if (data) {
    data->mInvalidateAllLayers = PR_TRUE;
  }
}


Layer*
FrameLayerBuilder::GetDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  void* propValue = aFrame->Properties().Get(DisplayItemDataProperty());
  if (!propValue)
    return nsnull;

  nsTArray<DisplayItemData>* array =
    (reinterpret_cast<nsTArray<DisplayItemData>*>(&propValue));
  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (!layer->HasUserData(&gColorLayerUserData) &&
          !layer->HasUserData(&gImageLayerUserData) &&
          !layer->HasUserData(&gThebesDisplayItemLayerUserData))
        return layer;
    }
  }
  return nsnull;
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

  if (builder->LayerBuilder()->CheckDOMModified())
    return;

  nsTArray<ClippedDisplayItem> items;
  nsIFrame* containerLayerFrame;
  {
    ThebesLayerItemsEntry* entry =
      builder->LayerBuilder()->mThebesLayerItems.GetEntry(aLayer);
    NS_ASSERTION(entry, "We shouldn't be drawing into a layer with no items!");
    items.SwapElements(entry->mItems);
    containerLayerFrame = entry->mContainerLayerFrame;
    
    
    
  }

  ThebesDisplayItemLayerUserData* userData =
    static_cast<ThebesDisplayItemLayerUserData*>
      (aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
  NS_ASSERTION(userData, "where did our user data go?");
  if (NS_GET_A(userData->mForcedBackgroundColor) > 0) {
    nsIntRect r = aLayer->GetVisibleRegion().GetBounds();
    aContext->NewPath();
    aContext->Rectangle(gfxRect(r.x, r.y, r.width, r.height));
    aContext->SetColor(gfxRGBA(userData->mForcedBackgroundColor));
    aContext->Fill();
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
  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  nsRect r = (aRegionToInvalidate.GetBounds() + offset).
    ToAppUnits(appUnitsPerDevPixel);
  containerLayerFrame->InvalidateWithFlags(r,
      nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
      nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);

  PRUint32 i;
  
  
  
  
  
  nsRegion visible = aRegionToDraw.ToAppUnits(appUnitsPerDevPixel);
  visible.MoveBy(NSIntPixelsToAppUnits(offset.x, appUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(offset.y, appUnitsPerDevPixel));

  for (i = items.Length(); i > 0; --i) {
    ClippedDisplayItem* cdi = &items[i - 1];

    NS_ASSERTION(AppUnitsPerDevPixel(cdi->mItem) == appUnitsPerDevPixel,
                 "a thebes layer should contain items only at the same zoom");

    NS_ABORT_IF_FALSE(cdi->mClip.mHaveClipRect ||
                      cdi->mClip.mRoundedClipRects.IsEmpty(),
                      "If we have rounded rects, we must have a clip rect");

    if (!cdi->mClip.mHaveClipRect ||
        (cdi->mClip.mRoundedClipRects.IsEmpty() &&
         cdi->mClip.mClipRect.Contains(visible.GetBounds()))) {
      cdi->mItem->RecomputeVisibility(builder, &visible);
      continue;
    }

    
    
    nsRegion clipped;
    clipped.And(visible, cdi->mClip.mClipRect);
    nsRegion finalClipped = clipped;
    cdi->mItem->RecomputeVisibility(builder, &finalClipped);
    
    
    if (cdi->mClip.mRoundedClipRects.IsEmpty()) {
      nsRegion removed;
      removed.Sub(clipped, finalClipped);
      nsRegion newVisible;
      newVisible.Sub(visible, removed);
      
      if (newVisible.GetNumRects() <= 15) {
        visible = newVisible;
      }
    }
    if (!cdi->mClip.IsRectClippedByRoundedCorner(cdi->mItem->GetVisibleRect())) {
      cdi->mClip.RemoveRoundedCorners();
    }
  }

  nsRefPtr<nsRenderingContext> rc = new nsRenderingContext();
  rc->Init(presContext->DeviceContext(), aContext);

  Clip currentClip;
  PRBool setClipRect = PR_FALSE;

  for (i = 0; i < items.Length(); ++i) {
    ClippedDisplayItem* cdi = &items[i];

    if (cdi->mItem->GetVisibleRect().IsEmpty())
      continue;

    
    
    if (setClipRect != cdi->mClip.mHaveClipRect ||
        (cdi->mClip.mHaveClipRect && cdi->mClip != currentClip)) {
      if (setClipRect) {
        aContext->Restore();
      }
      setClipRect = cdi->mClip.mHaveClipRect;
      if (setClipRect) {
        currentClip = cdi->mClip;
        aContext->Save();
        currentClip.ApplyTo(aContext, presContext);
      }
    }

    if (cdi->mInactiveLayer) {
      PaintInactiveLayer(builder, cdi->mItem, aContext);
    } else {
      cdi->mItem->Paint(builder, rc);
    }

    if (builder->LayerBuilder()->CheckDOMModified())
      break;
  }

  if (setClipRect) {
    aContext->Restore();
  }
}

PRBool
FrameLayerBuilder::CheckDOMModified()
{
  if (!mRootPresContext ||
      mInitialDOMGeneration == mRootPresContext->GetDOMGeneration())
    return PR_FALSE;
  if (mDetectedDOMModification) {
    
    return PR_TRUE;
  }
  mDetectedDOMModification = PR_TRUE;
  
  
  
  NS_WARNING("Detected DOM modification during paint, bailing out!");
  return PR_TRUE;
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

FrameLayerBuilder::Clip::Clip(const Clip& aOther, nsDisplayItem* aClipItem)
  : mRoundedClipRects(aOther.mRoundedClipRects),
    mHaveClipRect(PR_TRUE)
{
  nsDisplayItem::Type type = aClipItem->GetType();
  NS_ABORT_IF_FALSE(type == nsDisplayItem::TYPE_CLIP ||
                    type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT,
                    "unexpected display item type");
  nsDisplayClip* item = static_cast<nsDisplayClip*>(aClipItem);
  
  
  if (aOther.mHaveClipRect) {
    mClipRect.IntersectRect(aOther.mClipRect, item->GetClipRect());
  } else {
    mClipRect = item->GetClipRect();
  }

  if (type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
    RoundedRect *rr = mRoundedClipRects.AppendElement();
    if (rr) {
      rr->mRect = item->GetClipRect();
      static_cast<nsDisplayClipRoundedRect*>(item)->GetRadii(rr->mRadii);
    }
  }

  
}

void
FrameLayerBuilder::Clip::ApplyTo(gfxContext* aContext,
                                 nsPresContext* aPresContext)
{
  aContext->NewPath();
  PRInt32 A2D = aPresContext->AppUnitsPerDevPixel();
  gfxRect clip = nsLayoutUtils::RectToGfxRect(mClipRect, A2D);
  aContext->Rectangle(clip, PR_TRUE);
  aContext->Clip();

  for (PRUint32 i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    const Clip::RoundedRect &rr = mRoundedClipRects[i];

    gfxCornerSizes pixelRadii;
    nsCSSRendering::ComputePixelRadii(rr.mRadii, A2D, &pixelRadii);

    clip = nsLayoutUtils::RectToGfxRect(rr.mRect, A2D);
    clip.Round();
    clip.Condition();
    

    aContext->NewPath();
    aContext->RoundedRectangle(clip, pixelRadii);
    aContext->Clip();
  }
}

nsRect
FrameLayerBuilder::Clip::ApproximateIntersect(const nsRect& aRect) const
{
  nsRect r = aRect;
  if (mHaveClipRect) {
    r.IntersectRect(r, mClipRect);
  }
  for (PRUint32 i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    const Clip::RoundedRect &rr = mRoundedClipRects[i];
    nsRegion rgn = nsLayoutUtils::RoundedRectIntersectRect(rr.mRect, rr.mRadii, r);
    r = rgn.GetLargestRectangle();
  }
  return r;
}



bool IsInsideEllipse(nscoord aXRadius, nscoord aXCenter, nscoord aXPoint,
                     nscoord aYRadius, nscoord aYCenter, nscoord aYPoint)
{
  float scaledX = float(aXPoint - aXCenter) / float(aXRadius);
  float scaledY = float(aYPoint - aYCenter) / float(aYRadius);
  return scaledX * scaledX + scaledY * scaledY < 1.0f;
}

bool
FrameLayerBuilder::Clip::IsRectClippedByRoundedCorner(const nsRect& aRect) const
{
  if (mRoundedClipRects.IsEmpty())
    return false;

  nsRect rect;
  rect.IntersectRect(aRect, NonRoundedIntersection());
  for (PRUint32 i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    const Clip::RoundedRect &rr = mRoundedClipRects[i];
    
    if (rect.x < rr.mRect.x + rr.mRadii[NS_CORNER_TOP_LEFT_X] &&
        rect.y < rr.mRect.y + rr.mRadii[NS_CORNER_TOP_LEFT_Y]) {
      if (!IsInsideEllipse(rr.mRadii[NS_CORNER_TOP_LEFT_X],
                           rr.mRect.x + rr.mRadii[NS_CORNER_TOP_LEFT_X],
                           rect.x,
                           rr.mRadii[NS_CORNER_TOP_LEFT_Y],
                           rr.mRect.y + rr.mRadii[NS_CORNER_TOP_LEFT_Y],
                           rect.y)) {
        return true;
      }
    }
    
    if (rect.XMost() > rr.mRect.XMost() - rr.mRadii[NS_CORNER_TOP_RIGHT_X] &&
        rect.y < rr.mRect.y + rr.mRadii[NS_CORNER_TOP_RIGHT_Y]) {
      if (!IsInsideEllipse(rr.mRadii[NS_CORNER_TOP_RIGHT_X],
                           rr.mRect.XMost() - rr.mRadii[NS_CORNER_TOP_RIGHT_X],
                           rect.XMost(),
                           rr.mRadii[NS_CORNER_TOP_RIGHT_Y],
                           rr.mRect.y + rr.mRadii[NS_CORNER_TOP_RIGHT_Y],
                           rect.y)) {
        return true;
      }
    }
    
    if (rect.x < rr.mRect.x + rr.mRadii[NS_CORNER_BOTTOM_LEFT_X] &&
        rect.YMost() > rr.mRect.YMost() - rr.mRadii[NS_CORNER_BOTTOM_LEFT_Y]) {
      if (!IsInsideEllipse(rr.mRadii[NS_CORNER_BOTTOM_LEFT_X],
                           rr.mRect.x + rr.mRadii[NS_CORNER_BOTTOM_LEFT_X],
                           rect.x,
                           rr.mRadii[NS_CORNER_BOTTOM_LEFT_Y],
                           rr.mRect.YMost() - rr.mRadii[NS_CORNER_BOTTOM_LEFT_Y],
                           rect.YMost())) {
        return true;
      }
    }
    
    if (rect.XMost() > rr.mRect.XMost() - rr.mRadii[NS_CORNER_BOTTOM_RIGHT_X] &&
        rect.YMost() > rr.mRect.YMost() - rr.mRadii[NS_CORNER_BOTTOM_RIGHT_Y]) {
      if (!IsInsideEllipse(rr.mRadii[NS_CORNER_BOTTOM_RIGHT_X],
                           rr.mRect.XMost() - rr.mRadii[NS_CORNER_BOTTOM_RIGHT_X],
                           rect.XMost(),
                           rr.mRadii[NS_CORNER_BOTTOM_RIGHT_Y],
                           rr.mRect.YMost() - rr.mRadii[NS_CORNER_BOTTOM_RIGHT_Y],
                           rect.YMost())) {
        return true;
      }
    }
  }
  return false;
}

nsRect
FrameLayerBuilder::Clip::NonRoundedIntersection() const
{
  NS_ASSERTION(!mRoundedClipRects.IsEmpty(), "no rounded clip rects?");
  nsRect result = mClipRect;
  for (PRUint32 i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    result.IntersectRect(result, mRoundedClipRects[i].mRect);
  }
  return result;
}

void
FrameLayerBuilder::Clip::RemoveRoundedCorners()
{
  if (mRoundedClipRects.IsEmpty())
    return;

  mClipRect = NonRoundedIntersection();
  mRoundedClipRects.Clear();
}

} 
