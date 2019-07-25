





































#ifndef NSRECT_H
#define NSRECT_H

#include <stdio.h>
#include "nsCoord.h"
#include "nsPoint.h"
#include "nsSize.h"
#include "nsMargin.h"
#include "gfxCore.h"
#include "nsTraceRefcnt.h"

struct nsIntRect;

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
  void SetEmpty() {width = height = 0;}

  
  
  
  PRBool Contains(const nsRect& aRect) const;
  
  
  PRBool Contains(nscoord aX, nscoord aY) const;
  PRBool Contains(const nsPoint& aPoint) const {return Contains(aPoint.x, aPoint.y);}

  
  
  
  PRBool Intersects(const nsRect& aRect) const;

  
  
  
  
  
  PRBool IntersectRect(const nsRect& aRect1, const nsRect& aRect2);

  
  
  
  
  
  
  
  PRBool UnionRect(const nsRect& aRect1, const nsRect& aRect2);

  
  
  
  
  
  void UnionRectEdges(const nsRect& aRect1, const nsRect& aRect2);

  
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

  
  
  
  
  PRBool IsEqualEdges(const nsRect& aRect) const {
    return x == aRect.x && y == aRect.y &&
           width == aRect.width && height == aRect.height;
  }
  
  
  PRBool IsEqualInterior(const nsRect& aRect) const {
    return IsEqualEdges(aRect) || (IsEmpty() && aRect.IsEmpty());
  }

  
  nsRect  operator+(const nsPoint& aPoint) const {
    return nsRect(x + aPoint.x, y + aPoint.y, width, height);
  }
  nsRect  operator-(const nsPoint& aPoint) const {
    return nsRect(x - aPoint.x, y - aPoint.y, width, height);
  }
  nsRect& operator+=(const nsPoint& aPoint) {x += aPoint.x; y += aPoint.y; return *this;}
  nsRect& operator-=(const nsPoint& aPoint) {x -= aPoint.x; y -= aPoint.y; return *this;}

  
  nsMargin operator-(const nsRect& aRect) const; 
  nsRect& operator+=(const nsMargin& aMargin) { Inflate(aMargin); return *this; }
  nsRect& operator-=(const nsMargin& aMargin) { Deflate(aMargin); return *this; }
  nsRect  operator+(const nsMargin& aMargin) const { return nsRect(*this) += aMargin; }
  nsRect  operator-(const nsMargin& aMargin) const { return nsRect(*this) -= aMargin; }

  
  
  nsRect& ScaleRoundOut(float aScale) { return ScaleRoundOut(aScale, aScale); }
  nsRect& ScaleRoundOut(float aXScale, float aYScale);

  
  
  
  nsRect& ExtendForScaling(float aXMult, float aYMult);

  
  
  
  
  inline nsRect ConvertAppUnitsRoundOut(PRInt32 aFromAPP, PRInt32 aToAPP) const;
  inline nsRect ConvertAppUnitsRoundIn(PRInt32 aFromAPP, PRInt32 aToAPP) const;

  
  nsPoint TopLeft() const { return nsPoint(x, y); }
  nsPoint TopRight() const { return nsPoint(XMost(), y); }
  nsPoint BottomLeft() const { return nsPoint(x, YMost()); }
  nsPoint BottomRight() const { return nsPoint(XMost(), YMost()); }

  nsSize Size() const { return nsSize(width, height); }

  
  nscoord XMost() const {return x + width;}
  nscoord YMost() const {return y + height;}

  inline nsIntRect ToNearestPixels(nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToOutsidePixels(nscoord aAppUnitsPerPixel) const;
  inline nsIntRect ToInsidePixels(nscoord aAppUnitsPerPixel) const;
};

