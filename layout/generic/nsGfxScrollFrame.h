






#ifndef nsGfxScrollFrame_h___
#define nsGfxScrollFrame_h___

#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsBoxFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsIStatefulFrame.h"
#include "nsThreadUtils.h"
#include "nsIReflowCallback.h"
#include "nsBoxLayoutState.h"
#include "nsQueryFrame.h"
#include "nsExpirationTracker.h"

class nsPresContext;
class nsIPresShell;
class nsIContent;
class nsIAtom;
class nsIScrollFrameInternal;
class nsPresState;
class nsIScrollPositionListener;
struct ScrollReflowState;

namespace mozilla {
namespace layers {
class Layer;
}
namespace layout {
class ScrollbarActivity;
}
}

namespace mozilla {

class ScrollFrameHelper : public nsIReflowCallback {
public:
  typedef nsIFrame::Sides Sides;
  typedef mozilla::CSSIntPoint CSSIntPoint;
  typedef mozilla::layout::ScrollbarActivity ScrollbarActivity;
  typedef mozilla::layers::FrameMetrics FrameMetrics;
  typedef mozilla::layers::Layer Layer;

  class AsyncScroll;
  class AsyncSmoothMSDScroll;

  ScrollFrameHelper(nsContainerFrame* aOuter, bool aIsRoot);
  ~ScrollFrameHelper();

  mozilla::ScrollbarStyles GetScrollbarStylesFromFrame() const;

  
  
  
  void ReloadChildFrames();

  nsresult CreateAnonymousContent(
    nsTArray<nsIAnonymousContentCreator::ContentInfo>& aElements);
  void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements, uint32_t aFilter);
  nsresult FireScrollPortEvent();
  void PostOverflowEvent();
  void Destroy();

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists);

  void AppendScrollPartsTo(nsDisplayListBuilder*   aBuilder,
                           const nsRect&           aDirtyRect,
                           const nsDisplayListSet& aLists,
                           bool                    aCreateLayer,
                           bool                    aPositioned);

  bool GetBorderRadii(const nsSize& aFrameSize, const nsSize& aBorderArea,
                      Sides aSkipSides, nscoord aRadii[8]) const;

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

  



  void CurPosAttributeChanged(nsIContent* aChild);

  void PostScrollEvent();
  void FireScrollEvent();
  void PostScrolledAreaEvent();
  void FireScrolledAreaEvent();

  class ScrollEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit ScrollEvent(ScrollFrameHelper *helper) : mHelper(helper) {}
    void Revoke() { mHelper = nullptr; }
  private:
    ScrollFrameHelper *mHelper;
  };

  class AsyncScrollPortEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit AsyncScrollPortEvent(ScrollFrameHelper *helper) : mHelper(helper) {}
    void Revoke() { mHelper = nullptr; }
  private:
    ScrollFrameHelper *mHelper;
  };

  class ScrolledAreaEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit ScrolledAreaEvent(ScrollFrameHelper *helper) : mHelper(helper) {}
    void Revoke() { mHelper = nullptr; }
  private:
    ScrollFrameHelper *mHelper;
  };

  


  void FinishReflowForScrollbar(nsIContent* aContent, nscoord aMinXY,
                                nscoord aMaxXY, nscoord aCurPosXY,
                                nscoord aPageIncrement,
                                nscoord aIncrement);
  


  void SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos);
  


  void SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom, nscoord aSize);

  nscoord GetCoordAttribute(nsIFrame* aFrame, nsIAtom* aAtom, nscoord aDefaultValue,
                            nscoord* aRangeStart, nscoord* aRangeLength);

  



  void UpdateScrollbarPosition();

  nsRect GetScrollPortRect() const { return mScrollPort; }
  nsPoint GetScrollPosition() const {
    return mScrollPort.TopLeft() - mScrolledFrame->GetPosition();
  }
  






  nsPoint GetLogicalScrollPosition() const {
    nsPoint pt;
    pt.x = IsLTR() ?
      mScrollPort.x - mScrolledFrame->GetPosition().x :
      mScrollPort.XMost() - mScrolledFrame->GetRect().XMost();
    pt.y = mScrollPort.y - mScrolledFrame->GetPosition().y;
    return pt;
  }
  nsRect GetScrollRange() const;
  
  nsRect GetScrollRange(nscoord aWidth, nscoord aHeight) const;
  nsSize GetScrollPositionClampingScrollPortSize() const;
  gfxSize GetResolution() const;
  void SetResolution(const gfxSize& aResolution);

protected:
  nsRect GetScrollRangeForClamping() const;

