






































#ifndef nsGfxScrollFrame_h___
#define nsGfxScrollFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsBoxFrame.h"
#include "nsDisplayList.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollPositionListener.h"
#include "nsIStatefulFrame.h"
#include "nsThreadUtils.h"
#include "nsIReflowCallback.h"
#include "nsBoxLayoutState.h"
#include "nsQueryFrame.h"
#include "nsCOMArray.h"
#ifdef MOZ_SVG
#include "nsSVGIntegrationUtils.h"
#endif
#include "nsExpirationTracker.h"

class nsPresContext;
class nsIPresShell;
class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScrollFrameInternal;
class nsPresState;
struct ScrollReflowState;

class nsGfxScrollFrameInner : public nsIReflowCallback {
public:
  class AsyncScroll;

  nsGfxScrollFrameInner(nsContainerFrame* aOuter, PRBool aIsRoot);
  ~nsGfxScrollFrameInner();

  typedef nsIScrollableFrame::ScrollbarStyles ScrollbarStyles;
  ScrollbarStyles GetScrollbarStylesFromFrame() const;

  
  
  
  void ReloadChildFrames();

  nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  void AppendAnonymousContentTo(nsBaseContentList& aElements, PRUint32 aFilter);
  nsresult FireScrollPortEvent();
  void PostOverflowEvent();
  void Destroy();

  PRBool ShouldBuildLayer() const;

