











#ifndef NSDISPLAYLIST_H_
#define NSDISPLAYLIST_H_

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsISelection.h"
#include "nsCaret.h"
#include "plarena.h"
#include "nsRegion.h"
#include "FrameLayerBuilder.h"
#include "nsThemeConstants.h"
#include "nsLayoutUtils.h"
#include "nsDisplayListInvalidation.h"

#include "mozilla/StandardInteger.h"

#include <stdlib.h>
#include <algorithm>

class nsIPresShell;
class nsIContent;
class nsRenderingContext;
class nsDeviceContext;
class nsDisplayTableItem;
class nsDisplayItem;

namespace mozilla {
namespace layers {
class ImageLayer;
class ImageContainer;
} 
} 












































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
  typedef nsIWidget::ThemeGeometry ThemeGeometry;

  







  enum Mode {
    PAINTING,
    EVENT_DELIVERY,
    PLUGIN_GEOMETRY,
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
  bool WillComputePluginGeometry() { return mWillComputePluginGeometry; }
  



  bool IsBackgroundOnly() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "don't call this if we're not in a presshell");
    return CurrentPresShellState()->mIsBackgroundOnly;
  }
  






  bool IsAtRootOfPseudoStackingContext() { return mIsAtRootOfPseudoStackingContext; }

  



  nsISelection* GetBoundingSelection() { return mBoundingSelection; }

  



  const nsIFrame* FindReferenceFrameFor(const nsIFrame *aFrame)
  {
    if (aFrame == mCachedOffsetFrame) {
      return mCachedReferenceFrame;
    }
    for (const nsIFrame* f = aFrame; f; f = nsLayoutUtils::GetCrossDocParentFrame(f))
    {
      if (f == mReferenceFrame || f->IsTransformed()) {
        mCachedOffsetFrame = aFrame;
        mCachedReferenceFrame = f;
        mCachedOffset = aFrame->GetOffsetToCrossDoc(f);
        return f;
      }
    }
    mCachedOffsetFrame = aFrame;
    mCachedReferenceFrame = mReferenceFrame;
    mCachedOffset = aFrame->GetOffsetToCrossDoc(mReferenceFrame);
    return mReferenceFrame;
  }
  
  



  nsIFrame* RootReferenceFrame() 
  {
    return mReferenceFrame;
  }

  






  const nsPoint& ToReferenceFrame(const nsIFrame* aFrame) {
    if (aFrame != mCachedOffsetFrame) {
      FindReferenceFrameFor(aFrame);
    }
    return mCachedOffset;
  }
  





  void SetIgnoreScrollFrame(nsIFrame* aFrame) { mIgnoreScrollFrame = aFrame; }
  


  nsIFrame* GetIgnoreScrollFrame() { return mIgnoreScrollFrame; }
  




  void SetIncludeAllOutOfFlows() { mIncludeAllOutOfFlows = true; }
  bool GetIncludeAllOutOfFlows() const { return mIncludeAllOutOfFlows; }
  



  void SetSelectedFramesOnly() { mSelectedFramesOnly = true; }
  bool GetSelectedFramesOnly() { return mSelectedFramesOnly; }
  



  void SetAccurateVisibleRegions() { mAccurateVisibleRegions = true; }
  bool GetAccurateVisibleRegions() { return mAccurateVisibleRegions; }
  



  void IgnorePaintSuppression() { mIgnoreSuppression = true; }
  


  bool IsIgnoringPaintSuppression() { return mIgnoreSuppression; }
  



  bool GetHadToIgnorePaintSuppression() { return mHadToIgnoreSuppression; }
  


  void SetPaintingToWindow(bool aToWindow) { mIsPaintingToWindow = aToWindow; }
  bool IsPaintingToWindow() const { return mIsPaintingToWindow; }

  



  bool AllowMergingAndFlattening() { return mAllowMergingAndFlattening; }
  void SetAllowMergingAndFlattening(bool aAllow) { mAllowMergingAndFlattening = aAllow; }

  



  bool IsInFixedPosition() const { return mIsInFixedPosition; }

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
  


  nsCaret* GetCaret();
  




  void EnterPresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);
  


  void LeavePresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);

  



  bool IsInTransform() const { return mInTransform; }
  



  void SetInTransform(bool aInTransform) { mInTransform = aInTransform; }

  


  void SetDisplayPort(const nsRect& aDisplayPort);
  const nsRect* GetDisplayPort() { return mHasDisplayPort ? &mDisplayPort : nullptr; }

  




  void SetHasFixedItems() { mHasFixedItems = true; }
  bool GetHasFixedItems() { return mHasFixedItems; }

  







  bool IsFixedItem(nsDisplayItem* aItem,
                   const nsIFrame** aActiveScrolledRoot = nullptr,
                   const nsIFrame* aOverrideActiveScrolledRoot = nullptr);

  


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

  


  const nsRegion* GetFinalTransparentRegion() { return mFinalTransparentRegion; }
  



  void SetFinalTransparentRegion(const nsRegion& aFinalTransparentRegion)
  {
    mFinalTransparentRegion = &aFinalTransparentRegion;
  }

  const nsTArray<ThemeGeometry>& GetThemeGeometries() { return mThemeGeometries; }

  




  bool ShouldDescendIntoFrame(nsIFrame* aFrame) const {
    return
      (aFrame->GetStateBits() & NS_FRAME_FORCE_DISPLAY_LIST_DESCEND_INTO) ||
      GetIncludeAllOutOfFlows();
  }

  













  void RegisterThemeGeometry(uint8_t aWidgetType,
                             const nsIntRect& aRect) {
    if (mIsPaintingToWindow && mPresShellStates.Length() == 1) {
      ThemeGeometry geometry(aWidgetType, aRect);
      mThemeGeometries.AppendElement(geometry);
    }
  }

  




  void* Allocate(size_t aSize);
  
  




  class AutoBuildingDisplayList;
  friend class AutoBuildingDisplayList;
  class AutoBuildingDisplayList {
  public:
    AutoBuildingDisplayList(nsDisplayListBuilder* aBuilder, bool aIsRoot)
      : mBuilder(aBuilder),
        mPrevCachedOffsetFrame(aBuilder->mCachedOffsetFrame),
        mPrevCachedReferenceFrame(aBuilder->mCachedReferenceFrame),
        mPrevCachedOffset(aBuilder->mCachedOffset),
        mPrevIsAtRootOfPseudoStackingContext(aBuilder->mIsAtRootOfPseudoStackingContext) {
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
    }
    AutoBuildingDisplayList(nsDisplayListBuilder* aBuilder,
                            nsIFrame* aForChild, bool aIsRoot,
                            bool aIsInFixedPosition)
      : mBuilder(aBuilder),
        mPrevCachedOffsetFrame(aBuilder->mCachedOffsetFrame),
        mPrevCachedReferenceFrame(aBuilder->mCachedReferenceFrame),
        mPrevCachedOffset(aBuilder->mCachedOffset),
        mPrevIsAtRootOfPseudoStackingContext(aBuilder->mIsAtRootOfPseudoStackingContext),
        mPrevIsInFixedPosition(aBuilder->mIsInFixedPosition) {
      if (aForChild->IsTransformed()) {
        aBuilder->mCachedOffset = nsPoint();
        aBuilder->mCachedReferenceFrame = aForChild;
      } else if (mPrevCachedOffsetFrame == aForChild->GetParent()) {
        aBuilder->mCachedOffset += aForChild->GetPosition();
      } else {
        aBuilder->mCachedOffset = aBuilder->ToReferenceFrame(aForChild);
      }
      aBuilder->mCachedOffsetFrame = aForChild;
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
      if (aIsInFixedPosition) {
        aBuilder->mIsInFixedPosition = aIsInFixedPosition;
      }
    }
    ~AutoBuildingDisplayList() {
      mBuilder->mCachedOffsetFrame = mPrevCachedOffsetFrame;
      mBuilder->mCachedReferenceFrame = mPrevCachedReferenceFrame;
      mBuilder->mCachedOffset = mPrevCachedOffset;
      mBuilder->mIsAtRootOfPseudoStackingContext = mPrevIsAtRootOfPseudoStackingContext;
      mBuilder->mIsInFixedPosition = mPrevIsInFixedPosition;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    const nsIFrame*       mPrevCachedOffsetFrame;
    const nsIFrame*       mPrevCachedReferenceFrame;
    nsPoint               mPrevCachedOffset;
    bool                  mPrevIsAtRootOfPseudoStackingContext;
    bool                  mPrevIsInFixedPosition;
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

  
  nsDisplayTableItem* GetCurrentTableItem() { return mCurrentTableItem; }
  void SetCurrentTableItem(nsDisplayTableItem* aTableItem) { mCurrentTableItem = aTableItem; }

  NS_DECLARE_FRAME_PROPERTY(OutOfFlowDirtyRectProperty, nsIFrame::DestroyRect)
  NS_DECLARE_FRAME_PROPERTY(Preserve3DDirtyRectProperty, nsIFrame::DestroyRect)

  nsPresContext* CurrentPresContext() {
    return CurrentPresShellState()->mPresShell->GetPresContext();
  }

  



  
  void AddExcludedGlassRegion(nsRect &bounds) {
    mExcludedGlassRegion.Or(mExcludedGlassRegion, bounds);
  }
  const nsRegion& GetExcludedGlassRegion() {
    return mExcludedGlassRegion;
  }
  void SetGlassDisplayItem(nsDisplayItem* aItem) {
    if (mGlassDisplayItem) {
      
      
      
      
      NS_WARNING("Multiple glass backgrounds found?");
    } else {
      mGlassDisplayItem = aItem;
    }
  }
  bool NeedToForceTransparentSurfaceForItem(nsDisplayItem* aItem) {
    return aItem == mGlassDisplayItem;
  }

  void SetContainsPluginItem() { mContainsPluginItem = true; }
  bool ContainsPluginItem() { return mContainsPluginItem; }

private:
  void MarkOutOfFlowFrameForDisplay(nsIFrame* aDirtyFrame, nsIFrame* aFrame,
                                    const nsRect& aDirtyRect);

  struct PresShellState {
    nsIPresShell* mPresShell;
    nsIFrame*     mCaretFrame;
    uint32_t      mFirstFrameMarkedForDisplay;
    bool          mIsBackgroundOnly;
  };
  PresShellState* CurrentPresShellState() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "Someone forgot to enter a presshell");
    return &mPresShellStates[mPresShellStates.Length() - 1];
  }

  nsIFrame*                      mReferenceFrame;
  nsIFrame*                      mIgnoreScrollFrame;
  PLArenaPool                    mPool;
  nsCOMPtr<nsISelection>         mBoundingSelection;
  nsAutoTArray<PresShellState,8> mPresShellStates;
  nsAutoTArray<nsIFrame*,100>    mFramesMarkedForDisplay;
  nsAutoTArray<ThemeGeometry,2>  mThemeGeometries;
  nsDisplayTableItem*            mCurrentTableItem;
  const nsRegion*                mFinalTransparentRegion;
  
  
  const nsIFrame*                mCachedOffsetFrame;
  const nsIFrame*                mCachedReferenceFrame;
  nsPoint                        mCachedOffset;
  nsRect                         mDisplayPort;
  nsRegion                       mExcludedGlassRegion;
  
  nsDisplayItem*                 mGlassDisplayItem;
  Mode                           mMode;
  bool                           mBuildCaret;
  bool                           mIgnoreSuppression;
  bool                           mHadToIgnoreSuppression;
  bool                           mIsAtRootOfPseudoStackingContext;
  bool                           mIncludeAllOutOfFlows;
  bool                           mSelectedFramesOnly;
  bool                           mAccurateVisibleRegions;
  bool                           mAllowMergingAndFlattening;
  bool                           mWillComputePluginGeometry;
  
  
  bool                           mInTransform;
  bool                           mSyncDecodeImages;
  bool                           mIsPaintingToWindow;
  bool                           mHasDisplayPort;
  bool                           mHasFixedItems;
  bool                           mIsInFixedPosition;
  bool                           mIsCompositingCheap;
  bool                           mContainsPluginItem;
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
  typedef mozilla::FrameLayerBuilder::ContainerParameters ContainerParameters;
  typedef mozilla::layers::FrameMetrics::ViewID ViewID;
  typedef mozilla::layers::Layer Layer;
  typedef mozilla::layers::LayerManager LayerManager;
  typedef mozilla::LayerState LayerState;

  
  
  nsDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : mFrame(aFrame)
