




#include "mozilla/DebugOnly.h"

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
#include "mozilla/gfx/Tools.h"

#include "nsAnimationManager.h"
#include "nsTransitionManager.h"
#include <algorithm>

#ifdef DEBUG
#include <stdio.h>


#endif

using namespace mozilla::layers;
using namespace mozilla::gfx;

namespace mozilla {

FrameLayerBuilder::DisplayItemData::DisplayItemData(LayerManagerData* aParent, uint32_t aKey, 
                                                    Layer* aLayer, LayerState aLayerState, uint32_t aGeneration)

  : mParent(aParent)
  , mLayer(aLayer)
  , mDisplayItemKey(aKey)
  , mContainerLayerGeneration(aGeneration)
  , mLayerState(aLayerState)
  , mUsed(true)
  , mIsInvalid(false)
{
}

FrameLayerBuilder::DisplayItemData::DisplayItemData(DisplayItemData &toCopy)
{
  
  
  mParent = toCopy.mParent;
  mLayer = toCopy.mLayer;
  mInactiveManager = toCopy.mInactiveManager;
  mFrameList = toCopy.mFrameList;
  mGeometry = toCopy.mGeometry;
  mDisplayItemKey = toCopy.mDisplayItemKey;
  mClip = toCopy.mClip;
  mContainerLayerGeneration = toCopy.mContainerLayerGeneration;
  mLayerState = toCopy.mLayerState;
  mUsed = toCopy.mUsed;
}

void
FrameLayerBuilder::DisplayItemData::AddFrame(nsIFrame* aFrame)
{
  mFrameList.AppendElement(aFrame);

  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(FrameLayerBuilder::LayerManagerDataProperty()));
  if (!array) {
    array = new nsTArray<DisplayItemData*>();
    aFrame->Properties().Set(FrameLayerBuilder::LayerManagerDataProperty(), array);
  }
  array->AppendElement(this);
}

void
FrameLayerBuilder::DisplayItemData::RemoveFrame(nsIFrame* aFrame)
{
  DebugOnly<bool> result = mFrameList.RemoveElement(aFrame);
  NS_ASSERTION(result, "Can't remove a frame that wasn't added!");

  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(FrameLayerBuilder::LayerManagerDataProperty()));
  NS_ASSERTION(array, "Must be already stored on the frame!");
  array->RemoveElement(this);
}

void
FrameLayerBuilder::DisplayItemData::UpdateContents(Layer* aLayer, LayerState aState, 
                                                   uint32_t aContainerLayerGeneration,
                                                   nsDisplayItem* aItem )
{
  mLayer = aLayer;
  mOptLayer = nullptr;
  mInactiveManager = nullptr;
  mLayerState = aState;
  mContainerLayerGeneration = aContainerLayerGeneration;
  mGeometry = nullptr;
  mClip.mHaveClipRect = false;
  mClip.mRoundedClipRects.Clear();
  mUsed = true;

  if (!aItem) {
    return;
  }

  nsAutoTArray<nsIFrame*, 4> copy(mFrameList);
  if (!copy.RemoveElement(aItem->GetUnderlyingFrame())) {
    AddFrame(aItem->GetUnderlyingFrame());
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    if (!copy.RemoveElement(mergedFrames[i])) {
      AddFrame(mergedFrames[i]);
    }
  }

  for (uint32_t i = 0; i < copy.Length(); i++) {
    RemoveFrame(copy[i]); 
  }
}

static nsIFrame* sDestroyedFrame = NULL;
FrameLayerBuilder::DisplayItemData::~DisplayItemData()
{
  for (uint32_t i = 0; i < mFrameList.Length(); i++) {
    nsIFrame* frame = mFrameList[i];
    if (frame == sDestroyedFrame) {
      continue;
    }
    nsTArray<DisplayItemData*> *array = 
      reinterpret_cast<nsTArray<DisplayItemData*>*>(frame->Properties().Get(LayerManagerDataProperty()));
    array->RemoveElement(this);
  }
}

void
FrameLayerBuilder::DisplayItemData::GetFrameListChanges(nsDisplayItem* aOther, 
                                                        nsTArray<nsIFrame*>& aOut)
{
  aOut = mFrameList;
  nsAutoTArray<nsIFrame*, 4> added;
  if (!aOut.RemoveElement(aOther->GetUnderlyingFrame())) {
    added.AppendElement(aOther->GetUnderlyingFrame());
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aOther->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    if (!aOut.RemoveElement(mergedFrames[i])) {
      added.AppendElement(mergedFrames[i]);
    }
  }

  aOut.AppendElements(added); 
}




class LayerManagerData : public LayerUserData {
public:
  LayerManagerData(LayerManager *aManager)
    : mLayerManager(aManager)
#ifdef DEBUG_DISPLAY_ITEM_DATA
    , mParent(nullptr)
#endif
    , mInvalidateAllLayers(false)
  {
    MOZ_COUNT_CTOR(LayerManagerData);
    mDisplayItems.Init();
  }
  ~LayerManagerData() {
    MOZ_COUNT_DTOR(LayerManagerData);
  }
 
#ifdef DEBUG_DISPLAY_ITEM_DATA
  void Dump(const char *aPrefix = "") {
    printf("%sLayerManagerData %p\n", aPrefix, this);
    nsAutoCString prefix;
    prefix += aPrefix;
    prefix += "  ";
    mDisplayItems.EnumerateEntries(
        FrameLayerBuilder::DumpDisplayItemDataForFrame, (void*)prefix.get());
  }
#endif

  


