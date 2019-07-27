











#ifndef NSDISPLAYLIST_H_
#define NSDISPLAYLIST_H_

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "nsCOMPtr.h"
#include "nsContainerFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "plarena.h"
#include "Layers.h"
#include "nsRegion.h"
#include "nsDisplayListInvalidation.h"
#include "DisplayListClipState.h"
#include "LayerState.h"
#include "FrameMetrics.h"
#include "mozilla/UniquePtr.h"
#include "mozilla/gfx/UserData.h"

#include <stdint.h>
#include "nsTHashtable.h"

#include <stdlib.h>
#include <algorithm>

class nsIContent;
class nsRenderingContext;
class nsDisplayTableItem;
class nsISelection;
class nsDisplayLayerEventRegions;
class nsCaret;

namespace mozilla {
class FrameLayerBuilder;
namespace layers {
class Layer;
class ImageLayer;
class ImageContainer;
} 
} 



typedef mozilla::EnumSet<mozilla::gfx::CompositionOp> BlendModeSet;












































#ifdef MOZ_DUMP_PAINTING
#define NS_DISPLAY_DECL_NAME(n, e) \
  virtual const char* Name() { return n; } \
  virtual Type GetType() { return e; }
#else
#define NS_DISPLAY_DECL_NAME(n, e) \
  virtual Type GetType() { return e; }
#endif










class nsDisplayListBuilder {
public:
  typedef mozilla::FramePropertyDescriptor FramePropertyDescriptor;
  typedef mozilla::FrameLayerBuilder FrameLayerBuilder;
  typedef mozilla::DisplayItemClip DisplayItemClip;
  typedef mozilla::DisplayListClipState DisplayListClipState;
  typedef nsIWidget::ThemeGeometry ThemeGeometry;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;

  







  enum Mode {
    PAINTING,
    EVENT_DELIVERY,
    PLUGIN_GEOMETRY,
    IMAGE_VISIBILITY,
    OTHER
  };
  nsDisplayListBuilder(nsIFrame* aReferenceFrame, Mode aMode, bool aBuildCaret);
  ~nsDisplayListBuilder();

  void SetWillComputePluginGeometry(bool aWillComputePluginGeometry)
  {
    mWillComputePluginGeometry = aWillComputePluginGeometry;
  }
  void SetForPluginGeometry()
  {
    NS_ASSERTION(mMode == PAINTING, "Can only switch from PAINTING to PLUGIN_GEOMETRY");
    NS_ASSERTION(mWillComputePluginGeometry, "Should have signalled this in advance");
    mMode = PLUGIN_GEOMETRY;
  }

  



  bool IsForEventDelivery() { return mMode == EVENT_DELIVERY; }
  






  bool IsForPluginGeometry() { return mMode == PLUGIN_GEOMETRY; }
  


  bool IsForPainting() { return mMode == PAINTING; }
  



  bool IsForImageVisibility() { return mMode == IMAGE_VISIBILITY; }
  bool WillComputePluginGeometry() { return mWillComputePluginGeometry; }
  



