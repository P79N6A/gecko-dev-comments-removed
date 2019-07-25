




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
#include "MaskLayerImageCache.h"

#include "mozilla/Preferences.h"
#include "sampler.h"

#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#ifdef DEBUG
#include <stdio.h>
#endif

using namespace mozilla::layers;

namespace mozilla {




class LayerManagerData : public LayerUserData {
public:
  LayerManagerData(LayerManager *aManager) :
    mInvalidateAllLayers(false),
    mLayerManager(aManager)
  {
    MOZ_COUNT_CTOR(LayerManagerData);
    mFramesWithLayers.Init();
  }
  ~LayerManagerData() {
    
    
    mFramesWithLayers.EnumerateEntries(
        FrameLayerBuilder::RemoveDisplayItemDataForFrame, nullptr);
    MOZ_COUNT_DTOR(LayerManagerData);
  }

  


  nsTHashtable<FrameLayerBuilder::DisplayItemDataEntry> mFramesWithLayers;
  bool mInvalidateAllLayers;
  
  nsRefPtr<LayerManager> mLayerManager;
};

LayerManagerLayerBuilder::~LayerManagerLayerBuilder()
{
  MOZ_COUNT_DTOR(LayerManagerLayerBuilder);
  if (mDelete) {
    delete mLayerBuilder;
  }
}

namespace {


static MaskLayerImageCache* gMaskLayerImageCache = nullptr;

static inline MaskLayerImageCache* GetMaskLayerImageCache()
{
  if (!gMaskLayerImageCache) {
    gMaskLayerImageCache = new MaskLayerImageCache();
  }

  return gMaskLayerImageCache;
}

static void DestroyRefCountedRegion(void* aPropertyValue)
{
  static_cast<RefCountedRegion*>(aPropertyValue)->Release();
}















NS_DECLARE_FRAME_PROPERTY(ThebesLayerInvalidRegionProperty, DestroyRefCountedRegion)

static void DestroyPoint(void* aPropertyValue)
{
  delete static_cast<nsPoint*>(aPropertyValue);
}










NS_DECLARE_FRAME_PROPERTY(ThebesLayerLastPaintOffsetProperty, DestroyPoint)





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 FrameLayerBuilder* aLayerBuilder,
                 nsIFrame* aContainerFrame,
                 ContainerLayer* aContainerLayer,
                 const FrameLayerBuilder::ContainerParameters& aParameters) :
    mBuilder(aBuilder), mManager(aManager),
    mLayerBuilder(aLayerBuilder),
    mContainerFrame(aContainerFrame), mContainerLayer(aContainerLayer),
    mParameters(aParameters),
    mNextFreeRecycledThebesLayer(0), mNextFreeRecycledColorLayer(0),
    mNextFreeRecycledImageLayer(0), mInvalidateAllThebesContent(false)
  {
    nsPresContext* presContext = aContainerFrame->PresContext();
    mAppUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    
    
    
    mSnappingEnabled = aManager->IsSnappingEffectiveTransforms() &&
      !mParameters.AllowResidualTranslation();
    mRecycledMaskImageLayers.Init();
    CollectOldLayers();
  }

  enum ProcessDisplayItemsFlags {
    NO_COMPONENT_ALPHA = 0x01,
  };

  void AddInvalidThebesContent(const nsIntRegion& aRegion)
  {
    mInvalidThebesContent.Or(mInvalidThebesContent, aRegion);
  }
  void SetInvalidateAllThebesContent()
  {
    mInvalidateAllThebesContent = true;
  }
  





  void ProcessDisplayItems(const nsDisplayList& aList,
                           FrameLayerBuilder::Clip& aClip,
                           PRUint32 aFlags);
  







  void Finish(PRUint32 *aTextContentFlags);

  nsRect GetChildrenBounds() { return mBounds; }

  nscoord GetAppUnitsPerDevPixel() { return mAppUnitsPerDevPixel; }

  nsIntRect ScaleToNearestPixels(const nsRect& aRect)
  {
    return aRect.ScaleToNearestPixels(mParameters.mXScale, mParameters.mYScale,
                                      mAppUnitsPerDevPixel);
  }
  nsIntRegion ScaleRegionToNearestPixels(const nsRegion& aRegion)
  {
    return aRegion.ScaleToNearestPixels(mParameters.mXScale, mParameters.mYScale,
                                        mAppUnitsPerDevPixel);
  }
  nsIntRect ScaleToOutsidePixels(const nsRect& aRect, bool aSnap)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleToNearestPixels(aRect);
    }
    return aRect.ScaleToOutsidePixels(mParameters.mXScale, mParameters.mYScale,
                                      mAppUnitsPerDevPixel);
  }
  nsIntRect ScaleToInsidePixels(const nsRect& aRect, bool aSnap)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleToNearestPixels(aRect);
    }
    return aRect.ScaleToInsidePixels(mParameters.mXScale, mParameters.mYScale,
                                     mAppUnitsPerDevPixel);
  }

  nsIntRegion ScaleRegionToInsidePixels(const nsRegion& aRegion, bool aSnap)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleRegionToNearestPixels(aRegion);
    }
    return aRegion.ScaleToInsidePixels(mParameters.mXScale, mParameters.mYScale,
                                        mAppUnitsPerDevPixel);
  }

  const FrameLayerBuilder::ContainerParameters& ScaleParameters() { return mParameters; };

protected:
  








  class ThebesLayerData {
  public:
    ThebesLayerData() :
      mActiveScrolledRoot(nullptr), mLayer(nullptr),
      mIsSolidColorInVisibleRegion(false),
      mNeedComponentAlpha(false),
      mForceTransparentSurface(false),
      mImage(nullptr),
      mCommonClipCount(-1) {}
    












    void Accumulate(ContainerState* aState,
                    nsDisplayItem* aItem,
                    const nsIntRect& aVisibleRect,
                    const nsIntRect& aDrawRect,
                    const FrameLayerBuilder::Clip& aClip);
    nsIFrame* GetActiveScrolledRoot() { return mActiveScrolledRoot; }

    




    already_AddRefed<ImageContainer> CanOptimizeImageLayer();

    




    nsIntRegion  mVisibleRegion;
    






    nsIntRegion  mVisibleAboveRegion;
    





    nsIntRegion  mDrawRegion;
    







    nsIntRegion  mDrawAboveRegion;
    



    nsIntRegion  mOpaqueRegion;
    




    nsIFrame*    mActiveScrolledRoot;
    ThebesLayer* mLayer;
    



    nscolor      mSolidColor;
    


    bool mIsSolidColorInVisibleRegion;
    



    bool mNeedComponentAlpha;
    





    bool mForceTransparentSurface;

    



    nsDisplayImage* mImage;
    







    FrameLayerBuilder::Clip mItemClip;
    





    PRInt32 mCommonClipCount;
    





    void UpdateCommonClipCount(const FrameLayerBuilder::Clip& aCurrentClip);
  };
  friend class ThebesLayerData;

  





  already_AddRefed<ThebesLayer> CreateOrRecycleThebesLayer(nsIFrame* aActiveScrolledRoot);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer();
  



  already_AddRefed<ImageLayer> CreateOrRecycleImageLayer();
  




  already_AddRefed<ImageLayer> CreateOrRecycleMaskImageLayerFor(Layer* aLayer);
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem, Layer* aNewLayer);
  





  nscolor FindOpaqueBackgroundColorFor(PRInt32 aThebesLayerIndex);
  




  void PopThebesLayerData();
  
















  ThebesLayerData* FindThebesLayerFor(nsDisplayItem* aItem,
                                                   const nsIntRect& aVisibleRect,
                                                   const nsIntRect& aDrawRect,
                                                   const FrameLayerBuilder::Clip& aClip,
                                                   nsIFrame* aActiveScrolledRoot);
  ThebesLayerData* GetTopThebesLayerData()
  {
    return mThebesLayerDataStack.IsEmpty() ? nullptr
        : mThebesLayerDataStack[mThebesLayerDataStack.Length() - 1].get();
  }

  








  void SetupMaskLayer(Layer *aLayer, const FrameLayerBuilder::Clip& aClip,
                      PRUint32 aRoundedRectClipCount = PR_UINT32_MAX);

  nsDisplayListBuilder*            mBuilder;
  LayerManager*                    mManager;
  FrameLayerBuilder*               mLayerBuilder;
  nsIFrame*                        mContainerFrame;
  ContainerLayer*                  mContainerLayer;
  FrameLayerBuilder::ContainerParameters mParameters;
  



  nsIntRegion                      mInvalidThebesContent;
  nsRect                           mBounds;
  nsAutoTArray<nsAutoPtr<ThebesLayerData>,1>  mThebesLayerDataStack;
  




  typedef nsAutoTArray<nsRefPtr<Layer>,1> AutoLayersArray;
  AutoLayersArray                  mNewChildLayers;
  nsTArray<nsRefPtr<ThebesLayer> > mRecycledThebesLayers;
  nsTArray<nsRefPtr<ColorLayer> >  mRecycledColorLayers;
  nsTArray<nsRefPtr<ImageLayer> >  mRecycledImageLayers;
  nsDataHashtable<nsPtrHashKey<Layer>, nsRefPtr<ImageLayer> >
    mRecycledMaskImageLayers;
  PRUint32                         mNextFreeRecycledThebesLayer;
  PRUint32                         mNextFreeRecycledColorLayer;
  PRUint32                         mNextFreeRecycledImageLayer;
  nscoord                          mAppUnitsPerDevPixel;
  bool                             mInvalidateAllThebesContent;
  bool                             mSnappingEnabled;
};

