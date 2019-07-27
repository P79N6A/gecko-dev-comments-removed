




#ifndef nsTreeBodyFrame_h
#define nsTreeBodyFrame_h

#include "mozilla/Attributes.h"

#include "nsLeafBoxFrame.h"
#include "nsITreeView.h"
#include "nsICSSPseudoComparator.h"
#include "nsIScrollbarMediator.h"
#include "nsITimer.h"
#include "nsIReflowCallback.h"
#include "nsTArray.h"
#include "nsTreeStyleCache.h"
#include "nsTreeColumns.h"
#include "nsAutoPtr.h"
#include "nsDataHashtable.h"
#include "imgIRequest.h"
#include "imgINotificationObserver.h"
#include "nsScrollbarFrame.h"
#include "nsThreadUtils.h"
#include "mozilla/LookAndFeel.h"

class nsOverflowChecker;
class nsTreeImageListener;

namespace mozilla {
namespace layout {
class ScrollbarActivity;
}
}


struct nsTreeImageCacheEntry
{
  nsTreeImageCacheEntry() {}
  nsTreeImageCacheEntry(imgIRequest *aRequest, imgINotificationObserver *aListener)
    : request(aRequest), listener(aListener) {}

  nsCOMPtr<imgIRequest> request;
  nsCOMPtr<imgINotificationObserver> listener;
};


