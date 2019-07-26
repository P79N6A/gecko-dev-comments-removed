




#ifndef MOZILLA_SVGPOINT_H__
#define MOZILLA_SVGPOINT_H__

#include "nsDebug.h"
#include "gfxPoint.h"
#include "mozilla/gfx/Point.h"

namespace mozilla {






class SVGPoint
{
  typedef mozilla::gfx::Point Point;

public:

  SVGPoint()
    : mX(0.0f)
    , mY(0.0f)
  {}

  SVGPoint(float aX, float aY)
    : mX(aX)
    , mY(aY)
  {
    NS_ASSERTION(IsValid(), "Constructed an invalid SVGPoint");
  }

  SVGPoint(const SVGPoint &aOther)
    : mX(aOther.mX)
    , mY(aOther.mY)
  {}

  SVGPoint& operator=(const SVGPoint &rhs) {
    mX = rhs.mX;
    mY = rhs.mY;
    return *this;
  }

  bool operator==(const SVGPoint &rhs) const {
    return mX == rhs.mX && mY == rhs.mY;
  }

  SVGPoint& operator+=(const SVGPoint &rhs) {
    mX += rhs.mX;
    mY += rhs.mY;
    return *this;
  }

  operator gfxPoint() const {
    return gfxPoint(mX, mY);
  }

  operator Point() const {
    return Point(mX, mY);
  }

#ifdef DEBUG
  bool IsValid() const {
    return NS_finite(mX) && NS_finite(mY);
  }
#endif

  void SetX(float aX)
    { mX = aX; }
  void SetY(float aY)
    { mY = aY; }
  float GetX() const
    { return mX; }
  float GetY() const
    { return mY; }

  bool operator!=(const SVGPoint &rhs) const {
    return mX != rhs.mX || mY != rhs.mY;
  }

  float mX;
  float mY;
};

inline SVGPoint operator+(const SVGPoint& aP1,
                          const SVGPoint& aP2)
{
  return SVGPoint(aP1.mX + aP2.mX, aP1.mY + aP2.mY);
}

inline SVGPoint operator-(const SVGPoint& aP1,
                          const SVGPoint& aP2)
{
  return SVGPoint(aP1.mX - aP2.mX, aP1.mY - aP2.mY);
}

inline SVGPoint operator*(float aFactor,
                          const SVGPoint& aPoint)
{
  return SVGPoint(aFactor * aPoint.mX, aFactor * aPoint.mY);
}

inline SVGPoint operator*(const SVGPoint& aPoint,
                          float aFactor)
{
  return SVGPoint(aFactor * aPoint.mX, aFactor * aPoint.mY);
}

} 

#endif 