  LayerManager *mLayerManager;
#ifdef DEBUG_DISPLAY_ITEM_DATA
  LayerManagerData *mParent;
#endif
  nsTHashtable<nsRefPtrHashKey<FrameLayerBuilder::DisplayItemData> > mDisplayItems;
  bool mInvalidateAllLayers;
};

 void
FrameLayerBuilder::DestroyDisplayItemDataFor(nsIFrame* aFrame)
{
  FrameProperties props = aFrame->Properties();
  props.Delete(LayerManagerDataProperty());
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
    mContainerReferenceFrame = aContainerItem ? aContainerItem->ReferenceFrameForChildren() :
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
                           uint32_t aFlags,
                           const nsIFrame* aForceActiveScrolledRoot = nullptr);
  







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

    




    already_AddRefed<ImageContainer> CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder);

    




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
                                const nsPoint& aTopLeft,
                                nsDisplayItemGeometry *aGeometry);
  





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
                      uint32_t aRoundedRectClipCount = UINT32_MAX);

  bool ChooseActiveScrolledRoot(const nsDisplayList& aList,
                                const nsIFrame **aActiveScrolledRoot);

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










uint8_t gThebesDisplayItemLayerUserData;





uint8_t gColorLayerUserData;





uint8_t gImageLayerUserData;





uint8_t gLayerManagerUserData;





uint8_t gMaskLayerUserData;





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

FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemData(nsIFrame* aFrame, uint32_t aKey)
{
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
  if (array) {
    for (uint32_t i = 0; i < array->Length(); i++) {
      DisplayItemData* item = array->ElementAt(i);
      if (item->mDisplayItemKey == aKey &&
          item->mLayer->Manager() == mRetainingManager) {
        return item;
      }
    }
  }
  return nullptr;
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

static void
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsRect& aRect, 
                              const FrameLayerBuilder::Clip& aClip,
                              const nsIntPoint& aTranslation)
{
  ThebesDisplayItemLayerUserData* data =
      static_cast<ThebesDisplayItemLayerUserData*>(aLayer->GetUserData(&gThebesDisplayItemLayerUserData));

  nsRect rect = aClip.ApplyNonRoundedIntersection(aRect);

  nsIntRect pixelRect = rect.ScaleToOutsidePixels(data->mXScale, data->mYScale, data->mAppUnitsPerDevPixel);
  InvalidatePostTransformRegion(aLayer, pixelRect, aTranslation);
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

















 void
FrameLayerBuilder::RemoveFrameFromLayerManager(nsIFrame* aFrame,
                                               void* aPropertyValue)
{
  sDestroyedFrame = aFrame;
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aPropertyValue);

  
  
  nsTArray<nsRefPtr<DisplayItemData> > arrayCopy;
  for (uint32_t i = 0; i < array->Length(); ++i) {
    arrayCopy.AppendElement(array->ElementAt(i));
  }

#ifdef DEBUG_DISPLAY_ITEM_DATA
  if (array->Length()) {
    LayerManagerData *rootData = array->ElementAt(0)->mParent;
    while (rootData->mParent) {
      rootData = rootData->mParent;
    }
    printf("Removing frame %p - dumping display data\n", aFrame);
    rootData->Dump();
  }
#endif

  for (uint32_t i = 0; i < array->Length(); ++i) {
    DisplayItemData* data = array->ElementAt(i);

    ThebesLayer* t = data->mLayer->AsThebesLayer();
    if (t) {
      ThebesDisplayItemLayerUserData* thebesData =
          static_cast<ThebesDisplayItemLayerUserData*>(t->GetUserData(&gThebesDisplayItemLayerUserData));
      if (thebesData) {
        nsRegion old = data->mGeometry->ComputeInvalidationRegion();
        nsIntRegion rgn = old.ScaleToOutsidePixels(thebesData->mXScale, thebesData->mYScale, thebesData->mAppUnitsPerDevPixel);
        rgn.MoveBy(-GetTranslationForThebesLayer(t));
        thebesData->mRegionToInvalidate.Or(thebesData->mRegionToInvalidate, rgn);
        thebesData->mRegionToInvalidate.SimplifyOutward(8);
      }
    }

    data->mParent->mDisplayItems.RemoveEntry(data);
  }

  arrayCopy.Clear();
  delete array;
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
    data = new LayerManagerData(aManager);
    aManager->SetUserData(&gLayerManagerUserData, data);
  }
}

void
FrameLayerBuilder::StoreOptimizedLayerForFrame(nsDisplayItem* aItem, Layer* aLayer)
{
  if (!mRetainingManager) {
    return;
  }

  DisplayItemData* data = GetDisplayItemDataForManager(aItem, aLayer->Manager());
  NS_ASSERTION(data, "Must have already stored data for this item!");
  data->mOptLayer = aLayer;
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
  
  data->mDisplayItems.EnumerateEntries(ProcessRemovedDisplayItems, this);
  data->mInvalidateAllLayers = false;
}

 PLDHashOperator
FrameLayerBuilder::ProcessRemovedDisplayItems(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                              void* aUserArg)
{
  DisplayItemData* data = aEntry->GetKey();
  if (!data->mUsed) {
    
    FrameLayerBuilder* layerBuilder = static_cast<FrameLayerBuilder*>(aUserArg);

    ThebesLayer* t = data->mLayer->AsThebesLayer();
    if (t) {
#ifdef DEBUG_INVALIDATIONS
      printf("Invalidating unused display item (%i) belonging to frame %p from layer %p\n", data->mDisplayItemKey, data->mFrameList[0], t);
#endif
      InvalidatePostTransformRegion(t,
                                    data->mGeometry->ComputeInvalidationRegion(),
                                    data->mClip,
                                    layerBuilder->GetLastPaintOffset(t));
    }
    return PL_DHASH_REMOVE;
  }

  data->mUsed = false;
  data->mIsInvalid = false;
  return PL_DHASH_NEXT;
}
  
 PLDHashOperator 
