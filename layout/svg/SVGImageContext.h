




#ifndef MOZILLA_SVGCONTEXT_H_
#define MOZILLA_SVGCONTEXT_H_

#include "SVGPreserveAspectRatio.h"

namespace mozilla {




class NS_STACK_CLASS SVGImageContext
{
public:
  SVGImageContext(SVGPreserveAspectRatio aPreserveAspectRatio)
    : mPreserveAspectRatio(aPreserveAspectRatio)
  { }

  const SVGPreserveAspectRatio& GetPreserveAspectRatio() const {
    return mPreserveAspectRatio;
  }

private:
  const SVGPreserveAspectRatio mPreserveAspectRatio;
};

} 

#endif 
