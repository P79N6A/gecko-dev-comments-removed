






































#ifndef nsGfxScrollFrame_h___
#define nsGfxScrollFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsIAnonymousContentCreator.h"
#include "nsBoxFrame.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollPositionListener.h"
#include "nsIStatefulFrame.h"
#include "nsGUIEvent.h"
#include "nsThreadUtils.h"
#include "nsIScrollableView.h"
#include "nsIView.h"
#include "nsIReflowCallback.h"

class nsPresContext;
class nsIPresShell;
class nsIContent;
class nsIAtom;
class nsIDocument;
class nsIScrollFrameInternal;
class nsPresState;
struct ScrollReflowState;

class nsGfxScrollFrameInner : public nsIScrollPositionListener,
                              public nsIReflowCallback {
public:
  NS_IMETHOD QueryInterface(REFNSIID aIID, void** aInstancePtr);
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  nsGfxScrollFrameInner(nsContainerFrame* aOuter, PRBool aIsRoot,
                        PRBool aIsXUL);
  ~nsGfxScrollFrameInner();

  typedef nsIScrollableFrame::ScrollbarStyles ScrollbarStyles;
  ScrollbarStyles GetScrollbarStylesFromFrame() const;

  
  
  
  void ReloadChildFrames();
  PRBool NeedsClipWidget() const;
  void CreateScrollableView();

  nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);
  nsresult FireScrollPortEvent();
  void PostOverflowEvent();
  void Destroy();

  nsresult BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                            const nsRect&           aDirtyRect,
                            const nsDisplayListSet& aLists);

  virtual void InvalidateInternal(const nsRect& aDamageRect, nscoord aX, nscoord aY,
                                  nsIFrame* aForChild, PRBool aImmediate);

  
  virtual PRBool ReflowFinished();

  

  NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);
  NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY);

  
  void CurPosAttributeChanged(nsIContent* aChild);
  void PostScrollEvent();
  void FireScrollEvent();

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

  static void FinishReflowForScrollbar(nsIContent* aContent, nscoord aMinXY,
                                       nscoord aMaxXY, nscoord aCurPosXY,
                                       nscoord aPageIncrement,
                                       nscoord aIncrement);
  static void SetScrollbarEnabled(nsIContent* aContent, nscoord aMaxPos);
  static void SetCoordAttribute(nsIContent* aContent, nsIAtom* aAtom,
                                nscoord aSize);
  nscoord GetCoordAttribute(nsIBox* aFrame, nsIAtom* atom, nscoord defaultValue);

  
  
  void InternalScrollPositionDidChange(nscoord aX, nscoord aY);

  nsIScrollableView* GetScrollableView() const { return mScrollableView; }

  nsIView* GetParentViewForChildFrame(nsIFrame* aFrame) const;

  void ScrollToRestoredPosition();

  nsPresState* SaveState(nsIStatefulFrame::SpecialStateID aStateID);
  void RestoreState(nsPresState* aState);
  void SaveVScrollbarStateToGlobalHistory();
  nsresult GetVScrollbarHintFromGlobalHistory(PRBool* aVScrollbarNeeded);

  nsIFrame* GetScrolledFrame() const { return mScrolledFrame; }

  void ScrollbarChanged(nsPresContext* aPresContext, nscoord aX, nscoord aY, PRUint32 aFlags);

  static void SetScrollbarVisibility(nsIBox* aScrollbar, PRBool aVisible);

  










  nsRect GetScrolledRect(const nsSize& aScrollPortSize) const;
  nsSize GetScrollPortSize() const
  {
    return mScrollableView->View()->GetBounds().Size();
  }

  nsMargin GetActualScrollbarSizes() const;
  nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);
  PRBool IsLTR() const;
  PRBool IsScrollbarOnRight() const;
  void LayoutScrollbars(nsBoxLayoutState& aState,
                        const nsRect& aContentArea,
                        const nsRect& aOldScrollArea,
                        const nsRect& aScrollArea);

  
  nsCOMPtr<nsIContent> mHScrollbarContent;
  nsCOMPtr<nsIContent> mVScrollbarContent;
  nsCOMPtr<nsIContent> mScrollCornerContent;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;
  nsRevocableEventPtr<AsyncScrollPortEvent> mAsyncScrollPortEvent;
  nsIScrollableView* mScrollableView;
  nsIBox* mHScrollbarBox;
  nsIBox* mVScrollbarBox;
  nsIFrame* mScrolledFrame;
  nsIBox* mScrollCornerBox;
  nsContainerFrame* mOuter;

  nsRect mRestoreRect;
  nsPoint mLastPos;

  PRPackedBool mNeverHasVerticalScrollbar:1;
  PRPackedBool mNeverHasHorizontalScrollbar:1;
  PRPackedBool mHasVerticalScrollbar:1;
  PRPackedBool mHasHorizontalScrollbar:1;
  PRPackedBool mViewInitiatedScroll:1;
  PRPackedBool mFrameInitiatedScroll:1;
  PRPackedBool mDidHistoryRestore:1;
  
  PRPackedBool mIsRoot:1;
  
  PRPackedBool mIsXUL:1;
  
  
  
  PRPackedBool mSupppressScrollbarUpdate:1;
  
  
  
  
  PRPackedBool mSkippedScrollbarLayout:1;
  
  
  PRPackedBool mDidLoadHistoryVScrollbarHint:1;
  
  PRPackedBool mHistoryVScrollbarHint:1;
  PRPackedBool mHadNonInitialReflow:1;
  
  
  PRPackedBool mHorizontalOverflow:1;
  PRPackedBool mVerticalOverflow:1;
  PRPackedBool mPostedReflowCallback:1;
};