#ifdef MOZ_DUMP_PAINTING
    , mPainted(false)
#endif
  {
    if (aFrame) {
      mReferenceFrame = aBuilder->FindReferenceFrameFor(aFrame);
      mToReferenceFrame = aBuilder->ToReferenceFrame(aFrame);
    } else {
      mReferenceFrame = nullptr;
    }
  }
  nsDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                const nsIFrame* aReferenceFrame,
                const nsPoint& aToReferenceFrame)
    : mFrame(aFrame)
    , mReferenceFrame(aReferenceFrame)
    , mToReferenceFrame(aToReferenceFrame)
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
    typedef nsTArray<ViewID> ShadowArray;

    HitTestState(ShadowArray* aShadows = NULL)
      : mShadows(aShadows) {
    }

    ~HitTestState() {
      NS_ASSERTION(mItemBuffer.Length() == 0,
                   "mItemBuffer should have been cleared");
    }

    nsAutoTArray<nsDisplayItem*, 100> mItemBuffer;

    
    
    
    ShadowArray* mShadows;
  };

  




  virtual Type GetType() = 0;
  








  virtual uint32_t GetPerFrameKey() { return uint32_t(GetType()); }
  












  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {}
  





  inline nsIFrame* GetUnderlyingFrame() const { return mFrame; }
  









  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
  {
    *aSnap = false;
    return nsRect(ToReferenceFrame(), GetUnderlyingFrame()->GetSize());
  }
  nsRect GetBorderRect() {
    return nsRect(ToReferenceFrame(), GetUnderlyingFrame()->GetSize());
  }
  nsRect GetPaddingRect() {
    return GetUnderlyingFrame()->GetPaddingRectRelativeToSelf() + ToReferenceFrame();
  }
  nsRect GetContentRect() {
    return GetUnderlyingFrame()->GetContentRectRelativeToSelf() + ToReferenceFrame();
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

  







  virtual void NotifyRenderingChanged() {}

  








  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap)
  {
    *aSnap = false;
    return nsRegion();
  }
  




  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) { return false; }
  






  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame)
  { return false; }
  




  virtual bool ShouldFixToViewport(nsDisplayListBuilder* aBuilder)
  { return false; }

  



  static bool ForceActiveLayers();

  






















  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters)
  { return mozilla::LAYER_NONE; }
  




  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) {}

