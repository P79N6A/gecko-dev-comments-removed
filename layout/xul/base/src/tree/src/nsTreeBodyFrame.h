










































#include "nsLeafBoxFrame.h"
#include "nsITreeView.h"
#include "nsICSSPseudoComparator.h"
#include "nsIScrollbarMediator.h"
#include "nsIDragSession.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsILookAndFeel.h"
#include "nsValueArray.h"
#include "nsTreeStyleCache.h"
#include "nsTreeColumns.h"
#include "nsTreeImageListener.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "imgIRequest.h"
#include "imgIDecoderObserver.h"
#include "nsIScrollbarFrame.h"
#include "nsThreadUtils.h"


struct nsTreeImageCacheEntry
{
  nsTreeImageCacheEntry() {}
  nsTreeImageCacheEntry(imgIRequest *aRequest, imgIDecoderObserver *aListener)
    : request(aRequest), listener(aListener) {}

  nsCOMPtr<imgIRequest> request;
  nsCOMPtr<imgIDecoderObserver> listener;
};

static NS_DEFINE_CID(kTreeColumnImplCID, NS_TREECOLUMN_IMPL_CID);


class nsTreeBodyFrame : public nsLeafBoxFrame,
                        public nsITreeBoxObject,
                        public nsICSSPseudoComparator,
                        public nsIScrollbarMediator,
                        public nsIReflowCallback
{
public:
  nsTreeBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  virtual ~nsTreeBodyFrame();

  NS_DECL_ISUPPORTS
  NS_DECL_NSITREEBOXOBJECT

  
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         PRBool aRemoveOverflowArea = PR_FALSE);

  
  virtual PRBool ReflowFinished();

  
  NS_IMETHOD PseudoMatches(nsIAtom* aTag, nsCSSSelector* aSelector, PRBool* aResult);

  
  NS_IMETHOD PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex);
  NS_IMETHOD ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex);
  NS_IMETHOD VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible) { Invalidate(); return NS_OK; }

  
  NS_IMETHOD Init(nsIContent*     aContent,
                  nsIFrame*       aParent,
                  nsIFrame*       aPrevInFlow);
  virtual void Destroy();

  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD DidSetStyleContext();

  friend nsIFrame* NS_NewTreeBodyFrame(nsIPresShell* aPresShell);

  struct ScrollParts {
    nsIScrollbarFrame* mVScrollbar;
    nsIContent*        mVScrollbarContent;
    nsIScrollbarFrame* mHScrollbar;
    nsIContent*        mHScrollbarContent;
    nsIScrollableView* mColumnsScrollableView;
  };

  void PaintTreeBody(nsIRenderingContext& aRenderingContext,
                     const nsRect& aDirtyRect, nsPoint aPt);

  nsITreeBoxObject* GetTreeBoxObject() const { return mTreeBoxObject; }