  nsresult BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                            const nsRect&           aDirtyRect,
                            const nsDisplayListSet& aLists);

  nsresult AppendScrollPartsTo(nsDisplayListBuilder*          aBuilder,
                               const nsRect&                  aDirtyRect,
                               const nsDisplayListSet&        aLists,
                               const nsDisplayListCollection& aDest,
                               PRBool&                        aCreateLayer);

  PRBool GetBorderRadii(nscoord aRadii[8]) const;

  
  virtual PRBool ReflowFinished();
  virtual void ReflowCallbackCanceled();

  
  void CurPosAttributeChanged(nsIContent* aChild);
  void PostScrollEvent();
  void FireScrollEvent();
  void PostScrolledAreaEvent();
  void FireScrolledAreaEvent();

  class ScrollEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    ScrollEvent(nsGfxScrollFrameInner *inner) : mInner(inner) {}
    void Revoke() { mInner = nsnull; }
  private:
    nsGfxScrollFrameInner *mInner;
  };

  class AsyncScrollPortEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    AsyncScrollPortEvent(nsGfxScrollFrameInner *inner) : mInner(inner) {}
    void Revoke() { mInner = nsnull; }
  private:
    nsGfxScrollFrameInner *mInner;
  };

  class ScrolledAreaEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    ScrolledAreaEvent(nsGfxScrollFrameInner *inner) : mInner(inner) {}
    void Revoke() { mInner = nsnull; }
  private:
    nsGfxScrollFrameInner *mInner;
  };

  static void FinishReflowForScrollbar(nsIContent* aContent, nscoord aMinXY,
                                       nscoord aMaxXY, nscoord aCurPosXY,
                                       nscoord aPageIncrement,
                                       nscoord aIncrement);
  static void SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos);
  static void SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom,
                                nscoord aSize);
  nscoord GetCoordAttribute(nsIBox* aFrame, nsIAtom* atom, nscoord defaultValue);

  
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

  nsPoint ClampAndRestrictToDevPixels(const nsPoint& aPt, nsIntPoint* aPtDevPx) const;
  nsPoint ClampScrollPosition(const nsPoint& aPt) const;
  static void AsyncScrollCallback(nsITimer *aTimer, void* anInstance);
  void ScrollTo(nsPoint aScrollPosition, nsIScrollableFrame::ScrollMode aMode);
  void ScrollToImpl(nsPoint aScrollPosition);
  void ScrollVisual();
  void ScrollBy(nsIntPoint aDelta, nsIScrollableFrame::ScrollUnit aUnit,
                nsIScrollableFrame::ScrollMode aMode, nsIntPoint* aOverflow);
  void ScrollToRestoredPosition();
  nsSize GetLineScrollAmount() const;
  nsSize GetPageScrollAmount() const;

  nsPresState* SaveState(nsIStatefulFrame::SpecialStateID aStateID);
  void RestoreState(nsPresState* aState);

  nsIFrame* GetScrolledFrame() const { return mScrolledFrame; }
  nsIBox* GetScrollbarBox(PRBool aVertical) const {
    return aVertical ? mVScrollbarBox : mHScrollbarBox;
  }

  void AddScrollPositionListener(nsIScrollPositionListener* aListener) {
    mListeners.AppendElement(aListener);
  }
  void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) {
    mListeners.RemoveElement(aListener);
  }

  static void SetScrollbarVisibility(nsIBox* aScrollbar, PRBool aVisible);

  













  nsRect GetScrolledRect() const;

  










  nsRect GetScrolledRectInternal(const nsRect& aScrolledOverflowArea,
                                 const nsSize& aScrollPortSize) const;

  PRUint32 GetScrollbarVisibility() const {
    return (mHasVerticalScrollbar ? nsIScrollableFrame::VERTICAL : 0) |
           (mHasHorizontalScrollbar ? nsIScrollableFrame::HORIZONTAL : 0);
  }
  nsMargin GetActualScrollbarSizes() const;
  nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);
  PRBool IsLTR() const;
  PRBool IsScrollbarOnRight() const;
  PRBool IsScrollingActive() const { return mScrollingActive || ShouldBuildLayer(); }
  
  
  
  void AdjustScrollbarRectForResizer(nsIFrame* aFrame, nsPresContext* aPresContext,
                                     nsRect& aRect, PRBool aHasResizer, PRBool aVertical);
  
  PRBool HasResizer() { return mResizerBox && !mCollapsedResizer; }
  void LayoutScrollbars(nsBoxLayoutState& aState,
                        const nsRect& aContentArea,
                        const nsRect& aOldScrollArea);

  PRBool IsAlwaysActive() const;
  void MarkActive();
  void MarkInactive();
  nsExpirationState* GetExpirationState() { return &mActivityExpirationState; }

  
  nsCOMPtr<nsIContent> mHScrollbarContent;
  nsCOMPtr<nsIContent> mVScrollbarContent;
  nsCOMPtr<nsIContent> mScrollCornerContent;
  nsCOMPtr<nsIContent> mResizerContent;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;
  nsRevocableEventPtr<AsyncScrollPortEvent> mAsyncScrollPortEvent;
  nsRevocableEventPtr<ScrolledAreaEvent> mScrolledAreaEvent;
  nsIBox* mHScrollbarBox;
  nsIBox* mVScrollbarBox;
  nsIFrame* mScrolledFrame;
  nsIBox* mScrollCornerBox;
  nsIBox* mResizerBox;
  nsContainerFrame* mOuter;
  AsyncScroll* mAsyncScroll;
  nsTArray<nsIScrollPositionListener*> mListeners;
  nsRect mScrollPort;
  
  
  
  
  nsPoint mDestination;
  nsPoint mScrollPosAtLastPaint;

  nsPoint mRestorePos;
  nsPoint mLastPos;

  nsExpirationState mActivityExpirationState;

  PRPackedBool mNeverHasVerticalScrollbar:1;
  PRPackedBool mNeverHasHorizontalScrollbar:1;
  PRPackedBool mHasVerticalScrollbar:1;
  PRPackedBool mHasHorizontalScrollbar:1;
  PRPackedBool mFrameIsUpdatingScrollbar:1;
  PRPackedBool mDidHistoryRestore:1;
  
  PRPackedBool mIsRoot:1;
  
  
  
  PRPackedBool mSupppressScrollbarUpdate:1;
  
  
  
  
  PRPackedBool mSkippedScrollbarLayout:1;

  PRPackedBool mHadNonInitialReflow:1;
  
  
  PRPackedBool mHorizontalOverflow:1;
  PRPackedBool mVerticalOverflow:1;
  PRPackedBool mPostedReflowCallback:1;
  PRPackedBool mMayHaveDirtyFixedChildren:1;
  
  
  PRPackedBool mUpdateScrollbarAttributes:1;
  
  
  PRPackedBool mScrollingActive:1;
  
  
  PRPackedBool mScrollbarsCanOverlapContent:1;
  
  PRPackedBool mCollapsedResizer:1;

