




#ifndef MOZILLA_GFX_BASERECT_H_
#define MOZILLA_GFX_BASERECT_H_

#include <cmath>

namespace mozilla {
namespace gfx {



template<typename T>
T gfx_min(T aVal1, T aVal2)
{
  return (aVal1 < aVal2) ? aVal1 : aVal2;
}

template<typename T>
T gfx_max(T aVal1, T aVal2)
{
  return (aVal1 > aVal2) ? aVal1 : aVal2;
}
























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
    result.x = gfx_max(x, aRect.x);
    result.y = gfx_max(y, aRect.y);
    result.width = gfx_min(XMost(), aRect.XMost()) - result.x;
    result.height = gfx_min(YMost(), aRect.YMost()) - result.y;
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
    result.x = gfx_min(x, aRect.x);
    result.y = gfx_min(y, aRect.y);
    result.width = gfx_max(XMost(), aRect.XMost()) - result.x;
    result.height = gfx_max(YMost(), aRect.YMost()) - result.y;
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
  void Inflate(const Margin& aMargin)
  {
    x -= aMargin.left;
    y -= aMargin.top;
    width += aMargin.LeftRight();
    height += aMargin.TopBottom();
  }
  void Inflate(const SizeT& aSize) { Inflate(aSize.width, aSize.height); }

  void Deflate(T aD) { Deflate(aD, aD); }
  void Deflate(T aDx, T aDy)
  {
    x += aDx;
    y += aDy;
    width = gfx_max(T(0), width - 2 * aDx);
    height = gfx_max(T(0), height - 2 * aDy);
  }
  void Deflate(const Margin& aMargin)
  {
    x += aMargin.left;
    y += aMargin.top;
    width = gfx_max(T(0), width - aMargin.LeftRight());
    height = gfx_max(T(0), height - aMargin.TopBottom());
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

  
  void SetLeftEdge(T aX) {
    width = XMost() - aX;
    x = aX;
  }
  void SetRightEdge(T aXMost) { width = aXMost - x; }

  void SetTopEdge(T aY) {
    height = YMost() - aY;
    y = aY;
  }
  void SetBottomEdge(T aYMost) { height = aYMost - y; }

  
  
  
  
  
  
  
  
  
  
  
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
    width = gfx_max<T>(0, right - x);
    height = gfx_max<T>(0, bottom - y);
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

  



  Point ClampPoint(const Point& aPoint) const
  {
    return Point(NS_MAX(x, NS_MIN(XMost(), aPoint.x)),
                 NS_MAX(y, NS_MIN(YMost(), aPoint.y)));
  }

private:
  
  
  bool operator==(const Sub& aRect) const { return false; }
  bool operator!=(const Sub& aRect) const { return false; }
};

}
}

#endif 