class nsHTMLScrollFrame : public nsHTMLContainerFrame,
                          public nsIScrollableFrame,
                          public nsIAnonymousContentCreator,
                          public nsIStatefulFrame {
public:
  friend nsIFrame* NS_NewHTMLScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

  NS_DECL_ISUPPORTS

  
  
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return mInner.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  PRBool TryLayout(ScrollReflowState* aState,
                   const nsHTMLReflowMetrics& aKidMetrics,
                   PRBool aAssumeVScroll, PRBool aAssumeHScroll,
                   PRBool aForce);
  nsresult ReflowScrolledFrame(const ScrollReflowState& aState,
                               PRBool aAssumeHScroll,
                               PRBool aAssumeVScroll,
                               nsHTMLReflowMetrics* aMetrics,
                               PRBool aFirstPass);
  nsresult ReflowContents(ScrollReflowState* aState,
                          const nsHTMLReflowMetrics& aDesiredSize);
  void PlaceScrollArea(const ScrollReflowState& aState);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  NS_IMETHOD GetPadding(nsMargin& aPadding);
  virtual PRBool IsCollapsed(nsBoxLayoutState& aBoxLayoutState);
  
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);

  virtual void Destroy();

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIView* GetParentViewForChildFrame(nsIFrame* aFrame) const {
    return mInner.GetParentViewForChildFrame(aFrame);
  }

  virtual nsIFrame* GetContentInsertionFrame() {
    return mInner.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual nsIView* GetMouseCapturer() const {
    return mInner.GetScrolledFrame()->GetView();
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRBool aImmediate) {
    mInner.InvalidateInternal(aDamageRect, aX, aY, aForChild, aImmediate);
  }

  virtual PRBool NeedsView() { return PR_TRUE; }
  virtual PRBool DoesClipChildren() { return PR_TRUE; }
  virtual nsSplittableType GetSplittableType() const;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mInner.GetScrolledFrame()) pt += GetScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

  
  virtual nsIFrame* GetScrolledFrame() const;
  virtual nsIScrollableView* GetScrollableView();

  virtual nsPoint GetScrollPosition() const;
  virtual void ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags);

  virtual void SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible);

  virtual nsIBox* GetScrollbarBox(PRBool aVertical);

  virtual void CurPosAttributeChanged(nsIContent* aChild, PRInt32 aModType);

  
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

  virtual void ScrollToRestoredPosition() {
    mInner.ScrollToRestoredPosition();
  }

  virtual nsMargin GetActualScrollbarSizes() const {
    return mInner.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);
  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const;

  




  virtual nsIAtom* GetType() const;
  
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual nsresult GetContentOf(nsIContent** aContent);

  PRBool DidHistoryRestore() { return mInner.mDidHistoryRestore; }

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

protected:
  nsHTMLScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot);
  virtual PRIntn GetSkipSides() const;
  
  void SetSuppressScrollbarUpdate(PRBool aSuppress) {
    mInner.mSupppressScrollbarUpdate = aSuppress;
  }
  PRBool GuessVScrollbarNeeded(const ScrollReflowState& aState);
  nsSize GetScrollPortSize() const
  {
    return mInner.GetScrollPortSize();
  }

  PRBool IsScrollbarUpdateSuppressed() const {
    return mInner.mSupppressScrollbarUpdate;
  }
  
private:
  friend class nsGfxScrollFrameInner;
  nsGfxScrollFrameInner mInner;
};