  bool IsBackgroundOnly() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "don't call this if we're not in a presshell");
    return CurrentPresShellState()->mIsBackgroundOnly;
  }
  






  bool IsAtRootOfPseudoStackingContext() { return mIsAtRootOfPseudoStackingContext; }

  



  nsISelection* GetBoundingSelection() { return mBoundingSelection; }

  



  const nsIFrame* FindReferenceFrameFor(const nsIFrame *aFrame,
                                        nsPoint* aOffset = nullptr);
  
  



  nsIFrame* RootReferenceFrame() 
  {
    return mReferenceFrame;
  }

  





  const nsPoint ToReferenceFrame(const nsIFrame* aFrame) {
    nsPoint result;
    FindReferenceFrameFor(aFrame, &result);
    return result;
  }
  





  void SetIgnoreScrollFrame(nsIFrame* aFrame) { mIgnoreScrollFrame = aFrame; }
  


  nsIFrame* GetIgnoreScrollFrame() { return mIgnoreScrollFrame; }
  


  ViewID GetCurrentScrollParentId() const { return mCurrentScrollParentId; }
  



  void GetScrollbarInfo(ViewID* aOutScrollbarTarget, uint32_t* aOutScrollbarFlags)
  {
    *aOutScrollbarTarget = mCurrentScrollbarTarget;
    *aOutScrollbarFlags = mCurrentScrollbarFlags;
  }
  




  void SetIncludeAllOutOfFlows() { mIncludeAllOutOfFlows = true; }
  bool GetIncludeAllOutOfFlows() const { return mIncludeAllOutOfFlows; }
  



  void SetSelectedFramesOnly() { mSelectedFramesOnly = true; }
  bool GetSelectedFramesOnly() { return mSelectedFramesOnly; }
  



  void SetAccurateVisibleRegions() { mAccurateVisibleRegions = true; }
  bool GetAccurateVisibleRegions() { return mAccurateVisibleRegions; }
  



  bool IsBuildingCaret() { return mBuildCaret; }
  



  void IgnorePaintSuppression() { mIgnoreSuppression = true; }
  


  bool IsIgnoringPaintSuppression() { return mIgnoreSuppression; }
  



  bool GetHadToIgnorePaintSuppression() { return mHadToIgnoreSuppression; }
  


  void SetPaintingToWindow(bool aToWindow) { mIsPaintingToWindow = aToWindow; }
  bool IsPaintingToWindow() const { return mIsPaintingToWindow; }
  


  void SetDescendIntoSubdocuments(bool aDescend) { mDescendIntoSubdocuments = aDescend; }
  bool GetDescendIntoSubdocuments() { return mDescendIntoSubdocuments; }

  



  const nsRect& GetDirtyRect() { return mDirtyRect; }
  const nsIFrame* GetCurrentFrame() { return mCurrentFrame; }
  const nsIFrame* GetCurrentReferenceFrame() { return mCurrentReferenceFrame; }
  const nsPoint& GetCurrentFrameOffsetToReferenceFrame() { return mCurrentOffsetToReferenceFrame; }

  



  bool AllowMergingAndFlattening() { return mAllowMergingAndFlattening; }
  void SetAllowMergingAndFlattening(bool aAllow) { mAllowMergingAndFlattening = aAllow; }

  nsDisplayLayerEventRegions* GetLayerEventRegions() { return mLayerEventRegions; }
  void SetLayerEventRegions(nsDisplayLayerEventRegions* aItem)
  {
    mLayerEventRegions = aItem;
  }
  bool IsBuildingLayerEventRegions()
  {
    return (gfxPrefs::LayoutEventRegionsEnabled() && mMode == PAINTING);
  }

  bool GetAncestorHasTouchEventHandler() { return mAncestorHasTouchEventHandler; }
  void SetAncestorHasTouchEventHandler(bool aValue)
  {
    mAncestorHasTouchEventHandler = aValue;
  }

  bool HaveScrollableDisplayPort() const { return mHaveScrollableDisplayPort; }
  void SetHaveScrollableDisplayPort() { mHaveScrollableDisplayPort = true; }

  bool SetIsCompositingCheap(bool aCompositingCheap) { 
    bool temp = mIsCompositingCheap; 
    mIsCompositingCheap = aCompositingCheap;
    return temp;
  }
  bool IsCompositingCheap() const { return mIsCompositingCheap; }
  


  void DisplayCaret(nsIFrame* aFrame, const nsRect& aDirtyRect,
                    nsDisplayList* aList) {
    nsIFrame* frame = GetCaretFrame();
    if (aFrame == frame) {
      frame->DisplayCaret(this, aDirtyRect, aList);
    }
  }
  



  nsIFrame* GetCaretFrame() {
    return CurrentPresShellState()->mCaretFrame;
  }
  


  const nsRect& GetCaretRect() {
    return CurrentPresShellState()->mCaretRect;
  }
  


  nsCaret* GetCaret();
  



  void EnterPresShell(nsIFrame* aReferenceFrame);
  






  void ResetMarkedFramesForDisplayList();
  


  void LeavePresShell(nsIFrame* aReferenceFrame);

  



  bool IsInTransform() const { return mInTransform; }
  



  void SetInTransform(bool aInTransform) { mInTransform = aInTransform; }

  



  bool IsInSubdocument() { return mPresShellStates.Length() > 1; }

  


  bool ShouldSyncDecodeImages() { return mSyncDecodeImages; }

  




  void SetSyncDecodeImages(bool aSyncDecodeImages) {
    mSyncDecodeImages = aSyncDecodeImages;
  }

  




  uint32_t GetBackgroundPaintFlags();

  





  void SubtractFromVisibleRegion(nsRegion* aVisibleRegion,
                                 const nsRegion& aRegion);

  







  void MarkFramesForDisplayList(nsIFrame* aDirtyFrame,
                                const nsFrameList& aFrames,
                                const nsRect& aDirtyRect);
  




  void MarkPreserve3DFramesForDisplayList(nsIFrame* aDirtyFrame, const nsRect& aDirtyRect);

  const nsTArray<ThemeGeometry>& GetThemeGeometries() { return mThemeGeometries; }

  




  bool ShouldDescendIntoFrame(nsIFrame* aFrame) const {
    return
      (aFrame->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO) ||
      GetIncludeAllOutOfFlows();
  }

  













  void RegisterThemeGeometry(uint8_t aWidgetType,
                             const nsIntRect& aRect) {
    if (mIsPaintingToWindow) {
      mThemeGeometries.AppendElement(ThemeGeometry(aWidgetType, aRect));
    }
  }

  





  void AdjustWindowDraggingRegion(nsIFrame* aFrame);

  const nsRegion& GetWindowDraggingRegion() { return mWindowDraggingRegion; }

  




  void* Allocate(size_t aSize);

  



  const DisplayItemClip* AllocateDisplayItemClip(const DisplayItemClip& aOriginal);

  






  static void AddAnimationsAndTransitionsToLayer(Layer* aLayer,
                                                 nsDisplayListBuilder* aBuilder,
                                                 nsDisplayItem* aItem,
                                                 nsIFrame* aFrame,
                                                 nsCSSProperty aProperty);
  





  class AutoBuildingDisplayList;
  friend class AutoBuildingDisplayList;
  class AutoBuildingDisplayList {
  public:
    AutoBuildingDisplayList(nsDisplayListBuilder* aBuilder,
                            nsIFrame* aForChild,
                            const nsRect& aDirtyRect, bool aIsRoot)
      : mBuilder(aBuilder),
        mPrevFrame(aBuilder->mCurrentFrame),
        mPrevReferenceFrame(aBuilder->mCurrentReferenceFrame),
        mPrevLayerEventRegions(aBuilder->mLayerEventRegions),
        mPrevOffset(aBuilder->mCurrentOffsetToReferenceFrame),
        mPrevDirtyRect(aBuilder->mDirtyRect),
        mPrevIsAtRootOfPseudoStackingContext(aBuilder->mIsAtRootOfPseudoStackingContext),
        mPrevAncestorHasTouchEventHandler(aBuilder->mAncestorHasTouchEventHandler)
    {
      if (aForChild->IsTransformed()) {
        aBuilder->mCurrentOffsetToReferenceFrame = nsPoint();
        aBuilder->mCurrentReferenceFrame = aForChild;
      } else if (aBuilder->mCurrentFrame == aForChild->GetParent()) {
        aBuilder->mCurrentOffsetToReferenceFrame += aForChild->GetPosition();
      } else {
        aBuilder->mCurrentReferenceFrame =
          aBuilder->FindReferenceFrameFor(aForChild,
              &aBuilder->mCurrentOffsetToReferenceFrame);
      }
      aBuilder->mCurrentFrame = aForChild;
      aBuilder->mDirtyRect = aDirtyRect;
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
    }
    void SetDirtyRect(const nsRect& aRect) {
      mBuilder->mDirtyRect = aRect;
    }
    void SetReferenceFrameAndCurrentOffset(const nsIFrame* aFrame, const nsPoint& aOffset) {
      mBuilder->mCurrentReferenceFrame = aFrame;
      mBuilder->mCurrentOffsetToReferenceFrame = aOffset;
    }
    ~AutoBuildingDisplayList() {
      mBuilder->mCurrentFrame = mPrevFrame;
      mBuilder->mCurrentReferenceFrame = mPrevReferenceFrame;
      mBuilder->mLayerEventRegions = mPrevLayerEventRegions;
      mBuilder->mCurrentOffsetToReferenceFrame = mPrevOffset;
      mBuilder->mDirtyRect = mPrevDirtyRect;
      mBuilder->mIsAtRootOfPseudoStackingContext = mPrevIsAtRootOfPseudoStackingContext;
      mBuilder->mAncestorHasTouchEventHandler = mPrevAncestorHasTouchEventHandler;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    const nsIFrame*       mPrevFrame;
    const nsIFrame*       mPrevReferenceFrame;
    nsDisplayLayerEventRegions* mPrevLayerEventRegions;
    nsPoint               mPrevOffset;
    nsRect                mPrevDirtyRect;
    bool                  mPrevIsAtRootOfPseudoStackingContext;
    bool                  mPrevAncestorHasTouchEventHandler;
  };

  


  class AutoInTransformSetter;
  friend class AutoInTransformSetter;
  class AutoInTransformSetter {
  public:
    AutoInTransformSetter(nsDisplayListBuilder* aBuilder, bool aInTransform)
      : mBuilder(aBuilder), mOldValue(aBuilder->mInTransform) {
      aBuilder->mInTransform = aInTransform;
    }
    ~AutoInTransformSetter() {
      mBuilder->mInTransform = mOldValue;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    bool                  mOldValue;
  };

  


  class AutoCurrentScrollParentIdSetter;
  friend class AutoCurrentScrollParentIdSetter;
  class AutoCurrentScrollParentIdSetter {
  public:
    AutoCurrentScrollParentIdSetter(nsDisplayListBuilder* aBuilder, ViewID aScrollId)
      : mBuilder(aBuilder), mOldValue(aBuilder->mCurrentScrollParentId) {
      aBuilder->mCurrentScrollParentId = aScrollId;
    }
    ~AutoCurrentScrollParentIdSetter() {
      mBuilder->mCurrentScrollParentId = mOldValue;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    ViewID                mOldValue;
  };

  



  class AutoCurrentScrollbarInfoSetter;
  friend class AutoCurrentScrollbarInfoSetter;
  class AutoCurrentScrollbarInfoSetter {
  public:
    AutoCurrentScrollbarInfoSetter(nsDisplayListBuilder* aBuilder, ViewID aScrollTargetID,
                                   uint32_t aScrollbarFlags)
     : mBuilder(aBuilder) {
      aBuilder->mCurrentScrollbarTarget = aScrollTargetID;
      aBuilder->mCurrentScrollbarFlags = aScrollbarFlags;
    }
    ~AutoCurrentScrollbarInfoSetter() {
      
      mBuilder->mCurrentScrollbarTarget = FrameMetrics::NULL_SCROLL_ID;
      mBuilder->mCurrentScrollbarFlags = 0;
    }
  private:
    nsDisplayListBuilder* mBuilder;
  };

  
  nsDisplayTableItem* GetCurrentTableItem() { return mCurrentTableItem; }
  void SetCurrentTableItem(nsDisplayTableItem* aTableItem) { mCurrentTableItem = aTableItem; }

  struct OutOfFlowDisplayData {
    OutOfFlowDisplayData(const DisplayItemClip& aContainingBlockClip,
                         const nsRect &aDirtyRect)
      : mContainingBlockClip(aContainingBlockClip)
      , mDirtyRect(aDirtyRect)
    {}
    explicit OutOfFlowDisplayData(const nsRect &aDirtyRect)
      : mDirtyRect(aDirtyRect)
    {}
    DisplayItemClip mContainingBlockClip;
    nsRect mDirtyRect;
  };
  static void DestroyOutOfFlowDisplayData(void* aPropertyValue)
  {
    delete static_cast<OutOfFlowDisplayData*>(aPropertyValue);
  }

  NS_DECLARE_FRAME_PROPERTY(OutOfFlowDisplayDataProperty, DestroyOutOfFlowDisplayData)
  NS_DECLARE_FRAME_PROPERTY(Preserve3DDirtyRectProperty, nsIFrame::DestroyRect)

  nsPresContext* CurrentPresContext() {
    return CurrentPresShellState()->mPresShell->GetPresContext();
  }

  



  
  void AddWindowOpaqueRegion(const nsRegion& bounds) {
    mWindowOpaqueRegion.Or(mWindowOpaqueRegion, bounds);
  }
  



  const nsRegion& GetWindowOpaqueRegion() {
    return mWindowOpaqueRegion;
  }
  void SetGlassDisplayItem(nsDisplayItem* aItem) {
    if (mGlassDisplayItem) {
      
      
      
      
      NS_WARNING("Multiple glass backgrounds found?");
    } else {
      mGlassDisplayItem = aItem;
    }
  }
  bool NeedToForceTransparentSurfaceForItem(nsDisplayItem* aItem);

  void SetContainsPluginItem() { mContainsPluginItem = true; }
  bool ContainsPluginItem() { return mContainsPluginItem; }

  




  void SetContainsBlendMode(uint8_t aBlendMode);
  void SetContainsBlendModes(const BlendModeSet& aModes) {
    mContainedBlendModes = aModes;
  }
  bool ContainsBlendMode() const { return !mContainedBlendModes.isEmpty(); }
  BlendModeSet& ContainedBlendModes() {
    return mContainedBlendModes;
  }

  DisplayListClipState& ClipState() { return mClipState; }

  






  void AddToWillChangeBudget(nsIFrame* aFrame, const nsSize& aSize);

  bool IsInWillChangeBudget(nsIFrame* aFrame) const;

private:
  void MarkOutOfFlowFrameForDisplay(nsIFrame* aDirtyFrame, nsIFrame* aFrame,
                                    const nsRect& aDirtyRect);

  struct PresShellState {
    nsIPresShell* mPresShell;
    nsIFrame*     mCaretFrame;
    nsRect        mCaretRect;
    uint32_t      mFirstFrameMarkedForDisplay;
    bool          mIsBackgroundOnly;
  };

  PresShellState* CurrentPresShellState() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "Someone forgot to enter a presshell");
    return &mPresShellStates[mPresShellStates.Length() - 1];
  }

  struct DocumentWillChangeBudget {
    DocumentWillChangeBudget()
      : mBudget(0)
    {}

    uint32_t mBudget;
  };

  nsIFrame*                      mReferenceFrame;
  nsIFrame*                      mIgnoreScrollFrame;
  nsDisplayLayerEventRegions*    mLayerEventRegions;
  PLArenaPool                    mPool;
  nsCOMPtr<nsISelection>         mBoundingSelection;
  nsAutoTArray<PresShellState,8> mPresShellStates;
  nsAutoTArray<nsIFrame*,100>    mFramesMarkedForDisplay;
  nsAutoTArray<ThemeGeometry,2>  mThemeGeometries;
  nsDisplayTableItem*            mCurrentTableItem;
  DisplayListClipState           mClipState;
  
  
  const nsIFrame*                mCurrentFrame;
  
  const nsIFrame*                mCurrentReferenceFrame;
  
  nsPoint                        mCurrentOffsetToReferenceFrame;
  
  nsDataHashtable<nsPtrHashKey<nsPresContext>, DocumentWillChangeBudget>
                                 mWillChangeBudget;
  
  mutable mozilla::DebugOnly<bool> mWillChangeBudgetCalculated;
  
  nsRect                         mDirtyRect;
  nsRegion                       mWindowOpaqueRegion;
  nsRegion                       mWindowDraggingRegion;
  
  nsDisplayItem*                 mGlassDisplayItem;
  nsTArray<DisplayItemClip*>     mDisplayItemClipsToDestroy;
  Mode                           mMode;
  ViewID                         mCurrentScrollParentId;
  ViewID                         mCurrentScrollbarTarget;
  uint32_t                       mCurrentScrollbarFlags;
  BlendModeSet                   mContainedBlendModes;
  bool                           mBuildCaret;
  bool                           mIgnoreSuppression;
  bool                           mHadToIgnoreSuppression;
  bool                           mIsAtRootOfPseudoStackingContext;
  bool                           mIncludeAllOutOfFlows;
  bool                           mDescendIntoSubdocuments;
  bool                           mSelectedFramesOnly;
  bool                           mAccurateVisibleRegions;
  bool                           mAllowMergingAndFlattening;
  bool                           mWillComputePluginGeometry;
  
  
  bool                           mInTransform;
  bool                           mSyncDecodeImages;
  bool                           mIsPaintingToWindow;
  bool                           mIsCompositingCheap;
  bool                           mContainsPluginItem;
  bool                           mAncestorHasTouchEventHandler;
  
  
  
  bool                           mHaveScrollableDisplayPort;
};

