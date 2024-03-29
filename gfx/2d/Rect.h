




#ifndef MOZILLA_GFX_RECT_H_
#define MOZILLA_GFX_RECT_H_

#include "BaseRect.h"
#include "BaseMargin.h"
#include "NumericTools.h"
#include "Point.h"
#include "Tools.h"

#include <cmath>

namespace mozilla {

template <typename> struct IsPixel;

namespace gfx {

template<class units>
struct IntMarginTyped:
    public BaseMargin<int32_t, IntMarginTyped<units> >,
    public units {
    static_assert(IsPixel<units>::value,
                  "'units' must be a coordinate system tag");

    typedef BaseMargin<int32_t, IntMarginTyped<units> > Super;

    IntMarginTyped() : Super() {}
    IntMarginTyped(int32_t aTop, int32_t aRight, int32_t aBottom, int32_t aLeft) :
        Super(aTop, aRight, aBottom, aLeft) {}
};
typedef IntMarginTyped<UnknownUnits> IntMargin;

template<class units>
struct MarginTyped:
    public BaseMargin<Float, MarginTyped<units> >,
    public units {
    static_assert(IsPixel<units>::value,
                  "'units' must be a coordinate system tag");

    typedef BaseMargin<Float, MarginTyped<units> > Super;

    MarginTyped() : Super() {}
    MarginTyped(Float aTop, Float aRight, Float aBottom, Float aLeft) :
        Super(aTop, aRight, aBottom, aLeft) {}
    explicit MarginTyped(const IntMarginTyped<units>& aMargin) :
        Super(float(aMargin.top), float(aMargin.right),
              float(aMargin.bottom), float(aMargin.left)) {}
};
typedef MarginTyped<UnknownUnits> Margin;

template<class units>
IntMarginTyped<units> RoundedToInt(const MarginTyped<units>& aMargin)
{
  return IntMarginTyped<units>(int32_t(floorf(aMargin.top + 0.5f)),
                               int32_t(floorf(aMargin.right + 0.5f)),
                               int32_t(floorf(aMargin.bottom + 0.5f)),
                               int32_t(floorf(aMargin.left + 0.5f)));
}

template<class units>
struct IntRectTyped :
    public BaseRect<int32_t, IntRectTyped<units>, IntPointTyped<units>, IntSizeTyped<units>, IntMarginTyped<units> >,
    public units {
    static_assert(IsPixel<units>::value,
                  "'units' must be a coordinate system tag");

    typedef BaseRect<int32_t, IntRectTyped<units>, IntPointTyped<units>, IntSizeTyped<units>, IntMarginTyped<units> > Super;

    IntRectTyped() : Super() {}
    IntRectTyped(const IntPointTyped<units>& aPos, const IntSizeTyped<units>& aSize) :
        Super(aPos, aSize) {}
    IntRectTyped(int32_t _x, int32_t _y, int32_t _width, int32_t _height) :
        Super(_x, _y, _width, _height) {}

    
    void Round() {}
    void RoundIn() {}
    void RoundOut() {}

    
    

    static IntRectTyped<units> FromUnknownRect(const IntRectTyped<UnknownUnits>& rect) {
        return IntRectTyped<units>(rect.x, rect.y, rect.width, rect.height);
    }

    IntRectTyped<UnknownUnits> ToUnknownRect() const {
        return IntRectTyped<UnknownUnits>(this->x, this->y, this->width, this->height);
    }

    bool Overflows() const {
      CheckedInt<int32_t> xMost = this->x;
      xMost += this->width;
      CheckedInt<int32_t> yMost = this->y;
      yMost += this->height;
      return !xMost.isValid() || !yMost.isValid();
    }

    
    bool operator==(const IntRectTyped<units>& aRect) const
    {
      return IntRectTyped<units>::IsEqualEdges(aRect);
    }

    void InflateToMultiple(const IntSizeTyped<units>& aTileSize)
    {
      int32_t yMost = this->YMost();
      int32_t xMost = this->XMost();

      this->x = mozilla::RoundDownToMultiple(this->x, aTileSize.width);
      this->y = mozilla::RoundDownToMultiple(this->y, aTileSize.height);
      xMost = mozilla::RoundUpToMultiple(xMost, aTileSize.width);
      yMost = mozilla::RoundUpToMultiple(yMost, aTileSize.height);

      this->width = xMost - this->x;
      this->height = yMost - this->y;
    }

};
typedef IntRectTyped<UnknownUnits> IntRect;

template<class units>
struct RectTyped :
    public BaseRect<Float, RectTyped<units>, PointTyped<units>, SizeTyped<units>, MarginTyped<units> >,
    public units {
    static_assert(IsPixel<units>::value,
                  "'units' must be a coordinate system tag");

    typedef BaseRect<Float, RectTyped<units>, PointTyped<units>, SizeTyped<units>, MarginTyped<units> > Super;

    RectTyped() : Super() {}
    RectTyped(const PointTyped<units>& aPos, const SizeTyped<units>& aSize) :
        Super(aPos, aSize) {}
    RectTyped(Float _x, Float _y, Float _width, Float _height) :
        Super(_x, _y, _width, _height) {}
    explicit RectTyped(const IntRectTyped<units>& rect) :
        Super(float(rect.x), float(rect.y),
              float(rect.width), float(rect.height)) {}

    void NudgeToIntegers()
    {
      NudgeToInteger(&(this->x));
      NudgeToInteger(&(this->y));
      NudgeToInteger(&(this->width));
      NudgeToInteger(&(this->height));
    }

    bool ToIntRect(IntRectTyped<units> *aOut) const
    {
      *aOut = IntRectTyped<units>(int32_t(this->X()), int32_t(this->Y()),
                                  int32_t(this->Width()), int32_t(this->Height()));
      return RectTyped<units>(Float(aOut->x), Float(aOut->y), 
                              Float(aOut->width), Float(aOut->height))
             .IsEqualEdges(*this);
    }

    
    

    static RectTyped<units> FromUnknownRect(const RectTyped<UnknownUnits>& rect) {
        return RectTyped<units>(rect.x, rect.y, rect.width, rect.height);
    }

    RectTyped<UnknownUnits> ToUnknownRect() const {
        return RectTyped<UnknownUnits>(this->x, this->y, this->width, this->height);
    }

    
    bool operator==(const RectTyped<units>& aRect) const
    {
      return RectTyped<units>::IsEqualEdges(aRect);
    }
};
typedef RectTyped<UnknownUnits> Rect;

template<class units>
IntRectTyped<units> RoundedToInt(const RectTyped<units>& aRect)
{
  RectTyped<units> copy(aRect);
  copy.Round();
  return IntRectTyped<units>(int32_t(copy.x),
                             int32_t(copy.y),
                             int32_t(copy.width),
                             int32_t(copy.height));
}

template<class units>
IntRectTyped<units> RoundedIn(const RectTyped<units>& aRect)
{
  RectTyped<units> copy(aRect);
  copy.RoundIn();
  return IntRectTyped<units>(int32_t(copy.x),
                             int32_t(copy.y),
                             int32_t(copy.width),
                             int32_t(copy.height));
}

template<class units>
IntRectTyped<units> RoundedOut(const RectTyped<units>& aRect)
{
  RectTyped<units> copy(aRect);
  copy.RoundOut();
  return IntRectTyped<units>(int32_t(copy.x),
                             int32_t(copy.y),
                             int32_t(copy.width),
                             int32_t(copy.height));
}

} 
} 

#endif 