class nsXULScrollFrame : public nsBoxFrame,
                         public nsIScrollableFrame,
                         public nsIAnonymousContentCreator,
                         public nsIStatefulFrame {
public:
  friend nsIFrame* NS_NewXULScrollFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot);

  
  
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return mInner.BuildDisplayList(aBuilder, aDirtyRect, aLists);
  }

  
#if 0
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
#endif

  
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);

  virtual void Destroy();

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIView* GetParentViewForChildFrame(nsIFrame* aFrame) const {
    return mInner.GetParentViewForChildFrame(aFrame);
  }

  virtual nsIFrame* GetContentInsertionFrame() {
    return mInner.GetScrolledFrame()->GetContentInsertionFrame();
  }

  virtual nsIView* GetMouseCapturer() const {
    return mInner.GetScrolledFrame()->GetView();
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRBool aImmediate) {
    mInner.InvalidateInternal(aDamageRect, aX, aY, aForChild, aImmediate);
  }

  virtual PRBool NeedsView() { return PR_TRUE; }
  virtual PRBool DoesClipChildren() { return PR_TRUE; }
  virtual nsSplittableType GetSplittableType() const;

  virtual nsPoint GetPositionOfChildIgnoringScrolling(nsIFrame* aChild)
  { nsPoint pt = aChild->GetPosition();
    if (aChild == mInner.GetScrolledFrame()) pt += GetScrollPosition();
    return pt;
  }

  
  virtual nsresult CreateAnonymousContent(nsTArray<nsIContent*>& aElements);

  
  NS_DECL_ISUPPORTS

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetBoxAscent(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  NS_IMETHOD GetPadding(nsMargin& aPadding);

  nsresult Layout(nsBoxLayoutState& aState);
  void LayoutScrollArea(nsBoxLayoutState& aState, const nsRect& aRect);

  static PRBool AddRemoveScrollbar(PRBool& aHasScrollbar, 
                                   nscoord& aXY, 
                                   nscoord& aSize, 
                                   nscoord aSbSize, 
                                   PRBool aOnRightOrBottom, 
                                   PRBool aAdd);
  
  PRBool AddRemoveScrollbar(nsBoxLayoutState& aState, 
                            nsRect& aScrollAreaSize, 
                            PRBool aOnTop, 
                            PRBool aHorizontal, 
                            PRBool aAdd);
  
  PRBool AddHorizontalScrollbar (nsBoxLayoutState& aState, nsRect& aScrollAreaSize, PRBool aOnBottom);
  PRBool AddVerticalScrollbar   (nsBoxLayoutState& aState, nsRect& aScrollAreaSize, PRBool aOnRight);
  void RemoveHorizontalScrollbar(nsBoxLayoutState& aState, nsRect& aScrollAreaSize, PRBool aOnBottom);
  void RemoveVerticalScrollbar  (nsBoxLayoutState& aState, nsRect& aScrollAreaSize, PRBool aOnRight);

  static void AdjustReflowStateForPrintPreview(nsBoxLayoutState& aState, PRBool& aSetBack);
  static void AdjustReflowStateBack(nsBoxLayoutState& aState, PRBool aSetBack);

  
  virtual nsIFrame* GetScrolledFrame() const;
  virtual nsIScrollableView* GetScrollableView();

  virtual nsPoint GetScrollPosition() const;
  virtual void ScrollTo(nsPoint aScrollPosition, PRUint32 aFlags);

  virtual void SetScrollbarVisibility(PRBool aVerticalVisible, PRBool aHorizontalVisible);

  virtual nsIBox* GetScrollbarBox(PRBool aVertical);

  virtual void CurPosAttributeChanged(nsIContent* aChild, PRInt32 aModType);

  
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

  virtual void ScrollToRestoredPosition() {
    mInner.ScrollToRestoredPosition();
  }

  virtual nsMargin GetActualScrollbarSizes() const {
    return mInner.GetActualScrollbarSizes();
  }
  virtual nsMargin GetDesiredScrollbarSizes(nsBoxLayoutState* aState);
  virtual nsGfxScrollFrameInner::ScrollbarStyles GetScrollbarStyles() const;

  




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

  virtual nsresult GetContentOf(nsIContent** aContent);

protected:
  nsXULScrollFrame(nsIPresShell* aShell, nsStyleContext* aContext, PRBool aIsRoot);
  virtual PRIntn GetSkipSides() const;

private:
  friend class nsGfxScrollFrameInner;
  nsGfxScrollFrameInner mInner;
};

#endif 
