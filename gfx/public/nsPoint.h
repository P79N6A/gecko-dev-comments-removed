




































#ifndef NSPOINT_H
#define NSPOINT_H

#include "nsCoord.h"

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
};

#ifdef NS_COORD_IS_FLOAT
struct nsIntPoint {
  PRInt32 x, y;

  
  nsIntPoint() {}
  nsIntPoint(const nsIntPoint& aPoint) { x = aPoint.x; y = aPoint.y;}
  nsIntPoint(PRInt32 aX, PRInt32 aY) { x = aX; y = aY;}

  void MoveTo(PRInt32 aX, PRInt32 aY) {x = aX; y = aY;}
};

typedef nsPoint nsFloatPoint;
#else
typedef nsPoint nsIntPoint;

struct nsFloatPoint {
  float x, y;

  
  nsFloatPoint() {}
  nsFloatPoint(const nsFloatPoint& aPoint) {x = aPoint.x; y = aPoint.y;}
  nsFloatPoint(float aX, float aY) {x = aX; y = aY;}

  void MoveTo(float aX, float aY) {x = aX; y = aY;}
  void MoveTo(nscoord aX, nscoord aY) {x = (float)aX; y = (float)aY;}
  void MoveBy(float aDx, float aDy) {x += aDx; y += aDy;}

  
  
  PRBool   operator==(const nsFloatPoint& aPoint) const {
    return (PRBool) ((x == aPoint.x) && (y == aPoint.y));
  }
  PRBool   operator!=(const nsFloatPoint& aPoint) const {
    return (PRBool) ((x != aPoint.x) || (y != aPoint.y));
  }
  nsFloatPoint operator+(const nsFloatPoint& aPoint) const {
    return nsFloatPoint(x + aPoint.x, y + aPoint.y);
  }
  nsFloatPoint operator-(const nsFloatPoint& aPoint) const {
    return nsFloatPoint(x - aPoint.x, y - aPoint.y);
  }
  nsFloatPoint& operator+=(const nsFloatPoint& aPoint) {
    x += aPoint.x;
    y += aPoint.y;
    return *this;
  }
  nsFloatPoint& operator-=(const nsFloatPoint& aPoint) {
    x -= aPoint.x;
    y -= aPoint.y;
    return *this;
  }
};
#endif 

#endif 
