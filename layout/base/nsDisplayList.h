











































#ifndef NSDISPLAYLIST_H_
#define NSDISPLAYLIST_H_

#include "nsCOMPtr.h"
#include "nsIFrame.h"
#include "nsPoint.h"
#include "nsRect.h"
#include "nsISelection.h"
#include "nsCaret.h"
#include "plarena.h"
#include "Layers.h"
#include "nsRegion.h"
#include "FrameLayerBuilder.h"
#include "nsThemeConstants.h"

#include "mozilla/StandardInteger.h"

#include <stdlib.h>

class nsIPresShell;
class nsIContent;
class nsRenderingContext;
class nsDeviceContext;
class nsDisplayTableItem;
class nsDisplayItem;








































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

  



  bool IsForEventDelivery() { return mMode == EVENT_DELIVERY; }
  



  bool IsForPluginGeometry() { return mMode == PLUGIN_GEOMETRY; }
  


  bool IsForPainting() { return mMode == PAINTING; }
  



  bool IsBackgroundOnly() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "don't call this if we're not in a presshell");
    return CurrentPresShellState()->mIsBackgroundOnly;
  }
  






  bool IsAtRootOfPseudoStackingContext() { return mIsAtRootOfPseudoStackingContext; }

  



  nsISelection* GetBoundingSelection() { return mBoundingSelection; }
  



  nsIFrame* ReferenceFrame() const { return mReferenceFrame; }
  






  const nsPoint& ToReferenceFrame(const nsIFrame* aFrame) {
    if (aFrame != mCachedOffsetFrame) {
      mCachedOffsetFrame = aFrame;
      mCachedOffset = aFrame->GetOffsetToCrossDoc(ReferenceFrame());
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
  


  nsresult DisplayCaret(nsIFrame* aFrame, const nsRect& aDirtyRect,
      nsDisplayList* aList) {
    nsIFrame* frame = GetCaretFrame();
    if (aFrame != frame) {
      return NS_OK;
    }
    return frame->DisplayCaret(this, aDirtyRect, aList);
  }
  



  nsIFrame* GetCaretFrame() {
    return CurrentPresShellState()->mCaretFrame;
  }
  


  nsCaret* GetCaret();
  




  void EnterPresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);
  


  void LeavePresShell(nsIFrame* aReferenceFrame, const nsRect& aDirtyRect);

  




  bool IsInTransform() { return mInTransform; }
  



  void SetInTransform(bool aInTransform) { mInTransform = aInTransform; }

  


  void SetDisplayPort(const nsRect& aDisplayPort);
  const nsRect* GetDisplayPort() { return mHasDisplayPort ? &mDisplayPort : nsnull; }

  




  void SetHasFixedItems() { mHasFixedItems = true; }
  bool GetHasFixedItems() { return mHasFixedItems; }

  


  bool ShouldSyncDecodeImages() { return mSyncDecodeImages; }

  




  void SetSyncDecodeImages(bool aSyncDecodeImages) {
    mSyncDecodeImages = aSyncDecodeImages;
  }

  




  PRUint32 GetBackgroundPaintFlags();

  





  void SubtractFromVisibleRegion(nsRegion* aVisibleRegion,
                                 const nsRegion& aRegion);

  







  void MarkFramesForDisplayList(nsIFrame* aDirtyFrame,
                                const nsFrameList& aFrames,
                                const nsRect& aDirtyRect);

  


  FrameLayerBuilder* LayerBuilder() { return &mLayerBuilder; }

  


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

  













  void RegisterThemeGeometry(PRUint8 aWidgetType,
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
        mPrevCachedOffset(aBuilder->mCachedOffset),
        mPrevIsAtRootOfPseudoStackingContext(aBuilder->mIsAtRootOfPseudoStackingContext) {
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
    }
    AutoBuildingDisplayList(nsDisplayListBuilder* aBuilder,
                            nsIFrame* aForChild, bool aIsRoot)
      : mBuilder(aBuilder),
        mPrevCachedOffsetFrame(aBuilder->mCachedOffsetFrame),
        mPrevCachedOffset(aBuilder->mCachedOffset),
        mPrevIsAtRootOfPseudoStackingContext(aBuilder->mIsAtRootOfPseudoStackingContext) {
      if (mPrevCachedOffsetFrame == aForChild->GetParent()) {
        aBuilder->mCachedOffset += aForChild->GetPosition();
      } else {
        aBuilder->mCachedOffset = aForChild->GetOffsetToCrossDoc(aBuilder->ReferenceFrame());
      }
      aBuilder->mCachedOffsetFrame = aForChild;
      aBuilder->mIsAtRootOfPseudoStackingContext = aIsRoot;
    }
    ~AutoBuildingDisplayList() {
      mBuilder->mCachedOffsetFrame = mPrevCachedOffsetFrame;
      mBuilder->mCachedOffset = mPrevCachedOffset;
      mBuilder->mIsAtRootOfPseudoStackingContext = mPrevIsAtRootOfPseudoStackingContext;
    }
  private:
    nsDisplayListBuilder* mBuilder;
    const nsIFrame*       mPrevCachedOffsetFrame;
    nsPoint               mPrevCachedOffset;
    bool                  mPrevIsAtRootOfPseudoStackingContext;
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

  nsPresContext* CurrentPresContext() {
    return CurrentPresShellState()->mPresShell->GetPresContext();
  }

  



  
  void AddExcludedGlassRegion(nsRect &bounds) {
    mExcludedGlassRegion.Or(mExcludedGlassRegion, bounds);
  }
  const nsRegion& GetExcludedGlassRegion() {
    return mExcludedGlassRegion;
  }

private:
  void MarkOutOfFlowFrameForDisplay(nsIFrame* aDirtyFrame, nsIFrame* aFrame,
                                    const nsRect& aDirtyRect);

  struct PresShellState {
    nsIPresShell* mPresShell;
    nsIFrame*     mCaretFrame;
    PRUint32      mFirstFrameMarkedForDisplay;
    bool          mIsBackgroundOnly;
  };
  PresShellState* CurrentPresShellState() {
    NS_ASSERTION(mPresShellStates.Length() > 0,
                 "Someone forgot to enter a presshell");
    return &mPresShellStates[mPresShellStates.Length() - 1];
  }

  FrameLayerBuilder              mLayerBuilder;
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
  nsPoint                        mCachedOffset;
  nsRect                         mDisplayPort;
  nsRegion                       mExcludedGlassRegion;
  Mode                           mMode;
  bool                           mBuildCaret;
  bool                           mIgnoreSuppression;
  bool                           mHadToIgnoreSuppression;
  bool                           mIsAtRootOfPseudoStackingContext;
  bool                           mIncludeAllOutOfFlows;
  bool                           mSelectedFramesOnly;
  bool                           mAccurateVisibleRegions;
  
  
  bool                           mInTransform;
  bool                           mSyncDecodeImages;
  bool                           mIsPaintingToWindow;
  bool                           mHasDisplayPort;
  bool                           mHasFixedItems;
};

class nsDisplayItem;
class nsDisplayList;





class nsDisplayItemLink {
  
  
protected:
  nsDisplayItemLink() : mAbove(nsnull) {}
  nsDisplayItem* mAbove;  
  
  friend class nsDisplayList;
};
















class nsDisplayItem : public nsDisplayItemLink {
public:
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
      mToReferenceFrame = aBuilder->ToReferenceFrame(aFrame);
    }
  }
  nsDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                const nsPoint& aToReferenceFrame)
    : mFrame(aFrame)
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
  








  virtual PRUint32 GetPerFrameKey() { return PRUint32(GetType()); }
  












  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {}
  





  inline nsIFrame* GetUnderlyingFrame() const { return mFrame; }
  









  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
  {
    *aSnap = false;
    return nsRect(ToReferenceFrame(), GetUnderlyingFrame()->GetSize());
  }
  








  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap,
                                   bool* aForceTransparentSurface)
  {
    *aSnap = false;
    *aForceTransparentSurface = false;
    return nsRegion();
  }
  




  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor) { return false; }
  






  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame)
  { return false; }
  




  virtual bool ShouldFixToViewport(nsDisplayListBuilder* aBuilder)
  { return false; }

  



















  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager)
  { return mozilla::LAYER_NONE; }
  




  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) {}