class ThebesDisplayItemLayerUserData : public LayerUserData
{
public:
  ThebesDisplayItemLayerUserData() :
    mForcedBackgroundColor(NS_RGBA(0,0,0,0)),
    mXScale(1.f), mYScale(1.f),
    mActiveScrolledRootPosition(0, 0) {}

  



  nscolor mForcedBackgroundColor;
  


  float mXScale, mYScale;

  









  gfxPoint mActiveScrolledRootPosition;
};




struct MaskLayerUserData : public LayerUserData
{
  MaskLayerUserData() : mImageKey(nullptr) {}

  bool
  operator== (const MaskLayerUserData& aOther) const
  {
    return mRoundedClipRects == aOther.mRoundedClipRects &&
           mScaleX == aOther.mScaleX &&
           mScaleY == aOther.mScaleY;
  }

  nsRefPtr<const MaskLayerImageCache::MaskLayerImageKey> mImageKey;
  
  
  nsTArray<FrameLayerBuilder::Clip::RoundedRect> mRoundedClipRects;
  
  float mScaleX, mScaleY;
};










PRUint8 gThebesDisplayItemLayerUserData;





PRUint8 gColorLayerUserData;





PRUint8 gImageLayerUserData;





PRUint8 gLayerManagerUserData;





PRUint8 gMaskLayerUserData;





MaskLayerUserData* GetMaskLayerUserData(Layer* aLayer)
{
  return static_cast<MaskLayerUserData*>(aLayer->GetUserData(&gMaskLayerUserData));
}

ThebesDisplayItemLayerUserData* GetThebesDisplayItemLayerUserData(Layer* aLayer)
{
  return static_cast<ThebesDisplayItemLayerUserData*>(
    aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
}

} 

PRUint8 gLayerManagerLayerBuilder;

 void
FrameLayerBuilder::Shutdown()
{
  if (gMaskLayerImageCache) {
    delete gMaskLayerImageCache;
    gMaskLayerImageCache = nullptr;
  }
}

void
FrameLayerBuilder::Init(nsDisplayListBuilder* aBuilder)
{
  mRootPresContext = aBuilder->ReferenceFrame()->PresContext()->GetRootPresContext();
  if (mRootPresContext) {
    mInitialDOMGeneration = mRootPresContext->GetDOMGeneration();
  }
}

bool
FrameLayerBuilder::DisplayItemDataEntry::HasNonEmptyContainerLayer()
{
  if (mIsSharingContainerLayer)
    return true;
  for (PRUint32 i = 0; i < mData.Length(); ++i) {
    if (mData[i].mLayer->GetType() == Layer::TYPE_CONTAINER &&
        mData[i].mLayerState != LAYER_ACTIVE_EMPTY)
      return true;
  }
  return false;
}

void
FrameLayerBuilder::FlashPaint(gfxContext *aContext)
{
  static bool sPaintFlashingEnabled;
  static bool sPaintFlashingPrefCached = false;

  if (!sPaintFlashingPrefCached) {
    sPaintFlashingPrefCached = true;
    mozilla::Preferences::AddBoolVarCache(&sPaintFlashingEnabled,
                                          "nglayout.debug.paint_flashing");
  }

  if (sPaintFlashingEnabled) {
    float r = float(rand()) / RAND_MAX;
    float g = float(rand()) / RAND_MAX;
    float b = float(rand()) / RAND_MAX;
    aContext->SetColor(gfxRGBA(r, g, b, 0.2));
    aContext->Paint();
  }
}

 nsTArray<FrameLayerBuilder::DisplayItemData>*
FrameLayerBuilder::GetDisplayItemDataArrayForFrame(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  LayerManagerData *data =
    reinterpret_cast<LayerManagerData*>(props.Get(LayerManagerDataProperty()));
  if (!data)
    return nullptr;

  DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(aFrame);
  NS_ASSERTION(entry, "out of sync?");
  if (!entry)
    return nullptr;

  return &entry->mData;
}

 void
FrameLayerBuilder::RemoveFrameFromLayerManager(nsIFrame* aFrame,
                                               void* aPropertyValue)
{
  LayerManagerData *data = reinterpret_cast<LayerManagerData*>(aPropertyValue);
  data->mFramesWithLayers.RemoveEntry(aFrame);
  if (data->mFramesWithLayers.Count() == 0) {
    data->mLayerManager->RemoveUserData(&gLayerManagerUserData);
  }
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

  GetMaskLayerImageCache()->Sweep();
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
  data->mInvalidateAllLayers = false;

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

 void
FrameLayerBuilder::SetAndClearInvalidRegion(DisplayItemDataEntry* aEntry)
{
  if (aEntry->mInvalidRegion) {
    nsIFrame* f = aEntry->GetKey();
    FrameProperties props = f->Properties();

    RefCountedRegion* invalidRegion;
    aEntry->mInvalidRegion.forget(&invalidRegion);

    invalidRegion->mRegion.SetEmpty();
    props.Set(ThebesLayerInvalidRegionProperty(), invalidRegion);
  }
}

 PLDHashOperator
FrameLayerBuilder::UpdateDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                 void* aUserArg)
{
  FrameLayerBuilder* builder = static_cast<FrameLayerBuilder*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();
  FrameProperties props = f->Properties();
  DisplayItemDataEntry* newDisplayItems =
    builder ? builder->mNewDisplayItemData.GetEntry(f) : nullptr;
  if (!newDisplayItems || newDisplayItems->mData.IsEmpty()) {
    
    if (newDisplayItems) {
      builder->mNewDisplayItemData.RawRemoveEntry(newDisplayItems);
    }
    bool found;
    props.Remove(LayerManagerDataProperty(), &found);
    NS_ASSERTION(found, "How can the frame property be missing?");
    SetNoContainerLayer(f);
    return PL_DHASH_REMOVE;
  }

  if (!newDisplayItems->HasNonEmptyContainerLayer()) {
    SetNoContainerLayer(f);
  }

  
  aEntry->mData.SwapElements(newDisplayItems->mData);
  aEntry->mInvalidRegion.swap(newDisplayItems->mInvalidRegion);
  
  
  SetAndClearInvalidRegion(aEntry);
  
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

  
  
  SetAndClearInvalidRegion(aEntry);

  
  NS_ASSERTION(!data->mFramesWithLayers.GetEntry(f),
               "We shouldn't get here if we're already in mFramesWithLayers");
  DisplayItemDataEntry *newEntry = data->mFramesWithLayers.PutEntry(f);
  NS_ASSERTION(!props.Get(LayerManagerDataProperty()),
               "mFramesWithLayers out of sync");

  newEntry->mData.SwapElements(aEntry->mData);
  props.Set(LayerManagerDataProperty(), data);
  return PL_DHASH_REMOVE;
}

bool
FrameLayerBuilder::HasRetainedLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  nsTArray<DisplayItemData> *array = GetDisplayItemDataArrayForFrame(aFrame);
  if (!array)
    return false;

  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (layer->Manager()->GetUserData(&gLayerManagerUserData)) {
        
        return true;
      }
    }
  }
  return false;
}

Layer*
FrameLayerBuilder::GetOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  
  
  if (!mRetainingManager || mInvalidateAllLayers)
    return nullptr;

  nsTArray<DisplayItemData> *array = GetDisplayItemDataArrayForFrame(aFrame);
  if (!array)
    return nullptr;

  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (layer->Manager() == mRetainingManager)
        return layer;
    }
  }
  return nullptr;
}

 Layer*
FrameLayerBuilder::GetDebugOldLayerFor(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  FrameProperties props = aFrame->Properties();
  LayerManagerData* data = static_cast<LayerManagerData*>(props.Get(LayerManagerDataProperty()));
  if (!data) {
    return nullptr;
  }
  DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(aFrame);
  if (!entry)
    return nullptr;

  nsTArray<DisplayItemData> *array = &entry->mData;
  if (!array)
    return nullptr;

  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      return array->ElementAt(i).mLayer;
    }
  }
  return nullptr;
}






static void
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsIntRegion& aRegion,
                              const nsIntPoint& aTranslation)
{
  
  
  
  nsIntRegion rgn = aRegion;
  rgn.MoveBy(-aTranslation);
  aLayer->InvalidateRegion(rgn);
}

already_AddRefed<ColorLayer>
ContainerState::CreateOrRecycleColorLayer()
{
  nsRefPtr<ColorLayer> layer;
  if (mNextFreeRecycledColorLayer < mRecycledColorLayers.Length()) {
    
    layer = mRecycledColorLayers[mNextFreeRecycledColorLayer];
    ++mNextFreeRecycledColorLayer;
    
    
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);
  } else {
    
    layer = mManager->CreateColorLayer();
    if (!layer)
      return nullptr;
    
    layer->SetUserData(&gColorLayerUserData, nullptr);
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
    
    
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);
  } else {
    
    layer = mManager->CreateImageLayer();
    if (!layer)
      return nullptr;
    
    layer->SetUserData(&gImageLayerUserData, nullptr);
  }
  return layer.forget();
}

already_AddRefed<ImageLayer>
ContainerState::CreateOrRecycleMaskImageLayerFor(Layer* aLayer)
{
  nsRefPtr<ImageLayer> result = mRecycledMaskImageLayers.Get(aLayer);
  if (result) {
    mRecycledMaskImageLayers.Remove(aLayer);
    
  } else {
    
    result = mManager->CreateImageLayer();
    if (!result)
      return nullptr;
    result->SetUserData(&gMaskLayerUserData, new MaskLayerUserData());
    result->SetForceSingleTile(true);
  }
  
  return result.forget();
}

