




#include "mozilla/DebugOnly.h"

#include "FrameLayerBuilder.h"

#include "mozilla/gfx/Matrix.h"
#include "nsDisplayList.h"
#include "nsPresContext.h"
#include "nsLayoutUtils.h"
#include "Layers.h"
#include "BasicLayers.h"
#include "gfxUtils.h"
#include "nsRenderingContext.h"
#include "MaskLayerImageCache.h"
#include "nsIScrollableFrame.h"
#include "nsPrintfCString.h"
#include "LayerTreeInvalidation.h"
#include "nsSVGIntegrationUtils.h"
#include "ImageContainer.h"
#include "ActiveLayerTracker.h"
#include "gfx2DGlue.h"

#include "GeckoProfiler.h"
#include "mozilla/gfx/Tools.h"
#include "mozilla/gfx/2D.h"
#include "gfxPrefs.h"

#include <algorithm>

using namespace mozilla::layers;
using namespace mozilla::gfx;

namespace mozilla {

class ContainerState;

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
  mClip = DisplayItemClip();
  mUsed = true;

  if (!aItem) {
    return;
  }

  nsAutoTArray<nsIFrame*, 4> copy(mFrameList);
  if (!copy.RemoveElement(aItem->Frame())) {
    AddFrame(aItem->Frame());
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

static nsIFrame* sDestroyedFrame = nullptr;
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
  if (!aOut.RemoveElement(aOther->Frame())) {
    added.AppendElement(aOther->Frame());
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
  }
  ~LayerManagerData() {
    MOZ_COUNT_DTOR(LayerManagerData);
  }

#ifdef DEBUG_DISPLAY_ITEM_DATA
  void Dump(const char *aPrefix = "") {
    printf_stderr("%sLayerManagerData %p\n", aPrefix, this);
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


static MaskLayerImageCache* gMaskLayerImageCache = nullptr;

static inline MaskLayerImageCache* GetMaskLayerImageCache()
{
  if (!gMaskLayerImageCache) {
    gMaskLayerImageCache = new MaskLayerImageCache();
  }

  return gMaskLayerImageCache;
}










class ThebesLayerData {
public:
  ThebesLayerData() :
    mAnimatedGeometryRoot(nullptr),
    mFixedPosFrameForLayerData(nullptr),
    mReferenceFrame(nullptr),
    mLayer(nullptr),
    mIsSolidColorInVisibleRegion(false),
    mSingleItemFixedToViewport(false),
    mNeedComponentAlpha(false),
    mForceTransparentSurface(false),
    mImage(nullptr),
    mCommonClipCount(-1),
    mAllDrawingAbove(false)
  {}
  












  void Accumulate(ContainerState* aState,
                  nsDisplayItem* aItem,
                  const nsIntRect& aVisibleRect,
                  const nsIntRect& aDrawRect,
                  const DisplayItemClip& aClip);
  const nsIFrame* GetAnimatedGeometryRoot() { return mAnimatedGeometryRoot; }

  



  void AccumulateEventRegions(const nsRegion& aHitRegion,
                              const nsRegion& aMaybeHitRegion,
                              const nsRegion& aDispatchToContentHitRegion)
  {
    mHitRegion.Or(mHitRegion, aHitRegion);
    mMaybeHitRegion.Or(mMaybeHitRegion, aMaybeHitRegion);
    mDispatchToContentHitRegion.Or(mDispatchToContentHitRegion, aDispatchToContentHitRegion);
  }

  




  already_AddRefed<ImageContainer> CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder);

  void AddDrawAboveRegion(const nsIntRegion& aAbove)
  {
    if (!mAllDrawingAbove) {
      mDrawAboveRegion.Or(mDrawAboveRegion, aAbove);
      mDrawAboveRegion.SimplifyOutward(4);
    }
  }

  void AddVisibleAboveRegion(const nsIntRegion& aAbove)
  {
    if (!mAllDrawingAbove) {
      mVisibleAboveRegion.Or(mVisibleAboveRegion, aAbove);
      mVisibleAboveRegion.SimplifyOutward(4);
    }
  }

  void CopyAboveRegion(ThebesLayerData* aOther)
  {
    if (aOther->mAllDrawingAbove || mAllDrawingAbove) {
      SetAllDrawingAbove();
    } else {
      mVisibleAboveRegion.Or(mVisibleAboveRegion, aOther->mVisibleAboveRegion);
      mVisibleAboveRegion.Or(mVisibleAboveRegion, aOther->mVisibleRegion);
      mVisibleAboveRegion.SimplifyOutward(4);
      mDrawAboveRegion.Or(mDrawAboveRegion, aOther->mDrawAboveRegion);
      mDrawAboveRegion.Or(mDrawAboveRegion, aOther->mDrawRegion);
      mDrawAboveRegion.SimplifyOutward(4);
   }
  }

  void SetAllDrawingAbove()
  {
    mAllDrawingAbove = true;
    mDrawAboveRegion.SetEmpty();
    mVisibleAboveRegion.SetEmpty();
  }

  bool DrawAboveRegionIntersects(const nsIntRect& aRect)
  {
    return mAllDrawingAbove || mDrawAboveRegion.Intersects(aRect);
  }

  bool DrawRegionIntersects(const nsIntRect& aRect)
  {
    return IsSubjectToAsyncTransforms() || mDrawRegion.Intersects(aRect);
  }

  bool IntersectsVisibleAboveRegion(const nsIntRegion& aVisibleRegion)
  {
    if (mAllDrawingAbove) {
      return true;
    }
    nsIntRegion visibleAboveIntersection;
    visibleAboveIntersection.And(mVisibleAboveRegion, aVisibleRegion);
    if (visibleAboveIntersection.IsEmpty()) {
      return false;
    }
    return true;
  }

  bool IsSubjectToAsyncTransforms()
  {
    return mFixedPosFrameForLayerData != nullptr;
  }

  




  nsIntRegion  mVisibleRegion;
  





  nsIntRegion  mDrawRegion;
  



  nsIntRegion  mOpaqueRegion;
  


  nsRegion  mHitRegion;
  


  nsRegion  mMaybeHitRegion;
  


  nsRegion  mDispatchToContentHitRegion;
  




  const nsIFrame* mAnimatedGeometryRoot;
  




  const nsIFrame* mFixedPosFrameForLayerData;
  const nsIFrame* mReferenceFrame;
  ThebesLayer* mLayer;
  



  nscolor      mSolidColor;
  


  bool mIsSolidColorInVisibleRegion;
  



  bool mSingleItemFixedToViewport;
  



  bool mNeedComponentAlpha;
  





  bool mForceTransparentSurface;

  



  nsDisplayImageContainer* mImage;
  







  DisplayItemClip mItemClip;
  





  int32_t mCommonClipCount;
  





  void UpdateCommonClipCount(const DisplayItemClip& aCurrentClip);

private:
  






  nsIntRegion  mVisibleAboveRegion;
  







  nsIntRegion  mDrawAboveRegion;
  



  bool mAllDrawingAbove;
};





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 FrameLayerBuilder* aLayerBuilder,
                 nsIFrame* aContainerFrame,
                 nsDisplayItem* aContainerItem,
                 ContainerLayer* aContainerLayer,
                 const ContainerLayerParameters& aParameters) :
    mBuilder(aBuilder), mManager(aManager),
    mLayerBuilder(aLayerBuilder),
    mContainerFrame(aContainerFrame),
    mContainerLayer(aContainerLayer),
    mParameters(aParameters),
    mNextFreeRecycledThebesLayer(0)
  {
    nsPresContext* presContext = aContainerFrame->PresContext();
    mAppUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    mContainerReferenceFrame =
      const_cast<nsIFrame*>(aContainerItem ? aContainerItem->ReferenceFrameForChildren() :
                                             mBuilder->FindReferenceFrameFor(mContainerFrame));
    mContainerAnimatedGeometryRoot = aContainerItem
      ? nsLayoutUtils::GetAnimatedGeometryRootFor(aContainerItem, aBuilder)
      : mContainerReferenceFrame;
    
    
    
    mSnappingEnabled = aManager->IsSnappingEffectiveTransforms() &&
      !mParameters.AllowResidualTranslation();
    CollectOldLayers();
  }

  enum ProcessDisplayItemsFlags {
    NO_COMPONENT_ALPHA = 0x01,
  };

  



  void ProcessDisplayItems(const nsDisplayList& aList, uint32_t aFlags);
  







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
  nsIntRect ScaleToOutsidePixels(const nsRect& aRect, bool aSnap = false)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleToNearestPixels(aRect);
    }
    return aRect.ScaleToOutsidePixels(mParameters.mXScale, mParameters.mYScale,
                                      mAppUnitsPerDevPixel);
  }
  nsIntRect ScaleToInsidePixels(const nsRect& aRect, bool aSnap = false)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleToNearestPixels(aRect);
    }
    return aRect.ScaleToInsidePixels(mParameters.mXScale, mParameters.mYScale,
                                     mAppUnitsPerDevPixel);
  }

  nsIntRegion ScaleRegionToInsidePixels(const nsRegion& aRegion, bool aSnap = false)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleRegionToNearestPixels(aRegion);
    }
    return aRegion.ScaleToInsidePixels(mParameters.mXScale, mParameters.mYScale,
                                        mAppUnitsPerDevPixel);
  }

  nsIntRegion ScaleRegionToOutsidePixels(const nsRegion& aRegion, bool aSnap = false)
  {
    if (aSnap && mSnappingEnabled) {
      return ScaleRegionToNearestPixels(aRegion);
    }
    return aRegion.ScaleToOutsidePixels(mParameters.mXScale, mParameters.mYScale,
                                        mAppUnitsPerDevPixel);
  }