#ifdef MOZ_DUMP_PAINTING
  


  bool Painted() { return mPainted; }

  


  void SetPainted() { mPainted = true; }
#endif

  













  typedef mozilla::FrameLayerBuilder::ContainerParameters ContainerParameters;
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters)
  { return nsnull; }

  


















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

  



  virtual nsDisplayList* GetList() { return nsnull; }

  



  const nsRect& GetVisibleRect() { return mVisibleRect; }
  
#ifdef MOZ_DUMP_PAINTING
  


  virtual const char* Name() = 0;
#endif

  nsDisplayItem* GetAbove() { return mAbove; }

  






  bool RecomputeVisibility(nsDisplayListBuilder* aBuilder,
                             nsRegion* aVisibleRegion);

  


  const nsPoint& ToReferenceFrame() const {
    NS_ASSERTION(mFrame, "No frame?");
    return mToReferenceFrame;
  }

  





  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder) { return nsRect(); }

  


  virtual void DisableComponentAlpha() {}

protected:
  friend class nsDisplayList;
  
  nsDisplayItem() {
    mAbove = nsnull;
  }
  
  nsIFrame* mFrame;
  
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
    mSentinel.mAbove = nsnull;
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
  
  



  nsresult AppendNewToTop(nsDisplayItem* aItem) {
    if (!aItem)
      return NS_ERROR_OUT_OF_MEMORY;
    AppendToTop(aItem);
    return NS_OK;
  }
  
  



  nsresult AppendNewToBottom(nsDisplayItem* aItem) {
    if (!aItem)
      return NS_ERROR_OUT_OF_MEMORY;
    AppendToBottom(aItem);
    return NS_OK;
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
      aList->mSentinel.mAbove = nsnull;
    }
  }
  
  


  void AppendToBottom(nsDisplayList* aList) {
    if (aList->mSentinel.mAbove) {
      aList->mTop->mAbove = mSentinel.mAbove;
      mTop = aList->mTop;
      mSentinel.mAbove = aList->mSentinel.mAbove;
           
      aList->mTop = &aList->mSentinel;
      aList->mSentinel.mAbove = nsnull;
    }
  }
  
  


  nsDisplayItem* RemoveBottom();
  
  


  void DeleteAll();
  
  


  nsDisplayItem* GetTop() const {
    return mTop != &mSentinel ? static_cast<nsDisplayItem*>(mTop) : nsnull;
  }
  


  nsDisplayItem* GetBottom() const { return mSentinel.mAbove; }
  bool IsEmpty() const { return mTop == &mSentinel; }
  
  



  PRUint32 Count() const;
  








  void SortByZOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);
  







  void SortByContentOrder(nsDisplayListBuilder* aBuilder, nsIContent* aCommonAncestor);

  





  typedef bool (* SortLEQ)(nsDisplayItem* aItem1, nsDisplayItem* aItem2,
                             void* aClosure);
  void Sort(nsDisplayListBuilder* aBuilder, SortLEQ aCmp, void* aClosure);

  



















  bool ComputeVisibilityForSublist(nsDisplayListBuilder* aBuilder,
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
    PAINT_EXISTING_TRANSACTION = 0x04
  };
  void PaintRoot(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                 PRUint32 aFlags) const;
  



  void PaintForFrame(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx,
                     nsIFrame* aForFrame, PRUint32 aFlags) const;
  


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
    for (PRInt32 i = 0; i < 6; ++i) {
      mLists[i].SortByContentOrder(aBuilder, aCommonAncestor);
    }
  }

