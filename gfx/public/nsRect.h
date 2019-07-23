





































#ifndef NSRECT_H
#define NSRECT_H

#include <stdio.h>
#include "nsCoord.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsMargin.h"
#include "nsUnitConversion.h"
#include "gfxCore.h"
#include "nsTraceRefcnt.h"

struct NS_GFX nsRect {
  nscoord x, y;
  nscoord width, height;

  
  nsRect() : x(0), y(0), width(0), height(0) {
    MOZ_COUNT_CTOR(nsRect);
  }
  nsRect(const nsRect& aRect) {
    MOZ_COUNT_CTOR(nsRect);
    *this = aRect;
  }
  nsRect(const nsPoint& aOrigin, const nsSize &aSize) {
    MOZ_COUNT_CTOR(nsRect);
    x = aOrigin.x; y = aOrigin.y;
    width = aSize.width; height = aSize.height;
  }
  nsRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) {
    MOZ_COUNT_CTOR(nsRect);
    x = aX; y = aY; width = aWidth; height = aHeight;
    VERIFY_COORD(x); VERIFY_COORD(y); VERIFY_COORD(width); VERIFY_COORD(height);
  }

#ifdef NS_BUILD_REFCNT_LOGGING
  ~nsRect() {
    MOZ_COUNT_DTOR(nsRect);
  }
#endif
  
  
  
  PRBool IsEmpty() const {
    return (PRBool) ((height <= 0) || (width <= 0));
  }
  void   Empty() {width = height = 0;}

  
  PRBool Contains(const nsRect& aRect) const;
  PRBool Contains(nscoord aX, nscoord aY) const;
  PRBool Contains(const nsPoint& aPoint) const {return Contains(aPoint.x, aPoint.y);}

  
  
  PRBool Intersects(const nsRect& aRect) const;

  
  
  
  
  
  PRBool IntersectRect(const nsRect& aRect1, const nsRect& aRect2);

  
  
  
  
  
  PRBool UnionRect(const nsRect& aRect1, const nsRect& aRect2);

  
  void SetRect(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight) {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }
  void SetRect(const nsPoint& aPt, const nsSize& aSize) {
    SetRect(aPt.x, aPt.y, aSize.width, aSize.height);
  }
  void MoveTo(nscoord aX, nscoord aY) {x = aX; y = aY;}
  void MoveTo(const nsPoint& aPoint) {x = aPoint.x; y = aPoint.y;}
  void MoveBy(nscoord aDx, nscoord aDy) {x += aDx; y += aDy;}
  void MoveBy(const nsPoint& aPoint) {x += aPoint.x; y += aPoint.y;}
  void SizeTo(nscoord aWidth, nscoord aHeight) {width = aWidth; height = aHeight;}
  void SizeTo(const nsSize& aSize) {SizeTo(aSize.width, aSize.height);}
  void SizeBy(nscoord aDeltaWidth, nscoord aDeltaHeight) {width += aDeltaWidth;
                                                          height += aDeltaHeight;}

  
  void Inflate(nscoord aDx, nscoord aDy);
  void Inflate(const nsSize& aSize) {Inflate(aSize.width, aSize.height);}
  void Inflate(const nsMargin& aMargin);

  
  void Deflate(nscoord aDx, nscoord aDy);
  void Deflate(const nsSize& aSize) {Deflate(aSize.width, aSize.height);}
  void Deflate(const nsMargin& aMargin);

  
  
  PRBool  operator==(const nsRect& aRect) const {
    return (PRBool) ((IsEmpty() && aRect.IsEmpty()) ||
                     ((x == aRect.x) && (y == aRect.y) &&
                      (width == aRect.width) && (height == aRect.height)));
  }
  PRBool  operator!=(const nsRect& aRect) const {
    return (PRBool) !operator==(aRect);
  }

  nsRect  operator+(const nsPoint& aPoint) const {
    return nsRect(x + aPoint.x, y + aPoint.y, width, height);
  }
  nsRect  operator-(const nsPoint& aPoint) const {
    return nsRect(x - aPoint.x, y - aPoint.y, width, height);
  }
  nsRect& operator+=(const nsPoint& aPoint) {x += aPoint.x; y += aPoint.y; return *this;}
  nsRect& operator-=(const nsPoint& aPoint) {x -= aPoint.x; y -= aPoint.y; return *this;}

  nsRect& operator*=(const float aScale) {x = NSToCoordRound(x * aScale); 
                                          y = NSToCoordRound(y * aScale); 
                                          width = NSToCoordRound(width * aScale); 
                                          height = NSToCoordRound(height * aScale); 
                                          return *this;}

  nsRect& ScaleRoundOut(const float aScale);
  nsRect& ScaleRoundIn(const float aScale);

  
  nsPoint TopLeft() const { return nsPoint(x, y); }
  nsPoint TopRight() const { return nsPoint(XMost(), y); }
  nsPoint BottomLeft() const { return nsPoint(x, YMost()); }
  nsPoint BottomRight() const { return nsPoint(XMost(), YMost()); }

  nsSize Size() const { return nsSize(width, height); }

  
  nscoord XMost() const {return x + width;}
  nscoord YMost() const {return y + height;}
};

#ifdef NS_COORD_IS_FLOAT
struct NS_GFX nsIntRect {
  PRInt32 x, y;
  PRInt32 width, height;

  
  nsIntRect() : x(0), y(0), width(0), height(0) {}
  nsIntRect(const nsIntRect& aRect) {*this = aRect;}
  nsIntRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }

  
  
  PRBool IsEmpty() const {
    return (PRBool) ((height <= 0) || (width <= 0));
  }

  void SetRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }

  
  
  
  
  
  PRBool IntersectRect(const nsIntRect& aRect1, const nsIntRect& aRect2);

  
  
  
  
  
  PRBool UnionRect(const nsIntRect& aRect1, const nsIntRect& aRect2);

  
  PRInt32 XMost() const {return x + width;}
  PRInt32 YMost() const {return y + height;}
};
#else
typedef nsRect nsIntRect;
#endif

#ifdef DEBUG

extern NS_GFX FILE* operator<<(FILE* out, const nsRect& rect);
#endif 

#endif 
