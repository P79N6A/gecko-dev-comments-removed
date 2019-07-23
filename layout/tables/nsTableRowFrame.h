



































#ifndef nsTableRowFrame_h__
#define nsTableRowFrame_h__

#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsTablePainter.h"

class  nsTableFrame;
class  nsTableCellFrame;
struct nsTableCellReflowState;






#define NS_ROW_HAS_CELL_WITH_STYLE_HEIGHT   0x20000000

#define NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT 0x40000000











class nsTableRowFrame : public nsHTMLContainerFrame
{
public:

  NS_DECLARE_FRAME_ACCESSOR(nsTableRowFrame)
  NS_DECL_QUERYFRAME

  virtual ~nsTableRowFrame();

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  




  friend nsIFrame* NS_NewTableRowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsMargin GetUsedMargin() const;
  virtual nsMargin GetUsedBorder() const;
  virtual nsMargin GetUsedPadding() const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  nsTableCellFrame* GetFirstCell() ;

  












  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  void DidResize();

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
 
  void UpdateHeight(nscoord           aHeight,
                    nscoord           aAscent,
                    nscoord           aDescent,
                    nsTableFrame*     aTableFrame = nsnull,
                    nsTableCellFrame* aCellFrame  = nsnull);

  void ResetHeight(nscoord aRowStyleHeight);

  
  
  nscoord CalcHeight(const nsHTMLReflowState& aReflowState);

  

  




  nscoord GetMaxCellAscent() const;

  

  nscoord GetRowBaseline();
 
  
  virtual PRInt32 GetRowIndex() const;

  
  void SetRowIndex (int aRowIndex);

  
  nscoord ReflowCellFrame(nsPresContext*          aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          PRBool                   aIsTopOfPage,
                          nsTableCellFrame*        aCellFrame,
                          nscoord                  aAvailableHeight,
                          nsReflowStatus&          aStatus);
  









  nscoord CollapseRowIfNecessary(nscoord aRowOffset,
                                 nscoord aWidth,
                                 PRBool  aCollapseGroup,
                                 PRBool& aDidCollapse);

  void InsertCellFrame(nsTableCellFrame* aFrame, 
                       nsTableCellFrame* aPrevSibling);

  void InsertCellFrame(nsTableCellFrame* aFrame,
                       PRInt32           aColIndex);

  void RemoveCellFrame(nsTableCellFrame* aFrame);

  nsresult CalculateCellActualSize(nsIFrame*       aRowFrame,
                                   nscoord&        aDesiredWidth,
                                   nscoord&        aDesiredHeight,
                                   nscoord         aAvailWidth);

  PRBool IsFirstInserted() const;
  void   SetFirstInserted(PRBool aValue);

  PRBool GetContentHeight() const;
  void   SetContentHeight(nscoord aTwipValue);

  PRBool HasStyleHeight() const;

  PRBool HasFixedHeight() const;
  void   SetHasFixedHeight(PRBool aValue);

  PRBool HasPctHeight() const;
  void   SetHasPctHeight(PRBool aValue);

  nscoord GetFixedHeight() const;
  void    SetFixedHeight(nscoord aValue);

  float   GetPctHeight() const;
  void    SetPctHeight(float  aPctValue,
                       PRBool aForce = PR_FALSE);

  nscoord GetHeight(nscoord aBasis = 0) const;

  nsTableRowFrame* GetNextRow() const;

  PRBool  HasUnpaginatedHeight();
  void    SetHasUnpaginatedHeight(PRBool aValue);
  nscoord GetUnpaginatedHeight(nsPresContext* aPresContext);
  void    SetUnpaginatedHeight(nsPresContext* aPresContext, nscoord aValue);

  nscoord GetTopBCBorderWidth();
  void    SetTopBCBorderWidth(BCPixelSize aWidth);
  nscoord GetBottomBCBorderWidth();
  void    SetBottomBCBorderWidth(BCPixelSize aWidth);
  nsMargin* GetBCBorderWidth(nsMargin& aBorder);
                             
  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  


  nscoord GetOuterTopContBCBorderWidth();
  



  void SetContinuousBCBorderWidth(PRUint8     aForSide,
                                  BCPixelSize aPixelValue);

protected:

  


  nsTableRowFrame(nsStyleContext *aContext);

  void InitChildReflowState(nsPresContext&         aPresContext,
                            const nsSize&           aAvailSize,
                            PRBool                  aBorderCollapse,
                            nsTableCellReflowState& aReflowState);
  
  
  virtual PRIntn GetSkipSides() const;

  

  nscoord ComputeCellXOffset(const nsHTMLReflowState& aState,
                             nsIFrame*                aKidFrame,
                             const nsMargin&          aKidMargin) const;
  



