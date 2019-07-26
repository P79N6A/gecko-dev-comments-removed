




#ifndef MOZILLA_SVGCONTEXT_H_
#define MOZILLA_SVGCONTEXT_H_

#include "SVGPreserveAspectRatio.h"

namespace mozilla {




class SVGImageContext
{
public:
  SVGImageContext() { }

  SVGImageContext(SVGPreserveAspectRatio aPreserveAspectRatio)
    : mPreserveAspectRatio(aPreserveAspectRatio)
  { }

  const SVGPreserveAspectRatio& GetPreserveAspectRatio() const {
    return mPreserveAspectRatio;
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
};

} 

#endif 