private:
  
  
  void* operator new(size_t sz) CPP_THROW_NEW;

  nsDisplayList mLists[6];
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
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) {
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
                       PRUint32 aColor = 0)
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

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) {
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
      nsresult _rv =                                                          \
        aLists.Outlines()->AppendNewToTop(                                    \
            new (aBuilder) nsDisplayReflowCount(aBuilder, this, _name));      \
      NS_ENSURE_SUCCESS(_rv, _rv);                                            \
    }                                                                         \
  PR_END_MACRO

#define DO_GLOBAL_REFLOW_COUNT_DSP_COLOR(_name, _color)                       \
  PR_BEGIN_MACRO                                                              \
    if (!aBuilder->IsBackgroundOnly() && !aBuilder->IsForEventDelivery() &&   \
        PresContext()->PresShell()->IsPaintingFrameCounts()) {                \
      nsresult _rv =                                                          \
        aLists.Outlines()->AppendNewToTop(                                    \
             new (aBuilder) nsDisplayReflowCount(aBuilder, this, _name, _color)); \
      NS_ENSURE_SUCCESS(_rv, _rv);                                            \
    }                                                                         \
  PR_END_MACRO




#define DECL_DO_GLOBAL_REFLOW_COUNT_DSP(_class, _super)                   \
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,           \
                              const nsRect&           aDirtyRect,         \
                              const nsDisplayListSet& aLists) {           \
    DO_GLOBAL_REFLOW_COUNT_DSP(#_class);                                  \
    return _super::BuildDisplayList(aBuilder, aDirtyRect, aLists);        \
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
    *aSnap = false;
    
    return mCaret->GetCaretRect() + ToReferenceFrame();
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  NS_DISPLAY_DECL_NAME("Border", TYPE_BORDER)
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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);

  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap,
                                   bool* aOutTransparentBackground) {
    *aSnap = false;
    *aOutTransparentBackground = false;
    nsRegion result;
    if (NS_GET_A(mColor) == 255) {
      result = GetBounds(aBuilder, aSnap);
    }
    return result;
  }

  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor)
  {
    *aColor = mColor;
    return true;
  }

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);

  NS_DISPLAY_DECL_NAME("SolidColor", TYPE_SOLID_COLOR)