public:
  static void AsyncScrollCallback(ScrollFrameHelper* aInstance,
                                  mozilla::TimeStamp aTime);
  static void AsyncSmoothMSDScrollCallback(ScrollFrameHelper* aInstance,
                                           mozilla::TimeDuration aDeltaTime);
  





  void ScrollTo(nsPoint aScrollPosition, nsIScrollableFrame::ScrollMode aMode,
                const nsRect* aRange = nullptr) {
    ScrollToWithOrigin(aScrollPosition, aMode, nsGkAtoms::other, aRange);
  }
  


  void ScrollToCSSPixels(const CSSIntPoint& aScrollPosition,
                         nsIScrollableFrame::ScrollMode aMode
                           = nsIScrollableFrame::INSTANT);
  


  void ScrollToCSSPixelsApproximate(const mozilla::CSSPoint& aScrollPosition,
                                    nsIAtom* aOrigin = nullptr);

  CSSIntPoint GetScrollPositionCSSPixels();
  


  void ScrollToImpl(nsPoint aScrollPosition, const nsRect& aRange, nsIAtom* aOrigin = nullptr);
  void ScrollVisual(nsPoint aOldScrolledFramePosition);
  


  void ScrollBy(nsIntPoint aDelta, nsIScrollableFrame::ScrollUnit aUnit,
                nsIScrollableFrame::ScrollMode aMode, nsIntPoint* aOverflow,
                nsIAtom* aOrigin = nullptr, bool aIsMomentum = false);
  


  void ScrollToRestoredPosition();

  nsSize GetLineScrollAmount() const;
  nsSize GetPageScrollAmount() const;

  nsPresState* SaveState() const;
  void RestoreState(nsPresState* aState);

  nsIFrame* GetScrolledFrame() const { return mScrolledFrame; }
  nsIFrame* GetScrollbarBox(bool aVertical) const {
    return aVertical ? mVScrollbarBox : mHScrollbarBox;
  }

  void AddScrollPositionListener(nsIScrollPositionListener* aListener) {
    mListeners.AppendElement(aListener);
  }
  void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) {
    mListeners.RemoveElement(aListener);
  }

  static void SetScrollbarVisibility(nsIFrame* aScrollbar, bool aVisible);

  













  nsRect GetScrolledRect() const;

  










  nsRect GetScrolledRectInternal(const nsRect& aScrolledOverflowArea,
                                 const nsSize& aScrollPortSize) const;

  uint32_t GetScrollbarVisibility() const {
    return (mHasVerticalScrollbar ? nsIScrollableFrame::VERTICAL : 0) |
           (mHasHorizontalScrollbar ? nsIScrollableFrame::HORIZONTAL : 0);
  }
  nsMargin GetActualScrollbarSizes() const;
  nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);
  nscoord GetNondisappearingScrollbarWidth(nsBoxLayoutState* aState);
  bool IsLTR() const;
  bool IsScrollbarOnRight() const;
  bool IsScrollingActive() const { return mScrollingActive || mShouldBuildScrollableLayer; }
  bool IsProcessingAsyncScroll() const {
    return mAsyncScroll != nullptr || mAsyncSmoothMSDScroll != nullptr;
  }
  void ResetScrollPositionForLayerPixelAlignment()
  {
    mScrollPosForLayerPixelAlignment = GetScrollPosition();
  }

  bool UpdateOverflow();

  void UpdateSticky();

  bool IsRectNearlyVisible(const nsRect& aRect) const;
  nsRect ExpandRectToNearlyVisible(const nsRect& aRect) const;

  
  
  
  void AdjustScrollbarRectForResizer(nsIFrame* aFrame, nsPresContext* aPresContext,
                                     nsRect& aRect, bool aHasResizer, bool aVertical);
  
  bool HasResizer() { return mResizerBox && !mCollapsedResizer; }
  void LayoutScrollbars(nsBoxLayoutState& aState,
                        const nsRect& aContentArea,
                        const nsRect& aOldScrollArea);

  bool IsIgnoringViewportClipping() const;

  bool ShouldClampScrollPosition() const;

  bool IsAlwaysActive() const;
  void MarkActive();
  void MarkInactive();
  nsExpirationState* GetExpirationState() { return &mActivityExpirationState; }

  void ScheduleSyntheticMouseMove();
  static void ScrollActivityCallback(nsITimer *aTimer, void* anInstance);

  void HandleScrollbarStyleSwitching();

  nsIAtom* OriginOfLastScroll() const { return mOriginOfLastScroll; }
  uint32_t CurrentScrollGeneration() const { return mScrollGeneration; }
  void ResetOriginIfScrollAtGeneration(uint32_t aGeneration) {
    if (aGeneration == mScrollGeneration) {
      mOriginOfLastScroll = nullptr;
    }
  }
  bool WantAsyncScroll() const;
  void ComputeFrameMetrics(Layer* aLayer, nsIFrame* aContainerReferenceFrame,
                           const ContainerLayerParameters& aParameters,
                           nsRect* aClipRect,
                           nsTArray<FrameMetrics>* aOutput) const;

  
  void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection);
  void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection);
  void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection);
  void RepeatButtonScroll(nsScrollbarFrame* aScrollbar);
  void ThumbMoved(nsScrollbarFrame* aScrollbar,
                  nscoord aOldPos,
                  nscoord aNewPos);
  void ScrollByUnit(nsScrollbarFrame* aScrollbar,
                    nsIScrollableFrame::ScrollMode aMode,
                    int32_t aDirection,
                    nsIScrollableFrame::ScrollUnit aUnit);

  
  nsCOMPtr<nsIContent> mHScrollbarContent;
  nsCOMPtr<nsIContent> mVScrollbarContent;
  nsCOMPtr<nsIContent> mScrollCornerContent;
  nsCOMPtr<nsIContent> mResizerContent;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;
  nsRevocableEventPtr<AsyncScrollPortEvent> mAsyncScrollPortEvent;
  nsRevocableEventPtr<ScrolledAreaEvent> mScrolledAreaEvent;
  nsIFrame* mHScrollbarBox;
  nsIFrame* mVScrollbarBox;
  nsIFrame* mScrolledFrame;
  nsIFrame* mScrollCornerBox;
  nsIFrame* mResizerBox;
  nsContainerFrame* mOuter;
  nsRefPtr<AsyncScroll> mAsyncScroll;
  nsRefPtr<AsyncSmoothMSDScroll> mAsyncSmoothMSDScroll;
  nsRefPtr<ScrollbarActivity> mScrollbarActivity;
  nsTArray<nsIScrollPositionListener*> mListeners;
  nsIAtom* mOriginOfLastScroll;
  uint32_t mScrollGeneration;
  nsRect mScrollPort;
  
  
  
  
  nsPoint mDestination;
  nsPoint mScrollPosAtLastPaint;

  
  
  
  
  nsPoint mRestorePos;
  
  
  
  nsPoint mLastPos;

  
  gfxSize mResolution;

  nsExpirationState mActivityExpirationState;

  nsCOMPtr<nsITimer> mScrollActivityTimer;
  nsPoint mScrollPosForLayerPixelAlignment;

  
  nsPoint mLastUpdateImagesPos;

  FrameMetrics::ViewID mScrollParentID;

  bool mNeverHasVerticalScrollbar:1;
  bool mNeverHasHorizontalScrollbar:1;
  bool mHasVerticalScrollbar:1;
  bool mHasHorizontalScrollbar:1;
  bool mFrameIsUpdatingScrollbar:1;
  bool mDidHistoryRestore:1;
  
  bool mIsRoot:1;
  
  
  bool mClipAllDescendants:1;
  
  
  
  bool mSupppressScrollbarUpdate:1;
  
  
  
  
  bool mSkippedScrollbarLayout:1;

  bool mHadNonInitialReflow:1;
  
  
  bool mHorizontalOverflow:1;
  bool mVerticalOverflow:1;
  bool mPostedReflowCallback:1;
  bool mMayHaveDirtyFixedChildren:1;
  
  
  bool mUpdateScrollbarAttributes:1;
  
  
  bool mScrollingActive:1;
  
  bool mCollapsedResizer:1;

  
  
  bool mShouldBuildScrollableLayer:1;

  
  bool mHasBeenScrolled:1;

  
  
  bool mIsResolutionSet:1;

