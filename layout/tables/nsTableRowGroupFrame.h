



































#ifndef nsTableRowGroupFrame_h__
#define nsTableRowGroupFrame_h__

#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsIAtom.h"
#include "nsILineIterator.h"
#include "nsTablePainter.h"
#include "nsTArray.h"
#include "nsCSSAnonBoxes.h"

class nsTableFrame;
class nsTableRowFrame;
class nsTableCellFrame;

struct nsRowGroupReflowState {
  const nsHTMLReflowState& reflowState;  

  nsTableFrame* tableFrame;

  
  nsSize availSize;

  
  nscoord y;

  nsRowGroupReflowState(const nsHTMLReflowState& aReflowState,
                        nsTableFrame*            aTableFrame)
      :reflowState(aReflowState), tableFrame(aTableFrame)
  {
    availSize.width  = reflowState.availableWidth;
    availSize.height = reflowState.availableHeight;
    y = 0;  
  }

  ~nsRowGroupReflowState() {}
};




#define NS_ROWGROUP_REPEATABLE           0x80000000
#define NS_ROWGROUP_HAS_STYLE_HEIGHT     0x40000000

#define NS_REPEATED_ROW_OR_ROWGROUP      0x10000000
#define NS_ROWGROUP_HAS_ROW_CURSOR       0x08000000

#define MIN_ROWS_NEEDING_CURSOR 20










class nsTableRowGroupFrame
  : public nsHTMLContainerFrame
  , public nsILineIterator
{
public:
  NS_DECLARE_FRAME_ACCESSOR(nsTableRowGroupFrame)
  NS_DECL_QUERYFRAME

  




  friend nsIFrame* NS_NewTableRowGroupFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
  virtual ~nsTableRowGroupFrame();
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);

  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsMargin GetUsedMargin() const;
  virtual nsMargin GetUsedBorder() const;
  virtual nsMargin GetUsedPadding() const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

   








  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsContainingBlock() const;

  nsTableRowFrame* GetFirstRow();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  
  PRInt32 GetRowCount();

  


  PRInt32 GetStartRowIndex();

  



  void AdjustRowIndices(PRInt32   aRowIndex,
                        PRInt32   anAdjustment);

  








  nsresult  InitRepeatedFrame(nsPresContext*        aPresContext,
                              nsTableRowGroupFrame* aHeaderFooterFrame);

  
  


  nscoord GetHeightBasis(const nsHTMLReflowState& aReflowState);
  
  nsMargin* GetBCBorderWidth(nsMargin& aBorder);

  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  



  void SetContinuousBCBorderWidth(PRUint8     aForSide,
                                  BCPixelSize aPixelValue);
  








  nscoord CollapseRowGroupIfNecessary(nscoord aYTotalOffset,
                                      nscoord aWidth);


public:
  virtual void DisposeLineIterator() { }

  
  
  
  
  
  
   
  


  virtual PRInt32 GetNumLines();

  


  virtual PRBool GetDirection();
  
  











  NS_IMETHOD GetLine(PRInt32 aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     PRInt32* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     PRUint32* aLineFlags);
  
  




  virtual PRInt32 FindLineContaining(nsIFrame* aFrame);
  
  


  virtual PRInt32 FindLineAt(nscoord aY);

  










  NS_IMETHOD FindFrameAt(PRInt32 aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         PRBool* aXIsBeforeFirstFrame,
                         PRBool* aXIsAfterLastFrame);

#ifdef IBMBIDI
   







  NS_IMETHOD CheckLineOrder(PRInt32                  aLine,
                            PRBool                   *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual);
#endif

  



  
  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, PRInt32 aLineNumber);

  
  
  
  
  
  
  
  
  
  
  struct FrameCursorData {
    nsTArray<nsIFrame*> mFrames;
    PRUint32            mCursorIndex;
    nscoord             mOverflowAbove;
    nscoord             mOverflowBelow;
    
    FrameCursorData()
      : mFrames(MIN_ROWS_NEEDING_CURSOR), mCursorIndex(0), mOverflowAbove(0),
        mOverflowBelow(0) {}

    PRBool AppendFrame(nsIFrame* aFrame);
    
    void FinishBuildingCursor() {
      mFrames.Compact();
    }
  };

  
  void ClearRowCursor();

  










  nsIFrame* GetFirstRowContaining(nscoord aY, nscoord* aOverflowAbove);

  






  FrameCursorData* SetupRowCursor();
  
  PRBool IsScrolled() {
    
    
    return GetStyleContext()->GetPseudoType() == nsCSSAnonBoxes::scrolledContent ||
           GetStyleDisplay()->mOverflowY == NS_STYLE_OVERFLOW_CLIP;
  }

  virtual nsILineIterator* GetLineIterator() { return this; }

