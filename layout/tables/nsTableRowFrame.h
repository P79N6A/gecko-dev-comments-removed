



#ifndef nsTableRowFrame_h__
#define nsTableRowFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTablePainter.h"
#include "nsTableRowGroupFrame.h"
#include "mozilla/WritingModes.h"

class  nsTableCellFrame;
struct nsTableCellReflowState;











class nsTableRowFrame : public nsContainerFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsTableRowFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual ~nsTableRowFrame();

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;
  
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) override;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) override;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) override;

  




  friend nsTableRowFrame* NS_NewTableRowFrame(nsIPresShell* aPresShell,
                                              nsStyleContext* aContext);

  nsTableRowGroupFrame* GetTableRowGroupFrame() const
  {
    nsIFrame* parent = GetParent();
    MOZ_ASSERT(parent && parent->GetType() == nsGkAtoms::tableRowGroupFrame);
    return static_cast<nsTableRowGroupFrame*>(parent);
  }

  nsTableFrame* GetTableFrame() const
  {
    return GetTableRowGroupFrame()->GetTableFrame();
  }

  virtual nsMargin GetUsedMargin() const override;
  virtual nsMargin GetUsedBorder() const override;
  virtual nsMargin GetUsedPadding() const override;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  nsTableCellFrame* GetFirstCell() ;

  












  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  void DidResize();

  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual mozilla::WritingMode GetWritingMode() const override
    { return GetTableFrame()->GetWritingMode(); }
 
  void UpdateBSize(nscoord           aBSize,
                   nscoord           aAscent,
                   nscoord           aDescent,
                   nsTableFrame*     aTableFrame = nullptr,
                   nsTableCellFrame* aCellFrame  = nullptr);

  void ResetBSize(nscoord aRowStyleBSize);

  
  
  nscoord CalcBSize(const nsHTMLReflowState& aReflowState);

  

  




  nscoord GetMaxCellAscent() const;

  

  nscoord GetRowBaseline(mozilla::WritingMode aWritingMode);
 
  
  virtual int32_t GetRowIndex() const;

  
  void SetRowIndex (int aRowIndex);

  
  nscoord ReflowCellFrame(nsPresContext*           aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          bool                     aIsTopOfPage,
                          nsTableCellFrame*        aCellFrame,
                          nscoord                  aAvailableBSize,
                          nsReflowStatus&          aStatus);
  









  nscoord CollapseRowIfNecessary(nscoord aRowOffset,
                                 nscoord aISize,
                                 bool    aCollapseGroup,
                                 bool&   aDidCollapse);

  






  void InsertCellFrame(nsTableCellFrame* aFrame,
                       int32_t           aColIndex);

  nsresult CalculateCellActualBSize(nsTableCellFrame*    aCellFrame,
                                    nscoord&             aDesiredBSize,
                                    mozilla::WritingMode aWM);

  bool IsFirstInserted() const;
  void   SetFirstInserted(bool aValue);

  nscoord GetContentBSize() const;
  void    SetContentBSize(nscoord aTwipValue);

  bool HasStyleBSize() const;

  bool HasFixedBSize() const;
  void   SetHasFixedBSize(bool aValue);

  bool HasPctBSize() const;
  void   SetHasPctBSize(bool aValue);

  nscoord GetFixedBSize() const;
  void    SetFixedBSize(nscoord aValue);

  float   GetPctBSize() const;
  void    SetPctBSize(float  aPctValue,
                       bool aForce = false);

  nscoord GetInitialBSize(nscoord aBasis = 0) const;

  nsTableRowFrame* GetNextRow() const;

  bool    HasUnpaginatedBSize();
  void    SetHasUnpaginatedBSize(bool aValue);
  nscoord GetUnpaginatedBSize(nsPresContext* aPresContext);
  void    SetUnpaginatedBSize(nsPresContext* aPresContext, nscoord aValue);

  nscoord GetBStartBCBorderWidth() const { return mBStartBorderWidth; }
  nscoord GetBEndBCBorderWidth() const { return mBEndBorderWidth; }
  void SetBStartBCBorderWidth(BCPixelSize aWidth) { mBStartBorderWidth = aWidth; }
  void SetBEndBCBorderWidth(BCPixelSize aWidth) { mBEndBorderWidth = aWidth; }
  mozilla::LogicalMargin GetBCBorderWidth(mozilla::WritingMode aWM);
                             
  





  void GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                  mozilla::LogicalMargin& aBorder);

  


  nscoord GetOuterBStartContBCBorderWidth();
  



  void SetContinuousBCBorderWidth(mozilla::LogicalSide aForSide,
                                  BCPixelSize aPixelValue);

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameForRemoval() override { InvalidateFrameSubtree(); }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

