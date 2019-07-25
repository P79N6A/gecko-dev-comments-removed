



































#ifndef nsTableColFrame_h__
#define nsTableColFrame_h__

#include "nscore.h"
#include "nsContainerFrame.h"
#include "nsTablePainter.h"
#include "nsTArray.h"

class nsTableCellFrame;

enum nsTableColType {
  eColContent            = 0, 
  eColAnonymousCol       = 1, 
  eColAnonymousColGroup  = 2, 
  eColAnonymousCell      = 3  
};

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
  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
  
  PRInt32 GetColIndex() const;
  
  void SetColIndex (PRInt32 aColIndex);

  nsTableColFrame* GetNextCol() const;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  


  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) { return NS_OK; }

  




  virtual nsIAtom* GetType() const;
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  virtual nsSplittableType GetSplittableType() const;

  
  PRInt32 GetSpan();

  
  PRInt32 Count() const;

  nscoord GetLeftBorderWidth();
  void    SetLeftBorderWidth(BCPixelSize aWidth);
  nscoord GetRightBorderWidth();
  void    SetRightBorderWidth(BCPixelSize aWidth);

  







  nscoord GetContinuousBCBorderWidth(nsMargin& aBorder);
  



  void SetContinuousBCBorderWidth(PRUint8     aForSide,
                                  BCPixelSize aPixelValue);
#ifdef DEBUG
  void Dump(PRInt32 aIndent);
#endif

  



  void ResetIntrinsics() {
    mMinCoord = 0;
    mPrefCoord = 0;
    mPrefPercent = 0.0f;
    mHasSpecifiedCoord = PR_FALSE;
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
                 PRBool aHasSpecifiedCoord) {
    NS_ASSERTION(aMinCoord <= aPrefCoord, "intrinsic widths out of order");

    if (aHasSpecifiedCoord && !mHasSpecifiedCoord) {
      mPrefCoord = mMinCoord;
      mHasSpecifiedCoord = PR_TRUE;
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
  



  PRBool GetHasSpecifiedCoord() const { return mHasSpecifiedCoord; }

  



  float GetPrefPercent() const { return mPrefPercent; }

  



  void AddSpanCoords(nscoord aSpanMinCoord, nscoord aSpanPrefCoord,
                     PRBool aSpanHasSpecifiedCoord) {
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

  
  void ResetFinalWidth() {
    mFinalWidth = nscoord_MIN; 
  }
  void SetFinalWidth(nscoord aFinalWidth) {
    mFinalWidth = aFinalWidth;
  }
  nscoord GetFinalWidth() {
    return mFinalWidth;
  }

protected:

  nsTableColFrame(nsStyleContext* aContext);
  ~nsTableColFrame();

  nscoord mMinCoord;
  nscoord mPrefCoord;
  nscoord mSpanMinCoord; 
  nscoord mSpanPrefCoord; 
  float mPrefPercent;
  float mSpanPrefPercent; 
  
  
  
  
  nscoord mFinalWidth;

  
  
  
  PRUint32 mColIndex:        16;
  
  
  BCPixelSize mLeftBorderWidth;
  BCPixelSize mRightBorderWidth;
  BCPixelSize mTopContBorderWidth;
  BCPixelSize mRightContBorderWidth;
  BCPixelSize mBottomContBorderWidth;

  PRPackedBool mHasSpecifiedCoord;
};

inline PRInt32 nsTableColFrame::GetColIndex() const
{
  return mColIndex; 
}

inline void nsTableColFrame::SetColIndex (PRInt32 aColIndex)
{ 
  mColIndex = aColIndex; 
}

inline nscoord nsTableColFrame::GetLeftBorderWidth()
{
  return mLeftBorderWidth;
}

inline void nsTableColFrame::SetLeftBorderWidth(BCPixelSize aWidth)
{
  mLeftBorderWidth = aWidth;
}

inline nscoord nsTableColFrame::GetRightBorderWidth()
{
  return mRightBorderWidth;
}

inline void nsTableColFrame::SetRightBorderWidth(BCPixelSize aWidth)
{
  mRightBorderWidth = aWidth;
}

inline nscoord
nsTableColFrame::GetContinuousBCBorderWidth(nsMargin& aBorder)
{
  PRInt32 aPixelsToTwips = nsPresContext::AppUnitsPerCSSPixel();
  aBorder.top = BC_BORDER_BOTTOM_HALF_COORD(aPixelsToTwips,
                                            mTopContBorderWidth);
  aBorder.right = BC_BORDER_LEFT_HALF_COORD(aPixelsToTwips,
                                            mRightContBorderWidth);
  aBorder.bottom = BC_BORDER_TOP_HALF_COORD(aPixelsToTwips,
                                            mBottomContBorderWidth);
  return BC_BORDER_RIGHT_HALF_COORD(aPixelsToTwips, mRightContBorderWidth);
}

#endif

