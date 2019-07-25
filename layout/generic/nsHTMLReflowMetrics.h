






































#ifndef nsHTMLReflowMetrics_h___
#define nsHTMLReflowMetrics_h___

#include <stdio.h>
#include "nsISupports.h"
#include "nsMargin.h"
#include "nsRect.h"

#include "nsRenderingContext.h" 




#ifdef MOZ_MATHML
#define NS_REFLOW_CALC_BOUNDING_METRICS  0x0001
#endif









enum nsOverflowType { eVisualOverflow, eScrollableOverflow,
                      eOverflowType_LENGTH };

#define NS_FOR_FRAME_OVERFLOW_TYPES(var_)                                     \
  for (nsOverflowType var_ = nsOverflowType(0); var_ < 2;                     \
       var_ = nsOverflowType(var_ + 1))

struct nsOverflowAreas {
private:
  nsRect mRects[2];
public:
  nsRect& Overflow(size_t aIndex) {
    NS_ASSERTION(0 <= aIndex && aIndex < 2, "index out of range");
    return mRects[aIndex];
  }
  const nsRect& Overflow(size_t aIndex) const {
    NS_ASSERTION(0 <= aIndex && aIndex < 2, "index out of range");
    return mRects[aIndex];
  }

  nsRect& VisualOverflow() { return mRects[eVisualOverflow]; }
  const nsRect& VisualOverflow() const { return mRects[eVisualOverflow]; }

  nsRect& ScrollableOverflow() { return mRects[eScrollableOverflow]; }
  const nsRect& ScrollableOverflow() const { return mRects[eScrollableOverflow]; }

  nsOverflowAreas() {
    
  }

  nsOverflowAreas(const nsRect& aVisualOverflow,
                  const nsRect& aScrollableOverflow)
  {
    mRects[eVisualOverflow] = aVisualOverflow;
    mRects[eScrollableOverflow] = aScrollableOverflow;
  }

  nsOverflowAreas(const nsOverflowAreas& aOther) {
    *this = aOther;
  }

  nsOverflowAreas& operator=(const nsOverflowAreas& aOther) {
    mRects[0] = aOther.mRects[0];
    mRects[1] = aOther.mRects[1];
    return *this;
  }

  bool operator==(const nsOverflowAreas& aOther) const {
    
    
    return VisualOverflow() == aOther.VisualOverflow() &&
           ScrollableOverflow().IsExactEqual(aOther.ScrollableOverflow());
  }

  bool operator!=(const nsOverflowAreas& aOther) const {
    return !(*this == aOther);
  }

  nsOverflowAreas operator+(const nsPoint& aPoint) const {
    nsOverflowAreas result(*this);
    result += aPoint;
    return result;
  }

  nsOverflowAreas& operator+=(const nsPoint& aPoint) {
    mRects[0] += aPoint;
    mRects[1] += aPoint;
    return *this;
  }

  void Clear() {
    mRects[0].SetRect(0, 0, 0, 0);
    mRects[1].SetRect(0, 0, 0, 0);
  }

  
  void UnionWith(const nsOverflowAreas& aOther);

  
  void UnionAllWith(const nsRect& aRect);

  
  void SetAllTo(const nsRect& aRect);
};










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

  PRUint32 mFlags;

  enum { ASK_FOR_BASELINE = nscoord_MAX };

#ifdef MOZ_MATHML
  
  
  
  
  
  
  nsBoundingMetrics mBoundingMetrics;  
#endif

  
  
  nsCollapsingMargin mCarriedOutBottomMargin;

  
  
  
  
  
  
  
  
  nsOverflowAreas mOverflowAreas;

  nsRect& VisualOverflow()
    { return mOverflowAreas.VisualOverflow(); }
  const nsRect& VisualOverflow() const
    { return mOverflowAreas.VisualOverflow(); }
  nsRect& ScrollableOverflow()
    { return mOverflowAreas.ScrollableOverflow(); }
  const nsRect& ScrollableOverflow() const
    { return mOverflowAreas.ScrollableOverflow(); }

  
  void SetOverflowAreasToDesiredBounds();

  
  void UnionOverflowAreasWithDesiredBounds();

  
  
  
  
  
  
  nsHTMLReflowMetrics(PRUint32 aFlags = 0)
    : width(0), height(0), ascent(ASK_FOR_BASELINE), mFlags(aFlags)
  {}
};

#endif 