FrameLayerBuilder::DumpDisplayItemDataForFrame(nsRefPtrHashKey<DisplayItemData>* aEntry,
                                               void* aClosure)
{
#ifdef DEBUG_DISPLAY_ITEM_DATA
  DisplayItemData *data = aEntry->GetKey();

  nsAutoCString prefix;
  prefix += static_cast<const char*>(aClosure);
  
  const char *layerState;
  switch (data->mLayerState) {
    case LAYER_NONE:
      layerState = "LAYER_NONE"; break;
    case LAYER_INACTIVE:
      layerState = "LAYER_INACTIVE"; break;
    case LAYER_ACTIVE:
      layerState = "LAYER_ACTIVE"; break;
    case LAYER_ACTIVE_FORCE:
      layerState = "LAYER_ACTIVE_FORCE"; break;
    case LAYER_ACTIVE_EMPTY:
      layerState = "LAYER_ACTIVE_EMPTY"; break;
    case LAYER_SVG_EFFECTS:
      layerState = "LAYER_SVG_EFFECTS"; break;
  }
  uint32_t mask = (1 << nsDisplayItem::TYPE_BITS) - 1;

  nsAutoCString str;
  str += prefix;
  str += nsPrintfCString("Frame %p ", data->mFrameList[0]);
  str += nsDisplayItem::DisplayItemTypeName(static_cast<nsDisplayItem::Type>(data->mDisplayItemKey & mask));
  if ((data->mDisplayItemKey >> nsDisplayItem::TYPE_BITS)) {
    str += nsPrintfCString("(%i)", data->mDisplayItemKey >> nsDisplayItem::TYPE_BITS);
  }
  str += nsPrintfCString(", %s, Layer %p", layerState, data->mLayer.get());
  if (data->mOptLayer) {
    str += nsPrintfCString(", OptLayer %p", data->mOptLayer.get());
  }
  if (data->mInactiveManager) {
    str += nsPrintfCString(", InactiveLayerManager %p", data->mInactiveManager.get());
  }
  str += "\n";

  printf("%s", str.get());
    
  if (data->mInactiveManager) {
    prefix += "  ";
    printf("%sDumping inactive layer info:\n", prefix.get());
    LayerManagerData* lmd = static_cast<LayerManagerData*>
      (data->mInactiveManager->GetUserData(&gLayerManagerUserData));
    lmd->Dump(prefix.get());
  }
#endif
  return PL_DHASH_NEXT;
}

 FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetDisplayItemDataForManager(nsDisplayItem* aItem, 
                                                LayerManager* aManager)
{
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aItem->GetUnderlyingFrame()->Properties().Get(LayerManagerDataProperty()));
  if (array) {
    for (uint32_t i = 0; i < array->Length(); i++) {
      DisplayItemData* item = array->ElementAt(i);
      if (item->mDisplayItemKey == aItem->GetPerFrameKey() &&
          item->mLayer->Manager() == aManager) {
        return item;
      }
    }
  }
  return nullptr;
}

bool
FrameLayerBuilder::HasRetainedDataFor(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
  if (array) {
    for (uint32_t i = 0; i < array->Length(); i++) {
      if (array->ElementAt(i)->mDisplayItemKey == aDisplayItemKey) {
        return true;
      }
    }
  }
  return false;
}

void
FrameLayerBuilder::IterateRetainedDataFor(nsIFrame* aFrame, DisplayItemDataCallback aCallback)
{
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
  if (!array) {
    return;
  }
  
  for (uint32_t i = 0; i < array->Length(); i++) {
    DisplayItemData* data = array->ElementAt(i);
    if (data->mDisplayItemKey != nsDisplayItem::TYPE_ZERO) {
      aCallback(aFrame, data);
    }
  }
}

FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::GetOldLayerForFrame(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  
  
  if (!mRetainingManager || mInvalidateAllLayers)
    return nullptr;

  DisplayItemData *data = GetDisplayItemData(aFrame, aDisplayItemKey);

  if (data && data->mLayer->Manager() == mRetainingManager) {
    return data;
  }
  return nullptr;
}

Layer*
FrameLayerBuilder::GetOldLayerFor(nsDisplayItem* aItem, 
                                  nsDisplayItemGeometry** aOldGeometry, 
                                  Clip** aOldClip,
                                  nsTArray<nsIFrame*>* aChangedFrames,
                                  bool *aIsInvalid)
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
      if (aChangedFrames) {
        oldData->GetFrameListChanges(aItem, *aChangedFrames); 
      }
      if (aIsInvalid) {
        *aIsInvalid = oldData->mIsInvalid;
      }
      return oldData->mLayer;
    }
  }

  return nullptr;
} 

 Layer*
FrameLayerBuilder::GetDebugOldLayerFor(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));

  if (!array) {
    return nullptr;
  }

  for (uint32_t i = 0; i < array->Length(); i++) {
    DisplayItemData *data = array->ElementAt(i);

    if (data->mDisplayItemKey == aDisplayItemKey) {
      return data->mLayer; 
    }
  }
  return nullptr;
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
#ifndef MOZ_ANDROID_OMTC
  bool didResetScrollPositionForLayerPixelAlignment = false;