#ifdef MOZ_IPC
  
  
  PRPackedBool mShouldBuildLayer:1;
#endif
};










class nsHTMLScrollFrame : public nsHTMLContainerFrame,
                          public nsIScrollableFrame,
                          public nsIAnonymousContentCreator,
                          public nsIStatefulFrame {
public:
  friend nsIFrame* NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return mInner.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  PRBool TryLayout(ScrollReflowState* aState,
                   nsHTMLReflowMetrics* aKidMetrics,
                   PRBool aAssumeVScroll, PRBool aAssumeHScroll,
                   PRBool aForce, nsresult* aResult);
  PRBool ScrolledContentDependsOnHeight(ScrollReflowState* aState);
  nsresult ReflowScrolledFrame(ScrollReflowState* aState,
                               PRBool aAssumeHScroll,
                               PRBool aAssumeVScroll,
                               nsHTMLReflowMetrics* aMetrics,
                               PRBool aFirstPass);
  nsresult ReflowContents(ScrollReflowState* aState,
                          const nsHTMLReflowMetrics& aDesiredSize);
  void PlaceScrollArea(const ScrollReflowState& aState,
                       const nsPoint& aScrollPosition);
  nscoord GetIntrinsicVScrollbarWidth(nsIRenderingContext *aRenderingContext);

  virtual PRBool GetBorderRadii(nscoord aRadii[8]) const {
    return mInner.GetBorderRadii(aRadii);
  }

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  NS_IMETHOD GetPadding(nsMargin& aPadding);
  virtual PRBool IsCollapsed(nsBoxLayoutState& aBoxLayoutState);
  
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);


  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    return this;
  }

  virtual nsIFrame* GetContentInsertionFrame() {
    return mInner.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

  virtual PRBool DoesClipChildren() { return PR_TRUE; }
  virtual nsSplittableType GetSplittableType() const;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mInner.GetScrolledFrame()) pt += GetScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  
  virtual nsIFrame* GetScrolledFrame() const {
    return mInner.GetScrolledFrame();
  }
  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const {
    return mInner.GetScrollbarStylesFromFrame();
  }
  virtual PRUint32 GetScrollbarVisibility() const {
    return mInner.GetScrollbarVisibility();
  }
  virtual nsMargin GetActualScrollbarSizes() const {
    return mInner.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) {
    return mInner.GetDesiredScrollbarSizes(aState);
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
          nsIRenderingContext* aRC) {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return GetDesiredScrollbarSizes(&bls);
  }
  virtual nsRect GetScrollPortRect() const {
    return mInner.GetScrollPortRect();
  }
  virtual nsPoint GetScrollPosition() const {
    return mInner.GetScrollPosition();
  }
  virtual nsRect GetScrollRange() const {
    return mInner.GetScrollRange();
  }
  virtual nsSize GetLineScrollAmount() const {
    return mInner.GetLineScrollAmount();
  }
  virtual nsSize GetPageScrollAmount() const {
    return mInner.GetPageScrollAmount();
  }
  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode) {
    mInner.ScrollTo(aScrollPosition, aMode);
  }
  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow) {
    mInner.ScrollBy(aDelta, aUnit, aMode, aOverflow);
  }
  virtual void ScrollToRestoredPosition() {
    mInner.ScrollToRestoredPosition();
  }
  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) {
    mInner.AddScrollPositionListener(aListener);
  }
  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) {
    mInner.RemoveScrollPositionListener(aListener);
  }
  virtual nsIBox* GetScrollbarBox(PRBool aVertical) {
    return mInner.GetScrollbarBox(aVertical);
  }
  virtual void CurPosAttributeChanged(nsIContent* aChild) {
    mInner.CurPosAttributeChanged(aChild);
  }
  NS_IMETHOD PostScrolledAreaEventForCurrentArea() {
    mInner.PostScrolledAreaEvent();
    return NS_OK;
  }
  virtual PRBool IsScrollingActive() {
    return mInner.IsScrollingActive();
  }

  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState) {
    NS_ENSURE_ARG_POINTER(aState);
    *aState = mInner.SaveState(aStateID);
    return NS_OK;
  }
  NS_IMETHOD RestoreState(nsPresState* aState) {
    NS_ENSURE_ARG_POINTER(aState);
    mInner.RestoreState(aState);
    return NS_OK;
  }

  




  virtual nsIAtom* GetType() const;
  
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  PRBool DidHistoryRestore() { return mInner.mDidHistoryRestore; }

