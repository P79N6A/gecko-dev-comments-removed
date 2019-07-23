





































#include "nsIDOMSVGTransformable.h"
#include "nsSVGGFrame.h"
#include "nsIFrame.h"
#include "nsSVGMatrix.h"
#include "nsGkAtoms.h"
#include "nsSVGUtils.h"
#include "nsSVGGraphicElement.h"




nsIFrame*
NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGGFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsSVGGFrame::Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGTransformable> transformable = do_QueryInterface(aContent);
  NS_ASSERTION(transformable,
               "The element doesn't support nsIDOMSVGTransformable\n");

  return nsSVGGFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGGFrame::GetType() const
{
  return nsGkAtoms::svgGFrame;
}




void
nsSVGGFrame::NotifySVGChanged(PRUint32 aFlags)
{
  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nsnull;
  }

  nsSVGGFrameBase::NotifySVGChanged(aFlags);
}

gfxMatrix
nsSVGGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGGraphicElement *content = static_cast<nsSVGGraphicElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformTo(parent->GetCanvasTM());

    mCanvasTM = NS_NewSVGMatrix(tm);
  }
  return nsSVGUtils::ConvertSVGMatrixToThebes(mCanvasTM);
}

NS_IMETHODIMP
nsSVGGFrame::AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::transform) {
    
    mCanvasTM = nsnull;

    nsSVGUtils::NotifyChildrenOfSVGChange(this, TRANSFORM_CHANGED);
  }
  
  return NS_OK;
}