class nsDisplayItem;
class nsDisplayList;





class nsDisplayItemLink {
  
  
protected:
  nsDisplayItemLink() : mAbove(nullptr) {}
  nsDisplayItem* mAbove;  
  
  friend class nsDisplayList;
};
















class nsDisplayItem : public nsDisplayItemLink {
public:
  typedef mozilla::ContainerLayerParameters ContainerLayerParameters;
  typedef mozilla::DisplayItemClip DisplayItemClip;
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::LayerState LayerState;

  
  
  nsDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame);
  



  explicit nsDisplayItem(nsIFrame* aFrame)
    : mFrame(aFrame)
    , mClip(nullptr)
    , mReferenceFrame(nullptr)
#ifdef MOZ_DUMP_PAINTING
    , mPainted(false)
#endif
  {
  }
  virtual ~nsDisplayItem() {}

  void* operator new(size_t aSize,
                     nsDisplayListBuilder* aBuilder) CPP_THROW_NEW {
    return aBuilder->Allocate(aSize);
  }


#include "nsDisplayItemTypes.h"

  struct HitTestState {
    explicit HitTestState() {}

    ~HitTestState() {
      NS_ASSERTION(mItemBuffer.Length() == 0,
                   "mItemBuffer should have been cleared");
    }

    nsAutoTArray<nsDisplayItem*, 100> mItemBuffer;
  };

  




  virtual Type GetType() = 0;
  




  virtual uint32_t GetPerFrameKey() { return uint32_t(GetType()); }
  












  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {}
  




  inline nsIFrame* Frame() const { return mFrame; }
  




  virtual int32_t ZIndex() const;
  










  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
  {
    *aSnap = false;
    return nsRect(ToReferenceFrame(), Frame()->GetSize());
  }
  



  virtual bool IsInvisibleInRect(const nsRect& aRect)
  {
    return false;
  }
  




  nsRect GetClippedBounds(nsDisplayListBuilder* aBuilder);
  nsRect GetBorderRect() {
    return nsRect(ToReferenceFrame(), Frame()->GetSize());
  }
  nsRect GetPaddingRect() {
    return Frame()->GetPaddingRectRelativeToSelf() + ToReferenceFrame();
  }
  nsRect GetContentRect() {
    return Frame()->GetContentRectRelativeToSelf() + ToReferenceFrame();
  }

  



  virtual bool IsInvalid(nsRect& aRect) { 
    bool result = mFrame ? mFrame->IsInvalid(aRect) : false;
    aRect += ToReferenceFrame();
    return result;
  }

  












  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder)
  {
    return new nsDisplayItemGenericGeometry(this, aBuilder);
  }

  















  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion)
  {
    const nsDisplayItemGenericGeometry* geometry = static_cast<const nsDisplayItemGenericGeometry*>(aGeometry);
    bool snap;
    if (!geometry->mBounds.IsEqualInterior(GetBounds(aBuilder, &snap)) ||
        !geometry->mBorderRect.IsEqualInterior(GetBorderRect())) {
      aInvalidRegion->Or(GetBounds(aBuilder, &snap), geometry->mBounds);
    }
  }

  



  void ComputeInvalidationRegionDifference(nsDisplayListBuilder* aBuilder,
                                           const nsDisplayItemBoundsGeometry* aGeometry,
                                           nsRegion* aInvalidRegion)
  {
    bool snap;
    nsRect bounds = GetBounds(aBuilder, &snap);

    if (!aGeometry->mBounds.IsEqualInterior(bounds)) {
      nscoord radii[8];
      if (aGeometry->mHasRoundedCorners ||
          Frame()->GetBorderRadii(radii)) {
        aInvalidRegion->Or(aGeometry->mBounds, bounds);
      } else {
        aInvalidRegion->Xor(aGeometry->mBounds, bounds);
      }
    }
  }

  




  void AddInvalidRegionForSyncDecodeBackgroundImages(
    nsDisplayListBuilder* aBuilder,
    const nsDisplayItemGeometry* aGeometry,
    nsRegion* aInvalidRegion);

  







  virtual void NotifyRenderingChanged() {}

  









  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap)
  {
    *aSnap = false;
    return nsRegion();
  }
  




  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) { return false; }
  



  virtual bool ShouldFixToViewport(LayerManager* aManager)
  { return false; }

  virtual bool ClearsBackground()
  { return false; }

  



  static bool ForceActiveLayers();

  



  static int32_t MaxActiveLayers();

  






















  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters)
  { return mozilla::LAYER_NONE; }
  



  virtual bool ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder)
  { return false; }
  




  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) {}

#ifdef MOZ_DUMP_PAINTING
  


  bool Painted() { return mPainted; }

  


  void SetPainted() { mPainted = true; }
#endif

  













  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters)
  { return nullptr; }

  



















  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion);

  








  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
    return false;
  }

  




  virtual void GetMergedFrames(nsTArray<nsIFrame*>* aFrames) {}

  





  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) {
    return false;
  }

  




  virtual nsDisplayList* GetSameCoordinateSystemChildren() { return nullptr; }
  virtual void UpdateBounds(nsDisplayListBuilder* aBuilder) {}

  



  virtual nsDisplayList* GetChildren() { return nullptr; }

  


  const nsRect& GetVisibleRect() const { return mVisibleRect; }

  





  virtual const nsRect& GetVisibleRectForChildren() const { return mVisibleRect; }

  



  virtual bool ApplyOpacity(nsDisplayListBuilder* aBuilder,
                            float aOpacity,
                            const DisplayItemClip* aClip) {
    return false;
  }
  