static nsIntPoint
GetTranslationForThebesLayer(ThebesLayer* aLayer)
{
  gfxMatrix transform;
  if (!aLayer->GetTransform().Is2D(&transform) ||
      transform.HasNonIntegerTranslation()) {
    NS_ERROR("ThebesLayers should have integer translations only");
    return nsIntPoint(0, 0);
  }
  return nsIntPoint(PRInt32(transform.x0), PRInt32(transform.y0));
}

static const double SUBPIXEL_OFFSET_EPSILON = 0.02;

static PRBool
SubpixelOffsetFuzzyEqual(gfxPoint aV1, gfxPoint aV2)
{
  return fabs(aV2.x - aV1.x) < SUBPIXEL_OFFSET_EPSILON &&
         fabs(aV2.y - aV1.y) < SUBPIXEL_OFFSET_EPSILON;
}








static PRInt32
RoundToMatchResidual(double aValue, double aOldResidual)
{
  PRInt32 v = NSToIntRoundUp(aValue);
  double residual = aValue - v;
  if (aOldResidual < 0) {
    if (residual > 0 && fabs(residual - 1.0 - aOldResidual) < SUBPIXEL_OFFSET_EPSILON) {
      
      return PRInt32(ceil(aValue));
    }
  } else if (aOldResidual > 0) {
    if (residual < 0 && fabs(residual + 1.0 - aOldResidual) < SUBPIXEL_OFFSET_EPSILON) {
      
      return PRInt32(floor(aValue));
    }
  }
  return v;
}

already_AddRefed<ThebesLayer>
ContainerState::CreateOrRecycleThebesLayer(nsIFrame* aActiveScrolledRoot)
{
  
  nsRefPtr<ThebesLayer> layer;
  ThebesDisplayItemLayerUserData* data;
  if (mNextFreeRecycledThebesLayer < mRecycledThebesLayers.Length()) {
    
    layer = mRecycledThebesLayers[mNextFreeRecycledThebesLayer];
    ++mNextFreeRecycledThebesLayer;
    
    
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);

    data = static_cast<ThebesDisplayItemLayerUserData*>
        (layer->GetUserData(&gThebesDisplayItemLayerUserData));
    NS_ASSERTION(data, "Recycled ThebesLayers must have user data");

    
    
    
    
    
    
    
    
    if (mInvalidateAllThebesContent ||
        data->mXScale != mParameters.mXScale ||
        data->mYScale != mParameters.mYScale) {
      nsIntRect invalidate = layer->GetValidRegion().GetBounds();
      layer->InvalidateRegion(invalidate);
    } else {
      InvalidatePostTransformRegion(layer, mInvalidThebesContent,
                                    GetTranslationForThebesLayer(layer));
    }
    
    
    
  } else {
    
    layer = mManager->CreateThebesLayer();
    if (!layer)
      return nullptr;
    
    data = new ThebesDisplayItemLayerUserData();
    layer->SetUserData(&gThebesDisplayItemLayerUserData, data);
  }
  data->mXScale = mParameters.mXScale;
  data->mYScale = mParameters.mYScale;
  layer->SetAllowResidualTranslation(mParameters.AllowResidualTranslation());

  mLayerBuilder->SaveLastPaintOffset(layer);

  
  
  nsPoint offset = mBuilder->ToReferenceFrame(aActiveScrolledRoot);
  nscoord appUnitsPerDevPixel = aActiveScrolledRoot->PresContext()->AppUnitsPerDevPixel();
  gfxPoint scaledOffset(
      NSAppUnitsToDoublePixels(offset.x, appUnitsPerDevPixel)*mParameters.mXScale,
      NSAppUnitsToDoublePixels(offset.y, appUnitsPerDevPixel)*mParameters.mYScale);
  
  
  nsIntPoint pixOffset(RoundToMatchResidual(scaledOffset.x, data->mActiveScrolledRootPosition.x),
                       RoundToMatchResidual(scaledOffset.y, data->mActiveScrolledRootPosition.y));
  gfxMatrix matrix;
  matrix.Translate(gfxPoint(pixOffset.x, pixOffset.y));
  layer->SetBaseTransform(gfx3DMatrix::From2D(matrix));

  
#ifndef MOZ_JAVA_COMPOSITOR
  
  
  gfxPoint activeScrolledRootTopLeft = scaledOffset - matrix.GetTranslation();
  
  
  
  if (!SubpixelOffsetFuzzyEqual(activeScrolledRootTopLeft, data->mActiveScrolledRootPosition)) {
    data->mActiveScrolledRootPosition = activeScrolledRootTopLeft;
    nsIntRect invalidate = layer->GetValidRegion().GetBounds();
    layer->InvalidateRegion(invalidate);
  }
#endif

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
  gfx3DMatrix transform = aLayer->GetTransform();

  
  
  gfxRect itemVisible(aItemVisible.x, aItemVisible.y, aItemVisible.width, aItemVisible.height);
  gfxRect layerVisible = transform.Inverse().ProjectRectBounds(itemVisible);
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

    
    
    nsRect rect =
      target->mVisibleRegion.GetBounds().ToAppUnits(mAppUnitsPerDevPixel);
    rect.ScaleInverseRoundOut(mParameters.mXScale, mParameters.mYScale);
    return mLayerBuilder->
      FindOpaqueColorCovering(mBuilder, candidate->mLayer, rect);
  }
  return NS_RGBA(0,0,0,0);
}

void
ContainerState::ThebesLayerData::UpdateCommonClipCount(
    const FrameLayerBuilder::Clip& aCurrentClip)
{
  if (mCommonClipCount >= 0) {
    PRInt32 end = NS_MIN<PRInt32>(aCurrentClip.mRoundedClipRects.Length(),
                                  mCommonClipCount);
    PRInt32 clipCount = 0;
    for (; clipCount < end; ++clipCount) {
      if (mItemClip.mRoundedClipRects[clipCount] !=
          aCurrentClip.mRoundedClipRects[clipCount]) {
        break;
      }
    }
    mCommonClipCount = clipCount;
    NS_ASSERTION(mItemClip.mRoundedClipRects.Length() >= PRUint32(mCommonClipCount),
                 "Inconsistent common clip count.");
  } else {
    
    mCommonClipCount = aCurrentClip.mRoundedClipRects.Length();
  }
}

already_AddRefed<ImageContainer>
ContainerState::ThebesLayerData::CanOptimizeImageLayer()
{
  if (!mImage) {
    return nullptr;
  }

  return mImage->GetContainer();
}

