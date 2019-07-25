



#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "nsITableCellLayout.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTableRowFrame.h"  
#include "nsStyleContext.h"
#include "nsIPercentHeightObserver.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsTArray.h"

class nsTableFrame;




#define NS_TABLE_CELL_CONTENT_EMPTY       NS_FRAME_STATE_BIT(31)
#define NS_TABLE_CELL_HAD_SPECIAL_REFLOW  NS_FRAME_STATE_BIT(29)
#define NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT NS_FRAME_STATE_BIT(28)












class nsTableCellFrame : public nsContainerFrame,
                         public nsITableCellLayout,
                         public nsIPercentHeightObserver
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsTableCellFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  nsTableCellFrame(nsStyleContext* aContext);
  ~nsTableCellFrame();

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible();
#endif

  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  
  
  
  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual nsMargin GetUsedMargin() const;

  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState);

  virtual bool NeedsToObserve(const nsHTMLReflowState& aReflowState);

  




  friend nsIFrame* NS_NewTableCellFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintCellBackground(nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, nsPoint aPt,
                           uint32_t aFlags);

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsRenderingContext* aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  void VerticallyAlignChild(nscoord aMaxAscent);

  




  uint8_t GetVerticalAlign() const;

  bool HasVerticalAlignBaseline() const {
    return GetVerticalAlign() == NS_STYLE_VERTICAL_ALIGN_BASELINE;
  }

  bool CellHasVisibleContent(nscoord       height,
                               nsTableFrame* tableFrame,
                               nsIFrame*     kidFrame);

  



  nscoord GetCellBaseline() const;

  





  virtual int32_t GetRowSpan();

  

  

  




  NS_IMETHOD GetCellIndexes(int32_t &aRowIndex, int32_t &aColIndex);

  
  virtual nsresult GetRowIndex(int32_t &aRowIndex) const;

  





  virtual int32_t GetColSpan();

  
  virtual nsresult GetColIndex(int32_t &aColIndex) const;
  void SetColIndex(int32_t aColIndex);

  
  inline nscoord GetPriorAvailWidth();

  
  inline void SetPriorAvailWidth(nscoord aPriorAvailWidth);

  
  inline nsSize GetDesiredSize();

  
  inline void SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize);

  bool GetContentEmpty();
  void SetContentEmpty(bool aContentEmpty);

  bool HasPctOverHeight();
  void SetHasPctOverHeight(bool aValue);

  nsTableCellFrame* GetNextCell() const;

  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const;

  virtual void PaintBackground(nsRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect,
                               nsPoint              aPt,
                               uint32_t             aFlags);

  void DecorateForSelection(nsRenderingContext& aRenderingContext,
                            nsPoint              aPt);

  virtual bool UpdateOverflow();

protected:
  
  virtual int GetSkipSides() const;

  






  virtual nsMargin GetBorderOverflow();

  friend class nsTableRowFrame;

  uint32_t     mColIndex;             

  nscoord      mPriorAvailWidth;      
  nsSize       mDesiredSize;          
};

inline nscoord nsTableCellFrame::GetPriorAvailWidth()
{ return mPriorAvailWidth;}

inline void nsTableCellFrame::SetPriorAvailWidth(nscoord aPriorAvailWidth)
{ mPriorAvailWidth = aPriorAvailWidth;}

inline nsSize nsTableCellFrame::GetDesiredSize()
{ return mDesiredSize; }

inline void nsTableCellFrame::SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize)
{
  mDesiredSize.width = aDesiredSize.width;
  mDesiredSize.height = aDesiredSize.height;
}

inline bool nsTableCellFrame::GetContentEmpty()
{
  return (mState & NS_TABLE_CELL_CONTENT_EMPTY) ==
         NS_TABLE_CELL_CONTENT_EMPTY;
}

inline void nsTableCellFrame::SetContentEmpty(bool aContentEmpty)
{
  if (aContentEmpty) {
    mState |= NS_TABLE_CELL_CONTENT_EMPTY;
  } else {
    mState &= ~NS_TABLE_CELL_CONTENT_EMPTY;
  }
}

inline bool nsTableCellFrame::HasPctOverHeight()
{
  return (mState & NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT) ==
         NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT;
}

inline void nsTableCellFrame::SetHasPctOverHeight(bool aValue)
{
  if (aValue) {
    mState |= NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT;
  } else {
    mState &= ~NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT;
  }
}


class nsBCTableCellFrame : public nsTableCellFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsBCTableCellFrame(nsStyleContext* aContext);

  ~nsBCTableCellFrame();

  virtual nsIAtom* GetType() const;

  virtual nsMargin GetUsedBorder() const;
  virtual bool GetBorderRadii(nscoord aRadii[8]) const;

  
  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const;

  
  BCPixelSize GetBorderWidth(mozilla::css::Side aSide) const;

  
  void SetBorderWidth(mozilla::css::Side aSide, BCPixelSize aPixelValue);

  virtual nsMargin GetBorderOverflow();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual void PaintBackground(nsRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect,
                               nsPoint              aPt,
                               uint32_t             aFlags);

private:

  
  
  BCPixelSize mTopBorder;
  BCPixelSize mRightBorder;
  BCPixelSize mBottomBorder;
  BCPixelSize mLeftBorder;
};

#endif