#ifdef MOZ_DUMP_PAINTING
  


  bool Painted() { return mPainted; }

  


  void SetPainted() { mPainted = true; }
#endif

  













  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  { return nullptr; }

  



















  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion)
  { return !mVisibleRect.IsEmpty(); }

  








  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
    return false;
  }

  




  virtual void GetMergedFrames(nsTArray<nsIFrame*>* aFrames) {}

  





  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) {
    return false;
  }

  




  virtual nsDisplayList* GetSameCoordinateSystemChildren() { return nullptr; }

  



  virtual nsDisplayList* GetChildren() { return nullptr; }

  



  const nsRect& GetVisibleRect() { return mVisibleRect; }
  
#ifdef MOZ_DUMP_PAINTING
  


  virtual const char* Name() = 0;

  virtual void WriteDebugInfo(FILE *aOutput) {}
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

protected:
  friend class nsDisplayList;

  nsDisplayItem() {
    mAbove = nullptr;
  }

  nsIFrame* mFrame;
  
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
  typedef mozilla::layers::ThebesLayer ThebesLayer;

  


  nsDisplayList() :
    mIsOpaque(false)
  {
    mTop = &mSentinel;
    mSentinel.mAbove = nullptr;
#ifdef DEBUG
    mDidComputeVisibility = false;
#endif
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
                                   nsDisplayItem* aForItem,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aListVisibleBounds,
                                   const nsRect& aAllowVisibleRegionExpansion);

  






  bool ComputeVisibilityForRoot(nsDisplayListBuilder* aBuilder,
                                nsRegion* aVisibleRegion);

  



  bool IsOpaque() const {
    NS_ASSERTION(mDidComputeVisibility, "Need to have called ComputeVisibility");
    return mIsOpaque;
  }

  



  bool NeedsTransparentSurface() const {
    NS_ASSERTION(mDidComputeVisibility, "Need to have called ComputeVisibility");
    return mForceTransparentSurface;
  }
  
























  enum {
    PAINT_DEFAULT = 0,
    PAINT_USE_WIDGET_LAYERS = 0x01,
    PAINT_FLUSH_LAYERS = 0x02,
    PAINT_EXISTING_TRANSACTION = 0x04,
    PAINT_NO_COMPOSITE = 0x08,
    PAINT_NO_CLEAR_INVALIDATIONS = 0x10
  };
  void PaintRoot(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                 uint32_t aFlags) const;
  



  void PaintForFrame(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     nsIFrame* aForFrame, uint32_t aFlags) const;
  


  nsRect GetBounds(nsDisplayListBuilder* aBuilder) const;
  



  void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
               nsDisplayItem::HitTestState* aState,
               nsTArray<nsIFrame*> *aOutFrames) const;

