




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
#include "nsRenderingContext.h"
#include "MaskLayerImageCache.h"
#include "nsIScrollableFrame.h"
#include "nsPrintfCString.h"
#include "LayerTreeInvalidation.h"
#include "nsSVGIntegrationUtils.h"

#include "mozilla/Preferences.h"
#include "sampler.h"

#include "nsAnimationManager.h"
#include "nsTransitionManager.h"

#ifdef DEBUG
#include <stdio.h>

#endif

using namespace mozilla::layers;

namespace mozilla {

FrameLayerBuilder::DisplayItemData::DisplayItemData(Layer* aLayer, uint32_t aKey, LayerState aLayerState, uint32_t aGeneration)
  : mLayer(aLayer)
  , mDisplayItemKey(aKey)
  , mContainerLayerGeneration(aGeneration)
  , mLayerState(aLayerState)
  , mUsed(false)
{}

FrameLayerBuilder::DisplayItemData::DisplayItemData(DisplayItemData &toCopy)
{
  
  
  mLayer = toCopy.mLayer;
  mInactiveManager = toCopy.mInactiveManager;
  mFrameList = toCopy.mFrameList;
  mGeometry = toCopy.mGeometry;
  mDisplayItemKey = toCopy.mDisplayItemKey;
  mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
  mLayerState = toCopy.mLayerState;
  mUsed = toCopy.mUsed;
}

FrameLayerBuilder::DisplayItemData::~DisplayItemData()
{}

bool
FrameLayerBuilder::DisplayItemData::FrameListMatches(nsDisplayItem* aOther)
{
  nsAutoTArray<nsIFrame*, 4> copy = mFrameList;
  if (!copy.RemoveElement(aOther->GetUnderlyingFrame())) {
    return false;
  }
  
  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aOther->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    if (!copy.RemoveElement(mergedFrames[i])) {
      return false;
    }
  }

  return copy.IsEmpty();
}




class LayerManagerData : public LayerUserData {
public:
  LayerManagerData() :
    mInvalidateAllLayers(false)
  {
    MOZ_COUNT_CTOR(LayerManagerData);
    mFramesWithLayers.Init();
  }
  ~LayerManagerData() {
    MOZ_COUNT_DTOR(LayerManagerData);
    
    
    mFramesWithLayers.EnumerateEntries(
        FrameLayerBuilder::RemoveDisplayItemDataForFrame, this);
  }

  


  nsTHashtable<FrameLayerBuilder::DisplayItemDataEntry> mFramesWithLayers;
  bool mInvalidateAllLayers;
};

 void
FrameLayerBuilder::DestroyDisplayItemDataFor(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  props.Delete(LayerManagerDataProperty());
  props.Delete(LayerManagerSecondaryDataProperty());
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





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 FrameLayerBuilder* aLayerBuilder,
                 nsIFrame* aContainerFrame,
                 nsDisplayItem* aContainerItem,
                 ContainerLayer* aContainerLayer,
                 const FrameLayerBuilder::ContainerParameters& aParameters) :
    mBuilder(aBuilder), mManager(aManager),
    mLayerBuilder(aLayerBuilder),
    mContainerFrame(aContainerFrame),
    mContainerLayer(aContainerLayer),
    mParameters(aParameters),
    mNextFreeRecycledThebesLayer(0)
  {
    nsPresContext* presContext = aContainerFrame->PresContext();
    mAppUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    mContainerReferenceFrame = aContainerItem ? aContainerItem->ReferenceFrame() :
      mBuilder->FindReferenceFrameFor(mContainerFrame);
    
    
    
    mSnappingEnabled = aManager->IsSnappingEffectiveTransforms() &&
      !mParameters.AllowResidualTranslation();
    mRecycledMaskImageLayers.Init();
    CollectOldLayers();
  }

  enum ProcessDisplayItemsFlags {
    NO_COMPONENT_ALPHA = 0x01,
  };

  





  void ProcessDisplayItems(const nsDisplayList& aList,
                           FrameLayerBuilder::Clip& aClip,
                           uint32_t aFlags);
  







  void Finish(uint32_t *aTextContentFlags, LayerManagerData* aData);

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
    const nsIFrame* GetActiveScrolledRoot() { return mActiveScrolledRoot; }

    




    already_AddRefed<ImageContainer> CanOptimizeImageLayer();

    




    nsIntRegion  mVisibleRegion;
    






    nsIntRegion  mVisibleAboveRegion;
    





    nsIntRegion  mDrawRegion;
    







    nsIntRegion  mDrawAboveRegion;
    



    nsIntRegion  mOpaqueRegion;
    




    const nsIFrame* mActiveScrolledRoot;
    ThebesLayer* mLayer;
    



    nscolor      mSolidColor;
    


    bool mIsSolidColorInVisibleRegion;
    



    bool mNeedComponentAlpha;
    





    bool mForceTransparentSurface;

    



    nsDisplayImageContainer* mImage;
    







    FrameLayerBuilder::Clip mItemClip;
    





    int32_t mCommonClipCount;
    





    void UpdateCommonClipCount(const FrameLayerBuilder::Clip& aCurrentClip);
  };
  friend class ThebesLayerData;

  





  already_AddRefed<ThebesLayer> CreateOrRecycleThebesLayer(const nsIFrame* aActiveScrolledRoot, 
                                                           const nsIFrame *aReferenceFrame, 
                                                           const nsPoint& aTopLeft);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer(ThebesLayer* aThebes);
  



  already_AddRefed<ImageLayer> CreateOrRecycleImageLayer(ThebesLayer* aThebes);
  




  already_AddRefed<ImageLayer> CreateOrRecycleMaskImageLayerFor(Layer* aLayer);
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem, 
                                Layer* aNewLayer,
                                const FrameLayerBuilder::Clip& aClip,
                                const nsPoint& aTopLeft);
  





  nscolor FindOpaqueBackgroundColorFor(int32_t aThebesLayerIndex);
  




  void PopThebesLayerData();
  
















  ThebesLayerData* FindThebesLayerFor(nsDisplayItem* aItem,
                                                   const nsIntRect& aVisibleRect,
                                                   const nsIntRect& aDrawRect,
                                                   const FrameLayerBuilder::Clip& aClip,
                                                   const nsIFrame* aActiveScrolledRoot,
                                                   const nsPoint& aTopLeft);
  ThebesLayerData* GetTopThebesLayerData()
  {
    return mThebesLayerDataStack.IsEmpty() ? nullptr
        : mThebesLayerDataStack[mThebesLayerDataStack.Length() - 1].get();
  }

  








  void SetupMaskLayer(Layer *aLayer, const FrameLayerBuilder::Clip& aClip,
                      uint32_t aRoundedRectClipCount = PR_UINT32_MAX);

  nsDisplayListBuilder*            mBuilder;
  LayerManager*                    mManager;
  FrameLayerBuilder*               mLayerBuilder;
  nsIFrame*                        mContainerFrame;
  const nsIFrame*                  mContainerReferenceFrame;
  ContainerLayer*                  mContainerLayer;
  FrameLayerBuilder::ContainerParameters mParameters;
  



  nsIntRegion                      mInvalidThebesContent;
  nsRect                           mBounds;
  nsAutoTArray<nsAutoPtr<ThebesLayerData>,1>  mThebesLayerDataStack;
  




  typedef nsAutoTArray<nsRefPtr<Layer>,1> AutoLayersArray;
  AutoLayersArray                  mNewChildLayers;
  nsTArray<nsRefPtr<ThebesLayer> > mRecycledThebesLayers;
  nsDataHashtable<nsPtrHashKey<Layer>, nsRefPtr<ImageLayer> >
    mRecycledMaskImageLayers;
  uint32_t                         mNextFreeRecycledThebesLayer;
  nscoord                          mAppUnitsPerDevPixel;
  bool                             mSnappingEnabled;
};

class ThebesDisplayItemLayerUserData : public LayerUserData
{
public:
  ThebesDisplayItemLayerUserData() :
    mMaskClipCount(0),
    mForcedBackgroundColor(NS_RGBA(0,0,0,0)),
    mXScale(1.f), mYScale(1.f),
    mAppUnitsPerDevPixel(0),
    mTranslation(0, 0),
    mActiveScrolledRootPosition(0, 0) {}

  




  uint32_t mMaskClipCount;

  



  nscolor mForcedBackgroundColor;

  


  float mXScale, mYScale;

  


  nscoord mAppUnitsPerDevPixel;

  





  nsIntPoint mTranslation;

  









  gfxPoint mActiveScrolledRootPosition;

  nsIntRegion mRegionToInvalidate;

  
  
  
  nsPoint mLastActiveScrolledRootOrigin;
  nsPoint mActiveScrolledRootOrigin;

  nsRefPtr<ColorLayer> mColorLayer;
  nsRefPtr<ImageLayer> mImageLayer;
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




struct LayerOwnerUserData : public LayerUserData
{
  LayerOwnerUserData(nsIFrame* aOwnerFrame) : mOwnerFrame(aOwnerFrame) {}

  nsIFrame* mOwnerFrame;
};










uint8_t gThebesDisplayItemLayerUserData;





uint8_t gColorLayerUserData;





uint8_t gImageLayerUserData;





uint8_t gLayerManagerUserData;





uint8_t gMaskLayerUserData;





uint8_t gLayerOwnerUserData;





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

uint8_t gLayerManagerSecondary;

bool FrameLayerBuilder::sWidgetManagerSecondary = nullptr;