protected:
  friend class ThebesLayerData;

  





  already_AddRefed<ThebesLayer> CreateOrRecycleThebesLayer(const nsIFrame* aAnimatedGeometryRoot,
                                                           const nsIFrame *aReferenceFrame,
                                                           const nsPoint& aTopLeft);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer(ThebesLayer* aThebes);
  



  already_AddRefed<ImageLayer> CreateOrRecycleImageLayer(ThebesLayer* aThebes);
  




  already_AddRefed<ImageLayer> CreateOrRecycleMaskImageLayerFor(Layer* aLayer);
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem,
                                Layer* aNewLayer,
                                const DisplayItemClip& aClip,
                                const nsPoint& aTopLeft,
                                nsDisplayItemGeometry *aGeometry);
  





  nscolor FindOpaqueBackgroundColorFor(int32_t aThebesLayerIndex);
  










  const nsIFrame* FindFixedPosFrameForLayerData(const nsIFrame* aAnimatedGeometryRoot,
                                                bool aDisplayItemFixedToViewport);
  void AdjustLayerDataForFixedPositioning(const nsIFrame* aFixedPosFrame,
                                          const nsIntRegion& aDrawRegion,
                                          nsIntRegion* aVisibleRegion,
                                          bool* aIsSolidColorInVisibleRegion = nullptr);
  


  void SetFixedPositionLayerData(Layer* aLayer,
                                 const nsIFrame* aFixedPosFrame);
  




  void PopThebesLayerData();
  



















  ThebesLayerData* FindThebesLayerFor(nsDisplayItem* aItem,
                                      const nsIntRect& aVisibleRect,
                                      const nsIFrame* aAnimatedGeometryRoot,
                                      const nsPoint& aTopLeft,
                                      bool aShouldFixToViewport);
  ThebesLayerData* GetTopThebesLayerData()
  {
    return mThebesLayerDataStack.IsEmpty() ? nullptr
        : mThebesLayerDataStack[mThebesLayerDataStack.Length() - 1].get();
  }

  








  void SetupMaskLayer(Layer *aLayer, const DisplayItemClip& aClip,
                      uint32_t aRoundedRectClipCount = UINT32_MAX);

  bool ChooseAnimatedGeometryRoot(const nsDisplayList& aList,
                                  const nsIFrame **aAnimatedGeometryRoot);

  nsDisplayListBuilder*            mBuilder;
  LayerManager*                    mManager;
  FrameLayerBuilder*               mLayerBuilder;
  nsIFrame*                        mContainerFrame;
  nsIFrame*                        mContainerReferenceFrame;
  const nsIFrame*                  mContainerAnimatedGeometryRoot;
  ContainerLayer*                  mContainerLayer;
  ContainerLayerParameters         mParameters;
  



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
    mAnimatedGeometryRootPosition(0, 0) {}

  




  uint32_t mMaskClipCount;

  



  nscolor mForcedBackgroundColor;

  


  float mXScale, mYScale;

  


  nscoord mAppUnitsPerDevPixel;

  





  nsIntPoint mTranslation;

  









  gfxPoint mAnimatedGeometryRootPosition;

  nsIntRegion mRegionToInvalidate;

  
  
  
  nsPoint mLastAnimatedGeometryRootOrigin;
  nsPoint mAnimatedGeometryRootOrigin;

  nsRefPtr<ColorLayer> mColorLayer;
  nsRefPtr<ImageLayer> mImageLayer;
};




struct MaskLayerUserData : public LayerUserData
{
  MaskLayerUserData()
    : mScaleX(-1.0f)
    , mScaleY(-1.0f)
    , mAppUnitsPerDevPixel(-1)
  { }

  bool
  operator== (const MaskLayerUserData& aOther) const
  {
    return mRoundedClipRects == aOther.mRoundedClipRects &&
           mScaleX == aOther.mScaleX &&
           mScaleY == aOther.mScaleY &&
           mOffset == aOther.mOffset &&
           mAppUnitsPerDevPixel == aOther.mAppUnitsPerDevPixel;
  }

  nsRefPtr<const MaskLayerImageCache::MaskLayerImageKey> mImageKey;
  
  
  nsTArray<DisplayItemClip::RoundedRect> mRoundedClipRects;
  
  float mScaleX, mScaleY;
  
  nsIntPoint mOffset;
  int32_t mAppUnitsPerDevPixel;
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

 void
FrameLayerBuilder::Shutdown()
{
  if (gMaskLayerImageCache) {
    delete gMaskLayerImageCache;
    gMaskLayerImageCache = nullptr;
  }
}

void
FrameLayerBuilder::Init(nsDisplayListBuilder* aBuilder, LayerManager* aManager,
                        ThebesLayerData* aLayerData)
{
  mDisplayListBuilder = aBuilder;
  mRootPresContext = aBuilder->RootReferenceFrame()->PresContext()->GetRootPresContext();
  if (mRootPresContext) {
    mInitialDOMGeneration = mRootPresContext->GetDOMGeneration();
  }
  mContainingThebesLayer = aLayerData;
  aManager->SetUserData(&gLayerManagerLayerBuilder, this);
}

void
FrameLayerBuilder::FlashPaint(gfxContext *aContext)
{
  float r = float(rand()) / RAND_MAX;
  float g = float(rand()) / RAND_MAX;
  float b = float(rand()) / RAND_MAX;
  aContext->SetColor(gfxRGBA(r, g, b, 0.4));
  aContext->Paint();
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
#ifdef MOZ_DUMP_PAINTING
  if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
    nsAutoCString str;
    AppendToString(str, rgn);
    printf_stderr("Invalidating layer %p: %s\n", aLayer, str.get());
  }
#endif
}

static void
InvalidatePostTransformRegion(ThebesLayer* aLayer, const nsRect& aRect,
                              const DisplayItemClip& aClip,
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
    printf_stderr("Removing frame %p - dumping display data\n", aFrame);
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
  sDestroyedFrame = nullptr;
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
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("Invalidating unused display item (%i) belonging to frame %p from layer %p\n", data->mDisplayItemKey, data->mFrameList[0], t);
      }
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

  printf_stderr("%s", str.get());

  if (data->mInactiveManager) {
    prefix += "  ";
    printf_stderr("%sDumping inactive layer info:\n", prefix.get());
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
    reinterpret_cast<nsTArray<DisplayItemData*>*>(aItem->Frame()->Properties().Get(LayerManagerDataProperty()));
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
                                  DisplayItemClip** aOldClip,
                                  nsTArray<nsIFrame*>* aChangedFrames,
                                  bool *aIsInvalid)
{
  uint32_t key = aItem->GetPerFrameKey();
  nsIFrame* frame = aItem->Frame();

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
    result->SetDisallowBigImage(true);
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
ResetScrollPositionForLayerPixelAlignment(const nsIFrame* aAnimatedGeometryRoot)
{
  nsIScrollableFrame* sf = nsLayoutUtils::GetScrollableFrameFor(aAnimatedGeometryRoot);
  if (sf) {
    sf->ResetScrollPositionForLayerPixelAlignment();
  }
}

static void
InvalidateEntireThebesLayer(ThebesLayer* aLayer, const nsIFrame* aAnimatedGeometryRoot)
{
#ifdef MOZ_DUMP_PAINTING
  if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
    printf_stderr("Invalidating entire layer %p\n", aLayer);
  }
#endif
  nsIntRect invalidate = aLayer->GetValidRegion().GetBounds();
  aLayer->InvalidateRegion(invalidate);
  aLayer->SetInvalidRectToVisibleRegion();
  ResetScrollPositionForLayerPixelAlignment(aAnimatedGeometryRoot);
}

already_AddRefed<ThebesLayer>
ContainerState::CreateOrRecycleThebesLayer(const nsIFrame* aAnimatedGeometryRoot,
                                           const nsIFrame* aReferenceFrame,
                                           const nsPoint& aTopLeft)
{
  
  nsRefPtr<ThebesLayer> layer;
  ThebesDisplayItemLayerUserData* data;
  bool layerRecycled = false;
#ifndef MOZ_ANDROID_OMTC
  bool didResetScrollPositionForLayerPixelAlignment = false;
#endif

  
  
  LayerManager::ThebesLayerCreationHint creationHint = LayerManager::NONE;
  nsIFrame* animatedGeometryRootParent = aAnimatedGeometryRoot->GetParent();
  if (animatedGeometryRootParent &&
      animatedGeometryRootParent->GetType() == nsGkAtoms::scrollFrame) {
    creationHint = LayerManager::SCROLLABLE;
  }

  if (mNextFreeRecycledThebesLayer < mRecycledThebesLayers.Length()) {
    
    layer = mRecycledThebesLayers[mNextFreeRecycledThebesLayer];
    ++mNextFreeRecycledThebesLayer;

    
    
    if (mManager->IsOptimizedFor(layer->AsThebesLayer(), creationHint)) {
      layerRecycled = true;

      
      
      layer->SetMaskLayer(nullptr);

      data = static_cast<ThebesDisplayItemLayerUserData*>
          (layer->GetUserData(&gThebesDisplayItemLayerUserData));
      NS_ASSERTION(data, "Recycled ThebesLayers must have user data");

      
      
      
      
      
      
      
      
      if (!FuzzyEqual(data->mXScale, mParameters.mXScale, 0.00001f) ||
          !FuzzyEqual(data->mYScale, mParameters.mYScale, 0.00001f) ||
          data->mAppUnitsPerDevPixel != mAppUnitsPerDevPixel) {
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("Recycled layer %p changed scale\n", layer.get());
      }
#endif
        InvalidateEntireThebesLayer(layer, aAnimatedGeometryRoot);
#ifndef MOZ_ANDROID_OMTC
        didResetScrollPositionForLayerPixelAlignment = true;
#endif
      }
      if (!data->mRegionToInvalidate.IsEmpty()) {
#ifdef MOZ_DUMP_PAINTING
        if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
          printf_stderr("Invalidating deleted frame content from layer %p\n", layer.get());
        }
#endif
        layer->InvalidateRegion(data->mRegionToInvalidate);
#ifdef MOZ_DUMP_PAINTING
        if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
          nsAutoCString str;
          AppendToString(str, data->mRegionToInvalidate);
          printf_stderr("Invalidating layer %p: %s\n", layer.get(), str.get());
        }
#endif
        data->mRegionToInvalidate.SetEmpty();
      }

      
      
      
    }
  }

  if (!layerRecycled) {
    
    layer = mManager->CreateThebesLayerWithHint(creationHint);
    if (!layer)
      return nullptr;
    
    data = new ThebesDisplayItemLayerUserData();
    layer->SetUserData(&gThebesDisplayItemLayerUserData, data);
    ResetScrollPositionForLayerPixelAlignment(aAnimatedGeometryRoot);
#ifndef MOZ_ANDROID_OMTC
    didResetScrollPositionForLayerPixelAlignment = true;
#endif
  }
  data->mXScale = mParameters.mXScale;
  data->mYScale = mParameters.mYScale;
  data->mLastAnimatedGeometryRootOrigin = data->mAnimatedGeometryRootOrigin;
  data->mAnimatedGeometryRootOrigin = aTopLeft;
  data->mAppUnitsPerDevPixel = mAppUnitsPerDevPixel;
  layer->SetAllowResidualTranslation(mParameters.AllowResidualTranslation());

  mLayerBuilder->SaveLastPaintOffset(layer);

  
  
  nsPoint offset = aAnimatedGeometryRoot->GetOffsetToCrossDoc(aReferenceFrame);
  nscoord appUnitsPerDevPixel = aAnimatedGeometryRoot->PresContext()->AppUnitsPerDevPixel();
  gfxPoint scaledOffset(
      NSAppUnitsToDoublePixels(offset.x, appUnitsPerDevPixel)*mParameters.mXScale,
      NSAppUnitsToDoublePixels(offset.y, appUnitsPerDevPixel)*mParameters.mYScale);
  
  
  nsIntPoint pixOffset(RoundToMatchResidual(scaledOffset.x, data->mAnimatedGeometryRootPosition.x),
                       RoundToMatchResidual(scaledOffset.y, data->mAnimatedGeometryRootPosition.y));
  data->mTranslation = pixOffset;
  pixOffset += mParameters.mOffset;
  Matrix matrix;
  matrix.Translate(pixOffset.x, pixOffset.y);
  layer->SetBaseTransform(Matrix4x4::From2D(matrix));

  
