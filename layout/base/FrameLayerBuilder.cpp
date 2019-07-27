




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
#include "mozilla/LookAndFeel.h"
#include "nsDocShell.h"
#include "nsImageFrame.h"
#include "mozilla/dom/ProfileTimelineMarkerBinding.h"

#include "GeckoProfiler.h"
#include "mozilla/gfx/Tools.h"
#include "mozilla/gfx/2D.h"
#include "gfxPrefs.h"
#include "LayersLogging.h"
#include "mozilla/unused.h"

#include <algorithm>

using namespace mozilla::layers;
using namespace mozilla::gfx;

namespace mozilla {

class PaintedDisplayItemLayerUserData;

FrameLayerBuilder::DisplayItemData::DisplayItemData(LayerManagerData* aParent, uint32_t aKey,
                                                    nsIFrame* aFrame)

  : mParent(aParent)
  , mLayer(nullptr)
  , mDisplayItemKey(aKey)
  , mItem(nullptr)
  , mUsed(true)
  , mIsInvalid(false)
{
  if (aFrame) {
    AddFrame(aFrame);
  }
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
  mItem = toCopy.mItem;
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
FrameLayerBuilder::DisplayItemData::EndUpdate()
{
  MOZ_ASSERT(!mItem);
  mIsInvalid = false;
  mUsed = false;
}

void
FrameLayerBuilder::DisplayItemData::EndUpdate(nsAutoPtr<nsDisplayItemGeometry> aGeometry)
{
  MOZ_ASSERT(mItem);

  mGeometry = aGeometry;
  mClip = mItem->GetClip();
  mFrameListChanges.Clear();

  mItem = nullptr;
  EndUpdate();
}

void
FrameLayerBuilder::DisplayItemData::BeginUpdate(Layer* aLayer, LayerState aState,
                                                uint32_t aContainerLayerGeneration,
                                                nsDisplayItem* aItem )
{
  mLayer = aLayer;
  mOptLayer = nullptr;
  mInactiveManager = nullptr;
  mLayerState = aState;
  mContainerLayerGeneration = aContainerLayerGeneration;
  mUsed = true;

  if (aLayer->AsPaintedLayer()) {
    mItem = aItem;
  }

  if (!aItem) {
    return;
  }

  
  
  nsAutoTArray<nsIFrame*, 4> copy(mFrameList);
  if (!copy.RemoveElement(aItem->Frame())) {
    AddFrame(aItem->Frame());
    mFrameListChanges.AppendElement(aItem->Frame());
  }

  nsAutoTArray<nsIFrame*,4> mergedFrames;
  aItem->GetMergedFrames(&mergedFrames);
  for (uint32_t i = 0; i < mergedFrames.Length(); ++i) {
    if (!copy.RemoveElement(mergedFrames[i])) {
      AddFrame(mergedFrames[i]);
      mFrameListChanges.AppendElement(mergedFrames[i]);
    }
  }

  for (uint32_t i = 0; i < copy.Length(); i++) {
    RemoveFrame(copy[i]);
    mFrameListChanges.AppendElement(copy[i]);
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

const nsTArray<nsIFrame*>&
FrameLayerBuilder::DisplayItemData::GetFrameListChanges()
{
  return mFrameListChanges;
}




class LayerManagerData : public LayerUserData {
public:
  explicit LayerManagerData(LayerManager *aManager)
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




struct PossiblyInfiniteRegion
{
  PossiblyInfiniteRegion() : mIsInfinite(false) {}
  MOZ_IMPLICIT PossiblyInfiniteRegion(const nsIntRegion& aRegion)
   : mRegion(aRegion)
   , mIsInfinite(false)
  {}
  MOZ_IMPLICIT PossiblyInfiniteRegion(const nsIntRect& aRect)
   : mRegion(aRect)
   , mIsInfinite(false)
  {}

  
  static PossiblyInfiniteRegion InfiniteRegion()
  {
    PossiblyInfiniteRegion r;
    r.mIsInfinite = true;
    return r;
  }

  bool IsInfinite() const { return mIsInfinite; }
  bool Intersects(const nsIntRegion& aRegion) const
  {
    if (IsInfinite()) {
      return true;
    }
    return !mRegion.Intersect(aRegion).IsEmpty();
  }

  void AccumulateAndSimplifyOutward(const PossiblyInfiniteRegion& aRegion)
  {
    if (!IsInfinite()) {
      if (aRegion.IsInfinite()) {
        mIsInfinite = true;
        mRegion.SetEmpty();
      } else {
        mRegion.OrWith(aRegion.mRegion);
        mRegion.SimplifyOutward(8);
      }
    }
  }

protected:
  nsIntRegion mRegion;
  bool mIsInfinite;
};










class PaintedLayerData {
public:
  PaintedLayerData() :
    mAnimatedGeometryRoot(nullptr),
    mIsAsyncScrollable(false),
    mFixedPosFrameForLayerData(nullptr),
    mReferenceFrame(nullptr),
    mLayer(nullptr),
    mIsSolidColorInVisibleRegion(false),
    mFontSmoothingBackgroundColor(NS_RGBA(0,0,0,0)),
    mSingleItemFixedToViewport(false),
    mNeedComponentAlpha(false),
    mForceTransparentSurface(false),
    mHideAllLayersBelow(false),
    mOpaqueForAnimatedGeometryRootParent(false),
    mImage(nullptr),
    mCommonClipCount(-1),
    mNewChildLayersIndex(-1)
  {}

#ifdef MOZ_DUMP_PAINTING
  


  nsAutoCString mLog;

  #define FLB_LOG_PAINTED_LAYER_DECISION(pld, ...) \
          if (gfxPrefs::LayersDumpDecision()) { \
            pld->mLog.AppendPrintf("\t\t\t\t"); \
            pld->mLog.AppendPrintf(__VA_ARGS__); \
          }
#else
  #define FLB_LOG_PAINTED_LAYER_DECISION(...)
#endif

  






  void Accumulate(ContainerState* aState,
                  nsDisplayItem* aItem,
                  const nsIntRegion& aClippedOpaqueRegion,
                  const nsIntRect& aVisibleRect,
                  const DisplayItemClip& aClip);
  const nsIFrame* GetAnimatedGeometryRoot() { return mAnimatedGeometryRoot; }

  



  void AccumulateEventRegions(nsDisplayLayerEventRegions* aEventRegions)
  {
    FLB_LOG_PAINTED_LAYER_DECISION(this, "Accumulating event regions %p against pld=%p\n", aEventRegions, this);

    mHitRegion.Or(mHitRegion, aEventRegions->HitRegion());
    mMaybeHitRegion.Or(mMaybeHitRegion, aEventRegions->MaybeHitRegion());
    mDispatchToContentHitRegion.Or(mDispatchToContentHitRegion, aEventRegions->DispatchToContentHitRegion());
  }

  




  already_AddRefed<ImageContainer> CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder);

  bool VisibleAboveRegionIntersects(const nsIntRegion& aRegion) const
  {
    return mVisibleAboveRegion.Intersects(aRegion);
  }

  bool VisibleRegionIntersects(const nsIntRect& aRect) const
  {
    return IsSubjectToAsyncTransforms() || mVisibleRegion.Intersects(aRect);
  }

  bool IsSubjectToAsyncTransforms() const
  {
    return mFixedPosFrameForLayerData != nullptr
        || mIsAsyncScrollable;
  }

  




  nsIntRegion  mVisibleRegion;
  



  nsIntRegion  mOpaqueRegion;
  


  nsRegion  mHitRegion;
  


  nsRegion  mMaybeHitRegion;
  


  nsRegion  mDispatchToContentHitRegion;
  




  const nsIFrame* mAnimatedGeometryRoot;
  




  bool mIsAsyncScrollable;
  




  const nsIFrame* mFixedPosFrameForLayerData;
  const nsIFrame* mReferenceFrame;
  PaintedLayer* mLayer;
  



  nscolor      mSolidColor;
  


  bool mIsSolidColorInVisibleRegion;
  



  nscolor mFontSmoothingBackgroundColor;
  



  bool mSingleItemFixedToViewport;
  



  bool mNeedComponentAlpha;
  





  bool mForceTransparentSurface;
  


  bool mHideAllLayersBelow;
  





  bool mOpaqueForAnimatedGeometryRootParent;

  



  nsDisplayImageContainer* mImage;
  







  DisplayItemClip mItemClip;
  





  int32_t mCommonClipCount;
  


  int32_t mNewChildLayersIndex;
  





  void UpdateCommonClipCount(const DisplayItemClip& aCurrentClip);
  


  nsIntRect mBounds;
  






  PossiblyInfiniteRegion mVisibleAboveRegion;

};

struct NewLayerEntry {
  NewLayerEntry()
    : mAnimatedGeometryRoot(nullptr)
    , mFixedPosFrameForLayerData(nullptr)
    , mLayerContentsVisibleRect(0, 0, -1, -1)
    , mHideAllLayersBelow(false)
    , mOpaqueForAnimatedGeometryRootParent(false)
    , mPropagateComponentAlphaFlattening(true)
  {}
  
  
  nsRefPtr<Layer> mLayer;
  const nsIFrame* mAnimatedGeometryRoot;
  const nsIFrame* mFixedPosFrameForLayerData;
  
  
  UniquePtr<FrameMetrics> mBaseFrameMetrics;
  
  
  
  nsIntRegion mVisibleRegion;
  nsIntRegion mOpaqueRegion;
  
  
  nsIntRect mLayerContentsVisibleRect;
  bool mHideAllLayersBelow;
  
  
  
  
  
  
  
  
  bool mOpaqueForAnimatedGeometryRootParent;

  
  
  bool mPropagateComponentAlphaFlattening;
};





class ContainerState {
public:
  ContainerState(nsDisplayListBuilder* aBuilder,
                 LayerManager* aManager,
                 FrameLayerBuilder* aLayerBuilder,
                 nsIFrame* aContainerFrame,
                 nsDisplayItem* aContainerItem,
                 const nsRect& aContainerBounds,
                 ContainerLayer* aContainerLayer,
                 const ContainerLayerParameters& aParameters,
                 bool aFlattenToSingleLayer,
                 nscolor aBackgroundColor) :
    mBuilder(aBuilder), mManager(aManager),
    mLayerBuilder(aLayerBuilder),
    mContainerFrame(aContainerFrame),
    mContainerLayer(aContainerLayer),
    mContainerBounds(aContainerBounds),
    mParameters(aParameters),
    mNextFreeRecycledPaintedLayer(0),
    mContainerUniformBackgroundColor(aBackgroundColor),
    mFlattenToSingleLayer(aFlattenToSingleLayer)
  {
    nsPresContext* presContext = aContainerFrame->PresContext();
    mAppUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    mContainerReferenceFrame =
      const_cast<nsIFrame*>(aContainerItem ? aContainerItem->ReferenceFrameForChildren() :
                                             mBuilder->FindReferenceFrameFor(mContainerFrame));
    mContainerAnimatedGeometryRoot = aContainerItem
      ? nsLayoutUtils::GetAnimatedGeometryRootFor(aContainerItem, aBuilder, aManager)
      : mContainerReferenceFrame;
    NS_ASSERTION(!aContainerItem || !aContainerItem->ShouldFixToViewport(aManager),
                 "Container items never return true for ShouldFixToViewport");
    mContainerFixedPosFrame =
        FindFixedPosFrameForLayerData(mContainerAnimatedGeometryRoot, false);
    
    
    
    mSnappingEnabled = aManager->IsSnappingEffectiveTransforms() &&
      !mParameters.AllowResidualTranslation();
    CollectOldLayers();
  }

  ~ContainerState()
  {
    MOZ_ASSERT(mHoistedItems.IsEmpty());
  }

  



  void ProcessDisplayItems(nsDisplayList* aList);
  







  void Finish(uint32_t *aTextContentFlags, LayerManagerData* aData,
              const nsIntRect& aContainerPixelBounds,
              nsDisplayList* aChildItems, bool& aHasComponentAlphaChildren);

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

  nsIFrame* GetContainerFrame() const { return mContainerFrame; }

  





  void SetOuterVisibleRegionForLayer(Layer* aLayer,
                                     const nsIntRegion& aOuterVisibleRegion,
                                     const nsIntRect* aLayerContentsVisibleRect = nullptr) const;

  void AddHoistedItem(nsDisplayItem* aItem)
  {
    mHoistedItems.AppendToTop(aItem);
  }

protected:
  friend class PaintedLayerData;

  LayerManager::PaintedLayerCreationHint
    GetLayerCreationHint(const nsIFrame* aAnimatedGeometryRoot);

  





  already_AddRefed<PaintedLayer> CreateOrRecyclePaintedLayer(const nsIFrame* aAnimatedGeometryRoot,
                                                             const nsIFrame *aReferenceFrame,
                                                             const nsPoint& aTopLeft);

  



  already_AddRefed<PaintedLayer> AttemptToRecyclePaintedLayer(const nsIFrame* aAnimatedGeometryRoot,
                                                              const nsIFrame *aReferenceFrame,
                                                              const nsPoint& aTopLeft);
  


  PaintedDisplayItemLayerUserData* RecyclePaintedLayer(PaintedLayer* aLayer,
                                                       const nsIFrame* aAnimatedGeometryRoot,
                                                       bool& didResetScrollPositionForLayerPixelAlignment);

  




  void PreparePaintedLayerForUse(PaintedLayer* aLayer,
                                 PaintedDisplayItemLayerUserData* aData,
                                 const nsIFrame* aAnimatedGeometryRoot,
                                 const nsIFrame* aReferenceFrame,
                                 const nsPoint& aTopLeft,
                                 bool aDidResetScrollPositionForLayerPixelAlignment);
  



  already_AddRefed<ColorLayer> CreateOrRecycleColorLayer(PaintedLayer* aPainted);
  



  already_AddRefed<ImageLayer> CreateOrRecycleImageLayer(PaintedLayer* aPainted);
  




  already_AddRefed<ImageLayer> CreateOrRecycleMaskImageLayerFor(Layer* aLayer);
  



  void CollectOldLayers();
  




  void InvalidateForLayerChange(nsDisplayItem* aItem,
                                PaintedLayer* aNewLayer);

  







  nscolor FindOpaqueBackgroundColorFor(const nsIntRegion& aTargetVisibleRegion,
                                       int32_t aUnderPaintedLayerIndex);
  








  const nsIFrame* FindFixedPosFrameForLayerData(const nsIFrame* aAnimatedGeometryRoot,
                                                bool aDisplayItemFixedToViewport);
  


  void SetFixedPositionLayerData(Layer* aLayer,
                                 const nsIFrame* aFixedPosFrame);

  



  bool ItemCoversScrollableArea(nsDisplayItem* aItem, const nsRegion& aOpaque);

  


  void SetupScrollingMetadata(NewLayerEntry* aEntry);

  










  void PostprocessRetainedLayers(nsIntRegion* aOpaqueRegionForContainer);

  





  nsIntRegion ComputeOpaqueRect(nsDisplayItem* aItem,
                                const nsIFrame* aAnimatedGeometryRoot,
                                const nsIFrame* aFixedPosFrame,
                                const DisplayItemClip& aClip,
                                nsDisplayList* aList,
                                bool* aHideAllLayersBelow,
                                bool* aOpaqueForAnimatedGeometryRootParent);

  




  void PopPaintedLayerData();
  





  bool HasAsyncScrollableGeometryInContainer(const nsIFrame* aAnimatedGeometryRoot);
  



















  PaintedLayerData* FindPaintedLayerFor(nsDisplayItem* aItem,
                                      const nsIntRect& aVisibleRect,
                                      const nsIFrame* aAnimatedGeometryRoot,
                                      const nsPoint& aTopLeft,
                                      bool aShouldFixToViewport);
  PaintedLayerData* GetTopPaintedLayerData()
  {
    return mPaintedLayerDataStack.IsEmpty() ? nullptr
        : mPaintedLayerDataStack[mPaintedLayerDataStack.Length() - 1].get();
  }

  










  void SetupMaskLayer(Layer *aLayer, const DisplayItemClip& aClip,
                      const nsIntRegion& aLayerVisibleRegion,
                      uint32_t aRoundedRectClipCount = UINT32_MAX);

  bool ChooseAnimatedGeometryRoot(const nsDisplayList& aList,
                                  const nsIFrame **aAnimatedGeometryRoot);

  








  void UpdateVisibleAboveRegionForNewItem(const nsIntRect& aVisibleRect,
                                          bool aCanMoveFreely,
                                          const nsIntRect* aClipRectIfAny);

  








  void UpdateVisibleAboveRegionOnPop(PaintedLayerData* aData,
                                     PaintedLayerData* aNextPaintedLayerData);

  nsDisplayListBuilder*            mBuilder;
  LayerManager*                    mManager;
  FrameLayerBuilder*               mLayerBuilder;
  nsIFrame*                        mContainerFrame;
  nsIFrame*                        mContainerReferenceFrame;
  const nsIFrame*                  mContainerAnimatedGeometryRoot;
  const nsIFrame*                  mContainerFixedPosFrame;
  ContainerLayer*                  mContainerLayer;
  nsRect                           mContainerBounds;
  DebugOnly<nsRect>                mAccumulatedChildBounds;
  ContainerLayerParameters         mParameters;
  



  nsIntRegion                      mInvalidPaintedContent;
  nsAutoTArray<nsAutoPtr<PaintedLayerData>,1>  mPaintedLayerDataStack;
  








  typedef nsAutoTArray<NewLayerEntry,1> AutoLayersArray;
  AutoLayersArray                  mNewChildLayers;
  nsTArray<nsRefPtr<PaintedLayer> > mRecycledPaintedLayers;
  nsDataHashtable<nsPtrHashKey<Layer>, nsRefPtr<ImageLayer> >
    mRecycledMaskImageLayers;
  



  PossiblyInfiniteRegion           mVisibleAboveBackgroundRegion;
  uint32_t                         mNextFreeRecycledPaintedLayer;
  nscoord                          mAppUnitsPerDevPixel;
  





  nscolor                          mContainerUniformBackgroundColor;
  bool                             mSnappingEnabled;
  bool                             mFlattenToSingleLayer;
  



  nsDisplayList                    mHoistedItems;
};

class PaintedDisplayItemLayerUserData : public LayerUserData
{
public:
  PaintedDisplayItemLayerUserData() :
    mMaskClipCount(0),
    mForcedBackgroundColor(NS_RGBA(0,0,0,0)),
    mFontSmoothingBackgroundColor(NS_RGBA(0,0,0,0)),
    mXScale(1.f), mYScale(1.f),
    mAppUnitsPerDevPixel(0),
    mTranslation(0, 0),
    mAnimatedGeometryRootPosition(0, 0) {}

  




  uint32_t mMaskClipCount;

  



  nscolor mForcedBackgroundColor;

  



  nscolor mFontSmoothingBackgroundColor;

  


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










uint8_t gPaintedDisplayItemLayerUserData;





uint8_t gColorLayerUserData;





uint8_t gImageLayerUserData;





uint8_t gLayerManagerUserData;





uint8_t gMaskLayerUserData;





MaskLayerUserData* GetMaskLayerUserData(Layer* aLayer)
{
  return static_cast<MaskLayerUserData*>(aLayer->GetUserData(&gMaskLayerUserData));
}

PaintedDisplayItemLayerUserData* GetPaintedDisplayItemLayerUserData(Layer* aLayer)
{
  return static_cast<PaintedDisplayItemLayerUserData*>(
    aLayer->GetUserData(&gPaintedDisplayItemLayerUserData));
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
                        PaintedLayerData* aLayerData, ContainerState* aContainingContainerState)
{
  mDisplayListBuilder = aBuilder;
  mRootPresContext = aBuilder->RootReferenceFrame()->PresContext()->GetRootPresContext();
  if (mRootPresContext) {
    mInitialDOMGeneration = mRootPresContext->GetDOMGeneration();
  }
  mContainingPaintedLayer = aLayerData;
  mContainingContainerState = aContainingContainerState;
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
InvalidatePostTransformRegion(PaintedLayer* aLayer, const nsIntRegion& aRegion,
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
InvalidatePostTransformRegion(PaintedLayer* aLayer, const nsRect& aRect,
                              const DisplayItemClip& aClip,
                              const nsIntPoint& aTranslation)
{
  PaintedDisplayItemLayerUserData* data =
      static_cast<PaintedDisplayItemLayerUserData*>(aLayer->GetUserData(&gPaintedDisplayItemLayerUserData));

  nsRect rect = aClip.ApplyNonRoundedIntersection(aRect);

  nsIntRect pixelRect = rect.ScaleToOutsidePixels(data->mXScale, data->mYScale, data->mAppUnitsPerDevPixel);
  InvalidatePostTransformRegion(aLayer, pixelRect, aTranslation);
}


static nsIntPoint
GetTranslationForPaintedLayer(PaintedLayer* aLayer)
{
  PaintedDisplayItemLayerUserData* data =
    static_cast<PaintedDisplayItemLayerUserData*>
      (aLayer->GetUserData(&gPaintedDisplayItemLayerUserData));
  NS_ASSERTION(data, "Must be a tracked painted layer!");

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

    PaintedLayer* t = data->mLayer->AsPaintedLayer();
    if (t) {
      PaintedDisplayItemLayerUserData* paintedData =
          static_cast<PaintedDisplayItemLayerUserData*>(t->GetUserData(&gPaintedDisplayItemLayerUserData));
      if (paintedData) {
        nsRegion old = data->mGeometry->ComputeInvalidationRegion();
        nsIntRegion rgn = old.ScaleToOutsidePixels(paintedData->mXScale, paintedData->mYScale, paintedData->mAppUnitsPerDevPixel);
        rgn.MoveBy(-GetTranslationForPaintedLayer(t));
        paintedData->mRegionToInvalidate.Or(paintedData->mRegionToInvalidate, rgn);
        paintedData->mRegionToInvalidate.SimplifyOutward(8);
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
  FrameLayerBuilder* layerBuilder = static_cast<FrameLayerBuilder*>(aUserArg);
  if (!data->mUsed) {
    

    PaintedLayer* t = data->mLayer->AsPaintedLayer();
    if (t && data->mGeometry) {
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
  } else {
    layerBuilder->ComputeGeometryChangeForItem(data);
  }

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
                                  DisplayItemClip** aOldClip)
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
    return oldData->mLayer;
  }

  return nullptr;
}

void
FrameLayerBuilder::ClearCachedGeometry(nsDisplayItem* aItem)
{
  uint32_t key = aItem->GetPerFrameKey();
  nsIFrame* frame = aItem->Frame();

  DisplayItemData* oldData = GetOldLayerForFrame(frame, key);
  if (oldData) {
    oldData->mGeometry = nullptr;
  }
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
ContainerState::CreateOrRecycleColorLayer(PaintedLayer *aPainted)
{
  PaintedDisplayItemLayerUserData* data =
      static_cast<PaintedDisplayItemLayerUserData*>(aPainted->GetUserData(&gPaintedDisplayItemLayerUserData));
  nsRefPtr<ColorLayer> layer = data->mColorLayer;
  if (layer) {
    layer->SetMaskLayer(nullptr);
    layer->ClearExtraDumpInfo();
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
ContainerState::CreateOrRecycleImageLayer(PaintedLayer *aPainted)
{
  PaintedDisplayItemLayerUserData* data =
      static_cast<PaintedDisplayItemLayerUserData*>(aPainted->GetUserData(&gPaintedDisplayItemLayerUserData));
  nsRefPtr<ImageLayer> layer = data->mImageLayer;
  if (layer) {
    layer->SetMaskLayer(nullptr);
    layer->ClearExtraDumpInfo();
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
    aLayer->ClearExtraDumpInfo();
    
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
InvalidateEntirePaintedLayer(PaintedLayer* aLayer, const nsIFrame* aAnimatedGeometryRoot, const char *aReason)
{
#ifdef MOZ_DUMP_PAINTING
  if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
    printf_stderr("Invalidating entire layer %p: %s\n", aLayer, aReason);
  }
#endif
  nsIntRect invalidate = aLayer->GetValidRegion().GetBounds();
  aLayer->InvalidateRegion(invalidate);
  aLayer->SetInvalidRectToVisibleRegion();
  ResetScrollPositionForLayerPixelAlignment(aAnimatedGeometryRoot);
}

LayerManager::PaintedLayerCreationHint
ContainerState::GetLayerCreationHint(const nsIFrame* aAnimatedGeometryRoot)
{
  
  
  if (mParameters.mInLowPrecisionDisplayPort) {
    return LayerManager::SCROLLABLE;
  }
  nsIFrame* animatedGeometryRootParent = aAnimatedGeometryRoot->GetParent();
  if (animatedGeometryRootParent &&
      animatedGeometryRootParent->GetType() == nsGkAtoms::scrollFrame) {
    return LayerManager::SCROLLABLE;
  }
  return LayerManager::NONE;
}

already_AddRefed<PaintedLayer>
ContainerState::AttemptToRecyclePaintedLayer(const nsIFrame* aAnimatedGeometryRoot,
                                             const nsIFrame* aReferenceFrame,
                                             const nsPoint& aTopLeft)
{
  if (mNextFreeRecycledPaintedLayer >= mRecycledPaintedLayers.Length()) {
    return nullptr;
  }

  
  nsRefPtr<PaintedLayer> layer = mRecycledPaintedLayers[mNextFreeRecycledPaintedLayer];
  ++mNextFreeRecycledPaintedLayer;

  
  
  if (!mManager->IsOptimizedFor(layer, GetLayerCreationHint(aAnimatedGeometryRoot))) {
    return nullptr;
  }

  bool didResetScrollPositionForLayerPixelAlignment = false;
  PaintedDisplayItemLayerUserData* data =
    RecyclePaintedLayer(layer, aAnimatedGeometryRoot,
                        didResetScrollPositionForLayerPixelAlignment);
  PreparePaintedLayerForUse(layer, data, aAnimatedGeometryRoot, aReferenceFrame,
                            aTopLeft,
                            didResetScrollPositionForLayerPixelAlignment);

  return layer.forget();
}

already_AddRefed<PaintedLayer>
ContainerState::CreateOrRecyclePaintedLayer(const nsIFrame* aAnimatedGeometryRoot,
                                            const nsIFrame* aReferenceFrame,
                                            const nsPoint& aTopLeft)
{
  
  nsRefPtr<PaintedLayer> layer =
    AttemptToRecyclePaintedLayer(aAnimatedGeometryRoot, aReferenceFrame, aTopLeft);
  if (layer) {
    return layer.forget();
  }

  LayerManager::PaintedLayerCreationHint creationHint =
    GetLayerCreationHint(aAnimatedGeometryRoot);

  
  layer = mManager->CreatePaintedLayerWithHint(creationHint);
  if (!layer) {
    return nullptr;
  }

  
  PaintedDisplayItemLayerUserData* data = new PaintedDisplayItemLayerUserData();
  layer->SetUserData(&gPaintedDisplayItemLayerUserData, data);
  ResetScrollPositionForLayerPixelAlignment(aAnimatedGeometryRoot);

  PreparePaintedLayerForUse(layer, data, aAnimatedGeometryRoot,
                            aReferenceFrame, aTopLeft, true);

  return layer.forget();
}

PaintedDisplayItemLayerUserData*
ContainerState::RecyclePaintedLayer(PaintedLayer* aLayer,
                                    const nsIFrame* aAnimatedGeometryRoot,
                                    bool& didResetScrollPositionForLayerPixelAlignment)
{
  
  
  aLayer->SetMaskLayer(nullptr);
  aLayer->ClearExtraDumpInfo();

  PaintedDisplayItemLayerUserData* data =
    static_cast<PaintedDisplayItemLayerUserData*>(
      aLayer->GetUserData(&gPaintedDisplayItemLayerUserData));
  NS_ASSERTION(data, "Recycled PaintedLayers must have user data");

  
  
  
  
  
  
  
  
  if (!FuzzyEqual(data->mXScale, mParameters.mXScale, 0.00001f) ||
      !FuzzyEqual(data->mYScale, mParameters.mYScale, 0.00001f) ||
      data->mAppUnitsPerDevPixel != mAppUnitsPerDevPixel) {
#ifdef MOZ_DUMP_PAINTING
  if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
    printf_stderr("Recycled layer %p changed scale\n", aLayer);
  }
#endif
    InvalidateEntirePaintedLayer(aLayer, aAnimatedGeometryRoot, "recycled layer changed state");
    didResetScrollPositionForLayerPixelAlignment = true;
  }
  if (!data->mRegionToInvalidate.IsEmpty()) {
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Invalidating deleted frame content from layer %p\n", aLayer);
    }
#endif
    aLayer->InvalidateRegion(data->mRegionToInvalidate);
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      nsAutoCString str;
      AppendToString(str, data->mRegionToInvalidate);
      printf_stderr("Invalidating layer %p: %s\n", aLayer, str.get());
    }
#endif
    data->mRegionToInvalidate.SetEmpty();
  }
  return data;
}

void
ContainerState::PreparePaintedLayerForUse(PaintedLayer* aLayer,
                                          PaintedDisplayItemLayerUserData* aData,
                                          const nsIFrame* aAnimatedGeometryRoot,
                                          const nsIFrame* aReferenceFrame,
                                          const nsPoint& aTopLeft,
                                          bool didResetScrollPositionForLayerPixelAlignment)
{
  PaintedLayer* layer = aLayer;
  PaintedDisplayItemLayerUserData* data = aData;

  data->mXScale = mParameters.mXScale;
  data->mYScale = mParameters.mYScale;
  data->mLastAnimatedGeometryRootOrigin = data->mAnimatedGeometryRootOrigin;
  data->mAnimatedGeometryRootOrigin = aTopLeft;
  data->mAppUnitsPerDevPixel = mAppUnitsPerDevPixel;
  layer->SetAllowResidualTranslation(mParameters.AllowResidualTranslation());

  mLayerBuilder->SavePreviousDataForLayer(layer, data->mMaskClipCount);

  
  
  nsPoint offset = aAnimatedGeometryRoot->GetOffsetToCrossDoc(aReferenceFrame);
  nscoord appUnitsPerDevPixel = aAnimatedGeometryRoot->PresContext()->AppUnitsPerDevPixel();
  gfxPoint scaledOffset(
      NSAppUnitsToDoublePixels(offset.x, appUnitsPerDevPixel)*mParameters.mXScale,
      NSAppUnitsToDoublePixels(offset.y, appUnitsPerDevPixel)*mParameters.mYScale);
  
  
  nsIntPoint pixOffset(RoundToMatchResidual(scaledOffset.x, data->mAnimatedGeometryRootPosition.x),
                       RoundToMatchResidual(scaledOffset.y, data->mAnimatedGeometryRootPosition.y));
  data->mTranslation = pixOffset;
  pixOffset += mParameters.mOffset;
  Matrix matrix = Matrix::Translation(pixOffset.x, pixOffset.y);
  layer->SetBaseTransform(Matrix4x4::From2D(matrix));

  
#ifndef MOZ_WIDGET_ANDROID
  
  
  gfxPoint animatedGeometryRootTopLeft = scaledOffset - ThebesPoint(matrix.GetTranslation()) + mParameters.mOffset;
  
  
  
  if (!animatedGeometryRootTopLeft.WithinEpsilonOf(data->mAnimatedGeometryRootPosition, SUBPIXEL_OFFSET_EPSILON)) {
    data->mAnimatedGeometryRootPosition = animatedGeometryRootTopLeft;
    InvalidateEntirePaintedLayer(layer, aAnimatedGeometryRoot, "subpixel offset");
  } else if (didResetScrollPositionForLayerPixelAlignment) {
    data->mAnimatedGeometryRootPosition = animatedGeometryRootTopLeft;
  }
#else
  unused << didResetScrollPositionForLayerPixelAlignment;
#endif
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
SetOuterVisibleRegion(Layer* aLayer, nsIntRegion* aOuterVisibleRegion,
                      const nsIntRect* aLayerContentsVisibleRect = nullptr)
{
  Matrix4x4 transform = aLayer->GetTransform();
  Matrix transform2D;
  if (transform.Is2D(&transform2D) && !transform2D.HasNonIntegerTranslation()) {
    aOuterVisibleRegion->MoveBy(-int(transform2D._31), -int(transform2D._32));
    if (aLayerContentsVisibleRect) {
      aOuterVisibleRegion->And(*aOuterVisibleRegion, *aLayerContentsVisibleRect);
    }
  } else {
    nsIntRect outerRect = aOuterVisibleRegion->GetBounds();
    
    
    Rect outerVisible(outerRect.x, outerRect.y, outerRect.width, outerRect.height);
    transform.Invert();
    gfxRect layerVisible = ThebesRect(transform.ProjectRectBounds(outerVisible));
    if (aLayerContentsVisibleRect) {
      NS_ASSERTION(aLayerContentsVisibleRect->width >= 0 &&
                   aLayerContentsVisibleRect->height >= 0,
                   "Bad layer contents rectangle");
      
      
      
      gfxRect layerContentsVisible(
          aLayerContentsVisibleRect->x, aLayerContentsVisibleRect->y,
          aLayerContentsVisibleRect->width, aLayerContentsVisibleRect->height);
      layerVisible.IntersectRect(layerVisible, layerContentsVisible);
    }
    layerVisible.RoundOut();
    nsIntRect visRect;
    if (gfxUtils::GfxRectToIntRect(layerVisible, &visRect)) {
      *aOuterVisibleRegion = visRect;
    } else  {
      aOuterVisibleRegion->SetEmpty();
    }
  }

  aLayer->SetVisibleRegion(*aOuterVisibleRegion);
}

void
ContainerState::SetOuterVisibleRegionForLayer(Layer* aLayer,
                                              const nsIntRegion& aOuterVisibleRegion,
                                              const nsIntRect* aLayerContentsVisibleRect) const
{
  nsIntRegion visRegion = aOuterVisibleRegion;
  visRegion.MoveBy(mParameters.mOffset);
  SetOuterVisibleRegion(aLayer, &visRegion, aLayerContentsVisibleRect);
}

nscolor
ContainerState::FindOpaqueBackgroundColorFor(const nsIntRegion& aTargetVisibleRegion,
                                             int32_t aUnderPaintedLayerIndex)
{
  for (int32_t i = aUnderPaintedLayerIndex - 1; i >= 0; --i) {
    PaintedLayerData* candidate = mPaintedLayerDataStack[i];
    if (candidate->VisibleAboveRegionIntersects(aTargetVisibleRegion)) {
      
      
      return NS_RGBA(0,0,0,0);
    }

    nsIntRegion intersection;
    intersection.And(candidate->mVisibleRegion, aTargetVisibleRegion);
    if (intersection.IsEmpty()) {
      
      continue;
    }

    
    
    nsIntRect deviceRect = aTargetVisibleRegion.GetBounds();
    nsRect appUnitRect = deviceRect.ToAppUnits(mAppUnitsPerDevPixel);
    appUnitRect.ScaleInverseRoundOut(mParameters.mXScale, mParameters.mYScale);

    FrameLayerBuilder::PaintedLayerItemsEntry* entry =
      mLayerBuilder->GetPaintedLayerItemsEntry(candidate->mLayer);
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
          return NS_RGBA(0,0,0,0);

      } else {
        
        
        if (!bounds.Intersects(appUnitRect))
          continue;

        if (!bounds.Contains(appUnitRect))
          return NS_RGBA(0,0,0,0);
      }

      if (item->IsInvisibleInRect(appUnitRect)) {
        continue;
      }

      if (item->GetClip().IsRectAffectedByClip(deviceRect,
                                               mParameters.mXScale,
                                               mParameters.mYScale,
                                               mAppUnitsPerDevPixel)) {
        return NS_RGBA(0,0,0,0);
      }

      nscolor color;
      if (item->IsUniform(mBuilder, &color) && NS_GET_A(color) == 255)
        return color;

      return NS_RGBA(0,0,0,0);
    }
  }
  if (mVisibleAboveBackgroundRegion.Intersects(aTargetVisibleRegion)) {
    
    return NS_RGBA(0,0,0,0);
  }
  return mContainerUniformBackgroundColor;
}

void
PaintedLayerData::UpdateCommonClipCount(
    const DisplayItemClip& aCurrentClip)
{
  if (!mLayer->Manager()->IsWidgetLayerManager()) {
    return;
  }

  if (mCommonClipCount >= 0) {
    mCommonClipCount = mItemClip.GetCommonRoundedRectCount(aCurrentClip, mCommonClipCount);
  } else {
    
    mCommonClipCount = aCurrentClip.GetRoundedRectCount();
  }
}

already_AddRefed<ImageContainer>
PaintedLayerData::CanOptimizeImageLayer(nsDisplayListBuilder* aBuilder)
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
CanOptimizeAwayPaintedLayer(PaintedLayerData* aData,
                           FrameLayerBuilder* aLayerBuilder)
{
  if (!aLayerBuilder->IsBuildingRetainedLayers()) {
    return false;
  }

  
  
  
  if (aData->mLayer->GetValidRegion().IsEmpty()) {
    return true;
  }

  
  
  
  
  return aLayerBuilder->CheckInLayerTreeCompressionMode();
}

#ifdef DEBUG
static int32_t FindIndexOfLayerIn(nsTArray<NewLayerEntry>& aArray,
                                  Layer* aLayer)
{
  for (uint32_t i = 0; i < aArray.Length(); ++i) {
    if (aArray[i].mLayer == aLayer) {
      return i;
    }
  }
  return -1;
}
#endif

void
ContainerState::PopPaintedLayerData()
{
  NS_ASSERTION(!mPaintedLayerDataStack.IsEmpty(), "Can't pop");

  int32_t lastIndex = mPaintedLayerDataStack.Length() - 1;
  PaintedLayerData* data = mPaintedLayerDataStack[lastIndex];

  NewLayerEntry* newLayerEntry = &mNewChildLayers[data->mNewChildLayersIndex];
  nsRefPtr<Layer> layer;
  nsRefPtr<ImageContainer> imageContainer = data->CanOptimizeImageLayer(mBuilder);

  FLB_LOG_PAINTED_LAYER_DECISION(data, "Selecting layer for pld=%p\n", data);
  FLB_LOG_PAINTED_LAYER_DECISION(data, "  Solid=%i, hasImage=%i, canOptimizeAwayPaintedLayer=%i\n",
          data->mIsSolidColorInVisibleRegion, !!imageContainer,
          CanOptimizeAwayPaintedLayer(data, mLayerBuilder));

  if ((data->mIsSolidColorInVisibleRegion || imageContainer) &&
      CanOptimizeAwayPaintedLayer(data, mLayerBuilder)) {
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
      FLB_LOG_PAINTED_LAYER_DECISION(data, "  Selected image layer=%p\n", layer.get());
    } else {
      nsRefPtr<ColorLayer> colorLayer = CreateOrRecycleColorLayer(data->mLayer);
      colorLayer->SetColor(data->mSolidColor);

      
      colorLayer->SetBaseTransform(data->mLayer->GetBaseTransform());
      colorLayer->SetPostScale(data->mLayer->GetPostXScale(), data->mLayer->GetPostYScale());

      nsIntRect visibleRect = data->mVisibleRegion.GetBounds();
      visibleRect.MoveBy(-GetTranslationForPaintedLayer(data->mLayer));
      colorLayer->SetBounds(visibleRect);
      colorLayer->SetClipRect(nullptr);

      layer = colorLayer;
      FLB_LOG_PAINTED_LAYER_DECISION(data, "  Selected color layer=%p\n", layer.get());
    }

    NS_ASSERTION(FindIndexOfLayerIn(mNewChildLayers, layer) < 0,
                 "Layer already in list???");
    NS_ASSERTION(newLayerEntry->mLayer == data->mLayer,
                 "Painted layer at wrong index");
    
    newLayerEntry = &mNewChildLayers[data->mNewChildLayersIndex + 1];
    NS_ASSERTION(!newLayerEntry->mLayer, "Slot already occupied?");
    newLayerEntry->mLayer = layer;
    newLayerEntry->mAnimatedGeometryRoot = data->mAnimatedGeometryRoot;
    newLayerEntry->mFixedPosFrameForLayerData = data->mFixedPosFrameForLayerData;

    
    
    nsIntRect emptyRect;
    data->mLayer->SetClipRect(&emptyRect);
    data->mLayer->SetVisibleRegion(nsIntRegion());
    data->mLayer->InvalidateRegion(data->mLayer->GetValidRegion().GetBounds());
    data->mLayer->SetEventRegions(EventRegions());
  } else {
    layer = data->mLayer;
    imageContainer = nullptr;
    layer->SetClipRect(nullptr);
    FLB_LOG_PAINTED_LAYER_DECISION(data, "  Selected painted layer=%p\n", layer.get());
  }

  if (mLayerBuilder->IsBuildingRetainedLayers()) {
    newLayerEntry->mVisibleRegion = data->mVisibleRegion;
    newLayerEntry->mOpaqueRegion = data->mOpaqueRegion;
    newLayerEntry->mHideAllLayersBelow = data->mHideAllLayersBelow;
    newLayerEntry->mOpaqueForAnimatedGeometryRootParent = data->mOpaqueForAnimatedGeometryRootParent;
  } else {
    SetOuterVisibleRegionForLayer(layer, data->mVisibleRegion);
  }

  nsIntRect layerBounds = data->mBounds;
  layerBounds.MoveBy(-GetTranslationForPaintedLayer(data->mLayer));
  layer->SetLayerBounds(layerBounds);

#ifdef MOZ_DUMP_PAINTING
  if (PaintedLayerData* containingPld = mLayerBuilder->GetContainingPaintedLayerData()) {
    containingPld->mLayer->AddExtraDumpInfo(nsCString(data->mLog));
  } else {
    layer->AddExtraDumpInfo(nsCString(data->mLog));
  }
#endif

  nsIntRegion transparentRegion;
  transparentRegion.Sub(data->mVisibleRegion, data->mOpaqueRegion);
  bool isOpaque = transparentRegion.IsEmpty();
  
  
  
  if (layer == data->mLayer) {
    nscolor backgroundColor = NS_RGBA(0,0,0,0);
    if (!isOpaque) {
      backgroundColor = FindOpaqueBackgroundColorFor(data->mVisibleRegion, lastIndex);
      if (NS_GET_A(backgroundColor) == 255) {
        isOpaque = true;
      }
    }

    
    PaintedDisplayItemLayerUserData* userData =
      GetPaintedDisplayItemLayerUserData(data->mLayer);
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

    userData->mFontSmoothingBackgroundColor = data->mFontSmoothingBackgroundColor;

    
    
    
    int32_t commonClipCount = std::max(0, data->mCommonClipCount);
    SetupMaskLayer(layer, data->mItemClip, data->mVisibleRegion, commonClipCount);
    
    FrameLayerBuilder::PaintedLayerItemsEntry* entry = mLayerBuilder->
      GetPaintedLayerItemsEntry(static_cast<PaintedLayer*>(layer.get()));
    entry->mCommonClipCount = commonClipCount;
  } else {
    
    SetupMaskLayer(layer, data->mItemClip, data->mVisibleRegion);
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

  PaintedLayerData* containingPaintedLayerData =
     mLayerBuilder->GetContainingPaintedLayerData();
  if (containingPaintedLayerData) {
    if (!data->mDispatchToContentHitRegion.GetBounds().IsEmpty()) {
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mDispatchToContentHitRegion.GetBounds(),
        containingPaintedLayerData->mReferenceFrame);
      containingPaintedLayerData->mDispatchToContentHitRegion.Or(
        containingPaintedLayerData->mDispatchToContentHitRegion, rect);
    }
    if (!data->mMaybeHitRegion.GetBounds().IsEmpty()) {
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mMaybeHitRegion.GetBounds(),
        containingPaintedLayerData->mReferenceFrame);
      containingPaintedLayerData->mMaybeHitRegion.Or(
        containingPaintedLayerData->mMaybeHitRegion, rect);
    }
    if (!data->mHitRegion.GetBounds().IsEmpty()) {
      
      
      Matrix4x4 matrix = nsLayoutUtils::GetTransformToAncestor(
        mContainerReferenceFrame, containingPaintedLayerData->mReferenceFrame);
      Matrix matrix2D;
      bool isPrecise = matrix.Is2D(&matrix2D) && !matrix2D.HasNonAxisAlignedTransform();
      nsRect rect = nsLayoutUtils::TransformFrameRectToAncestor(
        mContainerReferenceFrame,
        data->mHitRegion.GetBounds(),
        containingPaintedLayerData->mReferenceFrame);
      nsRegion* dest = isPrecise ? &containingPaintedLayerData->mHitRegion
                                 : &containingPaintedLayerData->mMaybeHitRegion;
      dest->Or(*dest, rect);
    }
  } else {
    EventRegions regions;
    regions.mHitRegion = ScaleRegionToOutsidePixels(data->mHitRegion);
    
    
    
    
    nsIntRegion maybeHitRegion = ScaleRegionToOutsidePixels(data->mMaybeHitRegion);
    regions.mDispatchToContentHitRegion.Sub(maybeHitRegion, regions.mHitRegion);
    regions.mDispatchToContentHitRegion.OrWith(
        ScaleRegionToOutsidePixels(data->mDispatchToContentHitRegion));
    regions.mHitRegion.OrWith(maybeHitRegion);

    nsIntPoint translation = -GetTranslationForPaintedLayer(data->mLayer);
    regions.mHitRegion.MoveBy(translation);
    regions.mDispatchToContentHitRegion.MoveBy(translation);

    layer->SetEventRegions(regions);
  }

  
  
  
  
  
  UpdateVisibleAboveRegionOnPop(data,
    lastIndex > 0 ? mPaintedLayerDataStack[lastIndex - 1].get() : nullptr);

  mPaintedLayerDataStack.RemoveElementAt(lastIndex);
}

static bool
IsItemAreaInWindowOpaqueRegion(nsDisplayListBuilder* aBuilder,
                               nsDisplayItem* aItem,
                               const nsRect& aComponentAlphaBounds)
{
  if (!aItem->Frame()->PresContext()->IsChrome()) {
    
    return true;
  }
  if (aItem->ReferenceFrame() != aBuilder->RootReferenceFrame()) {
    
    
    
    
    return false;
  }
  return aBuilder->GetWindowOpaqueRegion().Contains(aComponentAlphaBounds);
}

void
PaintedLayerData::Accumulate(ContainerState* aState,
                            nsDisplayItem* aItem,
                            const nsIntRegion& aClippedOpaqueRegion,
                            const nsIntRect& aVisibleRect,
                            const DisplayItemClip& aClip)
{
  FLB_LOG_PAINTED_LAYER_DECISION(this, "Accumulating dp=%s(%p), f=%p against pld=%p\n", aItem->Name(), aItem, aItem->Frame(), this);

  bool snap;
  nsRect itemBounds = aItem->GetBounds(aState->mBuilder, &snap);
  mBounds = mBounds.Union(aState->ScaleToOutsidePixels(itemBounds, snap));

  if (aState->mBuilder->NeedToForceTransparentSurfaceForItem(aItem)) {
    mForceTransparentSurface = true;
  }
  if (aState->mParameters.mDisableSubpixelAntialiasingInDescendants) {
    
    
    
    aItem->DisableComponentAlpha();
  }

  bool clipMatches = mItemClip == aClip;
  mItemClip = aClip;

  if (!mIsSolidColorInVisibleRegion && mOpaqueRegion.Contains(aVisibleRect) &&
      mVisibleRegion.Contains(aVisibleRect) && !mImage) {
    
    
    
    
    
    
    
    
    return;
  }

  


  if (nsIntRegion(aVisibleRect).Contains(mVisibleRegion) &&
      aClippedOpaqueRegion.Contains(mVisibleRegion) &&
      aItem->SupportsOptimizingToImage()) {
    mImage = static_cast<nsDisplayImageContainer*>(aItem);
    FLB_LOG_PAINTED_LAYER_DECISION(this, "  Tracking image: nsDisplayImageContainer covers the layer\n");
  } else if (mImage) {
    FLB_LOG_PAINTED_LAYER_DECISION(this, "  No longer tracking image\n");
    mImage = nullptr;
  }

  bool isFirstVisibleItem = mVisibleRegion.IsEmpty();
  if (isFirstVisibleItem) {
    nscolor fontSmoothingBGColor;
    if (aItem->ProvidesFontSmoothingBackgroundColor(aState->mBuilder,
                                                    &fontSmoothingBGColor)) {
      mFontSmoothingBackgroundColor = fontSmoothingBGColor;
    }
  }

  nscolor uniformColor;
  bool isUniform = aItem->IsUniform(aState->mBuilder, &uniformColor);

  
  
  
  if (!isUniform || NS_GET_A(uniformColor) > 0) {
    
    
    
    if (isUniform) {
      bool snap;
      nsRect bounds = aItem->GetBounds(aState->mBuilder, &snap);
      if (!aState->ScaleToInsidePixels(bounds, snap).Contains(aVisibleRect)) {
        isUniform = false;
        FLB_LOG_PAINTED_LAYER_DECISION(this, "  Display item does not cover the visible rect\n");
      }
    }
    if (isUniform) {
      if (isFirstVisibleItem) {
        
        mSolidColor = uniformColor;
        mIsSolidColorInVisibleRegion = true;
      } else if (mIsSolidColorInVisibleRegion &&
                 mVisibleRegion.IsEqual(nsIntRegion(aVisibleRect)) &&
                 clipMatches) {
        
        mSolidColor = NS_ComposeColors(mSolidColor, uniformColor);
      } else {
        FLB_LOG_PAINTED_LAYER_DECISION(this, "  Layer not a solid color: Can't blend colors togethers\n");
        mIsSolidColorInVisibleRegion = false;
      }
    } else {
      FLB_LOG_PAINTED_LAYER_DECISION(this, "  Layer is not a solid color: Display item is not uniform over the visible bound\n");
      mIsSolidColorInVisibleRegion = false;
    }

    mVisibleRegion.Or(mVisibleRegion, aVisibleRect);
    mVisibleRegion.SimplifyOutward(4);
  }

  if (!aClippedOpaqueRegion.IsEmpty()) {
    nsIntRegionRectIterator iter(aClippedOpaqueRegion);
    for (const nsIntRect* r = iter.Next(); r; r = iter.Next()) {
      
      
      
      
      
      nsIntRegion tmp;
      tmp.Or(mOpaqueRegion, *r);
       
       
       
       if (tmp.GetNumRects() <= 4 || aItem->Frame()->PresContext()->IsChrome()) {
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
        if (IsItemAreaInWindowOpaqueRegion(aState->mBuilder, aItem,
              componentAlpha.Intersect(aItem->GetVisibleRect()))) {
          mNeedComponentAlpha = true;
        } else {
          aItem->DisableComponentAlpha();
        }
      }
    }
  }
}

bool
ContainerState::HasAsyncScrollableGeometryInContainer(const nsIFrame* aAnimatedGeometryRoot)
{
  const nsIFrame* f = aAnimatedGeometryRoot;
  while (f) {
    if (nsLayoutUtils::GetScrollableFrameFor(f) &&
        nsLayoutUtils::GetDisplayPort(f->GetContent(), nullptr)) {
      return true;
    }
    if (f == mContainerAnimatedGeometryRoot) {
      break;
    }
    nsIFrame* fParent = nsLayoutUtils::GetCrossDocParentFrame(f);
    if (!fParent) {
      break;
    }
    f = nsLayoutUtils::GetAnimatedGeometryRootForFrame(
          this->mBuilder, fParent, mContainerAnimatedGeometryRoot);
  }
  return false;
}

PaintedLayerData*
ContainerState::FindPaintedLayerFor(nsDisplayItem* aItem,
                                   const nsIntRect& aVisibleRect,
                                   const nsIFrame* aAnimatedGeometryRoot,
                                   const nsPoint& aTopLeft,
                                   bool aShouldFixToViewport)
{
  int32_t i;
  int32_t lowestUsableLayerWithScrolledRoot = -1;
  int32_t topmostLayerWithScrolledRoot = -1;
  for (i = mPaintedLayerDataStack.Length() - 1; i >= 0; --i) {
    
    if (aShouldFixToViewport) {
      ++i;
      break;
    }
    PaintedLayerData* data = mPaintedLayerDataStack[i];
    
    
    
    if (data->VisibleAboveRegionIntersects(aVisibleRect)) {
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
    
    
    
    if (data->VisibleRegionIntersects(aVisibleRect))
      break;
  }
  if (topmostLayerWithScrolledRoot < 0) {
    --i;
    for (; i >= 0; --i) {
      PaintedLayerData* data = mPaintedLayerDataStack[i];
      if (data->mAnimatedGeometryRoot == aAnimatedGeometryRoot) {
        topmostLayerWithScrolledRoot = i;
        break;
      }
    }
  }

  if (topmostLayerWithScrolledRoot >= 0) {
    while (uint32_t(topmostLayerWithScrolledRoot + 1) < mPaintedLayerDataStack.Length()) {
      PopPaintedLayerData();
    }
  }

  PaintedLayerData* paintedLayerData = nullptr;
  if (lowestUsableLayerWithScrolledRoot < 0) {
    nsRefPtr<PaintedLayer> layer =
      CreateOrRecyclePaintedLayer(aAnimatedGeometryRoot, aItem->ReferenceFrame(), aTopLeft);

    paintedLayerData = new PaintedLayerData();
    mPaintedLayerDataStack.AppendElement(paintedLayerData);
    paintedLayerData->mLayer = layer;
    paintedLayerData->mAnimatedGeometryRoot = aAnimatedGeometryRoot;
    paintedLayerData->mFixedPosFrameForLayerData =
      FindFixedPosFrameForLayerData(aAnimatedGeometryRoot, aShouldFixToViewport);
    paintedLayerData->mIsAsyncScrollable =
      HasAsyncScrollableGeometryInContainer(aAnimatedGeometryRoot);
    paintedLayerData->mReferenceFrame = aItem->ReferenceFrame();
    paintedLayerData->mSingleItemFixedToViewport = aShouldFixToViewport;

    NS_ASSERTION(FindIndexOfLayerIn(mNewChildLayers, layer) < 0,
                 "Layer already in list???");
    paintedLayerData->mNewChildLayersIndex = mNewChildLayers.Length();
    NewLayerEntry* newLayerEntry = mNewChildLayers.AppendElement();
    newLayerEntry->mLayer = layer.forget();
    newLayerEntry->mAnimatedGeometryRoot = aAnimatedGeometryRoot;
    newLayerEntry->mFixedPosFrameForLayerData = paintedLayerData->mFixedPosFrameForLayerData;
    
    

    
    mNewChildLayers.AppendElement();
  } else {
    paintedLayerData = mPaintedLayerDataStack[lowestUsableLayerWithScrolledRoot];
  }

  return paintedLayerData;
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
    context->SetMatrix(gfxMatrix::Translation(-itemVisibleRect.x,
                                              -itemVisibleRect.y));
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
    basic->EndTransaction(FrameLayerBuilder::DrawPaintedLayer, aBuilder);
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
      nsLayoutUtils::GetAnimatedGeometryRootFor(item, mBuilder, mManager);
    return true;
  }
  return false;
}




static bool
IsScrollLayerItemAndOverlayScrollbarForScrollFrame(
  nsDisplayItem* aPotentialScrollItem, nsDisplayItem* aPotentialScrollbarItem)
{
  if (aPotentialScrollItem->GetType() == nsDisplayItem::TYPE_SCROLL_LAYER &&
      aPotentialScrollbarItem &&
      aPotentialScrollbarItem->GetType() == nsDisplayItem::TYPE_OWN_LAYER &&
      LookAndFeel::GetInt(LookAndFeel::eIntID_UseOverlayScrollbars)) {
    nsDisplayScrollLayer* scrollItem =
      static_cast<nsDisplayScrollLayer*>(aPotentialScrollItem);
    nsDisplayOwnLayer* layerItem =
      static_cast<nsDisplayOwnLayer*>(aPotentialScrollbarItem);
    if ((layerItem->GetFlags() &
         (nsDisplayOwnLayer::VERTICAL_SCROLLBAR |
          nsDisplayOwnLayer::HORIZONTAL_SCROLLBAR)) &&
        layerItem->Frame()->GetParent() == scrollItem->GetScrollFrame()) {
      return true;
    }
  }
  return false;
}

nsIntRegion
ContainerState::ComputeOpaqueRect(nsDisplayItem* aItem,
                                  const nsIFrame* aAnimatedGeometryRoot,
                                  const nsIFrame* aFixedPosFrame,
                                  const DisplayItemClip& aClip,
                                  nsDisplayList* aList,
                                  bool* aHideAllLayersBelow,
                                  bool* aOpaqueForAnimatedGeometryRootParent)
{
  bool snapOpaque;
  nsRegion opaque = aItem->GetOpaqueRegion(mBuilder, &snapOpaque);
  nsIntRegion opaquePixels;
  if (!opaque.IsEmpty()) {
    nsRegion opaqueClipped;
    nsRegionRectIterator iter(opaque);
    for (const nsRect* r = iter.Next(); r; r = iter.Next()) {
      opaqueClipped.Or(opaqueClipped, aClip.ApproximateIntersectInward(*r));
    }
    if (aAnimatedGeometryRoot == mContainerAnimatedGeometryRoot &&
        aFixedPosFrame == mContainerFixedPosFrame &&
        opaqueClipped.Contains(mContainerBounds)) {
      *aHideAllLayersBelow = true;
      aList->SetIsOpaque();
    }
    
    
    
    
    if (!nsLayoutUtils::GetCrossDocParentFrame(mContainerFrame)) {
      mBuilder->AddWindowOpaqueRegion(opaqueClipped);
    }
    opaquePixels = ScaleRegionToInsidePixels(opaqueClipped, snapOpaque);

    nsIScrollableFrame* sf = nsLayoutUtils::GetScrollableFrameFor(aAnimatedGeometryRoot);
    if (sf) {
      nsRect displayport;
      bool usingDisplayport =
        nsLayoutUtils::GetDisplayPort(aAnimatedGeometryRoot->GetContent(), &displayport);
      if (!usingDisplayport) {
        
        
        displayport = sf->GetScrollPortRect();
      }
      nsIFrame* scrollFrame = do_QueryFrame(sf);
      displayport += scrollFrame->GetOffsetToCrossDoc(mContainerReferenceFrame);
      if (opaque.Contains(displayport)) {
        *aOpaqueForAnimatedGeometryRootParent = true;
      }
    }
  }
  return opaquePixels;
}

void
ContainerState::UpdateVisibleAboveRegionForNewItem(const nsIntRect& aVisibleRect,
                                                   bool aCanMoveFreely,
                                                   const nsIntRect* aClipRectIfAny)
{
  PaintedLayerData* data = GetTopPaintedLayerData();
  PossiblyInfiniteRegion& visibleAboveRegion = data
    ? data->mVisibleAboveRegion : mVisibleAboveBackgroundRegion;

  if (aCanMoveFreely) {
    
    
    
    
    
    
    
    visibleAboveRegion.AccumulateAndSimplifyOutward(
      aClipRectIfAny ? *aClipRectIfAny : PossiblyInfiniteRegion::InfiniteRegion());
  } else {
    visibleAboveRegion.AccumulateAndSimplifyOutward(aVisibleRect);
  }
}

void
ContainerState::UpdateVisibleAboveRegionOnPop(PaintedLayerData* aData,
                                              PaintedLayerData* aNextPaintedLayerData)
{
  PossiblyInfiniteRegion& visibleAboveRegion = aNextPaintedLayerData ?
    aNextPaintedLayerData->mVisibleAboveRegion : mVisibleAboveBackgroundRegion;

  
  
  
  
  
  
  if (aData->IsSubjectToAsyncTransforms() && !aData->mVisibleRegion.IsEmpty()) {
    visibleAboveRegion.AccumulateAndSimplifyOutward(PossiblyInfiniteRegion::InfiniteRegion());
  } else {
    visibleAboveRegion.AccumulateAndSimplifyOutward(aData->mVisibleAboveRegion);
    visibleAboveRegion.AccumulateAndSimplifyOutward(aData->mVisibleRegion);
  }
}















void
ContainerState::ProcessDisplayItems(nsDisplayList* aList)
{
  PROFILER_LABEL("ContainerState", "ProcessDisplayItems",
    js::ProfileEntry::Category::GRAPHICS);

  const nsIFrame* lastAnimatedGeometryRoot = mContainerReferenceFrame;
  nsPoint topLeft(0,0);

  
  
  
  if (mFlattenToSingleLayer) {
    if (ChooseAnimatedGeometryRoot(*aList, &lastAnimatedGeometryRoot)) {
      topLeft = lastAnimatedGeometryRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
    }
  }

  int32_t maxLayers = nsDisplayItem::MaxActiveLayers();
  int layerCount = 0;

  nsDisplayList savedItems;
  nsDisplayItem* item;
  while ((item = aList->RemoveBottom()) != nullptr) {
    
    
    nsDisplayItem* aboveItem;
    while ((aboveItem = aList->GetBottom()) != nullptr) {
      if (aboveItem->TryMerge(mBuilder, item)) {
        aList->RemoveBottom();
        item->~nsDisplayItem();
        item = aboveItem;
      } else if (IsScrollLayerItemAndOverlayScrollbarForScrollFrame(aboveItem, item)) {
        
        
        
        
        aList->RemoveBottom();
        aList->AppendToBottom(item);
        item = aboveItem;
      } else {
        break;
      }
    }

    nsDisplayList* itemSameCoordinateSystemChildren
      = item->GetSameCoordinateSystemChildren();
    if (item->ShouldFlattenAway(mBuilder)) {
      aList->AppendToBottom(itemSameCoordinateSystemChildren);
      item->~nsDisplayItem();
      continue;
    }

    nsDisplayItem::Type itemType = item->GetType();
    if (itemType == nsDisplayItem::TYPE_SCROLL_INFO_LAYER &&
        mLayerBuilder->GetContainingContainerState()) {
      
      
      
      
      
      
      static_cast<nsDisplayScrollInfoLayer*>(item)->MarkHoisted();
      mLayerBuilder->GetContainingContainerState()->AddHoistedItem(item);
      continue;
    }

    savedItems.AppendToTop(item);

    NS_ASSERTION(mAppUnitsPerDevPixel == AppUnitsPerDevPixel(item),
      "items in a container layer should all have the same app units per dev pixel");

    if (mBuilder->NeedToForceTransparentSurfaceForItem(item)) {
      aList->SetNeedsTransparentSurface();
    }

    bool snap;
    nsRect itemContent = item->GetBounds(mBuilder, &snap);
    if (itemType == nsDisplayItem::TYPE_LAYER_EVENT_REGIONS) {
      nsDisplayLayerEventRegions* eventRegions =
        static_cast<nsDisplayLayerEventRegions*>(item);
      itemContent = eventRegions->GetHitRegionBounds(mBuilder, &snap);
    }
    nsIntRect itemDrawRect = ScaleToOutsidePixels(itemContent, snap);
    bool prerenderedTransform = itemType == nsDisplayItem::TYPE_TRANSFORM &&
        static_cast<nsDisplayTransform*>(item)->ShouldPrerender(mBuilder);
    nsIntRect clipRect;
    const DisplayItemClip& itemClip = item->GetClip();
    if (itemClip.HasClip()) {
      itemContent.IntersectRect(itemContent, itemClip.GetClipRect());
      clipRect = ScaleToNearestPixels(itemClip.GetClipRect());
      if (!prerenderedTransform) {
        itemDrawRect.IntersectRect(itemDrawRect, clipRect);
      }
      clipRect.MoveBy(mParameters.mOffset);
    }
#ifdef DEBUG
    nsRect bounds = itemContent;
    bool dummy;
    if (itemType == nsDisplayItem::TYPE_LAYER_EVENT_REGIONS) {
      bounds = item->GetBounds(mBuilder, &dummy);
      if (itemClip.HasClip()) {
        bounds.IntersectRect(bounds, itemClip.GetClipRect());
      }
    }
    ((nsRect&)mAccumulatedChildBounds).UnionRect(mAccumulatedChildBounds, bounds);
#endif
    
    
    
    nsIntRect itemVisibleRect = itemDrawRect.Intersect(
      ScaleToOutsidePixels(item->GetVisibleRect(), false));

    LayerState layerState = item->GetLayerState(mBuilder, mManager, mParameters);
    if (layerState == LAYER_INACTIVE &&
        nsDisplayItem::ForceActiveLayers()) {
      layerState = LAYER_ACTIVE;
    }

    bool forceInactive;
    const nsIFrame* animatedGeometryRoot;
    if (mFlattenToSingleLayer) {
      forceInactive = true;
      animatedGeometryRoot = lastAnimatedGeometryRoot;
    } else {
      forceInactive = false;
      if (mManager->IsWidgetLayerManager()) {
        animatedGeometryRoot = nsLayoutUtils::GetAnimatedGeometryRootFor(item, mBuilder, mManager);
      } else {
        
        
        
        animatedGeometryRoot = mContainerAnimatedGeometryRoot;
      }
      if (animatedGeometryRoot != lastAnimatedGeometryRoot) {
        lastAnimatedGeometryRoot = animatedGeometryRoot;
        topLeft = animatedGeometryRoot->GetOffsetToCrossDoc(mContainerReferenceFrame);
      }
    }
    bool shouldFixToViewport = !animatedGeometryRoot->GetParent() &&
      item->ShouldFixToViewport(mManager);

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

      
      
      
      
      InvalidateForLayerChange(item, nullptr);

      
      
      
      
      if (itemVisibleRect.IsEmpty() &&
          !item->ShouldBuildLayerEvenIfInvisible(mBuilder)) {
        continue;
      }

      
      
      bool mayDrawOutOfOrder = itemType == nsDisplayItem::TYPE_TRANSFORM &&
        (item->Frame()->Preserves3D() || item->Frame()->Preserves3DChildren());

      
      mParameters.mBackgroundColor = (prerenderedTransform || mayDrawOutOfOrder)
        ? NS_RGBA(0,0,0,0)
        : FindOpaqueBackgroundColorFor(itemVisibleRect, mPaintedLayerDataStack.Length());

      
      
      
      
      nsIntRect layerContentsVisibleRect(0, 0, -1, -1);
      mParameters.mLayerContentsVisibleRect = &layerContentsVisibleRect;
      nsRefPtr<Layer> ownLayer = item->BuildLayer(mBuilder, mManager, mParameters);
      if (!ownLayer) {
        continue;
      }

      NS_ASSERTION(!ownLayer->AsPaintedLayer(),
                   "Should never have created a dedicated Painted layer!");

      const nsIFrame* fixedPosFrame =
        FindFixedPosFrameForLayerData(animatedGeometryRoot, shouldFixToViewport);
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

      
      
      
      
      UpdateVisibleAboveRegionForNewItem(itemVisibleRect, prerenderedTransform,
                                         itemClip.HasClip() ? &clipRect : nullptr);

      
      
      if (itemClip.IsRectClippedByRoundedCorner(itemContent)) {
        SetupMaskLayer(ownLayer, itemClip, itemVisibleRect);
      }

      ContainerLayer* oldContainer = ownLayer->GetParent();
      if (oldContainer && oldContainer != mContainerLayer) {
        oldContainer->RemoveChild(ownLayer);
      }
      NS_ASSERTION(FindIndexOfLayerIn(mNewChildLayers, ownLayer) < 0,
                   "Layer already in list???");

      NewLayerEntry* newLayerEntry = mNewChildLayers.AppendElement();
      newLayerEntry->mLayer = ownLayer;
      newLayerEntry->mAnimatedGeometryRoot = animatedGeometryRoot;
      newLayerEntry->mFixedPosFrameForLayerData = fixedPosFrame;

      
      
      if (itemType == nsDisplayItem::TYPE_TRANSFORM ||
          layerState == LAYER_ACTIVE_FORCE) {
        newLayerEntry->mPropagateComponentAlphaFlattening = false;
      }
      
      
      
      NS_ASSERTION(itemType != nsDisplayItem::TYPE_TRANSFORM ||
                   layerContentsVisibleRect.width >= 0,
                   "Transform items must set layerContentsVisibleRect!");
      if (mLayerBuilder->IsBuildingRetainedLayers()) {
        newLayerEntry->mLayerContentsVisibleRect = layerContentsVisibleRect;
        newLayerEntry->mVisibleRegion = itemVisibleRect;
        newLayerEntry->mOpaqueRegion = ComputeOpaqueRect(item,
          animatedGeometryRoot, fixedPosFrame, itemClip, aList,
          &newLayerEntry->mHideAllLayersBelow,
          &newLayerEntry->mOpaqueForAnimatedGeometryRootParent);
      } else {
        SetOuterVisibleRegionForLayer(ownLayer, itemVisibleRect,
            layerContentsVisibleRect.width >= 0 ? &layerContentsVisibleRect : nullptr);
      }
      if (itemType == nsDisplayItem::TYPE_SCROLL_LAYER ||
          itemType == nsDisplayItem::TYPE_SCROLL_INFO_LAYER) {
        nsDisplayScrollLayer* scrollItem = static_cast<nsDisplayScrollLayer*>(item);
        newLayerEntry->mOpaqueForAnimatedGeometryRootParent =
            scrollItem->IsDisplayPortOpaque();
        newLayerEntry->mBaseFrameMetrics =
            scrollItem->ComputeFrameMetrics(ownLayer, mParameters);
      } else if ((itemType == nsDisplayItem::TYPE_SUBDOCUMENT ||
                  itemType == nsDisplayItem::TYPE_ZOOM ||
                  itemType == nsDisplayItem::TYPE_RESOLUTION) &&
                 gfxPrefs::LayoutUseContainersForRootFrames())
      {
        newLayerEntry->mBaseFrameMetrics =
          static_cast<nsDisplaySubDocument*>(item)->ComputeFrameMetrics(ownLayer, mParameters);
      }

      



      mLayerBuilder->AddLayerDisplayItem(ownLayer, item,
                                         layerState,
                                         topLeft, nullptr);
    } else {
      PaintedLayerData* paintedLayerData =
        FindPaintedLayerFor(item, itemVisibleRect, animatedGeometryRoot, topLeft,
                           shouldFixToViewport);

      if (itemType == nsDisplayItem::TYPE_LAYER_EVENT_REGIONS) {
        nsDisplayLayerEventRegions* eventRegions =
            static_cast<nsDisplayLayerEventRegions*>(item);
        paintedLayerData->AccumulateEventRegions(eventRegions);
      } else {
        
        
        paintedLayerData->UpdateCommonClipCount(itemClip);

        InvalidateForLayerChange(item, paintedLayerData->mLayer);

        mLayerBuilder->AddPaintedDisplayItem(paintedLayerData, item, itemClip,
                                            *this, layerState, topLeft);
        nsIntRegion opaquePixels = ComputeOpaqueRect(item,
            animatedGeometryRoot, paintedLayerData->mFixedPosFrameForLayerData,
            itemClip, aList,
            &paintedLayerData->mHideAllLayersBelow,
            &paintedLayerData->mOpaqueForAnimatedGeometryRootParent);
        MOZ_ASSERT(nsIntRegion(itemDrawRect).Contains(opaquePixels));
        opaquePixels.AndWith(itemVisibleRect);
        paintedLayerData->Accumulate(this, item, opaquePixels,
            itemVisibleRect, itemClip);
      }
    }

    
    
    aList->AppendToBottom(&mHoistedItems);

    if (itemSameCoordinateSystemChildren &&
        itemSameCoordinateSystemChildren->NeedsTransparentSurface()) {
      aList->SetNeedsTransparentSurface();
    }
  }

  aList->AppendToTop(&savedItems);
  MOZ_ASSERT(mHoistedItems.IsEmpty());
}

void
ContainerState::InvalidateForLayerChange(nsDisplayItem* aItem, PaintedLayer* aNewLayer)
{
  NS_ASSERTION(aItem->GetPerFrameKey(),
               "Display items that render using Thebes must have a key");
  nsDisplayItemGeometry* oldGeometry = nullptr;
  DisplayItemClip* oldClip = nullptr;
  Layer* oldLayer = mLayerBuilder->GetOldLayerFor(aItem, &oldGeometry, &oldClip);
  if (aNewLayer != oldLayer && oldLayer) {
    
    
    PaintedLayer* t = oldLayer->AsPaintedLayer();
    if (t && oldGeometry) {
      
      
      
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
    
    
    mLayerBuilder->ClearCachedGeometry(aItem);
    aItem->NotifyRenderingChanged();
  }
}

void
FrameLayerBuilder::ComputeGeometryChangeForItem(DisplayItemData* aData)
{
  nsDisplayItem *item = aData->mItem;
  PaintedLayer* paintedLayer = aData->mLayer->AsPaintedLayer();
  if (!item || !paintedLayer) {
    aData->EndUpdate();
    return;
  }

  PaintedLayerItemsEntry* entry = mPaintedLayerItems.GetEntry(paintedLayer);

  nsAutoPtr<nsDisplayItemGeometry> geometry(item->AllocateGeometry(mDisplayListBuilder));

  PaintedDisplayItemLayerUserData* layerData =
    static_cast<PaintedDisplayItemLayerUserData*>(aData->mLayer->GetUserData(&gPaintedDisplayItemLayerUserData));
  nsPoint shift = layerData->mAnimatedGeometryRootOrigin - layerData->mLastAnimatedGeometryRootOrigin;

  const DisplayItemClip& clip = item->GetClip();

  
  
  
  nsRect invalid;
  nsRegion combined;
  bool notifyRenderingChanged = true;
  if (!aData->mGeometry) {
    
    
    combined = clip.ApplyNonRoundedIntersection(geometry->ComputeInvalidationRegion());
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Display item type %s(%p) added to layer %p!\n", item->Name(), item->Frame(), aData->mLayer.get());
    }
#endif
  } else if (aData->mIsInvalid || (item->IsInvalid(invalid) && invalid.IsEmpty())) {
    
    combined = aData->mClip.ApplyNonRoundedIntersection(aData->mGeometry->ComputeInvalidationRegion());
    combined.MoveBy(shift);
    combined.Or(combined, clip.ApplyNonRoundedIntersection(geometry->ComputeInvalidationRegion()));
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      printf_stderr("Display item type %s(%p) (in layer %p) belongs to an invalidated frame!\n", item->Name(), item->Frame(), aData->mLayer.get());
    }
#endif
  } else {
    
    

    const nsTArray<nsIFrame*>& changedFrames = aData->GetFrameListChanges();

    
    
    
    
    
    
    
    
    
    if (aData->mGeometry->ComputeInvalidationRegion() == geometry->ComputeInvalidationRegion() &&
        aData->mClip == clip && invalid.IsEmpty() && changedFrames.Length() == 0) {
      notifyRenderingChanged = false;
    }

    aData->mGeometry->MoveBy(shift);
    item->ComputeInvalidationRegion(mDisplayListBuilder, aData->mGeometry, &combined);
    aData->mClip.AddOffsetAndComputeDifference(entry->mCommonClipCount,
                                               shift, aData->mGeometry->ComputeInvalidationRegion(),
                                               clip, entry->mLastCommonClipCount, geometry->ComputeInvalidationRegion(),
                                               &combined);

    
    combined.Or(combined, invalid);

    for (uint32_t i = 0; i < changedFrames.Length(); i++) {
      combined.Or(combined, changedFrames[i]->GetVisualOverflowRect());
    }

    
    nsRegion clipRegion;
    if (clip.ComputeRegionInClips(&aData->mClip, shift, &clipRegion)) {
      combined.And(combined, clipRegion);
    }
#ifdef MOZ_DUMP_PAINTING
    if (nsLayoutUtils::InvalidationDebuggingIsEnabled()) {
      if (!combined.IsEmpty()) {
        printf_stderr("Display item type %s(%p) (in layer %p) changed geometry!\n", item->Name(), item->Frame(), aData->mLayer.get());
      }
    }
#endif
  }
  if (!combined.IsEmpty()) {
    if (notifyRenderingChanged) {
      item->NotifyRenderingChanged();
    }
    InvalidatePostTransformRegion(paintedLayer,
        combined.ScaleToOutsidePixels(layerData->mXScale, layerData->mYScale, layerData->mAppUnitsPerDevPixel),
        layerData->mTranslation);
  }

  aData->EndUpdate(geometry);
}

void
FrameLayerBuilder::AddPaintedDisplayItem(PaintedLayerData* aLayerData,
                                        nsDisplayItem* aItem,
                                        const DisplayItemClip& aClip,
                                        ContainerState& aContainerState,
                                        LayerState aLayerState,
                                        const nsPoint& aTopLeft)
{
  PaintedLayer* layer = aLayerData->mLayer;
  PaintedDisplayItemLayerUserData* paintedData =
    static_cast<PaintedDisplayItemLayerUserData*>
      (layer->GetUserData(&gPaintedDisplayItemLayerUserData));
  nsRefPtr<BasicLayerManager> tempManager;
  nsIntRect intClip;
  bool hasClip = false;
  if (aLayerState != LAYER_NONE) {
    DisplayItemData *data = GetDisplayItemDataForManager(aItem, layer->Manager());
    if (data) {
      tempManager = data->mInactiveManager;
    }
    if (!tempManager) {
      tempManager = new BasicLayerManager(BasicLayerManager::BLM_INACTIVE);
    }

    
    nsRegion clip;
    DisplayItemClip* oldClip = nullptr;
    GetOldLayerFor(aItem, nullptr, &oldClip);
    hasClip = aClip.ComputeRegionInClips(oldClip,
                                         aTopLeft - paintedData->mLastAnimatedGeometryRootOrigin,
                                         &clip);

    if (hasClip) {
      intClip = clip.GetBounds().ScaleToOutsidePixels(paintedData->mXScale,
                                                      paintedData->mYScale,
                                                      paintedData->mAppUnitsPerDevPixel);
    }
  }

  AddLayerDisplayItem(layer, aItem, aLayerState, aTopLeft, tempManager);

  PaintedLayerItemsEntry* entry = mPaintedLayerItems.PutEntry(layer);
  if (entry) {
    entry->mContainerLayerFrame = aContainerState.GetContainerFrame();
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
    if (tempManager) {
      FLB_LOG_PAINTED_LAYER_DECISION(aLayerData, "Creating nested FLB for item %p\n", aItem);
      FrameLayerBuilder* layerBuilder = new FrameLayerBuilder();
      layerBuilder->Init(mDisplayListBuilder, tempManager, aLayerData, &aContainerState);

      tempManager->BeginTransaction();
      if (mRetainingManager) {
        layerBuilder->DidBeginRetainedLayerTransaction(tempManager);
      }

      UniquePtr<LayerProperties> props(LayerProperties::CloneFrom(tempManager->GetRoot()));
      nsRefPtr<Layer> tmpLayer =
        aItem->BuildLayer(mDisplayListBuilder, tempManager, ContainerLayerParameters());
      
      
      if (!tmpLayer) {
        tempManager->EndTransaction(nullptr, nullptr);
        tempManager->SetUserData(&gLayerManagerLayerBuilder, nullptr);
        return;
      }

      bool snap;
      nsRect visibleRect =
        aItem->GetVisibleRect().Intersect(aItem->GetBounds(mDisplayListBuilder, &snap));
      nsIntRegion rgn = visibleRect.ToOutsidePixels(paintedData->mAppUnitsPerDevPixel);
      SetOuterVisibleRegion(tmpLayer, &rgn);

      
      
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

      nsIntPoint offset = GetLastPaintOffset(layer) - GetTranslationForPaintedLayer(layer);
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
        invalid.ScaleRoundOut(paintedData->mXScale, paintedData->mYScale);

        if (hasClip) {
          invalid.And(invalid, intClip);
        }

        InvalidatePostTransformRegion(layer, invalid,
                                      GetTranslationForPaintedLayer(layer));
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
      oldData->BeginUpdate(aLayer, aState, mContainerLayerGeneration, aItem);
    }
    return oldData;
  }

  LayerManagerData* lmd = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));

  nsRefPtr<DisplayItemData> data =
    new DisplayItemData(lmd, aItem->GetPerFrameKey());

  data->BeginUpdate(aLayer, aState, mContainerLayerGeneration, aItem);

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
    oldData->BeginUpdate(aLayer, aState, mContainerLayerGeneration);
    return;
  }

  LayerManagerData* lmd = static_cast<LayerManagerData*>
    (mRetainingManager->GetUserData(&gLayerManagerUserData));

  nsRefPtr<DisplayItemData> data =
    new DisplayItemData(lmd, aDisplayItemKey, aFrame);

  data->BeginUpdate(aLayer, aState, mContainerLayerGeneration);

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
                                       LayerState aLayerState,
                                       const nsPoint& aTopLeft,
                                       BasicLayerManager* aManager)
{
  if (aLayer->Manager() != mRetainingManager)
    return;

  DisplayItemData *data = StoreDataForFrame(aItem, aLayer, aLayerState);
  data->mInactiveManager = aManager;
}

nsIntPoint
FrameLayerBuilder::GetLastPaintOffset(PaintedLayer* aLayer)
{
  PaintedLayerItemsEntry* entry = mPaintedLayerItems.PutEntry(aLayer);
  if (entry) {
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
    if (entry->mHasExplicitLastPaintOffset)
      return entry->mLastPaintOffset;
  }
  return GetTranslationForPaintedLayer(aLayer);
}

void
FrameLayerBuilder::SavePreviousDataForLayer(PaintedLayer* aLayer, uint32_t aClipCount)
{
  PaintedLayerItemsEntry* entry = mPaintedLayerItems.PutEntry(aLayer);
  if (entry) {
    if (entry->mContainerLayerGeneration == 0) {
      entry->mContainerLayerGeneration = mContainerLayerGeneration;
    }
    entry->mLastPaintOffset = GetTranslationForPaintedLayer(aLayer);
    entry->mHasExplicitLastPaintOffset = true;
    entry->mLastCommonClipCount = aClipCount;
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
    if (layer->HasUserData(&gPaintedDisplayItemLayerUserData)) {
      NS_ASSERTION(layer->AsPaintedLayer(), "Wrong layer type");
      mRecycledPaintedLayers.AppendElement(static_cast<PaintedLayer*>(layer));
    }

    if (Layer* maskLayer = layer->GetMaskLayer()) {
      NS_ASSERTION(maskLayer->GetType() == Layer::TYPE_IMAGE,
                   "Could not recycle mask layer, unsupported layer type.");
      mRecycledMaskImageLayers.Put(layer, static_cast<ImageLayer*>(maskLayer));
    }
  }
}

struct OpaqueRegionEntry {
  const nsIFrame* mAnimatedGeometryRoot;
  const nsIFrame* mFixedPosFrameForLayerData;
  nsIntRegion mOpaqueRegion;
};

static OpaqueRegionEntry*
FindOpaqueRegionEntry(nsTArray<OpaqueRegionEntry>& aEntries,
                      const nsIFrame* aAnimatedGeometryRoot,
                      const nsIFrame* aFixedPosFrameForLayerData)
{
  for (uint32_t i = 0; i < aEntries.Length(); ++i) {
    OpaqueRegionEntry* d = &aEntries[i];
    if (d->mAnimatedGeometryRoot == aAnimatedGeometryRoot &&
        d->mFixedPosFrameForLayerData == aFixedPosFrameForLayerData) {
      return d;
    }
  }
  return nullptr;
}

void
ContainerState::SetupScrollingMetadata(NewLayerEntry* aEntry)
{
  if (mFlattenToSingleLayer) {
    
    
    return;
  }

  nsAutoTArray<FrameMetrics,2> metricsArray;
  if (aEntry->mBaseFrameMetrics) {
    metricsArray.AppendElement(*aEntry->mBaseFrameMetrics);
  }
  uint32_t baseLength = metricsArray.Length();

  nsIntRect tmpClipRect;
  const nsIntRect* layerClip = aEntry->mLayer->GetClipRect();
  nsIFrame* fParent;
  for (const nsIFrame* f = aEntry->mAnimatedGeometryRoot;
       f != mContainerAnimatedGeometryRoot;
       f = nsLayoutUtils::GetAnimatedGeometryRootForFrame(this->mBuilder,
           fParent, mContainerAnimatedGeometryRoot)) {
    fParent = nsLayoutUtils::GetCrossDocParentFrame(f);
    if (!fParent) {
      
      
      
      
      
      
      
      metricsArray.SetLength(baseLength);
      aEntry->mLayer->SetFrameMetrics(metricsArray);
      return;
    }

    nsIScrollableFrame* scrollFrame = nsLayoutUtils::GetScrollableFrameFor(f);
    if (!scrollFrame) {
      continue;
    }

    nsRect clipRect(0, 0, -1, -1);
    scrollFrame->ComputeFrameMetrics(aEntry->mLayer, mContainerReferenceFrame,
                                     mParameters, &clipRect, &metricsArray);
    if (clipRect.width >= 0) {
      nsIntRect pixClip = ScaleToNearestPixels(clipRect);
      if (layerClip) {
        tmpClipRect.IntersectRect(pixClip, *layerClip);
      } else {
        tmpClipRect = pixClip;
      }
      layerClip = &tmpClipRect;
      
      
      
    }
  }
  aEntry->mLayer->SetClipRect(layerClip);
  
  aEntry->mLayer->SetFrameMetrics(metricsArray);
}

void
ContainerState::PostprocessRetainedLayers(nsIntRegion* aOpaqueRegionForContainer)
{
  nsAutoTArray<OpaqueRegionEntry,4> opaqueRegions;
  bool hideAll = false;
  int32_t opaqueRegionForContainer = -1;

  for (int32_t i = mNewChildLayers.Length() - 1; i >= 0; --i) {
    NewLayerEntry* e = &mNewChildLayers.ElementAt(i);
    if (!e->mLayer) {
      continue;
    }

    
    
    
    const nsIFrame* animatedGeometryRootForOpaqueness =
        mFlattenToSingleLayer ? mContainerAnimatedGeometryRoot : e->mAnimatedGeometryRoot;
    OpaqueRegionEntry* data = FindOpaqueRegionEntry(opaqueRegions,
        animatedGeometryRootForOpaqueness, e->mFixedPosFrameForLayerData);

    SetupScrollingMetadata(e);

    if (hideAll) {
      e->mVisibleRegion.SetEmpty();
    } else if (!e->mLayer->IsScrollbarContainer()) {
      const nsIntRect* clipRect = e->mLayer->GetClipRect();
      if (clipRect && opaqueRegionForContainer >= 0 &&
          opaqueRegions[opaqueRegionForContainer].mOpaqueRegion.Contains(*clipRect)) {
        e->mVisibleRegion.SetEmpty();
      } else if (data) {
        e->mVisibleRegion.Sub(e->mVisibleRegion, data->mOpaqueRegion);
      }
    }

    SetOuterVisibleRegionForLayer(e->mLayer, e->mVisibleRegion,
      e->mLayerContentsVisibleRect.width >= 0 ? &e->mLayerContentsVisibleRect : nullptr);

    if (!e->mOpaqueRegion.IsEmpty()) {
      const nsIFrame* animatedGeometryRootToCover = animatedGeometryRootForOpaqueness;
      if (e->mOpaqueForAnimatedGeometryRootParent &&
          nsLayoutUtils::GetAnimatedGeometryRootForFrame(mBuilder, e->mAnimatedGeometryRoot->GetParent(),
                                                         mContainerAnimatedGeometryRoot)
            == mContainerAnimatedGeometryRoot) {
        animatedGeometryRootToCover = mContainerAnimatedGeometryRoot;
        data = FindOpaqueRegionEntry(opaqueRegions,
            animatedGeometryRootToCover, e->mFixedPosFrameForLayerData);
      }

      if (!data) {
        if (animatedGeometryRootToCover == mContainerAnimatedGeometryRoot &&
            e->mFixedPosFrameForLayerData == mContainerFixedPosFrame) {
          NS_ASSERTION(opaqueRegionForContainer == -1, "Already found it?");
          opaqueRegionForContainer = opaqueRegions.Length();
        }
        data = opaqueRegions.AppendElement();
        data->mAnimatedGeometryRoot = animatedGeometryRootToCover;
        data->mFixedPosFrameForLayerData = e->mFixedPosFrameForLayerData;
      }

      nsIntRegion clippedOpaque = e->mOpaqueRegion;
      const nsIntRect* clipRect = e->mLayer->GetClipRect();
      if (clipRect) {
        clippedOpaque.AndWith(*clipRect);
      }
      data->mOpaqueRegion.Or(data->mOpaqueRegion, clippedOpaque);
      if (e->mHideAllLayersBelow) {
        hideAll = true;
      }
    }

    if (e->mLayer->GetType() == Layer::TYPE_READBACK) {
      
      
      
      
      opaqueRegions.Clear();
      opaqueRegionForContainer = -1;
    }
  }

  if (opaqueRegionForContainer >= 0) {
    aOpaqueRegionForContainer->Or(*aOpaqueRegionForContainer,
        opaqueRegions[opaqueRegionForContainer].mOpaqueRegion);
  }
}

void
ContainerState::Finish(uint32_t* aTextContentFlags, LayerManagerData* aData,
                       const nsIntRect& aContainerPixelBounds,
                       nsDisplayList* aChildItems, bool& aHasComponentAlphaChildren)
{
  while (!mPaintedLayerDataStack.IsEmpty()) {
    PopPaintedLayerData();
  }

  NS_ASSERTION(mContainerBounds.IsEqualInterior(mAccumulatedChildBounds),
               "Bounds computation mismatch");

  if (mLayerBuilder->IsBuildingRetainedLayers()) {
    nsIntRegion containerOpaqueRegion;
    PostprocessRetainedLayers(&containerOpaqueRegion);
    if (containerOpaqueRegion.Contains(aContainerPixelBounds)) {
      aChildItems->SetIsOpaque();
    }
  }

  uint32_t textContentFlags = 0;

  
  
  Layer* layer = nullptr;
  Layer* prevChild = nullptr;
  for (uint32_t i = 0; i < mNewChildLayers.Length(); ++i, prevChild = layer) {
    if (!mNewChildLayers[i].mLayer) {
      continue;
    }

    layer = mNewChildLayers[i].mLayer;

    if (!layer->GetVisibleRegion().IsEmpty()) {
      textContentFlags |=
        layer->GetContentFlags() & (Layer::CONTENT_COMPONENT_ALPHA |
                                    Layer::CONTENT_COMPONENT_ALPHA_DESCENDANT);

      
      
      if (mNewChildLayers[i].mPropagateComponentAlphaFlattening &&
          (layer->GetContentFlags() & Layer::CONTENT_COMPONENT_ALPHA)) {
        aHasComponentAlphaChildren = true;
      }
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

static void RestrictScaleToMaxLayerSize(gfxSize& aScale,
                                        const nsRect& aVisibleRect,
                                        nsIFrame* aContainerFrame,
                                        Layer* aContainerLayer)
{
  if (!aContainerLayer->Manager()->IsWidgetLayerManager()) {
    return;
  }

  nsIntRect pixelSize =
    aVisibleRect.ScaleToOutsidePixels(aScale.width, aScale.height,
                                      aContainerFrame->PresContext()->AppUnitsPerDevPixel());

  int32_t maxLayerSize = aContainerLayer->GetMaxLayerSize();

  if (pixelSize.width > maxLayerSize) {
    float scale = (float)pixelSize.width / maxLayerSize;
    scale = gfxUtils::ClampToScaleFactor(scale);
    aScale.width /= scale;
  }
  if (pixelSize.height > maxLayerSize) {
    float scale = (float)pixelSize.height / maxLayerSize;
    scale = gfxUtils::ClampToScaleFactor(scale);
    aScale.height /= scale;
  }
}
static bool
ChooseScaleAndSetTransform(FrameLayerBuilder* aLayerBuilder,
                           nsDisplayListBuilder* aDisplayListBuilder,
                           nsIFrame* aContainerFrame,
                           const nsRect& aVisibleRect,
                           const Matrix4x4* aTransform,
                           const ContainerLayerParameters& aIncomingScale,
                           ContainerLayer* aLayer,
                           LayerState aState,
                           ContainerLayerParameters& aOutgoingScale)
{
  nsIntPoint offset;

  Matrix4x4 transform =
    Matrix4x4::Scaling(aIncomingScale.mXScale, aIncomingScale.mYScale, 1.0);
  if (aTransform) {
    
    transform = (*aTransform)*transform;
    
    
    
    
    
    transform.NudgeToIntegersFixedEpsilon();
  }
  Matrix transform2d;
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
  transform.PostTranslate(offset.x + aIncomingScale.mOffset.x,
                          offset.y + aIncomingScale.mOffset.y,
                          0);

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
      
      scale = RoundToFloatPrecision(ThebesMatrix(transform2d).ScaleFactors(true));
      
      
      
      
      
      
      Matrix frameTransform;
      if (ActiveLayerTracker::IsStyleAnimated(aDisplayListBuilder, aContainerFrame, eCSSProperty_transform) &&
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
    
    
    
    if (aTransform) {
      RestrictScaleToMaxLayerSize(scale, aVisibleRect, aContainerFrame, aLayer);
    }
  } else {
    scale = gfxSize(1.0, 1.0);
  }

  
  aLayer->SetBaseTransform(transform);
  aLayer->SetPreScale(1.0f/float(scale.width),
                      1.0f/float(scale.height));
  aLayer->SetInheritedScale(aIncomingScale.mXScale,
                            aIncomingScale.mYScale);

  aOutgoingScale =
    ContainerLayerParameters(scale.width, scale.height, -offset, aIncomingScale);
  if (aTransform) {
    aOutgoingScale.mInTransformedSubtree = true;
    if (ActiveLayerTracker::IsStyleAnimated(aDisplayListBuilder, aContainerFrame,
                                            eCSSProperty_transform)) {
      aOutgoingScale.mInActiveTransformedSubtree = true;
    }
  }
  if (aLayerBuilder->IsBuildingRetainedLayers() &&
      (!canDraw2D || transform2d.HasNonIntegerTranslation())) {
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
FrameLayerBuilder::RestorePaintedLayerItemEntries(PaintedLayerItemsEntry* aEntry, void* aUserArg)
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
                                          nsDisplayList* aChildren,
                                          const ContainerLayerParameters& aParameters,
                                          const Matrix4x4* aTransform,
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
      if (oldLayer->HasUserData(&gPaintedDisplayItemLayerUserData)) {
        
        
        
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
    
    
    
    
    NS_ASSERTION(aChildren->IsEmpty(), "Should have no children");
    return containerLayer.forget();
  }

  ContainerLayerParameters scaleParameters;
  nsRect bounds = aChildren->GetBounds(aBuilder);
  nsRect childrenVisible =
      aContainerItem ? aContainerItem->GetVisibleRectForChildren() :
          aContainerFrame->GetVisualOverflowRectRelativeToSelf();
  if (!ChooseScaleAndSetTransform(this, aBuilder, aContainerFrame,
                                  bounds.Intersect(childrenVisible),
                                  aTransform, aParameters,
                                  containerLayer, state, scaleParameters)) {
    return nullptr;
  }

  uint32_t oldGeneration = mContainerLayerGeneration;
  mContainerLayerGeneration = ++mMaxContainerLayerGeneration;

  nsRefPtr<RefCountedRegion> paintedLayerInvalidRegion = nullptr;
  if (mRetainingManager) {
    if (aContainerItem) {
      StoreDataForFrame(aContainerItem, containerLayer, LAYER_ACTIVE);
    } else {
      StoreDataForFrame(aContainerFrame, containerDisplayItemKey, containerLayer, LAYER_ACTIVE);
    }
  }

  LayerManagerData* data = static_cast<LayerManagerData*>
    (aManager->GetUserData(&gLayerManagerUserData));

  nsIntRect pixBounds;
  nscoord appUnitsPerDevPixel;
  bool flattenToSingleLayer = false;
  if ((aContainerFrame->GetStateBits() & NS_FRAME_NO_COMPONENT_ALPHA) &&
      mRetainingManager &&
      mRetainingManager->ShouldAvoidComponentAlphaLayers() &&
      !gfxPrefs::AsyncPanZoomEnabled())
  {
    flattenToSingleLayer = true;
  }

  nscolor backgroundColor = NS_RGBA(0,0,0,0);
  if (aFlags & CONTAINER_ALLOW_PULL_BACKGROUND_COLOR) {
    backgroundColor = aParameters.mBackgroundColor;
  }

  uint32_t flags;
  while (true) {
    ContainerState state(aBuilder, aManager, aManager->GetLayerBuilder(),
                         aContainerFrame, aContainerItem, bounds,
                         containerLayer, scaleParameters, flattenToSingleLayer,
                         backgroundColor);

    state.ProcessDisplayItems(aChildren);

    
    
    
    bool hasComponentAlphaChildren = false;
    pixBounds = state.ScaleToOutsidePixels(bounds, false);
    appUnitsPerDevPixel = state.GetAppUnitsPerDevPixel();
    state.Finish(&flags, data, pixBounds, aChildren, hasComponentAlphaChildren);

    if (hasComponentAlphaChildren &&
        mRetainingManager &&
        mRetainingManager->ShouldAvoidComponentAlphaLayers() &&
        containerLayer->HasMultipleChildren() &&
        !flattenToSingleLayer &&
        !gfxPrefs::AsyncPanZoomEnabled())
    {
      
      
      
      
      flattenToSingleLayer = true;
      data->mDisplayItems.EnumerateEntries(RestoreDisplayItemData,
                                           &mContainerLayerGeneration);
      mPaintedLayerItems.EnumerateEntries(RestorePaintedLayerItemEntries,
                                         &mContainerLayerGeneration);
      aContainerFrame->AddStateBits(NS_FRAME_NO_COMPONENT_ALPHA);
      continue;
    }
    break;
  }

  
  
  
  
  
  if (flags & Layer::CONTENT_COMPONENT_ALPHA) {
    flags |= Layer::CONTENT_COMPONENT_ALPHA_DESCENDANT;
  }

  
  
  if (aChildren->IsOpaque() && !aChildren->NeedsTransparentSurface()) {
    bounds.ScaleRoundIn(scaleParameters.mXScale, scaleParameters.mYScale);
    if (bounds.Contains(pixBounds.ToAppUnits(appUnitsPerDevPixel))) {
      
      flags &= ~Layer::CONTENT_COMPONENT_ALPHA;
      flags |= Layer::CONTENT_OPAQUE;
    }
  }
  containerLayer->SetContentFlags(flags);
  
  
  if (!aContainerItem) {
    containerLayer->SetVisibleRegion(pixBounds);
  }
  if (aParameters.mLayerContentsVisibleRect) {
    *aParameters.mLayerContentsVisibleRect = pixBounds + scaleParameters.mOffset;
  }

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
  if (layer->HasUserData(&gPaintedDisplayItemLayerUserData)) {
    
    
    
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
            !layer->HasUserData(&gPaintedDisplayItemLayerUserData)) {
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
  Matrix4x4 transform = Matrix4x4::Scaling(aScale.width, aScale.height, 1.0);
  if (aFrame != aAncestorWithScale) {
    
    transform = nsLayoutUtils::GetTransformToAncestor(aFrame, aAncestorWithScale)*transform;
  }
  Matrix transform2d;
  if (transform.CanDraw2D(&transform2d)) {
     return ThebesMatrix(transform2d).ScaleFactors(true);
  }
  return gfxSize(1.0, 1.0);
}

gfxSize
FrameLayerBuilder::GetPaintedLayerScaleForFrame(nsIFrame* aFrame)
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
        PaintedDisplayItemLayerUserData* data =
            static_cast<PaintedDisplayItemLayerUserData*>
              (l->GetUserData(&gPaintedDisplayItemLayerUserData));
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
static void DebugPaintItem(DrawTarget& aDrawTarget,
                           nsPresContext* aPresContext,
                           nsDisplayItem *aItem,
                           nsDisplayListBuilder* aBuilder)
{
  bool snap;
  Rect bounds = NSRectToRect(aItem->GetBounds(aBuilder, &snap),
                             aPresContext->AppUnitsPerDevPixel());

  RefPtr<DrawTarget> tempDT =
    aDrawTarget.CreateSimilarDrawTarget(IntSize(bounds.width, bounds.height),
                                        SurfaceFormat::B8G8R8A8);
  nsRefPtr<gfxContext> context = new gfxContext(tempDT);
  context->SetMatrix(gfxMatrix::Translation(-bounds.x, -bounds.y));
  nsRenderingContext ctx(context);

  aItem->Paint(aBuilder, &ctx);
  RefPtr<SourceSurface> surface = tempDT->Snapshot();
  DumpPaintedImage(aItem, surface);

  aDrawTarget.DrawSurface(surface, bounds, Rect(Point(0,0), bounds.Size()));

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
                 "a painted layer should contain items only at the same zoom");

    MOZ_ASSERT(clip.HasClip() || clip.GetRoundedRectCount() == 0,
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
  DrawTarget& aDrawTarget = *aRC->GetDrawTarget();

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
      bool saved = aDrawTarget.GetPermitSubpixelAA();
      PaintInactiveLayer(aBuilder, cdi->mInactiveLayerManager, cdi->mItem, aContext, aRC);
      aDrawTarget.SetPermitSubpixelAA(saved);
    } else {
      nsIFrame* frame = cdi->mItem->Frame();
      frame->AddStateBits(NS_FRAME_PAINTED_THEBES);
#ifdef MOZ_DUMP_PAINTING
      if (gfxUtils::sDumpPainting) {
        DebugPaintItem(aDrawTarget, aPresContext, cdi->mItem, aBuilder);
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
      aClip == DrawRegionClip::NONE) {
    return false;
  }

  DrawTarget *dt = aContext->GetDrawTarget();
  return !dt->SupportsRegionClipping();
}

static void DrawForcedBackgroundColor(DrawTarget& aDrawTarget,
                                      Layer* aLayer, nscolor
                                      aBackgroundColor)
{
  if (NS_GET_A(aBackgroundColor) > 0) {
    nsIntRect r = aLayer->GetVisibleRegion().GetBounds();
    ColorPattern color(ToDeviceColor(aBackgroundColor));
    aDrawTarget.FillRect(Rect(r.x, r.y, r.width, r.height), color);
  }
}

class LayerTimelineMarker : public TimelineMarker
{
public:
  LayerTimelineMarker(nsDocShell* aDocShell, const nsIntRegion& aRegion)
    : TimelineMarker(aDocShell, "Layer", TRACING_EVENT)
    , mRegion(aRegion)
  {
  }

  ~LayerTimelineMarker()
  {
  }

  virtual void AddLayerRectangles(mozilla::dom::Sequence<mozilla::dom::ProfileTimelineLayerRect>& aRectangles) MOZ_OVERRIDE
  {
    nsIntRegionRectIterator it(mRegion);
    while (const nsIntRect* iterRect = it.Next()) {
      mozilla::dom::ProfileTimelineLayerRect rect;
      rect.mX = iterRect->X();
      rect.mY = iterRect->Y();
      rect.mWidth = iterRect->Width();
      rect.mHeight = iterRect->Height();
      aRectangles.AppendElement(rect);
    }
  }

private:
  nsIntRegion mRegion;
};





























 void
FrameLayerBuilder::DrawPaintedLayer(PaintedLayer* aLayer,
                                   gfxContext* aContext,
                                   const nsIntRegion& aRegionToDraw,
                                   DrawRegionClip aClip,
                                   const nsIntRegion& aRegionToInvalidate,
                                   void* aCallbackData)
{
  DrawTarget& aDrawTarget = *aContext->GetDrawTarget();

  PROFILER_LABEL("FrameLayerBuilder", "DrawPaintedLayer",
    js::ProfileEntry::Category::GRAPHICS);

  nsDisplayListBuilder* builder = static_cast<nsDisplayListBuilder*>
    (aCallbackData);

  FrameLayerBuilder *layerBuilder = aLayer->Manager()->GetLayerBuilder();
  NS_ASSERTION(layerBuilder, "Unexpectedly null layer builder!");

  if (layerBuilder->CheckDOMModified())
    return;

  PaintedLayerItemsEntry* entry = layerBuilder->mPaintedLayerItems.GetEntry(aLayer);
  NS_ASSERTION(entry, "We shouldn't be drawing into a layer with no items!");
  if (!entry->mContainerLayerFrame) {
    return;
  }


  PaintedDisplayItemLayerUserData* userData =
    static_cast<PaintedDisplayItemLayerUserData*>
      (aLayer->GetUserData(&gPaintedDisplayItemLayerUserData));
  NS_ASSERTION(userData, "where did our user data go?");

  bool shouldDrawRectsSeparately = ShouldDrawRectsSeparately(aContext, aClip);

  if (!shouldDrawRectsSeparately) {
    if (aClip == DrawRegionClip::DRAW) {
      gfxUtils::ClipToRegion(aContext, aRegionToDraw);
    }

    DrawForcedBackgroundColor(aDrawTarget, aLayer,
                              userData->mForcedBackgroundColor);
  }

  if (NS_GET_A(userData->mFontSmoothingBackgroundColor) > 0) {
    aContext->SetFontSmoothingBackgroundColor(
      Color::FromABGR(userData->mFontSmoothingBackgroundColor));
  }

  
  
  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  nsIntPoint offset = GetTranslationForPaintedLayer(aLayer);
  nsPresContext* presContext = entry->mContainerLayerFrame->PresContext();

  if (!layerBuilder->GetContainingPaintedLayerData()) {
    
    
    
    int32_t appUnitsPerDevPixel = presContext->AppUnitsPerDevPixel();
    RecomputeVisibilityForItems(entry->mItems, builder, aRegionToDraw,
                                offset, appUnitsPerDevPixel,
                                userData->mXScale, userData->mYScale);
  }

  nsRenderingContext rc(aContext);

  if (shouldDrawRectsSeparately) {
    nsIntRegionRectIterator it(aRegionToDraw);
    while (const nsIntRect* iterRect = it.Next()) {
      gfxContextAutoSaveRestore save(aContext);
      aContext->NewPath();
      aContext->Rectangle(*iterRect);
      aContext->Clip();

      DrawForcedBackgroundColor(aDrawTarget, aLayer,
                                userData->mForcedBackgroundColor);

      
      
      
      aContext->SetMatrix(
        aContext->CurrentMatrix().Translate(aLayer->GetResidualTranslation() - gfxPoint(offset.x, offset.y)).
                                  Scale(userData->mXScale, userData->mYScale));

      layerBuilder->PaintItems(entry->mItems, *iterRect, aContext, &rc,
                               builder, presContext,
                               offset, userData->mXScale, userData->mYScale,
                               entry->mCommonClipCount);
    }
  } else {
    
    
    
    aContext->SetMatrix(
      aContext->CurrentMatrix().Translate(aLayer->GetResidualTranslation() - gfxPoint(offset.x, offset.y)).
                                Scale(userData->mXScale,userData->mYScale));

    layerBuilder->PaintItems(entry->mItems, aRegionToDraw.GetBounds(), aContext, &rc,
                             builder, presContext,
                             offset, userData->mXScale, userData->mYScale,
                             entry->mCommonClipCount);
  }

  aContext->SetFontSmoothingBackgroundColor(Color());

  bool isActiveLayerManager = !aLayer->Manager()->IsInactiveLayerManager();

  if (presContext->GetPaintFlashing() && isActiveLayerManager) {
    gfxContextAutoSaveRestore save(aContext);
    if (shouldDrawRectsSeparately) {
      if (aClip == DrawRegionClip::DRAW) {
        gfxUtils::ClipToRegion(aContext, aRegionToDraw);
      }
    }
    FlashPaint(aContext);
  }

  if (presContext && presContext->GetDocShell() && isActiveLayerManager) {
    nsDocShell* docShell = static_cast<nsDocShell*>(presContext->GetDocShell());
    bool isRecording;
    docShell->GetRecordProfileTimelineMarkers(&isRecording);
    if (isRecording) {
      mozilla::UniquePtr<TimelineMarker> marker =
        MakeUnique<LayerTimelineMarker>(docShell, aRegionToDraw);
      docShell->AddProfileTimelineMarker(marker);
    }
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

 void
FrameLayerBuilder::DumpRetainedLayerTree(LayerManager* aManager, std::stringstream& aStream, bool aDumpHtml)
{
  aManager->Dump(aStream, "", aDumpHtml);
}

nsDisplayItemGeometry*
FrameLayerBuilder::GetMostRecentGeometry(nsDisplayItem* aItem)
{
  typedef nsTArray<DisplayItemData*> DataArray;

  
  FrameProperties properties = aItem->Frame()->Properties();
  auto dataArray =
    static_cast<DataArray*>(properties.Get(LayerManagerDataProperty()));
  if (!dataArray) {
    return nullptr;
  }

  
  uint32_t itemPerFrameKey = aItem->GetPerFrameKey();
  for (uint32_t i = 0; i < dataArray->Length(); i++) {
    DisplayItemData* data = dataArray->ElementAt(i);
    if (data->GetDisplayItemKey() == itemPerFrameKey) {
      return data->GetGeometry();
    }
  }

  return nullptr;
}

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
SetClipCount(PaintedDisplayItemLayerUserData* apaintedData,
             uint32_t aClipCount)
{
  if (apaintedData) {
    apaintedData->mMaskClipCount = aClipCount;
  }
}

void
ContainerState::SetupMaskLayer(Layer *aLayer,
                               const DisplayItemClip& aClip,
                               const nsIntRegion& aLayerVisibleRegion,
                               uint32_t aRoundedRectClipCount)
{
  
  
  
  PaintedDisplayItemLayerUserData* paintedData = GetPaintedDisplayItemLayerUserData(aLayer);
  if (paintedData &&
      aRoundedRectClipCount < paintedData->mMaskClipCount) {
    PaintedLayer* painted = aLayer->AsPaintedLayer();
    painted->InvalidateRegion(painted->GetValidRegion().GetBounds());
  }

  
  nsIntRect layerBounds = aLayerVisibleRegion.GetBounds();
  if (aClip.GetRoundedRectCount() == 0 ||
      aRoundedRectClipCount == 0 ||
      layerBounds.IsEmpty()) {
    SetClipCount(paintedData, 0);
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
    SetClipCount(paintedData, aRoundedRectClipCount);
    return;
  }

  
  gfx::Rect boundingRect = CalculateBounds(newData.mRoundedClipRects,
                                           newData.mAppUnitsPerDevPixel);
  boundingRect.Scale(mParameters.mXScale, mParameters.mYScale);

  uint32_t maxSize = mManager->GetMaxTextureSize();
  NS_ASSERTION(maxSize > 0, "Invalid max texture size");
  gfx::Size surfaceSize(std::min<gfx::Float>(boundingRect.Width(), maxSize),
                        std::min<gfx::Float>(boundingRect.Height(), maxSize));

  
  
  
  
  gfx::Matrix maskTransform =
    Matrix::Scaling(surfaceSize.width / boundingRect.Width(),
                    surfaceSize.height / boundingRect.Height());
  gfx::Point p = boundingRect.TopLeft();
  maskTransform.PreTranslate(-p.x, -p.y);
  
  gfx::Matrix imageTransform = maskTransform;
  imageTransform.PreScale(mParameters.mXScale, mParameters.mYScale);

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
      SetClipCount(paintedData, 0);
      return;
    }

    nsRefPtr<gfxContext> context = new gfxContext(dt);
    context->Multiply(ThebesMatrix(imageTransform));

    
    aClip.FillIntersectionOfRoundedRectClips(context,
                                             Color(1.f, 1.f, 1.f, 1.f),
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
  matrix.PreTranslate(mParameters.mOffset.x, mParameters.mOffset.y, 0);
  maskLayer->SetBaseTransform(matrix);

  
  userData->mScaleX = newData.mScaleX;
  userData->mScaleY = newData.mScaleY;
  userData->mOffset = newData.mOffset;
  userData->mAppUnitsPerDevPixel = newData.mAppUnitsPerDevPixel;
  userData->mRoundedClipRects.SwapElements(newData.mRoundedClipRects);
  userData->mImageKey = lookupKey;

  aLayer->SetMaskLayer(maskLayer);
  SetClipCount(paintedData, aRoundedRectClipCount);
  return;
}

} 