 const FramePropertyDescriptor* 
FrameLayerBuilder::GetDescriptorForManager(LayerManager* aManager)
{
  bool secondary = sWidgetManagerSecondary;
  if (aManager) {
    secondary = !!static_cast<LayerManagerSecondary*>(aManager->GetUserData(&gLayerManagerSecondary));
  }

  return secondary ? LayerManagerSecondaryDataProperty() : LayerManagerDataProperty();
}

LayerManagerData*
FrameLayerBuilder::GetManagerData(nsIFrame* aFrame, LayerManager* aManager)
{
  FrameProperties props = aFrame->Properties();
  return static_cast<LayerManagerData*>(props.Get(GetDescriptorForManager(aManager)));
}

void
FrameLayerBuilder::SetManagerData(nsIFrame* aFrame, LayerManagerData* aData)
{
  FrameProperties props = aFrame->Properties();
  const FramePropertyDescriptor* desc = GetDescriptorForManager(nullptr);

  props.Remove(desc);
  if (aData) {
    props.Set(desc, aData);
  }
}

void
FrameLayerBuilder::ClearManagerData(nsIFrame* aFrame)
{
  SetManagerData(aFrame, nullptr);
}

void
FrameLayerBuilder::ClearManagerData(nsIFrame* aFrame, LayerManagerData* aData)
{
  NS_ABORT_IF_FALSE(aData, "Must have a widget manager to check for manager data!");

  FrameProperties props = aFrame->Properties();
  if (aData == static_cast<LayerManagerData*>(props.Get(LayerManagerDataProperty()))) {
    props.Remove(LayerManagerDataProperty());
    return;
  }
  if (aData == static_cast<LayerManagerData*>(props.Get(LayerManagerSecondaryDataProperty()))) {
    props.Remove(LayerManagerSecondaryDataProperty());
    return;
  }
}
 void
FrameLayerBuilder::Shutdown()
{
  if (gMaskLayerImageCache) {
    delete gMaskLayerImageCache;
    gMaskLayerImageCache = nullptr;
  }
}

void
FrameLayerBuilder::Init(nsDisplayListBuilder* aBuilder, LayerManager* aManager)
{
  mDisplayListBuilder = aBuilder;
  mRootPresContext = aBuilder->RootReferenceFrame()->PresContext()->GetRootPresContext();
  if (mRootPresContext) {
    mInitialDOMGeneration = mRootPresContext->GetDOMGeneration();
  }
  aManager->SetUserData(&gLayerManagerLayerBuilder, this);
}

bool
FrameLayerBuilder::DisplayItemDataEntry::HasNonEmptyContainerLayer()
{
  for (uint32_t i = 0; i < mData.Length(); ++i) {
    if (mData[i]->mLayer->GetType() == Layer::TYPE_CONTAINER &&
        mData[i]->mLayerState != LAYER_ACTIVE_EMPTY)
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

nsTArray<nsRefPtr<FrameLayerBuilder::DisplayItemData> >*
FrameLayerBuilder::GetDisplayItemDataArrayForFrame(nsIFrame* aFrame)
{
  LayerManagerData* data = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));
  if (!data) {
    return nullptr;
  }

  DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(aFrame);
  if (!entry)
    return nullptr;

  return &entry->mData;
}

nsACString&
AppendToString(nsACString& s, const nsIntRect& r,
               const char* pfx="", const char* sfx="")
{
  s += pfx;
  s += nsPrintfCString(
    "(x=%d, y=%d, w=%d, h=%d)",
    r.x, r.y, r.width, r.height);
  return s += sfx;
}

nsACString&
AppendToString(nsACString& s, const nsIntRegion& r,
               const char* pfx="", const char* sfx="")
{
  s += pfx;

  nsIntRegionRectIterator it(r);
  s += "< ";
  while (const nsIntRect* sr = it.Next()) {
    AppendToString(s, *sr) += "; ";
  }
  s += ">";

  return s += sfx;
}






static void
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsIntRegion& aRegion,
                              const nsIntPoint& aTranslation)
{
  
  
  
  nsIntRegion rgn = aRegion;
  rgn.MoveBy(-aTranslation);
  aLayer->InvalidateRegion(rgn);
#ifdef DEBUG_INVALIDATIONS
  nsAutoCString str;
  AppendToString(str, rgn);
  printf("Invalidating layer %p: %s\n", aLayer, str.get());
#endif
}

static nsIntPoint
GetTranslationForThebesLayer(ThebesLayer* aLayer)
{
  ThebesDisplayItemLayerUserData* data = 
    static_cast<ThebesDisplayItemLayerUserData*>
      (aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
  NS_ASSERTION(data, "Must be a tracked thebes layer!");

  return data->mTranslation;
}
















static nsIFrame* sDestroyedFrame = NULL;

 void
FrameLayerBuilder::RemoveFrameFromLayerManager(nsIFrame* aFrame,
                                               void* aPropertyValue)
{
  LayerManagerData *data = reinterpret_cast<LayerManagerData*>(aPropertyValue);

  DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(aFrame);
  for (uint32_t i = 0; i < entry->mData.Length(); ++i) {
    ThebesLayer* t = entry->mData[i]->mLayer->AsThebesLayer();
    if (t) {
      ThebesDisplayItemLayerUserData* data =
          static_cast<ThebesDisplayItemLayerUserData*>(t->GetUserData(&gThebesDisplayItemLayerUserData));
      if (data) {
        nsRegion old = entry->mData[i]->mGeometry->ComputeInvalidationRegion();
        nsIntRegion rgn = old.ScaleToOutsidePixels(data->mXScale, data->mYScale, data->mAppUnitsPerDevPixel);
        rgn.MoveBy(-GetTranslationForThebesLayer(t));
        data->mRegionToInvalidate.Or(data->mRegionToInvalidate, rgn);
      }
    }
  }

  sDestroyedFrame = aFrame;
  data->mFramesWithLayers.RemoveEntry(aFrame);
  sDestroyedFrame = NULL;
}

void
FrameLayerBuilder::DidBeginRetainedLayerTransaction(LayerManager* aManager)
{
  mRetainingManager = aManager;
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));
  if (data) {
    mInvalidateAllLayers = data->mInvalidateAllLayers;
  } else {
    data = new LayerManagerData();
    aManager->SetUserData(&gLayerManagerUserData, data);
  }
}

void
FrameLayerBuilder::StoreOptimizedLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey, Layer* aImage)
{
  DisplayItemDataEntry *entry = mNewDisplayItemData.GetEntry(aFrame);
  if (!entry)
    return;

  nsTArray<nsRefPtr<DisplayItemData> > *array = &entry->mData;
  if (!array)
    return;

  for (uint32_t i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i)->mDisplayItemKey == aDisplayItemKey) {
      array->ElementAt(i)->mOptLayer = aImage;
      return;
    }
  }
}

void
FrameLayerBuilder::DidEndTransaction()
{
  GetMaskLayerImageCache()->Sweep();
}

void
FrameLayerBuilder::WillEndTransaction()
{
  if (!mRetainingManager) {
    return;
  }

  
  LayerManagerData* data = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));
  NS_ASSERTION(data, "Must have data!");
  
  data->mFramesWithLayers.EnumerateEntries(UpdateDisplayItemDataForFrame, this);
  
  
  
  
  mNewDisplayItemData.EnumerateEntries(StoreNewDisplayItemData, data);
  data->mInvalidateAllLayers = false;
}

struct ProcessRemovedDisplayItemsData
{
  ProcessRemovedDisplayItemsData(Layer* aLayer, FrameLayerBuilder* aLayerBuilder)
    : mLayer(aLayer)
    , mLayerBuilder(aLayerBuilder)
  {}

  Layer *mLayer;
  FrameLayerBuilder *mLayerBuilder;
};

 PLDHashOperator
FrameLayerBuilder::ProcessRemovedDisplayItems(DisplayItemDataEntry* aEntry,
                                              void* aUserArg)
{
  ProcessRemovedDisplayItemsData *userData =
    static_cast<ProcessRemovedDisplayItemsData*>(aUserArg);
  Layer* layer = userData->mLayer;
  FrameLayerBuilder* layerBuilder = userData->mLayerBuilder;
  for (uint32_t i = 0; i < aEntry->mData.Length(); ++i) {
    DisplayItemData* item = aEntry->mData[i];
    ThebesLayer* t = item->mLayer->AsThebesLayer();
    if (!item->mUsed && t && item->mLayer == layer) {
#ifdef DEBUG_INVALIDATIONS
      printf("Invalidating unused display item (%i) belonging to frame %p from layer %p\n", item->mDisplayItemKey, aEntry->GetKey(), t);
#endif
      ThebesDisplayItemLayerUserData* data =
          static_cast<ThebesDisplayItemLayerUserData*>(t->GetUserData(&gThebesDisplayItemLayerUserData));
      InvalidatePostTransformRegion(t,
          item->mGeometry->ComputeInvalidationRegion().
            ScaleToOutsidePixels(data->mXScale, data->mYScale, data->mAppUnitsPerDevPixel),
          layerBuilder->GetLastPaintOffset(t));
    }
  }
  return PL_DHASH_NEXT;
}

 PLDHashOperator