void
ContainerState::PopThebesLayerData()
{
  NS_ASSERTION(!mThebesLayerDataStack.IsEmpty(), "Can't pop");

  PRInt32 lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer();

  if ((data->mIsSolidColorInVisibleRegion || imageContainer) &&
      data->mLayer->GetValidRegion().IsEmpty()) {
    NS_ASSERTION(!(data->mIsSolidColorInVisibleRegion && imageContainer),
                 "Can't be a solid color as well as an image!");
    if (imageContainer) {
      nsRefPtr<ImageLayer> imageLayer = CreateOrRecycleImageLayer();
      imageLayer->SetContainer(imageContainer);
      data->mImage->ConfigureLayer(imageLayer);
      imageLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      if (data->mItemClip.mHaveClipRect) {
        nsIntRect clip = ScaleToNearestPixels(data->mItemClip.mClipRect);
        imageLayer->IntersectClipRect(clip);
      }
      layer = imageLayer;
    } else {
      nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer();
      colorLayer->SetIsFixedPosition(data->mLayer->GetIsFixedPosition());
      colorLayer->SetColor(data->mSolidColor);

      
      colorLayer->SetBaseTransform(data->mLayer->GetBaseTransform());
      colorLayer->SetPostScale(data->mLayer->GetPostXScale(), data->mLayer->GetPostYScale());

      
      
      
      
      
      
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
    imageContainer = nullptr;
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
  bool isOpaque = transparentRegion.IsEmpty();
  
  
  
  if (layer == data->mLayer) {
    nscolor backgroundColor = NS_RGBA(0,0,0,0);
    if (!isOpaque) {
      backgroundColor = FindOpaqueBackgroundColorFor(lastIndex);
      if (NS_GET_A(backgroundColor) == 255) {
        isOpaque = true;
      }
    }

    
    ThebesDisplayItemLayerUserData* userData =
      GetThebesDisplayItemLayerUserData(data->mLayer);
    NS_ASSERTION(userData, "where did our user data go?");
    if (userData->mForcedBackgroundColor != backgroundColor) {
      
      
      data->mLayer->InvalidateRegion(data->mLayer->GetValidRegion());
    }
    userData->mForcedBackgroundColor = backgroundColor;

    
    PRInt32 commonClipCount = data->mCommonClipCount;
    NS_ASSERTION(commonClipCount >= 0, "Inconsistent clip count.");
    SetupMaskLayer(layer, data->mItemClip, commonClipCount);
    
    FrameLayerBuilder::ThebesLayerItemsEntry* entry = mLayerBuilder->
      GetThebesLayerItemsEntry(static_cast<ThebesLayer*>(layer.get()));
    entry->mCommonClipCount = commonClipCount;
  } else {
    
    SetupMaskLayer(layer, data->mItemClip);
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
    nextData->mVisibleAboveRegion.SimplifyOutward(4);
    nextData->mDrawAboveRegion.Or(nextData->mDrawAboveRegion,
                                     data->mDrawAboveRegion);
    nextData->mDrawAboveRegion.Or(nextData->mDrawAboveRegion,
                                     data->mDrawRegion);
    nextData->mDrawAboveRegion.SimplifyOutward(4);
  }

  mThebesLayerDataStack.RemoveElementAt(lastIndex);
}

static bool
SuppressComponentAlpha(nsDisplayListBuilder* aBuilder,
                       nsDisplayItem* aItem,
                       const nsRect& aComponentAlphaBounds)
{
  const nsRegion* windowTransparentRegion = aBuilder->GetFinalTransparentRegion();
  if (!windowTransparentRegion || windowTransparentRegion->IsEmpty())
    return false;

  
  
  nsIFrame* f = aItem->GetUnderlyingFrame();
  nsIFrame* ref = aBuilder->ReferenceFrame();
  if (f->PresContext() != ref->PresContext())
    return false;

  for (nsIFrame* t = f; t; t = t->GetParent()) {
    if (t->IsTransformed())
      return false;
  }

  return windowTransparentRegion->Intersects(aComponentAlphaBounds);
}

static bool
WindowHasTransparency(nsDisplayListBuilder* aBuilder)
{
  const nsRegion* windowTransparentRegion = aBuilder->GetFinalTransparentRegion();
  return windowTransparentRegion && !windowTransparentRegion->IsEmpty();
}

void
ContainerState::ThebesLayerData::Accumulate(ContainerState* aState,
                                            nsDisplayItem* aItem,
                                            const nsIntRect& aVisibleRect,
                                            const nsIntRect& aDrawRect,
                                            const FrameLayerBuilder::Clip& aClip)
{
  if (aState->mBuilder->NeedToForceTransparentSurfaceForItem(aItem)) {
    mForceTransparentSurface = true;
  }
  if (aState->mParameters.mDisableSubpixelAntialiasingInDescendants) {
    
    
    
    aItem->DisableComponentAlpha();
  }

  


  if (mVisibleRegion.IsEmpty() && aItem->GetType() == nsDisplayItem::TYPE_IMAGE) {
    mImage = static_cast<nsDisplayImage*>(aItem);
  } else {
    mImage = nullptr;
  }
  mItemClip = aClip;

  if (!mIsSolidColorInVisibleRegion && mOpaqueRegion.Contains(aDrawRect) &&
      mVisibleRegion.Contains(aVisibleRect)) {
    
    
    
    
    
    
    
    
    NS_ASSERTION(mDrawRegion.Contains(aDrawRect), "Draw region not covered");
    return;
  }

  nscolor uniformColor;
  bool isUniform = aItem->IsUniform(aState->mBuilder, &uniformColor);

  
  
  
  if (!isUniform || NS_GET_A(uniformColor) > 0) {
    
    
    
    if (isUniform) {
      bool snap;
      nsRect bounds = aItem->GetBounds(aState->mBuilder, &snap);
      if (!aState->ScaleToInsidePixels(bounds, snap).Contains(aVisibleRect)) {
        isUniform = false;
      }
    }
    if (isUniform && aClip.mRoundedClipRects.IsEmpty()) {
      if (mVisibleRegion.IsEmpty()) {
        
        mSolidColor = uniformColor;
        mIsSolidColorInVisibleRegion = true;
      } else if (mIsSolidColorInVisibleRegion &&
                 mVisibleRegion.IsEqual(nsIntRegion(aVisibleRect))) {
        
        mSolidColor = NS_ComposeColors(mSolidColor, uniformColor);
      } else {
        mIsSolidColorInVisibleRegion = false;
      }
    } else {
      mIsSolidColorInVisibleRegion = false;
    }

    mVisibleRegion.Or(mVisibleRegion, aVisibleRect);
    mVisibleRegion.SimplifyOutward(4);
    mDrawRegion.Or(mDrawRegion, aDrawRect);
    mDrawRegion.SimplifyOutward(4);
  }
  
  bool snap;
  nsRegion opaque = aItem->GetOpaqueRegion(aState->mBuilder, &snap);
  if (!opaque.IsEmpty()) {
    nsRegion opaqueClipped;
    nsRegionRectIterator iter(opaque);
    for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
      opaqueClipped.Or(opaqueClipped, aClip.ApproximateIntersect(*r));
    }

    nsIntRegion opaquePixels = aState->ScaleRegionToInsidePixels(opaqueClipped, snap);

    nsIntRegionRectIterator iter2(opaquePixels);
    for (const nsIntRect* r = iter2.Next(); r; r = iter2.Next()) {
      
      
      
      
      
      nsIntRegion tmp;
      tmp.Or(mOpaqueRegion, *r);
       
       
       
       if (tmp.GetNumRects() <= 4 ||
           (WindowHasTransparency(aState->mBuilder) &&
            aItem->GetUnderlyingFrame()->PresContext()->IsChrome())) {
        mOpaqueRegion = tmp;
      }
    }
  }

  if (!aState->mParameters.mDisableSubpixelAntialiasingInDescendants) {
    nsRect componentAlpha = aItem->GetComponentAlphaBounds(aState->mBuilder);
    if (!componentAlpha.IsEmpty()) {
      nsIntRect componentAlphaRect =
        aState->ScaleToOutsidePixels(componentAlpha, false).Intersect(aVisibleRect);
      if (!mOpaqueRegion.Contains(componentAlphaRect)) {
        if (SuppressComponentAlpha(aState->mBuilder, aItem, componentAlpha)) {
          aItem->DisableComponentAlpha();
        } else {
          mNeedComponentAlpha = true;
        }
      }
    }
  }
}

ContainerState::ThebesLayerData*
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
  ThebesLayerData* thebesLayerData = nullptr;
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

  thebesLayerData->Accumulate(this, aItem, aVisibleRect, aDrawRect, aClip);

  return thebesLayerData;
}

#ifdef MOZ_DUMP_PAINTING
static void
DumpPaintedImage(nsDisplayItem* aItem, gfxASurface* aSurf)
{
  nsCString string(aItem->Name());
  string.Append("-");
  string.AppendInt((PRUint64)aItem);
  fprintf(gfxUtils::sDumpPaintFile, "array[\"%s\"]=\"", string.BeginReading());
  aSurf->DumpAsDataURL(gfxUtils::sDumpPaintFile);
  fprintf(gfxUtils::sDumpPaintFile, "\";");
}
#endif

static void
PaintInactiveLayer(nsDisplayListBuilder* aBuilder,
                   nsDisplayItem* aItem,
                   gfxContext* aContext,
                   nsRenderingContext* aCtx,
                   FrameLayerBuilder *aLayerBuilder)
{
  
  
  PRInt32 appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
  nsIntRect itemVisibleRect =
    aItem->GetVisibleRect().ToOutsidePixels(appUnitsPerDevPixel);

  nsRefPtr<gfxContext> context = aContext;
#ifdef MOZ_DUMP_PAINTING
  nsRefPtr<gfxASurface> surf;
  if (gfxUtils::sDumpPainting) {
    surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(itemVisibleRect.Size(),
                                                              gfxASurface::CONTENT_COLOR_ALPHA);
    surf->SetDeviceOffset(-itemVisibleRect.TopLeft());
    context = new gfxContext(surf);
  }
#endif

  nsRefPtr<BasicLayerManager> tempManager = new BasicLayerManager();
  tempManager->SetUserData(&gLayerManagerLayerBuilder, new LayerManagerLayerBuilder(aLayerBuilder, false));
  tempManager->BeginTransactionWithTarget(context);
  nsRefPtr<Layer> layer =
    aItem->BuildLayer(aBuilder, tempManager, FrameLayerBuilder::ContainerParameters());
  if (!layer) {
    tempManager->EndTransaction(nullptr, nullptr);
    return;
  }
  RestrictVisibleRegionForLayer(layer, itemVisibleRect);
  
  tempManager->SetRoot(layer);
  aLayerBuilder->WillEndTransaction(tempManager);
  if (aItem->GetType() == nsDisplayItem::TYPE_SVG_EFFECTS) {
    static_cast<nsDisplaySVGEffects*>(aItem)->PaintAsLayer(aBuilder, aCtx, tempManager);
    if (tempManager->InTransaction()) {
      tempManager->AbortTransaction();
    }
  } else {
    tempManager->EndTransaction(FrameLayerBuilder::DrawThebesLayer, aBuilder);
  }
  aLayerBuilder->DidEndTransaction(tempManager);
 
#ifdef MOZ_DUMP_PAINTING
  if (gfxUtils::sDumpPainting) {
    DumpPaintedImage(aItem, surf);

    surf->SetDeviceOffset(gfxPoint(0, 0));
    aContext->SetSource(surf, itemVisibleRect.TopLeft());
    aContext->Rectangle(itemVisibleRect);
    aContext->Fill();
    aItem->SetPainted();
  }
#endif
}















