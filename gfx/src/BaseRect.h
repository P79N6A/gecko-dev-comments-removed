




































#ifndef MOZILLA_BASERECT_H_
#define MOZILLA_BASERECT_H_

#include "nsAlgorithm.h"

namespace mozilla {
























template <class T, class Sub, class Point, class SizeT, class Margin>
struct BaseRect {
  T x, y, width, height;

  
  BaseRect() : x(0), y(0), width(0), height(0) {}
  BaseRect(const Point& aOrigin, const SizeT &aSize) :
      x(aOrigin.x), y(aOrigin.y), width(aSize.width), height(aSize.height)
  {
  }
  BaseRect(T aX, T aY, T aWidth, T aHeight) :
      x(aX), y(aY), width(aWidth), height(aHeight)
  {
  }

  
  
  bool IsEmpty() const { return height <= 0 || width <= 0; }
  void SetEmpty() { width = height = 0; }

  
  
  
  bool Contains(const Sub& aRect) const
  {
    return aRect.IsEmpty() ||
           (x <= aRect.x && aRect.XMost() <= XMost() &&
            y <= aRect.y && aRect.YMost() <= YMost());
  }
  
  bool Contains(T aX, T aY) const
  {
    return x <= aX && aX + 1 <= XMost() &&
           y <= aY && aY + 1 <= YMost();
  }
  
  bool Contains(const Point& aPoint) const { return Contains(aPoint.x, aPoint.y); }

  
  
  
  bool Intersects(const Sub& aRect) const
  {
    return x < aRect.XMost() && aRect.x < XMost() &&
           y < aRect.YMost() && aRect.y < YMost();
  }
  
  
  
  
  Sub Intersect(const Sub& aRect) const
  {
    Sub result;
    result.x = NS_MAX(x, aRect.x);
    result.y = NS_MAX(y, aRect.y);
    result.width = NS_MIN(XMost(), aRect.XMost()) - result.x;
    result.height = NS_MIN(YMost(), aRect.YMost()) - result.y;
    if (result.width < 0 || result.height < 0) {
      result.SizeTo(0, 0);
    }
    return result;
  }
  
  
  
  
  
  
  bool IntersectRect(const Sub& aRect1, const Sub& aRect2)
  {
    *static_cast<Sub*>(this) = aRect1.Intersect(aRect2);
    return !IsEmpty();
  }

  
  
  
  
  Sub Union(const Sub& aRect) const
  {
    if (IsEmpty()) {
      return aRect;
    } else if (aRect.IsEmpty()) {
      return *static_cast<const Sub*>(this);
    } else {
      return UnionEdges(aRect);
    }
  }
  
  
  
  Sub UnionEdges(const Sub& aRect) const
  {
    Sub result;
    result.x = NS_MIN(x, aRect.x);
    result.y = NS_MIN(y, aRect.y);
    result.width = NS_MAX(XMost(), aRect.XMost()) - result.x;
    result.height = NS_MAX(YMost(), aRect.YMost()) - result.y;
    return result;
  }
  
  
  
  
  
  
  void UnionRect(const Sub& aRect1, const Sub& aRect2)
  {
    *static_cast<Sub*>(this) = aRect1.Union(aRect2);
  }

  
  
  
  
  
  void UnionRectEdges(const Sub& aRect1, const Sub& aRect2)
  {
    *static_cast<Sub*>(this) = aRect1.UnionEdges(aRect2);
  }

  void SetRect(T aX, T aY, T aWidth, T aHeight)
  {
    x = aX; y = aY; width = aWidth; height = aHeight;
  }
  void SetRect(const Point& aPt, const SizeT& aSize)
  {
    SetRect(aPt.x, aPt.y, aSize.width, aSize.height);
  }
  void MoveTo(T aX, T aY) { x = aX; y = aY; }
  void MoveTo(const Point& aPoint) { x = aPoint.x; y = aPoint.y; }
  void MoveBy(T aDx, T aDy) { x += aDx; y += aDy; }
  void MoveBy(const Point& aPoint) { x += aPoint.x; y += aPoint.y; }
  void SizeTo(T aWidth, T aHeight) { width = aWidth; height = aHeight; }
  void SizeTo(const SizeT& aSize) { width = aSize.width; height = aSize.height; }

  void Inflate(T aDx, T aDy)
  {
    x -= aDx;
    y -= aDy;
    width += 2 * aDx;
    height += 2 * aDy;
  }
  void Inflate(const Margin& aMargin)
  {
    x -= aMargin.left;
    y -= aMargin.top;
    width += aMargin.LeftRight();
    height += aMargin.TopBottom();
  }

  void Deflate(T aDx, T aDy)
  {
    x += aDx;
    y += aDy;
    width = NS_MAX(T(0), width - 2 * aDx);
    height = NS_MAX(T(0), height - 2 * aDy);
  }
  void Deflate(const Margin& aMargin)
  {
    x += aMargin.left;
    y += aMargin.top;
    width = NS_MAX(T(0), width - aMargin.LeftRight());
    height = NS_MAX(T(0), height - aMargin.TopBottom());
  }

  
  
  
  
  bool IsEqualEdges(const Sub& aRect) const
  {
    return x == aRect.x && y == aRect.y &&
           width == aRect.width && height == aRect.height;
  }
  
  
  bool IsEqualInterior(const Sub& aRect) const
  {
    return IsEqualEdges(aRect) || (IsEmpty() && aRect.IsEmpty());
  }

  Sub operator+(const Point& aPoint) const
  {
    return Sub(x + aPoint.x, y + aPoint.y, width, height);
  }
  Sub operator-(const Point& aPoint) const
  {
    return Sub(x - aPoint.x, y - aPoint.y, width, height);
  }
  Sub& operator+=(const Point& aPoint)
  {
    MoveBy(aPoint);
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const Point& aPoint)
  {
    MoveBy(-aPoint);
    return *static_cast<Sub*>(this);
  }

  
  Margin operator-(const Sub& aRect) const
  {
    return Margin(aRect.x - x, aRect.y - y,
                  XMost() - aRect.XMost(), YMost() - aRect.YMost());
  }

  
  Point TopLeft() const { return Point(x, y); }
  Point TopRight() const { return Point(XMost(), y); }
  Point BottomLeft() const { return Point(x, YMost()); }
  Point BottomRight() const { return Point(XMost(), YMost()); }
  Point Center() const { return Point(x, y) + Point(width, height)/2; }
  SizeT Size() const { return SizeT(width, height); }

  
  T X() const { return x; }
  T Y() const { return y; }
  T Width() const { return width; }
  T Height() const { return height; }
  T XMost() const { return x + width; }
  T YMost() const { return y + height; }

  
  
  void ScaleRoundOut(double aScale) { ScaleRoundOut(aScale, aScale); }
  void ScaleRoundOut(double aXScale, double aYScale)
  {
    T right = static_cast<T>(NS_ceil(double(XMost()) * aXScale));
    T bottom = static_cast<T>(NS_ceil(double(YMost()) * aYScale));
    x = static_cast<T>(NS_floor(double(x) * aXScale));
    y = static_cast<T>(NS_floor(double(y) * aYScale));
    width = right - x;
    height = bottom - y;
  }

private:
  
  
  bool operator==(const Sub& aRect) const { return false; }
  bool operator!=(const Sub& aRect) const { return false; }
};

}

#endif 
