




#ifndef MOZILLA_GFX_RECT_H_
#define MOZILLA_GFX_RECT_H_

#include "BaseRect.h"
#include "BaseMargin.h"
#include "Point.h"
#include "Tools.h"

namespace mozilla {
namespace gfx {

struct Margin :
  public BaseMargin<Float, Margin> {
  typedef BaseMargin<Float, Margin> Super;

  
  Margin() : Super(0, 0, 0, 0) {}
  Margin(const Margin& aMargin) : Super(aMargin) {}
  Margin(Float aTop, Float aRight, Float aBottom, Float aLeft)
    : Super(aTop, aRight, aBottom, aLeft) {}
};

template<class units>
struct IntRectTyped :
    public BaseRect<int32_t, IntRectTyped<units>, IntPointTyped<units>, IntSizeTyped<units>, Margin>,
    public units {
    typedef BaseRect<int32_t, IntRectTyped<units>, IntPointTyped<units>, IntSizeTyped<units>, Margin> Super;

    IntRectTyped() : Super() {}
    IntRectTyped(IntPointTyped<units> aPos, IntSizeTyped<units> aSize) :
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
};
typedef IntRectTyped<UnknownUnits> IntRect;

template<class units>
struct RectTyped :
    public BaseRect<Float, RectTyped<units>, PointTyped<units>, SizeTyped<units>, Margin>,
    public units {
    typedef BaseRect<Float, RectTyped<units>, PointTyped<units>, SizeTyped<units>, Margin> Super;

    RectTyped() : Super() {}
    RectTyped(PointTyped<units> aPos, SizeTyped<units> aSize) :
        Super(aPos, aSize) {}
    RectTyped(Float _x, Float _y, Float _width, Float _height) :
        Super(_x, _y, _width, _height) {}
    explicit RectTyped(const IntRectTyped<units>& rect) :
        Super(float(rect.x), float(rect.y),
              float(rect.width), float(rect.height)) {}

    GFX2D_API void NudgeToIntegers()
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
};
typedef RectTyped<UnknownUnits> Rect;

}
}

#endif 
