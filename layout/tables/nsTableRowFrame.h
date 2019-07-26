



#ifndef nsTableRowFrame_h__
#define nsTableRowFrame_h__

#include "mozilla/Attributes.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTablePainter.h"

class  nsTableFrame;
class  nsTableCellFrame;
struct nsTableCellReflowState;











class nsTableRowFrame : public nsContainerFrame
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsTableRowFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual ~nsTableRowFrame();

  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) MOZ_OVERRIDE;
  
  virtual nsresult AppendFrames(ChildListID     aListID,
                                nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual nsresult InsertFrames(ChildListID     aListID,
                                nsIFrame*       aPrevFrame,
                                nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual nsresult RemoveFrame(ChildListID     aListID,
                               nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  




  friend nsIFrame* NS_NewTableRowFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  virtual nsMargin GetUsedMargin() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedBorder() const MOZ_OVERRIDE;
  virtual nsMargin GetUsedPadding() const MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  nsTableCellFrame* GetFirstCell() ;

  












  virtual nsresult Reflow(nsPresContext*          aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  void DidResize();

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
 
  void UpdateHeight(nscoord           aHeight,
                    nscoord           aAscent,
                    nscoord           aDescent,
                    nsTableFrame*     aTableFrame = nullptr,
                    nsTableCellFrame* aCellFrame  = nullptr);

  void ResetHeight(nscoord aRowStyleHeight);

  
  
  nscoord CalcHeight(const nsHTMLReflowState& aReflowState);

  

  




  nscoord GetMaxCellAscent() const;

  

  nscoord GetRowBaseline();
 
  
  virtual int32_t GetRowIndex() const;

  
  void SetRowIndex (int aRowIndex);

  
  nscoord ReflowCellFrame(nsPresContext*          aPresContext,
                          const nsHTMLReflowState& aReflowState,
                          bool                     aIsTopOfPage,
                          nsTableCellFrame*        aCellFrame,
                          nscoord                  aAvailableHeight,
                          nsReflowStatus&          aStatus);
  









  nscoord CollapseRowIfNecessary(nscoord aRowOffset,
                                 nscoord aWidth,
                                 bool    aCollapseGroup,
                                 bool& aDidCollapse);

  






  void InsertCellFrame(nsTableCellFrame* aFrame,
                       int32_t           aColIndex);

  nsresult CalculateCellActualHeight(nsTableCellFrame* aCellFrame,
                                     nscoord&          aDesiredHeight);

  bool IsFirstInserted() const;
  void   SetFirstInserted(bool aValue);

  nscoord GetContentHeight() const;
  void    SetContentHeight(nscoord aTwipValue);

  bool HasStyleHeight() const;

  bool HasFixedHeight() const;
  void   SetHasFixedHeight(bool aValue);

  bool HasPctHeight() const;
  void   SetHasPctHeight(bool aValue);

  nscoord GetFixedHeight() const;
  void    SetFixedHeight(nscoord aValue);

  float   GetPctHeight() const;
  void    SetPctHeight(float  aPctValue,
                       bool aForce = false);

  nscoord GetHeight(nscoord aBasis = 0) const;

  nsTableRowFrame* GetNextRow() const;

  bool    HasUnpaginatedHeight();
  void    SetHasUnpaginatedHeight(bool aValue);
  nscoord GetUnpaginatedHeight(nsPresContext* aPresContext);
  void    SetUnpaginatedHeight(nsPresContext* aPresContext, nscoord aValue);

  nscoord GetTopBCBorderWidth();
  void    SetTopBCBorderWidth(BCPixelSize aWidth);
  nscoord GetBottomBCBorderWidth();
  void    SetBottomBCBorderWidth(BCPixelSize aWidth);
  nsMargin* GetBCBorderWidth(nsMargin& aBorder);
                             
  





  void GetContinuousBCBorderWidth(nsMargin& aBorder);
  


  nscoord GetOuterTopContBCBorderWidth();
  



  void SetContinuousBCBorderWidth(uint8_t     aForSide,
                                  BCPixelSize aPixelValue);

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameForRemoval() MOZ_OVERRIDE { InvalidateFrameSubtree(); }

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

protected:

  


  nsTableRowFrame(nsStyleContext *aContext);

  void InitChildReflowState(nsPresContext&         aPresContext,
                            const nsSize&           aAvailSize,
                            bool                    aBorderCollapse,
                            nsTableCellReflowState& aReflowState);
  
  virtual int GetSkipSides(const nsHTMLReflowState* aReflowState = nullptr) const MOZ_OVERRIDE;

  

  nscoord ComputeCellXOffset(const nsHTMLReflowState& aState,
                             nsIFrame*                aKidFrame,
                             const nsMargin&          aKidMargin) const;
  



  nsresult ReflowChildren(nsPresContext*           aPresContext,
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

inline bool nsTableRowFrame::HasStyleHeight() const
{
  return (bool)mBits.mHasFixedHeight || (bool)mBits.mHasPctHeight;
}

inline bool nsTableRowFrame::HasFixedHeight() const
{
  return (bool)mBits.mHasFixedHeight;
}

inline void nsTableRowFrame::SetHasFixedHeight(bool aValue)
{
  mBits.mHasFixedHeight = aValue;
}

inline bool nsTableRowFrame::HasPctHeight() const
{
  return (bool)mBits.mHasPctHeight;
}

inline void nsTableRowFrame::SetHasPctHeight(bool aValue)
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

inline bool nsTableRowFrame::HasUnpaginatedHeight()
{
  return (mState & NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT) ==
         NS_TABLE_ROW_HAS_UNPAGINATED_HEIGHT;
}

inline void nsTableRowFrame::SetHasUnpaginatedHeight(bool aValue)
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
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.right = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips,
                                            mLeftContBorderWidth);
  aBorder.top = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips,
                                            mTopContBorderWidth);
  aBorder.left = BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips,
                                            mRightContBorderWidth);
}

inline nscoord nsTableRowFrame::GetOuterTopContBCBorderWidth()
{
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  return BC_BORDER_TOP_HALF_COORD(aPixelsToTwips, mTopContBorderWidth);
}

#endif