void
ContainerState::ProcessDisplayItems(const nsDisplayList& aList,
                                    FrameLayerBuilder::Clip& aClip,
                                    PRUint32 aFlags)
{
  SAMPLE_LABEL("ContainerState", "ProcessDisplayItems");
  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    nsDisplayItem::Type type = item->GetType();
    if (type == nsDisplayItem::TYPE_CLIP ||
        type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
      FrameLayerBuilder::Clip childClip(aClip, item);
      ProcessDisplayItems(*item->GetList(), childClip, aFlags);
      continue;
    }

    NS_ASSERTION(mAppUnitsPerDevPixel == AppUnitsPerDevPixel(item),
      "items in a container layer should all have the same app units per dev pixel");

    nsIntRect itemVisibleRect =
      ScaleToOutsidePixels(item->GetVisibleRect(), false);
    bool snap;
    nsRect itemContent = item->GetBounds(mBuilder, &snap);
    nsIntRect itemDrawRect = ScaleToOutsidePixels(itemContent, snap);
    if (aClip.mHaveClipRect) {
      itemContent.IntersectRect(itemContent, aClip.mClipRect);
      nsIntRect clipRect = ScaleToNearestPixels(aClip.mClipRect);
      itemDrawRect.IntersectRect(itemDrawRect, clipRect);
    }
    mBounds.UnionRect(mBounds, itemContent);
    itemVisibleRect.IntersectRect(itemVisibleRect, itemDrawRect);

    LayerState layerState = item->GetLayerState(mBuilder, mManager, mParameters);

    nsIFrame* activeScrolledRoot;
    bool forceInactive = false;
    if (aFlags & NO_COMPONENT_ALPHA) {
      activeScrolledRoot =
        nsLayoutUtils::GetActiveScrolledRootFor(mContainerFrame,
                                                mBuilder->ReferenceFrame());
      forceInactive = true;
    } else {
      activeScrolledRoot = nsLayoutUtils::GetActiveScrolledRootFor(item, mBuilder);
    }

    
    if (layerState == LAYER_ACTIVE_FORCE ||
        (!forceInactive &&
         (layerState == LAYER_ACTIVE_EMPTY ||
          layerState == LAYER_ACTIVE))) {

      
      
      NS_ASSERTION(layerState != LAYER_ACTIVE_EMPTY ||
                   itemVisibleRect.IsEmpty(),
                   "State is LAYER_ACTIVE_EMPTY but visible rect is not.");

      
      
      
      
      if (itemVisibleRect.IsEmpty() && layerState != LAYER_ACTIVE_EMPTY) {
        InvalidateForLayerChange(item, nullptr);
        continue;
      }

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager, mParameters);
      if (!ownLayer) {
        InvalidateForLayerChange(item, ownLayer);
        continue;
      }

      
      
      if (!ownLayer->AsContainerLayer()) {
        ownLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      }

      ownLayer->SetIsFixedPosition(
        !nsLayoutUtils::IsScrolledByRootContentDocumentDisplayportScrolling(
                                      activeScrolledRoot, mBuilder));

      
      NS_ASSERTION(ownLayer->Manager() == mManager, "Wrong manager");
      NS_ASSERTION(!ownLayer->HasUserData(&gLayerManagerUserData),
                   "We shouldn't have a FrameLayerBuilder-managed layer here!");
      NS_ASSERTION(aClip.mHaveClipRect ||
                     aClip.mRoundedClipRects.IsEmpty(),
                   "If we have rounded rects, we must have a clip rect");
      
      if (aClip.mHaveClipRect) {
        ownLayer->IntersectClipRect(
          ScaleToNearestPixels(aClip.NonRoundedIntersection()));
      }
      ThebesLayerData* data = GetTopThebesLayerData();
      if (data) {
        data->mVisibleAboveRegion.Or(data->mVisibleAboveRegion, itemVisibleRect);
        data->mVisibleAboveRegion.SimplifyOutward(4);
        
        
        
        
        data->mDrawAboveRegion.Or(data->mDrawAboveRegion, itemDrawRect);
        data->mDrawAboveRegion.SimplifyOutward(4);
      }
      RestrictVisibleRegionForLayer(ownLayer, itemVisibleRect);

      
      
      if (aClip.IsRectClippedByRoundedCorner(itemContent)) {
          SetupMaskLayer(ownLayer, aClip);
      }

      ContainerLayer* oldContainer = ownLayer->GetParent();
      if (oldContainer && oldContainer != mContainerLayer) {
        oldContainer->RemoveChild(ownLayer);
      }
      NS_ASSERTION(!mNewChildLayers.Contains(ownLayer),
                   "Layer already in list???");

      InvalidateForLayerChange(item, ownLayer);

      mNewChildLayers.AppendElement(ownLayer);
      mLayerBuilder->AddLayerDisplayItem(ownLayer, item, layerState);
    } else {
      ThebesLayerData* data =
        FindThebesLayerFor(item, itemVisibleRect, itemDrawRect, aClip,
                           activeScrolledRoot);

      data->mLayer->SetIsFixedPosition(
        !nsLayoutUtils::IsScrolledByRootContentDocumentDisplayportScrolling(
                                       activeScrolledRoot, mBuilder));

      InvalidateForLayerChange(item, data->mLayer);

      mLayerBuilder->AddThebesDisplayItem(data->mLayer, item, aClip,
                                          mContainerFrame,
                                          layerState);

      
      
      data->UpdateCommonClipCount(aClip);
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
  Layer* oldLayer = mLayerBuilder->GetOldLayerFor(f, key);
  if (!oldLayer) {
    
    return;
  }
  if (aNewLayer != oldLayer) {
    
    
    
    
    
    bool snap;
    nsRect bounds = aItem->GetBounds(mBuilder, &snap);

    ThebesLayer* t = oldLayer->AsThebesLayer();
    if (t) {
      ThebesDisplayItemLayerUserData* data =
          static_cast<ThebesDisplayItemLayerUserData*>(t->GetUserData(&gThebesDisplayItemLayerUserData));
      
      
      
      InvalidatePostTransformRegion(t,
          bounds.ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
          mLayerBuilder->GetLastPaintOffset(t));
    }
    if (aNewLayer) {
      ThebesLayer* newLayer = aNewLayer->AsThebesLayer();
      if (newLayer) {
        ThebesDisplayItemLayerUserData* data =
            static_cast<ThebesDisplayItemLayerUserData*>(newLayer->GetUserData(&gThebesDisplayItemLayerUserData));
        InvalidatePostTransformRegion(newLayer,
            bounds.ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
            GetTranslationForThebesLayer(newLayer));
      }
    }

    mContainerFrame->InvalidateWithFlags(
        bounds - mBuilder->ToReferenceFrame(mContainerFrame),
        nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
        nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);
  }
}

bool
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
  AddLayerDisplayItem(aLayer, aItem, aLayerState);

  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerLayerFrame;
    entry->mContainerLayerGeneration = mContainerLayerGeneration;
    NS_ASSERTION(aItem->GetUnderlyingFrame(), "Must have frame");
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem, aClip,
                                                     mContainerLayerGeneration));
    cdi->mInactiveLayer = aLayerState != LAYER_NONE;
  }
}

void
FrameLayerBuilder::AddLayerDisplayItem(Layer* aLayer,
                                       nsDisplayItem* aItem,
                                       LayerState aLayerState)
{
  if (aLayer->Manager() != mRetainingManager)
    return;

  nsIFrame* f = aItem->GetUnderlyingFrame();
  DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(f);
  entry->mContainerLayerGeneration = mContainerLayerGeneration;
  if (entry) {
    entry->mData.AppendElement(DisplayItemData(aLayer, aItem->GetPerFrameKey(), aLayerState, mContainerLayerGeneration));
  }
}

nsIntPoint
FrameLayerBuilder::GetLastPaintOffset(ThebesLayer* aLayer)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerGeneration = mContainerLayerGeneration;
    if (entry->mHasExplicitLastPaintOffset)
      return entry->mLastPaintOffset;
  }
  return GetTranslationForThebesLayer(aLayer);
}

