





#include "nsSVGPaintServerFrame.h"


#include "nsSVGElement.h"
#include "nsSVGGeometryFrame.h"

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPaintServerFrame)

bool
nsSVGPaintServerFrame::SetupPaintServer(gfxContext *aContext,
                                        nsIFrame *aSource,
                                        nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                                        float aOpacity)
{
  nsRefPtr<gfxPattern> pattern =
    GetPaintServerPattern(aSource, aContext->CurrentMatrix(), aFillOrStroke,
                          aOpacity);
  if (!pattern)
    return false;

  aContext->SetPattern(pattern);
  return true;
}
