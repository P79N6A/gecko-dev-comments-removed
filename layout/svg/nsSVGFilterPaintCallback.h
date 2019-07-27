




#ifndef __NS_SVGFILTERPAINTCALLBACK_H__
#define __NS_SVGFILTERPAINTCALLBACK_H__

class nsIFrame;
class nsRenderingContext;

struct nsIntRect;

class nsSVGFilterPaintCallback {
public:
  











  virtual void Paint(nsRenderingContext *aContext, nsIFrame *aTarget,
                     const gfxMatrix& aTransform,
                     const nsIntRect *aDirtyRect) = 0;
};

#endif
