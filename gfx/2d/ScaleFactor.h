




#ifndef MOZILLA_GFX_SCALEFACTOR_H_
#define MOZILLA_GFX_SCALEFACTOR_H_

#include "mozilla/Attributes.h"

#include "gfxPoint.h"

namespace mozilla {
namespace gfx {












template<class src, class dst>
struct ScaleFactor {
  float scale;

  MOZ_CONSTEXPR ScaleFactor() : scale(1.0) {}
  MOZ_CONSTEXPR ScaleFactor(const ScaleFactor<src, dst>& aCopy) : scale(aCopy.scale) {}
  explicit MOZ_CONSTEXPR ScaleFactor(float aScale) : scale(aScale) {}

  explicit ScaleFactor(float aX, float aY) : scale(aX) {
    MOZ_ASSERT(fabs(aX - aY) < 1e-6);
  }

  explicit ScaleFactor(gfxSize aScale) : scale(aScale.width) {
    MOZ_ASSERT(fabs(aScale.width - aScale.height) < 1e-6);
  }

  ScaleFactor<dst, src> Inverse() {
    return ScaleFactor<dst, src>(1 / scale);
  }

  bool operator==(const ScaleFactor<src, dst>& aOther) const {
    return scale == aOther.scale;
  }

  bool operator!=(const ScaleFactor<src, dst>& aOther) const {
    return !(*this == aOther);
  }

  bool operator<(const ScaleFactor<src, dst>& aOther) const {
    return scale < aOther.scale;
  }

  bool operator<=(const ScaleFactor<src, dst>& aOther) const {
    return scale <= aOther.scale;
  }

  bool operator>(const ScaleFactor<src, dst>& aOther) const {
    return scale > aOther.scale;
  }

  bool operator>=(const ScaleFactor<src, dst>& aOther) const {
    return scale >= aOther.scale;
  }

  template<class other>
  ScaleFactor<other, dst> operator/(const ScaleFactor<src, other>& aOther) const {
    return ScaleFactor<other, dst>(scale / aOther.scale);
  }

  template<class other>
  ScaleFactor<src, other> operator/(const ScaleFactor<other, dst>& aOther) const {
    return ScaleFactor<src, other>(scale / aOther.scale);
  }

  template<class other>
  ScaleFactor<src, other> operator*(const ScaleFactor<dst, other>& aOther) const {
    return ScaleFactor<src, other>(scale * aOther.scale);
  }

  template<class other>
  ScaleFactor<other, dst> operator*(const ScaleFactor<other, src>& aOther) const {
    return ScaleFactor<other, dst>(scale * aOther.scale);
  }
};

}
}

#endif 