void
FrameLayerBuilder::SaveLastPaintOffset(ThebesLayer* aLayer)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerGeneration = mContainerLayerGeneration;
    entry->mLastPaintOffset = GetTranslationForThebesLayer(aLayer);
    entry->mHasExplicitLastPaintOffset = true;
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
    NS_ASSERTION(!layer->HasUserData(&gMaskLayerUserData),
                 "Mask layer in layer tree; could not be recycled.");
    if (layer->HasUserData(&gColorLayerUserData)) {
      mRecycledColorLayers.AppendElement(static_cast<ColorLayer*>(layer));
    } else if (layer->HasUserData(&gImageLayerUserData)) {
      mRecycledImageLayers.AppendElement(static_cast<ImageLayer*>(layer));
    } else if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
      NS_ASSERTION(layer->AsThebesLayer(), "Wrong layer type");
      mRecycledThebesLayers.AppendElement(static_cast<ThebesLayer*>(layer));
    }

    if (Layer* maskLayer = layer->GetMaskLayer()) {
      NS_ASSERTION(maskLayer->GetType() == Layer::TYPE_IMAGE,
                   "Could not recycle mask layer, unsupported layer type.");
      mRecycledMaskImageLayers.Put(layer, static_cast<ImageLayer*>(maskLayer));
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
        
        
        Layer* prevChild = i == 0 ? nullptr : mNewChildLayers[i - 1].get();
        mContainerLayer->InsertAfter(layer, prevChild);
        continue;
      }
      NS_ASSERTION(layer->GetParent() == mContainerLayer,
                   "Layer shouldn't be the child of some other container");
    } else {
      layer = nullptr;
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

static FrameLayerBuilder::ContainerParameters
ChooseScaleAndSetTransform(FrameLayerBuilder* aLayerBuilder,
                           nsIFrame* aContainerFrame,
                           const gfx3DMatrix* aTransform,
                           const FrameLayerBuilder::ContainerParameters& aIncomingScale,
                           ContainerLayer* aLayer)
{
  gfx3DMatrix transform =
    gfx3DMatrix::ScalingMatrix(aIncomingScale.mXScale, aIncomingScale.mYScale, 1.0);
  if (aTransform) {
    
    transform = (*aTransform)*transform;
  }

  gfxMatrix transform2d;
  bool canDraw2D = transform.CanDraw2D(&transform2d);
  gfxSize scale;
  bool isRetained = aLayerBuilder->GetRetainingLayerManager() == aLayer->Manager();
  
  
  
  if (canDraw2D && isRetained) {
    
    scale = transform2d.ScaleFactors(true);
    
    
    
    
    
    
    gfxMatrix frameTransform;
    if (aContainerFrame->AreLayersMarkedActive(nsChangeHint_UpdateTransformLayer) &&
        aTransform &&
        (!aTransform->Is2D(&frameTransform) || frameTransform.HasNonTranslationOrFlip())) {
      
      
      bool clamp = true;
      gfxMatrix oldFrameTransform2d;
      if (aLayer->GetTransform().Is2D(&oldFrameTransform2d)) {
        gfxSize oldScale = oldFrameTransform2d.ScaleFactors(true);
        if (oldScale == scale || oldScale == gfxSize(1.0, 1.0))
          clamp = false;
      }
      if (clamp) {
        scale.width = gfxUtils::ClampToScaleFactor(scale.width);
        scale.height = gfxUtils::ClampToScaleFactor(scale.height);
      }
    } else {
      
    }
    
    
    if (fabs(scale.width) < 1e-8 || fabs(scale.height) < 1e-8) {
      scale = gfxSize(1.0, 1.0);
    }
  } else {
    scale = gfxSize(1.0, 1.0);
  }

  
  aLayer->SetBaseTransform(transform);
  aLayer->SetPreScale(1.0f/float(scale.width),
                      1.0f/float(scale.height));

  FrameLayerBuilder::ContainerParameters
    result(scale.width, scale.height, aIncomingScale);
  if (aTransform) {
    result.mInTransformedSubtree = true;
    if (aContainerFrame->AreLayersMarkedActive(nsChangeHint_UpdateTransformLayer)) {
      result.mInActiveTransformedSubtree = true;
    }
  }
  if (isRetained && (!canDraw2D || transform2d.HasNonIntegerTranslation())) {
    result.mDisableSubpixelAntialiasingInDescendants = true;
  }
  return result;
}

static void
ApplyThebesLayerInvalidation(nsDisplayListBuilder* aBuilder,
                             nsIFrame* aContainerFrame,
                             nsDisplayItem* aContainerItem,
                             ContainerState& aState,
                             nsPoint* aCurrentOffset)
{
  *aCurrentOffset = aContainerItem ? aContainerItem->ToReferenceFrame()
    : aBuilder->ToReferenceFrame(aContainerFrame);

  FrameProperties props = aContainerFrame->Properties();
  RefCountedRegion* invalidThebesContent = static_cast<RefCountedRegion*>
    (props.Get(ThebesLayerInvalidRegionProperty()));
  if (invalidThebesContent) {
    const FrameLayerBuilder::ContainerParameters& scaleParameters = aState.ScaleParameters();
    aState.AddInvalidThebesContent(invalidThebesContent->mRegion.
      ScaleToOutsidePixels(scaleParameters.mXScale, scaleParameters.mYScale,
                           aState.GetAppUnitsPerDevPixel()));
    
    
    
    
    
  } else {
    
    
    aState.SetInvalidateAllThebesContent();
  }
}

 PLDHashOperator
FrameLayerBuilder::RestoreDisplayItemData(DisplayItemDataEntry* aEntry, void* aUserArg)
{
  PRUint32 *generation = static_cast<PRUint32*>(aUserArg);

  if (aEntry->mContainerLayerGeneration >= *generation) {
    return PL_DHASH_REMOVE;
  }

  for (PRUint32 i = 0; i < aEntry->mData.Length(); i++) {
    if (aEntry->mData[i].mContainerLayerGeneration >= *generation) {
      aEntry->mData.TruncateLength(i);
      return PL_DHASH_NEXT;
    }
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
FrameLayerBuilder::RestoreThebesLayerItemEntries(ThebesLayerItemsEntry* aEntry, void* aUserArg)
{
  PRUint32 *generation = static_cast<PRUint32*>(aUserArg);

  if (aEntry->mContainerLayerGeneration >= *generation) {
    return PL_DHASH_REMOVE;
  }

  for (PRUint32 i = 0; i < aEntry->mItems.Length(); i++) {
    if (aEntry->mItems[i].mContainerLayerGeneration >= *generation) {
      aEntry->mItems.TruncateLength(i);
      return PL_DHASH_NEXT;
    }
  }

  return PL_DHASH_NEXT;
}

already_AddRefed<ContainerLayer>
FrameLayerBuilder::BuildContainerLayerFor(nsDisplayListBuilder* aBuilder,
                                          LayerManager* aManager,
                                          nsIFrame* aContainerFrame,
                                          nsDisplayItem* aContainerItem,
                                          const nsDisplayList& aChildren,
                                          const ContainerParameters& aParameters,
                                          const gfx3DMatrix* aTransform)
{
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
        
        containerLayer->SetClipRect(nullptr);
        containerLayer->SetMaskLayer(nullptr);
      }
    }
  }
  if (!containerLayer) {
    
    containerLayer = aManager->CreateContainerLayer();
    if (!containerLayer)
      return nullptr;
  }

  if (aContainerItem &&
      aContainerItem->GetLayerState(aBuilder, aManager, aParameters) == LAYER_ACTIVE_EMPTY) {
    
    
    
    
    NS_ASSERTION(aChildren.IsEmpty(), "Should have no children");
    return containerLayer.forget();
  }

  ContainerParameters scaleParameters =
    ChooseScaleAndSetTransform(this, aContainerFrame, aTransform, aParameters,
                               containerLayer);

  PRUint32 oldGeneration = mContainerLayerGeneration;
  mContainerLayerGeneration = ++mMaxContainerLayerGeneration;

  nsRefPtr<RefCountedRegion> thebesLayerInvalidRegion = nullptr;
  if (aManager == mRetainingManager) {
    FrameProperties props = aContainerFrame->Properties();
    DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(aContainerFrame);
    if (entry) {
      entry->mData.AppendElement(
          DisplayItemData(containerLayer, containerDisplayItemKey,
                          LAYER_ACTIVE, mContainerLayerGeneration));
      thebesLayerInvalidRegion = static_cast<RefCountedRegion*>
        (props.Get(ThebesLayerInvalidRegionProperty()));
      if (!thebesLayerInvalidRegion) {
        thebesLayerInvalidRegion = new RefCountedRegion();
      }
      entry->mInvalidRegion = thebesLayerInvalidRegion;
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
  }

  nsRect bounds;
  nsIntRect pixBounds;
  PRInt32 appUnitsPerDevPixel;
  PRUint32 stateFlags =
    (aContainerFrame->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) ?
      ContainerState::NO_COMPONENT_ALPHA : 0;
  PRUint32 flags;
  bool flattenedLayers = false;
  while (true) {
    ContainerState state(aBuilder, aManager, GetLayerBuilderForManager(aManager),
                         aContainerFrame, containerLayer, scaleParameters);
    if (flattenedLayers) {
      state.SetInvalidateAllThebesContent();
    }

    if (aManager == mRetainingManager) {
      nsPoint currentOffset;
      ApplyThebesLayerInvalidation(aBuilder, aContainerFrame, aContainerItem, state,
                                   &currentOffset);
      SetHasContainerLayer(aContainerFrame, currentOffset);

      nsAutoTArray<nsIFrame*,4> mergedFrames;
      if (aContainerItem) {
        aContainerItem->GetMergedFrames(&mergedFrames);
      }
      for (PRUint32 i = 0; i < mergedFrames.Length(); ++i) {
        nsIFrame* mergedFrame = mergedFrames[i];
        DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(mergedFrame);
        if (entry) {
          
          
          entry->mIsSharingContainerLayer = true;

          
          
          
          entry->mInvalidRegion = thebesLayerInvalidRegion;
        }
        ApplyThebesLayerInvalidation(aBuilder, mergedFrame, nullptr, state,
                                     &currentOffset);
        SetHasContainerLayer(mergedFrame, currentOffset);
      }
    }

    Clip clip;
    state.ProcessDisplayItems(aChildren, clip, stateFlags);

    
    
    
    state.Finish(&flags);
    bounds = state.GetChildrenBounds();
    pixBounds = state.ScaleToOutsidePixels(bounds, false);
    appUnitsPerDevPixel = state.GetAppUnitsPerDevPixel();

    if ((flags & Layer::CONTENT_COMPONENT_ALPHA) &&
        mRetainingManager &&
        !mRetainingManager->AreComponentAlphaLayersEnabled() &&
        !stateFlags) {
      
      
      
      
      stateFlags = ContainerState::NO_COMPONENT_ALPHA;
      mNewDisplayItemData.EnumerateEntries(RestoreDisplayItemData,
                                           &mContainerLayerGeneration);
      mThebesLayerItems.EnumerateEntries(RestoreThebesLayerItemEntries,
                                         &mContainerLayerGeneration);
      aContainerFrame->AddStateBits(NS_FRAME_NO_COMPONENT_ALPHA);
      flattenedLayers = true;
      continue;
    }
    break;
  }

  NS_ASSERTION(bounds.IsEqualInterior(aChildren.GetBounds(aBuilder)), "Wrong bounds");
  containerLayer->SetVisibleRegion(pixBounds);
  
  
  if (aChildren.IsOpaque() && !aChildren.NeedsTransparentSurface()) {
    bounds.ScaleRoundIn(scaleParameters.mXScale, scaleParameters.mYScale);
    if (bounds.Contains(pixBounds.ToAppUnits(appUnitsPerDevPixel))) {
      
      flags = Layer::CONTENT_OPAQUE;
    }
  }
  containerLayer->SetContentFlags(flags);

  mContainerLayerGeneration = oldGeneration;
  return containerLayer.forget();
}

Layer*
FrameLayerBuilder::GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   nsDisplayItem* aItem)
{
  if (aManager != mRetainingManager)
    return nullptr;

  nsIFrame* f = aItem->GetUnderlyingFrame();
  NS_ASSERTION(f, "Can only call GetLeafLayerFor on items that have a frame");
  Layer* layer = GetOldLayerFor(f, aItem->GetPerFrameKey());
  if (!layer)
    return nullptr;
  if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
    
    
    
    return nullptr;
  }
  
  layer->SetClipRect(nullptr);
  layer->SetMaskLayer(nullptr);
  return layer;
}

 void
