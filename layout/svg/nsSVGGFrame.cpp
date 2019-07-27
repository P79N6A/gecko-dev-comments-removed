





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
nsSVGGFrame::Init(nsIContent*       aContent,
                  nsContainerFrame* aParent,
                  nsIFrame*         aPrevInFlow)
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
nsSVGGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(GetParent(), "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
    SVGGraphicsElement *content = static_cast<SVGGraphicsElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM());

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
