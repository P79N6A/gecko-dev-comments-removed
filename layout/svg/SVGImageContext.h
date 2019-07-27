




#ifndef MOZILLA_SVGCONTEXT_H_
#define MOZILLA_SVGCONTEXT_H_

#include "mozilla/Maybe.h"
#include "SVGPreserveAspectRatio.h"
#include "Units.h"

namespace mozilla {






class SVGImageContext
{
public:
  SVGImageContext()
    : mGlobalOpacity(1.0)
  { }

  SVGImageContext(CSSIntSize aViewportSize,
                  Maybe<SVGPreserveAspectRatio> aPreserveAspectRatio,
                  gfxFloat aOpacity = 1.0)
    : mViewportSize(aViewportSize)
    , mPreserveAspectRatio(aPreserveAspectRatio)
    , mGlobalOpacity(aOpacity)
  { }

  const CSSIntSize& GetViewportSize() const {
    return mViewportSize;
  }

  const Maybe<SVGPreserveAspectRatio>& GetPreserveAspectRatio() const {
    return mPreserveAspectRatio;
  }

  gfxFloat GetGlobalOpacity() const {
    return mGlobalOpacity;
  }

  bool operator==(const SVGImageContext& aOther) const {
    return mViewportSize == aOther.mViewportSize &&
           mPreserveAspectRatio == aOther.mPreserveAspectRatio &&
           mGlobalOpacity == aOther.mGlobalOpacity;
  }

  bool operator!=(const SVGImageContext& aOther) const {
    return !(*this == aOther);
  }

  uint32_t Hash() const {
    return HashGeneric(mViewportSize.width,
                       mViewportSize.height,
                       mPreserveAspectRatio.map(HashPAR).valueOr(0),
                       HashBytes(&mGlobalOpacity, sizeof(gfxFloat)));
  }

private:
  static uint32_t HashPAR(const SVGPreserveAspectRatio& aPAR) {
    return aPAR.Hash();
  }

  CSSIntSize                    mViewportSize;
  Maybe<SVGPreserveAspectRatio> mPreserveAspectRatio;
  gfxFloat                      mGlobalOpacity;
};

} 

#endif 