private:
  nsRect  mBounds;
  nscolor mColor;
};




class nsDisplayBackground : public nsDisplayItem {
public:
  nsDisplayBackground(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBackground() {
    MOZ_COUNT_DTOR(nsDisplayBackground);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame);
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor);
  virtual bool ShouldFixToViewport(nsDisplayListBuilder* aBuilder);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("Background", TYPE_BACKGROUND)
  
  bool IsThemed() { return mIsThemed; }

protected:
  nsRegion GetInsideClipRegion(nsPresContext* aPresContext, PRUint8 aClip,
                               const nsRect& aRect, bool* aSnap);

  
  bool mIsThemed;
  nsITheme::Transparency mThemeTransparency;
};




class nsDisplayBoxShadowOuter : public nsDisplayItem {
public:
  nsDisplayBoxShadowOuter(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayBoxShadowOuter);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayBoxShadowOuter() {
    MOZ_COUNT_DTOR(nsDisplayBoxShadowOuter);
  }
#endif

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  NS_DISPLAY_DECL_NAME("BoxShadowOuter", TYPE_BOX_SHADOW_OUTER)

private:
  nsRegion mVisibleRegion;
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

  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  NS_DISPLAY_DECL_NAME("BoxShadowInner", TYPE_BOX_SHADOW_INNER)

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

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
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
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  NS_DISPLAY_DECL_NAME("EventReceiver", TYPE_EVENT_RECEIVER)
};















class nsDisplayWrapList : public nsDisplayItem {
  
  

public:
  


  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayItem* aItem);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayItem* aItem, const nsPoint& aToReferenceFrame);
  nsDisplayWrapList(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {}
  virtual ~nsDisplayWrapList();
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual bool IsUniform(nsDisplayListBuilder* aBuilder, nscolor* aColor);
  virtual bool IsVaryingRelativeToMovingFrame(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                 nsRegion* aVisibleRegion,
                                 const nsRect& aAllowVisibleRegionExpansion);
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem) {
    NS_WARNING("This list should already have been flattened!!!");
    return false;
  }
  virtual void GetMergedFrames(nsTArray<nsIFrame*>* aFrames)
  {
    aFrames->AppendElements(mMergedFrames);
  }
  NS_DISPLAY_DECL_NAME("WrapList", TYPE_WRAP_LIST)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder);
                                    
  virtual nsDisplayList* GetList() { return &mList; }
  
  





  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem) {
    NS_NOTREACHED("We never returned nsnull for GetUnderlyingFrame!");
    return nsnull;
  }

  




  static bool ChildrenCanBeInactive(nsDisplayListBuilder* aBuilder,
                                      LayerManager* aManager,
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
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters);
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);  
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("Opacity", TYPE_OPACITY)
};





