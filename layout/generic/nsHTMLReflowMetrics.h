






#ifndef nsHTMLReflowMetrics_h___
#define nsHTMLReflowMetrics_h___

#include "nsRect.h"
#include "nsBoundingMetrics.h"
#include "WritingModes.h"



struct nsHTMLReflowState;


#define NS_REFLOW_CALC_BOUNDING_METRICS  0x0001









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
    NS_ASSERTION(aIndex < 2, "index out of range");
    return mRects[aIndex];
  }
  const nsRect& Overflow(size_t aIndex) const {
    NS_ASSERTION(aIndex < 2, "index out of range");
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
    
    
    return VisualOverflow().IsEqualInterior(aOther.VisualOverflow()) &&
           ScrollableOverflow().IsEqualEdges(aOther.ScrollableOverflow());
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

    bool operator==(const nsCollapsingMargin& aOther)
      {
        return mMostPos == aOther.mMostPos &&
          mMostNeg == aOther.mMostNeg;
      }

    bool operator!=(const nsCollapsingMargin& aOther)
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

    bool IsZero() const
      {
        return (mMostPos == 0) && (mMostNeg == 0);
      }

    nscoord get() const
      {
        return mMostPos + mMostNeg;
      }
};







class nsHTMLReflowMetrics {
public:
  
  
  
  
  
  
  explicit nsHTMLReflowMetrics(mozilla::WritingMode aWritingMode, uint32_t aFlags = 0)
    : mISize(0)
    , mBSize(0)
    , mBlockStartAscent(ASK_FOR_BASELINE)
    , mFlags(aFlags)
    , mWritingMode(aWritingMode)
  {}

  explicit nsHTMLReflowMetrics(const nsHTMLReflowState& aState, uint32_t aFlags = 0);

  
  
  
  
  nscoord ISize(mozilla::WritingMode aWritingMode) const {
    CHECK_WRITING_MODE(aWritingMode);
    return mISize;
  }
  nscoord BSize(mozilla::WritingMode aWritingMode) const {
    CHECK_WRITING_MODE(aWritingMode);
    return mBSize;
  }
  mozilla::LogicalSize Size(mozilla::WritingMode aWritingMode) const {
    CHECK_WRITING_MODE(aWritingMode);
    return mozilla::LogicalSize(aWritingMode, mISize, mBSize);
  }

  nscoord& ISize(mozilla::WritingMode aWritingMode) {
    CHECK_WRITING_MODE(aWritingMode);
    return mISize;
  }
  nscoord& BSize(mozilla::WritingMode aWritingMode) {
    CHECK_WRITING_MODE(aWritingMode);
    return mBSize;
  }

  
  
  void SetSize(mozilla::WritingMode aWM, mozilla::LogicalSize aSize)
  {
    mozilla::LogicalSize convertedSize = aSize.ConvertTo(mWritingMode, aWM);
    mBSize = convertedSize.BSize(mWritingMode);
    mISize = convertedSize.ISize(mWritingMode);
  }

  
  void ClearSize()
  {
    mISize = mBSize = 0;
  }

  
  
  
  
  nscoord Width() const { return mWritingMode.IsVertical() ? mBSize : mISize; }
  nscoord Height() const { return mWritingMode.IsVertical() ? mISize : mBSize; }

  
  
  nscoord BlockStartAscent() const
  {
    return mBlockStartAscent;
  }

  nscoord& Width() { return mWritingMode.IsVertical() ? mBSize : mISize; }
  nscoord& Height() { return mWritingMode.IsVertical() ? mISize : mBSize; }

  void SetBlockStartAscent(nscoord aAscent)
  {
    mBlockStartAscent = aAscent;
  }

  enum { ASK_FOR_BASELINE = nscoord_MAX };

  
  
  
  
  
  
  nsBoundingMetrics mBoundingMetrics;  

  
  
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

  mozilla::WritingMode GetWritingMode() const { return mWritingMode; }

private:
  nscoord mISize, mBSize; 
  nscoord mBlockStartAscent; 

public:
  uint32_t mFlags;

private:
  mozilla::WritingMode mWritingMode;
};

#endif 
