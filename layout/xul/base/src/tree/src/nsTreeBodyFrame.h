










































#ifndef nsTreeBodyFrame_h
#define nsTreeBodyFrame_h

#include "mozilla/Attributes.h"

#include "nsLeafBoxFrame.h"
#include "nsITreeView.h"
#include "nsICSSPseudoComparator.h"
#include "nsIScrollbarMediator.h"
#include "nsIDragSession.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsTArray.h"
#include "nsTreeStyleCache.h"
#include "nsTreeColumns.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "imgIRequest.h"
#include "imgIDecoderObserver.h"
#include "nsScrollbarFrame.h"
#include "nsThreadUtils.h"
#include "mozilla/LookAndFeel.h"
#include "nsITreeImageListener.h"

class nsOverflowChecker;
class nsTreeImageListener;


struct nsTreeImageCacheEntry
{
  nsTreeImageCacheEntry() {}
  nsTreeImageCacheEntry(imgIRequest *aRequest, imgIDecoderObserver *aListener)
    : request(aRequest), listener(aListener) {}

  nsCOMPtr<imgIRequest> request;
  nsCOMPtr<imgIDecoderObserver> listener;
};


class nsTreeBodyFrame MOZ_FINAL
  : public nsLeafBoxFrame
  , public nsICSSPseudoComparator
  , public nsIScrollbarMediator
  , public nsIReflowCallback
{
public:
  nsTreeBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  ~nsTreeBodyFrame();

  NS_DECL_QUERYFRAME_TARGET(nsTreeBodyFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
  
  nsresult OnImageIsAnimated(imgIRequest* aRequest);

  
  nsresult GetColumns(nsITreeColumns **aColumns);
  nsresult GetView(nsITreeView **aView);
  nsresult SetView(nsITreeView *aView);
  nsresult GetFocused(bool *aFocused);
  nsresult SetFocused(bool aFocused);
  nsresult GetTreeBody(nsIDOMElement **aElement);
  nsresult GetRowHeight(PRInt32 *aValue);
  nsresult GetRowWidth(PRInt32 *aValue);
  nsresult GetHorizontalPosition(PRInt32 *aValue);
  nsresult GetSelectionRegion(nsIScriptableRegion **aRegion);
  nsresult GetFirstVisibleRow(PRInt32 *aValue);
  nsresult GetLastVisibleRow(PRInt32 *aValue);
  nsresult GetPageLength(PRInt32 *aValue);
  nsresult EnsureRowIsVisible(PRInt32 aRow);
  nsresult EnsureCellIsVisible(PRInt32 aRow, nsITreeColumn *aCol);
  nsresult ScrollToRow(PRInt32 aRow);
  nsresult ScrollByLines(PRInt32 aNumLines);
  nsresult ScrollByPages(PRInt32 aNumPages);
  nsresult ScrollToCell(PRInt32 aRow, nsITreeColumn *aCol);
  nsresult ScrollToColumn(nsITreeColumn *aCol);
  nsresult ScrollToHorizontalPosition(PRInt32 aValue);
  nsresult Invalidate();
  nsresult InvalidateColumn(nsITreeColumn *aCol);
  nsresult InvalidateRow(PRInt32 aRow);
  nsresult InvalidateCell(PRInt32 aRow, nsITreeColumn *aCol);
  nsresult InvalidateRange(PRInt32 aStart, PRInt32 aEnd);
  nsresult InvalidateColumnRange(PRInt32 aStart, PRInt32 aEnd,
                                 nsITreeColumn *aCol);
  nsresult GetRowAt(PRInt32 aX, PRInt32 aY, PRInt32 *aValue);
  nsresult GetCellAt(PRInt32 aX, PRInt32 aY, PRInt32 *aRow,
                     nsITreeColumn **aCol, nsACString &aChildElt);
  nsresult GetCoordsForCellItem(PRInt32 aRow, nsITreeColumn *aCol,
                                const nsACString &aElt,
                                PRInt32 *aX, PRInt32 *aY,
                                PRInt32 *aWidth, PRInt32 *aHeight);
  nsresult IsCellCropped(PRInt32 aRow, nsITreeColumn *aCol, bool *aResult);
  nsresult RowCountChanged(PRInt32 aIndex, PRInt32 aCount);
  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();
  nsresult ClearStyleAndImageCaches();

  
  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState);
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         bool aRemoveOverflowArea = false);

  
  virtual bool ReflowFinished();
  virtual void ReflowCallbackCanceled();

  
  virtual bool PseudoMatches(nsCSSSelector* aSelector);

  
  NS_IMETHOD PositionChanged(nsScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex);
  NS_IMETHOD ScrollbarButtonPressed(nsScrollbarFrame* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex);
  NS_IMETHOD VisibilityChanged(bool aVisible) { Invalidate(); return NS_OK; }

  
  NS_IMETHOD Init(nsIContent*     aContent,
                  nsIFrame*       aParent,
                  nsIFrame*       aPrevInFlow);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);

  NS_IMETHOD HandleEvent(nsPresContext* aPresContext,
                         nsGUIEvent* aEvent,
                         nsEventStatus* aEventStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  friend nsIFrame* NS_NewTreeBodyFrame(nsIPresShell* aPresShell);
  friend class nsTreeColumn;

  struct ScrollParts {
    nsScrollbarFrame*    mVScrollbar;
    nsCOMPtr<nsIContent> mVScrollbarContent;
    nsScrollbarFrame*    mHScrollbar;
    nsCOMPtr<nsIContent> mHScrollbarContent;
    nsIFrame*            mColumnsFrame;
    nsIScrollableFrame*  mColumnsScrollFrame;
  };

  void PaintTreeBody(nsRenderingContext& aRenderingContext,
                     const nsRect& aDirtyRect, nsPoint aPt);

  nsITreeBoxObject* GetTreeBoxObject() const { return mTreeBoxObject; }

  bool GetVerticalOverflow() const { return mVerticalOverflow; }
  bool GetHorizontalOverflow() const {return mHorizontalOverflow; }

protected:
  friend class nsOverflowChecker;

  
  void PaintColumn(nsTreeColumn*        aColumn,
                   const nsRect&        aColumnRect,
                   nsPresContext*      aPresContext,
                   nsRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);

  
  void PaintRow(PRInt32              aRowIndex,
                const nsRect&        aRowRect,
                nsPresContext*       aPresContext,
                nsRenderingContext& aRenderingContext,
                const nsRect&        aDirtyRect,
                nsPoint              aPt);

  
  void PaintSeparator(PRInt32              aRowIndex,
                      const nsRect&        aSeparatorRect,
                      nsPresContext*      aPresContext,
                      nsRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  
  void PaintCell(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aCellRect,
                 nsPresContext*       aPresContext,
                 nsRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX,
                 nsPoint              aPt);

  
  void PaintTwisty(PRInt32              aRowIndex,
                   nsTreeColumn*        aColumn,
                   const nsRect&        aTwistyRect,
                   nsPresContext*      aPresContext,
                   nsRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nscoord&             aRemainingWidth,
                   nscoord&             aCurrX);

  
  void PaintImage(PRInt32              aRowIndex,
                  nsTreeColumn*        aColumn,
                  const nsRect&        aImageRect,
                  nsPresContext*      aPresContext,
                  nsRenderingContext& aRenderingContext,
                  const nsRect&        aDirtyRect,
                  nscoord&             aRemainingWidth,
                  nscoord&             aCurrX);

  
  void PaintText(PRInt32              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aTextRect,
                 nsPresContext*      aPresContext,
                 nsRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX,
                 bool                 aTextRTL);

  
  void PaintCheckbox(PRInt32              aRowIndex, 
                     nsTreeColumn*        aColumn,
                     const nsRect&        aCheckboxRect,
                     nsPresContext*      aPresContext,
                     nsRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect);

  
  void PaintProgressMeter(PRInt32              aRowIndex, 
                          nsTreeColumn*        aColumn,
                          const nsRect&        aProgressMeterRect,
                          nsPresContext*      aPresContext,
                          nsRenderingContext& aRenderingContext,
                          const nsRect&        aDirtyRect);

  
  void PaintDropFeedback(const nsRect&        aDropFeedbackRect, 
                         nsPresContext*      aPresContext,
                         nsRenderingContext& aRenderingContext,
                         const nsRect&        aDirtyRect,
                         nsPoint              aPt);

  
  
  void PaintBackgroundLayer(nsStyleContext*      aStyleContext,
                            nsPresContext*      aPresContext, 
                            nsRenderingContext& aRenderingContext, 
                            const nsRect&        aRect,
                            const nsRect&        aDirtyRect);


  PRInt32 GetLastVisibleRow() {
    return mTopRowIndex + mPageLength;
  }

  
  
  PRInt32 GetRowAt(nscoord aX, nscoord aY);

  
  
  void CheckTextForBidi(nsAutoString& aText);

  void AdjustForCellText(nsAutoString& aText,
                         PRInt32 aRowIndex,  nsTreeColumn* aColumn,
                         nsRenderingContext& aRenderingContext,
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
                          nsRenderingContext& aRenderingContext,
                          nsStyleContext* aTwistyContext);

  
  nsresult GetImage(PRInt32 aRowIndex, nsTreeColumn* aCol, bool aUseContext,
                    nsStyleContext* aStyleContext, bool& aAllowImageRegions, imgIContainer** aResult);

  
  
  nsRect GetImageSize(PRInt32 aRowIndex, nsTreeColumn* aCol, bool aUseContext, nsStyleContext* aStyleContext);

  
  nsSize GetImageDestSize(nsStyleContext* aStyleContext, bool useImageRegion, imgIContainer* image);

  
  nsRect GetImageSourceRect(nsStyleContext* aStyleContext, bool useImageRegion, imgIContainer* image);

  
  PRInt32 GetRowHeight();

  
  PRInt32 GetIndentation();

  
  void CalcInnerBox();

  
  nscoord CalcHorzWidth(const ScrollParts& aParts);

  
  
  nsStyleContext* GetPseudoStyleContext(nsIAtom* aPseudoElement);

  
  
  
  
  
  ScrollParts GetScrollParts();

  
  void UpdateScrollbars(const ScrollParts& aParts);

  
  void InvalidateScrollbars(const ScrollParts& aParts, nsWeakFrame& aWeakColumnsFrame);

  
  void CheckOverflow(const ScrollParts& aParts);

  
  
  
  bool FullScrollbarsUpdate(bool aNeedsFullInvalidation);

  
  
  void PrefillPropertyArray(PRInt32 aRowIndex, nsTreeColumn* aCol);

  
  nsresult ScrollInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult ScrollToRowInternal(const ScrollParts& aParts, PRInt32 aRow);
  nsresult ScrollToColumnInternal(const ScrollParts& aParts, nsITreeColumn* aCol);
  nsresult ScrollHorzInternal(const ScrollParts& aParts, PRInt32 aPosition);
  nsresult EnsureRowIsVisibleInternal(const ScrollParts& aParts, PRInt32 aRow);

  
  nsPoint AdjustClientCoordsToBoxCoordSpace(PRInt32 aX, PRInt32 aY);

  
  void EnsureBoxObject();

  void EnsureView();

  
  nsIContent* GetBaseElement();

  nsresult GetCellWidth(PRInt32 aRow, nsTreeColumn* aCol,
                        nsRenderingContext* aRenderingContext,
                        nscoord& aDesiredSize, nscoord& aCurrentSize);
  nscoord CalcMaxRowWidth();

  
  
  
  
  
  bool OffsetForHorzScroll(nsRect& rect, bool clip);

  bool CanAutoScroll(PRInt32 aRowIndex);

  
  
  
  
  void ComputeDropPosition(nsGUIEvent* aEvent, PRInt32* aRow, PRInt16* aOrient,
                           PRInt16* aScrollLines);

  
  void MarkDirtyIfSelect();

  void InvalidateDropFeedback(PRInt32 aRow, PRInt16 aOrientation) {
    InvalidateRow(aRow);
    if (aOrientation != nsITreeView::DROP_ON)
      InvalidateRow(aRow + aOrientation);
  }

public:
  static
  already_AddRefed<nsTreeColumn> GetColumnImpl(nsITreeColumn* aUnknownCol) {
    if (!aUnknownCol)
      return nsnull;

    nsTreeColumn* col;
    aUnknownCol->QueryInterface(NS_GET_IID(nsTreeColumn), (void**)&col);
    return col;
  }

  






  void RemoveTreeImageListener(nsTreeImageListener* aListener);

protected:

  
  
  
  
  nsresult CreateTimer(const mozilla::LookAndFeel::IntID aID,
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

  



  void DetachImageListeners();

#ifdef ACCESSIBILITY
  








  void FireRowCountChangedEvent(PRInt32 aIndex, PRInt32 aCount);

  













  void FireInvalidateEvent(PRInt32 aStartRow, PRInt32 aEndRow,
                           nsITreeColumn *aStartCol, nsITreeColumn *aEndCol);
#endif

protected: 

  class Slots {
    public:
      Slots() {
      }

      ~Slots() {
        if (mTimer)
          mTimer->Cancel();
      }

      friend class nsTreeBodyFrame;

    protected:
      
      bool                     mDropAllowed;

      
      PRInt32                  mDropRow;

      
      PRInt16                  mDropOrient;

      
      PRInt16                  mScrollLines;

      
      PRUint32                 mDragAction;

      nsCOMPtr<nsIDragSession> mDragSession;

      
      nsCOMPtr<nsITimer>       mTimer;

      
      nsTArray<PRInt32>        mArray;
  };

  Slots* mSlots;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;

  
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  
  nsRefPtr<nsTreeColumns> mColumns;

  
  
  nsCOMPtr<nsITreeView> mView;

  
  
  
  
  nsTreeStyleCache mStyleCache;

  
  
  
  
  nsDataHashtable<nsStringHashKey, nsTreeImageCacheEntry> mImageCache;

  
  nsCOMPtr<nsISupportsArray> mScratchArray;

  
  
  
  PRInt32 mTopRowIndex;
  PRInt32 mPageLength;

  
  nscoord mHorzPosition;
  
  
  nscoord mHorzWidth;
  
  
  nscoord mAdjustWidth;

  
  nsRect mInnerBox; 
  PRInt32 mRowHeight;
  PRInt32 mIndentation;
  nscoord mStringWidth;

  PRInt32 mUpdateBatchNest;

  
  PRInt32 mRowCount;

  
  PRInt32 mMouseOverRow;

  
  bool mFocused;

  
  bool mHasFixedRowCount;

  bool mVerticalOverflow;
  bool mHorizontalOverflow;

  bool mReflowCallbackPosted;

  
  
  nsTHashtable<nsPtrHashKey<nsTreeImageListener> > mCreatedListeners;

}; 

#endif