  NS_IMETHOD ReflowChildren(nsPresContext*          aPresContext,
                            nsHTMLReflowMetrics&     aDesiredSize,
                            const nsHTMLReflowState& aReflowState,
                            nsTableFrame&            aTableFrame,
                            nsReflowStatus&          aStatus);

private:
  struct RowBits {
    unsigned mRowIndex:29;
    unsigned mHasFixedHeight:1; 
    unsigned mHasPctHeight:1;   
    unsigned mFirstInserted:1;  
  } mBits;

  
  nscoord mContentHeight;
  
  
  nscoord mStylePctHeight;
  
  
  nscoord mStyleFixedHeight;

  
  nscoord mMaxCellAscent;  
  nscoord mMaxCellDescent; 

  
  
  BCPixelSize mTopBorderWidth;
  BCPixelSize mBottomBorderWidth;
  BCPixelSize mRightContBorderWidth;
  BCPixelSize mTopContBorderWidth;
  BCPixelSize mLeftContBorderWidth;

  




  void InitHasCellWithStyleHeight(nsTableFrame* aTableFrame);

};

inline PRInt32 nsTableRowFrame::GetRowIndex() const
{
  return PRInt32(mBits.mRowIndex);
}

inline void nsTableRowFrame::SetRowIndex (int aRowIndex)
{
  mBits.mRowIndex = aRowIndex;
}

inline PRBool nsTableRowFrame::IsFirstInserted() const
{
  return PRBool(mBits.mFirstInserted);
}

inline void nsTableRowFrame::SetFirstInserted(PRBool aValue)
{
  mBits.mFirstInserted = aValue;
}

inline PRBool nsTableRowFrame::HasStyleHeight() const
{
  return (PRBool)mBits.mHasFixedHeight || (PRBool)mBits.mHasPctHeight;
}

inline PRBool nsTableRowFrame::HasFixedHeight() const
{
  return (PRBool)mBits.mHasFixedHeight;
}

inline void nsTableRowFrame::SetHasFixedHeight(PRBool aValue)
{
  mBits.mHasFixedHeight = aValue;
}

inline PRBool nsTableRowFrame::HasPctHeight() const
{
  return (PRBool)mBits.mHasPctHeight;
}

inline void nsTableRowFrame::SetHasPctHeight(PRBool aValue)
{
  mBits.mHasPctHeight = aValue;
}

inline nscoord nsTableRowFrame::GetContentHeight() const
{
  return mContentHeight;
}

inline void nsTableRowFrame::SetContentHeight(nscoord aValue)
{
  mContentHeight = aValue;
}

inline nscoord nsTableRowFrame::GetFixedHeight() const
{
  if (mBits.mHasFixedHeight)
    return mStyleFixedHeight;
  else
    return 0;
}

inline float nsTableRowFrame::GetPctHeight() const
{
  if (mBits.mHasPctHeight) 
    return (float)mStylePctHeight / 100.0f;
  else
    return 0.0f;
}

inline PRBool nsTableRowFrame::HasUnpaginatedHeight()
{
  return (mState & NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT) ==
         NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT;
}

inline void nsTableRowFrame::SetHasUnpaginatedHeight(PRBool aValue)
{
  if (aValue) {
    mState |= NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT;
  } else {
    mState &= ~NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT;
  }
}

inline nscoord nsTableRowFrame::GetTopBCBorderWidth()
{
  return mTopBorderWidth;
}

inline void nsTableRowFrame::SetTopBCBorderWidth(BCPixelSize aWidth)
{
  mTopBorderWidth = aWidth;
}

inline nscoord nsTableRowFrame::GetBottomBCBorderWidth()
{
  return mBottomBorderWidth;
}

inline void nsTableRowFrame::SetBottomBCBorderWidth(BCPixelSize aWidth)
{
  mBottomBorderWidth = aWidth;
}

inline nsMargin* nsTableRowFrame::GetBCBorderWidth(nsMargin& aBorder)
{
  aBorder.left = aBorder.right = 0;

  aBorder.top    = nsPresContext::CSSPixelsToAppUnits(mTopBorderWidth);
  aBorder.bottom = nsPresContext::CSSPixelsToAppUnits(mBottomBorderWidth);

  return &aBorder;
}

inline void
nsTableRowFrame::GetContinuousBCBorderWidth(nsMargin& aBorder)
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.right = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips,
                                            mLeftContBorderWidth);
  aBorder.top = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips,
                                            mTopContBorderWidth);
  aBorder.left = BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips,
                                            mRightContBorderWidth);
}

inline nscoord nsTableRowFrame::GetOuterTopContBCBorderWidth()
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  return BC_BORDER_TOP_HALF_COORD(aPixelsToTwips, mTopContBorderWidth);
}

#endif