#ifdef DEBUG
  bool DidComputeVisibility() const { return mDidComputeVisibility; }
#endif

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;
  
  
  void FlattenTo(nsTArray<nsDisplayItem*>* aElements);
  
  
  void ExplodeAnonymousChildLists(nsDisplayListBuilder* aBuilder);
  
  nsDisplayItemLink  mSentinel;
  nsDisplayItemLink* mTop;

  
  nsRect mVisibleRect;
  
  
  
  bool mIsOpaque;
  
  
  bool mForceTransparentSurface;
#ifdef DEBUG
  bool mDidComputeVisibility;
#endif
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
  nsDisplayListCollection(nsDisplayList* aBorderBackground) :
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

  virtual bool SupportsOptimizingToImage() { return true; }
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
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE {
    mPaint(mFrame, aCtx, mVisibleRect, ToReferenceFrame());
  }
  NS_DISPLAY_DECL_NAME(mName, mType)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) {
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
  nsDisplayCaret(nsDisplayListBuilder* aBuilder, nsIFrame* aCaretFrame,
                 nsCaret *aCaret)
    : nsDisplayItem(aBuilder, aCaretFrame), mCaret(aCaret) {
    MOZ_COUNT_CTOR(nsDisplayCaret);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayCaret() {
    MOZ_COUNT_DTOR(nsDisplayCaret);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    
    return mCaret->GetCaretRect() + ToReferenceFrame();
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Caret", TYPE_CARET)
protected:
  nsRefPtr<nsCaret> mCaret;
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Border", TYPE_BORDER)
  
  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder);

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion);
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

  NS_DISPLAY_DECL_NAME("SolidColor", TYPE_SOLID_COLOR)

private:
  nsRect  mBounds;
  nscolor mColor;
};





