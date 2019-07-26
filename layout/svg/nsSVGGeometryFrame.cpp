





#include "nsSVGGeometryFrame.h"


#include "gfxContext.h"
#include "gfxSVGGlyphs.h"
#include "nsPresContext.h"
#include "nsSVGEffects.h"
#include "nsSVGPaintServerFrame.h"
#include "nsSVGUtils.h"

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGeometryFrame)




void
nsSVGGeometryFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
  AddStateBits(aParent->GetStateBits() &
               (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_CLIPPATH_CHILD));
  nsSVGGeometryFrameBase::Init(aContent, aParent, aPrevInFlow);
}



uint16_t
nsSVGGeometryFrame::GetClipRule()
{
  return StyleSVG()->mClipRule;
}

uint16_t
nsSVGGeometryFrame::GetHitTestFlags()
{
  return nsSVGUtils::GetGeometryHitTestFlags(this);
}