class nsDisplayOwnLayer : public nsDisplayWrapList {
public:
  nsDisplayOwnLayer(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                    nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOwnLayer();
#endif
  
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters);
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager)
  {
    return mozilla::LAYER_ACTIVE;
  }
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem)
  {
    
    return false;
  }
  NS_DISPLAY_DECL_NAME("OwnLayer", TYPE_OWN_LAYER)
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
                                             const ContainerParameters& aContainerParameters);

  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);

  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager);

  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem);

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder);

  
  
  
  intptr_t GetScrollLayerCount();
  intptr_t RemoveScrollLayerCount();

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
                                   LayerManager* aManager);

  virtual bool TryMerge(nsDisplayListBuilder* aBuilder,
                          nsDisplayItem* aItem);

  virtual bool ShouldFlattenAway(nsDisplayListBuilder* aBuilder);
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
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("Clip", TYPE_CLIP)
  virtual PRUint32 GetPerFrameKey() { return 0; }
  
  const nsRect& GetClipRect() { return mClip; }
  void SetClipRect(const nsRect& aRect) { mClip = aRect; }

  virtual nsDisplayWrapList* WrapWithClone(nsDisplayListBuilder* aBuilder,
                                           nsDisplayItem* aItem);

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
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
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
                PRInt32 aAPD, PRInt32 aParentAPD);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayZoom();
#endif
  
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  NS_DISPLAY_DECL_NAME("Zoom", TYPE_ZOOM)

  
  PRInt32 GetChildAppUnitsPerDevPixel() { return mAPD; }
  
  PRInt32 GetParentAppUnitsPerDevPixel() { return mParentAPD; }

private:
  PRInt32 mAPD, mParentAPD;
};





class nsDisplaySVGEffects : public nsDisplayWrapList {
public:
  nsDisplaySVGEffects(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame,
                      nsDisplayList* aList);
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVGEffects();
#endif
  
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder* aBuilder,
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
    *aSnap = false;
    return mEffectsBounds + ToReferenceFrame();
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx);
  virtual bool ComputeVisibility(nsDisplayListBuilder* aBuilder,
                                   nsRegion* aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);  
  virtual bool TryMerge(nsDisplayListBuilder* aBuilder, nsDisplayItem* aItem);
  NS_DISPLAY_DECL_NAME("SVGEffects", TYPE_SVG_EFFECTS)

#ifdef MOZ_DUMP_PAINTING
  void PrintEffects(FILE* aOutput);
#endif

private:
  
  nsRect mEffectsBounds;
};












 
class nsDisplayTransform: public nsDisplayItem
{
public:
  


  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayList *aList, PRUint32 aIndex = 0) :
    nsDisplayItem(aBuilder, aFrame), mStoredList(aBuilder, aFrame, aList), mIndex(aIndex)
  {
    MOZ_COUNT_CTOR(nsDisplayTransform);
    NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  }

  nsDisplayTransform(nsDisplayListBuilder* aBuilder, nsIFrame *aFrame,
                     nsDisplayItem *aItem, PRUint32 aIndex = 0) :
  nsDisplayItem(aBuilder, aFrame), mStoredList(aBuilder, aFrame, aItem), mIndex(aIndex)
  {
    MOZ_COUNT_CTOR(nsDisplayTransform);
    NS_ABORT_IF_FALSE(aFrame, "Must have a frame!");
  }

#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayTransform()
  {
    MOZ_COUNT_DTOR(nsDisplayTransform);
  }
