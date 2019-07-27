



#ifndef nsTableRowGroupFrame_h__
#define nsTableRowGroupFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsIAtom.h"
#include "nsILineIterator.h"
#include "nsTablePainter.h"
#include "nsTArray.h"
#include "nsTableFrame.h"
#include "mozilla/WritingModes.h"

class nsTableRowFrame;

struct nsRowGroupReflowState {
  const nsHTMLReflowState& reflowState;  

  nsTableFrame* tableFrame;

  
  mozilla::LogicalSize availSize;

  
  nscoord bCoord;

  nsRowGroupReflowState(const nsHTMLReflowState& aReflowState,
                        nsTableFrame*            aTableFrame)
      : reflowState(aReflowState)
      , tableFrame(aTableFrame)
      , availSize(aReflowState.GetWritingMode(),
                  aReflowState.AvailableISize(),
                  aReflowState.AvailableBSize())
      , bCoord(0)
  {
  }

  ~nsRowGroupReflowState() {}
};

#define MIN_ROWS_NEEDING_CURSOR 20










class nsTableRowGroupFrame final
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

  nsTableFrame* GetTableFrame() const
  {
    nsIFrame* parent = GetParent();
    MOZ_ASSERT(parent && parent->GetType() == nsGkAtoms::tableFrame);
    return static_cast<nsTableFrame*>(parent);
  }

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;

  virtual nsMargin GetUsedMargin() const override;
  virtual nsMargin GetUsedBorder() const override;
  virtual nsMargin GetUsedPadding() const override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

   








  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  virtual bool UpdateOverflow() override;

  




  virtual nsIAtom* GetType() const override;

  nsTableRowFrame* GetFirstRow();

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual mozilla::WritingMode GetWritingMode() const override
    { return GetTableFrame()->GetWritingMode(); }

  
  int32_t GetRowCount();

  


  int32_t GetStartRowIndex();

  



  void AdjustRowIndices(int32_t   aRowIndex,
                        int32_t   anAdjustment);

  








  nsresult  InitRepeatedFrame(nsPresContext*        aPresContext,
                              nsTableRowGroupFrame* aHeaderFooterFrame);


  


  nscoord GetBSizeBasis(const nsHTMLReflowState& aReflowState);

  mozilla::LogicalMargin GetBCBorderWidth(mozilla::WritingMode aWM);

  





  void GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                  mozilla::LogicalMargin& aBorder);

  



  void SetContinuousBCBorderWidth(mozilla::LogicalSide aForSide,
                                  BCPixelSize aPixelValue);
  









  nscoord CollapseRowGroupIfNecessary(nscoord aBTotalOffset,
                                      nscoord aISize,
                                      mozilla::WritingMode aWM);


public:
  virtual void DisposeLineIterator() override { }

  
  
  
  
  
  

  


  virtual int32_t GetNumLines() override;

  


  virtual bool GetDirection() override;

  










  NS_IMETHOD GetLine(int32_t aLineNumber,
                     nsIFrame** aFirstFrameOnLine,
                     int32_t* aNumFramesOnLine,
                     nsRect& aLineBounds) override;

  






  virtual int32_t FindLineContaining(nsIFrame* aFrame, int32_t aStartLine = 0) override;

  










  NS_IMETHOD FindFrameAt(int32_t aLineNumber,
                         nsPoint aPos,
                         nsIFrame** aFrameFound,
                         bool* aPosIsBeforeFirstFrame,
                         bool* aPosIsAfterLastFrame) override;

   







  NS_IMETHOD CheckLineOrder(int32_t                  aLine,
                            bool                     *aIsReordered,
                            nsIFrame                 **aFirstVisual,
                            nsIFrame                 **aLastVisual) override;

  




  NS_IMETHOD GetNextSiblingOnLine(nsIFrame*& aFrame, int32_t aLineNumber) override;

  
  
  
  
  
  
  
  
  
  
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

  virtual nsILineIterator* GetLineIterator() override { return this; }

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameForRemoval() override { InvalidateFrameSubtree(); }

protected:
  explicit nsTableRowGroupFrame(nsStyleContext* aContext);

  void InitChildReflowState(nsPresContext&     aPresContext,
                            bool               aBorderCollapse,
                            nsHTMLReflowState& aReflowState);

  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

  void PlaceChild(nsPresContext*         aPresContext,
                  nsRowGroupReflowState& aReflowState,
                  nsIFrame*              aKidFrame,
                  mozilla::WritingMode   aWM,
                  const mozilla::LogicalPoint& aKidPosition,
                  nscoord                aContainerWidth,
                  nsHTMLReflowMetrics&   aDesiredSize,
                  const nsRect&          aOriginalKidRect,
                  const nsRect&          aOriginalKidVisualOverflow);

  void CalculateRowBSizes(nsPresContext*           aPresContext,
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
  
  BCPixelSize mIEndContBorderWidth;
  BCPixelSize mBEndContBorderWidth;
  BCPixelSize mIStartContBorderWidth;

public:
  bool IsRepeatable() const;
  void SetRepeatable(bool aRepeatable);
  bool HasStyleBSize() const;
  void SetHasStyleBSize(bool aValue);
  bool HasInternalBreakBefore() const;
  bool HasInternalBreakAfter() const;
};


inline bool nsTableRowGroupFrame::IsRepeatable() const
{
  return HasAnyStateBits(NS_ROWGROUP_REPEATABLE);
}

inline void nsTableRowGroupFrame::SetRepeatable(bool aRepeatable)
{
  if (aRepeatable) {
    AddStateBits(NS_ROWGROUP_REPEATABLE);
  } else {
    RemoveStateBits(NS_ROWGROUP_REPEATABLE);
  }
}

inline bool nsTableRowGroupFrame::HasStyleBSize() const
{
  return HasAnyStateBits(NS_ROWGROUP_HAS_STYLE_BSIZE);
}

inline void nsTableRowGroupFrame::SetHasStyleBSize(bool aValue)
{
  if (aValue) {
    AddStateBits(NS_ROWGROUP_HAS_STYLE_BSIZE);
  } else {
    RemoveStateBits(NS_ROWGROUP_HAS_STYLE_BSIZE);
  }
}

inline void
nsTableRowGroupFrame::GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                                 mozilla::LogicalMargin& aBorder)
{
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.IEnd(aWM) = BC_BORDER_START_HALF_COORD(aPixelsToTwips,
                                                 mIEndContBorderWidth);
  aBorder.BEnd(aWM) = BC_BORDER_START_HALF_COORD(aPixelsToTwips,
                                                 mBEndContBorderWidth);
  aBorder.IStart(aWM) = BC_BORDER_END_HALF_COORD(aPixelsToTwips,
                                                 mIStartContBorderWidth);
}
#endif