#ifdef MOZ_DUMP_PAINTING
  


  virtual const char* Name() = 0;

  virtual void WriteDebugInfo(nsACString& aTo) {}
#endif

  nsDisplayItem* GetAbove() { return mAbove; }

  






  bool RecomputeVisibility(nsDisplayListBuilder* aBuilder,
                           nsRegion* aVisibleRegion);

  


  const nsPoint& ToReferenceFrame() const {
    NS_ASSERTION(mFrame, "No frame?");
    return mToReferenceFrame;
  }
  



  const nsIFrame* ReferenceFrame() const { return mReferenceFrame; }

  


  virtual const nsIFrame* ReferenceFrameForChildren() const { return mReferenceFrame; }

  





  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) { return nsRect(); }

  


  virtual void DisableComponentAlpha() {}

  


  virtual bool CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder) {
    return false;
  }
  
  virtual bool SupportsOptimizingToImage() { return false; }

  const DisplayItemClip& GetClip()
  {
    return mClip ? *mClip : DisplayItemClip::NoClip();
  }
  void SetClip(nsDisplayListBuilder* aBuilder, const DisplayItemClip& aClip)
  {
    if (!aClip.HasClip()) {
      mClip = nullptr;
      return;
    }
    mClip = aBuilder->AllocateDisplayItemClip(aClip);
  }

  void IntersectClip(nsDisplayListBuilder* aBuilder, const DisplayItemClip& aClip)
  {
    if (mClip) {
      DisplayItemClip temp = *mClip;
      temp.IntersectWith(aClip);
      SetClip(aBuilder, temp);
    } else {
      SetClip(aBuilder, aClip);
    }
  }

protected:
  friend class nsDisplayList;

  nsDisplayItem() { mAbove = nullptr; }

  nsIFrame* mFrame;
  const DisplayItemClip* mClip;
  
  const nsIFrame* mReferenceFrame;
  
  nsPoint   mToReferenceFrame;
  
  
  
  
  
  
  nsRect    mVisibleRect;
#ifdef MOZ_DUMP_PAINTING
  
  bool      mPainted;
#endif
};
















class nsDisplayList {
public:
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::layers::PaintedLayer PaintedLayer;

  


  nsDisplayList()
    : mIsOpaque(false)
    , mForceTransparentSurface(false)
  {
    mTop = &mSentinel;
    mSentinel.mAbove = nullptr;
  }
  ~nsDisplayList() {
    if (mSentinel.mAbove) {
      NS_WARNING("Nonempty list left over?");
    }
    DeleteAll();
  }

  



  void AppendToTop(nsDisplayItem* aItem) {
    NS_ASSERTION(aItem, "No item to append!");
    NS_ASSERTION(!aItem->mAbove, "Already in a list!");
    mTop->mAbove = aItem;
    mTop = aItem;
  }
  
  



  void AppendNewToTop(nsDisplayItem* aItem) {
    if (aItem) {
      AppendToTop(aItem);
    }
  }
  
  



  void AppendNewToBottom(nsDisplayItem* aItem) {
    if (aItem) {
      AppendToBottom(aItem);
    }
  }
  
  



  void AppendToBottom(nsDisplayItem* aItem) {
    NS_ASSERTION(aItem, "No item to append!");
    NS_ASSERTION(!aItem->mAbove, "Already in a list!");
    aItem->mAbove = mSentinel.mAbove;
    mSentinel.mAbove = aItem;
    if (mTop == &mSentinel) {
      mTop = aItem;
    }
  }
  
  


  void AppendToTop(nsDisplayList* aList) {
    if (aList->mSentinel.mAbove) {
      mTop->mAbove = aList->mSentinel.mAbove;
      mTop = aList->mTop;
      aList->mTop = &aList->mSentinel;
      aList->mSentinel.mAbove = nullptr;
    }
  }
  
  


  void AppendToBottom(nsDisplayList* aList) {
    if (aList->mSentinel.mAbove) {
      aList->mTop->mAbove = mSentinel.mAbove;
      mSentinel.mAbove = aList->mSentinel.mAbove;
      if (mTop == &mSentinel) {
        mTop = aList->mTop;
      }
           
      aList->mTop = &aList->mSentinel;
      aList->mSentinel.mAbove = nullptr;
    }
  }
  
  


  nsDisplayItem* RemoveBottom();
  
  


  void DeleteAll();
  
  


  nsDisplayItem* GetTop() const {
    return mTop != &mSentinel ? static_cast<nsDisplayItem*>(mTop) : nullptr;
  }
  


  nsDisplayItem* GetBottom() const { return mSentinel.mAbove; }
  bool IsEmpty() const { return mTop == &mSentinel; }
  
  



  uint32_t Count() const;
  








  void SortByZOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);
  







  void SortByContentOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);

  





  typedef bool (* SortLEQ)(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                             void* aClosure);
  void Sort(nsDisplayListBuilder* aBuilder, SortLEQ aCmp, void* aClosure);

  























  bool ComputeVisibilityForSublist(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aListVisibleBounds,
                                   nsIFrame* aDisplayPortFrame = nullptr);

  







  bool ComputeVisibilityForRoot(nsDisplayListBuilder* aBuilder,
                                nsRegion* aVisibleRegion,
                                nsIFrame* aDisplayPortFrame = nullptr);

  



  bool IsOpaque() const {
    return mIsOpaque;
  }

  


  bool NeedsTransparentSurface() const {
    return mForceTransparentSurface;
  }
  



























  enum {
    PAINT_DEFAULT = 0,
    PAINT_USE_WIDGET_LAYERS = 0x01,
    PAINT_FLUSH_LAYERS = 0x02,
    PAINT_EXISTING_TRANSACTION = 0x04,
    PAINT_NO_COMPOSITE = 0x08,
    PAINT_COMPRESSED = 0x10
  };
  void PaintRoot(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                 uint32_t aFlags);
  



  void PaintForFrame(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     nsIFrame* aForFrame, uint32_t aFlags);
  



  nsRect GetBounds(nsDisplayListBuilder* aBuilder) const;
  



  void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
               nsDisplayItem::HitTestState* aState,
               nsTArray<nsIFrame*> *aOutFrames) const;
  



  nsRect GetVisibleRect() const;

  void SetIsOpaque()
  {
    mIsOpaque = true;
  }
  void SetNeedsTransparentSurface()
  {
    mForceTransparentSurface = true;
  }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;
  
  nsDisplayItemLink  mSentinel;
  nsDisplayItemLink* mTop;

  
  nsRect mVisibleRect;
  
  
  
  bool mIsOpaque;
  
  
  bool mForceTransparentSurface;
};








class nsDisplayListSet {
public:
  



  nsDisplayList* BorderBackground() const { return mBorderBackground; }
  



  nsDisplayList* BlockBorderBackgrounds() const { return mBlockBorderBackgrounds; }
  



  nsDisplayList* Floats() const { return mFloats; }
  




  nsDisplayList* PositionedDescendants() const { return mPositioned; }
  



  nsDisplayList* Outlines() const { return mOutlines; }
  


  nsDisplayList* Content() const { return mContent; }
  
  nsDisplayListSet(nsDisplayList* aBorderBackground,
                   nsDisplayList* aBlockBorderBackgrounds,
                   nsDisplayList* aFloats,
                   nsDisplayList* aContent,
                   nsDisplayList* aPositionedDescendants,
                   nsDisplayList* aOutlines) :
     mBorderBackground(aBorderBackground),
     mBlockBorderBackgrounds(aBlockBorderBackgrounds),
     mFloats(aFloats),
     mContent(aContent),
     mPositioned(aPositionedDescendants),
     mOutlines(aOutlines) {
  }

  


  
  nsDisplayListSet(const nsDisplayListSet& aLists,
                   nsDisplayList* aBorderBackground) :
     mBorderBackground(aBorderBackground),
     mBlockBorderBackgrounds(aLists.BlockBorderBackgrounds()),
     mFloats(aLists.Floats()),
     mContent(aLists.Content()),
     mPositioned(aLists.PositionedDescendants()),
     mOutlines(aLists.Outlines()) {
  }
  
  



  void MoveTo(const nsDisplayListSet& aDestination) const;

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;

protected:
  nsDisplayList* mBorderBackground;
  nsDisplayList* mBlockBorderBackgrounds;
  nsDisplayList* mFloats;
  nsDisplayList* mContent;
  nsDisplayList* mPositioned;
  nsDisplayList* mOutlines;
};





struct nsDisplayListCollection : public nsDisplayListSet {
  nsDisplayListCollection() :
    nsDisplayListSet(&mLists[0], &mLists[1], &mLists[2], &mLists[3], &mLists[4],
                     &mLists[5]) {}
  explicit nsDisplayListCollection(nsDisplayList* aBorderBackground) :
    nsDisplayListSet(aBorderBackground, &mLists[1], &mLists[2], &mLists[3], &mLists[4],
                     &mLists[5]) {}

  

                     
  void SortAllByContentOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor) {
    for (int32_t i = 0; i < 6; ++i) {
      mLists[i].SortByContentOrder(aBuilder, aCommonAncestor);
    }
  }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;

  nsDisplayList mLists[6];
};


