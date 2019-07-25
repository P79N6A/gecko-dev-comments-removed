



































#ifndef __NS_SVGFILTERPAINTCALLBACK_H__
#define __NS_SVGFILTERPAINTCALLBACK_H__

#include "nsRect.h"

class nsIFrame;
class nsRenderingContext;

class nsSVGFilterPaintCallback {
public:
  









  virtual void Paint(nsRenderingContext *aContext, nsIFrame *aTarget,
                     const nsIntRect *aDirtyRect) = 0;
};

#endif