#endif
  if (mNextFreeRecycledThebesLayer < mRecycledThebesLayers.Length()) {
    
    layer = mRecycledThebesLayers[mNextFreeRecycledThebesLayer];
    ++mNextFreeRecycledThebesLayer;
    
    
    layer->SetClipRect(nullptr);
    layer->SetMaskLayer(nullptr);

    data = static_cast<ThebesDisplayItemLayerUserData*>
        (layer->GetUserData(&gThebesDisplayItemLayerUserData));
    NS_ASSERTION(data, "Recycled ThebesLayers must have user data");

    
    
    
    
    
    
    
    
    if (!FuzzyEqual(data->mXScale, mParameters.mXScale, 0.00001) ||
        !FuzzyEqual(data->mYScale, mParameters.mYScale, 0.00001) ||
        data->mAppUnitsPerDevPixel != mAppUnitsPerDevPixel) {
      InvalidateEntireThebesLayer(layer, aActiveScrolledRoot);
#ifndef MOZ_ANDROID_OMTC
      didResetScrollPositionForLayerPixelAlignment = true;
#endif
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
#ifndef MOZ_ANDROID_OMTC
    didResetScrollPositionForLayerPixelAlignment = true;
#endif
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

  
#ifndef MOZ_ANDROID_OMTC
  
  
  gfxPoint activeScrolledRootTopLeft = scaledOffset - matrix.GetTranslation() + mParameters.mOffset;
  
  
  
  if (!activeScrolledRootTopLeft.WithinEpsilonOf(data->mActiveScrolledRootPosition, SUBPIXEL_OFFSET_EPSILON)) {
    data->mActiveScrolledRootPosition = activeScrolledRootTopLeft;
    InvalidateEntireThebesLayer(layer, aActiveScrolledRoot);
  } else if (didResetScrollPositionForLayerPixelAlignment) {
    data->mActiveScrolledRootPosition = activeScrolledRootTopLeft;
  }
#endif

  return layer.forget();
}

#ifdef MOZ_DUMP_PAINTING





static int32_t
AppUnitsPerDevPixel(nsDisplayItem* aItem)
{
  
  
  
  
  if (aItem->GetType() == nsDisplayItem::TYPE_ZOOM) {
    return static_cast<nsDisplayZoom*>(aItem)->GetParentAppUnitsPerDevPixel();
  }
  return aItem->GetUnderlyingFrame()->PresContext()->AppUnitsPerDevPixel();
}
#endif









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

    
    
    nsIntRect deviceRect = target->mVisibleRegion.GetBounds();
    nsRect appUnitRect = deviceRect.ToAppUnits(mAppUnitsPerDevPixel);
    appUnitRect.ScaleInverseRoundOut(mParameters.mXScale, mParameters.mYScale);

    FrameLayerBuilder::ThebesLayerItemsEntry* entry =
      mLayerBuilder->GetThebesLayerItemsEntry(candidate->mLayer);
    NS_ASSERTION(entry, "Must know about this layer!");
    for (int32_t j = entry->mItems.Length() - 1; j >= 0; --j) {
      nsDisplayItem* item = entry->mItems[j].mItem;
      bool snap;
      nsRect bounds = item->GetBounds(mBuilder, &snap);
      if (snap && mSnappingEnabled) {
        nsIntRect snappedBounds = ScaleToNearestPixels(bounds);
        if (!snappedBounds.Intersects(deviceRect))
          continue;

        if (!snappedBounds.Contains(deviceRect))
          break;

      } else {
        
        
        if (!bounds.Intersects(appUnitRect))
          continue;

        if (!bounds.Contains(appUnitRect))
          break;
      }

      nscolor color;
      if (item->IsUniform(mBuilder, &color) && NS_GET_A(color) == 255)
        return color;

      break;
    }
    break;
  }
  return NS_RGBA(0,0,0,0);
}