#endif

  NS_DISPLAY_DECL_NAME("nsDisplayTransform", TYPE_TRANSFORM);

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
  {
    if (mStoredList.GetComponentAlphaBounds(aBuilder).IsEmpty())
      return nsRect();
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  nsDisplayWrapList* GetStoredList() { return &mStoredList; }

  virtual void HitTest(nsDisplayListBuilder *aBuilder, const nsRect& aRect,
                       HitTestState *aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual nsRect GetBounds(nsDisplayListBuilder *aBuilder, bool* aSnap);
  virtual nsRegion GetOpaqueRegion(nsDisplayListBuilder *aBuilder,
                                   bool* aSnap,
                                   bool* aForceTransparentSurface);
  virtual bool IsUniform(nsDisplayListBuilder *aBuilder, nscolor* aColor);
  virtual LayerState GetLayerState(nsDisplayListBuilder* aBuilder,
                                   LayerManager* aManager);
  virtual already_AddRefed<Layer> BuildLayer(nsDisplayListBuilder* aBuilder,
                                             LayerManager* aManager,
                                             const ContainerParameters& aContainerParameters);
  virtual bool ComputeVisibility(nsDisplayListBuilder *aBuilder,
                                   nsRegion *aVisibleRegion,
                                   const nsRect& aAllowVisibleRegionExpansion);
  virtual bool TryMerge(nsDisplayListBuilder *aBuilder, nsDisplayItem *aItem);
  
  virtual PRUint32 GetPerFrameKey() { return (mIndex << nsDisplayItem::TYPE_BITS) | nsDisplayItem::GetPerFrameKey(); }

  enum {
    INDEX_MAX = PR_UINT32_MAX >> nsDisplayItem::TYPE_BITS
  };

  const gfx3DMatrix& GetTransform(float aAppUnitsPerPixel);

  float GetHitDepthAtPoint(const nsPoint& aPoint);

  

















  static nsRect TransformRect(const nsRect &aUntransformedBounds, 
                              const nsIFrame* aFrame,
                              const nsPoint &aOrigin,
                              const nsRect* aBoundsOverride = nsnull);

  static nsRect TransformRectOut(const nsRect &aUntransformedBounds, 
                                 const nsIFrame* aFrame,
                                 const nsPoint &aOrigin,
                                 const nsRect* aBoundsOverride = nsnull);

  


  static bool UntransformRect(const nsRect &aUntransformedBounds, 
                                const nsIFrame* aFrame,
                                const nsPoint &aOrigin,
                                nsRect* aOutRect);
  
  static bool UntransformRectMatrix(const nsRect &aUntransformedBounds, 
                                    const gfx3DMatrix& aMatrix,
                                    float aAppUnitsPerPixel,
                                    nsRect* aOutRect);

  













  static nsRect GetFrameBoundsForTransform(const nsIFrame* aFrame);

  












  static gfx3DMatrix GetResultingTransformMatrix(const nsIFrame* aFrame,
                                                 const nsPoint& aOrigin,
                                                 float aAppUnitsPerPixel,
                                                 const nsRect* aBoundsOverride = nsnull,
                                                 nsIFrame** aOutAncestor = nsnull);
  



  static bool ShouldPrerenderTransformedContent(nsDisplayListBuilder* aBuilder,
                                                nsIFrame* aFrame);

private:
  nsDisplayWrapList mStoredList;
  gfx3DMatrix mTransform;
  float mCachedAppUnitsPerPixel;
  PRUint32 mIndex;
};












class nsCharClipDisplayItem : public nsDisplayItem {
public:
  nsCharClipDisplayItem(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame), mLeftEdge(0), mRightEdge(0) {}

  struct ClipEdges {
    ClipEdges(const nsDisplayItem& aItem,
              nscoord aLeftEdge, nscoord aRightEdge) {
      nsRect r = aItem.GetUnderlyingFrame()->GetScrollableOverflowRect() +
                 aItem.ToReferenceFrame();
      mX = aLeftEdge > 0 ? r.x + aLeftEdge : nscoord_MIN;
      mXMost = aRightEdge > 0 ? NS_MAX(r.XMost() - aRightEdge, mX) : nscoord_MAX;
    }
    void Intersect(nscoord* aX, nscoord* aWidth) const {
      nscoord xmost1 = *aX + *aWidth;
      *aX = NS_MAX(*aX, mX);
      *aWidth = NS_MAX(NS_MIN(xmost1, mXMost) - *aX, 0);
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
      ? static_cast<nsCharClipDisplayItem*>(aItem) : nsnull;
  }

  nscoord mLeftEdge;  
  nscoord mRightEdge; 
};

#endif
