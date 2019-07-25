




































#ifndef NSPOINT_H
#define NSPOINT_H

#include "nsCoord.h"
#include "mozilla/BaseSize.h"
#include "mozilla/BasePoint.h"
#include "nsSize.h"

struct nsIntPoint;

struct nsPoint : public mozilla::BasePoint<nscoord, nsPoint> {
  typedef mozilla::BasePoint<nscoord, nsPoint> Super;

  nsPoint() : Super() {}
  nsPoint(const nsPoint& aPoint) : Super(aPoint) {}
  nsPoint(nscoord aX, nscoord aY) : Super(aX, aY) {}

  inline nsIntPoint ToNearestPixels(nscoord aAppUnitsPerPixel) const;

  
  inline nsPoint ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const;
};

struct nsIntPoint : public mozilla::BasePoint<PRInt32, nsIntPoint> {
  typedef mozilla::BasePoint<PRInt32, nsIntPoint> Super;

  nsIntPoint() : Super() {}
  nsIntPoint(const nsIntPoint& aPoint) : Super(aPoint) {}
  nsIntPoint(PRInt32 aX, PRInt32 aY) : Super(aX, aY) {}
};

inline nsIntPoint
nsPoint::ToNearestPixels(nscoord aAppUnitsPerPixel) const {
  return nsIntPoint(
      NSToIntRoundUp(NSAppUnitsToDoublePixels(x, aAppUnitsPerPixel)),
      NSToIntRoundUp(NSAppUnitsToDoublePixels(y, aAppUnitsPerPixel)));
}

inline nsPoint
nsPoint::ConvertAppUnits(PRInt32 aFromAPP, PRInt32 aToAPP) const {
  if (aFromAPP != aToAPP) {
    nsPoint point;
    point.x = NSToCoordRound(NSCoordScale(x, aFromAPP, aToAPP));
    point.y = NSToCoordRound(NSCoordScale(y, aFromAPP, aToAPP));
    return point;
  }
  return *this;
}

#endif 