struct NS_GFX nsIntRect {
  PRInt32 x, y;
  PRInt32 width, height;

  
  nsIntRect() : x(0), y(0), width(0), height(0) {}
  nsIntRect(const nsIntRect& aRect) {*this = aRect;}
  nsIntRect(const nsIntPoint& aOrigin, const nsIntSize &aSize) {
    x = aOrigin.x; y = aOrigin.y;
    width = aSize.width; height = aSize.height;
  }
  nsIntRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }

  
  
  PRBool IsEmpty() const {
    return (PRBool) ((height <= 0) || (width <= 0));
  }
  void Empty() {width = height = 0;}

  
  void Inflate(PRInt32 aDx, PRInt32 aDy) {
    x -= aDx;
    y -= aDy;
    width += aDx*2;
    height += aDy*2;
  }
  void Inflate(const nsIntMargin &aMargin) {
    x -= aMargin.left;
    y -= aMargin.top;
    width += aMargin.left + aMargin.right;
    height += aMargin.top + aMargin.bottom;
  }

  
  void Deflate(PRInt32 aDx, PRInt32 aDy) {
    x += aDx;
    y += aDy;
    width -= aDx*2;
    height -= aDy*2;
  }
  void Deflate(const nsIntMargin &aMargin) {
    x += aMargin.left;
    y += aMargin.top;
    width -= (aMargin.left + aMargin.right);
    height -= (aMargin.top + aMargin.bottom);
  }

  
  
  PRBool operator==(const nsIntRect& aRect) const {
    return IsEqualEdges(aRect);
  }

  
  
  
  
  PRBool IsEqualEdges(const nsIntRect& aRect) const {
    return x == aRect.x && y == aRect.y &&
           width == aRect.width && height == aRect.height;
  }
  
  
  PRBool IsEqualInterior(const nsIntRect& aRect) const {
    return IsEqualEdges(aRect) || (IsEmpty() && aRect.IsEmpty());
  }

  nsIntRect  operator+(const nsIntPoint& aPoint) const {
    return nsIntRect(x + aPoint.x, y + aPoint.y, width, height);
  }
  nsIntRect  operator-(const nsIntPoint& aPoint) const {
    return nsIntRect(x - aPoint.x, y - aPoint.y, width, height);
  }
  nsIntRect& operator+=(const nsIntPoint& aPoint) {x += aPoint.x; y += aPoint.y; return *this;}
  nsIntRect& operator-=(const nsIntPoint& aPoint) {x -= aPoint.x; y -= aPoint.y; return *this;}

  void SetRect(PRInt32 aX, PRInt32 aY, PRInt32 aWidth, PRInt32 aHeight) {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }

  void MoveTo(PRInt32 aX, PRInt32 aY) {x = aX; y = aY;}
  void MoveTo(const nsIntPoint& aPoint) {x = aPoint.x; y = aPoint.y;}
  void MoveBy(PRInt32 aDx, PRInt32 aDy) {x += aDx; y += aDy;}
  void MoveBy(const nsIntPoint& aPoint) {x += aPoint.x; y += aPoint.y;}
  void SizeTo(PRInt32 aWidth, PRInt32 aHeight) {width = aWidth; height = aHeight;}
  void SizeTo(const nsIntSize& aSize) {SizeTo(aSize.width, aSize.height);}
  void SizeBy(PRInt32 aDeltaWidth, PRInt32 aDeltaHeight) {width += aDeltaWidth;
                                                          height += aDeltaHeight;}

  PRBool Contains(const nsIntRect& aRect) const
  {
    return aRect.IsEmpty() ||
        (PRBool) ((aRect.x >= x) && (aRect.y >= y) &&
                  (aRect.XMost() <= XMost()) && (aRect.YMost() <= YMost()));
  }
  PRBool Contains(PRInt32 aX, PRInt32 aY) const
  {
    return (PRBool) ((aX >= x) && (aY >= y) &&
                     (aX < XMost()) && (aY < YMost()));
  }
  PRBool Contains(const nsIntPoint& aPoint) const { return Contains(aPoint.x, aPoint.y); }

  
  
  PRBool Intersects(const nsIntRect& aRect) const {
    return (PRBool) ((x < aRect.XMost()) && (y < aRect.YMost()) &&
                     (aRect.x < XMost()) && (aRect.y < YMost()));
  }

  
  
  
  
  
  PRBool IntersectRect(const nsIntRect& aRect1, const nsIntRect& aRect2);

  
  
  
  
  
  PRBool UnionRect(const nsIntRect& aRect1, const nsIntRect& aRect2);

  
  nsIntPoint TopLeft() const { return nsIntPoint(x, y); }
  nsIntPoint TopRight() const { return nsIntPoint(XMost(), y); }
  nsIntPoint BottomLeft() const { return nsIntPoint(x, YMost()); }
  nsIntPoint BottomRight() const { return nsIntPoint(XMost(), YMost()); }

  nsIntSize Size() const { return nsIntSize(width, height); }

  
  PRInt32 XMost() const {return x + width;}
  PRInt32 YMost() const {return y + height;}

  inline nsRect ToAppUnits(nscoord aAppUnitsPerPixel) const;

  nsIntRect& ScaleRoundOut(float aXScale, float aYScale);

  
  
  static const nsIntRect& GetMaxSizedIntRect() { return kMaxSizedIntRect; }