class nsDisplayBackgroundImage : public nsDisplayImageContainer {
public:
  





  nsDisplayBackgroundImage(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           uint32_t aLayer, bool aIsThemed,
                           const nsStyleBackground* aBackgroundStyle);
  virtual ~nsDisplayBackgroundImage();

  
  
  
  static nsresult AppendBackgroundItemsToTop(nsDisplayListBuilder* aBuilder,
                                             nsIFrame* aFrame,
                                             nsDisplayList* aList,
                                             nsDisplayBackgroundImage** aBackground = nullptr);

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  


  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual uint32_t GetPerFrameKey() MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Background", TYPE_BACKGROUND)
  
  bool IsThemed() { return mIsThemed; }

  




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

#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(FILE *aOutput);
#endif
protected:
  typedef class mozilla::layers::ImageContainer ImageContainer;
  typedef class mozilla::layers::ImageLayer ImageLayer;

  bool TryOptimizeToImageLayer(LayerManager* aManager, nsDisplayListBuilder* aBuilder);
  bool IsSingleFixedPositionImage(nsDisplayListBuilder* aBuilder,
                                  const nsRect& aClipRect,
                                  gfxRect* aDestRect);
  nsRect GetBoundsInternal();

  void PaintInternal(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     const nsRect& aBounds, nsRect* aClipRect);

  
  
  const nsStyleBackground* mBackgroundStyle;
  
  nsRefPtr<ImageContainer> mImageContainer;
  gfxRect mDestRect;
  
  nsRect mBounds;
  uint32_t mLayer;

  nsITheme::Transparency mThemeTransparency;
  
  bool mIsThemed;
  
  bool mIsBottommostLayer;
};

class nsDisplayBackgroundColor : public nsDisplayItem
{
public:
  nsDisplayBackgroundColor(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           const nsStyleBackground* aBackgroundStyle,
                           nscolor aColor)
    : nsDisplayItem(aBuilder, aFrame)
    , mBackgroundStyle(aBackgroundStyle)
    , mColor(aColor)
  { }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE
  {
    *aSnap = true;
    return nsRect(ToReferenceFrame(), GetUnderlyingFrame()->GetSize());
  }

  NS_DISPLAY_DECL_NAME("BackgroundColor", TYPE_BACKGROUND_COLOR)
#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(FILE *aOutput) {
    fprintf(aOutput, "(rgba %d,%d,%d,%d)", 
            NS_GET_R(mColor), NS_GET_G(mColor),
            NS_GET_B(mColor), NS_GET_A(mColor));

  }
