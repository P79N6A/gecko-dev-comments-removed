



































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

  


  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          float aOpacity,
                          const gfxRect *aOverrideBounds = nsnull) = 0;

  



  virtual bool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aOpacity);

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGPaintServerFrameBase::IsFrameOfType(aFlags & ~nsIFrame::eSVGPaintServer);
  }
};

#endif 