protected:
  


  void ScrollToWithOrigin(nsPoint aScrollPosition,
                          nsIScrollableFrame::ScrollMode aMode,
                          nsIAtom *aOrigin, 
                          const nsRect* aRange);

  void CompleteAsyncScroll(const nsRect &aRange, nsIAtom* aOrigin = nullptr);

  static void EnsureImageVisPrefsCached();
  static bool sImageVisPrefsCached;
  
  static uint32_t sHorzExpandScrollPort;
  static uint32_t sVertExpandScrollPort;
  
  
  static int32_t sHorzScrollFraction;
  static int32_t sVertScrollFraction;
};

}










class nsHTMLScrollFrame : public nsContainerFrame,
                          public nsIScrollableFrame,
                          public nsIAnonymousContentCreator,
                          public nsIStatefulFrame {
public:
  typedef mozilla::ScrollFrameHelper ScrollFrameHelper;
  typedef mozilla::CSSIntPoint CSSIntPoint;
  friend nsHTMLScrollFrame* NS_NewHTMLScrollFrame(nsIPresShell* aPresShell,
                                                  nsStyleContext* aContext,
                                                  bool aIsRoot);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {
    mHelper.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  bool TryLayout(ScrollReflowState* aState,
                   nsHTMLReflowMetrics* aKidMetrics,
                   bool aAssumeVScroll, bool aAssumeHScroll,
                   bool aForce);
  bool ScrolledContentDependsOnHeight(ScrollReflowState* aState);
  void ReflowScrolledFrame(ScrollReflowState* aState,
                           bool aAssumeHScroll,
                           bool aAssumeVScroll,
                           nsHTMLReflowMetrics* aMetrics,
                           bool aFirstPass);
  void ReflowContents(ScrollReflowState* aState,
                      const nsHTMLReflowMetrics& aDesiredSize);
  void PlaceScrollArea(const ScrollReflowState& aState,
                       const nsPoint& aScrollPosition);
  nscoord GetIntrinsicVScrollbarWidth(nsRenderingContext *aRenderingContext);

  virtual bool GetBorderRadii(const nsSize& aFrameSize, const nsSize& aBorderArea,
                              Sides aSkipSides, nscoord aRadii[8]) const MOZ_OVERRIDE {
    return mHelper.GetBorderRadii(aFrameSize, aBorderArea, aSkipSides, aRadii);
  }

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nsresult GetPadding(nsMargin& aPadding) MOZ_OVERRIDE;
  virtual bool IsCollapsed() MOZ_OVERRIDE;
  
  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE {
    return mHelper.UpdateOverflow();
  }

  
  
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE {
    return this;
  }

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    return mHelper.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual bool DoesClipChildren() MOZ_OVERRIDE { return true; }
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild) MOZ_OVERRIDE
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mHelper.GetScrolledFrame()) pt += GetScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  
  virtual nsIFrame* GetScrolledFrame() const MOZ_OVERRIDE {
    return mHelper.GetScrolledFrame();
  }
  virtual mozilla::ScrollbarStyles GetScrollbarStyles() const MOZ_OVERRIDE {
    return mHelper.GetScrollbarStylesFromFrame();
  }
  virtual uint32_t GetScrollbarVisibility() const MOZ_OVERRIDE {
    return mHelper.GetScrollbarVisibility();
  }
  virtual nsMargin GetActualScrollbarSizes() const MOZ_OVERRIDE {
    return mHelper.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) MOZ_OVERRIDE {
    return mHelper.GetDesiredScrollbarSizes(aState);
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
          nsRenderingContext* aRC) MOZ_OVERRIDE {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return GetDesiredScrollbarSizes(&bls);
  }
  virtual nscoord GetNondisappearingScrollbarWidth(nsPresContext* aPresContext,
          nsRenderingContext* aRC) MOZ_OVERRIDE {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return mHelper.GetNondisappearingScrollbarWidth(&bls);
  }
  virtual nsRect GetScrolledRect() const MOZ_OVERRIDE {
    return mHelper.GetScrolledRect();
  }
  virtual nsRect GetScrollPortRect() const MOZ_OVERRIDE {
    return mHelper.GetScrollPortRect();
  }
  virtual nsPoint GetScrollPosition() const MOZ_OVERRIDE {
    return mHelper.GetScrollPosition();
  }
  virtual nsPoint GetLogicalScrollPosition() const MOZ_OVERRIDE {
    return mHelper.GetLogicalScrollPosition();
  }
  virtual nsRect GetScrollRange() const MOZ_OVERRIDE {
    return mHelper.GetScrollRange();
  }
  virtual nsSize GetScrollPositionClampingScrollPortSize() const MOZ_OVERRIDE {
    return mHelper.GetScrollPositionClampingScrollPortSize();
  }
  virtual gfxSize GetResolution() const MOZ_OVERRIDE {
    return mHelper.GetResolution();
  }
  virtual void SetResolution(const gfxSize& aResolution) MOZ_OVERRIDE {
    return mHelper.SetResolution(aResolution);
  }
  virtual nsSize GetLineScrollAmount() const MOZ_OVERRIDE {
    return mHelper.GetLineScrollAmount();
  }
  virtual nsSize GetPageScrollAmount() const MOZ_OVERRIDE {
    return mHelper.GetPageScrollAmount();
  }
  


  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode,
                        const nsRect* aRange = nullptr) MOZ_OVERRIDE {
    mHelper.ScrollTo(aScrollPosition, aMode, aRange);
  }
  


  virtual void ScrollToCSSPixels(const CSSIntPoint& aScrollPosition,
                                 nsIScrollableFrame::ScrollMode aMode
                                   = nsIScrollableFrame::INSTANT) MOZ_OVERRIDE {
    mHelper.ScrollToCSSPixels(aScrollPosition, aMode);
  }
  virtual void ScrollToCSSPixelsApproximate(const mozilla::CSSPoint& aScrollPosition,
                                            nsIAtom* aOrigin = nullptr) MOZ_OVERRIDE {
    mHelper.ScrollToCSSPixelsApproximate(aScrollPosition, aOrigin);
  }
  


  virtual CSSIntPoint GetScrollPositionCSSPixels() MOZ_OVERRIDE {
    return mHelper.GetScrollPositionCSSPixels();
  }
  


  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow, nsIAtom* aOrigin = nullptr,
                        bool aIsMomentum = false) MOZ_OVERRIDE {
    mHelper.ScrollBy(aDelta, aUnit, aMode, aOverflow, aOrigin, aIsMomentum);
  }
  


  virtual void ScrollToRestoredPosition() MOZ_OVERRIDE {
    mHelper.ScrollToRestoredPosition();
  }
  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) MOZ_OVERRIDE {
    mHelper.AddScrollPositionListener(aListener);
  }
  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) MOZ_OVERRIDE {
    mHelper.RemoveScrollPositionListener(aListener);
  }
  


  virtual void CurPosAttributeChanged(nsIContent* aChild) MOZ_OVERRIDE {
    mHelper.CurPosAttributeChanged(aChild);
  }
  NS_IMETHOD PostScrolledAreaEventForCurrentArea() MOZ_OVERRIDE {
    mHelper.PostScrolledAreaEvent();
    return NS_OK;
  }
  virtual bool IsScrollingActive() MOZ_OVERRIDE {
    return mHelper.IsScrollingActive();
  }
  virtual bool IsProcessingAsyncScroll() MOZ_OVERRIDE {
    return mHelper.IsProcessingAsyncScroll();
  }
  virtual void ResetScrollPositionForLayerPixelAlignment() MOZ_OVERRIDE {
    mHelper.ResetScrollPositionForLayerPixelAlignment();
  }
  virtual bool IsResolutionSet() const MOZ_OVERRIDE {
    return mHelper.mIsResolutionSet;
  }
  virtual bool DidHistoryRestore() const MOZ_OVERRIDE {
    return mHelper.mDidHistoryRestore;
  }
  virtual void ClearDidHistoryRestore() MOZ_OVERRIDE {
    mHelper.mDidHistoryRestore = false;
  }
  virtual bool IsRectNearlyVisible(const nsRect& aRect) MOZ_OVERRIDE {
    return mHelper.IsRectNearlyVisible(aRect);
  }
  virtual nsRect ExpandRectToNearlyVisible(const nsRect& aRect) const MOZ_OVERRIDE {
    return mHelper.ExpandRectToNearlyVisible(aRect);
  }
  virtual nsIAtom* OriginOfLastScroll() MOZ_OVERRIDE {
    return mHelper.OriginOfLastScroll();
  }
  virtual uint32_t CurrentScrollGeneration() MOZ_OVERRIDE {
    return mHelper.CurrentScrollGeneration();
  }
  virtual void ResetOriginIfScrollAtGeneration(uint32_t aGeneration) MOZ_OVERRIDE {
    mHelper.ResetOriginIfScrollAtGeneration(aGeneration);
  }
  virtual bool WantAsyncScroll() const MOZ_OVERRIDE {
    return mHelper.WantAsyncScroll();
  }
  virtual void ComputeFrameMetrics(Layer* aLayer, nsIFrame* aContainerReferenceFrame,
                                   const ContainerLayerParameters& aParameters,
                                   nsRect* aClipRect,
                                   nsTArray<FrameMetrics>* aOutput) const MOZ_OVERRIDE {
    mHelper.ComputeFrameMetrics(aLayer, aContainerReferenceFrame,
                                aParameters, aClipRect, aOutput);
  }

  
  NS_IMETHOD SaveState(nsPresState** aState) MOZ_OVERRIDE {
    NS_ENSURE_ARG_POINTER(aState);
    *aState = mHelper.SaveState();
    return NS_OK;
  }
  NS_IMETHOD RestoreState(nsPresState* aState) MOZ_OVERRIDE {
    NS_ENSURE_ARG_POINTER(aState);
    mHelper.RestoreState(aState);
    return NS_OK;
  }

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  
  virtual void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByPage(aScrollbar, aDirection);
  }
  virtual void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByWhole(aScrollbar, aDirection);
  }
  virtual void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByLine(aScrollbar, aDirection);
  }
  virtual void RepeatButtonScroll(nsScrollbarFrame* aScrollbar) MOZ_OVERRIDE {
    mHelper.RepeatButtonScroll(aScrollbar);
  }
  virtual void ThumbMoved(nsScrollbarFrame* aScrollbar,
                          nscoord aOldPos,
                          nscoord aNewPos) MOZ_OVERRIDE {
    mHelper.ThumbMoved(aScrollbar, aOldPos, aNewPos);
  }
  virtual void VisibilityChanged(bool aVisible) {}
  virtual nsIFrame* GetScrollbarBox(bool aVertical) MOZ_OVERRIDE {
    return mHelper.GetScrollbarBox(aVertical);
  }
  virtual void ScrollbarActivityStarted() const MOZ_OVERRIDE;
  virtual void ScrollbarActivityStopped() const MOZ_OVERRIDE;
  
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

