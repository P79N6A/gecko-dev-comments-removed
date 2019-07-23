



































#ifndef __NS_SVGPAINTSERVERFRAME_H__
#define __NS_SVGPAINTSERVERFRAME_H__

#include "nsSVGContainerFrame.h"

class gfxContext;
class nsSVGGeometryFrame;

typedef nsSVGContainerFrame nsSVGPaintServerFrameBase;

class nsSVGPaintServerFrame : public nsSVGPaintServerFrameBase
{
protected:
  nsSVGPaintServerFrame(nsStyleContext* aContext) :
    nsSVGPaintServerFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  



  virtual PRBool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aOpacity) = 0;
};

#endif 