FrameLayerBuilder::InvalidateThebesLayerContents(nsIFrame* aFrame,
                                                 const nsRect& aRect)
{
  FrameProperties props = aFrame->Properties();
  RefCountedRegion* invalidThebesContent = static_cast<RefCountedRegion*>
    (props.Get(ThebesLayerInvalidRegionProperty()));
  if (!invalidThebesContent)
    return;

  nsPoint* offsetAtLastPaint = static_cast<nsPoint*>
    (props.Get(ThebesLayerLastPaintOffsetProperty()));
  NS_ASSERTION(offsetAtLastPaint,
               "This must have been set up along with ThebesLayerInvalidRegionProperty");
  invalidThebesContent->mRegion.Or(invalidThebesContent->mRegion,
          aRect + *offsetAtLastPaint);
  invalidThebesContent->mRegion.SimplifyOutward(20);
}




static bool
InternalInvalidateThebesLayersInSubtree(nsIFrame* aFrame)
{
  if (!(aFrame->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER_DESCENDANT))
    return false;

  bool foundContainerLayer = false;
  if (aFrame->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER) {
    
    
    FrameLayerBuilder::InvalidateThebesLayerContents(aFrame, aFrame->GetVisualOverflowRectRelativeToSelf());
    foundContainerLayer = true;
  }

  nsAutoTArray<nsIFrame::ChildList,4> childListArray;
  if (!aFrame->GetFirstPrincipalChild()) {
    nsSubDocumentFrame* subdocumentFrame = do_QueryFrame(aFrame);
    if (subdocumentFrame) {
      
      nsIFrame* root = subdocumentFrame->GetSubdocumentRootFrame();
      if (root) {
        childListArray.AppendElement(nsIFrame::ChildList(
          nsFrameList(root, nsLayoutUtils::GetLastSibling(root)),
          nsIFrame::kPrincipalList));
      }
    }
  }

  aFrame->GetChildLists(&childListArray);
  nsIFrame::ChildListArrayIterator lists(childListArray);
  for (; !lists.IsDone(); lists.Next()) {
    nsFrameList::Enumerator childFrames(lists.CurrentList());
    for (; !childFrames.AtEnd(); childFrames.Next()) {
      if (InternalInvalidateThebesLayersInSubtree(childFrames.get())) {
        foundContainerLayer = true;
      }
    }
  }

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
    data->mInvalidateAllLayers = true;
  }
}


Layer*
FrameLayerBuilder::GetDedicatedLayer(nsIFrame* aFrame, PRUint32 aDisplayItemKey)
{
  nsTArray<DisplayItemData>* array = GetDisplayItemDataArrayForFrame(aFrame);
  if (!array)
    return nullptr;

  for (PRUint32 i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i).mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i).mLayer;
      if (!layer->HasUserData(&gColorLayerUserData) &&
          !layer->HasUserData(&gImageLayerUserData) &&
          !layer->HasUserData(&gThebesDisplayItemLayerUserData))
        return layer;
    }
  }
  return nullptr;
}

static gfxSize
PredictScaleForContent(nsIFrame* aFrame, nsIFrame* aAncestorWithScale,
                       const gfxSize& aScale)
{
  gfx3DMatrix transform =
    gfx3DMatrix::ScalingMatrix(aScale.width, aScale.height, 1.0);
  
  transform = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestorWithScale)*transform;
  gfxMatrix transform2d;
  if (transform.CanDraw2D(&transform2d)) {
     return transform2d.ScaleFactors(true);
  }
  return gfxSize(1.0, 1.0);
}

gfxSize
FrameLayerBuilder::GetThebesLayerScaleForFrame(nsIFrame* aFrame)
{
  nsIFrame* last;
  for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    last = f;
    if (f->GetStateBits() & NS_FRAME_HAS_CONTAINER_LAYER) {
      nsTArray<DisplayItemData>* array = GetDisplayItemDataArrayForFrame(f);
	  
	  
      if (!array)
	    continue;
      for (PRUint32 i = 0; i < array->Length(); ++i) {
        Layer* layer = array->ElementAt(i).mLayer;
        ContainerLayer* container = layer->AsContainerLayer();
        if (!container) {
          continue;
        }
        for (Layer* l = container->GetFirstChild(); l; l = l->GetNextSibling()) {
          ThebesDisplayItemLayerUserData* data =
              static_cast<ThebesDisplayItemLayerUserData*>
                (l->GetUserData(&gThebesDisplayItemLayerUserData));
          if (data) {
            return PredictScaleForContent(aFrame, f, gfxSize(data->mXScale, data->mYScale));
          }
        }
      }
    }
  }

  return PredictScaleForContent(aFrame, last,
      last->PresContext()->PresShell()->GetResolution());
}

#ifdef MOZ_DUMP_PAINTING
static void DebugPaintItem(nsRenderingContext* aDest, nsDisplayItem *aItem, nsDisplayListBuilder* aBuilder)
{
  bool snap;
  nsRect appUnitBounds = aItem->GetBounds(aBuilder, &snap);
  gfxRect bounds(appUnitBounds.x, appUnitBounds.y, appUnitBounds.width, appUnitBounds.height);
  bounds.ScaleInverse(aDest->AppUnitsPerDevPixel());

  nsRefPtr<gfxASurface> surf =
    gfxPlatform::GetPlatform()->CreateOffscreenSurface(gfxIntSize(bounds.width, bounds.height),
                                                       gfxASurface::CONTENT_COLOR_ALPHA);
  surf->SetDeviceOffset(-bounds.TopLeft());
  nsRefPtr<gfxContext> context = new gfxContext(surf);
  nsRefPtr<nsRenderingContext> ctx = new nsRenderingContext();
  ctx->Init(aDest->DeviceContext(), context);

  aItem->Paint(aBuilder, ctx);
  DumpPaintedImage(aItem, surf);
  aItem->SetPainted();

  surf->SetDeviceOffset(gfxPoint(0, 0));
  aDest->ThebesContext()->SetSource(surf, bounds.TopLeft());
  aDest->ThebesContext()->Rectangle(bounds);
  aDest->ThebesContext()->Fill();
}
#endif





























 void
FrameLayerBuilder::DrawThebesLayer(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw,
                                   const nsIntRegion& aRegionToInvalidate,
                                   void* aCallbackData)
{
  SAMPLE_LABEL("gfx", "DrawThebesLayer");

  nsDisplayListBuilder* builder = static_cast<nsDisplayListBuilder*>
    (aCallbackData);

  FrameLayerBuilder *layerBuilder = GetLayerBuilderForManager(aLayer->Manager());

  if (layerBuilder->CheckDOMModified())
    return;

  nsTArray<ClippedDisplayItem> items;
  PRUint32 commonClipCount;
  nsIFrame* containerLayerFrame;
  {
    ThebesLayerItemsEntry* entry = layerBuilder->mThebesLayerItems.GetEntry(aLayer);
    NS_ASSERTION(entry, "We shouldn't be drawing into a layer with no items!");
    items.SwapElements(entry->mItems);
    commonClipCount = entry->mCommonClipCount;
    containerLayerFrame = entry->mContainerLayerFrame;
    
    
    
  }

  if (!containerLayerFrame) {
    return;
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

  
  
  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  nsIntPoint offset = GetTranslationForThebesLayer(aLayer);
  
  
  
  aContext->Translate(aLayer->GetResidualTranslation() - gfxPoint(offset.x, offset.y));
  aContext->Scale(userData->mXScale, userData->mYScale);

  nsPresContext* presContext = containerLayerFrame->PresContext();
  PRInt32 appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
  if (!aRegionToInvalidate.IsEmpty()) {
    nsRect r = (aRegionToInvalidate.GetBounds() + offset).
      ToAppUnits(appUnitsPerDevPixel);
    r.ScaleInverseRoundOut(userData->mXScale, userData->mYScale);
    containerLayerFrame->InvalidateWithFlags(r,
        nsIFrame::INVALIDATE_NO_THEBES_LAYERS |
        nsIFrame::INVALIDATE_EXCLUDE_CURRENT_PAINT);
  }

  PRUint32 i;
  
  
  
  
  
  nsRegion visible = aRegionToDraw.ToAppUnits(appUnitsPerDevPixel);
  visible.MoveBy(NSIntPixelsToAppUnits(offset.x, appUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(offset.y, appUnitsPerDevPixel));
  visible.ScaleInverseRoundOut(userData->mXScale, userData->mYScale);

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
  bool setClipRect = false;

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
        NS_ASSERTION(commonClipCount < 100,
          "Maybe you really do have more than a hundred clipping rounded rects, or maybe something has gone wrong.");
        currentClip.ApplyTo(aContext, presContext, commonClipCount);
      }
    }

    if (cdi->mInactiveLayer) {
      PaintInactiveLayer(builder, cdi->mItem, aContext, rc, layerBuilder);
    } else {
      nsIFrame* frame = cdi->mItem->GetUnderlyingFrame();
      if (frame) {
        frame->AddStateBits(NS_FRAME_PAINTED_THEBES);
      }
#ifdef MOZ_DUMP_PAINTING

      if (gfxUtils::sDumpPainting) {
        DebugPaintItem(rc, cdi->mItem, builder);
      } else {
#else
      {
#endif
        cdi->mItem->Paint(builder, rc);
      }
    }

    if (layerBuilder->CheckDOMModified())
      break;
  }

  {
    ThebesLayerItemsEntry* entry =
      layerBuilder->mThebesLayerItems.GetEntry(aLayer);
    items.SwapElements(entry->mItems);
  }

  if (setClipRect) {
    aContext->Restore();
  }

  FlashPaint(aContext);
}