FrameLayerBuilder::UpdateDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                 void* aUserArg)
{
  FrameLayerBuilder* builder = static_cast<FrameLayerBuilder*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();
  DisplayItemDataEntry* newDisplayItems =
    builder ? builder->mNewDisplayItemData.GetEntry(f) : nullptr;

  LayerManagerData* managerData = static_cast<LayerManagerData*>
    (builder->GetRetainingLayerManager()->GetUserData(&gLayerManagerUserData));
  LayerManagerData* data = GetManagerData(f);
  if (!newDisplayItems || newDisplayItems->mData.IsEmpty()) {
    
    if (newDisplayItems) {
      builder->mNewDisplayItemData.RawRemoveEntry(newDisplayItems);
    }
    if (data == managerData) {
      ClearManagerData(f);
    }
    return PL_DHASH_REMOVE;
  }

  SetManagerData(f, managerData);

  
  aEntry->mData.SwapElements(newDisplayItems->mData);
  
  builder->mNewDisplayItemData.RawRemoveEntry(newDisplayItems);
  return PL_DHASH_NEXT;
}
  
 PLDHashOperator 
FrameLayerBuilder::RemoveDisplayItemDataForFrame(DisplayItemDataEntry* aEntry,
                                                 void* aClosure)
{
  LayerManagerData* managerData = static_cast<LayerManagerData*>(aClosure);
  nsIFrame* f = aEntry->GetKey();
  
  
  if (f != sDestroyedFrame) {
    ClearManagerData(f, managerData);
  }
  return PL_DHASH_REMOVE;
}

 PLDHashOperator
FrameLayerBuilder::StoreNewDisplayItemData(DisplayItemDataEntry* aEntry,
                                           void* aUserArg)
{
  LayerManagerData* data = static_cast<LayerManagerData*>(aUserArg);
  nsIFrame* f = aEntry->GetKey();

  
  NS_ASSERTION(!data->mFramesWithLayers.GetEntry(f),
               "We shouldn't get here if we're already in mFramesWithLayers");
  DisplayItemDataEntry *newEntry = data->mFramesWithLayers.PutEntry(f);

  newEntry->mData.SwapElements(aEntry->mData);
  
  
  
  
  
  SetManagerData(f, data);
  return PL_DHASH_REMOVE;
}





static LayerManagerData*
GetDefaultLayerManagerDataForFrame(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  return static_cast<LayerManagerData*>(props.Get(FrameLayerBuilder::LayerManagerDataProperty()));
}

static LayerManagerData*
GetSecondaryLayerManagerDataForFrame(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  return static_cast<LayerManagerData*>(props.Get(FrameLayerBuilder::LayerManagerSecondaryDataProperty()));
}

 FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemDataForManager(nsIFrame* aFrame,
                                                uint32_t aDisplayItemKey,
                                                LayerManagerData* aData)
{
  DisplayItemDataEntry *entry = aData->mFramesWithLayers.GetEntry(aFrame);
  if (!entry) {
    return nullptr;
  }

  for (uint32_t i = 0; i < entry->mData.Length(); ++i) {
    if (entry->mData[i]->mDisplayItemKey == aDisplayItemKey) {
      return entry->mData[i];
    }
  }
  
  return nullptr;
}

 FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                uint32_t aDisplayItemKey, 
                                                LayerManager* aManager)
{
  LayerManagerData *data = 
    static_cast<LayerManagerData*>(aManager->GetUserData(&gLayerManagerUserData));
  
  if (!data) {
    return nullptr;
  }

  return GetDisplayItemDataForManager(aFrame, aDisplayItemKey, data);
}

 FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemDataForManager(nsDisplayItem* aItem, 
                                                LayerManager* aManager)
{
  DisplayItemData* data = 
    GetDisplayItemDataForManager(aItem->GetUnderlyingFrame(), 
                                 aItem->GetPerFrameKey(), 
                                 aManager);
  if (data) {
    return data->FrameListMatches(aItem) ? data : nullptr;
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    data = 
      GetDisplayItemDataForManager(mergedFrames[i], 
                                   aItem->GetPerFrameKey(), 
                                   aManager);
    if (data) {
      return data->FrameListMatches(aItem) ? data : nullptr;
    }
  }
  return nullptr;
}

 FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemDataForManager(nsIFrame* aFrame, 
                                                uint32_t aDisplayItemKey)
{
  LayerManagerData *data = GetDefaultLayerManagerDataForFrame(aFrame);
  
  if (!data) {
    return nullptr;
  }
  
  return GetDisplayItemDataForManager(aFrame, aDisplayItemKey, data);
}

bool
FrameLayerBuilder::HasRetainedLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey, LayerManager* aManager)
{
  DisplayItemData* data = GetDisplayItemDataForManager(aFrame, aDisplayItemKey, aManager);
  if (data) {
    Layer* layer = data->mLayer;
    if (layer->Manager()->GetUserData(&gLayerManagerUserData)) {
      
      return true;
    }
  }
  return false;
}

bool
FrameLayerBuilder::HasRetainedDataFor(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  LayerManagerData* data = GetDefaultLayerManagerDataForFrame(aFrame);
  if (data && GetDisplayItemDataForManager(aFrame, aDisplayItemKey, data)) {
    return true;
  }
  data = GetSecondaryLayerManagerDataForFrame(aFrame);
  if (data && GetDisplayItemDataForManager(aFrame, aDisplayItemKey, data)) {
    return true;
  }
  return false;
}

FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetOldLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  
  
  if (!mRetainingManager || mInvalidateAllLayers)
    return nullptr;

  nsTArray<nsRefPtr<DisplayItemData> > *array = GetDisplayItemDataArrayForFrame(aFrame);
  if (!array)
    return nullptr;

  for (uint32_t i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i)->mDisplayItemKey == aDisplayItemKey) {
      Layer* layer = array->ElementAt(i)->mLayer;
      if (layer->Manager() == mRetainingManager) {
        return array->ElementAt(i);
      }
    }
  }
  return nullptr;
}

Layer*
FrameLayerBuilder::GetOldLayerFor(nsDisplayItem* aItem, nsDisplayItemGeometry** aOldGeometry, Clip** aOldClip)
{
  uint32_t key = aItem->GetPerFrameKey();
  nsIFrame* frame = aItem->GetUnderlyingFrame();

  if (frame) {
    DisplayItemData* oldData = GetOldLayerForFrame(frame, key);
    if (oldData) {
      if (aOldGeometry) {
        *aOldGeometry = oldData->mGeometry.get();
      }
      if (aOldClip) {
        *aOldClip = &oldData->mClip;
      }
      return oldData->mLayer;
    }
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    DisplayItemData* oldData = GetOldLayerForFrame(mergedFrames[i], key);
    if (oldData) {
      if (aOldGeometry) {
        *aOldGeometry = oldData->mGeometry.get();
      }
      if (aOldClip) {
        *aOldClip = &oldData->mClip;
      }
      return oldData->mLayer;
    }
  }

  return nullptr;
} 

 Layer*
FrameLayerBuilder::GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  LayerManagerData* data = GetManagerData(aFrame);
  if (!data) {
    return nullptr;
  }
  DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(aFrame);
  if (!entry)
    return nullptr;

  nsTArray<nsRefPtr<DisplayItemData> > *array = &entry->mData;
  if (!array)
    return nullptr;

  for (uint32_t i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i)->mDisplayItemKey == aDisplayItemKey) {
      return array->ElementAt(i)->mLayer;
    }
  }
  return nullptr;
}

LayerManager*
FrameLayerBuilder::GetInactiveLayerManagerFor(nsDisplayItem* aItem)
{
  nsTArray<nsRefPtr<FrameLayerBuilder::DisplayItemData> > *array = GetDisplayItemDataArrayForFrame(aItem->GetUnderlyingFrame());
  NS_ASSERTION(array, "We need an array here!. Really, we do.");

  nsRefPtr<LayerManager> tempManager;
  for (uint32_t i = 0; i < array->Length(); ++i) {
    if (array->ElementAt(i)->mDisplayItemKey == aItem->GetPerFrameKey()) {
      NS_ASSERTION(array->ElementAt(i)->mInactiveManager, "Must already have one of these");
      return array->ElementAt(i)->mInactiveManager;
      
    }
  }
  NS_ERROR("Failed to find data for display item");
  return NULL;
}

already_AddRefed<ColorLayer>
ContainerState::CreateOrRecycleColorLayer(ThebesLayer *aThebes)
{
  ThebesDisplayItemLayerUserData* data = 
      static_cast<ThebesDisplayItemLayerUserData*>(aThebes->GetUserData(&gThebesDisplayItemLayerUserData));
  nsRefPtr<ColorLayer> layer = data->mColorLayer;
  if (layer) {
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);
  } else {
    
    layer = mManager->CreateColorLayer();
    if (!layer)
      return nullptr;
    
    data->mColorLayer = layer;
    layer->SetUserData(&gColorLayerUserData, nullptr);
    
    
    data->mImageLayer = nullptr;
  }
  return layer.forget();
}

