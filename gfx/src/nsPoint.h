




#ifndef NSPOINT_H
#define NSPOINT_H

#include "nsCoord.h"
#include "mozilla/gfx/BaseSize.h"
#include "mozilla/gfx/BasePoint.h"
#include "nsSize.h"
#include "mozilla/gfx/Point.h"




typedef mozilla::gfx::IntPoint nsIntPoint;



struct nsPoint : public mozilla::gfx::BasePoint<nscoord, nsPoint> {
  typedef mozilla::gfx::BasePoint<nscoord, nsPoint> Super;

  nsPoint() : Super() {}
  nsPoint(const nsPoint& aPoint) : Super(aPoint) {}
  nsPoint(nscoord aX, nscoord aY) : Super(aX, aY) {}

  inline nsIntPoint ScaleToNearestPixels(float aXScale, float aYScale,
                                         nscoord aAppUnitsPerPixel) const;
  inline nsIntPoint ToNearestPixels(nscoord aAppUnitsPerPixel) const;

  




  MOZ_WARN_UNUSED_RESULT inline nsPoint
    ScaleToOtherAppUnits(int32_t aFromAPP, int32_t aToAPP) const;
};

inline nsPoint ToAppUnits(const nsIntPoint& aPoint, nscoord aAppUnitsPerPixel);

inline nsIntPoint
nsPoint::ScaleToNearestPixels(float aXScale, float aYScale,
                              nscoord aAppUnitsPerPixel) const
{
  return nsIntPoint(
      NSToIntRoundUp(NSAppUnitsToDoublePixels(x, aAppUnitsPerPixel) * aXScale),
      NSToIntRoundUp(NSAppUnitsToDoublePixels(y, aAppUnitsPerPixel) * aYScale));
}

inline nsIntPoint
nsPoint::ToNearestPixels(nscoord aAppUnitsPerPixel) const
{
  return ScaleToNearestPixels(1.0f, 1.0f, aAppUnitsPerPixel);
}

inline nsPoint
nsPoint::ScaleToOtherAppUnits(int32_t aFromAPP, int32_t aToAPP) const
{
  if (aFromAPP != aToAPP) {
    nsPoint point;
    point.x = NSToCoordRound(NSCoordScale(x, aFromAPP, aToAPP));
    point.y = NSToCoordRound(NSCoordScale(y, aFromAPP, aToAPP));
    return point;
  }
  return *this;
}


inline nsPoint
ToAppUnits(const nsIntPoint& aPoint, nscoord aAppUnitsPerPixel)
{
  return nsPoint(NSIntPixelsToAppUnits(aPoint.x, aAppUnitsPerPixel),
                 NSIntPixelsToAppUnits(aPoint.y, aAppUnitsPerPixel));
}

#endif 
