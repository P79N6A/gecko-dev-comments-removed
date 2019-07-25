




































#ifndef GFX_QUAD_H
#define GFX_QUAD_H

#include "nsMathUtils.h"
#include "mozilla/gfx/BaseSize.h"
#include "mozilla/gfx/BasePoint.h"
#include "nsSize.h"
#include "nsPoint.h"

#include "gfxTypes.h"

static PRBool SameSideOfLine(const gfxPoint& aPoint1, const gfxPoint& aPoint2, const gfxPoint& aTest, const gfxPoint& aRef)
{
  
  
  gfxFloat deltaY = (aPoint2.y - aPoint1.y);
  gfxFloat deltaX = (aPoint2.x - aPoint1.x);
  
  gfxFloat test = deltaX * (aTest.y - aPoint1.y) - deltaY * (aTest.x - aPoint1.x);
  gfxFloat ref = deltaX * (aRef.y - aPoint1.y) - deltaY * (aRef.x - aPoint1.x);

  
  

  if ((test >= 0 && ref >= 0) || (test <= 0 && ref <= 0))
    return PR_TRUE;
  return PR_FALSE;
}

struct THEBES_API gfxQuad {
    gfxQuad(const gfxPoint& aOne, const gfxPoint& aTwo, const gfxPoint& aThree, const gfxPoint& aFour)
    {
        mPoints[0] = aOne;
        mPoints[1] = aTwo;
        mPoints[2] = aThree;
        mPoints[3] = aFour;
    }

    PRBool Contains(const gfxPoint& aPoint)
    {
        return (SameSideOfLine(mPoints[0], mPoints[1], aPoint, mPoints[2]) &&
                SameSideOfLine(mPoints[1], mPoints[2], aPoint, mPoints[3]) &&
                SameSideOfLine(mPoints[2], mPoints[3], aPoint, mPoints[0]) &&
                SameSideOfLine(mPoints[3], mPoints[0], aPoint, mPoints[1]));
    }

    gfxPoint mPoints[4];
};

#endif 