protected:
  nsTableRowGroupFrame(nsStyleContext* aContext);

  void InitChildReflowState(nsPresContext&     aPresContext, 
                            PRBool             aBorderCollapse,
                            nsHTMLReflowState& aReflowState);
  
  
  virtual PRIntn GetSkipSides() const;

  void PlaceChild(nsPresContext*         aPresContext,
                  nsRowGroupReflowState& aReflowState,
                  nsIFrame*              aKidFrame,
                  nsHTMLReflowMetrics&   aDesiredSize,
                  const nsRect&          aOriginalKidRect,
                  const nsRect&          aOriginalKidOverflowRect);

  void CalculateRowHeights(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState);

  void DidResizeRows(nsHTMLReflowMetrics& aDesiredSize);

  void SlideChild(nsRowGroupReflowState& aReflowState,
                  nsIFrame*              aKidFrame);
  
  







  NS_METHOD ReflowChildren(nsPresContext*         aPresContext,
                           nsHTMLReflowMetrics&   aDesiredSize,
                           nsRowGroupReflowState& aReflowState,
                           nsReflowStatus&        aStatus,
                           PRBool*                aPageBreakBeforeEnd = nsnull);

  nsresult SplitRowGroup(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsTableFrame*            aTableFrame,
                         nsReflowStatus&          aStatus);

  void SplitSpanningCells(nsPresContext&           aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          nsTableFrame&            aTableFrame,
                          nsTableRowFrame&         aFirstRow, 
                          nsTableRowFrame&         aLastRow,  
                          PRBool                   aFirstRowIsTopOfPage,
                          nscoord                  aSpanningRowBottom,
                          nsTableRowFrame*&        aContRowFrame,
                          nsTableRowFrame*&        aFirstTruncatedRow,
                          nscoord&                 aDesiredHeight);

  void CreateContinuingRowFrame(nsPresContext& aPresContext,
                                nsIFrame&      aRowFrame,
                                nsIFrame**     aContRowFrame);

  PRBool IsSimpleRowFrame(nsTableFrame* aTableFrame, 
                          nsIFrame*     aFrame);

  void GetNextRowSibling(nsIFrame** aRowFrame);

  void UndoContinuedRow(nsPresContext*   aPresContext,
                        nsTableRowFrame* aRow);
                        
private:
  
  BCPixelSize mRightContBorderWidth;
  BCPixelSize mBottomContBorderWidth;
  BCPixelSize mLeftContBorderWidth;

public:
  virtual nsIFrame* GetFirstFrame() { return mFrames.FirstChild(); }
  virtual nsIFrame* GetLastFrame() { return mFrames.LastChild(); }
  virtual void GetNextFrame(nsIFrame*  aFrame, 
                            nsIFrame** aResult) { *aResult = aFrame->GetNextSibling(); }
  PRBool IsRepeatable() const;
  void   SetRepeatable(PRBool aRepeatable);
  PRBool HasStyleHeight() const;
  void   SetHasStyleHeight(PRBool aValue);
};


inline PRBool nsTableRowGroupFrame::IsRepeatable() const
{
  return (mState & NS_ROWGROUP_REPEATABLE) == NS_ROWGROUP_REPEATABLE;
}

inline void nsTableRowGroupFrame::SetRepeatable(PRBool aRepeatable)
{
  if (aRepeatable) {
    mState |= NS_ROWGROUP_REPEATABLE;
  } else {
    mState &= ~NS_ROWGROUP_REPEATABLE;
  }
}

inline PRBool nsTableRowGroupFrame::HasStyleHeight() const
{
  return (mState & NS_ROWGROUP_HAS_STYLE_HEIGHT) == NS_ROWGROUP_HAS_STYLE_HEIGHT;
}

inline void nsTableRowGroupFrame::SetHasStyleHeight(PRBool aValue)
{
  if (aValue) {
    mState |= NS_ROWGROUP_HAS_STYLE_HEIGHT;
  } else {
    mState &= ~NS_ROWGROUP_HAS_STYLE_HEIGHT;
  }
}

inline void
nsTableRowGroupFrame::GetContinuousBCBorderWidth(nsMargin& aBorder)
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.right = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips,
                                            mRightContBorderWidth);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips,
                                            mBottomContBorderWidth);
  aBorder.left = BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips,
                                            mLeftContBorderWidth);
  return;
}
#endif
