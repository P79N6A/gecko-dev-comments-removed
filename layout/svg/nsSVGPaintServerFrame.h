




#ifndef __NS_SVGPAINTSERVERFRAME_H__
#define __NS_SVGPAINTSERVERFRAME_H__

#include "mozilla/Attributes.h"
#include "nsCOMPtr.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsQueryFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class gfxContext;
class gfxPattern;
class nsStyleContext;

struct gfxRect;

typedef nsSVGContainerFrame nsSVGPaintServerFrameBase;

class nsSVGPaintServerFrame : public nsSVGPaintServerFrameBase
{
protected:
  explicit nsSVGPaintServerFrame(nsStyleContext* aContext)
    : nsSVGPaintServerFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  







  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          const gfxMatrix& aContextMatrix,
                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                          float aOpacity,
                          const gfxRect *aOverrideBounds = nullptr) = 0;

  



  virtual bool SetupPaintServer(gfxContext *aContext,
                                nsIFrame *aSource,
                                nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                float aOpacity);

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsSVGPaintServerFrameBase::IsFrameOfType(aFlags & ~nsIFrame::eSVGPaintServer);
  }
};

#endif 