already_AddRefed<ImageLayer>
ContainerState::CreateOrRecycleImageLayer(ThebesLayer *aThebes)
{
  ThebesDisplayItemLayerUserData* data = 
      static_cast<ThebesDisplayItemLayerUserData*>(aThebes->GetUserData(&gThebesDisplayItemLayerUserData));
  nsRefPtr<ImageLayer> layer = data->mImageLayer;
  if (layer) {
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);
  } else {
    
    layer = mManager->CreateImageLayer();
    if (!layer)
      return nullptr;
    
    data->mImageLayer = layer;
    layer->SetUserData(&gImageLayerUserData, nullptr);

    
    data->mColorLayer = nullptr;
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

static const double SUBPIXEL_OFFSET_EPSILON = 0.02;








static int32_t
RoundToMatchResidual(double aValue, double aOldResidual)
{
  int32_t v = NSToIntRoundUp(aValue);
  double residual = aValue - v;
  if (aOldResidual < 0) {
    if (residual > 0 && fabs(residual - 1.0 - aOldResidual) < SUBPIXEL_OFFSET_EPSILON) {
      
      return int32_t(ceil(aValue));
    }
  } else if (aOldResidual > 0) {
    if (residual < 0 && fabs(residual + 1.0 - aOldResidual) < SUBPIXEL_OFFSET_EPSILON) {
      
      return int32_t(floor(aValue));
    }
  }
  return v;
}

static void
ResetScrollPositionForLayerPixelAlignment(const nsIFrame* aActiveScrolledRoot)
{
  nsIScrollableFrame* sf = nsLayoutUtils::GetScrollableFrameFor(aActiveScrolledRoot);
  if (sf) {
    sf->ResetScrollPositionForLayerPixelAlignment();
  }
}

static void
InvalidateEntireThebesLayer(ThebesLayer* aLayer, const nsIFrame* aActiveScrolledRoot)
{
#ifdef DEBUG_INVALIDATIONS
  printf("Invalidating entire layer %p\n", aLayer);
#endif
  nsIntRect invalidate = aLayer->GetValidRegion().GetBounds();
  aLayer->InvalidateRegion(invalidate);
  ResetScrollPositionForLayerPixelAlignment(aActiveScrolledRoot);
}

already_AddRefed<ThebesLayer>
ContainerState::CreateOrRecycleThebesLayer(const nsIFrame* aActiveScrolledRoot,
                                           const nsIFrame* aReferenceFrame,
                                           const nsPoint& aTopLeft)
{
  
  nsRefPtr<ThebesLayer> layer;
  ThebesDisplayItemLayerUserData* data;
  bool didResetScrollPositionForLayerPixelAlignment = false;
  if (mNextFreeRecycledThebesLayer < mRecycledThebesLayers.Length()) {
    
    layer = mRecycledThebesLayers[mNextFreeRecycledThebesLayer];
    ++mNextFreeRecycledThebesLayer;
    
    
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);

    data = static_cast<ThebesDisplayItemLayerUserData*>
        (layer->GetUserData(&gThebesDisplayItemLayerUserData));
    NS_ASSERTION(data, "Recycled ThebesLayers must have user data");

    
    
    
    
    
    
    
    
    if (data->mXScale != mParameters.mXScale ||
        data->mYScale != mParameters.mYScale) {
      InvalidateEntireThebesLayer(layer, aActiveScrolledRoot);
      didResetScrollPositionForLayerPixelAlignment = true;
    }
    if (!data->mRegionToInvalidate.IsEmpty()) {
#ifdef DEBUG_INVALIDATIONS
      printf("Invalidating deleted frame content from layer %p\n", layer.get());
#endif
      layer->InvalidateRegion(data->mRegionToInvalidate);
#ifdef DEBUG_INVALIDATIONS
      nsAutoCString str;
      AppendToString(str, data->mRegionToInvalidate);
      printf("Invalidating layer %p: %s\n", layer.get(), str.get());
#endif
      data->mRegionToInvalidate.SetEmpty();
    }

    
    
    
  } else {
    
    layer = mManager->CreateThebesLayer();
    if (!layer)
      return nullptr;
    
    data = new ThebesDisplayItemLayerUserData();
    layer->SetUserData(&gThebesDisplayItemLayerUserData, data);
    ResetScrollPositionForLayerPixelAlignment(aActiveScrolledRoot);
    didResetScrollPositionForLayerPixelAlignment = true;
  }
  data->mXScale = mParameters.mXScale;
  data->mYScale = mParameters.mYScale;
  data->mLastActiveScrolledRootOrigin = data->mActiveScrolledRootOrigin;
  data->mActiveScrolledRootOrigin = aTopLeft;
  data->mAppUnitsPerDevPixel = mAppUnitsPerDevPixel;
  layer->SetAllowResidualTranslation(mParameters.AllowResidualTranslation());

  mLayerBuilder->SaveLastPaintOffset(layer);

  
  
  nsPoint offset = aActiveScrolledRoot->GetOffsetToCrossDoc(aReferenceFrame);
  nscoord appUnitsPerDevPixel = aActiveScrolledRoot->PresContext()->AppUnitsPerDevPixel();
  gfxPoint scaledOffset(
      NSAppUnitsToDoublePixels(offset.x, appUnitsPerDevPixel)*mParameters.mXScale,
      NSAppUnitsToDoublePixels(offset.y, appUnitsPerDevPixel)*mParameters.mYScale);
  
  
  nsIntPoint pixOffset(RoundToMatchResidual(scaledOffset.x, data->mActiveScrolledRootPosition.x),
                       RoundToMatchResidual(scaledOffset.y, data->mActiveScrolledRootPosition.y));
  data->mTranslation = pixOffset;
  pixOffset += mParameters.mOffset;
  gfxMatrix matrix;
  matrix.Translate(gfxPoint(pixOffset.x, pixOffset.y));
  layer->SetBaseTransform(gfx3DMatrix::From2D(matrix));

  
#ifndef MOZ_JAVA_COMPOSITOR
  
  
  gfxPoint activeScrolledRootTopLeft = scaledOffset - matrix.GetTranslation();
  
  
  
  if (!activeScrolledRootTopLeft.WithinEpsilonOf(data->mActiveScrolledRootPosition, SUBPIXEL_OFFSET_EPSILON)) {
    data->mActiveScrolledRootPosition = activeScrolledRootTopLeft;
    InvalidateEntireThebesLayer(layer, aActiveScrolledRoot);
  } else if (didResetScrollPositionForLayerPixelAlignment) {
    data->mActiveScrolledRootPosition = activeScrolledRootTopLeft;
  }
#endif

  return layer.forget();
}






