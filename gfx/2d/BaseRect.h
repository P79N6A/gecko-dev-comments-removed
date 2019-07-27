




#ifndef MOZILLA_GFX_BASERECT_H_
#define MOZILLA_GFX_BASERECT_H_

#include <algorithm>
#include <cmath>
#include <ostream>

#include "mozilla/Assertions.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypeTraits.h"

namespace mozilla {
namespace gfx {
























template <class T, class Sub, class Point, class SizeT, class MarginT>
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

  
  bool IsFinite() const
  {
    typedef typename mozilla::Conditional<mozilla::IsSame<T, float>::value, float, double>::Type FloatType;
    return (mozilla::IsFinite(FloatType(x)) &&
            mozilla::IsFinite(FloatType(y)) &&
            mozilla::IsFinite(FloatType(width)) &&
            mozilla::IsFinite(FloatType(height)));
  }

  
  
  
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
    return !IsEmpty() && !aRect.IsEmpty() &&
           x < aRect.XMost() && aRect.x < XMost() &&
           y < aRect.YMost() && aRect.y < YMost();
  }
  
  
  
  
  Sub Intersect(const Sub& aRect) const
  {
    Sub result;
    result.x = std::max<T>(x, aRect.x);
    result.y = std::max<T>(y, aRect.y);
    result.width = std::min<T>(XMost(), aRect.XMost()) - result.x;
    result.height = std::min<T>(YMost(), aRect.YMost()) - result.y;
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
    result.x = std::min(x, aRect.x);
    result.y = std::min(y, aRect.y);
    result.width = std::max(XMost(), aRect.XMost()) - result.x;
    result.height = std::max(YMost(), aRect.YMost()) - result.y;
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

  void Inflate(T aD) { Inflate(aD, aD); }
  void Inflate(T aDx, T aDy)
  {
    x -= aDx;
    y -= aDy;
    width += 2 * aDx;
    height += 2 * aDy;
  }
  void Inflate(const MarginT& aMargin)
  {
    x -= aMargin.left;
    y -= aMargin.top;
    width += aMargin.LeftRight();
    height += aMargin.TopBottom();
  }
  void Inflate(const SizeT& aSize) { Inflate(aSize.width, aSize.height); }

  void InflateToMultiple(const SizeT& aMultiple)
  {
    T xMost = XMost();
    T yMost = YMost();

    x = static_cast<T>(floor(x / aMultiple.width)) * aMultiple.width;
    y = static_cast<T>(floor(y / aMultiple.height)) * aMultiple.height;
    xMost = static_cast<T>(ceil(x / aMultiple.width)) * aMultiple.width;
    yMost = static_cast<T>(ceil(y / aMultiple.height)) * aMultiple.height;

    width = xMost - x;
    height = yMost - y;
  }

  void Deflate(T aD) { Deflate(aD, aD); }
  void Deflate(T aDx, T aDy)
  {
    x += aDx;
    y += aDy;
    width = std::max(T(0), width - 2 * aDx);
    height = std::max(T(0), height - 2 * aDy);
  }
  void Deflate(const MarginT& aMargin)
  {
    x += aMargin.left;
    y += aMargin.top;
    width = std::max(T(0), width - aMargin.LeftRight());
    height = std::max(T(0), height - aMargin.TopBottom());
  }
  void Deflate(const SizeT& aSize) { Deflate(aSize.width, aSize.height); }

  
  
  
  
  bool IsEqualEdges(const Sub& aRect) const
  {
    return x == aRect.x && y == aRect.y &&
           width == aRect.width && height == aRect.height;
  }
  
  
  bool IsEqualInterior(const Sub& aRect) const
  {
    return IsEqualEdges(aRect) || (IsEmpty() && aRect.IsEmpty());
  }

  friend Sub operator+(Sub aSub, const Point& aPoint)
  {
    aSub += aPoint;
    return aSub;
  }
  friend Sub operator-(Sub aSub, const Point& aPoint)
  {
    aSub -= aPoint;
    return aSub;
  }
  friend Sub operator+(Sub aSub, const SizeT& aSize)
  {
    aSub += aSize;
    return aSub;
  }
  friend Sub operator-(Sub aSub, const SizeT& aSize)
  {
    aSub -= aSize;
    return aSub;
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
  Sub& operator+=(const SizeT& aSize)
  {
    width += aSize.width;
    height += aSize.height;
    return *static_cast<Sub*>(this);
  }
  Sub& operator-=(const SizeT& aSize)
  {
    width -= aSize.width;
    height -= aSize.height;
    return *static_cast<Sub*>(this);
  }
  
  MarginT operator-(const Sub& aRect) const
  {
    return MarginT(aRect.y - y,
                   XMost() - aRect.XMost(),
                   YMost() - aRect.YMost(),
                   aRect.x - x);
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

  
  void SetLeftEdge(T aX) {
    MOZ_ASSERT(aX <= XMost());
    width = XMost() - aX;
    x = aX;
  }
  void SetRightEdge(T aXMost) { 
    MOZ_ASSERT(aXMost >= x);
    width = aXMost - x; 
  }
  void SetTopEdge(T aY) {
    MOZ_ASSERT(aY <= YMost());
    height = YMost() - aY;
    y = aY;
  }
  void SetBottomEdge(T aYMost) { 
    MOZ_ASSERT(aYMost >= y);
    height = aYMost - y; 
  }

  
  
  
  
  
  
  
  
  
  
  
  void Round()
  {
    T x0 = static_cast<T>(floor(T(X()) + 0.5));
    T y0 = static_cast<T>(floor(T(Y()) + 0.5));
    T x1 = static_cast<T>(floor(T(XMost()) + 0.5));
    T y1 = static_cast<T>(floor(T(YMost()) + 0.5));

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
  }

  
  
  void RoundIn()
  {
    T x0 = static_cast<T>(ceil(T(X())));
    T y0 = static_cast<T>(ceil(T(Y())));
    T x1 = static_cast<T>(floor(T(XMost())));
    T y1 = static_cast<T>(floor(T(YMost())));

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
  }

  
  
  void RoundOut()
  {
    T x0 = static_cast<T>(floor(T(X())));
    T y0 = static_cast<T>(floor(T(Y())));
    T x1 = static_cast<T>(ceil(T(XMost())));
    T y1 = static_cast<T>(ceil(T(YMost())));

    x = x0;
    y = y0;

    width = x1 - x0;
    height = y1 - y0;
  }

  
  void Scale(T aScale) { Scale(aScale, aScale); }
  
  void Scale(T aXScale, T aYScale)
  {
    T right = XMost() * aXScale;
    T bottom = YMost() * aYScale;
    x = x * aXScale;
    y = y * aYScale;
    width = right - x;
    height = bottom - y;
  }
  
  
  
  void ScaleRoundOut(double aScale) { ScaleRoundOut(aScale, aScale); }
  
  
  
  
  void ScaleRoundOut(double aXScale, double aYScale)
  {
    T right = static_cast<T>(ceil(double(XMost()) * aXScale));
    T bottom = static_cast<T>(ceil(double(YMost()) * aYScale));
    x = static_cast<T>(floor(double(x) * aXScale));
    y = static_cast<T>(floor(double(y) * aYScale));
    width = right - x;
    height = bottom - y;
  }
  
  
  void ScaleRoundIn(double aScale) { ScaleRoundIn(aScale, aScale); }
  
  
  
  void ScaleRoundIn(double aXScale, double aYScale)
  {
    T right = static_cast<T>(floor(double(XMost()) * aXScale));
    T bottom = static_cast<T>(floor(double(YMost()) * aYScale));
    x = static_cast<T>(ceil(double(x) * aXScale));
    y = static_cast<T>(ceil(double(y) * aYScale));
    width = std::max<T>(0, right - x);
    height = std::max<T>(0, bottom - y);
  }
  
  
  
  void ScaleInverseRoundOut(double aScale) { ScaleInverseRoundOut(aScale, aScale); }
  
  
  
  
  void ScaleInverseRoundOut(double aXScale, double aYScale)
  {
    T right = static_cast<T>(ceil(double(XMost()) / aXScale));
    T bottom = static_cast<T>(ceil(double(YMost()) / aYScale));
    x = static_cast<T>(floor(double(x) / aXScale));
    y = static_cast<T>(floor(double(y) / aYScale));
    width = right - x;
    height = bottom - y;
  }
  
  
  void ScaleInverseRoundIn(double aScale) { ScaleInverseRoundIn(aScale, aScale); }
  
  
  
  void ScaleInverseRoundIn(double aXScale, double aYScale)
  {
    T right = static_cast<T>(floor(double(XMost()) / aXScale));
    T bottom = static_cast<T>(floor(double(YMost()) / aYScale));
    x = static_cast<T>(ceil(double(x) / aXScale));
    y = static_cast<T>(ceil(double(y) / aYScale));
    width = std::max<T>(0, right - x);
    height = std::max<T>(0, bottom - y);
  }

  



  Point ClampPoint(const Point& aPoint) const
  {
    return Point(std::max(x, std::min(XMost(), aPoint.x)),
                 std::max(y, std::min(YMost(), aPoint.y)));
  }

  




  Sub ForceInside(const Sub& aRect) const
  {
    Sub rect(std::max(aRect.x, x),
             std::max(aRect.y, y),
             std::min(aRect.width, width),
             std::min(aRect.height, height));
    rect.x = std::min(rect.XMost(), aRect.XMost()) - rect.width;
    rect.y = std::min(rect.YMost(), aRect.YMost()) - rect.height;
    return rect;
  }

  friend std::ostream& operator<<(std::ostream& stream, const Sub& aRect) {
    return stream << '(' << aRect.x << ',' << aRect.y << ','
                  << aRect.width << ',' << aRect.height << ')';
  }

private:
  
  
  bool operator==(const Sub& aRect) const { return false; }
  bool operator!=(const Sub& aRect) const { return false; }
};

}
}

#endif 