protected:
  
  void PaintColumn(nsTreeColumn*        aColumn,
                   const nsRect&        aColumnRect,
                   nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);

  
  void PaintRow(PRInt32              aRowIndex,
                const nsRect&        aRowRect,
                nsPresContext*       aPresContext,
                nsIRenderingContext& aRenderingContext,
                const nsRect&        aDirtyRect,
                nsPoint              aPt);

  
  void PaintSeparator(PRInt32              aRowIndex,
                      const nsRect&        aSeparatorRect,
                      nsPresContext*      aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  
  void PaintCell(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aCellRect,
                 nsPresContext*       aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX,
                 nsPoint              aPt);

  
  void PaintTwisty(PRInt32              aRowIndex,
                   nsTreeColumn*        aColumn,
                   const nsRect&        aTwistyRect,
                   nsPresContext*      aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nscoord&             aRemainingWidth,
                   nscoord&             aCurrX);

  
  void PaintImage(PRInt32              aRowIndex,
                  nsTreeColumn*        aColumn,
                  const nsRect&        aImageRect,
                  nsPresContext*      aPresContext,
                  nsIRenderingContext& aRenderingContext,
                  const nsRect&        aDirtyRect,
                  nscoord&             aRemainingWidth,
                  nscoord&             aCurrX);

  
  void PaintText(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aTextRect,
                 nsPresContext*      aPresContext,
                 nsIRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX);

  
  void PaintCheckbox(PRInt32              aRowIndex, 
                     nsTreeColumn*        aColumn,
                     const nsRect&        aCheckboxRect,
                     nsPresContext*      aPresContext,
                     nsIRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect);

  
  void PaintProgressMeter(PRInt32              aRowIndex, 
                          nsTreeColumn*        aColumn,
                          const nsRect&        aProgressMeterRect,
                          nsPresContext*      aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect);

  
  void PaintDropFeedback(const nsRect&        aDropFeedbackRect, 
                         nsPresContext*      aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         const nsRect&        aDirtyRect,
                         nsPoint              aPt);

  
  
  void PaintBackgroundLayer(nsStyleContext*      aStyleContext,
                            nsPresContext*      aPresContext, 
                            nsIRenderingContext& aRenderingContext, 
                            const nsRect&        aRect,
                            const nsRect&        aDirtyRect);


  PRInt32 GetLastVisibleRow() {
    return mTopRowIndex + mPageLength;
  }

  
  
  PRInt32 GetRowAt(nscoord aX, nscoord aY);

  
  
  void CheckTextForBidi(nsAutoString& aText);

  void AdjustForCellText(nsAutoString& aText,
                         PRInt32 aRowIndex,  nsTreeColumn* aColumn,
                         nsIRenderingContext& aRenderingContext,
                         nsRect& aTextRect);

  
  nsIAtom* GetItemWithinCellAt(nscoord aX, const nsRect& aCellRect,
                               PRInt32 aRowIndex, nsTreeColumn* aColumn);

  
  
  void GetCellAt(nscoord aX, nscoord aY, PRInt32* aRow, nsTreeColumn** aCol,
                 nsIAtom** aChildElt);

  
  nsITheme* GetTwistyRect(PRInt32 aRowIndex,
                          nsTreeColumn* aColumn,
                          nsRect& aImageRect,
                          nsRect& aTwistyRect,
                          nsPresContext* aPresContext,
                          nsIRenderingContext& aRenderingContext,
                          nsStyleContext* aTwistyContext);

  
  nsresult GetImage(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext,
                    nsStyleContext* aStyleContext, PRBool& aAllowImageRegions, imgIContainer** aResult);

  
  
  nsRect GetImageSize(PRInt32 aRowIndex, nsTreeColumn* aCol, PRBool aUseContext, nsStyleContext* aStyleContext);

  
  nsSize GetImageDestSize(nsStyleContext* aStyleContext, PRBool useImageRegion, imgIContainer* image);

  
  nsRect GetImageSourceRect(nsStyleContext* aStyleContext, PRBool useImageRegion, imgIContainer* image);

  
  PRInt32 GetRowHeight();

  
  PRInt32 GetIndentation();

  
  void CalcInnerBox();

  
  nscoord CalcHorzWidth(const ScrollParts& aParts);

  
  
  nsStyleContext* GetPseudoStyleContext(nsIAtom* aPseudoElement);

  
  
  
  
  
  ScrollParts GetScrollParts();

  
  void UpdateScrollbars(const ScrollParts& aParts);

  
  void InvalidateScrollbars(const ScrollParts& aParts);

  
  void CheckOverflow(const ScrollParts& aParts);

  
  
  
  PRBool FullScrollbarsUpdate(PRBool aNeedsFullInvalidation);

  
  
  void PrefillPropertyArray(PRInt32 aRowIndex, nsTreeColumn* aCol);

  
  nsresult ScrollInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult ScrollToRowInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult ScrollToColumnInternal(const ScrollParts& aParts, nsITreeColumn* aCol);
  nsresult ScrollHorzInternal(const ScrollParts& aParts, PRInt32 aPosition);
  nsresult EnsureRowIsVisibleInternal(const ScrollParts& aParts, PRInt32 aRow);
  
  
  void AdjustClientCoordsToBoxCoordSpace(PRInt32 aX, PRInt32 aY,
                                         nscoord* aResultX, nscoord* aResultY);

  
  nsLineStyle ConvertBorderStyleToLineStyle(PRUint8 aBorderStyle);

  
  void EnsureBoxObject();

  void EnsureView();

  
  nsIContent* GetBaseElement();

  nsresult GetCellWidth(PRInt32 aRow, nsTreeColumn* aCol,
                        nsIRenderingContext* aRenderingContext,
                        nscoord& aDesiredSize, nscoord& aCurrentSize);
  nscoord CalcMaxRowWidth();

  
  
  
  
  
  PRBool OffsetForHorzScroll(nsRect& rect, PRBool clip);

  PRBool CanAutoScroll(PRInt32 aRowIndex);

  
  
  
  
  void ComputeDropPosition(nsGUIEvent* aEvent, PRInt32* aRow, PRInt16* aOrient,
                           PRInt16* aScrollLines);

  
  void MarkDirtyIfSelect();

  void InvalidateDropFeedback(PRInt32 aRow, PRInt16 aOrientation) {
    InvalidateRow(aRow);
    if (aOrientation != nsITreeView::DROP_ON)
      InvalidateRow(aRow + aOrientation);
  }

  already_AddRefed<nsTreeColumn> GetColumnImpl(nsITreeColumn* aUnknownCol) {
    if (!aUnknownCol)
      return nsnull;

    nsTreeColumn* col;
    aUnknownCol->QueryInterface(kTreeColumnImplCID, (void**)&col);
    return col;
  }

  
  
  
  
  nsresult CreateTimer(const nsILookAndFeel::nsMetricID aID,
                       nsTimerCallbackFunc aFunc, PRInt32 aType,
                       nsITimer** aTimer);

  static void OpenCallback(nsITimer *aTimer, void *aClosure);

  static void CloseCallback(nsITimer *aTimer, void *aClosure);

  static void LazyScrollCallback(nsITimer *aTimer, void *aClosure);

  static void ScrollCallback(nsITimer *aTimer, void *aClosure);

  class ScrollEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    ScrollEvent(nsTreeBodyFrame *aInner) : mInner(aInner) {}
    void Revoke() { mInner = nsnull; }
  private:
    nsTreeBodyFrame* mInner;
  };

  void PostScrollEvent();
  void FireScrollEvent();

