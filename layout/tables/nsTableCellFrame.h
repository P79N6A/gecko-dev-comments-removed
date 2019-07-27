



#ifndef nsTableCellFrame_h__
#define nsTableCellFrame_h__

#include "mozilla/Attributes.h"
#include "celldata.h"
#include "nsITableCellLayout.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsStyleContext.h"
#include "nsIPercentHeightObserver.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsTArray.h"

class nsTableFrame;












class nsTableCellFrame : public nsContainerFrame,
                         public nsITableCellLayout,
                         public nsIPercentHeightObserver
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsTableCellFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  

  explicit nsTableCellFrame(nsStyleContext* aContext);
  ~nsTableCellFrame();

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;

#ifdef DEBUG
  
  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;
#endif

  virtual nsContainerFrame* GetContentInsertionFrame() MOZ_OVERRIDE {
    return GetFirstPrincipalChild()->GetContentInsertionFrame();
  }

  virtual nsMargin GetUsedMargin() const MOZ_OVERRIDE;

  virtual void NotifyPercentHeight(const nsHTMLReflowState& aReflowState) MOZ_OVERRIDE;

  virtual bool NeedsToObserve(const nsHTMLReflowState& aReflowState) MOZ_OVERRIDE;

  




  friend nsIFrame* NS_NewTableCellFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  void PaintCellBackground(nsRenderingContext& aRenderingContext,
                           const nsRect& aDirtyRect, nsPoint aPt,
                           uint32_t aFlags);

 
  virtual nsresult ProcessBorders(nsTableFrame* aFrame,
                                  nsDisplayListBuilder* aBuilder,
                                  const nsDisplayListSet& aLists);

  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual IntrinsicISizeOffsetData
    IntrinsicISizeOffsets(nsRenderingContext* aRenderingContext) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*      aPresContext,
                      nsHTMLReflowMetrics& aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&      aStatus) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  void VerticallyAlignChild(nscoord aMaxAscent);

  




  virtual uint8_t GetVerticalAlign() const;

  bool HasVerticalAlignBaseline() const {
    return GetVerticalAlign() == NS_STYLE_VERTICAL_ALIGN_BASELINE;
  }

  bool CellHasVisibleContent(nscoord       height,
                               nsTableFrame* tableFrame,
                               nsIFrame*     kidFrame);

  



  nscoord GetCellBaseline() const;

  





  virtual int32_t GetRowSpan();

  

  

  




  NS_IMETHOD GetCellIndexes(int32_t &aRowIndex, int32_t &aColIndex) MOZ_OVERRIDE;

  
  virtual nsresult GetRowIndex(int32_t &aRowIndex) const MOZ_OVERRIDE;

  





  virtual int32_t GetColSpan();

  
  virtual nsresult GetColIndex(int32_t &aColIndex) const MOZ_OVERRIDE;
  void SetColIndex(int32_t aColIndex);

  
  inline nscoord GetPriorAvailWidth();

  
  inline void SetPriorAvailWidth(nscoord aPriorAvailWidth);

  
  inline mozilla::LogicalSize GetDesiredSize();

  
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

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }
  
  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameForRemoval() MOZ_OVERRIDE { InvalidateFrameSubtree(); }

protected:
  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState= nullptr) const MOZ_OVERRIDE;

  






  virtual nsMargin GetBorderOverflow();

  friend class nsTableRowFrame;

  uint32_t     mColIndex;             

  nscoord      mPriorAvailWidth;      
  mozilla::LogicalSize mDesiredSize;  
};

inline nscoord nsTableCellFrame::GetPriorAvailWidth()
{ return mPriorAvailWidth;}

inline void nsTableCellFrame::SetPriorAvailWidth(nscoord aPriorAvailWidth)
{ mPriorAvailWidth = aPriorAvailWidth;}

inline mozilla::LogicalSize nsTableCellFrame::GetDesiredSize()
{ return mDesiredSize; }

inline void nsTableCellFrame::SetDesiredSize(const nsHTMLReflowMetrics & aDesiredSize)
{
  mozilla::WritingMode wm = aDesiredSize.GetWritingMode();
  mDesiredSize = aDesiredSize.Size(wm).ConvertTo(GetWritingMode(), wm);
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


class nsBCTableCellFrame MOZ_FINAL : public nsTableCellFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsBCTableCellFrame(nsStyleContext* aContext);

  ~nsBCTableCellFrame();

  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual nsMargin GetUsedBorder() const MOZ_OVERRIDE;
  virtual bool GetBorderRadii(const nsSize& aFrameSize,
                              const nsSize& aBorderArea,
                              Sides aSkipSides,
                              nscoord aRadii[8]) const MOZ_OVERRIDE;

  
  virtual nsMargin* GetBorderWidth(nsMargin& aBorder) const MOZ_OVERRIDE;

  
  BCPixelSize GetBorderWidth(mozilla::css::Side aSide) const;

  
  void SetBorderWidth(mozilla::css::Side aSide, BCPixelSize aPixelValue);

  virtual nsMargin GetBorderOverflow() MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  virtual void PaintBackground(nsRenderingContext& aRenderingContext,
                               const nsRect&        aDirtyRect,
                               nsPoint              aPt,
                               uint32_t             aFlags) MOZ_OVERRIDE;

private:

  
  
  BCPixelSize mTopBorder;
  BCPixelSize mRightBorder;
  BCPixelSize mBottomBorder;
  BCPixelSize mLeftBorder;
};

#endif