#endif

protected:
  const nsStyleBackground* mBackgroundStyle;
  nscolor mColor;
};




class nsDisplayBoxShadowOuter : public nsDisplayItem {
public:
  nsDisplayBoxShadowOuter(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
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
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("BoxShadowOuter", TYPE_BOX_SHADOW_OUTER)
  
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion)
  {
    const nsDisplayItemGenericGeometry* geometry = static_cast<const nsDisplayItemGenericGeometry*>(aGeometry);
    bool snap;
    if (!geometry->mBounds.IsEqualInterior(GetBounds(aBuilder, &snap)) ||
        !geometry->mBorderRect.IsEqualInterior(GetBorderRect())) {
      nsRegion oldShadow, newShadow;
      oldShadow = oldShadow.Sub(geometry->mBounds, geometry->mBorderRect);
      newShadow = newShadow.Sub(GetBounds(aBuilder, &snap), GetBorderRect());
      aInvalidRegion->Or(oldShadow, newShadow);
    }
  }

  nsRect GetBoundsInternal();

private:
  nsRegion mVisibleRegion;
  nsRect mBounds;
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
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("BoxShadowInner", TYPE_BOX_SHADOW_INNER)
  
  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder)
  {
    return new nsDisplayBoxShadowInnerGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion)
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
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















class nsDisplayWrapList : public nsDisplayItem {
  
  

public:
  


  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayItem* aItem);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayItem* aItem, const nsIFrame* aReferenceFrame, const nsPoint& aToReferenceFrame);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {}
  virtual ~nsDisplayWrapList();
  


  void UpdateBounds(nsDisplayListBuilder* aBuilder)
  {
    mBounds = mList.GetBounds(aBuilder);
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) MOZ_OVERRIDE;
  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion,
                                 const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE {
    NS_WARNING("This list should already have been flattened!!!");
    return false;
  }
  virtual void GetMergedFrames(nsTArray<nsIFrame*>* aFrames) MOZ_OVERRIDE
  {
    aFrames->AppendElements(mMergedFrames);
  }
  virtual bool IsInvalid(nsRect& aRect)
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

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder);
                                    
  virtual nsDisplayList* GetSameCoordinateSystemChildren() MOZ_OVERRIDE
  {
    NS_ASSERTION(mList.IsEmpty() || !ReferenceFrame() ||
                 !mList.GetBottom()->ReferenceFrame() ||
                 mList.GetBottom()->ReferenceFrame() == ReferenceFrame(),
                 "Children must have same reference frame");
    return &mList;
  }
  virtual nsDisplayList* GetChildren() MOZ_OVERRIDE { return &mList; }

  





  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem) {
    NS_NOTREACHED("We never returned nullptr for GetUnderlyingFrame!");
    return nullptr;
  }

  




  static LayerState RequiredLayerStateForChildren(nsDisplayListBuilder* aBuilder,
                                                  LayerManager* aManager,
                                                  const ContainerParameters& aParameters,
                                                  const nsDisplayList& aList,
                                                  nsIFrame* aActiveScrolledRoot);

protected:
  nsDisplayWrapList() {}

  void MergeFrom(nsDisplayWrapList* aOther)
  {
    mList.AppendToBottom(&aOther->mList);
    mBounds.UnionRect(mBounds, aOther->mBounds);
  }
  void MergeFromTrackingMergedFrames(nsDisplayWrapList* aOther)
  {
    MergeFrom(aOther);
    mMergedFrames.AppendElement(aOther->mFrame);
    mMergedFrames.MoveElementsFrom(aOther->mMergedFrames);
  }

  nsDisplayList mList;
  
  
  nsTArray<nsIFrame*> mMergedFrames;
  nsRect mBounds;
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
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;  
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    
  }
  NS_DISPLAY_DECL_NAME("Opacity", TYPE_OPACITY)
#ifdef MOZ_DUMP_PAINTING
  virtual void WriteDebugInfo(FILE *aOutput) {
    fprintf(aOutput, "(opacity %f)", mFrame->StyleDisplay()->mOpacity);
  }
#endif

  bool CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder);
};





class nsDisplayOwnLayer : public nsDisplayWrapList {
public:

  


  enum {
    GENERATE_SUBDOC_INVALIDATIONS = 0x01
  };

  