void
ContainerState::ThebesLayerData::UpdateCommonClipCount(
    const FrameLayerBuilder::Clip& aCurrentClip)
{
  if (mCommonClipCount >= 0) {
    int32_t end = std::min<int32_t>(aCurrentClip.mRoundedClipRects.Length(),
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
ContainerState::ThebesLayerData::CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder)
{
  if (!mImage) {
    return nullptr;
  }

  return mImage->GetContainer(mLayer->Manager(), aBuilder);
}

void
ContainerState::PopThebesLayerData()
{
  NS_ASSERTION(!mThebesLayerDataStack.IsEmpty(), "Can't pop");

  int32_t lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer(mBuilder);

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
      mLayerBuilder->StoreOptimizedLayerForFrame(data->mImage,
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
      aItem->SupportsOptimizingToImage()) {
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

  basic->SetTarget(nullptr);

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





bool
ContainerState::ChooseActiveScrolledRoot(const nsDisplayList& aList,
                                         const nsIFrame **aActiveScrolledRoot)
{
  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    nsDisplayItem::Type type = item->GetType();
    if (type == nsDisplayItem::TYPE_CLIP ||
        type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
      if (ChooseActiveScrolledRoot(*item->GetSameCoordinateSystemChildren(),
                                   aActiveScrolledRoot)) {
        return true;
      }
      continue;
    }

    LayerState layerState = item->GetLayerState(mBuilder, mManager, mParameters);
    
    
    if (layerState == LAYER_ACTIVE_FORCE) {
      continue;
    }

    
    
    mBuilder->IsFixedItem(item, aActiveScrolledRoot);
    if (*aActiveScrolledRoot) {
      return true;
    }
  }
  return false;
}















void
ContainerState::ProcessDisplayItems(const nsDisplayList& aList,
                                    FrameLayerBuilder::Clip& aClip,
                                    uint32_t aFlags,
                                    const nsIFrame* aForceActiveScrolledRoot)
{
  SAMPLE_LABEL("ContainerState", "ProcessDisplayItems");

  const nsIFrame* lastActiveScrolledRoot = nullptr;
  nsPoint topLeft;

  
  
  
  if (aFlags & NO_COMPONENT_ALPHA) {
    if (aForceActiveScrolledRoot) {
      lastActiveScrolledRoot = aForceActiveScrolledRoot;
    } else if (!ChooseActiveScrolledRoot(aList, &lastActiveScrolledRoot)) {
      lastActiveScrolledRoot = mContainerReferenceFrame;
    }

    topLeft = lastActiveScrolledRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
  }

  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    nsDisplayItem::Type type = item->GetType();
    if (type == nsDisplayItem::TYPE_CLIP ||
        type == nsDisplayItem::TYPE_CLIP_ROUNDED_RECT) {
      FrameLayerBuilder::Clip childClip(aClip, item);
      ProcessDisplayItems(*item->GetSameCoordinateSystemChildren(), childClip, aFlags, lastActiveScrolledRoot);
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
    if (layerState == LAYER_INACTIVE &&
        nsDisplayItem::ForceActiveLayers()) {
      layerState = LAYER_ACTIVE;
    }

    bool isFixed;
    bool forceInactive;
    const nsIFrame* activeScrolledRoot;
    if (aFlags & NO_COMPONENT_ALPHA) {
      forceInactive = true;
      activeScrolledRoot = lastActiveScrolledRoot;
      isFixed = mBuilder->IsFixedItem(item, nullptr, activeScrolledRoot);
    } else {
      forceInactive = false;
      isFixed = mBuilder->IsFixedItem(item, &activeScrolledRoot);
      if (activeScrolledRoot != lastActiveScrolledRoot) {
        lastActiveScrolledRoot = activeScrolledRoot;
        topLeft = activeScrolledRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
      }
    }

    
    if (layerState == LAYER_ACTIVE_FORCE ||
        (layerState == LAYER_INACTIVE && !mManager->IsWidgetLayerManager()) ||
        (!forceInactive &&
         (layerState == LAYER_ACTIVE_EMPTY ||
          layerState == LAYER_ACTIVE))) {

      
      
      NS_ASSERTION(layerState != LAYER_ACTIVE_EMPTY ||
                   itemVisibleRect.IsEmpty(),
                   "State is LAYER_ACTIVE_EMPTY but visible rect is not.");

      
      
      
      
      InvalidateForLayerChange(item, nullptr, aClip, topLeft, nullptr);

      
      
      
      
      if (itemVisibleRect.IsEmpty() && layerState != LAYER_ACTIVE_EMPTY) {
        continue;
      }

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager, mParameters);
      if (!ownLayer) {
        continue;
      }

      NS_ASSERTION(!ownLayer->AsThebesLayer(), 
                   "Should never have created a dedicated Thebes layer!");

      nsRect invalid;
      if (item->IsInvalid(invalid)) {
        ownLayer->SetInvalidRectToVisibleRegion();
      }

      
      
      if (!ownLayer->AsContainerLayer()) {
        ownLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      }

      
      
      
      
      ownLayer->SetIsFixedPosition(isFixed && type != nsDisplayItem::TYPE_TRANSFORM);

      
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

      mNewChildLayers.AppendElement(ownLayer);

      



      nsAutoPtr<nsDisplayItemGeometry> dummy;
      mLayerBuilder->AddLayerDisplayItem(ownLayer, item, 
                                         aClip, layerState, 
                                         topLeft, nullptr,
                                         dummy);
    } else {
      ThebesLayerData* data =
        FindThebesLayerFor(item, itemVisibleRect, itemDrawRect, aClip,
                           activeScrolledRoot, topLeft);

      data->mLayer->SetIsFixedPosition(isFixed);

      nsAutoPtr<nsDisplayItemGeometry> geometry(item->AllocateGeometry(mBuilder));

      InvalidateForLayerChange(item, data->mLayer, aClip, topLeft, geometry);

      mLayerBuilder->AddThebesDisplayItem(data->mLayer, item, aClip,
                                          mContainerFrame,
                                          layerState, topLeft,
                                          geometry);

      
      
      data->UpdateCommonClipCount(aClip);
    }
  }
}












static bool ComputeCombinedClip(const FrameLayerBuilder::Clip& aClip,
                                FrameLayerBuilder::Clip* aOldClip,
                                const nsPoint& aShift,
                                nsRegion& aCombined)
{
  if (!aClip.mHaveClipRect ||
      (aOldClip && !aOldClip->mHaveClipRect)) {
    return false;
  }

  if (aOldClip) {
    aCombined = aOldClip->NonRoundedIntersection();
    aCombined.MoveBy(aShift);
    aCombined.Or(aCombined, aClip.NonRoundedIntersection());
  } else {
    aCombined = aClip.NonRoundedIntersection();
  }
  return true;
}

void
ContainerState::InvalidateForLayerChange(nsDisplayItem* aItem, 
                                         Layer* aNewLayer,
                                         const FrameLayerBuilder::Clip& aClip,
                                         const nsPoint& aTopLeft,
                                         nsDisplayItemGeometry *aGeometry)
{
  NS_ASSERTION(aItem->GetUnderlyingFrame(),
               "Display items that render using Thebes must have a frame");
  NS_ASSERTION(aItem->GetPerFrameKey(),
               "Display items that render using Thebes must have a key");
  nsDisplayItemGeometry *oldGeometry = NULL;
  FrameLayerBuilder::Clip* oldClip = NULL;
  nsAutoTArray<nsIFrame*,4> changedFrames;
  bool isInvalid = false;
  Layer* oldLayer = mLayerBuilder->GetOldLayerFor(aItem, &oldGeometry, &oldClip, &changedFrames, &isInvalid);
  if (aNewLayer != oldLayer && oldLayer) {
    
    
    ThebesLayer* t = oldLayer->AsThebesLayer();
    if (t) {
      
      
      
#ifdef DEBUG_INVALIDATIONS
      printf("Display item type %s(%p) changed layers %p to %p!\n", aItem->Name(), aItem->GetUnderlyingFrame(), t, aNewLayer);
#endif
      InvalidatePostTransformRegion(t,
          oldGeometry->ComputeInvalidationRegion(),
          *oldClip,
          mLayerBuilder->GetLastPaintOffset(t));
    }
    if (aNewLayer) {
      ThebesLayer* newThebesLayer = aNewLayer->AsThebesLayer();
      if (newThebesLayer) {
        InvalidatePostTransformRegion(newThebesLayer,
            aGeometry->ComputeInvalidationRegion(),
            aClip,
            GetTranslationForThebesLayer(newThebesLayer));
      }
    }
    aItem->NotifyRenderingChanged();
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
  nsPoint shift = aTopLeft - data->mLastActiveScrolledRootOrigin;
  if (!oldLayer) {
    
    
    combined = aClip.ApplyNonRoundedIntersection(aGeometry->ComputeInvalidationRegion());
#ifdef DEBUG_INVALIDATIONS
    printf("Display item type %s(%p) added to layer %p!\n", aItem->Name(), aItem->GetUnderlyingFrame(), aNewLayer);
#endif
  } else if (isInvalid || (aItem->IsInvalid(invalid) && invalid.IsEmpty())) {
    
    combined = oldClip->ApplyNonRoundedIntersection(oldGeometry->ComputeInvalidationRegion());
    combined.MoveBy(shift);
    combined.Or(combined, aClip.ApplyNonRoundedIntersection(aGeometry->ComputeInvalidationRegion()));
#ifdef DEBUG_INVALIDATIONS
    printf("Display item type %s(%p) (in layer %p) belongs to an invalidated frame!\n", aItem->Name(), aItem->GetUnderlyingFrame(), aNewLayer);
#endif
  } else {
    
    
    oldGeometry->MoveBy(shift);
    aItem->ComputeInvalidationRegion(mBuilder, oldGeometry, &combined);
    oldClip->AddOffsetAndComputeDifference(shift, oldGeometry->ComputeInvalidationRegion(),
                                           aClip, aGeometry->ComputeInvalidationRegion(),
                                           &combined);

    
    combined.Or(combined, invalid);
 
    for (uint32_t i = 0; i < changedFrames.Length(); i++) {
      combined.Or(combined, changedFrames[i]->GetVisualOverflowRect());
    } 

    
    nsRegion clip;
    if (ComputeCombinedClip(aClip, oldClip, shift, clip)) {
      combined.And(combined, clip);
    }
#ifdef DEBUG_INVALIDATIONS
    if (!combined.IsEmpty()) {
      printf("Display item type %s(%p) (in layer %p) changed geometry!\n", aItem->Name(), aItem->GetUnderlyingFrame(), aNewLayer);
    }
#endif
  }
  if (!combined.IsEmpty()) {
    aItem->NotifyRenderingChanged();
    InvalidatePostTransformRegion(newThebesLayer,
        combined.ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
        GetTranslationForThebesLayer(newThebesLayer));
  }
}

void
FrameLayerBuilder::AddThebesDisplayItem(ThebesLayer* aLayer,
                                        nsDisplayItem* aItem,
                                        const Clip& aClip,
                                        nsIFrame* aContainerLayerFrame,
                                        LayerState aLayerState,
                                        const nsPoint& aTopLeft,
                                        nsAutoPtr<nsDisplayItemGeometry> aGeometry)
{
  ThebesDisplayItemLayerUserData* thebesData =
    static_cast<ThebesDisplayItemLayerUserData*>(aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
  nsRefPtr<LayerManager> tempManager;
  nsIntRect intClip;
  bool hasClip = false;
  if (aLayerState != LAYER_NONE) {
    DisplayItemData *data = GetDisplayItemDataForManager(aItem, aLayer->Manager());
    if (data) {
      tempManager = data->mInactiveManager;
    }
    if (!tempManager) {
      tempManager = new BasicLayerManager();
    }

    
    nsRegion clip;
    FrameLayerBuilder::Clip* oldClip = nullptr;
    GetOldLayerFor(aItem, nullptr, &oldClip);
    hasClip = ComputeCombinedClip(aClip, oldClip, 
                                  aTopLeft - thebesData->mLastActiveScrolledRootOrigin,
                                  clip);

    if (hasClip) {
      intClip = clip.GetBounds().ScaleToOutsidePixels(thebesData->mXScale, 
                                                      thebesData->mYScale, 
                                                      thebesData->mAppUnitsPerDevPixel);
    }
  }

  AddLayerDisplayItem(aLayer, aItem, aClip, aLayerState, aTopLeft, tempManager, aGeometry);

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

      
      
      if (mRetainingManager) {
#ifdef DEBUG_DISPLAY_ITEM_DATA
        LayerManagerData* parentLmd = static_cast<LayerManagerData*>
          (aLayer->Manager()->GetUserData(&gLayerManagerUserData));
        LayerManagerData* lmd = static_cast<LayerManagerData*>
          (tempManager->GetUserData(&gLayerManagerUserData));
        lmd->mParent = parentLmd;
#endif
        layerBuilder->StoreDataForFrame(aItem, layer, LAYER_ACTIVE);
      }

      tempManager->SetRoot(layer);
      layerBuilder->WillEndTransaction();

      nsIntPoint offset = GetLastPaintOffset(aLayer) - GetTranslationForThebesLayer(aLayer);
      props->MoveBy(-offset);
      nsIntRegion invalid = props->ComputeDifferences(layer, nullptr);
      if (aLayerState == LAYER_SVG_EFFECTS) {
        invalid = nsSVGIntegrationUtils::AdjustInvalidAreaForSVGEffects(aItem->GetUnderlyingFrame(),
                                                                        aItem->ToReferenceFrame(),
                                                                        invalid.GetBounds());
      }
      if (!invalid.IsEmpty()) {
#ifdef DEBUG_INVALIDATIONS
        printf("Inactive LayerManager(%p) for display item %s(%p) has an invalid region - invalidating layer %p\n", tempManager.get(), aItem->Name(), aItem->GetUnderlyingFrame(), aLayer);
#endif
        if (hasClip) {
          invalid.And(invalid, intClip);
        }

        invalid.ScaleRoundOut(thebesData->mXScale, thebesData->mYScale);
        InvalidatePostTransformRegion(aLayer, invalid,
                                      GetTranslationForThebesLayer(aLayer));
      }
    }
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem, aClip,
                                                     mContainerLayerGeneration));
    cdi->mInactiveLayerManager = tempManager;
  }
}

