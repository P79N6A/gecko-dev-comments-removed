




#ifndef MOZILLA_SVGCONTEXT_H_
#define MOZILLA_SVGCONTEXT_H_

#include "SVGPreserveAspectRatio.h"

namespace mozilla {




class SVGImageContext
{
public:
  SVGImageContext() { }

  SVGImageContext(SVGPreserveAspectRatio aPreserveAspectRatio,
                  gfxFloat aOpacity = 1.0)
    : mPreserveAspectRatio(aPreserveAspectRatio), mGlobalOpacity(aOpacity)
  { }

  const SVGPreserveAspectRatio& GetPreserveAspectRatio() const {
    return mPreserveAspectRatio;
  }

  gfxFloat GetGlobalOpacity() const {
    return mGlobalOpacity;
  }

  bool operator==(const SVGImageContext& aOther) const {
    return mPreserveAspectRatio == aOther.mPreserveAspectRatio;
  }

  bool operator!=(const SVGImageContext& aOther) const {
    return !(*this == aOther);
  }

  uint32_t Hash() const {
    return mPreserveAspectRatio.Hash();
  }

private:
  SVGPreserveAspectRatio mPreserveAspectRatio;
  gfxFloat mGlobalOpacity;
};

} 

#endif 