#ifndef MOZ_ANDROID_OMTC
  
  
  gfxPoint animatedGeometryRootTopLeft = scaledOffset - ThebesPoint(matrix.GetTranslation()) + mParameters.mOffset;
  
  
  
  if (!animatedGeometryRootTopLeft.WithinEpsilonOf(data->mAnimatedGeometryRootPosition, SUBPIXEL_OFFSET_EPSILON)) {
    data->mAnimatedGeometryRootPosition = animatedGeometryRootTopLeft;
    InvalidateEntireThebesLayer(layer, aAnimatedGeometryRoot);
  } else if (didResetScrollPositionForLayerPixelAlignment) {
    data->mAnimatedGeometryRootPosition = animatedGeometryRootTopLeft;
  }
#endif

  return layer.forget();
}

#if defined(DEBUG) || defined(MOZ_DUMP_PAINTING)



static int32_t
AppUnitsPerDevPixel(nsDisplayItem* aItem)
{
  
  
  
  
  if (aItem->GetType() == nsDisplayItem::TYPE_ZOOM) {
    return static_cast<nsDisplayZoom*>(aItem)->GetParentAppUnitsPerDevPixel();
  }
  return aItem->Frame()->PresContext()->AppUnitsPerDevPixel();
}
#endif











static void
SetVisibleRegionForLayer(Layer* aLayer, const nsIntRegion& aLayerVisibleRegion,
                         const nsIntRect& aRestrictToRect)
{
  gfx3DMatrix transform;
  To3DMatrix(aLayer->GetTransform(), transform);

  
  
  gfxRect itemVisible(aRestrictToRect.x, aRestrictToRect.y,
                      aRestrictToRect.width, aRestrictToRect.height);
  nsIntRect childBounds = aLayerVisibleRegion.GetBounds();
  gfxRect childGfxBounds(childBounds.x, childBounds.y,
                         childBounds.width, childBounds.height);
  gfxRect layerVisible = transform.Inverse().ProjectRectBounds(itemVisible);
  layerVisible = layerVisible.Intersect(childGfxBounds);
  layerVisible.RoundOut();

  nsIntRect visibleRect;
  if (!gfxUtils::GfxRectToIntRect(layerVisible, &visibleRect)) {
    aLayer->SetVisibleRegion(nsIntRegion());
  } else {
    nsIntRegion rgn;
    rgn.And(aLayerVisibleRegion, visibleRect);
    aLayer->SetVisibleRegion(rgn);
  }
}

