




































#ifndef GFX_LINESEGMENT_H
#define GFX_LINESEGMENT_H

#include "gfxTypes.h"
#include "gfxPoint.h"

struct THEBES_API gfxLineSegment {
  gfxLineSegment(const gfxPoint &aStart, const gfxPoint &aEnd) 
    : mStart(aStart)
    , mEnd(aEnd)
  {}

  bool PointsOnSameSide(const gfxPoint& aOne, const gfxPoint& aTwo)
  {
    
  
    gfxFloat deltaY = (mEnd.y - mStart.y);
    gfxFloat deltaX = (mEnd.x - mStart.x);
  
    gfxFloat one = deltaX * (aOne.y - mStart.y) - deltaY * (aOne.x - mStart.x);
    gfxFloat two = deltaX * (aTwo.y - mStart.y) - deltaY * (aTwo.x - mStart.x);

    
    

    if ((one >= 0 && two >= 0) || (one <= 0 && two <= 0))
      return true;
    return false;
  }

  






  bool Intersects(const gfxLineSegment& aOther, gfxPoint& aIntersection)
  {
    gfxFloat denominator = (aOther.mEnd.y - aOther.mStart.y) * (mEnd.x - mStart.x ) - 
                           (aOther.mEnd.x - aOther.mStart.x ) * (mEnd.y - mStart.y);

    
    
    if (!denominator) {
      return false;
    }

    gfxFloat anumerator = (aOther.mEnd.x - aOther.mStart.x) * (mStart.y - aOther.mStart.y) -
                         (aOther.mEnd.y - aOther.mStart.y) * (mStart.x - aOther.mStart.x);
  
    gfxFloat bnumerator = (mEnd.x - mStart.x) * (mStart.y - aOther.mStart.y) -
                         (mEnd.y - mStart.y) * (mStart.x - aOther.mStart.x);

    gfxFloat ua = anumerator / denominator;
    gfxFloat ub = bnumerator / denominator;

    if (ua <= 0.0 || ua >= 1.0 ||
        ub <= 0.0 || ub >= 1.0) {
      
      return false;
    }

    aIntersection = mStart + (mEnd - mStart) * ua;  
    return true;
  }

  gfxPoint mStart;
  gfxPoint mEnd;
};

#endif 