protected: 
  
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  
  nsRefPtr<nsTreeColumns> mColumns;

  
  
  nsCOMPtr<nsITreeView> mView;    

  
  
  
  
  nsTreeStyleCache mStyleCache;

  
  
  
  
  nsDataHashtable<nsStringHashKey, nsTreeImageCacheEntry> mImageCache;

  
  
  
  PRInt32 mTopRowIndex;
  PRInt32 mPageLength;

  
  nscoord mHorzPosition;
  
  
  nscoord mHorzWidth;

  
  nsRect mInnerBox;
  PRInt32 mRowHeight;
  PRInt32 mIndentation;
  nscoord mStringWidth;

  
  nsCOMPtr<nsISupportsArray> mScratchArray;

  
  PRPackedBool mFocused;

  
  PRPackedBool mHasFixedRowCount;

  PRPackedBool mVerticalOverflow;
  PRPackedBool mHorizontalOverflow;

  PRPackedBool mReflowCallbackPosted;

  PRInt32 mUpdateBatchNest;

  
  PRInt32 mRowCount;

  class Slots {
    public:
      Slots()
        : mValueArray(~PRInt32(0)) {
      }

      ~Slots() {
        if (mTimer)
          mTimer->Cancel();
      }

      friend class nsTreeBodyFrame;

    protected:
      
      PRBool                   mDropAllowed;

      
      PRInt32                  mDropRow;

      
      PRInt16                  mDropOrient;

      
      PRInt16                  mScrollLines;

      
      PRUint32                 mDragAction;

      nsCOMPtr<nsIDragSession> mDragSession;

      
      nsCOMPtr<nsITimer>       mTimer;

      
      nsValueArray             mValueArray;
  };

  Slots* mSlots;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;
}; 