nscolor
ContainerState::FindOpaqueBackgroundColorFor(int32_t aThebesLayerIndex)
{
  ThebesLayerData* target = mThebesLayerDataStack[aThebesLayerIndex];
  for (int32_t i = aThebesLayerIndex - 1; i >= 0; --i) {
    ThebesLayerData* candidate = mThebesLayerDataStack[i];
    if (candidate->IntersectsVisibleAboveRegion(target->mVisibleRegion)) {
      
      
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

      if (item->IsInvisibleInRect(appUnitRect)) {
        continue;
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
ThebesLayerData::UpdateCommonClipCount(
    const DisplayItemClip& aCurrentClip)
{
  if (mCommonClipCount >= 0) {
    mCommonClipCount = mItemClip.GetCommonRoundedRectCount(aCurrentClip, mCommonClipCount);
  } else {
    
    mCommonClipCount = aCurrentClip.GetRoundedRectCount();
  }
}

already_AddRefed<ImageContainer>
ThebesLayerData::CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder)
{
  if (!mImage) {
    return nullptr;
  }

  return mImage->GetContainer(mLayer->Manager(), aBuilder);
}

const nsIFrame*
ContainerState::FindFixedPosFrameForLayerData(const nsIFrame* aAnimatedGeometryRoot,
                                              bool aDisplayItemFixedToViewport)
{
  if (!mManager->IsWidgetLayerManager()) {
    
    return nullptr;
  }

  nsPresContext* presContext = mContainerFrame->PresContext();
  nsIFrame* viewport = presContext->PresShell()->GetRootFrame();

  if (viewport == aAnimatedGeometryRoot && aDisplayItemFixedToViewport &&
      nsLayoutUtils::ViewportHasDisplayPort(presContext)) {
    
    return viewport;
  }
  
  if (!viewport->GetFirstChild(nsIFrame::kFixedList)) {
    return nullptr;
  }
  for (const nsIFrame* f = aAnimatedGeometryRoot; f; f = f->GetParent()) {
    if (nsLayoutUtils::IsFixedPosFrameInDisplayPort(f)) {
      return f;
    }
    if (f == mContainerReferenceFrame) {
      
      return nullptr;
    }
  }
  return nullptr;
}

void
ContainerState::AdjustLayerDataForFixedPositioning(const nsIFrame* aFixedPosFrame,
                                                   const nsIntRegion& aDrawRegion,
                                                   nsIntRegion* aVisibleRegion,
                                                   bool* aIsSolidColorInVisibleRegion)
{
  if (!aFixedPosFrame) {
    return;
  }

  nsRect fixedVisibleRect;
  nsPresContext* presContext = aFixedPosFrame->PresContext();
  nsIPresShell* presShell = presContext->PresShell();
  DebugOnly<bool> hasDisplayPort =
    nsLayoutUtils::ViewportHasDisplayPort(presContext, &fixedVisibleRect);
  NS_ASSERTION(hasDisplayPort, "No fixed-pos layer data if there's no displayport");
  
  
  nsIFrame* viewport = presShell->GetRootFrame();
  if (aFixedPosFrame != viewport) {
    
    
    
    
    
    NS_ASSERTION(aFixedPosFrame->StyleDisplay()->mPosition == NS_STYLE_POSITION_FIXED,
      "should be position fixed items only");
    fixedVisibleRect.MoveTo(0, 0);
    if (presShell->IsScrollPositionClampingScrollPortSizeSet()) {
      fixedVisibleRect.SizeTo(presShell->GetScrollPositionClampingScrollPortSize());
    } else {
      fixedVisibleRect.SizeTo(viewport->GetSize());
    }
  }
  fixedVisibleRect += viewport->GetOffsetToCrossDoc(mContainerReferenceFrame);
  nsIntRegion newVisibleRegion;
  newVisibleRegion.And(ScaleToOutsidePixels(fixedVisibleRect, false),
                       aDrawRegion);
  if (!aVisibleRegion->Contains(newVisibleRegion)) {
    if (aIsSolidColorInVisibleRegion) {
      *aIsSolidColorInVisibleRegion = false;
    }
    *aVisibleRegion = newVisibleRegion;
  }
}

void
ContainerState::SetFixedPositionLayerData(Layer* aLayer,
                                          const nsIFrame* aFixedPosFrame)
{
  aLayer->SetIsFixedPosition(aFixedPosFrame != nullptr);
  if (!aFixedPosFrame) {
    return;
  }

  nsPresContext* presContext = aFixedPosFrame->PresContext();

  const nsIFrame* viewportFrame = aFixedPosFrame->GetParent();
  
  
  nsRect anchorRect;
  if (viewportFrame) {
    
    
    if (presContext->PresShell()->IsScrollPositionClampingScrollPortSizeSet()) {
      anchorRect.SizeTo(presContext->PresShell()->GetScrollPositionClampingScrollPortSize());
    } else {
      anchorRect.SizeTo(viewportFrame->GetSize());
    }
  } else {
    
    
    
    viewportFrame = aFixedPosFrame;
  }
  
  anchorRect.MoveTo(viewportFrame->GetOffsetToCrossDoc(mContainerReferenceFrame));

  nsLayoutUtils::SetFixedPositionLayerData(aLayer,
      viewportFrame, anchorRect, aFixedPosFrame, presContext, mParameters);
}

static bool
CanOptimizeAwayThebesLayer(ThebesLayerData* aData,
                           FrameLayerBuilder* aLayerBuilder)
{
  bool isRetained = aData->mLayer->Manager()->IsWidgetLayerManager();
  if (!isRetained) {
    return false;
  }

  
  
  
  if (aData->mLayer->GetValidRegion().IsEmpty()) {
    return true;
  }

  
  
  
  
  return aLayerBuilder->CheckInLayerTreeCompressionMode();
}

void
ContainerState::PopThebesLayerData()
{
  NS_ASSERTION(!mThebesLayerDataStack.IsEmpty(), "Can't pop");

  int32_t lastIndex = mThebesLayerDataStack.Length() - 1;
  ThebesLayerData* data = mThebesLayerDataStack[lastIndex];

  AdjustLayerDataForFixedPositioning(data->mFixedPosFrameForLayerData,
                                     data->mDrawRegion,
                                     &data->mVisibleRegion,
                                     &data->mIsSolidColorInVisibleRegion);
  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer(mBuilder);

  if ((data->mIsSolidColorInVisibleRegion || imageContainer) &&
      CanOptimizeAwayThebesLayer(data, mLayerBuilder)) {
    NS_ASSERTION(!(data->mIsSolidColorInVisibleRegion && imageContainer),
                 "Can't be a solid color as well as an image!");
    if (imageContainer) {
      nsRefPtr<ImageLayer> imageLayer = CreateOrRecycleImageLayer(data->mLayer);
      imageLayer->SetContainer(imageContainer);
      data->mImage->ConfigureLayer(imageLayer, mParameters.mOffset);
      imageLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      if (data->mItemClip.HasClip()) {
        nsIntRect clip = ScaleToNearestPixels(data->mItemClip.GetClipRect());
        clip.MoveBy(mParameters.mOffset);
        imageLayer->SetClipRect(&clip);
      } else {
        imageLayer->SetClipRect(nullptr);
      }
      layer = imageLayer;
      mLayerBuilder->StoreOptimizedLayerForFrame(data->mImage,
                                                 imageLayer);
    } else {
      nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer(data->mLayer);
      colorLayer->SetColor(data->mSolidColor);

      
      colorLayer->SetBaseTransform(data->mLayer->GetBaseTransform());
      colorLayer->SetPostScale(data->mLayer->GetPostXScale(), data->mLayer->GetPostYScale());

      nsIntRect visibleRect = data->mVisibleRegion.GetBounds();
      visibleRect.MoveBy(-GetTranslationForThebesLayer(data->mLayer));
      colorLayer->SetBounds(visibleRect);

      layer = colorLayer;
    }

    NS_ASSERTION(!mNewChildLayers.Contains(layer), "Layer already in list???");
    AutoLayersArray::index_type index = mNewChildLayers.IndexOf(data->mLayer);
    NS_ASSERTION(index != AutoLayersArray::NoIndex, "Thebes layer not found?");
    mNewChildLayers.InsertElementAt(index + 1, layer);

    
    
    nsIntRect emptyRect;
    data->mLayer->SetClipRect(&emptyRect);
    data->mLayer->SetVisibleRegion(nsIntRegion());
    data->mLayer->SetEventRegions(EventRegions());
  } else {
    layer = data->mLayer;
    imageContainer = nullptr;
    layer->SetClipRect(nullptr);
  }

  Matrix transform;
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
      
      
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("Forced background color has changed from #%08X to #%08X on layer %p\n",
                      userData->mForcedBackgroundColor, backgroundColor, data->mLayer);
        nsAutoCString str;
        AppendToString(str, data->mLayer->GetValidRegion());
        printf_stderr("Invalidating layer %p: %s\n", data->mLayer, str.get());
      }
#endif
      data->mLayer->InvalidateRegion(data->mLayer->GetValidRegion());
    }
    userData->mForcedBackgroundColor = backgroundColor;

    
    
    
    int32_t commonClipCount = std::max(0, data->mCommonClipCount);
    SetupMaskLayer(layer, data->mItemClip, commonClipCount);
    
    FrameLayerBuilder::ThebesLayerItemsEntry* entry = mLayerBuilder->
      GetThebesLayerItemsEntry(static_cast<ThebesLayer*>(layer.get()));
    entry->mCommonClipCount = commonClipCount;
  } else {
    
    SetupMaskLayer(layer, data->mItemClip);
  }

  uint32_t flags = 0;
  nsIWidget* widget = mContainerReferenceFrame->PresContext()->GetRootWidget();
  
  bool hidpi = false && widget && widget->GetDefaultScale().scale >= 2;
  if (hidpi) {
    flags |= Layer::CONTENT_DISABLE_SUBPIXEL_AA;
  }
  if (isOpaque && !data->mForceTransparentSurface) {
    flags |= Layer::CONTENT_OPAQUE;
  } else if (data->mNeedComponentAlpha && !hidpi) {
    flags |= Layer::CONTENT_COMPONENT_ALPHA;
  }
  layer->SetContentFlags(flags);

  SetFixedPositionLayerData(layer, data->mFixedPosFrameForLayerData);

  ThebesLayerData* containingThebesLayerData =
     mLayerBuilder->GetContainingThebesLayerData();
  if (containingThebesLayerData) {
    if (!data->mDispatchToContentHitRegion.GetBounds().IsEmpty()) {
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mDispatchToContentHitRegion.GetBounds(),
        containingThebesLayerData->mReferenceFrame);
      containingThebesLayerData->mDispatchToContentHitRegion.Or(
        containingThebesLayerData->mDispatchToContentHitRegion, rect);
    }
    if (!data->mMaybeHitRegion.GetBounds().IsEmpty()) {
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mMaybeHitRegion.GetBounds(),
        containingThebesLayerData->mReferenceFrame);
      containingThebesLayerData->mMaybeHitRegion.Or(
        containingThebesLayerData->mMaybeHitRegion, rect);
    }
    if (!data->mHitRegion.GetBounds().IsEmpty()) {
      
      
      gfx3DMatrix matrix = nsLayoutUtils::GetTransformToAncestor(
        mContainerReferenceFrame, containingThebesLayerData->mReferenceFrame);
      gfxMatrix matrix2D;
      bool isPrecise = matrix.Is2D(&matrix2D) && !matrix2D.HasNonAxisAlignedTransform();
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mHitRegion.GetBounds(),
        containingThebesLayerData->mReferenceFrame);
      nsRegion* dest = isPrecise ? &containingThebesLayerData->mHitRegion
                                 : &containingThebesLayerData->mMaybeHitRegion;
      dest->Or(*dest, rect);
    }
  } else {
    EventRegions regions;
    regions.mHitRegion = ScaleRegionToOutsidePixels(data->mHitRegion);
    
    
    nsIntRegion maybeHitRegion = ScaleRegionToOutsidePixels(data->mMaybeHitRegion);
    regions.mDispatchToContentHitRegion.Sub(maybeHitRegion, regions.mHitRegion);
    regions.mDispatchToContentHitRegion.Or(regions.mDispatchToContentHitRegion,
                                           ScaleRegionToOutsidePixels(data->mDispatchToContentHitRegion));

    nsIntPoint translation = -GetTranslationForThebesLayer(data->mLayer);
    regions.mHitRegion.MoveBy(translation);
    regions.mDispatchToContentHitRegion.MoveBy(translation);

    layer->SetEventRegions(regions);
  }

  if (lastIndex > 0) {
    
    
    
    ThebesLayerData* nextData = mThebesLayerDataStack[lastIndex - 1];
    nextData->CopyAboveRegion(data);
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

  
  
  nsIFrame* f = aItem->Frame();
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
ThebesLayerData::Accumulate(ContainerState* aState,
                            nsDisplayItem* aItem,
                            const nsIntRect& aVisibleRect,
                            const nsIntRect& aDrawRect,
                            const DisplayItemClip& aClip)
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
  bool clipMatches = mItemClip == aClip;
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
    if (isUniform) {
      if (mVisibleRegion.IsEmpty()) {
        
        mSolidColor = uniformColor;
        mIsSolidColorInVisibleRegion = true;
      } else if (mIsSolidColorInVisibleRegion &&
                 mVisibleRegion.IsEqual(nsIntRegion(aVisibleRect)) &&
                 clipMatches) {
        
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
      opaqueClipped.Or(opaqueClipped, aClip.ApproximateIntersectInward(*r));
    }

    nsIntRegion opaquePixels = aState->ScaleRegionToInsidePixels(opaqueClipped, snap);

    nsIntRegionRectIterator iter2(opaquePixels);
    for (const nsIntRect* r = iter2.Next(); r; r = iter2.Next()) {
      
      
      
      
      
      nsIntRegion tmp;
      tmp.Or(mOpaqueRegion, *r);
       
       
       
       if (tmp.GetNumRects() <= 4 ||
           (WindowHasTransparency(aState->mBuilder) &&
            aItem->Frame()->PresContext()->IsChrome())) {
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

ThebesLayerData*
ContainerState::FindThebesLayerFor(nsDisplayItem* aItem,
                                   const nsIntRect& aVisibleRect,
                                   const nsIFrame* aAnimatedGeometryRoot,
                                   const nsPoint& aTopLeft,
                                   bool aShouldFixToViewport)
{
  int32_t i;
  int32_t lowestUsableLayerWithScrolledRoot = -1;
  int32_t topmostLayerWithScrolledRoot = -1;
  for (i = mThebesLayerDataStack.Length() - 1; i >= 0; --i) {
    
    if (aShouldFixToViewport) {
      ++i;
      break;
    }
    ThebesLayerData* data = mThebesLayerDataStack[i];
    
    
    
    if (data->DrawAboveRegionIntersects(aVisibleRect)) {
      ++i;
      break;
    }
    
    
    if (data->mAnimatedGeometryRoot == aAnimatedGeometryRoot &&
        !data->mSingleItemFixedToViewport) {
      lowestUsableLayerWithScrolledRoot = i;
      if (topmostLayerWithScrolledRoot < 0) {
        topmostLayerWithScrolledRoot = i;
      }
    }
    
    
    
    if (data->DrawRegionIntersects(aVisibleRect))
      break;
  }
  if (topmostLayerWithScrolledRoot < 0) {
    --i;
    for (; i >= 0; --i) {
      ThebesLayerData* data = mThebesLayerDataStack[i];
      if (data->mAnimatedGeometryRoot == aAnimatedGeometryRoot) {
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

  ThebesLayerData* thebesLayerData = nullptr;
  if (lowestUsableLayerWithScrolledRoot < 0) {
    nsRefPtr<ThebesLayer> layer =
      CreateOrRecycleThebesLayer(aAnimatedGeometryRoot, aItem->ReferenceFrame(), aTopLeft);

    thebesLayerData = new ThebesLayerData();
    mThebesLayerDataStack.AppendElement(thebesLayerData);
    thebesLayerData->mLayer = layer;
    thebesLayerData->mAnimatedGeometryRoot = aAnimatedGeometryRoot;
    thebesLayerData->mFixedPosFrameForLayerData =
      FindFixedPosFrameForLayerData(aAnimatedGeometryRoot, aShouldFixToViewport);
    thebesLayerData->mReferenceFrame = aItem->ReferenceFrame();
    thebesLayerData->mSingleItemFixedToViewport = aShouldFixToViewport;

    NS_ASSERTION(!mNewChildLayers.Contains(layer), "Layer already in list???");
    *mNewChildLayers.AppendElement() = layer.forget();
  } else {
    thebesLayerData = mThebesLayerDataStack[lowestUsableLayerWithScrolledRoot];
  }

  return thebesLayerData;
}

#ifdef MOZ_DUMP_PAINTING
static void
DumpPaintedImage(nsDisplayItem* aItem, SourceSurface* aSurface)
{
  nsCString string(aItem->Name());
  string.Append('-');
  string.AppendInt((uint64_t)aItem);
  fprintf_stderr(gfxUtils::sDumpPaintFile, "array[\"%s\"]=\"", string.BeginReading());
  gfxUtils::DumpAsDataURI(aSurface, gfxUtils::sDumpPaintFile);
  fprintf_stderr(gfxUtils::sDumpPaintFile, "\";");
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

  RefPtr<DrawTarget> tempDT;
  if (gfxUtils::sDumpPainting) {
    tempDT = gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
                                      itemVisibleRect.Size().ToIntSize(),
                                      SurfaceFormat::B8G8R8A8);
    context = new gfxContext(tempDT);
    context->SetMatrix(gfxMatrix().Translate(-gfxPoint(itemVisibleRect.x,
                                                       itemVisibleRect.y)));
  }
#endif
  basic->BeginTransaction();
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
    RefPtr<SourceSurface> surface = tempDT->Snapshot();
    DumpPaintedImage(aItem, surface);

    DrawTarget* drawTarget = aContext->GetDrawTarget();
    Rect rect(itemVisibleRect.x, itemVisibleRect.y,
              itemVisibleRect.width, itemVisibleRect.height);
    drawTarget->DrawSurface(surface, rect, Rect(Point(0,0), rect.Size()));

    aItem->SetPainted();
  }
#endif
}





bool
ContainerState::ChooseAnimatedGeometryRoot(const nsDisplayList& aList,
                                           const nsIFrame **aAnimatedGeometryRoot)
{
  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    LayerState layerState = item->GetLayerState(mBuilder, mManager, mParameters);
    
    
    if (layerState == LAYER_ACTIVE_FORCE) {
      continue;
    }

    
    
    *aAnimatedGeometryRoot =
      nsLayoutUtils::GetAnimatedGeometryRootFor(item, mBuilder);
    return true;
  }
  return false;
}















void
ContainerState::ProcessDisplayItems(const nsDisplayList& aList,
                                    uint32_t aFlags)
{
  PROFILER_LABEL("ContainerState", "ProcessDisplayItems",
    js::ProfileEntry::Category::GRAPHICS);

  const nsIFrame* lastAnimatedGeometryRoot = mContainerReferenceFrame;
  nsPoint topLeft(0,0);

  
  
  
  if (aFlags & NO_COMPONENT_ALPHA) {
    if (ChooseAnimatedGeometryRoot(aList, &lastAnimatedGeometryRoot)) {
      topLeft = lastAnimatedGeometryRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
    }
  }

  int32_t maxLayers = nsDisplayItem::MaxActiveLayers();
  int layerCount = 0;

  for (nsDisplayItem* item = aList.GetBottom(); item; item = item->GetAbove()) {
    NS_ASSERTION(mAppUnitsPerDevPixel == AppUnitsPerDevPixel(item),
      "items in a container layer should all have the same app units per dev pixel");

    nsIntRect itemVisibleRect =
      ScaleToOutsidePixels(item->GetVisibleRect(), false);
    bool snap;
    nsRect itemContent = item->GetBounds(mBuilder, &snap);
    nsIntRect itemDrawRect = ScaleToOutsidePixels(itemContent, snap);
    nsDisplayItem::Type itemType = item->GetType();
    nsIntRect clipRect;
    const DisplayItemClip& itemClip = item->GetClip();
    if (itemClip.HasClip()) {
      itemContent.IntersectRect(itemContent, itemClip.GetClipRect());
      clipRect = ScaleToNearestPixels(itemClip.GetClipRect());
      itemDrawRect.IntersectRect(itemDrawRect, clipRect);
      clipRect.MoveBy(mParameters.mOffset);
    }
    mBounds.UnionRect(mBounds, itemContent);
    itemVisibleRect.IntersectRect(itemVisibleRect, itemDrawRect);

    LayerState layerState = item->GetLayerState(mBuilder, mManager, mParameters);
    if (layerState == LAYER_INACTIVE &&
        nsDisplayItem::ForceActiveLayers()) {
      layerState = LAYER_ACTIVE;
    }

    bool forceInactive;
    const nsIFrame* animatedGeometryRoot;
    if (aFlags & NO_COMPONENT_ALPHA) {
      forceInactive = true;
      animatedGeometryRoot = lastAnimatedGeometryRoot;
    } else {
      forceInactive = false;
      if (mManager->IsWidgetLayerManager()) {
        animatedGeometryRoot = nsLayoutUtils::GetAnimatedGeometryRootFor(item, mBuilder);
      } else {
        
        
        
        animatedGeometryRoot = mContainerAnimatedGeometryRoot;
      }
      if (animatedGeometryRoot != lastAnimatedGeometryRoot) {
        lastAnimatedGeometryRoot = animatedGeometryRoot;
        topLeft = animatedGeometryRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
      }
    }
    bool shouldFixToViewport = !animatedGeometryRoot->GetParent() &&
      item->ShouldFixToViewport(mBuilder);

    if (maxLayers != -1 && layerCount >= maxLayers) {
      forceInactive = true;
    }

    
    if (layerState == LAYER_ACTIVE_FORCE ||
        (layerState == LAYER_INACTIVE && !mManager->IsWidgetLayerManager()) ||
        (!forceInactive &&
         (layerState == LAYER_ACTIVE_EMPTY ||
          layerState == LAYER_ACTIVE))) {

      layerCount++;

      
      
      NS_ASSERTION(layerState != LAYER_ACTIVE_EMPTY ||
                   itemVisibleRect.IsEmpty(),
                   "State is LAYER_ACTIVE_EMPTY but visible rect is not.");

      
      
      
      
      InvalidateForLayerChange(item, nullptr, itemClip, topLeft, nullptr);

      
      
      
      
      if (itemVisibleRect.IsEmpty() &&
          !item->ShouldBuildLayerEvenIfInvisible(mBuilder)) {
        continue;
      }

      if (itemType == nsDisplayItem::TYPE_TRANSFORM) {
        mParameters.mAncestorClipRect = itemClip.HasClip() ? &clipRect : nullptr;
      } else {
        mParameters.mAncestorClipRect = nullptr;
      }

      
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager, mParameters);
      if (!ownLayer) {
        continue;
      }

      NS_ASSERTION(!ownLayer->AsThebesLayer(),
                   "Should never have created a dedicated Thebes layer!");

      const nsIFrame* fixedPosFrame =
        FindFixedPosFrameForLayerData(animatedGeometryRoot, shouldFixToViewport);
      if (fixedPosFrame) {
        nsIntRegion visibleRegion(itemVisibleRect);
        AdjustLayerDataForFixedPositioning(fixedPosFrame,
                                           nsIntRegion(itemDrawRect), &visibleRegion);
        itemVisibleRect = visibleRegion.GetBounds();
      }
      SetFixedPositionLayerData(ownLayer, fixedPosFrame);

      nsRect invalid;
      if (item->IsInvalid(invalid)) {
        ownLayer->SetInvalidRectToVisibleRegion();
      }

      
      
      if (!ownLayer->AsContainerLayer()) {
        ownLayer->SetPostScale(mParameters.mXScale,
                               mParameters.mYScale);
      }

      
      NS_ASSERTION(ownLayer->Manager() == mManager, "Wrong manager");
      NS_ASSERTION(!ownLayer->HasUserData(&gLayerManagerUserData),
                   "We shouldn't have a FrameLayerBuilder-managed layer here!");
      NS_ASSERTION(itemClip.HasClip() ||
                   itemClip.GetRoundedRectCount() == 0,
                   "If we have rounded rects, we must have a clip rect");
      
      if (itemClip.HasClip()) {
        ownLayer->SetClipRect(&clipRect);
      } else {
        ownLayer->SetClipRect(nullptr);
      }
      ThebesLayerData* data = GetTopThebesLayerData();
      if (data) {
        
        
        
        
        if (item->GetType() == nsDisplayItem::TYPE_TRANSFORM &&
            nsDisplayTransform::ShouldPrerenderTransformedContent(mBuilder,
                                                                  item->Frame(),
                                                                  false)) {
          if (!itemClip.HasClip()) {
            
            
            data->SetAllDrawingAbove();
          } else {
            
            
            
            data->AddVisibleAboveRegion(clipRect);
            data->AddDrawAboveRegion(clipRect);
          }
        } else {
          data->AddVisibleAboveRegion(itemVisibleRect);

          
          
          
          
          data->AddDrawAboveRegion(itemDrawRect);
        }
      }
      itemVisibleRect.MoveBy(mParameters.mOffset);
      if (item->SetVisibleRegionOnLayer()) {
        SetVisibleRegionForLayer(ownLayer, ownLayer->GetVisibleRegion(), itemVisibleRect);
      }

      
      
      if (itemClip.IsRectClippedByRoundedCorner(itemContent)) {
        SetupMaskLayer(ownLayer, itemClip);
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
                                         itemClip, layerState,
                                         topLeft, nullptr,
                                         dummy);
    } else {
      ThebesLayerData* data =
        FindThebesLayerFor(item, itemVisibleRect, animatedGeometryRoot, topLeft,
                           shouldFixToViewport);

      if (itemType == nsDisplayItem::TYPE_LAYER_EVENT_REGIONS) {
        nsDisplayLayerEventRegions* eventRegions =
            static_cast<nsDisplayLayerEventRegions*>(item);
        data->AccumulateEventRegions(eventRegions->HitRegion(),
                                     eventRegions->MaybeHitRegion(),
                                     eventRegions->DispatchToContentHitRegion());
      } else {
        
        
        data->UpdateCommonClipCount(itemClip);
        data->Accumulate(this, item, itemVisibleRect, itemDrawRect, itemClip);

        nsAutoPtr<nsDisplayItemGeometry> geometry(item->AllocateGeometry(mBuilder));
        InvalidateForLayerChange(item, data->mLayer, itemClip, topLeft, geometry);

        mLayerBuilder->AddThebesDisplayItem(data, item, itemClip,
                                            mContainerFrame,
                                            layerState, topLeft,
                                            geometry);
      }
    }
  }
}

void
ContainerState::InvalidateForLayerChange(nsDisplayItem* aItem,
                                         Layer* aNewLayer,
                                         const DisplayItemClip& aClip,
                                         const nsPoint& aTopLeft,
                                         nsDisplayItemGeometry *aGeometry)
{
  NS_ASSERTION(aItem->GetPerFrameKey(),
               "Display items that render using Thebes must have a key");
  nsDisplayItemGeometry *oldGeometry = nullptr;
  DisplayItemClip* oldClip = nullptr;
  nsAutoTArray<nsIFrame*,4> changedFrames;
  bool isInvalid = false;
  Layer* oldLayer = mLayerBuilder->GetOldLayerFor(aItem, &oldGeometry, &oldClip, &changedFrames, &isInvalid);
  if (aNewLayer != oldLayer && oldLayer) {
    
    
    ThebesLayer* t = oldLayer->AsThebesLayer();
    if (t) {
      
      
      
#ifdef MOZ_DUMP_PAINTING
      if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
        printf_stderr("Display item type %s(%p) changed layers %p to %p!\n", aItem->Name(), aItem->Frame(), t, aNewLayer);
      }
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
  nsPoint shift = aTopLeft - data->mLastAnimatedGeometryRootOrigin;
  bool notifyRenderingChanged = true;
  if (!oldLayer) {
    
    
    combined = aClip.ApplyNonRoundedIntersection(aGeometry->ComputeInvalidationRegion());
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Display item type %s(%p) added to layer %p!\n", aItem->Name(), aItem->Frame(), aNewLayer);
    }
#endif
  } else if (isInvalid || (aItem->IsInvalid(invalid) && invalid.IsEmpty())) {
    
    combined = oldClip->ApplyNonRoundedIntersection(oldGeometry->ComputeInvalidationRegion());
    combined.MoveBy(shift);
    combined.Or(combined, aClip.ApplyNonRoundedIntersection(aGeometry->ComputeInvalidationRegion()));
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Display item type %s(%p) (in layer %p) belongs to an invalidated frame!\n", aItem->Name(), aItem->Frame(), aNewLayer);
    }
#endif
  } else {
    
    

    
    
    
    
    
    
    
    
    
    if (oldGeometry->ComputeInvalidationRegion() == aGeometry->ComputeInvalidationRegion() &&
        *oldClip == aClip && invalid.IsEmpty() && changedFrames.Length() == 0) {
      notifyRenderingChanged = false;
    }

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
    if (aClip.ComputeRegionInClips(oldClip, shift, &clip)) {
      combined.And(combined, clip);
    }
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      if (!combined.IsEmpty()) {
        printf_stderr("Display item type %s(%p) (in layer %p) changed geometry!\n", aItem->Name(), aItem->Frame(), aNewLayer);
      }
    }
#endif
  }
  if (!combined.IsEmpty()) {
    if (notifyRenderingChanged) {
      aItem->NotifyRenderingChanged();
    }
    InvalidatePostTransformRegion(newThebesLayer,
        combined.ScaleToOutsidePixels(data->mXScale, data->mYScale, mAppUnitsPerDevPixel),
        GetTranslationForThebesLayer(newThebesLayer));
  }
}

void
FrameLayerBuilder::AddThebesDisplayItem(ThebesLayerData* aLayerData,
                                        nsDisplayItem* aItem,
                                        const DisplayItemClip& aClip,
                                        nsIFrame* aContainerLayerFrame,
                                        LayerState aLayerState,
                                        const nsPoint& aTopLeft,
                                        nsAutoPtr<nsDisplayItemGeometry> aGeometry)
{
  ThebesLayer* layer = aLayerData->mLayer;
  ThebesDisplayItemLayerUserData* thebesData =
    static_cast<ThebesDisplayItemLayerUserData*>
      (layer->GetUserData(&gThebesDisplayItemLayerUserData));
  nsRefPtr<BasicLayerManager> tempManager;
  nsIntRect intClip;
  bool hasClip = false;
  if (aLayerState != LAYER_NONE) {
    DisplayItemData *data = GetDisplayItemDataForManager(aItem, layer->Manager());
    if (data) {
      tempManager = data->mInactiveManager;
    }
    if (!tempManager) {
      tempManager = new BasicLayerManager();
    }

    
    nsRegion clip;
    DisplayItemClip* oldClip = nullptr;
    GetOldLayerFor(aItem, nullptr, &oldClip);
    hasClip = aClip.ComputeRegionInClips(oldClip,
                                         aTopLeft - thebesData->mLastAnimatedGeometryRootOrigin,
                                         &clip);

    if (hasClip) {
      intClip = clip.GetBounds().ScaleToOutsidePixels(thebesData->mXScale,
                                                      thebesData->mYScale,
                                                      thebesData->mAppUnitsPerDevPixel);
    }
  }

  AddLayerDisplayItem(layer, aItem, aClip, aLayerState, aTopLeft, tempManager, aGeometry);

  ThebesLayerItemsEntry* entry = mThebesLayerItems.PutEntry(layer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerLayerFrame;
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
    if (tempManager) {
      FrameLayerBuilder* layerBuilder = new FrameLayerBuilder();
      layerBuilder->Init(mDisplayListBuilder, tempManager, aLayerData);

      tempManager->BeginTransaction();
      if (mRetainingManager) {
        layerBuilder->DidBeginRetainedLayerTransaction(tempManager);
      }

      nsAutoPtr<LayerProperties> props(LayerProperties::CloneFrom(tempManager->GetRoot()));
      nsRefPtr<Layer> tmpLayer =
        aItem->BuildLayer(mDisplayListBuilder, tempManager, ContainerLayerParameters());
      
      
      if (!tmpLayer) {
        tempManager->EndTransaction(nullptr, nullptr);
        tempManager->SetUserData(&gLayerManagerLayerBuilder, nullptr);
        return;
      }

      
      
      if (mRetainingManager) {
#ifdef DEBUG_DISPLAY_ITEM_DATA
        LayerManagerData* parentLmd = static_cast<LayerManagerData*>
          (layer->Manager()->GetUserData(&gLayerManagerUserData));
        LayerManagerData* lmd = static_cast<LayerManagerData*>
          (tempManager->GetUserData(&gLayerManagerUserData));
        lmd->mParent = parentLmd;
#endif
        layerBuilder->StoreDataForFrame(aItem, tmpLayer, LAYER_ACTIVE);
      }

      tempManager->SetRoot(tmpLayer);
      layerBuilder->WillEndTransaction();
      tempManager->AbortTransaction();

      nsIntPoint offset = GetLastPaintOffset(layer) - GetTranslationForThebesLayer(layer);
      props->MoveBy(-offset);
      nsIntRegion invalid = props->ComputeDifferences(tmpLayer, nullptr);
      if (aLayerState == LAYER_SVG_EFFECTS) {
        invalid = nsSVGIntegrationUtils::AdjustInvalidAreaForSVGEffects(aItem->Frame(),
                                                                        aItem->ToReferenceFrame(),
                                                                        invalid);
      }
      if (!invalid.IsEmpty()) {
#ifdef MOZ_DUMP_PAINTING
        if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
          printf_stderr("Inactive LayerManager(%p) for display item %s(%p) has an invalid region - invalidating layer %p\n", tempManager.get(), aItem->Name(), aItem->Frame(), layer);
        }
#endif
        if (hasClip) {
          invalid.And(invalid, intClip);
        }

        invalid.ScaleRoundOut(thebesData->mXScale, thebesData->mYScale);
        InvalidatePostTransformRegion(layer, invalid,
                                      GetTranslationForThebesLayer(layer));
      }
    }
    ClippedDisplayItem* cdi =
      entry->mItems.AppendElement(ClippedDisplayItem(aItem,
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

  data->AddFrame(aItem->Frame());

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
    basic->SetUserData(&gLayerManagerLayerBuilder, nullptr);
  }
}

void
FrameLayerBuilder::AddLayerDisplayItem(Layer* aLayer,
                                       nsDisplayItem* aItem,
                                       const DisplayItemClip& aClip,
                                       LayerState aLayerState,
                                       const nsPoint& aTopLeft,
                                       BasicLayerManager* aManager,
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

bool
FrameLayerBuilder::CheckInLayerTreeCompressionMode()
{
  if (mInLayerTreeCompressionMode) {
    return true;
  }

  
  
  mRootPresContext->PresShell()->GetRootFrame()->SchedulePaint(nsIFrame::PAINT_DELAYED_COMPRESS);

  return false;
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
    if (layer->GetPrevSibling() != prevChild) {
      mContainerLayer->RepositionChild(layer, prevChild);
    }
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

static inline gfxSize RoundToFloatPrecision(const gfxSize& aSize)
{
  return gfxSize(float(aSize.width), float(aSize.height));
}

static bool
ChooseScaleAndSetTransform(FrameLayerBuilder* aLayerBuilder,
                           nsDisplayListBuilder* aDisplayListBuilder,
                           nsIFrame* aContainerFrame,
                           const gfx3DMatrix* aTransform,
                           const ContainerLayerParameters& aIncomingScale,
                           ContainerLayer* aLayer,
                           LayerState aState,
                           ContainerLayerParameters& aOutgoingScale)
{
  nsIntPoint offset;

  gfx3DMatrix transform =
    gfx3DMatrix::ScalingMatrix(aIncomingScale.mXScale, aIncomingScale.mYScale, 1.0);
  if (aTransform) {
    
    transform = (*aTransform)*transform;
    
    
    
    
    
    transform.NudgeToIntegersFixedEpsilon();
  }
  gfxMatrix transform2d;
  if (aContainerFrame &&
      (aState == LAYER_INACTIVE || aState == LAYER_SVG_EFFECTS) &&
      (!aTransform || (aTransform->Is2D(&transform2d) &&
                       !transform2d.HasNonTranslation()))) {
    
    
    
    
    
    
    
    nsPoint appUnitOffset = aDisplayListBuilder->ToReferenceFrame(aContainerFrame);
    nscoord appUnitsPerDevPixel = aContainerFrame->PresContext()->AppUnitsPerDevPixel();
    offset = nsIntPoint(
        NS_lround(NSAppUnitsToDoublePixels(appUnitOffset.x, appUnitsPerDevPixel)*aIncomingScale.mXScale),
        NS_lround(NSAppUnitsToDoublePixels(appUnitOffset.y, appUnitsPerDevPixel)*aIncomingScale.mYScale));
  }
  transform = transform * gfx3DMatrix::Translation(offset.x + aIncomingScale.mOffset.x, offset.y + aIncomingScale.mOffset.y, 0);

  if (transform.IsSingular()) {
    return false;
  }

  bool canDraw2D = transform.CanDraw2D(&transform2d);
  gfxSize scale;
  
  if (canDraw2D) {
    
    
    if (aContainerFrame->GetContent() &&
        nsLayoutUtils::HasAnimationsForCompositor(
          aContainerFrame->GetContent(), eCSSProperty_transform)) {
      scale = nsLayoutUtils::ComputeSuitableScaleForAnimation(aContainerFrame->GetContent());
    } else {
      
      scale = RoundToFloatPrecision(transform2d.ScaleFactors(true));
      
      
      
      
      
      
      gfxMatrix frameTransform;
      if (ActiveLayerTracker::IsStyleAnimated(aContainerFrame, eCSSProperty_transform) &&
          aTransform &&
          (!aTransform->Is2D(&frameTransform) || frameTransform.HasNonTranslationOrFlip())) {
        
        
        bool clamp = true;
        Matrix oldFrameTransform2d;
        if (aLayer->GetBaseTransform().Is2D(&oldFrameTransform2d)) {
          gfxSize oldScale = RoundToFloatPrecision(ThebesMatrix(oldFrameTransform2d).ScaleFactors(true));
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

  
  Matrix4x4 baseTransform;
  ToMatrix4x4(transform, baseTransform);
  aLayer->SetBaseTransform(baseTransform);
  aLayer->SetPreScale(1.0f/float(scale.width),
                      1.0f/float(scale.height));
  aLayer->SetInheritedScale(aIncomingScale.mXScale,
                            aIncomingScale.mYScale);

  aOutgoingScale =
    ContainerLayerParameters(scale.width, scale.height, -offset, aIncomingScale);
  if (aTransform) {
    aOutgoingScale.mInTransformedSubtree = true;
    if (ActiveLayerTracker::IsStyleAnimated(aContainerFrame, eCSSProperty_transform)) {
      aOutgoingScale.mInActiveTransformedSubtree = true;
    }
  }
  bool isRetained = aLayer->Manager()->IsWidgetLayerManager();
  if (isRetained && (!canDraw2D || transform2d.HasNonIntegerTranslation())) {
    aOutgoingScale.mDisableSubpixelAntialiasingInDescendants = true;
  }
  return true;
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
                                          const ContainerLayerParameters& aParameters,
                                          const gfx3DMatrix* aTransform,
                                          uint32_t aFlags)
{
  uint32_t containerDisplayItemKey =
    aContainerItem ? aContainerItem->GetPerFrameKey() : nsDisplayItem::TYPE_ZERO;
  NS_ASSERTION(aContainerFrame, "Container display items here should have a frame");
  NS_ASSERTION(!aContainerItem ||
               aContainerItem->Frame() == aContainerFrame,
               "Container display item must match given frame");

  if (!aParameters.mXScale || !aParameters.mYScale) {
    return nullptr;
  }

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

  ContainerLayerParameters scaleParameters;
  if (!ChooseScaleAndSetTransform(this, aBuilder, aContainerFrame, aTransform, aParameters,
                                  containerLayer, state, scaleParameters)) {
    return nullptr;
  }

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

    state.ProcessDisplayItems(aChildren, stateFlags);

    
    
    
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
  if (aParameters.mAncestorClipRect && !(aFlags & CONTAINER_NOT_CLIPPED_BY_ANCESTORS)) {
    SetVisibleRegionForLayer(containerLayer, nsIntRegion(pixBounds),
                             *aParameters.mAncestorClipRect);
  } else {
    containerLayer->SetVisibleRegion(pixBounds);
  }
  
  
  if (aChildren.IsOpaque() && !aChildren.NeedsTransparentSurface()) {
    bounds.ScaleRoundIn(scaleParameters.mXScale, scaleParameters.mYScale);
    if (bounds.Contains(pixBounds.ToAppUnits(appUnitsPerDevPixel))) {
      
      flags = Layer::CONTENT_OPAQUE;
    }
  }
  containerLayer->SetContentFlags(flags);

  mContainerLayerGeneration = oldGeneration;
  nsPresContext::ClearNotifySubDocInvalidationData(containerLayer);

  return containerLayer.forget();
}

Layer*
FrameLayerBuilder::GetLeafLayerFor(nsDisplayListBuilder* aBuilder,
                                   nsDisplayItem* aItem)
{
  Layer* layer = GetOldLayerFor(aItem);
  if (!layer)
    return nullptr;
  if (layer->HasUserData(&gThebesDisplayItemLayerUserData)) {
    
    
    
    return nullptr;
  }
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
  MOZ_ASSERT(aFrame, "need a frame");
  nsIFrame* last = nullptr;
  for (nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f)) {
    last = f;

    if (nsLayoutUtils::IsPopup(f)) {
      
      
      
      break;
    }

    nsTArray<DisplayItemData*> *array =
      reinterpret_cast<nsTArray<DisplayItemData*>*>(f->Properties().Get(LayerManagerDataProperty()));
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
static void DebugPaintItem(nsRenderingContext* aDest,
                           nsPresContext* aPresContext,
                           nsDisplayItem *aItem,
                           nsDisplayListBuilder* aBuilder)
{
  bool snap;
  nsRect appUnitBounds = aItem->GetBounds(aBuilder, &snap);
  gfxRect bounds(appUnitBounds.x, appUnitBounds.y, appUnitBounds.width, appUnitBounds.height);
  bounds.ScaleInverse(aPresContext->AppUnitsPerDevPixel());

  RefPtr<DrawTarget> tempDT =
    gfxPlatform::GetPlatform()->CreateOffscreenContentDrawTarget(
                                          IntSize(bounds.width, bounds.height),
                                          SurfaceFormat::B8G8R8A8);
  nsRefPtr<gfxContext> context = new gfxContext(tempDT);
  context->SetMatrix(gfxMatrix().Translate(-gfxPoint(bounds.x, bounds.y)));
  nsRefPtr<nsRenderingContext> ctx = new nsRenderingContext();
  ctx->Init(aDest->DeviceContext(), context);

  aItem->Paint(aBuilder, ctx);
  RefPtr<SourceSurface> surface = tempDT->Snapshot();
  DumpPaintedImage(aItem, surface);

  DrawTarget* drawTarget = aDest->ThebesContext()->GetDrawTarget();
  Rect rect = ToRect(bounds);
  drawTarget->DrawSurface(surface, rect, Rect(Point(0,0), rect.Size()));

  aItem->SetPainted();
}
#endif

 void
FrameLayerBuilder::RecomputeVisibilityForItems(nsTArray<ClippedDisplayItem>& aItems,
                                               nsDisplayListBuilder *aBuilder,
                                               const nsIntRegion& aRegionToDraw,
                                               const nsIntPoint& aOffset,
                                               int32_t aAppUnitsPerDevPixel,
                                               float aXScale,
                                               float aYScale)
{
  uint32_t i;
  
  
  
  
  
  nsRegion visible = aRegionToDraw.ToAppUnits(aAppUnitsPerDevPixel);
  visible.MoveBy(NSIntPixelsToAppUnits(aOffset.x, aAppUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(aOffset.y, aAppUnitsPerDevPixel));
  visible.ScaleInverseRoundOut(aXScale, aYScale);

  for (i = aItems.Length(); i > 0; --i) {
    ClippedDisplayItem* cdi = &aItems[i - 1];
    const DisplayItemClip& clip = cdi->mItem->GetClip();

    NS_ASSERTION(AppUnitsPerDevPixel(cdi->mItem) == aAppUnitsPerDevPixel,
                 "a thebes layer should contain items only at the same zoom");

    NS_ABORT_IF_FALSE(clip.HasClip() ||
                      clip.GetRoundedRectCount() == 0,
                      "If we have rounded rects, we must have a clip rect");

    if (!clip.IsRectAffectedByClip(visible.GetBounds())) {
      cdi->mItem->RecomputeVisibility(aBuilder, &visible);
      continue;
    }

    
    
    nsRegion clipped;
    clipped.And(visible, clip.NonRoundedIntersection());
    nsRegion finalClipped = clipped;
    cdi->mItem->RecomputeVisibility(aBuilder, &finalClipped);
    
    
    if (clip.GetRoundedRectCount() == 0) {
      nsRegion removed;
      removed.Sub(clipped, finalClipped);
      nsRegion newVisible;
      newVisible.Sub(visible, removed);
      
      if (newVisible.GetNumRects() <= 15) {
        visible = newVisible;
      }
    }
  }
}

void
FrameLayerBuilder::PaintItems(nsTArray<ClippedDisplayItem>& aItems,
                              const nsIntRect& aRect,
                              gfxContext *aContext,
                              nsRenderingContext *aRC,
                              nsDisplayListBuilder* aBuilder,
                              nsPresContext* aPresContext,
                              const nsIntPoint& aOffset,
                              float aXScale, float aYScale,
                              int32_t aCommonClipCount)
{
  int32_t appUnitsPerDevPixel = aPresContext->AppUnitsPerDevPixel();
  nsRect boundRect = aRect.ToAppUnits(appUnitsPerDevPixel);
  boundRect.MoveBy(NSIntPixelsToAppUnits(aOffset.x, appUnitsPerDevPixel),
                 NSIntPixelsToAppUnits(aOffset.y, appUnitsPerDevPixel));
  boundRect.ScaleInverseRoundOut(aXScale, aYScale);

  DisplayItemClip currentClip;
  bool currentClipIsSetInContext = false;
  DisplayItemClip tmpClip;

  for (uint32_t i = 0; i < aItems.Length(); ++i) {
    ClippedDisplayItem* cdi = &aItems[i];

    nsRect paintRect = cdi->mItem->GetVisibleRect().Intersect(boundRect);
    if (paintRect.IsEmpty())
      continue;

#ifdef MOZ_DUMP_PAINTING
    PROFILER_LABEL_PRINTF("DisplayList", "Draw", js::ProfileEntry::Category::GRAPHICS, "%s %p", cdi->mItem->Name(), cdi->mItem);
#else
    PROFILER_LABEL_PRINTF("DisplayList", "Draw", js::ProfileEntry::Category::GRAPHICS, "%p", cdi->mItem);
#endif

    
    
    const DisplayItemClip* clip = &cdi->mItem->GetClip();
    if (clip->GetRoundedRectCount() > 0 &&
        !clip->IsRectClippedByRoundedCorner(cdi->mItem->GetVisibleRect())) {
      tmpClip = *clip;
      tmpClip.RemoveRoundedCorners();
      clip = &tmpClip;
    }
    if (currentClipIsSetInContext != clip->HasClip() ||
        (clip->HasClip() && *clip != currentClip)) {
      if (currentClipIsSetInContext) {
        aContext->Restore();
      }
      currentClipIsSetInContext = clip->HasClip();
      if (currentClipIsSetInContext) {
        currentClip = *clip;
        aContext->Save();
        NS_ASSERTION(aCommonClipCount < 100,
          "Maybe you really do have more than a hundred clipping rounded rects, or maybe something has gone wrong.");
        currentClip.ApplyTo(aContext, aPresContext, aCommonClipCount);
        aContext->NewPath();
      }
    }

    if (cdi->mInactiveLayerManager) {
      PaintInactiveLayer(aBuilder, cdi->mInactiveLayerManager, cdi->mItem, aContext, aRC);
    } else {
      nsIFrame* frame = cdi->mItem->Frame();
      frame->AddStateBits(NS_FRAME_PAINTED_THEBES);
#ifdef MOZ_DUMP_PAINTING

      if (gfxUtils::sDumpPainting) {
        DebugPaintItem(aRC, aPresContext, cdi->mItem, aBuilder);
      } else {
#else
      {
#endif
        cdi->mItem->Paint(aBuilder, aRC);
      }
    }

    if (CheckDOMModified())
      break;
  }

  if (currentClipIsSetInContext) {
    aContext->Restore();
  }
}






static bool ShouldDrawRectsSeparately(gfxContext* aContext, DrawRegionClip aClip)
{
  if (!gfxPrefs::LayoutPaintRectsSeparately() ||
      aContext->IsCairo() ||
      aClip == DrawRegionClip::CLIP_NONE) {
    return false;
  }

  DrawTarget *dt = aContext->GetDrawTarget();
  return dt->GetBackendType() == BackendType::DIRECT2D;
}

static void DrawForcedBackgroundColor(gfxContext* aContext, Layer* aLayer, nscolor aBackgroundColor)
{
  if (NS_GET_A(aBackgroundColor) > 0) {
    nsIntRect r = aLayer->GetVisibleRegion().GetBounds();
    aContext->NewPath();
    aContext->Rectangle(gfxRect(r.x, r.y, r.width, r.height));
    aContext->SetColor(gfxRGBA(aBackgroundColor));
    aContext->Fill();
  }
}





























 void
FrameLayerBuilder::DrawThebesLayer(ThebesLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw,
                                   DrawRegionClip aClip,
                                   const nsIntRegion& aRegionToInvalidate,
                                   void* aCallbackData)
{
  PROFILER_LABEL("FrameLayerBuilder", "DrawThebesLayer",
    js::ProfileEntry::Category::GRAPHICS);

  nsDisplayListBuilder* builder = static_cast<nsDisplayListBuilder*>
    (aCallbackData);

  FrameLayerBuilder *layerBuilder = aLayer->Manager()->GetLayerBuilder();
  NS_ASSERTION(layerBuilder, "Unexpectedly null layer builder!");

  if (layerBuilder->CheckDOMModified())
    return;

  ThebesLayerItemsEntry* entry = layerBuilder->mThebesLayerItems.GetEntry(aLayer);
  NS_ASSERTION(entry, "We shouldn't be drawing into a layer with no items!");
  if (!entry->mContainerLayerFrame) {
    return;
  }


  ThebesDisplayItemLayerUserData* userData =
    static_cast<ThebesDisplayItemLayerUserData*>
      (aLayer->GetUserData(&gThebesDisplayItemLayerUserData));
  NS_ASSERTION(userData, "where did our user data go?");

  bool shouldDrawRectsSeparately = ShouldDrawRectsSeparately(aContext, aClip);

  if (!shouldDrawRectsSeparately) {
    if (aClip == DrawRegionClip::DRAW_SNAPPED) {
      gfxUtils::ClipToRegionSnapped(aContext, aRegionToDraw);
    } else if (aClip == DrawRegionClip::DRAW) {
      gfxUtils::ClipToRegion(aContext, aRegionToDraw);
    }

    DrawForcedBackgroundColor(aContext, aLayer, userData->mForcedBackgroundColor);
  }

  
  
  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  nsIntPoint offset = GetTranslationForThebesLayer(aLayer);

  nsPresContext* presContext = entry->mContainerLayerFrame->PresContext();
  int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();

  RecomputeVisibilityForItems(entry->mItems, builder, aRegionToDraw,
                              offset, appUnitsPerDevPixel,
                              userData->mXScale, userData->mYScale);

  nsRefPtr<nsRenderingContext> rc = new nsRenderingContext();
  rc->Init(presContext->DeviceContext(), aContext);

  if (shouldDrawRectsSeparately) {
    nsIntRegionRectIterator it(aRegionToDraw);
    while (const nsIntRect* iterRect = it.Next()) {
      gfxContextAutoSaveRestore save(aContext);
      aContext->NewPath();
      aContext->Rectangle(*iterRect, aClip == DrawRegionClip::DRAW_SNAPPED);
      aContext->Clip();

      DrawForcedBackgroundColor(aContext, aLayer, userData->mForcedBackgroundColor);

      
      
      
      aContext->Translate(aLayer->GetResidualTranslation() - gfxPoint(offset.x, offset.y));
      aContext->Scale(userData->mXScale, userData->mYScale);

      layerBuilder->PaintItems(entry->mItems, *iterRect, aContext, rc,
                               builder, presContext,
                               offset, userData->mXScale, userData->mYScale,
                               entry->mCommonClipCount);
    }
  } else {
    
    
    
    aContext->Translate(aLayer->GetResidualTranslation() - gfxPoint(offset.x, offset.y));
    aContext->Scale(userData->mXScale, userData->mYScale);

    layerBuilder->PaintItems(entry->mItems, aRegionToDraw.GetBounds(), aContext, rc,
                             builder, presContext,
                             offset, userData->mXScale, userData->mYScale,
                             entry->mCommonClipCount);
  }

  if (presContext->GetPaintFlashing()) {
    gfxContextAutoSaveRestore save(aContext);
    if (shouldDrawRectsSeparately) {
      if (aClip == DrawRegionClip::DRAW_SNAPPED) {
        gfxUtils::ClipToRegionSnapped(aContext, aRegionToDraw);
      } else if (aClip == DrawRegionClip::DRAW) {
        gfxUtils::ClipToRegion(aContext, aRegionToDraw);
      }
    }
    FlashPaint(aContext);
  }

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
FrameLayerBuilder::DumpRetainedLayerTree(LayerManager* aManager, std::stringstream& aStream, bool aDumpHtml)
{
  aManager->Dump(aStream, "", aDumpHtml);
}
#endif

gfx::Rect
CalculateBounds(const nsTArray<DisplayItemClip::RoundedRect>& aRects, int32_t A2D)
{
  nsRect bounds = aRects[0].mRect;
  for (uint32_t i = 1; i < aRects.Length(); ++i) {
    bounds.UnionRect(bounds, aRects[i].mRect);
   }

  return gfx::ToRect(nsLayoutUtils::RectToGfxRect(bounds, A2D));
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
ContainerState::SetupMaskLayer(Layer *aLayer, const DisplayItemClip& aClip,
                               uint32_t aRoundedRectClipCount)
{
  
  
  
  ThebesDisplayItemLayerUserData* thebesData = GetThebesDisplayItemLayerUserData(aLayer);
  if (thebesData &&
      aRoundedRectClipCount < thebesData->mMaskClipCount) {
    ThebesLayer* thebes = aLayer->AsThebesLayer();
    thebes->InvalidateRegion(thebes->GetValidRegion().GetBounds());
  }

  
  nsIntRect layerBounds = aLayer->GetVisibleRegion().GetBounds();
  if (aClip.GetRoundedRectCount() == 0 ||
      aRoundedRectClipCount == 0 ||
      layerBounds.IsEmpty()) {
    SetClipCount(thebesData, 0);
    return;
  }

  
  nsRefPtr<ImageLayer> maskLayer =  CreateOrRecycleMaskImageLayerFor(aLayer);
  MaskLayerUserData* userData = GetMaskLayerUserData(maskLayer);

  MaskLayerUserData newData;
  aClip.AppendRoundedRects(&newData.mRoundedClipRects, aRoundedRectClipCount);
  newData.mScaleX = mParameters.mXScale;
  newData.mScaleY = mParameters.mYScale;
  newData.mOffset = mParameters.mOffset;
  newData.mAppUnitsPerDevPixel = mContainerFrame->PresContext()->AppUnitsPerDevPixel();

  if (*userData == newData) {
    aLayer->SetMaskLayer(maskLayer);
    SetClipCount(thebesData, aRoundedRectClipCount);
    return;
  }

  
  gfx::Rect boundingRect = CalculateBounds(newData.mRoundedClipRects,
                                           newData.mAppUnitsPerDevPixel);
  boundingRect.Scale(mParameters.mXScale, mParameters.mYScale);

  uint32_t maxSize = mManager->GetMaxTextureSize();
  NS_ASSERTION(maxSize > 0, "Invalid max texture size");
  gfx::Size surfaceSize(std::min<gfx::Float>(boundingRect.Width(), maxSize),
                        std::min<gfx::Float>(boundingRect.Height(), maxSize));

  
  
  
  
  gfx::Matrix maskTransform;
  maskTransform.Scale(surfaceSize.width/boundingRect.Width(),
                      surfaceSize.height/boundingRect.Height());
  gfx::Point p = boundingRect.TopLeft();
  maskTransform.Translate(-p.x, -p.y);
  
  gfx::Matrix imageTransform = maskTransform;
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
    IntSize surfaceSizeInt(NSToIntCeil(surfaceSize.width),
                           NSToIntCeil(surfaceSize.height));
    
    RefPtr<DrawTarget> dt =
      aLayer->Manager()->CreateOptimalMaskDrawTarget(surfaceSizeInt);

    
    if (!dt) {
      NS_WARNING("Could not create DrawTarget for mask layer.");
      SetClipCount(thebesData, 0);
      return;
    }

    nsRefPtr<gfxContext> context = new gfxContext(dt);
    context->Multiply(ThebesMatrix(imageTransform));

    
    context->SetColor(gfxRGBA(1, 1, 1, 1));
    aClip.DrawRoundedRectsTo(context,
                             newData.mAppUnitsPerDevPixel,
                             0,
                             aRoundedRectClipCount);

    RefPtr<SourceSurface> surface = dt->Snapshot();

    
    container = aLayer->Manager()->CreateImageContainer();
    NS_ASSERTION(container, "Could not create image container for mask layer.");
    nsRefPtr<Image> image = container->CreateImage(ImageFormat::CAIRO_SURFACE);
    NS_ASSERTION(image, "Could not create image container for mask layer.");
    CairoImage::Data data;
    data.mSize = surfaceSizeInt;
    data.mSourceSurface = surface;

    static_cast<CairoImage*>(image.get())->SetData(data);
    container->SetCurrentImageInTransaction(image);

    GetMaskLayerImageCache()->PutImage(newKey.forget(), container);
  }

  maskLayer->SetContainer(container);

  maskTransform.Invert();
  Matrix4x4 matrix = Matrix4x4::From2D(maskTransform);
  matrix.Translate(mParameters.mOffset.x, mParameters.mOffset.y, 0);
  maskLayer->SetBaseTransform(matrix);

  
  userData->mScaleX = newData.mScaleX;
  userData->mScaleY = newData.mScaleY;
  userData->mOffset = newData.mOffset;
  userData->mAppUnitsPerDevPixel = newData.mAppUnitsPerDevPixel;
  userData->mRoundedClipRects.SwapElements(newData.mRoundedClipRects);
  userData->mImageKey = lookupKey;

  aLayer->SetMaskLayer(maskLayer);
  SetClipCount(thebesData, aRoundedRectClipCount);
  return;
}

} 