static int32_t
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
ContainerState::FindOpaqueBackgroundColorFor(int32_t aThebesLayerIndex)
{
  ThebesLayerData* target = mThebesLayerDataStack[aThebesLayerIndex];
  for (int32_t i = aThebesLayerIndex - 1; i >= 0; --i) {
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
    int32_t end = NS_MIN<int32_t>(aCurrentClip.mRoundedClipRects.Length(),
                                  mCommonClipCount);
    int32_t clipCount = 0;
    for (; clipCount < end; ++clipCount) {
      if (mItemClip.mRoundedClipRects[clipCount] !=
          aCurrentClip.mRoundedClipRects[clipCount]) {
        break;
      }
    }
    mCommonClipCount = clipCount;
    NS_ASSERTION(mItemClip.mRoundedClipRects.Length() >= uint32_t(mCommonClipCount),
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

  int32_t lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer();

  if ((data->mIsSolidColorInVisibleRegion || imageContainer) &&
      data->mLayer->GetValidRegion().IsEmpty()) {
    NS_ASSERTION(!(data->mIsSolidColorInVisibleRegion && imageContainer),
                 "Can't be a solid color as well as an image!");
    if (imageContainer) {
      nsRefPtr<ImageLayer> imageLayer = CreateOrRecycleImageLayer(data->mLayer);
      imageLayer->SetContainer(imageContainer);
      data->mImage->ConfigureLayer(imageLayer, mParameters.mOffset);
      imageLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      if (data->mItemClip.mHaveClipRect) {
        nsIntRect clip = ScaleToNearestPixels(data->mItemClip.mClipRect);
        clip.MoveBy(mParameters.mOffset);
        imageLayer->IntersectClipRect(clip);
      }
      layer = imageLayer;
      mLayerBuilder->StoreOptimizedLayerForFrame(data->mImage->GetUnderlyingFrame(), 
                                                 data->mImage->GetPerFrameKey(),
                                                 imageLayer);
    } else {
      nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer(data->mLayer);
      colorLayer->SetIsFixedPosition(data->mLayer->GetIsFixedPosition());
      colorLayer->SetColor(data->mSolidColor);

      
      colorLayer->SetBaseTransform(data->mLayer->GetBaseTransform());
      colorLayer->SetPostScale(data->mLayer->GetPostXScale(), data->mLayer->GetPostYScale());

      
      
      
      
      
      
      nsIntRect visibleRect = data->mVisibleRegion.GetBounds();
      visibleRect.MoveBy(mParameters.mOffset);
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
    rgn.MoveBy(-GetTranslationForThebesLayer(data->mLayer));
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

    
    int32_t commonClipCount = data->mCommonClipCount;
    NS_ASSERTION(commonClipCount >= 0, "Inconsistent clip count.");
    SetupMaskLayer(layer, data->mItemClip, commonClipCount);
    
    FrameLayerBuilder::ThebesLayerItemsEntry* entry = mLayerBuilder->
      GetThebesLayerItemsEntry(static_cast<ThebesLayer*>(layer.get()));
    entry->mCommonClipCount = commonClipCount;
  } else {
    
    SetupMaskLayer(layer, data->mItemClip);
  }
  uint32_t flags;
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
  nsIFrame* ref = aBuilder->RootReferenceFrame();
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

  


  if (mVisibleRegion.IsEmpty() &&
      (aItem->GetType() == nsDisplayItem::TYPE_IMAGE ||
       aItem->GetType() == nsDisplayItem::TYPE_XUL_IMAGE)) {
    mImage = static_cast<nsDisplayImageContainer*>(aItem);
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
                                   const nsIFrame* aActiveScrolledRoot,
                                   const nsPoint& aTopLeft)
{
  int32_t i;
  int32_t lowestUsableLayerWithScrolledRoot = -1;
  int32_t topmostLayerWithScrolledRoot = -1;
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
    while (uint32_t(topmostLayerWithScrolledRoot + 1) < mThebesLayerDataStack.Length()) {
      PopThebesLayerData();
    }
  }

  nsRefPtr<ThebesLayer> layer;
  ThebesLayerData* thebesLayerData = nullptr;
  if (lowestUsableLayerWithScrolledRoot < 0) {
    layer = CreateOrRecycleThebesLayer(aActiveScrolledRoot, aItem->ReferenceFrame(), aTopLeft);

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

  
  
  thebesLayerData->UpdateCommonClipCount(aClip);

  thebesLayerData->Accumulate(this, aItem, aVisibleRect, aDrawRect, aClip);

  return thebesLayerData;
}

#ifdef MOZ_DUMP_PAINTING
static void
DumpPaintedImage(nsDisplayItem* aItem, gfxASurface* aSurf)
{
  nsCString string(aItem->Name());
  string.Append("-");
  string.AppendInt((uint64_t)aItem);
  fprintf(gfxUtils::sDumpPaintFile, "array[\"%s\"]=\"", string.BeginReading());
  aSurf->DumpAsDataURL(gfxUtils::sDumpPaintFile);
  fprintf(gfxUtils::sDumpPaintFile, "\";");
}
#endif

static void
PaintInactiveLayer(nsDisplayListBuilder* aBuilder,
                   LayerManager* aManager,
                   nsDisplayItem* aItem,
                   gfxContext* aContext,
                   nsRenderingContext* aCtx)
{
  
  
  BasicLayerManager* basic = static_cast<BasicLayerManager*>(aManager);
  nsRefPtr<gfxContext> context = aContext;
#ifdef MOZ_DUMP_PAINTING
  int32_t appUnitsPerDevPixel = AppUnitsPerDevPixel(aItem);
  nsIntRect itemVisibleRect =
    aItem->GetVisibleRect().ToOutsidePixels(appUnitsPerDevPixel);

  nsRefPtr<gfxASurface> surf;
  if (gfxUtils::sDumpPainting) {
    surf = gfxPlatform::GetPlatform()->CreateOffscreenSurface(itemVisibleRect.Size(),
                                                              gfxASurface::CONTENT_COLOR_ALPHA);
    surf->SetDeviceOffset(-itemVisibleRect.TopLeft());
    context = new gfxContext(surf);
  }
#endif
  basic->SetTarget(context);

  if (aItem->GetType() == nsDisplayItem::TYPE_SVG_EFFECTS) {
    static_cast<nsDisplaySVGEffects*>(aItem)->PaintAsLayer(aBuilder, aCtx, basic);
    if (basic->InTransaction()) {
      basic->AbortTransaction();
    }
  } else {
    basic->EndTransaction(FrameLayerBuilder::DrawThebesLayer, aBuilder);
  }
  FrameLayerBuilder *builder = static_cast<FrameLayerBuilder*>(basic->GetUserData(&gLayerManagerLayerBuilder));
  if (builder) {
    builder->DidEndTransaction();
  }
 
  basic->SetUserData(&gLayerManagerLayerBuilder, NULL);
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
                                    uint32_t aFlags)
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

    bool isFixed;
    bool forceInactive;
    const nsIFrame* activeScrolledRoot;
    nsPoint topLeft;
    if (aFlags & NO_COMPONENT_ALPHA) {
      
      
      
      forceInactive = true;
      activeScrolledRoot = mContainerReferenceFrame;
      topLeft = nsPoint(0, 0);
      isFixed = mBuilder->IsFixedItem(item, nullptr, activeScrolledRoot);
    } else {
      forceInactive = false;
      isFixed = mBuilder->IsFixedItem(item, &activeScrolledRoot);
      topLeft = activeScrolledRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
    }

    
    if (layerState == LAYER_ACTIVE_FORCE ||
        (!forceInactive &&
         (layerState == LAYER_ACTIVE_EMPTY ||
          layerState == LAYER_ACTIVE))) {

      
      
      NS_ASSERTION(layerState != LAYER_ACTIVE_EMPTY ||
                   itemVisibleRect.IsEmpty(),
                   "State is LAYER_ACTIVE_EMPTY but visible rect is not.");

      
      
      
      
      if (itemVisibleRect.IsEmpty() && layerState != LAYER_ACTIVE_EMPTY) {
        InvalidateForLayerChange(item, nullptr, aClip, topLeft);
        continue;
      }

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager, mParameters);
      if (!ownLayer) {
        InvalidateForLayerChange(item, ownLayer, aClip, topLeft);
        continue;
      }

      nsRect invalid;
      if (item->IsInvalid(invalid)) {
        ownLayer->SetInvalidRectToVisibleRegion();
      }

      
      
      if (!ownLayer->AsContainerLayer()) {
        ownLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      }

      ownLayer->SetIsFixedPosition(isFixed);

      
      NS_ASSERTION(ownLayer->Manager() == mManager, "Wrong manager");
      NS_ASSERTION(!ownLayer->HasUserData(&gLayerManagerUserData),
                   "We shouldn't have a FrameLayerBuilder-managed layer here!");
      NS_ASSERTION(aClip.mHaveClipRect ||
                     aClip.mRoundedClipRects.IsEmpty(),
                   "If we have rounded rects, we must have a clip rect");
      
      if (aClip.mHaveClipRect) {
        nsIntRect clip = ScaleToNearestPixels(aClip.NonRoundedIntersection());
        clip.MoveBy(mParameters.mOffset);
        ownLayer->IntersectClipRect(clip);
      }
      ThebesLayerData* data = GetTopThebesLayerData();
      if (data) {
        data->mVisibleAboveRegion.Or(data->mVisibleAboveRegion, itemVisibleRect);
        data->mVisibleAboveRegion.SimplifyOutward(4);
        
        
        
        
        data->mDrawAboveRegion.Or(data->mDrawAboveRegion, itemDrawRect);
        data->mDrawAboveRegion.SimplifyOutward(4);
      }
      itemVisibleRect.MoveBy(mParameters.mOffset);
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

      InvalidateForLayerChange(item, ownLayer, aClip, topLeft);

      mNewChildLayers.AppendElement(ownLayer);
      mLayerBuilder->AddLayerDisplayItem(ownLayer, item, 
                                         aClip, layerState, 
                                         topLeft, nullptr);
    } else {
      ThebesLayerData* data =
        FindThebesLayerFor(item, itemVisibleRect, itemDrawRect, aClip,
                           activeScrolledRoot, topLeft);

      data->mLayer->SetIsFixedPosition(isFixed);

      InvalidateForLayerChange(item, data->mLayer, aClip, topLeft);

      mLayerBuilder->AddThebesDisplayItem(data->mLayer, item, aClip,
                                          mContainerFrame,
                                          layerState, topLeft);

      
      
      data->UpdateCommonClipCount(aClip);
    }
  }
}

void
ContainerState::InvalidateForLayerChange(nsDisplayItem* aItem, 
                                         Layer* aNewLayer,
                                         const FrameLayerBuilder::Clip& aClip,
                                         const nsPoint& aTopLeft)
{
  nsIFrame* f = aItem->GetUnderlyingFrame();
  NS_ASSERTION(f, "Display items that render using Thebes must have a frame");
  uint32_t key = aItem->GetPerFrameKey();
  NS_ASSERTION(key, "Display items that render using Thebes must have a key");
  nsDisplayItemGeometry *oldGeometry = NULL;
  FrameLayerBuilder::Clip* oldClip = NULL;
  nsAutoPtr<nsDisplayItemGeometry> geometry(aItem->AllocateGeometry(mBuilder));
  Layer* oldLayer = mLayerBuilder->GetOldLayerFor(aItem, &oldGeometry, &oldClip);
  if (aNewLayer != oldLayer && oldLayer) {
    
    
    ThebesLayer* t = oldLayer->AsThebesLayer();
    if (t) {
      
      
      
#ifdef DEBUG_INVALIDATIONS
      printf("Display item type %s(%p) changed layers %p to %p!\n", aItem->Name(), f, t, aNewLayer);
#endif
      ThebesDisplayItemLayerUserData* data =
          static_cast<ThebesDisplayItemLayerUserData*>(t->GetUserData(&gThebesDisplayItemLayerUserData));
      InvalidatePostTransformRegion(t,
          oldGeometry->ComputeInvalidationRegion().
            ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
          mLayerBuilder->GetLastPaintOffset(t));
    }
    if (aNewLayer) {
      ThebesLayer* newThebesLayer = aNewLayer->AsThebesLayer();
      if (newThebesLayer) {
        ThebesDisplayItemLayerUserData* data =
            static_cast<ThebesDisplayItemLayerUserData*>(newThebesLayer->GetUserData(&gThebesDisplayItemLayerUserData));
        InvalidatePostTransformRegion(newThebesLayer,
            geometry->ComputeInvalidationRegion().
              ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
            GetTranslationForThebesLayer(newThebesLayer));
      }
    }
    return;
  } 
  if (!aNewLayer) {
    return;
  }

  ThebesLayer* newThebesLayer = aNewLayer->AsThebesLayer();
  if (!newThebesLayer) {
    return;
  }

  ThebesDisplayItemLayerUserData* data =
    static_cast<ThebesDisplayItemLayerUserData*>(newThebesLayer->GetUserData(&gThebesDisplayItemLayerUserData));
  
  
  
  nsRect invalid;
  nsRegion combined;
  if (!oldLayer) {
    
    
    combined = geometry->ComputeInvalidationRegion();
#ifdef DEBUG_INVALIDATIONS
    printf("Display item type %s(%p) added to layer %p!\n", aItem->Name(), f, aNewLayer);
#endif
  } else if (aItem->IsInvalid(invalid) && invalid.IsEmpty()) {
    
    combined.Or(geometry->ComputeInvalidationRegion(), oldGeometry->ComputeInvalidationRegion());
#ifdef DEBUG_INVALIDATIONS
    printf("Display item type %s(%p) (in layer %p) belongs to an invalidated frame!\n", aItem->Name(), f, aNewLayer);
#endif
  } else {
    
    
    nsPoint shift = aTopLeft - data->mLastActiveScrolledRootOrigin;
    oldGeometry->MoveBy(shift);
    aItem->ComputeInvalidationRegion(mBuilder, oldGeometry, &combined);
    oldClip->AddOffsetAndComputeDifference(shift, oldGeometry->ComputeInvalidationRegion(),
                                           aClip, geometry->ComputeInvalidationRegion(),
                                           &combined);

    
    combined = combined.Or(combined, invalid);
#ifdef DEBUG_INVALIDATIONS
    if (!combined.IsEmpty()) {
      printf("Display item type %s(%p) (in layer %p) changed geometry!\n", aItem->Name(), f, aNewLayer);
    }
#endif
  }
  if (!combined.IsEmpty()) {
    InvalidatePostTransformRegion(newThebesLayer,
        combined.ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
        GetTranslationForThebesLayer(newThebesLayer));
  }
}

bool
FrameLayerBuilder::NeedToInvalidateFixedDisplayItem(nsDisplayListBuilder* aBuilder,
                                                    nsDisplayItem* aItem)
{
  if (!aItem->ShouldFixToViewport(aBuilder)) {
    return true;
  }

  nsRefPtr<LayerManager> layerManager;
  nsIFrame* referenceFrame = aBuilder->RootReferenceFrame();
  NS_ASSERTION(referenceFrame == nsLayoutUtils::GetDisplayRootFrame(referenceFrame),
               "Reference frame must be a display root for us to use the layer manager");
  nsIWidget* window = referenceFrame->GetNearestWidget();
  if (window) {
    layerManager = window->GetLayerManager();
  }

  if (layerManager) {
    return !HasRetainedLayerFor(aItem->GetUnderlyingFrame(), aItem->GetPerFrameKey(), layerManager);
  }

  return true;
}

void
FrameLayerBuilder::AddThebesDisplayItem(ThebesLayer* aLayer,
                                        nsDisplayItem* aItem,
                                        const Clip& aClip,
                                        nsIFrame* aContainerLayerFrame,
                                        LayerState aLayerState,
                                        const nsPoint& aTopLeft)
{
  nsRefPtr<LayerManager> tempManager;
  if (aLayerState != LAYER_NONE) {
    DisplayItemData *data = GetDisplayItemDataForManager(aItem, aLayer->Manager());
    if (data) {
      tempManager = data->mInactiveManager;
    }
    if (!tempManager) {
      tempManager = new BasicLayerManager();
    }
  }

  AddLayerDisplayItem(aLayer, aItem, aClip, aLayerState, aTopLeft, tempManager);

  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerLayerFrame;
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
    NS_ASSERTION(aItem->GetUnderlyingFrame(), "Must have frame");
    if (tempManager) {
      FrameLayerBuilder* layerBuilder = new FrameLayerBuilder();
      layerBuilder->Init(mDisplayListBuilder, tempManager);

      tempManager->BeginTransaction();
      if (mRetainingManager) {
        layerBuilder->DidBeginRetainedLayerTransaction(tempManager);
      }
  
      nsAutoPtr<LayerProperties> props(LayerProperties::CloneFrom(tempManager->GetRoot()));
      nsRefPtr<Layer> layer =
        aItem->BuildLayer(mDisplayListBuilder, tempManager, FrameLayerBuilder::ContainerParameters());
      
      
      if (!layer) {
        tempManager->EndTransaction(nullptr, nullptr);
        tempManager->SetUserData(&gLayerManagerLayerBuilder, nullptr);
        return;
      }

      
      
      nsRefPtr<DisplayItemData> data = 
        new DisplayItemData(layer, aItem->GetPerFrameKey(), 
                            LAYER_ACTIVE, mContainerLayerGeneration);
      layerBuilder->StoreDataForFrame(aItem->GetUnderlyingFrame(), data);

      tempManager->SetRoot(layer);
      layerBuilder->WillEndTransaction();

      nsIntPoint offset = GetLastPaintOffset(aLayer) - GetTranslationForThebesLayer(aLayer);
      props->MoveBy(-offset);
      nsIntRect invalid = props->ComputeDifferences(layer, nullptr);
      if (aLayerState == LAYER_SVG_EFFECTS) {
        invalid = nsSVGIntegrationUtils::AdjustInvalidAreaForSVGEffects(aItem->GetUnderlyingFrame(), invalid);
      }
      if (!invalid.IsEmpty()) {
#ifdef DEBUG_INVALIDATIONS
        printf("Inactive LayerManager(%p) for display item %s(%p) has an invalid region - invalidating layer %p\n", tempManager.get(), aItem->Name(), aItem->GetUnderlyingFrame(), aLayer);
#endif
        ThebesDisplayItemLayerUserData* data =
          static_cast<ThebesDisplayItemLayerUserData*>(aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
        invalid.ScaleRoundOut(data->mXScale, data->mYScale);
        InvalidatePostTransformRegion(aLayer, invalid,
                                      GetTranslationForThebesLayer(aLayer));
      }
    }
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem, aClip,
                                                     mContainerLayerGeneration));
    cdi->mInactiveLayer = tempManager;
  }
}