  nsDisplayOwnLayer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList, uint32_t aFlags = 0);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOwnLayer();
#endif
  
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE_FORCE;
  }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE
  {
    
    return false;
  }
  NS_DISPLAY_DECL_NAME("OwnLayer", TYPE_OWN_LAYER)
private:
  uint32_t mFlags;
};






class nsDisplayFixedPosition : public nsDisplayOwnLayer {
public:
  nsDisplayFixedPosition(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                         nsIFrame* aFixedPosFrame, nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayFixedPosition();
#endif

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE
  {
    return mozilla::LAYER_ACTIVE;
  }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;

  NS_DISPLAY_DECL_NAME("FixedPosition", TYPE_FIXED_POSITION)

protected:
  nsIFrame* mFixedPosFrame;
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

  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;

  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;

  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem) MOZ_OVERRIDE;

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

  
  
  
  intptr_t GetScrollLayerCount();
  intptr_t RemoveScrollLayerCount();

  virtual nsIFrame* GetScrolledFrame() { return mScrolledFrame; }

private:
  nsIFrame* mScrollFrame;
  nsIFrame* mScrolledFrame;
};












class nsDisplayScrollInfoLayer : public nsDisplayScrollLayer
{
public:
  nsDisplayScrollInfoLayer(nsDisplayListBuilder* aBuilder,
                           nsIFrame* aScrolledFrame, nsIFrame* aScrollFrame);
  NS_DISPLAY_DECL_NAME("ScrollInfoLayer", TYPE_SCROLL_INFO_LAYER)

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayScrollInfoLayer();
#endif

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;

  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem) MOZ_OVERRIDE;

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;
};






class nsDisplayClip : public nsDisplayWrapList {
public:
  




  nsDisplayClip(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                nsDisplayItem* aItem, const nsRect& aRect);
  nsDisplayClip(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                nsDisplayList* aList, const nsRect& aRect);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayClip();
#endif
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("Clip", TYPE_CLIP)
  virtual uint32_t GetPerFrameKey() { return 0; }
  
  const nsRect& GetClipRect() { return mClip; }
  void SetClipRect(const nsRect& aRect) { mClip = aRect; }

  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem) MOZ_OVERRIDE;

protected:
  nsRect    mClip;
};





class nsDisplayClipRoundedRect : public nsDisplayClip {
public:
  




  nsDisplayClipRoundedRect(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           nsDisplayItem* aItem,
                           const nsRect& aRect, nscoord aRadii[8]);
  nsDisplayClipRoundedRect(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                           nsDisplayList* aList,
                           const nsRect& aRect, nscoord aRadii[8]);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayClipRoundedRect();
#endif

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap) MOZ_OVERRIDE;
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("ClipRoundedRect", TYPE_CLIP_ROUNDED_RECT)

  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem);

  void GetRadii(nscoord aRadii[8]) {
    memcpy(aRadii, mRadii, sizeof(mRadii));
  }

private:
  nscoord mRadii[8];
};





class nsDisplayZoom : public nsDisplayOwnLayer {
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
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE
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
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) MOZ_OVERRIDE;
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) MOZ_OVERRIDE {
    *aSnap = false;
    return mEffectsBounds + ToReferenceFrame();
  }
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;  
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) MOZ_OVERRIDE;
  NS_DISPLAY_DECL_NAME("SVGEffects", TYPE_SVG_EFFECTS)

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager,
                                   const ContainerParameters& aParameters);
 
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;
  
  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) MOZ_OVERRIDE
  {
    
  }

  void PaintAsLayer(nsDisplayListBuilder* aBuilder,
                    nsRenderingContext* aCtx,
                    LayerManager* aManager);

#ifdef MOZ_DUMP_PAINTING
  void PrintEffects(FILE* aOutput);
#endif

private:
  
  nsRect mEffectsBounds;
};













 
class nsDisplayTransform: public nsDisplayItem
{
public:
  