#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

protected:
  nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot);
  virtual PRIntn GetSkipSides() const;
  
  void SetSuppressScrollbarUpdate(PRBool aSuppress) {
    mInner.mSupppressScrollbarUpdate = aSuppress;
  }
  PRBool GuessHScrollbarNeeded(const ScrollReflowState& aState);
  PRBool GuessVScrollbarNeeded(const ScrollReflowState& aState);

  PRBool IsScrollbarUpdateSuppressed() const {
    return mInner.mSupppressScrollbarUpdate;
  }

  
  
  PRBool InInitialReflow() const;
  
  




  virtual PRBool ShouldPropagateComputedHeightToScrolledContent() const { return PR_TRUE; }

private:
  friend class nsGfxScrollFrameInner;
  nsGfxScrollFrameInner mInner;
};










class nsXULScrollFrame : public nsBoxFrame,
                         public nsIScrollableFrame,
                         public nsIAnonymousContentCreator,
                         public nsIStatefulFrame {
public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

  
  
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return mInner.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  
#if 0
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
#endif

  
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIScrollableFrame* GetScrollTargetFrame() {
    return this;
  }

  virtual nsIFrame* GetContentInsertionFrame() {
    return mInner.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

  virtual PRBool DoesClipChildren() { return PR_TRUE; }
  virtual nsSplittableType GetSplittableType() const;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mInner.GetScrolledFrame())
      pt += mInner.GetLogicalScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  virtual void AppendAnonymousContentTo(nsBaseContentList& aElements,
                                        PRUint32 aFilter);

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD GetPadding(nsMargin& aPadding);

  virtual PRBool GetBorderRadii(nscoord aRadii[8]) const {
    return mInner.GetBorderRadii(aRadii);
  }

  nsresult Layout(nsBoxLayoutState& aState);
  void LayoutScrollArea(nsBoxLayoutState& aState, const nsPoint& aScrollPosition);

  static PRBool AddRemoveScrollbar(PRBool& aHasScrollbar, 
                                   nscoord& aXY, 
                                   nscoord& aSize, 
                                   nscoord aSbSize, 
                                   PRBool aOnRightOrBottom, 
                                   PRBool aAdd);
  
  PRBool AddRemoveScrollbar(nsBoxLayoutState& aState, 
                            PRBool aOnRightOrBottom, 
                            PRBool aHorizontal, 
                            PRBool aAdd);
  
  PRBool AddHorizontalScrollbar (nsBoxLayoutState& aState, PRBool aOnBottom);
  PRBool AddVerticalScrollbar   (nsBoxLayoutState& aState, PRBool aOnRight);
  void RemoveHorizontalScrollbar(nsBoxLayoutState& aState, PRBool aOnBottom);
  void RemoveVerticalScrollbar  (nsBoxLayoutState& aState, PRBool aOnRight);

  static void AdjustReflowStateForPrintPreview(nsBoxLayoutState& aState, PRBool& aSetBack);
  static void AdjustReflowStateBack(nsBoxLayoutState& aState, PRBool aSetBack);

  
  virtual nsIFrame* GetScrolledFrame() const {
    return mInner.GetScrolledFrame();
  }
  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const {
    return mInner.GetScrollbarStylesFromFrame();
  }
  virtual PRUint32 GetScrollbarVisibility() const {
    return mInner.GetScrollbarVisibility();
  }
  virtual nsMargin GetActualScrollbarSizes() const {
    return mInner.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState) {
    return mInner.GetDesiredScrollbarSizes(aState);
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsPresContext* aPresContext,
          nsIRenderingContext* aRC) {
    nsBoxLayoutState bls(aPresContext, aRC, 0);
    return GetDesiredScrollbarSizes(&bls);
  }
  virtual nsRect GetScrollPortRect() const {
    return mInner.GetScrollPortRect();
  }
  virtual nsPoint GetScrollPosition() const {
    return mInner.GetScrollPosition();
  }
  virtual nsRect GetScrollRange() const {
    return mInner.GetScrollRange();
  }
  virtual nsSize GetLineScrollAmount() const {
    return mInner.GetLineScrollAmount();
  }
  virtual nsSize GetPageScrollAmount() const {
    return mInner.GetPageScrollAmount();
  }
  virtual void ScrollTo(nsPoint aScrollPosition, ScrollMode aMode) {
    mInner.ScrollTo(aScrollPosition, aMode);
  }
  virtual void ScrollBy(nsIntPoint aDelta, ScrollUnit aUnit, ScrollMode aMode,
                        nsIntPoint* aOverflow) {
    mInner.ScrollBy(aDelta, aUnit, aMode, aOverflow);
  }
  virtual void ScrollToRestoredPosition() {
    mInner.ScrollToRestoredPosition();
  }
  virtual void AddScrollPositionListener(nsIScrollPositionListener* aListener) {
    mInner.AddScrollPositionListener(aListener);
  }
  virtual void RemoveScrollPositionListener(nsIScrollPositionListener* aListener) {
    mInner.RemoveScrollPositionListener(aListener);
  }
  virtual nsIBox* GetScrollbarBox(PRBool aVertical) {
    return mInner.GetScrollbarBox(aVertical);
  }
  virtual void CurPosAttributeChanged(nsIContent* aChild) {
    mInner.CurPosAttributeChanged(aChild);
  }
  NS_IMETHOD PostScrolledAreaEventForCurrentArea() {
    mInner.PostScrolledAreaEvent();
    return NS_OK;
  }
  virtual PRBool IsScrollingActive() {
    return mInner.IsScrollingActive();
  }

  
  NS_IMETHOD SaveState(SpecialStateID aStateID, nsPresState** aState) {
    NS_ENSURE_ARG_POINTER(aState);
    *aState = mInner.SaveState(aStateID);
    return NS_OK;
  }
  NS_IMETHOD RestoreState(nsPresState* aState) {
    NS_ENSURE_ARG_POINTER(aState);
    mInner.RestoreState(aState);
    return NS_OK;
  }

  




  virtual nsIAtom* GetType() const;
  
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return PR_FALSE;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsXULScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot);
  virtual PRIntn GetSkipSides() const;

  void ClampAndSetBounds(nsBoxLayoutState& aState, 
                         nsRect& aRect,
                         nsPoint aScrollPosition,
                         PRBool aRemoveOverflowAreas = PR_FALSE) {
    





    if (!mInner.IsLTR()) {
      aRect.x = PresContext()->RoundAppUnitsToNearestDevPixels(
         mInner.mScrollPort.XMost() - aScrollPosition.x - aRect.width);
    }
    mInner.mScrolledFrame->SetBounds(aState, aRect, aRemoveOverflowAreas);
  }

private:
  friend class nsGfxScrollFrameInner;
  nsGfxScrollFrameInner mInner;
};

#endif 
