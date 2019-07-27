



#ifndef nsTableRowGroupFrame_h__
#define nsTableRowGroupFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsIAtom.h"
#include "nsILineIterator.h"
#include "nsTablePainter.h"
#include "nsTArray.h"

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
    availSize.width  = reflowState.AvailableWidth();
    availSize.height = reflowState.AvailableHeight();
    y = 0;  
  }

  ~nsRowGroupReflowState() {}
};

#define MIN_ROWS_NEEDING_CURSOR 20










class nsTableRowGroupFrame MOZ_FINAL
  : public nsContainerFrame
  , public nsILineIterator
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsTableRowGroupFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  




  friend nsTableRowGroupFrame* NS_NewTableRowGroupFrame(nsIPresShell* aPresShell,
                                                        nsStyleContext* aContext);
  virtual ~nsTableRowGroupFrame();

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;
  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual nsMargin GetUsedMargin() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedBorder() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedPadding() const MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

   








  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  nsTableRowFrame* GetFirstRow();

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  
  int32_t GetRowCount();

  


  int32_t GetStartRowIndex();

  



  void AdjustRowIndices(int32_t   aRowIndex,
                        int32_t   anAdjustment);

  








  nsresult  InitRepeatedFrame(nsPresContext*        aPresContext,
                              nsTableRowGroupFrame* aHeaderFooterFrame);

  
  


  nscoord GetHeightBasis(const nsHTMLReflowState& aReflowState);
  
  nsMargin* GetBCBorderWidth(nsMargin& aBorder);

  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  



  void SetContinuousBCBorderWidth(uint8_t     aForSide,
                                  BCPixelSize aPixelValue);
  








  nscoord CollapseRowGroupIfNecessary(nscoord aYTotalOffset,
                                      nscoord aWidth);


public:
  virtual void DisposeLineIterator() MOZ_OVERRIDE { }

  
  
  
  
  
  
   
  


  virtual int32_t GetNumLines() MOZ_OVERRIDE;

  


  virtual bool GetDirection() MOZ_OVERRIDE;
  
  











  NS_IMETHOD GetLine(int32_t aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     int32_t* aNumFramesOnLine,
                     nsRect& aLineBounds,
                     uint32_t* aLineFlags) MOZ_OVERRIDE;
  
  






  virtual int32_t FindLineContaining(nsIFrame* aFrame, int32_t aStartLine = 0) MOZ_OVERRIDE;

  










  NS_IMETHOD FindFrameAt(int32_t aLineNumber,
                         nscoord aX,
                         nsIFrame** aFrameFound,
                         bool* aXIsBeforeFirstFrame,
                         bool* aXIsAfterLastFrame) MOZ_OVERRIDE;

   







  NS_IMETHOD CheckLineOrder(int32_t                  aLine,
                            bool                     *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) MOZ_OVERRIDE;

  



  
  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, int32_t aLineNumber) MOZ_OVERRIDE;

  
  
  
  
  
  
  
  
  
  
  struct FrameCursorData {
    nsTArray<nsIFrame*> mFrames;
    uint32_t            mCursorIndex;
    nscoord             mOverflowAbove;
    nscoord             mOverflowBelow;
    
    FrameCursorData()
      : mFrames(MIN_ROWS_NEEDING_CURSOR), mCursorIndex(0), mOverflowAbove(0),
        mOverflowBelow(0) {}

    bool AppendFrame(nsIFrame* aFrame);
    
    void FinishBuildingCursor() {
      mFrames.Compact();
    }
  };

  
  void ClearRowCursor();

  










  nsIFrame* GetFirstRowContaining(nscoord aY, nscoord* aOverflowAbove);

  






  FrameCursorData* SetupRowCursor();

  virtual nsILineIterator* GetLineIterator() MOZ_OVERRIDE { return this; }

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameForRemoval() MOZ_OVERRIDE { InvalidateFrameSubtree(); }

protected:
  explicit nsTableRowGroupFrame(nsStyleContext* aContext);

  void InitChildReflowState(nsPresContext&     aPresContext, 
                            bool               aBorderCollapse,
                            nsHTMLReflowState& aReflowState);
  
  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

  void PlaceChild(nsPresContext*         aPresContext,
                  nsRowGroupReflowState& aReflowState,
                  nsIFrame*              aKidFrame,
                  nsHTMLReflowMetrics&   aDesiredSize,
                  const nsRect&          aOriginalKidRect,
                  const nsRect&          aOriginalKidVisualOverflow);

  void CalculateRowHeights(nsPresContext*           aPresContext, 
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState);

  void DidResizeRows(nsHTMLReflowMetrics& aDesiredSize);

  void SlideChild(nsRowGroupReflowState& aReflowState,
                  nsIFrame*              aKidFrame);
  
  





  void ReflowChildren(nsPresContext*         aPresContext,
                      nsHTMLReflowMetrics&   aDesiredSize,
                      nsRowGroupReflowState& aReflowState,
                      nsReflowStatus&        aStatus,
                      bool*                aPageBreakBeforeEnd = nullptr);

  nsresult SplitRowGroup(nsPresContext*           aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsTableFrame*            aTableFrame,
                         nsReflowStatus&          aStatus,
                         bool                     aRowForcedPageBreak);

  void SplitSpanningCells(nsPresContext&           aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          nsTableFrame&            aTableFrame,
                          nsTableRowFrame&         aFirstRow, 
                          nsTableRowFrame&         aLastRow,  
                          bool                     aFirstRowIsTopOfPage,
                          nscoord                  aSpanningRowBottom,
                          nsTableRowFrame*&        aContRowFrame,
                          nsTableRowFrame*&        aFirstTruncatedRow,
                          nscoord&                 aDesiredHeight);

  void CreateContinuingRowFrame(nsPresContext& aPresContext,
                                nsIFrame&      aRowFrame,
                                nsIFrame**     aContRowFrame);

  bool IsSimpleRowFrame(nsTableFrame* aTableFrame, 
                          nsIFrame*     aFrame);

  void GetNextRowSibling(nsIFrame** aRowFrame);

  void UndoContinuedRow(nsPresContext*   aPresContext,
                        nsTableRowFrame* aRow);
                        
private:
  
  BCPixelSize mRightContBorderWidth;
  BCPixelSize mBottomContBorderWidth;
  BCPixelSize mLeftContBorderWidth;

public:
  bool IsRepeatable() const;
  void   SetRepeatable(bool aRepeatable);
  bool HasStyleHeight() const;
  void   SetHasStyleHeight(bool aValue);
  bool HasInternalBreakBefore() const;
  bool HasInternalBreakAfter() const;
};


inline bool nsTableRowGroupFrame::IsRepeatable() const
{
  return (mState & NS_ROWGROUP_REPEATABLE) == NS_ROWGROUP_REPEATABLE;
}

inline void nsTableRowGroupFrame::SetRepeatable(bool aRepeatable)
{
  if (aRepeatable) {
    mState |= NS_ROWGROUP_REPEATABLE;
  } else {
    mState &= ~NS_ROWGROUP_REPEATABLE;
  }
}

inline bool nsTableRowGroupFrame::HasStyleHeight() const
{
  return (mState & NS_ROWGROUP_HAS_STYLE_HEIGHT) == NS_ROWGROUP_HAS_STYLE_HEIGHT;
}

inline void nsTableRowGroupFrame::SetHasStyleHeight(bool aValue)
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
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.right = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips,
                                            mRightContBorderWidth);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips,
                                            mBottomContBorderWidth);
  aBorder.left = BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips,
                                            mLeftContBorderWidth);
  return;
}
#endif
