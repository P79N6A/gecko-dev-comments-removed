




#ifndef MOZILLA_SVGCONTEXT_H_
#define MOZILLA_SVGCONTEXT_H_

#include "mozilla/Maybe.h"
#include "SVGPreserveAspectRatio.h"

namespace mozilla {






class SVGImageContext
{
public:
  SVGImageContext() { }

  SVGImageContext(nsIntSize aViewportSize,
                  Maybe<SVGPreserveAspectRatio> aPreserveAspectRatio)
    : mViewportSize(aViewportSize)
    , mPreserveAspectRatio(aPreserveAspectRatio)
  { }

  const nsIntSize& GetViewportSize() const {
    return mViewportSize;
  }

  const Maybe<SVGPreserveAspectRatio>& GetPreserveAspectRatio() const {
    return mPreserveAspectRatio;
  }

  bool operator==(const SVGImageContext& aOther) const {
    return mViewportSize == aOther.mViewportSize &&
           mPreserveAspectRatio == aOther.mPreserveAspectRatio;
  }

  bool operator!=(const SVGImageContext& aOther) const {
    return !(*this == aOther);
  }

  uint32_t Hash() const {
    return HashGeneric(mViewportSize.width,
                       mViewportSize.height,
                       mPreserveAspectRatio.map(HashPAR).valueOr(0));
  }

private:
  static uint32_t HashPAR(const SVGPreserveAspectRatio& aPAR) {
    return aPAR.Hash();
  }

  nsIntSize                     mViewportSize;
  Maybe<SVGPreserveAspectRatio> mPreserveAspectRatio;
};

} 

#endif 