class nsDisplayImageContainer : public nsDisplayItem {
public:
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::ImageLayer ImageLayer;

  nsDisplayImageContainer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {}

  virtual already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager,
                                                        nsDisplayListBuilder* aBuilder) = 0;
  virtual void ConfigureLayer(ImageLayer* aLayer, const nsIntPoint& aOffset) = 0;

  virtual bool SupportsOptimizingToImage() MOZ_OVERRIDE { return true; }
};











class nsDisplayGeneric : public nsDisplayItem {
public:
  typedef void (* PaintCallback)(nsIFrame* aFrame, nsRenderingContext* aCtx,
                                 const nsRect& aDirtyRect, nsPoint aFramePt);

  nsDisplayGeneric(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                   PaintCallback aPaint, const char* aName, Type aType)
    : nsDisplayItem(aBuilder, aFrame), mPaint(aPaint)
#ifdef MOZ_DUMP_PAINTING
      , mName(aName)
#endif
      , mType(aType)
  {
    MOZ_COUNT_CTOR(nsDisplayGeneric);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayGeneric() {
    MOZ_COUNT_DTOR(nsDisplayGeneric);
  }
#endif
  
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) MOZ_OVERRIDE {
    mPaint(mFrame, aCtx, mVisibleRect, ToReferenceFrame());
  }
  NS_DISPLAY_DECL_NAME(mName, mType)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    if (mType == nsDisplayItem::TYPE_HEADER_FOOTER) {
      bool snap;
      return GetBounds(aBuilder, &snap);
    }
    return nsRect();
  }

protected:
  PaintCallback mPaint;
#ifdef MOZ_DUMP_PAINTING
  const char*   mName;
#endif
  Type mType;
};






class nsDisplayGenericOverflow : public nsDisplayGeneric {
  public:
    nsDisplayGenericOverflow(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                             PaintCallback aPaint, const char* aName, Type aType)
      : nsDisplayGeneric(aBuilder, aFrame, aPaint, aName, aType)
    {
      MOZ_COUNT_CTOR(nsDisplayGenericOverflow);
    }
  #ifdef NS_BUILD_REFCNT_LOGGING
    virtual ~nsDisplayGenericOverflow() {
      MOZ_COUNT_DTOR(nsDisplayGenericOverflow);
    }
  #endif

    


    virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                             bool* aSnap) MOZ_OVERRIDE
    {
      *aSnap = false;
      return Frame()->GetVisualOverflowRect() + ToReferenceFrame();
    }
};

#if defined(MOZ_REFLOW_PERF_DSP) && defined(MOZ_REFLOW_PERF)












class nsDisplayReflowCount : public nsDisplayItem {
public:
  nsDisplayReflowCount(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                       const char* aFrameName,
                       uint32_t aColor = 0)
    : nsDisplayItem(aBuilder, aFrame),
      mFrameName(aFrameName),
      mColor(aColor)
  {
    MOZ_COUNT_CTOR(nsDisplayReflowCount);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayReflowCount() {
    MOZ_COUNT_DTOR(nsDisplayReflowCount);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE {
    mFrame->PresContext()->PresShell()->PaintCount(mFrameName, aCtx,
                                                   mFrame->PresContext(),
                                                   mFrame, ToReferenceFrame(),
                                                   mColor);
  }
  NS_DISPLAY_DECL_NAME("nsDisplayReflowCount", TYPE_REFLOW_COUNT)
protected:
  const char* mFrameName;
  nscolor mColor;
};

#define DO_GLOBAL_REFLOW_COUNT_DSP(_name)                                     \
  PR_BEGIN_MACRO                                                              \
    if (!aBuilder->IsBackgroundOnly() && !aBuilder->IsForEventDelivery() &&   \
        PresContext()->PresShell()->IsPaintingFrameCounts()) {                \
        aLists.Outlines()->AppendNewToTop(                                    \
            new (aBuilder) nsDisplayReflowCount(aBuilder, this, _name));      \
    }                                                                         \
  PR_END_MACRO

#define DO_GLOBAL_REFLOW_COUNT_DSP_COLOR(_name, _color)                       \
  PR_BEGIN_MACRO                                                              \
    if (!aBuilder->IsBackgroundOnly() && !aBuilder->IsForEventDelivery() &&   \
        PresContext()->PresShell()->IsPaintingFrameCounts()) {                \
        aLists.Outlines()->AppendNewToTop(                                    \
             new (aBuilder) nsDisplayReflowCount(aBuilder, this, _name, _color)); \
    }                                                                         \
  PR_END_MACRO




#define DECL_DO_GLOBAL_REFLOW_COUNT_DSP(_class, _super)                   \
  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,                 \
                        const nsRect&           aDirtyRect,               \
                        const nsDisplayListSet& aLists) {                 \
    DO_GLOBAL_REFLOW_COUNT_DSP(#_class);                                  \
    _super::BuildDisplayList(aBuilder, aDirtyRect, aLists);               \
  }

#else 

#define DO_GLOBAL_REFLOW_COUNT_DSP(_name)
#define DO_GLOBAL_REFLOW_COUNT_DSP_COLOR(_name, _color)
#define DECL_DO_GLOBAL_REFLOW_COUNT_DSP(_class, _super)

#endif 

class nsDisplayCaret : public nsDisplayItem {
public:
  nsDisplayCaret(nsDisplayListBuilder* aBuilder, nsIFrame* aCaretFrame);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayCaret();
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Caret", TYPE_CARET)
protected:
  nsRefPtr<nsCaret> mCaret;
  nsRect mBounds;
};




class nsDisplayBorder : public nsDisplayItem {
public:
  nsDisplayBorder(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayBorder);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBorder() {
    MOZ_COUNT_DTOR(nsDisplayBorder);
  }
#endif

  virtual bool IsInvisibleInRect(const nsRect& aRect) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Border", TYPE_BORDER)
  
  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;

protected:
  nsRect CalculateBounds(const nsStyleBorder& aStyleBorder);
};












class nsDisplaySolidColor : public nsDisplayItem {
public:
  nsDisplaySolidColor(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                      const nsRect& aBounds, nscolor aColor)
    : nsDisplayItem(aBuilder, aFrame), mBounds(aBounds), mColor(aColor)
  {
    NS_ASSERTION(NS_GET_A(aColor) > 0, "Don't create invisible nsDisplaySolidColors!");
    MOZ_COUNT_CTOR(nsDisplaySolidColor);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySolidColor() {
    MOZ_COUNT_DTOR(nsDisplaySolidColor);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    nsRegion result;
    if (NS_GET_A(mColor) == 255) {
      result = GetBounds(aBuilder, aSnap);
    }
    return result;
  }

  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE
  {
    *aColor = mColor;
    return true;
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplaySolidColorGeometry(this, aBuilder, mColor);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    const nsDisplaySolidColorGeometry* geometry =
      static_cast<const nsDisplaySolidColorGeometry*>(aGeometry);
    if (mColor != geometry->mColor) {
      bool dummy;
      aInvalidRegion->Or(geometry->mBounds, GetBounds(aBuilder, &dummy));
      return;
    }
    ComputeInvalidationRegionDifference(aBuilder, geometry, aInvalidRegion);
  }

#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif

  NS_DISPLAY_DECL_NAME("SolidColor", TYPE_SOLID_COLOR)

private:
  nsRect  mBounds;
  nscolor mColor;
};





class nsDisplayBackgroundImage : public nsDisplayImageContainer {
public:
  





  nsDisplayBackgroundImage(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           uint32_t aLayer,
                           const nsStyleBackground* aBackgroundStyle);
  virtual ~nsDisplayBackgroundImage();

  
  
  static bool AppendBackgroundItemsToTop(nsDisplayListBuilder* aBuilder,
                                         nsIFrame* aFrame,
                                         nsDisplayList* aList);

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  


  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual uint32_t GetPerFrameKey() MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Background", TYPE_BACKGROUND)

  




  nsRect GetPositioningArea();

  






  bool RenderingMightDependOnPositioningAreaSizeChange();

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayBackgroundGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;
  
  virtual already_AddRefed<ImageContainer> GetContainer(LayerManager* aManager,
                                                        nsDisplayListBuilder *aBuilder) MOZ_OVERRIDE;
  virtual void ConfigureLayer(ImageLayer* aLayer, const nsIntPoint& aOffset) MOZ_OVERRIDE;

  static nsRegion GetInsideClipRegion(nsDisplayItem* aItem, nsPresContext* aPresContext, uint8_t aClip,
                                      const nsRect& aRect, bool* aSnap);

  virtual bool ShouldFixToViewport(LayerManager* aManager) MOZ_OVERRIDE;

protected:
  typedef class mozilla::layers::ImageContainer ImageContainer;
  typedef class mozilla::layers::ImageLayer ImageLayer;

  bool TryOptimizeToImageLayer(LayerManager* aManager, nsDisplayListBuilder* aBuilder);
  bool IsSingleFixedPositionImage(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aClipRect,
                                  gfxRect* aDestRect);
  nsRect GetBoundsInternal(nsDisplayListBuilder* aBuilder);

  void PaintInternal(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     const nsRect& aBounds, nsRect* aClipRect);

  
  
  const nsStyleBackground* mBackgroundStyle;
  
  nsRefPtr<ImageContainer> mImageContainer;
  gfxRect mDestRect;
  
  nsRect mBounds;
  uint32_t mLayer;
};





class nsDisplayThemedBackground : public nsDisplayItem {
public:
  nsDisplayThemedBackground(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame);
  virtual ~nsDisplayThemedBackground();

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  


  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("ThemedBackground", TYPE_THEMED_BACKGROUND)

  




  nsRect GetPositioningArea();

  



  bool IsWindowActive();

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayThemedBackgroundGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;

#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif
protected:
  nsRect GetBoundsInternal();

  void PaintInternal(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     const nsRect& aBounds, nsRect* aClipRect);

  nsRect mBounds;
  nsITheme::Transparency mThemeTransparency;
  uint8_t mAppearance;
};

class nsDisplayBackgroundColor : public nsDisplayItem
{
public:
  nsDisplayBackgroundColor(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           const nsStyleBackground* aBackgroundStyle,
                           nscolor aColor)
    : nsDisplayItem(aBuilder, aFrame)
    , mBackgroundStyle(aBackgroundStyle)
    , mColor(gfxRGBA(aColor))
  { }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;

  virtual bool ApplyOpacity(nsDisplayListBuilder* aBuilder,
                            float aOpacity,
                            const DisplayItemClip* aClip) MOZ_OVERRIDE;

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    return nsRect(ToReferenceFrame(), Frame()->GetSize());
  }

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplaySolidColorGeometry(this, aBuilder,
                                           NS_RGBA_FROM_GFXRGBA(mColor));
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    const nsDisplaySolidColorGeometry* geometry = static_cast<const nsDisplaySolidColorGeometry*>(aGeometry);
    if (NS_RGBA_FROM_GFXRGBA(mColor) != geometry->mColor) {
      bool dummy;
      aInvalidRegion->Or(geometry->mBounds, GetBounds(aBuilder, &dummy));
      return;
    }
    ComputeInvalidationRegionDifference(aBuilder, geometry, aInvalidRegion);
  }

  NS_DISPLAY_DECL_NAME("BackgroundColor", TYPE_BACKGROUND_COLOR)
#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif

protected:
  const nsStyleBackground* mBackgroundStyle;
  gfxRGBA mColor;
};

class nsDisplayClearBackground : public nsDisplayItem
{
public:
  nsDisplayClearBackground(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  { }

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    return nsRect(ToReferenceFrame(), Frame()->GetSize());
  }

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    return GetBounds(aBuilder, aSnap);
  }

  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE
  {
    *aColor = NS_RGBA(0, 0, 0, 0);
    return true;
  }

