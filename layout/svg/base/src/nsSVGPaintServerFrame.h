



































#ifndef __NS_SVGPAINTSERVERFRAME_H__
#define __NS_SVGPAINTSERVERFRAME_H__

#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsQueryFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class gfxContext;
class gfxPattern;
class nsStyleContext;
class nsSVGGeometryFrame;

struct gfxRect;

typedef nsSVGContainerFrame nsSVGPaintServerFrameBase;

class nsSVGPaintServerFrame : public nsSVGPaintServerFrameBase
{
protected:
  nsSVGPaintServerFrame(nsStyleContext* aContext)
    : nsSVGPaintServerFrameBase(aContext)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

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
