




































#ifndef NSPOINT_H
#define NSPOINT_H

#include "nsCoord.h"

struct nsIntPoint;

struct nsPoint {
  nscoord x, y;

  
  nsPoint() {}
  nsPoint(const nsPoint& aPoint) { x = aPoint.x; y = aPoint.y;}
  nsPoint(nscoord aX, nscoord aY) { VERIFY_COORD(aX); VERIFY_COORD(aY); x = aX; y = aY;}

  void MoveTo(nscoord aX, nscoord aY) {x = aX; y = aY;}
  void MoveBy(nscoord aDx, nscoord aDy) {x += aDx; y += aDy;}

  
  
  PRBool   operator==(const nsPoint& aPoint) const {
    return (PRBool) ((x == aPoint.x) && (y == aPoint.y));
  }
  PRBool   operator!=(const nsPoint& aPoint) const {
    return (PRBool) ((x != aPoint.x) || (y != aPoint.y));
  }
  nsPoint operator+(const nsPoint& aPoint) const {
    return nsPoint(x + aPoint.x, y + aPoint.y);
  }
  nsPoint operator-(const nsPoint& aPoint) const {
    return nsPoint(x - aPoint.x, y - aPoint.y);
  }
  nsPoint& operator+=(const nsPoint& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    return *this;
  }
  nsPoint& operator-=(const nsPoint& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    return *this;
  }

  nsPoint operator-() const {
    return nsPoint(-x, -y);
  }

  inline nsIntPoint ToNearestPixels(nscoord aAppUnitsPerPixel) const;
};

struct nsIntPoint {
  PRInt32 x, y;

  
  nsIntPoint() {}
  nsIntPoint(const nsIntPoint& aPoint) { x = aPoint.x; y = aPoint.y;}
  nsIntPoint(PRInt32 aX, PRInt32 aY) { x = aX; y = aY;}

  PRBool   operator==(const nsIntPoint& aPoint) const {
    return (PRBool) ((x == aPoint.x) && (y == aPoint.y));
  }
  PRBool   operator!=(const nsIntPoint& aPoint) const {
    return (PRBool) ((x != aPoint.x) || (y != aPoint.y));
  }
  nsIntPoint operator+(const nsIntPoint& aPoint) const {
    return nsIntPoint(x + aPoint.x, y + aPoint.y);
  }
  nsIntPoint operator-(const nsIntPoint& aPoint) const {
    return nsIntPoint(x - aPoint.x, y - aPoint.y);
  }
  nsIntPoint& operator+=(const nsIntPoint& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    return *this;
  }
  nsIntPoint& operator-=(const nsIntPoint& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    return *this;
  }
  nsIntPoint operator-() const {
    return nsIntPoint(-x, -y);
  }
  void MoveTo(PRInt32 aX, PRInt32 aY) {x = aX; y = aY;}
};

inline nsIntPoint
nsPoint::ToNearestPixels(nscoord aAppUnitsPerPixel) const {
  return nsIntPoint(
      NSToIntRound(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel))),
      NSToIntRound(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel))));
}

#endif 