protected:
  nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, bool aIsRoot);
  void SetSuppressScrollbarUpdate(bool aSuppress) {
    mHelper.mSupppressScrollbarUpdate = aSuppress;
  }
  bool GuessHScrollbarNeeded(const ScrollReflowState& aState);
  bool GuessVScrollbarNeeded(const ScrollReflowState& aState);

  bool IsScrollbarUpdateSuppressed() const {
    return mHelper.mSupppressScrollbarUpdate;
  }

  
  
  bool InInitialReflow() const;
  
  




  virtual bool ShouldPropagateComputedHeightToScrolledContent() const { return true; }

private:
  friend class mozilla::ScrollFrameHelper;
  ScrollFrameHelper mHelper;
};










class nsXULScrollFrame MOZ_FINAL : public nsBoxFrame,
                                   public nsIScrollableFrame,
                                   public nsIAnonymousContentCreator,
                                   public nsIStatefulFrame {
public:
  typedef mozilla::ScrollFrameHelper ScrollFrameHelper;
  typedef mozilla::CSSIntPoint CSSIntPoint;

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsXULScrollFrame* NS_NewXULScrollFrame(nsIPresShell* aPresShell,
                                                nsStyleContext* aContext,
                                                bool aIsRoot,
                                                bool aClipAllDescendants);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {
    mHelper.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  
#if 0
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
#endif

  virtual bool UpdateOverflow() MOZ_OVERRIDE {
    return mHelper.UpdateOverflow();
  }

  
  
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;


  virtual nsIScrollableFrame* GetScrollTargetFrame() MOZ_OVERRIDE {
    return this;
  }

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    return mHelper.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual bool DoesClipChildren() MOZ_OVERRIDE { return true; }
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild) MOZ_OVERRIDE
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mHelper.GetScrolledFrame())
      pt += mHelper.GetLogicalScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<ContentInfo>& aElements) MOZ_OVERRIDE;
  virtual void AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                        uint32_t aFilter) MOZ_OVERRIDE;

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsresult GetPadding(nsMargin& aPadding) MOZ_OVERRIDE;

  virtual bool GetBorderRadii(const nsSize& aFrameSize, const nsSize& aBorderArea,
                              Sides aSkipSides, nscoord aRadii[8]) const MOZ_OVERRIDE {
    return mHelper.GetBorderRadii(aFrameSize, aBorderArea, aSkipSides, aRadii);
  }

  nsresult Layout(nsBoxLayoutState& aState);
  void LayoutScrollArea(nsBoxLayoutState& aState, const nsPoint& aScrollPosition);

  static bool AddRemoveScrollbar(bool& aHasScrollbar, 
                                   nscoord& aXY, 
                                   nscoord& aSize, 
                                   nscoord aSbSize, 
                                   bool aOnRightOrBottom, 
                                   bool aAdd);
  
  bool AddRemoveScrollbar(nsBoxLayoutState& aState, 
                            bool aOnRightOrBottom, 
                            bool aHorizontal, 
                            bool aAdd);
  
  bool AddHorizontalScrollbar (nsBoxLayoutState& aState, bool aOnBottom);
  bool AddVerticalScrollbar   (nsBoxLayoutState& aState, bool aOnRight);
  void RemoveHorizontalScrollbar(nsBoxLayoutState& aState, bool aOnBottom);
  void RemoveVerticalScrollbar  (nsBoxLayoutState& aState, bool aOnRight);

  static void AdjustReflowStateForPrintPreview(nsBoxLayoutState& aState, bool& aSetBack);
  static void AdjustReflowStateBack(nsBoxLayoutState& aState, bool aSetBack);

  
  virtual nsIFrame* GetScrolledFrame() const MOZ_OVERRIDE {
    return mHelper.GetScrolledFrame();
  }
  virtual mozilla::ScrollbarStyles GetScrollbarStyles() const MOZ_OVERRIDE {
    return mHelper.GetScrollbarStylesFromFrame();
  }
  virtual uint32_t GetScrollbarVisibility() const MOZ_OVERRIDE {
    return mHelper.GetScrollbarVisibility();
  }
  virtual nsMargin GetActualScrollbarSizes() const MOZ_OVERRIDE {
    return mHelper.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) MOZ_OVERRIDE {
    return mHelper.GetDesiredScrollbarSizes(aState);
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
          nsRenderingContext* aRC) MOZ_OVERRIDE {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return GetDesiredScrollbarSizes(&bls);
  }
  virtual nscoord GetNondisappearingScrollbarWidth(nsPresContext* aPresContext,
          nsRenderingContext* aRC) MOZ_OVERRIDE {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return mHelper.GetNondisappearingScrollbarWidth(&bls);
  }
  virtual nsRect GetScrolledRect() const MOZ_OVERRIDE {
    return mHelper.GetScrolledRect();
  }
  virtual nsRect GetScrollPortRect() const MOZ_OVERRIDE {
    return mHelper.GetScrollPortRect();
  }
  virtual nsPoint GetScrollPosition() const MOZ_OVERRIDE {
    return mHelper.GetScrollPosition();
  }
  virtual nsPoint GetLogicalScrollPosition() const MOZ_OVERRIDE {
    return mHelper.GetLogicalScrollPosition();
  }
  virtual nsRect GetScrollRange() const MOZ_OVERRIDE {
    return mHelper.GetScrollRange();
  }
  virtual nsSize GetScrollPositionClampingScrollPortSize() const MOZ_OVERRIDE {
    return mHelper.GetScrollPositionClampingScrollPortSize();
  }
  virtual gfxSize GetResolution() const MOZ_OVERRIDE {
    return mHelper.GetResolution();
  }
  virtual void SetResolution(const gfxSize& aResolution) MOZ_OVERRIDE {
    return mHelper.SetResolution(aResolution);
  }
  virtual nsSize GetLineScrollAmount() const MOZ_OVERRIDE {
    return mHelper.GetLineScrollAmount();
  }
  virtual nsSize GetPageScrollAmount() const MOZ_OVERRIDE {
    return mHelper.GetPageScrollAmount();
  }
  


  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode,
                        const nsRect* aRange = nullptr) MOZ_OVERRIDE {
    mHelper.ScrollTo(aScrollPosition, aMode, aRange);
  }
  


  virtual void ScrollToCSSPixels(const CSSIntPoint& aScrollPosition,
                                 nsIScrollableFrame::ScrollMode aMode
                                   = nsIScrollableFrame::INSTANT) MOZ_OVERRIDE {
    mHelper.ScrollToCSSPixels(aScrollPosition, aMode);
  }
  virtual void ScrollToCSSPixelsApproximate(const mozilla::CSSPoint& aScrollPosition,
                                            nsIAtom* aOrigin = nullptr) MOZ_OVERRIDE {
    mHelper.ScrollToCSSPixelsApproximate(aScrollPosition, aOrigin);
  }
  virtual CSSIntPoint GetScrollPositionCSSPixels() MOZ_OVERRIDE {
    return mHelper.GetScrollPositionCSSPixels();
  }
  


  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow, nsIAtom* aOrigin = nullptr,
                        bool aIsMomentum = false) MOZ_OVERRIDE {
    mHelper.ScrollBy(aDelta, aUnit, aMode, aOverflow, aOrigin, aIsMomentum);
  }
  


  virtual void ScrollToRestoredPosition() MOZ_OVERRIDE {
    mHelper.ScrollToRestoredPosition();
  }
  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) MOZ_OVERRIDE {
    mHelper.AddScrollPositionListener(aListener);
  }
  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) MOZ_OVERRIDE {
    mHelper.RemoveScrollPositionListener(aListener);
  }
  


  virtual void CurPosAttributeChanged(nsIContent* aChild) MOZ_OVERRIDE {
    mHelper.CurPosAttributeChanged(aChild);
  }
  NS_IMETHOD PostScrolledAreaEventForCurrentArea() MOZ_OVERRIDE {
    mHelper.PostScrolledAreaEvent();
    return NS_OK;
  }
  virtual bool IsScrollingActive() MOZ_OVERRIDE {
    return mHelper.IsScrollingActive();
  }
  virtual bool IsProcessingAsyncScroll() MOZ_OVERRIDE {
    return mHelper.IsProcessingAsyncScroll();
  }
  virtual void ResetScrollPositionForLayerPixelAlignment() MOZ_OVERRIDE {
    mHelper.ResetScrollPositionForLayerPixelAlignment();
  }
  virtual bool IsResolutionSet() const MOZ_OVERRIDE {
    return mHelper.mIsResolutionSet;
  }
  virtual bool DidHistoryRestore() const MOZ_OVERRIDE {
    return mHelper.mDidHistoryRestore;
  }
  virtual void ClearDidHistoryRestore() MOZ_OVERRIDE {
    mHelper.mDidHistoryRestore = false;
  }
  virtual bool IsRectNearlyVisible(const nsRect& aRect) MOZ_OVERRIDE {
    return mHelper.IsRectNearlyVisible(aRect);
  }
  virtual nsRect ExpandRectToNearlyVisible(const nsRect& aRect) const MOZ_OVERRIDE {
    return mHelper.ExpandRectToNearlyVisible(aRect);
  }
  virtual nsIAtom* OriginOfLastScroll() MOZ_OVERRIDE {
    return mHelper.OriginOfLastScroll();
  }
  virtual uint32_t CurrentScrollGeneration() MOZ_OVERRIDE {
    return mHelper.CurrentScrollGeneration();
  }
  virtual void ResetOriginIfScrollAtGeneration(uint32_t aGeneration) MOZ_OVERRIDE {
    mHelper.ResetOriginIfScrollAtGeneration(aGeneration);
  }
  virtual bool WantAsyncScroll() const MOZ_OVERRIDE {
    return mHelper.WantAsyncScroll();
  }
  virtual void ComputeFrameMetrics(Layer* aLayer, nsIFrame* aContainerReferenceFrame,
                                   const ContainerLayerParameters& aParameters,
                                   nsRect* aClipRect,
                                   nsTArray<FrameMetrics>* aOutput) const MOZ_OVERRIDE {
    mHelper.ComputeFrameMetrics(aLayer, aContainerReferenceFrame,
                                aParameters, aClipRect, aOutput);
  }

  
  NS_IMETHOD SaveState(nsPresState** aState) MOZ_OVERRIDE {
    NS_ENSURE_ARG_POINTER(aState);
    *aState = mHelper.SaveState();
    return NS_OK;
  }
  NS_IMETHOD RestoreState(nsPresState* aState) MOZ_OVERRIDE {
    NS_ENSURE_ARG_POINTER(aState);
    mHelper.RestoreState(aState);
    return NS_OK;
  }

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return false;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

  virtual void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByPage(aScrollbar, aDirection);
  }
  virtual void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByWhole(aScrollbar, aDirection);
  }
  virtual void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE {
    mHelper.ScrollByLine(aScrollbar, aDirection);
  }
  virtual void RepeatButtonScroll(nsScrollbarFrame* aScrollbar) MOZ_OVERRIDE {
    mHelper.RepeatButtonScroll(aScrollbar);
  }
  virtual void ThumbMoved(nsScrollbarFrame* aScrollbar,
                          nscoord aOldPos,
                          nscoord aNewPos) MOZ_OVERRIDE {
    mHelper.ThumbMoved(aScrollbar, aOldPos, aNewPos);
  }
  virtual void VisibilityChanged(bool aVisible) {}
  virtual nsIFrame* GetScrollbarBox(bool aVertical) MOZ_OVERRIDE {
    return mHelper.GetScrollbarBox(aVertical);
  }

  virtual void ScrollbarActivityStarted() const MOZ_OVERRIDE;
  virtual void ScrollbarActivityStopped() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  nsXULScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, bool aIsRoot,
                   bool aClipAllDescendants);

  void ClampAndSetBounds(nsBoxLayoutState& aState, 
                         nsRect& aRect,
                         nsPoint aScrollPosition,
                         bool aRemoveOverflowAreas = false) {
    



    if (!mHelper.IsLTR()) {
      aRect.x = mHelper.mScrollPort.XMost() - aScrollPosition.x - aRect.width;
    }
    mHelper.mScrolledFrame->SetBounds(aState, aRect, aRemoveOverflowAreas);
  }

private:
  friend class mozilla::ScrollFrameHelper;
  ScrollFrameHelper mHelper;
};

#endif 