  virtual bool ClearsBackground() MOZ_OVERRIDE
  {
    return true;
  }

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE_FORCE;
  }

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("ClearBackground", TYPE_CLEAR_BACKGROUND)
};




class nsDisplayBoxShadowOuter MOZ_FINAL : public nsDisplayItem {
public:
  nsDisplayBoxShadowOuter(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
    , mOpacity(1.0) {
    MOZ_COUNT_CTOR(nsDisplayBoxShadowOuter);
    mBounds = GetBoundsInternal();
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBoxShadowOuter() {
    MOZ_COUNT_DTOR(nsDisplayBoxShadowOuter);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsInvisibleInRect(const nsRect& aRect) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("BoxShadowOuter", TYPE_BOX_SHADOW_OUTER)
  
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;
  
  virtual bool ApplyOpacity(nsDisplayListBuilder* aBuilder,
                            float aOpacity,
                            const DisplayItemClip* aClip) MOZ_OVERRIDE
  {
    mOpacity = aOpacity;
    if (aClip) {
      IntersectClip(aBuilder, *aClip);
    }
    return true;
  }

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayBoxShadowOuterGeometry(this, aBuilder, mOpacity);
  }

  nsRect GetBoundsInternal();

private:
  nsRegion mVisibleRegion;
  nsRect mBounds;
  float mOpacity;
};




class nsDisplayBoxShadowInner : public nsDisplayItem {
public:
  nsDisplayBoxShadowInner(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBoxShadowInner);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBoxShadowInner() {
    MOZ_COUNT_DTOR(nsDisplayBoxShadowInner);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("BoxShadowInner", TYPE_BOX_SHADOW_INNER)
  
  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplayBoxShadowInnerGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    const nsDisplayBoxShadowInnerGeometry* geometry = static_cast<const nsDisplayBoxShadowInnerGeometry*>(aGeometry);
    if (!geometry->mPaddingRect.IsEqualInterior(GetPaddingRect())) {
      
      
      bool snap;
      aInvalidRegion->Or(geometry->mBounds, GetBounds(aBuilder, &snap));
    }
  }

private:
  nsRegion mVisibleRegion;
};




class nsDisplayOutline : public nsDisplayItem {
public:
  nsDisplayOutline(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayOutline);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOutline() {
    MOZ_COUNT_DTOR(nsDisplayOutline);
  }
#endif

  virtual bool IsInvisibleInRect(const nsRect& aRect) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Outline", TYPE_OUTLINE)
};




class nsDisplayEventReceiver : public nsDisplayItem {
public:
  nsDisplayEventReceiver(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayEventReceiver);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayEventReceiver() {
    MOZ_COUNT_DTOR(nsDisplayEventReceiver);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("EventReceiver", TYPE_EVENT_RECEIVER)
};




















class nsDisplayLayerEventRegions MOZ_FINAL : public nsDisplayItem {
public:
  nsDisplayLayerEventRegions(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame)
  {
    MOZ_COUNT_CTOR(nsDisplayEventReceiver);
    AddFrame(aBuilder, aFrame);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayLayerEventRegions() {
    MOZ_COUNT_DTOR(nsDisplayEventReceiver);
  }
#endif
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = false;
    return mHitRegion.GetBounds().Union(mMaybeHitRegion.GetBounds());
  }

  NS_DISPLAY_DECL_NAME("LayerEventRegions", TYPE_LAYER_EVENT_REGIONS)

  
  
  void AddFrame(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame);

  const nsRegion& HitRegion() { return mHitRegion; }
  const nsRegion& MaybeHitRegion() { return mMaybeHitRegion; }
  const nsRegion& DispatchToContentHitRegion() { return mDispatchToContentHitRegion; }

private:
  
  
  nsRegion mHitRegion;
  
  
  nsRegion mMaybeHitRegion;
  
  
  nsRegion mDispatchToContentHitRegion;
};















class nsDisplayWrapList : public nsDisplayItem {
public:
  


  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayItem* aItem);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame), mOverrideZIndex(0), mHasZIndexOverride(false)
  {
    MOZ_COUNT_CTOR(nsDisplayWrapList);
  }
  virtual ~nsDisplayWrapList();
  


  virtual void UpdateBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    mBounds = mList.GetBounds(aBuilder);
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE {
    return false;
  }
  virtual void GetMergedFrames(nsTArray<nsIFrame*>* aFrames) MOZ_OVERRIDE
  {
    aFrames->AppendElements(mMergedFrames);
  }
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) {
    return true;
  }
  virtual bool IsInvalid(nsRect& aRect) MOZ_OVERRIDE
  {
    if (mFrame->IsInvalid(aRect) && aRect.IsEmpty()) {
      return true;
    }
    nsRect temp;
    for (uint32_t i = 0; i < mMergedFrames.Length(); i++) {
      if (mMergedFrames[i]->IsInvalid(temp) && temp.IsEmpty()) {
        aRect.SetEmpty();
        return true;
      }
      aRect = aRect.Union(temp);
    }
    aRect += ToReferenceFrame();
    return !aRect.IsEmpty();
  }
  NS_DISPLAY_DECL_NAME("WrapList", TYPE_WRAP_LIST)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
                                    
  virtual nsDisplayList* GetSameCoordinateSystemChildren() MOZ_OVERRIDE
  {
    NS_ASSERTION(mList.IsEmpty() || !ReferenceFrame() ||
                 !mList.GetBottom()->ReferenceFrame() ||
                 mList.GetBottom()->ReferenceFrame() == ReferenceFrame(),
                 "Children must have same reference frame");
    return &mList;
  }
  virtual nsDisplayList* GetChildren() MOZ_OVERRIDE { return &mList; }