FrameLayerBuilder::DisplayItemData*
FrameLayerBuilder::StoreDataForFrame(nsDisplayItem* aItem, Layer* aLayer, LayerState aState)
{
  DisplayItemData* oldData = GetDisplayItemDataForManager(aItem, mRetainingManager);
  if (oldData) {
    if (!oldData->mUsed) {
      oldData->UpdateContents(aLayer, aState, mContainerLayerGeneration, aItem);
    }
    return oldData;
  }
  
  LayerManagerData* lmd = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));
  
  nsRefPtr<DisplayItemData> data = 
    new DisplayItemData(lmd, aItem->GetPerFrameKey(),
                        aLayer, aState, mContainerLayerGeneration);

  data->AddFrame(aItem->GetUnderlyingFrame());

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);

  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    data->AddFrame(mergedFrames[i]);
  }

  lmd->mDisplayItems.PutEntry(data);
  return data;
}

void
FrameLayerBuilder::StoreDataForFrame(nsIFrame* aFrame,
                                     uint32_t aDisplayItemKey,
                                     Layer* aLayer,
                                     LayerState aState)
{
  DisplayItemData* oldData = GetDisplayItemData(aFrame, aDisplayItemKey);
  if (oldData && oldData->mFrameList.Length() == 1) {
    oldData->UpdateContents(aLayer, aState, mContainerLayerGeneration);
    return;
  }
  
  LayerManagerData* lmd = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));

  nsRefPtr<DisplayItemData> data =
    new DisplayItemData(lmd, aDisplayItemKey, aLayer,
                        aState, mContainerLayerGeneration);

  data->AddFrame(aFrame);

  lmd->mDisplayItems.PutEntry(data);
}

