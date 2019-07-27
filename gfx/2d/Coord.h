




#ifndef MOZILLA_GFX_COORD_H_
#define MOZILLA_GFX_COORD_H_

#include "mozilla/Attributes.h"
#include "Types.h"
#include "BaseCoord.h"

#include <cmath>

namespace mozilla {

template <typename> struct IsPixel;

namespace gfx {







template <class coord, class primitive>
struct CoordOperatorsHelper {
  friend bool operator==(coord aA, primitive aB) {
    return aA.value == aB;
  }
  friend bool operator==(primitive aA, coord aB) {
    return aA == aB.value;
  }
  friend bool operator!=(coord aA, primitive aB) {
    return aA.value != aB;
  }
  friend bool operator!=(primitive aA, coord aB) {
    return aA != aB.value;
  }
  friend primitive operator+(coord aA, primitive aB) {
    return aA.value + aB;
  }
  friend primitive operator+(primitive aA, coord aB) {
    return aA + aB.value;
  }
  friend primitive operator-(coord aA, primitive aB) {
    return aA.value - aB;
  }
  friend primitive operator-(primitive aA, coord aB) {
    return aA - aB.value;
  }
  friend primitive operator*(coord aCoord, primitive aScale) {
    return aCoord.value * aScale;
  }
  friend primitive operator*(primitive aScale, coord aCoord) {
    return aScale * aCoord.value;
  }
  friend primitive operator/(coord aCoord, primitive aScale) {
    return aCoord.value / aScale;
  }
  
};




template<class units>
struct IntCoordTyped :
  public BaseCoord< int32_t, IntCoordTyped<units> >,
  public CoordOperatorsHelper< IntCoordTyped<units>, float >,
  public CoordOperatorsHelper< IntCoordTyped<units>, double > {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BaseCoord< int32_t, IntCoordTyped<units> > Super;

  MOZ_CONSTEXPR IntCoordTyped() : Super() {}
  MOZ_CONSTEXPR IntCoordTyped(int32_t aValue) : Super(aValue) {}
};

template<class units>
struct CoordTyped :
  public BaseCoord< Float, CoordTyped<units> >,
  public CoordOperatorsHelper< CoordTyped<units>, int32_t >,
  public CoordOperatorsHelper< CoordTyped<units>, uint32_t >,
  public CoordOperatorsHelper< CoordTyped<units>, double > {
  static_assert(IsPixel<units>::value,
                "'units' must be a coordinate system tag");

  typedef BaseCoord< Float, CoordTyped<units> > Super;

  MOZ_CONSTEXPR CoordTyped() : Super() {}
  MOZ_CONSTEXPR CoordTyped(Float aValue) : Super(aValue) {}
  explicit MOZ_CONSTEXPR CoordTyped(const IntCoordTyped<units>& aCoord) : Super(float(aCoord.value)) {}

  void Round() {
    this->value = floor(this->value + 0.5);
  }
  void Truncate() {
    this->value = int32_t(this->value);
  }

  IntCoordTyped<units> Rounded() const {
    return IntCoordTyped<units>(int32_t(floor(this->value + 0.5)));
  }
  IntCoordTyped<units> Truncated() const {
    return IntCoordTyped<units>(int32_t(this->value));
  }
};

}
}

#endif 
