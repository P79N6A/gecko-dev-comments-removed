




#ifndef MOZILLA_GFX_SCALEFACTORS2D_H_
#define MOZILLA_GFX_SCALEFACTORS2D_H_

#include <ostream>
#include <iomanip>

#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/gfx/ScaleFactor.h"

#include "gfxPoint.h"

namespace mozilla {
namespace gfx {





template<class src, class dst>
struct ScaleFactors2D {
  float xScale;
  float yScale;

  MOZ_CONSTEXPR ScaleFactors2D() : xScale(1.0), yScale(1.0) {}
  MOZ_CONSTEXPR ScaleFactors2D(const ScaleFactors2D<src, dst>& aCopy)
    : xScale(aCopy.xScale), yScale(aCopy.yScale) {}
  MOZ_CONSTEXPR ScaleFactors2D(float aXScale, float aYScale)
    : xScale(aXScale), yScale(aYScale) {}
  
  explicit MOZ_CONSTEXPR ScaleFactors2D(const gfxSize& aSize)
    : xScale(aSize.width), yScale(aSize.height) {}

  
  
  
  
  explicit MOZ_CONSTEXPR ScaleFactors2D(const ScaleFactor<src, dst>& aScale)
    : xScale(aScale.scale), yScale(aScale.scale) {}

  bool AreScalesSame() const {
    return FuzzyEqualsMultiplicative(xScale, yScale);
  }

  
  ScaleFactor<src, dst> ToScaleFactor() const {
    MOZ_ASSERT(AreScalesSame());
    return ScaleFactor<src, dst>(xScale);
  }

  bool operator==(const ScaleFactors2D<src, dst>& aOther) const {
    return xScale == aOther.xScale && yScale == aOther.yScale;
  }

  bool operator!=(const ScaleFactors2D<src, dst>& aOther) const {
    return !(*this == aOther);
  }

  friend std::ostream& operator<<(std::ostream& aStream,
                                  const ScaleFactors2D<src, dst>& aScale) {
    if (aScale.AreScalesSame()) {
      return aStream << aScale.xScale;
    } else {
      return aStream << '(' << aScale.xScale << ',' << aScale.yScale << ')';
    }
  }

  template<class other>
  ScaleFactors2D<other, dst> operator/(const ScaleFactors2D<src, other>& aOther) const {
    return ScaleFactors2D<other, dst>(xScale / aOther.xScale, yScale / aOther.yScale);
  }

  template<class other>
  ScaleFactors2D<src, other> operator/(const ScaleFactors2D<other, dst>& aOther) const {
    return ScaleFactors2D<src, other>(xScale / aOther.xScale, yScale / aOther.yScale);
  }

  template<class other>
  ScaleFactors2D<src, other> operator*(const ScaleFactors2D<dst, other>& aOther) const {
    return ScaleFactors2D<src, other>(xScale * aOther.xScale, yScale * aOther.yScale);
  }

  template<class other>
  ScaleFactors2D<other, dst> operator*(const ScaleFactors2D<other, src>& aOther) const {
    return ScaleFactors2D<other, dst>(xScale * aOther.xScale, yScale * aOther.yScale);
  }

  template<class other>
  ScaleFactors2D<src, other> operator*(const ScaleFactor<dst, other>& aOther) const {
    return *this * ScaleFactors2D<dst, other>(aOther);
  }

  template<class other>
  ScaleFactors2D<other, dst> operator*(const ScaleFactor<other, src>& aOther) const {
    return *this * ScaleFactors2D<other, src>(aOther);
  }

  template<class other>
  ScaleFactors2D<src, other> operator/(const ScaleFactor<other, dst>& aOther) const {
    return *this / ScaleFactors2D<other, dst>(aOther);
  }

  template<class other>
  ScaleFactors2D<other, dst> operator/(const ScaleFactor<src, other>& aOther) const {
    return *this / ScaleFactors2D<src, other>(aOther);
  }

  template<class other>
  friend ScaleFactors2D<other, dst> operator*(const ScaleFactor<other, src>& aA,
                                              const ScaleFactors2D<src, dst>& aB) {
    return ScaleFactors2D<other, src>(aA) * aB;
  }

  template<class other>
  friend ScaleFactors2D<other, src> operator/(const ScaleFactor<other, dst>& aA,
                                              const ScaleFactors2D<src, dst>& aB) {
    return ScaleFactors2D<other, src>(aA) / aB;
  }

  
  
  
  gfxSize operator/(const ScaleFactors2D& aOther) const {
    return gfxSize(xScale / aOther.xScale, yScale / aOther.yScale);
  }
};

} 
} 

#endif 