class nsTreeBodyFrame MOZ_FINAL
  : public nsLeafBoxFrame
  , public nsICSSPseudoComparator
  , public nsIScrollbarMediator
  , public nsIReflowCallback
{
public:
  typedef mozilla::layout::ScrollbarActivity ScrollbarActivity;

  nsTreeBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  ~nsTreeBodyFrame();

  NS_DECL_QUERYFRAME_TARGET(nsTreeBodyFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  
  
  nsresult OnImageIsAnimated(imgIRequest* aRequest);

  
  already_AddRefed<nsTreeColumns> Columns() const
  {
    nsRefPtr<nsTreeColumns> cols = mColumns;
    return cols.forget();
  }
  already_AddRefed<nsITreeView> GetExistingView() const
  {
    nsCOMPtr<nsITreeView> view = mView;
    return view.forget();
  }
  nsresult GetView(nsITreeView **aView);
  nsresult SetView(nsITreeView *aView);
  nsresult GetFocused(bool *aFocused);
  nsresult SetFocused(bool aFocused);
  nsresult GetTreeBody(nsIDOMElement **aElement);
  nsresult GetRowHeight(int32_t *aValue);
  nsresult GetRowWidth(int32_t *aValue);
  nsresult GetHorizontalPosition(int32_t *aValue);
  nsresult GetSelectionRegion(nsIScriptableRegion **aRegion);
  int32_t FirstVisibleRow() const { return mTopRowIndex; }
  int32_t LastVisibleRow() const { return mTopRowIndex + mPageLength; }
  int32_t PageLength() const { return mPageLength; }
  nsresult EnsureRowIsVisible(int32_t aRow);
  nsresult EnsureCellIsVisible(int32_t aRow, nsITreeColumn *aCol);
  nsresult ScrollToRow(int32_t aRow);
  nsresult ScrollByLines(int32_t aNumLines);
  nsresult ScrollByPages(int32_t aNumPages);
  nsresult ScrollToCell(int32_t aRow, nsITreeColumn *aCol);
  nsresult ScrollToColumn(nsITreeColumn *aCol);
  nsresult ScrollToHorizontalPosition(int32_t aValue);
  nsresult Invalidate();
  nsresult InvalidateColumn(nsITreeColumn *aCol);
  nsresult InvalidateRow(int32_t aRow);
  nsresult InvalidateCell(int32_t aRow, nsITreeColumn *aCol);
  nsresult InvalidateRange(int32_t aStart, int32_t aEnd);
  nsresult InvalidateColumnRange(int32_t aStart, int32_t aEnd,
                                 nsITreeColumn *aCol);
  nsresult GetRowAt(int32_t aX, int32_t aY, int32_t *aValue);
  nsresult GetCellAt(int32_t aX, int32_t aY, int32_t *aRow,
                     nsITreeColumn **aCol, nsACString &aChildElt);
  nsresult GetCoordsForCellItem(int32_t aRow, nsITreeColumn *aCol,
                                const nsACString &aElt,
                                int32_t *aX, int32_t *aY,
                                int32_t *aWidth, int32_t *aHeight);
  nsresult IsCellCropped(int32_t aRow, nsITreeColumn *aCol, bool *aResult);
  nsresult RowCountChanged(int32_t aIndex, int32_t aCount);
  nsresult BeginUpdateBatch();
  nsresult EndUpdateBatch();
  nsresult ClearStyleAndImageCaches();

  void ManageReflowCallback(const nsRect& aRect, nscoord aHorzWidth);

  virtual nsSize GetMinSize(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual void SetBounds(nsBoxLayoutState& aBoxLayoutState, const nsRect& aRect,
                         bool aRemoveOverflowArea = false) MOZ_OVERRIDE;

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

  
  virtual bool PseudoMatches(nsCSSSelector* aSelector) MOZ_OVERRIDE;

  
  virtual void ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection) MOZ_OVERRIDE;
  virtual void RepeatButtonScroll(nsScrollbarFrame* aScrollbar) MOZ_OVERRIDE;
  virtual void ThumbMoved(nsScrollbarFrame* aScrollbar,
                          nscoord aOldPos,
                          nscoord aNewPos) MOZ_OVERRIDE;
  virtual void VisibilityChanged(bool aVisible) MOZ_OVERRIDE { Invalidate(); }
  virtual nsIFrame* GetScrollbarBox(bool aVertical) MOZ_OVERRIDE {
    ScrollParts parts = GetScrollParts();
    return aVertical ? parts.mVScrollbar : parts.mHScrollbar;
  }
  virtual void ScrollbarActivityStarted() const MOZ_OVERRIDE;
  virtual void ScrollbarActivityStopped() const MOZ_OVERRIDE;

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  virtual nsresult GetCursor(const nsPoint& aPoint,
                             nsIFrame::Cursor& aCursor) MOZ_OVERRIDE;

  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               mozilla::WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

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

  
  nsIContent* GetBaseElement();

  bool GetVerticalOverflow() const { return mVerticalOverflow; }
  bool GetHorizontalOverflow() const {return mHorizontalOverflow; }

protected:
  friend class nsOverflowChecker;

  
  void PaintColumn(nsTreeColumn*        aColumn,
                   const nsRect&        aColumnRect,
                   nsPresContext*      aPresContext,
                   nsRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect);

  
  void PaintRow(int32_t              aRowIndex,
                const nsRect&        aRowRect,
                nsPresContext*       aPresContext,
                nsRenderingContext& aRenderingContext,
                const nsRect&        aDirtyRect,
                nsPoint              aPt);

  
  void PaintSeparator(int32_t              aRowIndex,
                      const nsRect&        aSeparatorRect,
                      nsPresContext*      aPresContext,
                      nsRenderingContext& aRenderingContext,
                      const nsRect&        aDirtyRect);

  
  void PaintCell(int32_t              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aCellRect,
                 nsPresContext*       aPresContext,
                 nsRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX,
                 nsPoint              aPt);

  
  void PaintTwisty(int32_t              aRowIndex,
                   nsTreeColumn*        aColumn,
                   const nsRect&        aTwistyRect,
                   nsPresContext*      aPresContext,
                   nsRenderingContext& aRenderingContext,
                   const nsRect&        aDirtyRect,
                   nscoord&             aRemainingWidth,
                   nscoord&             aCurrX);

  
  void PaintImage(int32_t              aRowIndex,
                  nsTreeColumn*        aColumn,
                  const nsRect&        aImageRect,
                  nsPresContext*      aPresContext,
                  nsRenderingContext& aRenderingContext,
                  const nsRect&        aDirtyRect,
                  nscoord&             aRemainingWidth,
                  nscoord&             aCurrX);

  
  void PaintText(int32_t              aRowIndex, 
                 nsTreeColumn*        aColumn,
                 const nsRect&        aTextRect,
                 nsPresContext*      aPresContext,
                 nsRenderingContext& aRenderingContext,
                 const nsRect&        aDirtyRect,
                 nscoord&             aCurrX);

  
  void PaintCheckbox(int32_t              aRowIndex, 
                     nsTreeColumn*        aColumn,
                     const nsRect&        aCheckboxRect,
                     nsPresContext*      aPresContext,
                     nsRenderingContext& aRenderingContext,
                     const nsRect&        aDirtyRect);

  
  void PaintProgressMeter(int32_t              aRowIndex, 
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


  
  
  int32_t GetRowAt(nscoord aX, nscoord aY);

  
  
  void CheckTextForBidi(nsAutoString& aText);

  void AdjustForCellText(nsAutoString& aText,
                         int32_t aRowIndex,  nsTreeColumn* aColumn,
                         nsRenderingContext& aRenderingContext,
                         nsRect& aTextRect);

  
  nsIAtom* GetItemWithinCellAt(nscoord aX, const nsRect& aCellRect,
                               int32_t aRowIndex, nsTreeColumn* aColumn);

  
  
  void GetCellAt(nscoord aX, nscoord aY, int32_t* aRow, nsTreeColumn** aCol,
                 nsIAtom** aChildElt);

  
  nsITheme* GetTwistyRect(int32_t aRowIndex,
                          nsTreeColumn* aColumn,
                          nsRect& aImageRect,
                          nsRect& aTwistyRect,
                          nsPresContext* aPresContext,
                          nsRenderingContext& aRenderingContext,
                          nsStyleContext* aTwistyContext);

  
  nsresult GetImage(int32_t aRowIndex, nsTreeColumn* aCol, bool aUseContext,
                    nsStyleContext* aStyleContext, bool& aAllowImageRegions, imgIContainer** aResult);

  
  
  nsRect GetImageSize(int32_t aRowIndex, nsTreeColumn* aCol, bool aUseContext, nsStyleContext* aStyleContext);

  
  nsSize GetImageDestSize(nsStyleContext* aStyleContext, bool useImageRegion, imgIContainer* image);

  
  nsRect GetImageSourceRect(nsStyleContext* aStyleContext, bool useImageRegion, imgIContainer* image);

  
  int32_t GetRowHeight();

  
  int32_t GetIndentation();

  
  void CalcInnerBox();

  
  nscoord CalcHorzWidth(const ScrollParts& aParts);

  
  
  nsStyleContext* GetPseudoStyleContext(nsIAtom* aPseudoElement);

  
  
  
  
  
  ScrollParts GetScrollParts();

  
  void UpdateScrollbars(const ScrollParts& aParts);

  
  void InvalidateScrollbars(const ScrollParts& aParts, nsWeakFrame& aWeakColumnsFrame);

  
  void CheckOverflow(const ScrollParts& aParts);

  
  
  
  bool FullScrollbarsUpdate(bool aNeedsFullInvalidation);

  
  
  void PrefillPropertyArray(int32_t aRowIndex, nsTreeColumn* aCol);

  
  nsresult ScrollInternal(const ScrollParts& aParts, int32_t aRow);
  nsresult ScrollToRowInternal(const ScrollParts& aParts, int32_t aRow);
  nsresult ScrollToColumnInternal(const ScrollParts& aParts, nsITreeColumn* aCol);
  nsresult ScrollHorzInternal(const ScrollParts& aParts, int32_t aPosition);
  nsresult EnsureRowIsVisibleInternal(const ScrollParts& aParts, int32_t aRow);

  
  nsPoint AdjustClientCoordsToBoxCoordSpace(int32_t aX, int32_t aY);

  
  void EnsureBoxObject();

  void EnsureView();

  nsresult GetCellWidth(int32_t aRow, nsTreeColumn* aCol,
                        nsRenderingContext* aRenderingContext,
                        nscoord& aDesiredSize, nscoord& aCurrentSize);
  nscoord CalcMaxRowWidth();

  
  
  
  
  
  bool OffsetForHorzScroll(nsRect& rect, bool clip);

  bool CanAutoScroll(int32_t aRowIndex);

  
  
  
  
  void ComputeDropPosition(mozilla::WidgetGUIEvent* aEvent,
                           int32_t* aRow,
                           int16_t* aOrient,
                           int16_t* aScrollLines);

  
  void MarkDirtyIfSelect();

  void InvalidateDropFeedback(int32_t aRow, int16_t aOrientation) {
    InvalidateRow(aRow);
    if (aOrientation != nsITreeView::DROP_ON)
      InvalidateRow(aRow + aOrientation);
  }

public:
  static
  already_AddRefed<nsTreeColumn> GetColumnImpl(nsITreeColumn* aUnknownCol) {
    if (!aUnknownCol)
      return nullptr;

    nsCOMPtr<nsTreeColumn> col = do_QueryInterface(aUnknownCol);
    return col.forget();
  }

  






  void RemoveTreeImageListener(nsTreeImageListener* aListener);

protected:

  
  
  
  
  nsresult CreateTimer(const mozilla::LookAndFeel::IntID aID,
                       nsTimerCallbackFunc aFunc, int32_t aType,
                       nsITimer** aTimer);

  static void OpenCallback(nsITimer *aTimer, void *aClosure);

  static void CloseCallback(nsITimer *aTimer, void *aClosure);

  static void LazyScrollCallback(nsITimer *aTimer, void *aClosure);

  static void ScrollCallback(nsITimer *aTimer, void *aClosure);

  class ScrollEvent : public nsRunnable {
  public:
    NS_DECL_NSIRUNNABLE
    explicit ScrollEvent(nsTreeBodyFrame *aInner) : mInner(aInner) {}
    void Revoke() { mInner = nullptr; }
  private:
    nsTreeBodyFrame* mInner;
  };

  void PostScrollEvent();
  void FireScrollEvent();

  



  void DetachImageListeners();

#ifdef ACCESSIBILITY
  








  void FireRowCountChangedEvent(int32_t aIndex, int32_t aCount);

  













  void FireInvalidateEvent(int32_t aStartRow, int32_t aEndRow,
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

      
      bool mIsDragging;

      
      int32_t                  mDropRow;

      
      int16_t                  mDropOrient;

      
      int16_t                  mScrollLines;

      
      uint32_t                 mDragAction;

      
      nsCOMPtr<nsITimer>       mTimer;

      
      nsTArray<int32_t>        mArray;
  };

  Slots* mSlots;

  nsRevocableEventPtr<ScrollEvent> mScrollEvent;

  nsRefPtr<ScrollbarActivity> mScrollbarActivity;

  
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  
  nsRefPtr<nsTreeColumns> mColumns;

  
  
  nsCOMPtr<nsITreeView> mView;

  
  
  
  
  nsTreeStyleCache mStyleCache;

  
  
  
  
  nsDataHashtable<nsStringHashKey, nsTreeImageCacheEntry> mImageCache;

  
  AtomArray mScratchArray;

  
  
  
  int32_t mTopRowIndex;
  int32_t mPageLength;

  
  nscoord mHorzPosition;

  
  
  
  
  
  nscoord mOriginalHorzWidth;
  
  
  nscoord mHorzWidth;
  
  
  nscoord mAdjustWidth;

  
  nsRect mInnerBox; 
  int32_t mRowHeight;
  int32_t mIndentation;
  nscoord mStringWidth;

  int32_t mUpdateBatchNest;

  
  int32_t mRowCount;

  
  int32_t mMouseOverRow;

  
  bool mFocused;

  
  bool mHasFixedRowCount;

  bool mVerticalOverflow;
  bool mHorizontalOverflow;

  bool mReflowCallbackPosted;

  
  
  bool mCheckingOverflow;

  
  
  nsTHashtable<nsPtrHashKey<nsTreeImageListener> > mCreatedListeners;

}; 

#endif
