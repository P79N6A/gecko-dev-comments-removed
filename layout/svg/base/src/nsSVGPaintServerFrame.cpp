



































#include "nsSVGPaintServerFrame.h"
#include "nsSVGGeometryFrame.h"

NS_IMPL_FRAMEARENA_HELPERS(nsSVGPaintServerFrame)

bool
nsSVGPaintServerFrame::SetupPaintServer(gfxContext *aContext,
                                        nsSVGGeometryFrame *aSource,
                                        float aOpacity)
{
  nsRefPtr<gfxPattern> pattern = GetPaintServerPattern(aSource, aOpacity);
  if (!pattern)
    return false;

  aContext->SetPattern(pattern);
  return true;
}