  virtual int32_t ZIndex() const MOZ_OVERRIDE
  {
    return (mHasZIndexOverride) ? mOverrideZIndex : nsDisplayItem::ZIndex();
  }

  void SetOverrideZIndex(int32_t aZIndex)
  {
    mHasZIndexOverride = true;
    mOverrideZIndex = aZIndex;
  }

  void SetVisibleRect(const nsRect& aRect);

  





  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem) {
    NS_NOTREACHED("We never returned nullptr for GetUnderlyingFrame!");
    return nullptr;
  }

protected:
  nsDisplayWrapList() {}

  void MergeFromTrackingMergedFrames(nsDisplayWrapList* aOther)
  {
    mList.AppendToBottom(&aOther->mList);
    mBounds.UnionRect(mBounds, aOther->mBounds);
    mVisibleRect.UnionRect(mVisibleRect, aOther->mVisibleRect);
    mMergedFrames.AppendElement(aOther->mFrame);
    mMergedFrames.MoveElementsFrom(aOther->mMergedFrames);
  }

  nsDisplayList mList;
  
  
  nsTArray<nsIFrame*> mMergedFrames;
  nsRect mBounds;
  int32_t mOverrideZIndex;
  bool mHasZIndexOverride;
};








class nsDisplayWrapper {
public:
  
  

  virtual bool WrapBorderBackground() { return true; }
  virtual nsDisplayItem* WrapList(nsDisplayListBuilder* aBuilder,
                                  nsIFrame* aFrame, nsDisplayList* aList) = 0;
  virtual nsDisplayItem* WrapItem(nsDisplayListBuilder* aBuilder,
                                  nsDisplayItem* aItem) = 0;

  nsresult WrapLists(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                     const nsDisplayListSet& aIn, const nsDisplayListSet& aOut);
  nsresult WrapListsInPlace(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                            const nsDisplayListSet& aLists);
protected:
  nsDisplayWrapper() {}
};





class nsDisplayOpacity : public nsDisplayWrapList {
public:
  nsDisplayOpacity(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                   nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOpacity();
#endif

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    
  }
  virtual bool ApplyOpacity(nsDisplayListBuilder* aBuilder,
                            float aOpacity,
                            const DisplayItemClip* aClip) MOZ_OVERRIDE;
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
  bool NeedsActiveLayer(nsDisplayListBuilder* aBuilder);
  NS_DISPLAY_DECL_NAME("Opacity", TYPE_OPACITY)
#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif

  bool CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

private:
  float mOpacity;
};

class nsDisplayMixBlendMode : public nsDisplayWrapList {
public:
  nsDisplayMixBlendMode(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                        nsDisplayList* aList, uint32_t aFlags = 0);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayMixBlendMode();
#endif

  nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE;

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    
  }
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    return false;
  }
  NS_DISPLAY_DECL_NAME("MixBlendMode", TYPE_MIX_BLEND_MODE)
};

class nsDisplayBlendContainer : public nsDisplayWrapList {
public:
    nsDisplayBlendContainer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                            nsDisplayList* aList,
                            BlendModeSet& aContainedBlendModes);
    nsDisplayBlendContainer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                            nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
    virtual ~nsDisplayBlendContainer();
#endif
    
    virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                               LayerManager* aManager,
                                               const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
    virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                     LayerManager* aManager,
                                     const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
    {
      if (mCanBeActive && aManager->SupportsMixBlendModes(mContainedBlendModes)) {
        return mozilla::LAYER_ACTIVE;
      }
      return mozilla::LAYER_INACTIVE;
    }
    virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
    virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
      return false;
    }
    NS_DISPLAY_DECL_NAME("BlendContainer", TYPE_BLEND_CONTAINER)

private:
    
    BlendModeSet mContainedBlendModes;
    
    
    bool mCanBeActive;
};





class nsDisplayOwnLayer : public nsDisplayWrapList {
public:

  


  enum {
    GENERATE_SUBDOC_INVALIDATIONS = 0x01,
    VERTICAL_SCROLLBAR = 0x02,
    HORIZONTAL_SCROLLBAR = 0x04,
    GENERATE_SCROLLABLE_LAYER = 0x08
  };

  










  nsDisplayOwnLayer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList, uint32_t aFlags = 0,
                    ViewID aScrollTarget = mozilla::layers::FrameMetrics::NULL_SCROLL_ID);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOwnLayer();
#endif
  
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE_FORCE;
  }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE
  {
    
    return false;
  }
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    return false;
  }
  uint32_t GetFlags() { return mFlags; }
  NS_DISPLAY_DECL_NAME("OwnLayer", TYPE_OWN_LAYER)
protected:
  uint32_t mFlags;
  ViewID mScrollTarget;
};






class nsDisplaySubDocument : public nsDisplayOwnLayer {
public:
  nsDisplaySubDocument(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                       nsDisplayList* aList, uint32_t aFlags);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySubDocument();
#endif

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;

  virtual bool ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("SubDocument", TYPE_SUBDOCUMENT)

  mozilla::UniquePtr<FrameMetrics> ComputeFrameMetrics(Layer* aLayer,
                                                       const ContainerLayerParameters& aContainerParameters);

protected:
  ViewID mScrollParentId;
};






class nsDisplayResolution : public nsDisplaySubDocument {
public:
  nsDisplayResolution(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                      nsDisplayList* aList, uint32_t aFlags);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayResolution();
#endif

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Resolution", TYPE_RESOLUTION)
};






class nsDisplayStickyPosition : public nsDisplayOwnLayer {
public:
  nsDisplayStickyPosition(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                          nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayStickyPosition();
#endif

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("StickyPosition", TYPE_STICKY_POSITION)
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE;
  }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
};






















class nsDisplayScrollLayer : public nsDisplayWrapList
{
public:
  






  nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder, nsDisplayList* aList,
                       nsIFrame* aForFrame, nsIFrame* aScrolledFrame,
                       nsIFrame* aScrollFrame);
  nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem,
                       nsIFrame* aForFrame, nsIFrame* aScrolledFrame,
                       nsIFrame* aScrollFrame);
  nsDisplayScrollLayer(nsDisplayListBuilder* aBuilder,
                       nsIFrame* aForFrame, nsIFrame* aScrolledFrame,
                       nsIFrame* aScrollFrame);
  NS_DISPLAY_DECL_NAME("ScrollLayer", TYPE_SCROLL_LAYER)

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayScrollLayer();
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  virtual bool ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    return nsRegion();
  }

  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;

  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem) MOZ_OVERRIDE;

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  
  
  
  intptr_t GetScrollLayerCount();

  virtual nsIFrame* GetScrollFrame() { return mScrollFrame; }
  virtual nsIFrame* GetScrolledFrame() { return mScrolledFrame; }

#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif

  bool IsDisplayPortOpaque() { return mDisplayPortContentsOpaque; }

  static FrameMetrics ComputeFrameMetrics(nsIFrame* aForFrame,
                                          nsIFrame* aScrollFrame,
                                          const nsIFrame* aReferenceFrame,
                                          Layer* aLayer,
                                          ViewID aScrollParentId,
                                          const nsRect& aViewport,
                                          bool aForceNullScrollId,
                                          bool aIsRoot,
                                          const ContainerLayerParameters& aContainerParameters);

  mozilla::UniquePtr<FrameMetrics> ComputeFrameMetrics(Layer* aLayer,
                                                       const ContainerLayerParameters& aContainerParameters);

protected:
  nsRect GetScrolledContentRectToDraw(nsDisplayListBuilder* aBuilder,
                                      nsRect* aDisplayPort);

  nsIFrame* mScrollFrame;
  nsIFrame* mScrolledFrame;
  ViewID mScrollParentId;
  bool mDisplayPortContentsOpaque;
};












class nsDisplayScrollInfoLayer : public nsDisplayScrollLayer
{
public:
  nsDisplayScrollInfoLayer(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aScrolledFrame, nsIFrame* aScrollFrame);
  NS_DISPLAY_DECL_NAME("ScrollInfoLayer", TYPE_SCROLL_INFO_LAYER)

  virtual ~nsDisplayScrollInfoLayer();

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
  virtual bool ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  { return true; }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem) MOZ_OVERRIDE;

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
};





class nsDisplayZoom : public nsDisplaySubDocument {
public:
  









  nsDisplayZoom(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                nsDisplayList* aList,
                int32_t aAPD, int32_t aParentAPD,
                uint32_t aFlags = 0);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayZoom();
#endif
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE;
  }
  NS_DISPLAY_DECL_NAME("Zoom", TYPE_ZOOM)

  
  int32_t GetChildAppUnitsPerDevPixel() { return mAPD; }
  
  int32_t GetParentAppUnitsPerDevPixel() { return mParentAPD; }

