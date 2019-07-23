



































#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "nsITableCellLayout.h"
#include "nscore.h"
#include "nsHTMLContainerFrame.h"
#include "nsTableRowFrame.h"  
#include "nsStyleContext.h"
#include "nsIPercentHeightObserver.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"

class nsTableFrame;




#define NS_TABLE_CELL_CONTENT_EMPTY       0x80000000
#define NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT 0x10000000












class nsTableCellFrame : public nsHTMLContainerFrame, 
                         public nsITableCellLayout, 
                         public nsIPercentHeightObserver
{
public:

  
  NS_DECL_ISUPPORTS_INHERITED

  

  nsTableCellFrame(nsStyleContext* aContext);
  ~nsTableCellFrame();

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  
  
  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

  virtual nsMargin GetUsedMargin() const;

  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState);

  virtual PRBool NeedsToObserve(const nsHTMLReflowState& aReflowState);

  




  friend nsIFrame* NS_NewTableCellFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
                              
  void PaintCellBackground(nsIRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, nsPoint aPt);

  NS_IMETHOD SetSelected(nsPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual IntrinsicWidthOffsetData
    IntrinsicWidthOffsets(nsIRenderingContext* aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*      aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&      aStatus);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsContainingBlock() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  void VerticallyAlignChild(nscoord aMaxAscent);

  PRBool HasVerticalAlignBaseline();

  



  nscoord GetCellBaseline() const;

  





  virtual PRInt32 GetRowSpan();

  

  

  




  NS_IMETHOD GetCellIndexes(PRInt32 &aRowIndex, PRInt32 &aColIndex);

  
  virtual nsresult GetRowIndex(PRInt32 &aRowIndex) const;

  





  virtual PRInt32 GetColSpan();
  
  
  virtual nsresult GetColIndex(PRInt32 &aColIndex) const;
  void SetColIndex(PRInt32 aColIndex);

  
  inline nscoord GetPriorAvailWidth();
  
  
  inline void SetPriorAvailWidth(nscoord aPriorAvailWidth);

  
  inline nsSize GetDesiredSize();

  
  inline void SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize);

  PRBool GetContentEmpty();
  void SetContentEmpty(PRBool aContentEmpty);

  PRBool HasPctOverHeight();
  void SetHasPctOverHeight(PRBool aValue);

  nsTableCellFrame* GetNextCell() const;

  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const;

  virtual void PaintBackground(nsIRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect,
                               nsPoint              aPt);

  void DecorateForSelection(nsIRenderingContext& aRenderingContext,
                            nsPoint              aPt);
                                 
protected:
  
  virtual PRIntn GetSkipSides() const;

  virtual PRBool ParentDisablesSelection() const; 

  







  virtual void GetSelfOverflow(nsRect& aOverflowArea);

private:  

  
  nsIFrame* GetFrameAt(nsVoidArray* aList,  PRInt32 aIndex);

protected:

  friend class nsTableRowFrame;

  PRUint32     mColIndex;             

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

inline PRBool nsTableCellFrame::GetContentEmpty()
{
  return (mState & NS_TABLE_CELL_CONTENT_EMPTY) ==
         NS_TABLE_CELL_CONTENT_EMPTY;
}

inline void nsTableCellFrame::SetContentEmpty(PRBool aContentEmpty)
{
  if (aContentEmpty) {
    mState |= NS_TABLE_CELL_CONTENT_EMPTY;
  } else {
    mState &= ~NS_TABLE_CELL_CONTENT_EMPTY;
  }
}

inline PRBool nsTableCellFrame::HasPctOverHeight()
{
  return (mState & NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT) ==
         NS_TABLE_CELL_HAS_PCT_OVER_HEIGHT;
}

inline void nsTableCellFrame::SetHasPctOverHeight(PRBool aValue)
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

  nsBCTableCellFrame(nsStyleContext* aContext);

  ~nsBCTableCellFrame();

  virtual nsIAtom* GetType() const;

  virtual nsMargin GetUsedBorder() const;

  
  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const;

  
  BCPixelSize GetBorderWidth(PRUint8 aSide) const;

  
  void SetBorderWidth(PRUint8 aSide, BCPixelSize aPixelValue);

  virtual void GetSelfOverflow(nsRect& aOverflowArea);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual void PaintBackground(nsIRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect,
                               nsPoint              aPt);

private:
  
  
  
  BCPixelSize mTopBorder;
  BCPixelSize mRightBorder;
  BCPixelSize mBottomBorder;
  BCPixelSize mLeftBorder;
};

#endif