protected:
  static const nsIntRect kMaxSizedIntRect;
};





inline nsRect
nsRect::ConvertAppUnitsRoundOut(PRInt32 aFromAPP, PRInt32 aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRect rect;
  nscoord right = NSToCoordCeil(NSCoordScale(XMost(), aFromAPP, aToAPP));
  nscoord bottom = NSToCoordCeil(NSCoordScale(YMost(), aFromAPP, aToAPP));
  rect.x = NSToCoordFloor(NSCoordScale(x, aFromAPP, aToAPP));
  rect.y = NSToCoordFloor(NSCoordScale(y, aFromAPP, aToAPP));
  rect.width = (right - rect.x);
  rect.height = (bottom - rect.y);

  return rect;
}

inline nsRect
nsRect::ConvertAppUnitsRoundIn(PRInt32 aFromAPP, PRInt32 aToAPP) const
{
  if (aFromAPP == aToAPP) {
    return *this;
  }

  nsRect rect;
  nscoord right = NSToCoordFloor(NSCoordScale(XMost(), aFromAPP, aToAPP));
  nscoord bottom = NSToCoordFloor(NSCoordScale(YMost(), aFromAPP, aToAPP));
  rect.x = NSToCoordCeil(NSCoordScale(x, aFromAPP, aToAPP));
  rect.y = NSToCoordCeil(NSCoordScale(y, aFromAPP, aToAPP));
  rect.width = (right - rect.x);
  rect.height = (bottom - rect.y);

  return rect;
}


inline nsIntRect
nsRect::ToNearestPixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntRoundUp(NSAppUnitsToDoublePixels(x, aAppUnitsPerPixel));
  rect.y = NSToIntRoundUp(NSAppUnitsToDoublePixels(y, aAppUnitsPerPixel));
  rect.width  = NSToIntRoundUp(NSAppUnitsToDoublePixels(XMost(),
                               aAppUnitsPerPixel)) - rect.x;
  rect.height = NSToIntRoundUp(NSAppUnitsToDoublePixels(YMost(),
                               aAppUnitsPerPixel)) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ToOutsidePixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntFloor(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)));
  rect.y = NSToIntFloor(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)));
  rect.width  = NSToIntCeil(NSAppUnitsToFloatPixels(XMost(),
                            float(aAppUnitsPerPixel))) - rect.x;
  rect.height = NSToIntCeil(NSAppUnitsToFloatPixels(YMost(),
                            float(aAppUnitsPerPixel))) - rect.y;
  return rect;
}


inline nsIntRect
nsRect::ToInsidePixels(nscoord aAppUnitsPerPixel) const
{
  nsIntRect rect;
  rect.x = NSToIntCeil(NSAppUnitsToFloatPixels(x, float(aAppUnitsPerPixel)));
  rect.y = NSToIntCeil(NSAppUnitsToFloatPixels(y, float(aAppUnitsPerPixel)));
  rect.width  = NSToIntFloor(NSAppUnitsToFloatPixels(XMost(),
                             float(aAppUnitsPerPixel))) - rect.x;
  rect.height = NSToIntFloor(NSAppUnitsToFloatPixels(YMost(),
                             float(aAppUnitsPerPixel))) - rect.y;
  return rect;
}


inline nsRect
nsIntRect::ToAppUnits(nscoord aAppUnitsPerPixel) const
{
  return nsRect(NSIntPixelsToAppUnits(x, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(y, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(width, aAppUnitsPerPixel),
                NSIntPixelsToAppUnits(height, aAppUnitsPerPixel));
}

#ifdef DEBUG

extern NS_GFX FILE* operator<<(FILE* out, const nsRect& rect);
#endif 

#endif 