FrameLayerBuilder::ClippedDisplayItem::~ClippedDisplayItem()
{
  if (mInactiveLayerManager) {
    
    
    
    BasicLayerManager* basic = static_cast<BasicLayerManager*>(mInactiveLayerManager.get());
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
                                       LayerManager* aManager,
                                       nsAutoPtr<nsDisplayItemGeometry> aGeometry)
{
  if (aLayer->Manager() != mRetainingManager)
    return;

  DisplayItemData *data = StoreDataForFrame(aItem, aLayer, aLayerState);
  ThebesLayer *t = aLayer->AsThebesLayer();
  if (t) {
    data->mGeometry = aGeometry;
    data->mClip = aClip;
  }
  data->mInactiveManager = aManager;
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
  gfxMatrix transform2d;
  if (aContainerFrame &&
      aState == LAYER_INACTIVE &&
      (!aTransform || (aTransform->Is2D(&transform2d) &&
                       !transform2d.HasNonTranslation()))) {
    
    
    
    
    
    
    
    nsPoint appUnitOffset = aDisplayListBuilder->ToReferenceFrame(aContainerFrame);
    nscoord appUnitsPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
    offset = nsIntPoint(
        int32_t(NSAppUnitsToDoublePixels(appUnitOffset.x, appUnitsPerDevPixel)*aIncomingScale.mXScale),
        int32_t(NSAppUnitsToDoublePixels(appUnitOffset.y, appUnitsPerDevPixel)*aIncomingScale.mYScale));
  }
  transform = transform * gfx3DMatrix::Translation(offset.x + aIncomingScale.mOffset.x, offset.y + aIncomingScale.mOffset.y, 0);


  bool canDraw2D = transform.CanDraw2D(&transform2d);
  gfxSize scale;
  bool isRetained = aLayer->Manager()->IsWidgetLayerManager();
  
  
  
  if (canDraw2D && isRetained) {
    
    
    if (aContainerFrame->GetContent() &&
        nsLayoutUtils::HasAnimationsForCompositor(
          aContainerFrame->GetContent(), eCSSProperty_transform)) {
      scale = nsLayoutUtils::GetMaximumAnimatedScale(aContainerFrame->GetContent());
    } else {
      
      scale = transform2d.ScaleFactors(true);
      
      
      
      
      
      
      gfxMatrix frameTransform;
      if (aContainerFrame->AreLayersMarkedActive(nsChangeHint_UpdateTransformLayer) &&
          aTransform &&
          (!aTransform->Is2D(&frameTransform) || frameTransform.HasNonTranslationOrFlip())) {
        
        
        bool clamp = true;
        gfxMatrix oldFrameTransform2d;
        if (aLayer->GetBaseTransform().Is2D(&oldFrameTransform2d)) {
          gfxSize oldScale = oldFrameTransform2d.ScaleFactors(true);
          if (oldScale == scale || oldScale == gfxSize(1.0, 1.0)) {
            clamp = false;
          }
        }
        if (clamp) {
          scale.width = gfxUtils::ClampToScaleFactor(scale.width);
          scale.height = gfxUtils::ClampToScaleFactor(scale.height);
        }
      } else {
        
      }
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
  aLayer->SetInheritedScale(aIncomingScale.mXScale,
                            aIncomingScale.mYScale);

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
FrameLayerBuilder::RestoreDisplayItemData(nsRefPtrHashKey<DisplayItemData>* aEntry, void* aUserArg)
{
  DisplayItemData* data = aEntry->GetKey();
  uint32_t *generation = static_cast<uint32_t*>(aUserArg);

  if (data->mUsed && data->mContainerLayerGeneration >= *generation) {
    return PL_DHASH_REMOVE;
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
    aContainerItem ? aContainerItem->GetPerFrameKey() : nsDisplayItem::TYPE_ZERO;
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

  LayerState state = aContainerItem ? aContainerItem->GetLayerState(aBuilder, aManager, aParameters) : LAYER_ACTIVE;
  if (state == LAYER_INACTIVE &&
      nsDisplayItem::ForceActiveLayers()) {
    state = LAYER_ACTIVE;
  }

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
    if (aContainerItem) {
      StoreDataForFrame(aContainerItem, containerLayer, LAYER_ACTIVE);
    } else {
      StoreDataForFrame(aContainerFrame, containerDisplayItemKey, containerLayer, LAYER_ACTIVE);
    }
  }
  
  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));

  nsRect bounds;
  nsIntRect pixBounds;
  int32_t appUnitsPerDevPixel;
  uint32_t stateFlags = 0;
  if ((aContainerFrame->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) &&
      mRetainingManager && !mRetainingManager->AreComponentAlphaLayersEnabled()) {
    stateFlags = ContainerState::NO_COMPONENT_ALPHA;
  }
  uint32_t flags;
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
      data->mDisplayItems.EnumerateEntries(RestoreDisplayItemData,
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
  NS_ASSERTION(aItem->GetUnderlyingFrame(),
               "Can only call GetLeafLayerFor on items that have a frame");
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
  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
  if (array) {
    for (uint32_t i = 0; i < array->Length(); i++) {
      array->ElementAt(i)->mParent->mInvalidateAllLayers = true;
    }
  }
}


Layer*
FrameLayerBuilder::GetDedicatedLayer(nsIFrame* aFrame, uint32_t aDisplayItemKey)
{
  
  
  

  nsTArray<DisplayItemData*> *array = 
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
  if (array) {
    for (uint32_t i = 0; i < array->Length(); i++) {
      DisplayItemData *element = array->ElementAt(i);
      if (!element->mParent->mLayerManager->IsWidgetLayerManager()) {
        continue;
      }
      if (element->mDisplayItemKey == aDisplayItemKey) {
        if (element->mOptLayer) {
          return element->mOptLayer;
        }

        Layer* layer = element->mLayer;
        if (!layer->HasUserData(&gColorLayerUserData) &&
            !layer->HasUserData(&gImageLayerUserData) &&
            !layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
          return layer;
        }
      }
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
  if (aFrame != aAncestorWithScale) {
    
    transform = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestorWithScale)*transform;
  }
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

    if (nsLayoutUtils::IsPopup(f)) {
      
      
      
      break;
    }
  
    nsTArray<DisplayItemData*> *array = 
      reinterpret_cast<nsTArray<DisplayItemData*>*>(aFrame->Properties().Get(LayerManagerDataProperty()));
    if (!array) {
      continue;
    }

    for (uint32_t i = 0; i < array->Length(); i++) {
      Layer* layer = array->ElementAt(i)->mLayer;
      ContainerLayer* container = layer->AsContainerLayer();
      if (!container ||
          !layer->Manager()->IsWidgetLayerManager()) {
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
  NS_ASSERTION(layerBuilder, "Unexpectedly null layer builder!");

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

    if (cdi->mInactiveLayerManager) {
      PaintInactiveLayer(builder, cdi->mInactiveLayerManager, cdi->mItem, aContext, rc);
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
  aEnd = std::min<uint32_t>(aEnd, mRoundedClipRects.Length());

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
  aEnd = std::min<uint32_t>(aEnd, mRoundedClipRects.Length());

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
  NS_ASSERTION(mHaveClipRect, "Must have a clip rect!");
  nsRect result = mClipRect;
  for (uint32_t i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    result.IntersectRect(result, mRoundedClipRects[i].mRect);
  }
  return result;
}

nsRect
FrameLayerBuilder::Clip::ApplyNonRoundedIntersection(const nsRect& aRect) const
{
  if (!mHaveClipRect) {
    return aRect;
  }

  nsRect result = aRect.Intersect(mClipRect);
  for (uint32_t i = 0, iEnd = mRoundedClipRects.Length();
       i < iEnd; ++i) {
    result.Intersect(mRoundedClipRects[i].mRect);
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
  nsIntSize surfaceSize(std::min<int32_t>(boundingRect.Width(), maxSize),
                        std::min<int32_t>(boundingRect.Height(), maxSize));

  
  
  
  
  gfxMatrix maskTransform;
  maskTransform.Scale(float(surfaceSize.width)/float(boundingRect.Width()),
                      float(surfaceSize.height)/float(boundingRect.Height()));
  maskTransform.Translate(-boundingRect.TopLeft());
  
  gfxMatrix imageTransform = maskTransform;
  imageTransform.Scale(mParameters.mXScale, mParameters.mYScale);

  nsAutoPtr<MaskLayerImageCache::MaskLayerImageKey> newKey(
    new MaskLayerImageCache::MaskLayerImageKey());

  
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
