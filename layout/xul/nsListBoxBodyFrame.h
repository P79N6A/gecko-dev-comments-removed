




#ifndef nsListBoxBodyFrame_h
#define nsListBoxBodyFrame_h

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsBoxFrame.h"
#include "nsIListBoxObject.h"
#include "nsIScrollbarMediator.h"
#include "nsIReflowCallback.h"
#include "nsBoxLayoutState.h"
#include "nsThreadUtils.h"
#include "nsPIBoxObject.h"

class nsPresContext;
class nsListScrollSmoother;
nsIFrame* NS_NewListBoxBodyFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);

class nsListBoxBodyFrame MOZ_FINAL : public nsBoxFrame,
                                     public nsIScrollbarMediator,
                                     public nsIReflowCallback
{
  nsListBoxBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                     nsBoxLayout* aLayoutManager);
  virtual ~nsListBoxBodyFrame();

public:
  NS_DECL_QUERYFRAME_TARGET(nsListBoxBodyFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  nsresult GetRowCount(int32_t *aResult);
  nsresult GetNumberOfVisibleRows(int32_t *aResult);
  nsresult GetIndexOfFirstVisibleRow(int32_t *aResult);
  nsresult EnsureIndexIsVisible(int32_t aRowIndex);
  nsresult ScrollToIndex(int32_t aRowIndex);
  nsresult ScrollByLines(int32_t aNumLines);
  nsresult GetItemAtIndex(int32_t aIndex, nsIDOMElement **aResult);
  nsresult GetIndexOfItem(nsIDOMElement *aItem, int32_t *aResult);

  friend nsIFrame* NS_NewListBoxBodyFrame(nsIPresShell* aPresShell,
                                          nsStyleContext* aContext);
  
  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent, 
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID, nsIAtom* aAttribute, int32_t aModType) MOZ_OVERRIDE;

  
  virtual void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void RepeatButtonScroll(nsScrollbarFrame* aScrollbar) MOZ_OVERRIDE;
  virtual void ThumbMoved(nsScrollbarFrame* aScrollbar,
                          int32_t aOldPos,
                          int32_t aNewPos) MOZ_OVERRIDE;
  virtual void VisibilityChanged(bool aVisible) MOZ_OVERRIDE;
  virtual nsIFrame* GetScrollbarBox(bool aVertical) MOZ_OVERRIDE;
  virtual void ScrollbarActivityStarted() const MOZ_OVERRIDE {}
  virtual void ScrollbarActivityStopped() const MOZ_OVERRIDE {}


  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual void MarkIntrinsicISizesDirty() MOZ_OVERRIDE;

  virtual nsSize GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetPrefSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  
  int32_t GetRowCount();
  int32_t GetRowHeightAppUnits() { return mRowHeight; }
  int32_t GetFixedRowSize();
  void SetRowHeight(nscoord aRowHeight);
  nscoord GetYPosition();
  nscoord GetAvailableHeight();
  nscoord ComputeIntrinsicISize(nsBoxLayoutState& aBoxLayoutState);

  
  nsresult InternalPositionChangedCallback();
  nsresult InternalPositionChanged(bool aUp, int32_t aDelta);
  
  
  nsresult DoInternalPositionChangedSync(bool aUp, int32_t aDelta);
  
  nsresult DoInternalPositionChanged(bool aUp, int32_t aDelta);
  nsListScrollSmoother* GetSmoother();
  void VerticalScroll(int32_t aDelta);
  void UpdateIndex(int32_t aDirection);

  
  nsIFrame* GetFirstFrame();
  nsIFrame* GetLastFrame();

  
  void CreateRows();
  void DestroyRows(int32_t& aRowsToLose);
  void ReverseDestroyRows(int32_t& aRowsToLose);
  nsIFrame* GetFirstItemBox(int32_t aOffset, bool* aCreated);
  nsIFrame* GetNextItemBox(nsIFrame* aBox, int32_t aOffset, bool* aCreated);
  bool ContinueReflow(nscoord height);
  NS_IMETHOD ListBoxAppendFrames(nsFrameList& aFrameList);
  NS_IMETHOD ListBoxInsertFrames(nsIFrame* aPrevFrame, nsFrameList& aFrameList);
  void OnContentInserted(nsPresContext* aPresContext, nsIContent* aContent);
  void OnContentRemoved(nsPresContext* aPresContext,  nsIContent* aContainer,
                        nsIFrame* aChildFrame, nsIContent* aOldNextSibling);

  void GetListItemContentAt(int32_t aIndex, nsIContent** aContent);
  void GetListItemNextSibling(nsIContent* aListItem, nsIContent** aContent, int32_t& aSiblingIndex);

  void PostReflowCallback();

  bool SetBoxObject(nsPIBoxObject* aBoxObject)
  {
    NS_ENSURE_TRUE(!mBoxObject, false);
    mBoxObject = aBoxObject;
    return true;
  }

  virtual bool SupportsOrdinalsInChildren() MOZ_OVERRIDE;

  virtual bool ComputesOwnOverflowArea() MOZ_OVERRIDE { return true; }

protected:
  class nsPositionChangedEvent;
  friend class nsPositionChangedEvent;

  class nsPositionChangedEvent : public nsRunnable
  {
  public:
    nsPositionChangedEvent(nsListBoxBodyFrame* aFrame,
                           bool aUp, int32_t aDelta) :
      mFrame(aFrame), mUp(aUp), mDelta(aDelta)
    {}
  
    NS_IMETHOD Run() MOZ_OVERRIDE
    {
      if (!mFrame) {
        return NS_OK;
      }

      mFrame->mPendingPositionChangeEvents.RemoveElement(this);

      return mFrame->DoInternalPositionChanged(mUp, mDelta);
    }

    void Revoke() {
      mFrame = nullptr;
    }

    nsListBoxBodyFrame* mFrame;
    bool mUp;
    int32_t mDelta;
  };

  void ComputeTotalRowCount();
  void RemoveChildFrame(nsBoxLayoutState &aState, nsIFrame *aChild);

  nsTArray< nsRefPtr<nsPositionChangedEvent> > mPendingPositionChangeEvents;
  nsCOMPtr<nsPIBoxObject> mBoxObject;

  
  nsWeakFrame mTopFrame;
  nsIFrame* mBottomFrame;
  nsIFrame* mLinkupFrame;

  nsListScrollSmoother* mScrollSmoother;

  int32_t mRowsToPrepend;

  
  int32_t mRowCount;
  nscoord mRowHeight;
  nscoord mAvailableHeight;
  nscoord mStringWidth;

  
  int32_t mCurrentIndex; 
  int32_t mOldIndex; 
  int32_t mYPosition;
  int32_t mTimePerRow;

  
  bool mRowHeightWasSet;
  
  bool mScrolling;
  bool mAdjustScroll;

  bool mReflowCallbackPosted;
};

#endif 