private:
  int32_t mAPD, mParentAPD;
};





class nsDisplaySVGEffects : public nsDisplayWrapList {
public:
  nsDisplaySVGEffects(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                      nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGEffects();
#endif
  
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder,
                           bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    return mEffectsBounds + ToReferenceFrame();
  }
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                        nsDisplayItem* aItem) MOZ_OVERRIDE;
  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE {
    return false;
  }
  NS_DISPLAY_DECL_NAME("SVGEffects", TYPE_SVG_EFFECTS)

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
 
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;

  gfxRect BBoxInUserSpace() const;
  gfxPoint UserSpaceOffset() const;

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    return new nsDisplaySVGEffectsGeometry(this, aBuilder);
  }
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE;

  void PaintAsLayer(nsDisplayListBuilder* aBuilder,
                    nsRenderingContext* aCtx,
                    LayerManager* aManager);

#ifdef MOZ_DUMP_PAINTING
  void PrintEffects(nsACString& aTo);
#endif

private:
  
  nsRect mEffectsBounds;
};













 
class nsDisplayTransform: public nsDisplayItem
{
  typedef mozilla::gfx::Matrix4x4 Matrix4x4;
  typedef mozilla::gfx::Point3D Point3D;
public:
  






  typedef Matrix4x4 (* ComputeTransformFunction)(nsIFrame* aFrame, float aAppUnitsPerPixel);

  


  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayList *aList, const nsRect& aChildrenVisibleRect,
                     uint32_t aIndex = 0);
  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayItem *aItem, const nsRect& aChildrenVisibleRect,
                     uint32_t aIndex = 0);
  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayList *aList, const nsRect& aChildrenVisibleRect,
                     ComputeTransformFunction aTransformGetter, uint32_t aIndex = 0);

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTransform()
  {
    MOZ_COUNT_DTOR(nsDisplayTransform);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplayTransform", TYPE_TRANSFORM)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE
  {
    if (mStoredList.GetComponentAlphaBounds(aBuilder).IsEmpty())
      return nsRect();
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  virtual nsDisplayList* GetChildren() MOZ_OVERRIDE { return mStoredList.GetChildren(); }

  virtual void HitTest(nsDisplayListBuilder *aBuilder, const nsRect& aRect,
                       HitTestState *aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder *aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder *aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder *aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerLayerParameters& aParameters) MOZ_OVERRIDE;
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerLayerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual bool ShouldBuildLayerEvenIfInvisible(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                 nsRegion *aVisibleRegion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder *aBuilder, nsDisplayItem *aItem) MOZ_OVERRIDE;
  
  virtual uint32_t GetPerFrameKey() MOZ_OVERRIDE { return (mIndex << nsDisplayItem::TYPE_BITS) | nsDisplayItem::GetPerFrameKey(); }
  
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    
  }

  virtual const nsIFrame* ReferenceFrameForChildren() const MOZ_OVERRIDE {
    
    
    
    if (!mTransformGetter) {
      return mFrame;
    }
    return nsDisplayItem::ReferenceFrameForChildren(); 
  }

  virtual const nsRect& GetVisibleRectForChildren() const MOZ_OVERRIDE
  {
    return mChildrenVisibleRect;
  }

  enum {
    INDEX_MAX = UINT32_MAX >> nsDisplayItem::TYPE_BITS
  };

  const Matrix4x4& GetTransform();

  float GetHitDepthAtPoint(nsDisplayListBuilder* aBuilder, const nsPoint& aPoint);

  

















  static nsRect TransformRect(const nsRect &aUntransformedBounds, 
                              const nsIFrame* aFrame,
                              const nsPoint &aOrigin,
                              const nsRect* aBoundsOverride = nullptr);

  static nsRect TransformRectOut(const nsRect &aUntransformedBounds, 
                                 const nsIFrame* aFrame,
                                 const nsPoint &aOrigin,
                                 const nsRect* aBoundsOverride = nullptr);

  


  static bool UntransformRect(const nsRect &aTransformedBounds,
                              const nsRect &aChildBounds,
                              const nsIFrame* aFrame,
                              const nsPoint &aOrigin,
                              nsRect *aOutRect);

  bool UntransformVisibleRect(nsDisplayListBuilder* aBuilder,
                              nsRect* aOutRect);

  static Point3D GetDeltaToTransformOrigin(const nsIFrame* aFrame,
                                           float aAppUnitsPerPixel,
                                           const nsRect* aBoundsOverride);

  static Point3D GetDeltaToPerspectiveOrigin(const nsIFrame* aFrame,
                                             float aAppUnitsPerPixel);

  













  static nsRect GetFrameBoundsForTransform(const nsIFrame* aFrame);

  struct FrameTransformProperties
  {
    FrameTransformProperties(const nsIFrame* aFrame,
                             float aAppUnitsPerPixel,
                             const nsRect* aBoundsOverride);
    FrameTransformProperties(nsCSSValueSharedList* aTransformList,
                             const Point3D& aToTransformOrigin,
                             const Point3D& aToPerspectiveOrigin,
                             nscoord aChildPerspective)
      : mFrame(nullptr)
      , mTransformList(aTransformList)
      , mToTransformOrigin(aToTransformOrigin)
      , mToPerspectiveOrigin(aToPerspectiveOrigin)
      , mChildPerspective(aChildPerspective)
    {}

    const nsIFrame* mFrame;
    nsRefPtr<nsCSSValueSharedList> mTransformList;
    const Point3D mToTransformOrigin;
    const Point3D mToPerspectiveOrigin;
    nscoord mChildPerspective;
  };

  














  static gfx3DMatrix GetResultingTransformMatrix(const nsIFrame* aFrame,
                                                 const nsPoint& aOrigin,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride = nullptr,
                                                 nsIFrame** aOutAncestor = nullptr,
                                                 bool aOffsetByOrigin = false);
  static gfx3DMatrix GetResultingTransformMatrix(const FrameTransformProperties& aProperties,
                                                 const nsPoint& aOrigin,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride = nullptr,
                                                 nsIFrame** aOutAncestor = nullptr);
  



  static bool ShouldPrerenderTransformedContent(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame,
                                                bool aLogAnimations = false);
  bool CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  



  bool MaybePrerender() const { return mMaybePrerender; }
  



  bool ShouldPrerender(nsDisplayListBuilder* aBuilder);

#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(nsACString& aTo) MOZ_OVERRIDE;
#endif

private:
  void SetReferenceFrameToAncestor(nsDisplayListBuilder* aBuilder);
  void Init(nsDisplayListBuilder* aBuilder);

  static gfx3DMatrix GetResultingTransformMatrixInternal(const FrameTransformProperties& aProperties,
                                                         const nsPoint& aOrigin,
                                                         float aAppUnitsPerPixel,
                                                         const nsRect* aBoundsOverride,
                                                         nsIFrame** aOutAncestor,
                                                         bool aOffsetByOrigin);

  nsDisplayWrapList mStoredList;
  Matrix4x4 mTransform;
  ComputeTransformFunction mTransformGetter;
  nsRect mChildrenVisibleRect;
  uint32_t mIndex;
  
  
  bool mMaybePrerender;
};












class nsCharClipDisplayItem : public nsDisplayItem {
public:
  nsCharClipDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame), mLeftEdge(0), mRightEdge(0) {}

  explicit nsCharClipDisplayItem(nsIFrame* aFrame)
    : nsDisplayItem(aFrame) {}

  struct ClipEdges {
    ClipEdges(const nsDisplayItem& aItem,
              nscoord aLeftEdge, nscoord aRightEdge) {
      nsRect r = aItem.Frame()->GetScrollableOverflowRect() +
                 aItem.ToReferenceFrame();
      mX = aLeftEdge > 0 ? r.x + aLeftEdge : nscoord_MIN;
      mXMost = aRightEdge > 0 ? std::max(r.XMost() - aRightEdge, mX) : nscoord_MAX;
    }
    void Intersect(nscoord* aX, nscoord* aWidth) const {
      nscoord xmost1 = *aX + *aWidth;
      *aX = std::max(*aX, mX);
      *aWidth = std::max(std::min(xmost1, mXMost) - *aX, 0);
    }
    nscoord mX;
    nscoord mXMost;
  };

  ClipEdges Edges() const { return ClipEdges(*this, mLeftEdge, mRightEdge); }

  static nsCharClipDisplayItem* CheckCast(nsDisplayItem* aItem) {
    nsDisplayItem::Type t = aItem->GetType();
    return (t == nsDisplayItem::TYPE_TEXT ||
            t == nsDisplayItem::TYPE_TEXT_DECORATION ||
            t == nsDisplayItem::TYPE_TEXT_SHADOW)
      ? static_cast<nsCharClipDisplayItem*>(aItem) : nullptr;
  }

  nscoord mLeftEdge;  
  nscoord mRightEdge; 
};

#endif
