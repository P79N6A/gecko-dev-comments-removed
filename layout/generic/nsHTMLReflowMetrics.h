






































#ifndef nsHTMLReflowMetrics_h___
#define nsHTMLReflowMetrics_h___

#include <stdio.h>
#include "nsISupports.h"
#include "nsMargin.h"
#include "nsRect.h"

#include "nsIRenderingContext.h" 




#ifdef MOZ_MATHML
#define NS_REFLOW_CALC_BOUNDING_METRICS  0x0001
#endif










struct nsCollapsingMargin {
  private:
    nscoord mMostPos;  
    nscoord mMostNeg;  

  public:
    nsCollapsingMargin()
        : mMostPos(0),
          mMostNeg(0)
      {
      }

    nsCollapsingMargin(const nsCollapsingMargin& aOther)
        : mMostPos(aOther.mMostPos),
          mMostNeg(aOther.mMostNeg)
      {
      }

    PRBool operator==(const nsCollapsingMargin& aOther)
      {
        return mMostPos == aOther.mMostPos &&
          mMostNeg == aOther.mMostNeg;
      }

    PRBool operator!=(const nsCollapsingMargin& aOther)
      {
        return !(*this == aOther);
      }

    nsCollapsingMargin& operator=(const nsCollapsingMargin& aOther)
      {
        mMostPos = aOther.mMostPos;
        mMostNeg = aOther.mMostNeg;
        return *this;
      }

    void Include(nscoord aCoord)
      {
        if (aCoord > mMostPos)
          mMostPos = aCoord;
        else if (aCoord < mMostNeg)
          mMostNeg = aCoord;
      }

    void Include(const nsCollapsingMargin& aOther)
      {
        if (aOther.mMostPos > mMostPos)
          mMostPos = aOther.mMostPos;
        if (aOther.mMostNeg < mMostNeg)
          mMostNeg = aOther.mMostNeg;
      }

    void Zero()
      {
        mMostPos = 0;
        mMostNeg = 0;
      }

    PRBool IsZero() const
      {
        return (mMostPos == 0) && (mMostNeg == 0);
      }

    nscoord get() const
      {
        return mMostPos + mMostNeg;
      }
};







struct nsHTMLReflowMetrics {
  nscoord width, height;    
  nscoord ascent;           

  enum { ASK_FOR_BASELINE = nscoord_MAX };

#ifdef MOZ_MATHML
  
  
  
  
  
  
  nsBoundingMetrics mBoundingMetrics;  
#endif

  
  
  nsCollapsingMargin mCarriedOutBottomMargin;
  
  
  
  
  
  
  
  
  
  nsRect mOverflowArea;

  PRUint32 mFlags;
 
  
  
  
  nsHTMLReflowMetrics(PRUint32 aFlags = 0) {
    mFlags = aFlags;
    mOverflowArea.x = 0;
    mOverflowArea.y = 0;
    mOverflowArea.width = 0;
    mOverflowArea.height = 0;
#ifdef MOZ_MATHML
    mBoundingMetrics.Clear();
#endif

    
    
    
    width = height = 0;
    ascent = ASK_FOR_BASELINE;
  }

  nsHTMLReflowMetrics& operator=(const nsHTMLReflowMetrics& aOther)
  {
    mFlags = aOther.mFlags;
    mCarriedOutBottomMargin = aOther.mCarriedOutBottomMargin;
    mOverflowArea.x = aOther.mOverflowArea.x;
    mOverflowArea.y = aOther.mOverflowArea.y;
    mOverflowArea.width = aOther.mOverflowArea.width;
    mOverflowArea.height = aOther.mOverflowArea.height;
#ifdef MOZ_MATHML
    mBoundingMetrics = aOther.mBoundingMetrics;
#endif

    width = aOther.width;
    height = aOther.height;
    ascent = aOther.ascent;
    return *this;
  }

};

#endif 