bool
FrameLayerBuilder::CheckDOMModified()
{
  if (!mRootPresContext ||
      mInitialDOMGeneration == mRootPresContext->GetDOMGeneration())
    return false;
  if (mDetectedDOMModification) {
    
    return true;
  }
  mDetectedDOMModification = true;
  
  
  
  NS_WARNING("Detected DOM modification during paint, bailing out!");
  return true;
}

#ifdef MOZ_DUMP_PAINTING
 void
FrameLayerBuilder::DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile)
{
  aManager->Dump(aFile);
}
#endif

FrameLayerBuilder::Clip::Clip(const Clip& aOther, nsDisplayItem* aClipItem)
  : mRoundedClipRects(aOther.mRoundedClipRects),
    mHaveClipRect(true)
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
                                 nsPresContext* aPresContext,
                                 PRUint32 aBegin, PRUint32 aEnd)
{
  PRInt32 A2D = aPresContext->AppUnitsPerDevPixel();
  ApplyRectTo(aContext, A2D);
  ApplyRoundedRectsTo(aContext, A2D, aBegin, aEnd);
}

void
FrameLayerBuilder::Clip::ApplyRectTo(gfxContext* aContext, PRInt32 A2D) const
{
  aContext->NewPath();
  gfxRect clip = nsLayoutUtils::RectToGfxRect(mClipRect, A2D);
  aContext->Rectangle(clip, true);
  aContext->Clip();
}

void
FrameLayerBuilder::Clip::ApplyRoundedRectsTo(gfxContext* aContext,
                                             PRInt32 A2D,
                                             PRUint32 aBegin, PRUint32 aEnd) const
{
  aEnd = NS_MIN<PRUint32>(aEnd, mRoundedClipRects.Length());

  for (PRUint32 i = aBegin; i < aEnd; ++i) {
    AddRoundedRectPathTo(aContext, A2D, mRoundedClipRects[i]);
    aContext->Clip();
  }
}

void
FrameLayerBuilder::Clip::DrawRoundedRectsTo(gfxContext* aContext,
                                            PRInt32 A2D,
                                            PRUint32 aBegin, PRUint32 aEnd) const
{
  aEnd = NS_MIN<PRUint32>(aEnd, mRoundedClipRects.Length());

  if (aEnd - aBegin == 0)
    return;

  
  
  ApplyRoundedRectsTo(aContext, A2D, aBegin, aEnd - 1);
  AddRoundedRectPathTo(aContext, A2D, mRoundedClipRects[aEnd - 1]);
  aContext->Fill();
}

void
FrameLayerBuilder::Clip::AddRoundedRectPathTo(gfxContext* aContext,
                                              PRInt32 A2D,
                                              const RoundedRect &aRoundRect) const
{
  gfxCornerSizes pixelRadii;
  nsCSSRendering::ComputePixelRadii(aRoundRect.mRadii, A2D, &pixelRadii);

  gfxRect clip = nsLayoutUtils::RectToGfxRect(aRoundRect.mRect, A2D);
  clip.Round();
  clip.Condition();

  aContext->NewPath();
  aContext->RoundedRectangle(clip, pixelRadii);
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

gfxRect
CalculateBounds(nsTArray<FrameLayerBuilder::Clip::RoundedRect> aRects, PRInt32 A2D)
{
  nsRect bounds = aRects[0].mRect;
  for (PRUint32 i = 1; i < aRects.Length(); ++i) {
    bounds.UnionRect(bounds, aRects[i].mRect);
   }
 
  return nsLayoutUtils::RectToGfxRect(bounds, A2D);
}
 
void
ContainerState::SetupMaskLayer(Layer *aLayer, const FrameLayerBuilder::Clip& aClip,
                               PRUint32 aRoundedRectClipCount)
{
  
  nsIntRect layerBounds = aLayer->GetVisibleRegion().GetBounds();
  if (aClip.mRoundedClipRects.IsEmpty() ||
      aRoundedRectClipCount <= 0 ||
      layerBounds.IsEmpty()) {
    return;
  }

  
  nsRefPtr<ImageLayer> maskLayer =  CreateOrRecycleMaskImageLayerFor(aLayer);
  MaskLayerUserData* userData = GetMaskLayerUserData(maskLayer);

  MaskLayerUserData newData;
  newData.mRoundedClipRects.AppendElements(aClip.mRoundedClipRects);
  if (aRoundedRectClipCount < newData.mRoundedClipRects.Length()) {
    newData.mRoundedClipRects.TruncateLength(aRoundedRectClipCount);
  }
  newData.mScaleX = mParameters.mXScale;
  newData.mScaleY = mParameters.mYScale;

  if (*userData == newData) {
    aLayer->SetMaskLayer(maskLayer);
    return;
  }

  
  const PRInt32 A2D = mContainerFrame->PresContext()->AppUnitsPerDevPixel();
  gfxRect boundingRect = CalculateBounds(newData.mRoundedClipRects, A2D);
  boundingRect.Scale(mParameters.mXScale, mParameters.mYScale);

  PRUint32 maxSize = mManager->GetMaxTextureSize();
  NS_ASSERTION(maxSize > 0, "Invalid max texture size");
  nsIntSize surfaceSize(NS_MIN<PRInt32>(boundingRect.Width(), maxSize),
                        NS_MIN<PRInt32>(boundingRect.Height(), maxSize));

  
  
  
  
  gfxMatrix maskTransform;
  maskTransform.Scale(float(surfaceSize.width)/float(boundingRect.Width()),
                      float(surfaceSize.height)/float(boundingRect.Height()));
  maskTransform.Translate(-boundingRect.TopLeft());
  
  gfxMatrix imageTransform = maskTransform;
  imageTransform.Scale(mParameters.mXScale, mParameters.mYScale);

  
  nsTArray<MaskLayerImageCache::PixelRoundedRect> roundedRects;
  for (PRUint32 i = 0; i < newData.mRoundedClipRects.Length(); ++i) {
    roundedRects.AppendElement(
      MaskLayerImageCache::PixelRoundedRect(newData.mRoundedClipRects[i],
                                            mContainerFrame->PresContext()));
    roundedRects[i].ScaleAndTranslate(imageTransform);
  }
 
  
  const MaskLayerImageCache::MaskLayerImageKey* key =
    new MaskLayerImageCache::MaskLayerImageKey(roundedRects);
  const MaskLayerImageCache::MaskLayerImageKey* lookupKey = key;

  nsRefPtr<ImageContainer> container =
    GetMaskLayerImageCache()->FindImageFor(&lookupKey);

  if (container) {
    
    delete key;
    key = lookupKey;
  } else {
    
    nsRefPtr<gfxASurface> surface =
      aLayer->Manager()->CreateOptimalMaskSurface(surfaceSize);

    
    if (!surface || surface->CairoStatus()) {
      NS_WARNING("Could not create surface for mask layer.");
      return;
    }

    nsRefPtr<gfxContext> context = new gfxContext(surface);
    context->Multiply(imageTransform);

    
    context->SetColor(gfxRGBA(1, 1, 1, 1));
    aClip.DrawRoundedRectsTo(context, A2D, 0, aRoundedRectClipCount);

    
    container = aLayer->Manager()->CreateImageContainer();
    NS_ASSERTION(container, "Could not create image container for mask layer.");
    static const Image::Format format = Image::CAIRO_SURFACE;
    nsRefPtr<Image> image = container->CreateImage(&format, 1);
    NS_ASSERTION(image, "Could not create image container for mask layer.");
    CairoImage::Data data;
    data.mSurface = surface;
    data.mSize = surfaceSize;
    static_cast<CairoImage*>(image.get())->SetData(data);
    container->SetCurrentImageInTransaction(image);

    GetMaskLayerImageCache()->PutImage(key, container);
  }

  maskLayer->SetContainer(container);
  maskLayer->SetBaseTransform(gfx3DMatrix::From2D(maskTransform.Invert()));

  
  userData->mScaleX = newData.mScaleX;
  userData->mScaleY = newData.mScaleY;
  userData->mRoundedClipRects.SwapElements(newData.mRoundedClipRects);
  userData->mImageKey = key;

  aLayer->SetMaskLayer(maskLayer);
  return;
}

} 