  typedef gfx3DMatrix (* ComputeTransformFunction)(nsIFrame* aFrame, float aAppUnitsPerPixel);

  


  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayList *aList, uint32_t aIndex = 0);
  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayList *aList, ComputeTransformFunction aTransformGetter, uint32_t aIndex = 0);
  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayItem *aItem, uint32_t aIndex = 0);

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTransform()
  {
    MOZ_COUNT_DTOR(nsDisplayTransform);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplayTransform", TYPE_TRANSFORM)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
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
                                   const ContainerParameters& aParameters) MOZ_OVERRIDE;
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters) MOZ_OVERRIDE;
  virtual bool ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                   nsRegion *aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion) MOZ_OVERRIDE;
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

  enum {
    INDEX_MAX = UINT32_MAX >> nsDisplayItem::TYPE_BITS
  };

  const gfx3DMatrix& GetTransform(float aAppUnitsPerPixel);

  float GetHitDepthAtPoint(const nsPoint& aPoint);

  

















  static nsRect TransformRect(const nsRect &aUntransformedBounds, 
                              const nsIFrame* aFrame,
                              const nsPoint &aOrigin,
                              const nsRect* aBoundsOverride = nullptr);

  static nsRect TransformRectOut(const nsRect &aUntransformedBounds, 
                                 const nsIFrame* aFrame,
                                 const nsPoint &aOrigin,
                                 const nsRect* aBoundsOverride = nullptr);

  


  static bool UntransformRect(const nsRect &aUntransformedBounds, 
                                const nsIFrame* aFrame,
                                const nsPoint &aOrigin,
                                nsRect* aOutRect);
  
  static bool UntransformRectMatrix(const nsRect &aUntransformedBounds, 
                                    const gfx3DMatrix& aMatrix,
                                    float aAppUnitsPerPixel,
                                    nsRect* aOutRect);

  static gfxPoint3D GetDeltaToMozTransformOrigin(const nsIFrame* aFrame,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride);

  static gfxPoint3D GetDeltaToMozPerspectiveOrigin(const nsIFrame* aFrame,
                                                   float aAppUnitsPerPixel);

  













  static nsRect GetFrameBoundsForTransform(const nsIFrame* aFrame);

  struct FrameTransformProperties
  {
    FrameTransformProperties(const nsIFrame* aFrame,
                             float aAppUnitsPerPixel,
                             const nsRect* aBoundsOverride);
    FrameTransformProperties(const nsCSSValueList* aTransformList,
                             const gfxPoint3D& aToMozOrigin,
                             const gfxPoint3D& aToPerspectiveOrigin,
                             nscoord aChildPerspective)
      : mFrame(nullptr)
      , mTransformList(aTransformList)
      , mToMozOrigin(aToMozOrigin)
      , mToPerspectiveOrigin(aToPerspectiveOrigin)
      , mChildPerspective(aChildPerspective)
    {}

    const nsIFrame* mFrame;
    const nsCSSValueList* mTransformList;
    const gfxPoint3D mToMozOrigin;
    const gfxPoint3D mToPerspectiveOrigin;
    nscoord mChildPerspective;
  };

  












  static gfx3DMatrix GetResultingTransformMatrix(const nsIFrame* aFrame,
                                                 const nsPoint& aOrigin,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride = nullptr,
                                                 nsIFrame** aOutAncestor = nullptr);
  static gfx3DMatrix GetResultingTransformMatrix(const FrameTransformProperties& aProperties,
                                                 const nsPoint& aOrigin,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride = nullptr,
                                                 nsIFrame** aOutAncestor = nullptr);
  



  static bool ShouldPrerenderTransformedContent(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame,
                                                bool aLogAnimations = false);
  bool CanUseAsyncAnimations(nsDisplayListBuilder* aBuilder) MOZ_OVERRIDE;

private:
  static gfx3DMatrix GetResultingTransformMatrixInternal(const FrameTransformProperties& aProperties,
                                                         const nsPoint& aOrigin,
                                                         float aAppUnitsPerPixel,
                                                         const nsRect* aBoundsOverride,
                                                         nsIFrame** aOutAncestor);

  nsDisplayWrapList mStoredList;
  gfx3DMatrix mTransform;
  ComputeTransformFunction mTransformGetter;
  float mCachedAppUnitsPerPixel;
  uint32_t mIndex;
};












class nsCharClipDisplayItem : public nsDisplayItem {
public:
  nsCharClipDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame), mLeftEdge(0), mRightEdge(0) {}

  nsCharClipDisplayItem(nsIFrame* aFrame)
    : nsDisplayItem(nullptr, aFrame, nullptr, nsPoint()) {}

  struct ClipEdges {
    ClipEdges(const nsDisplayItem& aItem,
              nscoord aLeftEdge, nscoord aRightEdge) {
      nsRect r = aItem.GetUnderlyingFrame()->GetScrollableOverflowRect() +
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
