



































#ifndef __NS_SVGFILTERPAINTCALLBACK_H__
#define __NS_SVGFILTERPAINTCALLBACK_H__

#include "nsRect.h"

class nsIFrame;
class nsSVGRenderState;

class nsSVGFilterPaintCallback {
public:
  









  virtual void Paint(nsSVGRenderState *aContext, nsIFrame *aTarget,
                     const nsIntRect *aDirtyRect) = 0;
};

#endif