void
FrameLayerBuilder::StoreDataForFrame(nsIFrame* aFrame, DisplayItemData* aData)
{
  DisplayItemDataEntry *entry = mNewDisplayItemData.GetEntry(aFrame);
  if (entry) {
    return;
  }
  entry = mNewDisplayItemData.PutEntry(aFrame);
  if (entry) {
    entry->mData.AppendElement(aData);
  }
}

FrameLayerBuilder::ClippedDisplayItem::~ClippedDisplayItem()
{
  if (mInactiveLayer) {
    
    
    
    BasicLayerManager* basic = static_cast<BasicLayerManager*>(mInactiveLayer.get());
    if (basic->InTransaction()) {
      basic->EndTransaction(nullptr, nullptr);
    }
    basic->SetUserData(&gLayerManagerLayerBuilder, nullptr);
  }
}

void
FrameLayerBuilder::AddLayerDisplayItem(Layer* aLayer,
                                       nsDisplayItem* aItem,
                                       const Clip& aClip,
                                       LayerState aLayerState,
                                       const nsPoint& aTopLeft,
                                       LayerManager* aManager)
{
  if (aLayer->Manager() != mRetainingManager)
    return;
    
  nsRefPtr<DisplayItemData> data = 
    new DisplayItemData(aLayer, aItem->GetPerFrameKey(), aLayerState, mContainerLayerGeneration);
    
  ThebesLayer *t = aLayer->AsThebesLayer();
  if (t) {
    data->mGeometry = aItem->AllocateGeometry(mDisplayListBuilder);
    data->mClip = aClip;
  }
  data->mInactiveManager = aManager;

  DisplayItemDataEntry* entry = 
    mNewDisplayItemData.PutEntry(aItem->GetUnderlyingFrame());
  if (entry) {
    entry->mContainerLayerGeneration = mContainerLayerGeneration;
    entry->mData.AppendElement(data);
    data->AddFrame(aItem->GetUnderlyingFrame());
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    entry = mNewDisplayItemData.PutEntry(mergedFrames[i]);
    if (entry) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
      entry->mData.AppendElement(data);
      data->AddFrame(mergedFrames[i]);
    }
  }
  
  DisplayItemData* oldData = GetDisplayItemDataForManager(aItem, mRetainingManager);
  if (oldData && oldData->FrameListMatches(aItem)) {
    oldData->mUsed = true;
  }
}

