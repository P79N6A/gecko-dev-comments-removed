



#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "mozilla/Attributes.h"
#include "celldata.h"
#include "imgIContainer.h"
#include "nsITableCellLayout.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsStyleContext.h"
#include "nsIPercentBSizeObserver.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsTArray.h"
#include "nsTableRowFrame.h"
#include "mozilla/WritingModes.h"












class nsTableCellFrame : public nsContainerFrame,
                         public nsITableCellLayout,
                         public nsIPercentBSizeObserver
{
  typedef mozilla::image::DrawResult DrawResult;

protected:
  typedef mozilla::WritingMode WritingMode;
  typedef mozilla::LogicalSide LogicalSide;
  typedef mozilla::LogicalMargin LogicalMargin;

public:
  NS_DECL_QUERYFRAME_TARGET(nsTableCellFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  nsTableCellFrame(nsStyleContext* aContext, nsTableFrame* aTableFrame);
  ~nsTableCellFrame();

  nsTableRowFrame* GetTableRowFrame() const
  {
    nsIFrame* parent = GetParent();
    MOZ_ASSERT(parent && parent->GetType() == nsGkAtoms::tableRowFrame);
    return static_cast<nsTableRowFrame*>(parent);
  }

  nsTableFrame* GetTableFrame() const
  {
    return GetTableRowFrame()->GetTableFrame();
  }

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) override;

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

#ifdef DEBUG
  
  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;
#endif

  virtual nsContainerFrame* GetContentInsertionFrame() override {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual nsMargin GetUsedMargin() const override;

  virtual void NotifyPercentBSize(const nsHTMLReflowState& aReflowState) override;

  virtual bool NeedsToObserve(const nsHTMLReflowState& aReflowState) override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  DrawResult PaintCellBackground(nsRenderingContext& aRenderingContext,
                                 const nsRect& aDirtyRect, nsPoint aPt,
                                 uint32_t aFlags);

 
  virtual nsresult ProcessBorders(nsTableFrame* aFrame,
                                  nsDisplayListBuilder* aBuilder,
                                  const nsDisplayListSet& aLists);

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual IntrinsicISizeOffsetData IntrinsicISizeOffsets() override;

  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&      aStatus) override;

  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  
  
  
  
  
  virtual mozilla::WritingMode GetWritingMode() const override
    { return GetTableFrame()->GetWritingMode(); }

  void BlockDirAlignChild(mozilla::WritingMode aWM, nscoord aMaxAscent);

  




  virtual uint8_t GetVerticalAlign() const;

  bool HasVerticalAlignBaseline() const {
    return GetVerticalAlign() == NS_STYLE_VERTICAL_ALIGN_BASELINE;
  }

  bool CellHasVisibleContent(nscoord       aBSize,
                             nsTableFrame* tableFrame,
                             nsIFrame*     kidFrame);

  



  nscoord GetCellBaseline() const;

  





  virtual int32_t GetRowSpan();

  

  

  




  NS_IMETHOD GetCellIndexes(int32_t &aRowIndex, int32_t &aColIndex) override;

  
  virtual nsresult GetRowIndex(int32_t &aRowIndex) const override;

  





  virtual int32_t GetColSpan();

  
  virtual nsresult GetColIndex(int32_t &aColIndex) const override;
  void SetColIndex(int32_t aColIndex);

  
  inline nscoord GetPriorAvailISize();

  
  inline void SetPriorAvailISize(nscoord aPriorAvailISize);

  
  inline mozilla::LogicalSize GetDesiredSize();

  
  inline void SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize);

  bool GetContentEmpty();
  void SetContentEmpty(bool aContentEmpty);

  bool HasPctOverBSize();
  void SetHasPctOverBSize(bool aValue);

  nsTableCellFrame* GetNextCell() const;

  virtual LogicalMargin GetBorderWidth(WritingMode aWM) const;

  virtual DrawResult PaintBackground(nsRenderingContext& aRenderingContext,
                                     const nsRect&        aDirtyRect,
                                     nsPoint              aPt,
                                     uint32_t             aFlags);

  void DecorateForSelection(nsRenderingContext& aRenderingContext,
                            nsPoint              aPt);

  virtual bool UpdateOverflow() override;

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }
  
  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameForRemoval() override { InvalidateFrameSubtree(); }

protected:
  virtual LogicalSides
  GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

  






  virtual nsMargin GetBorderOverflow();

  friend class nsTableRowFrame;

  uint32_t     mColIndex;             

  nscoord      mPriorAvailISize;      
  mozilla::LogicalSize mDesiredSize;  
};

inline nscoord nsTableCellFrame::GetPriorAvailISize()
{ return mPriorAvailISize; }

inline void nsTableCellFrame::SetPriorAvailISize(nscoord aPriorAvailISize)
{ mPriorAvailISize = aPriorAvailISize; }

inline mozilla::LogicalSize nsTableCellFrame::GetDesiredSize()
{ return mDesiredSize; }

inline void nsTableCellFrame::SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize)
{
  mozilla::WritingMode wm = aDesiredSize.GetWritingMode();
  mDesiredSize = aDesiredSize.Size(wm).ConvertTo(GetWritingMode(), wm);
}

inline bool nsTableCellFrame::GetContentEmpty()
{
  return HasAnyStateBits(NS_TABLE_CELL_CONTENT_EMPTY);
}

inline void nsTableCellFrame::SetContentEmpty(bool aContentEmpty)
{
  if (aContentEmpty) {
    AddStateBits(NS_TABLE_CELL_CONTENT_EMPTY);
  } else {
    RemoveStateBits(NS_TABLE_CELL_CONTENT_EMPTY);
  }
}

inline bool nsTableCellFrame::HasPctOverBSize()
{
  return HasAnyStateBits(NS_TABLE_CELL_HAS_PCT_OVER_BSIZE);
}

inline void nsTableCellFrame::SetHasPctOverBSize(bool aValue)
{
  if (aValue) {
    AddStateBits(NS_TABLE_CELL_HAS_PCT_OVER_BSIZE);
  } else {
    RemoveStateBits(NS_TABLE_CELL_HAS_PCT_OVER_BSIZE);
  }
}


class nsBCTableCellFrame final : public nsTableCellFrame
{
  typedef mozilla::image::DrawResult DrawResult;
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsBCTableCellFrame(nsStyleContext* aContext, nsTableFrame* aTableFrame);

  ~nsBCTableCellFrame();

  virtual nsIAtom* GetType() const override;

  virtual nsMargin GetUsedBorder() const override;
  virtual bool GetBorderRadii(const nsSize& aFrameSize,
                              const nsSize& aBorderArea,
                              Sides aSkipSides,
                              nscoord aRadii[8]) const override;

  
  virtual LogicalMargin GetBorderWidth(WritingMode aWM) const override;

  
  BCPixelSize GetBorderWidth(LogicalSide aSide) const;

  
  void SetBorderWidth(LogicalSide aSide, BCPixelSize aPixelValue);

  virtual nsMargin GetBorderOverflow() override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual DrawResult PaintBackground(nsRenderingContext& aRenderingContext,
                                     const nsRect&        aDirtyRect,
                                     nsPoint              aPt,
                                     uint32_t             aFlags) override;

private:

  
  
  BCPixelSize mBStartBorder;
  BCPixelSize mIEndBorder;
  BCPixelSize mBEndBorder;
  BCPixelSize mIStartBorder;
};

#endif
