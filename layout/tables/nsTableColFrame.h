



#ifndef nsTableColFrame_h__
#define nsTableColFrame_h__

#include "mozilla/Attributes.h"
#include "celldata.h"
#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTArray.h"
#include "nsTableColGroupFrame.h"
#include "mozilla/WritingModes.h"

class nsTableColFrame : public nsSplittableFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  enum {eWIDTH_SOURCE_NONE          =0,   
        eWIDTH_SOURCE_CELL          =1,   
        eWIDTH_SOURCE_CELL_WITH_SPAN=2    
  };

  nsTableColType GetColType() const;
  void SetColType(nsTableColType aType);

  




  friend nsTableColFrame* NS_NewTableColFrame(nsIPresShell* aPresShell,
                                              nsStyleContext*  aContext);

  nsTableColGroupFrame* GetTableColGroupFrame() const
  {
    nsIFrame* parent = GetParent();
    MOZ_ASSERT(parent && parent->GetType() == nsGkAtoms::tableColGroupFrame);
    return static_cast<nsTableColGroupFrame*>(parent);
  }

  nsTableFrame* GetTableFrame() const
  {
    return GetTableColGroupFrame()->GetTableFrame();
  }

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext) override;

  int32_t GetColIndex() const;

  void SetColIndex (int32_t aColIndex);

  nsTableColFrame* GetNextCol() const;

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) override;

  


  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override {}

  




  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

  virtual nsSplittableType GetSplittableType() const override;

  virtual mozilla::WritingMode GetWritingMode() const override
    { return GetTableFrame()->GetWritingMode(); }

  
  int32_t GetSpan();

  
  int32_t Count() const;

  nscoord GetIStartBorderWidth() const { return mIStartBorderWidth; }
  nscoord GetIEndBorderWidth() const { return mIEndBorderWidth; }
  void SetIStartBorderWidth(BCPixelSize aWidth) { mIStartBorderWidth = aWidth; }
  void SetIEndBorderWidth(BCPixelSize aWidth) { mIEndBorderWidth = aWidth; }

  







  nscoord GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                     mozilla::LogicalMargin& aBorder);
  



  void SetContinuousBCBorderWidth(mozilla::LogicalSide aForSide,
                                  BCPixelSize aPixelValue);
#ifdef DEBUG
  void Dump(int32_t aIndent);
#endif

  



  void ResetIntrinsics() {
    mMinCoord = 0;
    mPrefCoord = 0;
    mPrefPercent = 0.0f;
    mHasSpecifiedCoord = false;
  }

  



  void ResetPrefPercent() {
    mPrefPercent = 0.0f;
  }

  



  void ResetSpanIntrinsics() {
    mSpanMinCoord = 0;
    mSpanPrefCoord = 0;
    mSpanPrefPercent = 0.0f;
  }

  



















  void AddCoords(nscoord aMinCoord, nscoord aPrefCoord,
                 bool aHasSpecifiedCoord) {
    NS_ASSERTION(aMinCoord <= aPrefCoord, "intrinsic widths out of order");

    if (aHasSpecifiedCoord && !mHasSpecifiedCoord) {
      mPrefCoord = mMinCoord;
      mHasSpecifiedCoord = true;
    }
    if (!aHasSpecifiedCoord && mHasSpecifiedCoord) {
      aPrefCoord = aMinCoord; 
    }

    if (aMinCoord > mMinCoord)
      mMinCoord = aMinCoord;
    if (aPrefCoord > mPrefCoord)
      mPrefCoord = aPrefCoord;

    NS_ASSERTION(mMinCoord <= mPrefCoord, "min larger than pref");
  }

  




  void AddPrefPercent(float aPrefPercent) {
    if (aPrefPercent > mPrefPercent)
      mPrefPercent = aPrefPercent;
  }

  


  nscoord GetMinCoord() const { return mMinCoord; }
  




  nscoord GetPrefCoord() const { return mPrefCoord; }
  



  bool GetHasSpecifiedCoord() const { return mHasSpecifiedCoord; }

  



  float GetPrefPercent() const { return mPrefPercent; }

  



  void AddSpanCoords(nscoord aSpanMinCoord, nscoord aSpanPrefCoord,
                     bool aSpanHasSpecifiedCoord) {
    NS_ASSERTION(aSpanMinCoord <= aSpanPrefCoord,
                 "intrinsic widths out of order");

    if (!aSpanHasSpecifiedCoord && mHasSpecifiedCoord) {
      aSpanPrefCoord = aSpanMinCoord; 
    }

    if (aSpanMinCoord > mSpanMinCoord)
      mSpanMinCoord = aSpanMinCoord;
    if (aSpanPrefCoord > mSpanPrefCoord)
      mSpanPrefCoord = aSpanPrefCoord;

    NS_ASSERTION(mSpanMinCoord <= mSpanPrefCoord, "min larger than pref");
  }

  



  void AddSpanPrefPercent(float aSpanPrefPercent) {
    if (aSpanPrefPercent > mSpanPrefPercent)
      mSpanPrefPercent = aSpanPrefPercent;
  }

  



  void AccumulateSpanIntrinsics() {
    AddCoords(mSpanMinCoord, mSpanPrefCoord, mHasSpecifiedCoord);
    AddPrefPercent(mSpanPrefPercent);
  }

  
  
  
  void AdjustPrefPercent(float *aTableTotalPercent) {
    float allowed = 1.0f - *aTableTotalPercent;
    if (mPrefPercent > allowed)
      mPrefPercent = allowed;
    *aTableTotalPercent += mPrefPercent;
  }

  
  void ResetFinalISize() {
    mFinalISize = nscoord_MIN; 
  }
  void SetFinalISize(nscoord aFinalISize) {
    mFinalISize = aFinalISize;
  }
  nscoord GetFinalISize() {
    return mFinalISize;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eTablePart));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameForRemoval() override { InvalidateFrameSubtree(); }

protected:

  explicit nsTableColFrame(nsStyleContext* aContext);
  ~nsTableColFrame();

  nscoord mMinCoord;
  nscoord mPrefCoord;
  nscoord mSpanMinCoord; 
  nscoord mSpanPrefCoord; 
  float mPrefPercent;
  float mSpanPrefPercent; 
  
  
  
  
  nscoord mFinalISize;

  
  
  
  uint32_t mColIndex;

  
  BCPixelSize mIStartBorderWidth;
  BCPixelSize mIEndBorderWidth;
  BCPixelSize mBStartContBorderWidth;
  BCPixelSize mIEndContBorderWidth;
  BCPixelSize mBEndContBorderWidth;

  bool mHasSpecifiedCoord;
};

inline int32_t nsTableColFrame::GetColIndex() const
{
  return mColIndex;
}

inline void nsTableColFrame::SetColIndex (int32_t aColIndex)
{
  mColIndex = aColIndex;
}

inline nscoord
nsTableColFrame::GetContinuousBCBorderWidth(mozilla::WritingMode aWM,
                                            mozilla::LogicalMargin& aBorder)
{
  int32_t aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.BStart(aWM) = BC_BORDER_END_HALF_COORD(aPixelsToTwips,
                                                 mBStartContBorderWidth);
  aBorder.IEnd(aWM) = BC_BORDER_START_HALF_COORD(aPixelsToTwips,
                                                 mIEndContBorderWidth);
  aBorder.BEnd(aWM) = BC_BORDER_START_HALF_COORD(aPixelsToTwips,
                                                 mBEndContBorderWidth);
  return BC_BORDER_END_HALF_COORD(aPixelsToTwips, mIEndContBorderWidth);
}

#endif

