





#include "nsSVGGFrame.h"


#include "nsGkAtoms.h"
#include "SVGTransformableElement.h"
#include "nsIFrame.h"
#include "SVGGraphicsElement.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"

using namespace mozilla::dom;




nsIFrame*
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGGFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGFrame)

#ifdef DEBUG
void
nsSVGGFrame::Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG() &&
               static_cast<nsSVGElement*>(aContent)->IsTransformable(),
               "The element doesn't support nsIDOMSVGTransformable");

  nsSVGGFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGGFrame::GetType() const
{
  return nsGkAtoms::svgGFrame;
}




void
nsSVGGFrame::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nullptr;
  }

  nsSVGGFrameBase::NotifySVGChanged(aFlags);
}

gfxMatrix
nsSVGGFrame::GetCanvasTM(uint32_t aFor, nsIFrame* aTransformRoot)
{
  if (!(GetStateBits() & NS_FRAME_IS_NONDISPLAY) && !aTransformRoot) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    SVGGraphicsElement *content = static_cast<SVGGraphicsElement*>(mContent);
    gfxMatrix tm = content->PrependLocalTransformsTo(
        this == aTransformRoot ? gfxMatrix() :
                                 parent->GetCanvasTM(aFor, aTransformRoot));

    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}

nsresult
nsSVGGFrame::AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::transform) {
    
    
    
    
    NotifySVGChanged(TRANSFORM_CHANGED);
  }
  
  return NS_OK;
}
