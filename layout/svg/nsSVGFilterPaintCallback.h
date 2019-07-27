




#ifndef __NS_SVGFILTERPAINTCALLBACK_H__
#define __NS_SVGFILTERPAINTCALLBACK_H__

#include "nsRect.h"

class nsIFrame;
class gfxContext;

class nsSVGFilterPaintCallback {
public:
  











  virtual void Paint(gfxContext& aContext, nsIFrame *aTarget,
                     const gfxMatrix& aTransform,
                     const nsIntRect *aDirtyRect) = 0;
};

#endif