nsIntPoint
FrameLayerBuilder::GetLastPaintOffset(ThebesLayer* aLayer)
{
  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(aLayer);
  if (entry) {
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
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
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
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
  for (int32_t i = entry->mItems.Length() - 1; i >= 0; --i) {
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
    if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
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
ContainerState::Finish(uint32_t* aTextContentFlags, LayerManagerData* aData)
{
  while (!mThebesLayerDataStack.IsEmpty()) {
    PopThebesLayerData();
  }

  uint32_t textContentFlags = 0;

  
  
  Layer* layer = nullptr;
  for (uint32_t i = 0; i < mNewChildLayers.Length(); ++i) {
    Layer* prevChild = i == 0 ? nullptr : mNewChildLayers[i - 1].get();
    layer = mNewChildLayers[i];
      
    if (aData) {
      ProcessRemovedDisplayItemsData data(layer, mLayerBuilder);
      aData->mFramesWithLayers.EnumerateEntries(FrameLayerBuilder::ProcessRemovedDisplayItems, &data);
    }

    if (!layer->GetVisibleRegion().IsEmpty()) {
      textContentFlags |= layer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA;
    }

    if (!layer->GetParent()) {
      
      
      mContainerLayer->InsertAfter(layer, prevChild);
      continue;
    }

    NS_ASSERTION(layer->GetParent() == mContainerLayer,
                 "Layer shouldn't be the child of some other container");
    mContainerLayer->RepositionChild(layer, prevChild);
  }

  
  if (!layer) {
    layer = mContainerLayer->GetFirstChild();
  } else {
    layer = layer->GetNextSibling();
  }
  while (layer) {
    Layer *layerToRemove = layer;
    layer = layer->GetNextSibling();
    mContainerLayer->RemoveChild(layerToRemove);
  }

  *aTextContentFlags = textContentFlags;
}

static FrameLayerBuilder::ContainerParameters
ChooseScaleAndSetTransform(FrameLayerBuilder* aLayerBuilder,
                           nsDisplayListBuilder* aDisplayListBuilder,
                           nsIFrame* aContainerFrame,
                           const gfx3DMatrix* aTransform,
                           const FrameLayerBuilder::ContainerParameters& aIncomingScale,
                           ContainerLayer* aLayer,
                           LayerState aState)
{
  nsIntPoint offset;

  gfx3DMatrix transform =
    gfx3DMatrix::ScalingMatrix(aIncomingScale.mXScale, aIncomingScale.mYScale, 1.0);
  if (aTransform) {
    
    transform = (*aTransform)*transform;
    
    
    
    transform.NudgeToIntegers();
  } 
  if (aContainerFrame && aState == LAYER_INACTIVE) {
    
    
    
    nsPoint appUnitOffset = aDisplayListBuilder->ToReferenceFrame(aContainerFrame);
    nscoord appUnitsPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
    offset = nsIntPoint(
        int32_t(NSAppUnitsToDoublePixels(appUnitOffset.x, appUnitsPerDevPixel)*aIncomingScale.mXScale),
        int32_t(NSAppUnitsToDoublePixels(appUnitOffset.y, appUnitsPerDevPixel)*aIncomingScale.mYScale));
  }
  transform = transform * gfx3DMatrix::Translation(offset.x + aIncomingScale.mOffset.x, offset.y + aIncomingScale.mOffset.y, 0);

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
    result(scale.width, scale.height, -offset, aIncomingScale);
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

 PLDHashOperator
FrameLayerBuilder::RestoreDisplayItemData(DisplayItemDataEntry* aEntry, void* aUserArg)
{
  uint32_t *generation = static_cast<uint32_t*>(aUserArg);

  if (aEntry->mContainerLayerGeneration >= *generation) {
    return PL_DHASH_REMOVE;
  }

  for (uint32_t i = 0; i < aEntry->mData.Length(); i++) {
    if (aEntry->mData[i]->mContainerLayerGeneration >= *generation) {
      aEntry->mData.TruncateLength(i);
      return PL_DHASH_NEXT;
    }
  }

  return PL_DHASH_NEXT;
}

 PLDHashOperator
FrameLayerBuilder::RestoreThebesLayerItemEntries(ThebesLayerItemsEntry* aEntry, void* aUserArg)
{
  uint32_t *generation = static_cast<uint32_t*>(aUserArg);

  if (aEntry->mContainerLayerGeneration >= *generation) {
    return PL_DHASH_REMOVE;
  }

  for (uint32_t i = 0; i < aEntry->mItems.Length(); i++) {
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
  uint32_t containerDisplayItemKey =
    aContainerItem ? aContainerItem->GetPerFrameKey() : 0;
  NS_ASSERTION(aContainerFrame, "Container display items here should have a frame");
  NS_ASSERTION(!aContainerItem ||
               aContainerItem->GetUnderlyingFrame() == aContainerFrame,
               "Container display item must match given frame");

  nsRefPtr<ContainerLayer> containerLayer;
  if (aManager == mRetainingManager) {
    
    
    
    
    Layer* oldLayer = nullptr;
    if (aContainerItem) {
      oldLayer = GetOldLayerFor(aContainerItem);
    } else {
      DisplayItemData *data = GetOldLayerForFrame(aContainerFrame, containerDisplayItemKey);
      if (data) {
        oldLayer = data->mLayer;
      }
    }

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

  
  
  
  containerLayer->SetUserData(&gLayerOwnerUserData,
                              new LayerOwnerUserData(aContainerFrame));

  LayerState state = aContainerItem ? aContainerItem->GetLayerState(aBuilder, aManager, aParameters) : LAYER_ACTIVE;

  if (aContainerItem && state == LAYER_ACTIVE_EMPTY) {
    
    
    
    
    NS_ASSERTION(aChildren.IsEmpty(), "Should have no children");
    return containerLayer.forget();
  }

  ContainerParameters scaleParameters =
    ChooseScaleAndSetTransform(this, aBuilder, aContainerFrame, aTransform, aParameters,
                               containerLayer, state);

  uint32_t oldGeneration = mContainerLayerGeneration;
  mContainerLayerGeneration = ++mMaxContainerLayerGeneration;

  nsRefPtr<RefCountedRegion> thebesLayerInvalidRegion = nullptr;
  if (mRetainingManager) {
    nsRefPtr<DisplayItemData> data =
      new  DisplayItemData(containerLayer, containerDisplayItemKey,
                           LAYER_ACTIVE, mContainerLayerGeneration);

    DisplayItemDataEntry* entry = mNewDisplayItemData.PutEntry(aContainerFrame);
    if (entry) {
      entry->mData.AppendElement(data);
      data->AddFrame(aContainerFrame);
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }

    nsAutoTArray<nsIFrame*,4> mergedFrames;
    if (aContainerItem) {
      aContainerItem->GetMergedFrames(&mergedFrames);
    }
    for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
      nsIFrame* mergedFrame = mergedFrames[i];
      entry = mNewDisplayItemData.PutEntry(mergedFrame);
      if (entry) {
        entry->mContainerLayerGeneration = mContainerLayerGeneration;
        entry->mData.AppendElement(data);
        data->AddFrame(mergedFrame);
      }
    }
  }

  nsRect bounds;
  nsIntRect pixBounds;
  int32_t appUnitsPerDevPixel;
  uint32_t stateFlags =
    (aContainerFrame->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) ?
      ContainerState::NO_COMPONENT_ALPHA : 0;
  uint32_t flags;
  
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));
  while (true) {
    ContainerState state(aBuilder, aManager, aManager->GetLayerBuilder(),
                         aContainerFrame, aContainerItem,
                         containerLayer, scaleParameters);
    
    Clip clip;
    state.ProcessDisplayItems(aChildren, clip, stateFlags);

    
    
    
    state.Finish(&flags, data);
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
      continue;
    }
    break;
  }

  NS_ASSERTION(bounds.IsEqualInterior(aChildren.GetBounds(aBuilder)), "Wrong bounds");
  pixBounds.MoveBy(nsIntPoint(scaleParameters.mOffset.x, scaleParameters.mOffset.y));
  containerLayer->SetVisibleRegion(pixBounds);
  
  
  if (aChildren.IsOpaque() && !aChildren.NeedsTransparentSurface()) {
    bounds.ScaleRoundIn(scaleParameters.mXScale, scaleParameters.mYScale);
    if (bounds.Contains(pixBounds.ToAppUnits(appUnitsPerDevPixel))) {
      
      flags = Layer::CONTENT_OPAQUE;
    }
  }
  containerLayer->SetContentFlags(flags);

  mContainerLayerGeneration = oldGeneration;
  containerLayer->SetUserData(&gNotifySubDocInvalidationData, nullptr);

  return containerLayer.forget();
}

Layer*
FrameLayerBuilder::GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                                   nsDisplayItem* aItem)
{
  nsIFrame* f = aItem->GetUnderlyingFrame();
  NS_ASSERTION(f, "Can only call GetLeafLayerFor on items that have a frame");
  Layer* layer = GetOldLayerFor(aItem);
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
FrameLayerBuilder::InvalidateAllLayers(LayerManager* aManager)
{
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));
  if (data) {
    data->mInvalidateAllLayers = true;
  }
}

 void
FrameLayerBuilder::InvalidateAllLayersForFrame(nsIFrame *aFrame)
{
  LayerManagerData* data = GetDefaultLayerManagerDataForFrame(aFrame);
  if (data) {
    data->mInvalidateAllLayers = true;
  }
  data = GetSecondaryLayerManagerDataForFrame(aFrame);
  if (data) {
    data->mInvalidateAllLayers = true;
  }
}