protected:

  


  explicit nsTableRowFrame(nsStyleContext *aContext);

  void InitChildReflowState(nsPresContext&              aPresContext,
                            const mozilla::LogicalSize& aAvailSize,
                            bool                        aBorderCollapse,
                            nsTableCellReflowState&     aReflowState);
  
  virtual LogicalSides GetLogicalSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const override;

  

  nscoord ComputeCellXOffset(const nsHTMLReflowState& aState,
                             nsIFrame*                aKidFrame,
                             const nsMargin&          aKidMargin) const;
  



  void ReflowChildren(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsTableFrame&            aTableFrame,
                      nsReflowStatus&          aStatus);

private:
  struct RowBits {
    unsigned mRowIndex:29;
    unsigned mHasFixedBSize:1; 
    unsigned mHasPctBSize:1;   
    unsigned mFirstInserted:1; 
  } mBits;

  
  nscoord mContentBSize;
  
  
  nscoord mStylePctBSize;
  
  
  nscoord mStyleFixedBSize;

  
  nscoord mMaxCellAscent;  
  nscoord mMaxCellDescent; 

  
  
  BCPixelSize mBStartBorderWidth;
  BCPixelSize mBEndBorderWidth;
  BCPixelSize mIEndContBorderWidth;
  BCPixelSize mBStartContBorderWidth;
  BCPixelSize mIStartContBorderWidth;

  




  void InitHasCellWithStyleBSize(nsTableFrame* aTableFrame);

};

inline int32_t nsTableRowFrame::GetRowIndex() const
{
  return int32_t(mBits.mRowIndex);
}

inline void nsTableRowFrame::SetRowIndex (int aRowIndex)
{
  mBits.mRowIndex = aRowIndex;
}

inline bool nsTableRowFrame::IsFirstInserted() const
{
  return bool(mBits.mFirstInserted);
}

inline void nsTableRowFrame::SetFirstInserted(bool aValue)
{
  mBits.mFirstInserted = aValue;
}

inline bool nsTableRowFrame::HasStyleBSize() const
{
  return (bool)mBits.mHasFixedBSize || (bool)mBits.mHasPctBSize;
}

inline bool nsTableRowFrame::HasFixedBSize() const
{
  return (bool)mBits.mHasFixedBSize;
}

inline void nsTableRowFrame::SetHasFixedBSize(bool aValue)
{
  mBits.mHasFixedBSize = aValue;
}

inline bool nsTableRowFrame::HasPctBSize() const
{
  return (bool)mBits.mHasPctBSize;
}

inline void nsTableRowFrame::SetHasPctBSize(bool aValue)
{
  mBits.mHasPctBSize = aValue;
}

inline nscoord nsTableRowFrame::GetContentBSize() const
{
  return mContentBSize;
}

inline void nsTableRowFrame::SetContentBSize(nscoord aValue)
{
  mContentBSize = aValue;
}

inline nscoord nsTableRowFrame::GetFixedBSize() const
{
  if (mBits.mHasFixedBSize) {
    return mStyleFixedBSize;
  }
  return 0;
}

inline float nsTableRowFrame::GetPctBSize() const
{
  if (mBits.mHasPctBSize) {
    return (float)mStylePctBSize / 100.0f;
  }
  return 0.0f;
}

inline bool nsTableRowFrame::HasUnpaginatedBSize()
{
  return HasAnyStateBits(NS_TABLE_ROW_HAS_UNPAGINATED_BSIZE);
}

inline void nsTableRowFrame::SetHasUnpaginatedBSize(bool aValue)
{
  if (aValue) {
    AddStateBits(NS_TABLE_ROW_HAS_UNPAGINATED_BSIZE);
  } else {
    RemoveStateBits(NS_TABLE_ROW_HAS_UNPAGINATED_BSIZE);
  }
}

inline mozilla::LogicalMargin
nsTableRowFrame::GetBCBorderWidth(mozilla::WritingMode aWM)
{
  return mozilla::LogicalMargin(
    aWM, nsPresContext::CSSPixelsToAppUnits(mBStartBorderWidth), 0,
    nsPresContext::CSSPixelsToAppUnits(mBEndBorderWidth), 0);
}

inline void
nsTableRowFrame::GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                            mozilla::LogicalMargin& aBorder)
{
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.IEnd(aWM) = BC_BORDER_START_HALF_COORD(aPixelsToTwips,
                                                 mIStartContBorderWidth);
  aBorder.BStart(aWM) = BC_BORDER_END_HALF_COORD(aPixelsToTwips,
                                                 mBStartContBorderWidth);
  aBorder.IStart(aWM) = BC_BORDER_END_HALF_COORD(aPixelsToTwips,
                                                 mIEndContBorderWidth);
}

inline nscoord nsTableRowFrame::GetOuterBStartContBCBorderWidth()
{
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  return BC_BORDER_START_HALF_COORD(aPixelsToTwips, mBStartContBorderWidth);
}

#endif
