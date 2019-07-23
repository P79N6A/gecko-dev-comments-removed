







































#ifndef nsListBoxBodyFrame_h
#define nsListBoxBodyFrame_h

#include "nsCOMPtr.h"
#include "nsBoxFrame.h"
#include "nsIListBoxObject.h"
#include "nsIScrollbarMediator.h"
#include "nsIReflowCallback.h"
#include "nsPresContext.h"
#include "nsBoxLayoutState.h"
#include "nsThreadUtils.h"
#include "nsPIBoxObject.h"

class nsListScrollSmoother;
nsIFrame* NS_NewListBoxBodyFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);

class nsListBoxBodyFrame : public nsBoxFrame,
                           public nsIScrollbarMediator,
                           public nsIReflowCallback
{
  nsListBoxBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     nsIBoxLayout* aLayoutManager);
  virtual ~nsListBoxBodyFrame();

public:
  NS_DECLARE_FRAME_ACCESSOR(nsListBoxBodyFrame)

  NS_DECL_QUERYFRAME

  
  nsresult GetRowCount(PRInt32 *aResult);
  nsresult GetNumberOfVisibleRows(PRInt32 *aResult);
  nsresult GetIndexOfFirstVisibleRow(PRInt32 *aResult);
  nsresult EnsureIndexIsVisible(PRInt32 aRowIndex);
  nsresult ScrollToIndex(PRInt32 aRowIndex);
  nsresult ScrollByLines(PRInt32 aNumLines);
  nsresult GetItemAtIndex(PRInt32 aIndex, nsIDOMElement **aResult);
  nsresult GetIndexOfItem(nsIDOMElement *aItem, PRInt32 *aResult);

  friend nsIFrame* NS_NewListBoxBodyFrame(nsIPresShell* aPresShell,
                                          nsStyleContext* aContext);
  
  
  NS_IMETHOD Init(nsIContent*     aContent,
                  nsIFrame*       aParent, 
                  nsIFrame*       aPrevInFlow);
  virtual void Destroy();

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID, nsIAtom* aAttribute, PRInt32 aModType);

  
  NS_IMETHOD PositionChanged(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex);
  NS_IMETHOD ScrollbarButtonPressed(nsIScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex);
  NS_IMETHOD VisibilityChanged(PRBool aVisible);

  
  virtual PRBool ReflowFinished();
  virtual void ReflowCallbackCanceled();

  
  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);
  virtual void MarkIntrinsicWidthsDirty();

  virtual nsSize GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState);

  
  PRInt32 GetRowCount();
  PRInt32 GetRowHeightAppUnits() { return mRowHeight; }
  PRInt32 GetFixedRowSize();
  void SetRowHeight(PRInt32 aRowHeight);
  nscoord GetYPosition();
  nscoord GetAvailableHeight();
  nscoord ComputeIntrinsicWidth(nsBoxLayoutState& aBoxLayoutState);

  
  nsresult InternalPositionChangedCallback();
  nsresult InternalPositionChanged(PRBool aUp, PRInt32 aDelta);
  
  
  nsresult DoInternalPositionChangedSync(PRBool aUp, PRInt32 aDelta);
  
  nsresult DoInternalPositionChanged(PRBool aUp, PRInt32 aDelta);
  nsListScrollSmoother* GetSmoother();
  void VerticalScroll(PRInt32 aDelta);

  
  nsIFrame* GetFirstFrame();
  nsIFrame* GetLastFrame();

  
  void CreateRows();
  void DestroyRows(PRInt32& aRowsToLose);
  void ReverseDestroyRows(PRInt32& aRowsToLose);
  nsIBox* GetFirstItemBox(PRInt32 aOffset, PRBool* aCreated);
  nsIBox* GetNextItemBox(nsIBox* aBox, PRInt32 aOffset, PRBool* aCreated);
  PRBool ContinueReflow(nscoord height);
  NS_IMETHOD ListBoxAppendFrames(nsFrameList& aFrameList);
  NS_IMETHOD ListBoxInsertFrames(nsIFrame* aPrevFrame, nsFrameList& aFrameList);
  void OnContentInserted(nsPresContext* aPresContext, nsIContent* aContent);
  void OnContentRemoved(nsPresContext* aPresContext,  nsIContent* aContainer,
                        nsIFrame* aChildFrame, PRInt32 aIndex);

  void GetListItemContentAt(PRInt32 aIndex, nsIContent** aContent);
  void GetListItemNextSibling(nsIContent* aListItem, nsIContent** aContent, PRInt32& aSiblingIndex);

  void PostReflowCallback();

  PRBool SetBoxObject(nsPIBoxObject* aBoxObject)
  {
    NS_ENSURE_TRUE(!mBoxObject, PR_FALSE);
    mBoxObject = aBoxObject;
    return PR_TRUE;
  }

  virtual PRBool SupportsOrdinalsInChildren();

protected:
  class nsPositionChangedEvent;
  friend class nsPositionChangedEvent;

  class nsPositionChangedEvent : public nsRunnable
  {
  public:
    nsPositionChangedEvent(nsListBoxBodyFrame* aFrame,
                           PRBool aUp, PRInt32 aDelta) :
      mFrame(aFrame), mUp(aUp), mDelta(aDelta)
    {}
  
    NS_IMETHOD Run()
    {
      if (!mFrame) {
        return NS_OK;
      }

      mFrame->mPendingPositionChangeEvents.RemoveElement(this);

      return mFrame->DoInternalPositionChanged(mUp, mDelta);
    }

    void Revoke() {
      mFrame = nsnull;
    }

    nsListBoxBodyFrame* mFrame;
    PRBool mUp;
    PRInt32 mDelta;
  };

  void ComputeTotalRowCount();
  void RemoveChildFrame(nsBoxLayoutState &aState, nsIFrame *aChild);

  nsTArray< nsRefPtr<nsPositionChangedEvent> > mPendingPositionChangeEvents;
  nsCOMPtr<nsPIBoxObject> mBoxObject;

  
  nsWeakFrame mTopFrame;
  nsIFrame* mBottomFrame;
  nsIFrame* mLinkupFrame;

  nsListScrollSmoother* mScrollSmoother;

  PRInt32 mRowsToPrepend;

  
  PRInt32 mRowCount;
  nscoord mRowHeight;
  nscoord mAvailableHeight;
  nscoord mStringWidth;

  
  PRInt32 mCurrentIndex; 
  PRInt32 mOldIndex; 
  PRInt32 mYPosition;
  PRInt32 mTimePerRow;

  
  PRPackedBool mRowHeightWasSet;
  
  PRPackedBool mScrolling;
  PRPackedBool mAdjustScroll;

  PRPackedBool mReflowCallbackPosted;
};

#endif 