Layer*
FrameLayerBuilder::GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  
  
  

  DisplayItemData *data = GetDisplayItemDataForManager(aFrame, aDisplayItemKey);
  if (!data) {
    return nullptr;
  }

  if (data->mOptLayer) {
    return data->mOptLayer;
  }

  Layer* layer = data->mLayer;
  if (!layer->HasUserData(&gColorLayerUserData) &&
      !layer->HasUserData(&gImageLayerUserData) &&
      !layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
    return layer;
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
    LayerManagerData *data = GetDefaultLayerManagerDataForFrame(f);
    if (!data)
      continue;
    DisplayItemDataEntry *entry = data->mFramesWithLayers.GetEntry(f);
    
    
    if (!entry)
      continue;
    nsTArray<nsRefPtr<DisplayItemData> >* array = &entry->mData;
    for (uint32_t i = 0; i < array->Length(); ++i) {
      Layer* layer = array->ElementAt(i)->mLayer;
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

  FrameLayerBuilder *layerBuilder = aLayer->Manager()->GetLayerBuilder();

  if (layerBuilder->CheckDOMModified())
    return;

  nsTArray<ClippedDisplayItem> items;
  uint32_t commonClipCount;
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
  int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();

  uint32_t i;
  
  
  
  
  
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
      PaintInactiveLayer(builder, cdi->mInactiveLayer, cdi->mItem, aContext, rc);
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
  if (!aRegionToInvalidate.IsEmpty()) {
    aLayer->AddInvalidRect(aRegionToInvalidate.GetBounds());
  }
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
FrameLayerBuilder::DumpRetainedLayerTree(LayerManager* aManager, FILE* aFile, bool aDumpHtml)
{
  aManager->Dump(aFile, "", aDumpHtml);
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
                                 uint32_t aBegin, uint32_t aEnd)
{
  int32_t A2D = aPresContext->AppUnitsPerDevPixel();
  ApplyRectTo(aContext, A2D);
  ApplyRoundedRectsTo(aContext, A2D, aBegin, aEnd);
}

void
FrameLayerBuilder::Clip::ApplyRectTo(gfxContext* aContext, int32_t A2D) const
{
  aContext->NewPath();
  gfxRect clip = nsLayoutUtils::RectToGfxRect(mClipRect, A2D);
  aContext->Rectangle(clip, true);
  aContext->Clip();
}

void
FrameLayerBuilder::Clip::ApplyRoundedRectsTo(gfxContext* aContext,
                                             int32_t A2D,
                                             uint32_t aBegin, uint32_t aEnd) const
{
  aEnd = NS_MIN<uint32_t>(aEnd, mRoundedClipRects.Length());

  for (uint32_t i = aBegin; i < aEnd; ++i) {
    AddRoundedRectPathTo(aContext, A2D, mRoundedClipRects[i]);
    aContext->Clip();
  }
}

void
FrameLayerBuilder::Clip::DrawRoundedRectsTo(gfxContext* aContext,
                                            int32_t A2D,
                                            uint32_t aBegin, uint32_t aEnd) const
{
  aEnd = NS_MIN<uint32_t>(aEnd, mRoundedClipRects.Length());

  if (aEnd - aBegin == 0)
    return;

  
  
  ApplyRoundedRectsTo(aContext, A2D, aBegin, aEnd - 1);
  AddRoundedRectPathTo(aContext, A2D, mRoundedClipRects[aEnd - 1]);
  aContext->Fill();
}

void
FrameLayerBuilder::Clip::AddRoundedRectPathTo(gfxContext* aContext,
                                              int32_t A2D,
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
  for (uint32_t i = 0, iEnd = mRoundedClipRects.Length();
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
  for (uint32_t i = 0, iEnd = mRoundedClipRects.Length();
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
  for (uint32_t i = 0, iEnd = mRoundedClipRects.Length();
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

static void
AccumulateRectDifference(const nsRect& aR1, const nsRect& aR2, nsRegion* aOut)
{
  if (aR1.IsEqualInterior(aR2))
    return;
  nsRegion r;
  r.Xor(aR1, aR2);
  aOut->Or(*aOut, r);
}

void
FrameLayerBuilder::Clip::AddOffsetAndComputeDifference(const nsPoint& aOffset,
                                                       const nsRect& aBounds,
                                                       const Clip& aOther,
                                                       const nsRect& aOtherBounds,
                                                       nsRegion* aDifference)
{
  if (mHaveClipRect != aOther.mHaveClipRect ||
      mRoundedClipRects.Length() != aOther.mRoundedClipRects.Length()) {
    aDifference->Or(*aDifference, aBounds);
    aDifference->Or(*aDifference, aOtherBounds);
    return;
  }
  if (mHaveClipRect) {
    AccumulateRectDifference((mClipRect + aOffset).Intersect(aBounds),
                             aOther.mClipRect.Intersect(aOtherBounds),
                             aDifference);
  }
  for (uint32_t i = 0; i < mRoundedClipRects.Length(); ++i) {
    if (mRoundedClipRects[i] + aOffset != aOther.mRoundedClipRects[i]) {
      
      aDifference->Or(*aDifference, mRoundedClipRects[i].mRect.Intersect(aBounds));
      aDifference->Or(*aDifference, aOther.mRoundedClipRects[i].mRect.Intersect(aOtherBounds));
    }
  }
}

gfxRect
CalculateBounds(const nsTArray<FrameLayerBuilder::Clip::RoundedRect>& aRects, int32_t A2D)
{
  nsRect bounds = aRects[0].mRect;
  for (uint32_t i = 1; i < aRects.Length(); ++i) {
    bounds.UnionRect(bounds, aRects[i].mRect);
   }
 
  return nsLayoutUtils::RectToGfxRect(bounds, A2D);
}

static void
SetClipCount(ThebesDisplayItemLayerUserData* aThebesData,
             uint32_t aClipCount)
{
  if (aThebesData) {
    aThebesData->mMaskClipCount = aClipCount;
  }
}

void
ContainerState::SetupMaskLayer(Layer *aLayer, const FrameLayerBuilder::Clip& aClip,
                               uint32_t aRoundedRectClipCount)
{
  
  
  
  ThebesDisplayItemLayerUserData* thebesData = GetThebesDisplayItemLayerUserData(aLayer);
  if (thebesData &&
      aRoundedRectClipCount < thebesData->mMaskClipCount) {
    ThebesLayer* thebes = aLayer->AsThebesLayer();
    thebes->InvalidateRegion(thebes->GetValidRegion().GetBounds());
  }

  
  nsIntRect layerBounds = aLayer->GetVisibleRegion().GetBounds();
  if (aClip.mRoundedClipRects.IsEmpty() ||
      aRoundedRectClipCount == 0 ||
      layerBounds.IsEmpty()) {
    SetClipCount(thebesData, 0);
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
    SetClipCount(thebesData, aRoundedRectClipCount);
    return;
  }

  
  const int32_t A2D = mContainerFrame->PresContext()->AppUnitsPerDevPixel();
  gfxRect boundingRect = CalculateBounds(newData.mRoundedClipRects, A2D);
  boundingRect.Scale(mParameters.mXScale, mParameters.mYScale);

  uint32_t maxSize = mManager->GetMaxTextureSize();
  NS_ASSERTION(maxSize > 0, "Invalid max texture size");
  nsIntSize surfaceSize(NS_MIN<int32_t>(boundingRect.Width(), maxSize),
                        NS_MIN<int32_t>(boundingRect.Height(), maxSize));

  
  
  
  
  gfxMatrix maskTransform;
  maskTransform.Scale(float(surfaceSize.width)/float(boundingRect.Width()),
                      float(surfaceSize.height)/float(boundingRect.Height()));
  maskTransform.Translate(-boundingRect.TopLeft());
  
  gfxMatrix imageTransform = maskTransform;
  imageTransform.Scale(mParameters.mXScale, mParameters.mYScale);

  nsAutoPtr<MaskLayerImageCache::MaskLayerImageKey> newKey(
    new MaskLayerImageCache::MaskLayerImageKey(aLayer->Manager()->GetBackendType()));

  
  for (uint32_t i = 0; i < newData.mRoundedClipRects.Length(); ++i) {
    newKey->mRoundedClipRects.AppendElement(
      MaskLayerImageCache::PixelRoundedRect(newData.mRoundedClipRects[i],
                                            mContainerFrame->PresContext()));
    newKey->mRoundedClipRects[i].ScaleAndTranslate(imageTransform);
  }
 
  const MaskLayerImageCache::MaskLayerImageKey* lookupKey = newKey;

  
  nsRefPtr<ImageContainer> container =
    GetMaskLayerImageCache()->FindImageFor(&lookupKey);

  if (!container) {
    
    nsRefPtr<gfxASurface> surface =
      aLayer->Manager()->CreateOptimalMaskSurface(surfaceSize);

    
    if (!surface || surface->CairoStatus()) {
      NS_WARNING("Could not create surface for mask layer.");
      SetClipCount(thebesData, 0);
      return;
    }

    nsRefPtr<gfxContext> context = new gfxContext(surface);
    context->Multiply(imageTransform);

    
    context->SetColor(gfxRGBA(1, 1, 1, 1));
    aClip.DrawRoundedRectsTo(context, A2D, 0, aRoundedRectClipCount);

    
    container = aLayer->Manager()->CreateImageContainer();
    NS_ASSERTION(container, "Could not create image container for mask layer.");
    static const ImageFormat format = CAIRO_SURFACE;
    nsRefPtr<Image> image = container->CreateImage(&format, 1);
    NS_ASSERTION(image, "Could not create image container for mask layer.");
    CairoImage::Data data;
    data.mSurface = surface;
    data.mSize = surfaceSize;
    static_cast<CairoImage*>(image.get())->SetData(data);
    container->SetCurrentImageInTransaction(image);

    GetMaskLayerImageCache()->PutImage(newKey.forget(), container);
  }

  maskLayer->SetContainer(container);
  
  gfx3DMatrix matrix = gfx3DMatrix::From2D(maskTransform.Invert());
  matrix.Translate(gfxPoint3D(mParameters.mOffset.x, mParameters.mOffset.y, 0));
  maskLayer->SetBaseTransform(matrix);

  
  userData->mScaleX = newData.mScaleX;
  userData->mScaleY = newData.mScaleY;
  userData->mRoundedClipRects.SwapElements(newData.mRoundedClipRects);
  userData->mImageKey = lookupKey;

  aLayer->SetMaskLayer(maskLayer);
  SetClipCount(thebesData, aRoundedRectClipCount);
  return;
}

} 
