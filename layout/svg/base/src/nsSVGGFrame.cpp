





































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

already_AddRefed<nsIDOMSVGMatrix>
nsSVGGFrame::GetCanvasTM()
{
  if (!GetMatrixPropagation()) {
    nsIDOMSVGMatrix *retval;
    NS_NewSVGMatrix(&retval);
    return retval;
  }

  if (!mCanvasTM) {
    
    NS_ASSERTION(mParent, "null parent");
    nsSVGContainerFrame *containerFrame = static_cast<nsSVGContainerFrame*>
                                                     (mParent);
    nsCOMPtr<nsIDOMSVGMatrix> parentTM = containerFrame->GetCanvasTM();
    NS_ASSERTION(parentTM, "null TM");

    
    nsSVGGraphicElement *element =
      static_cast<nsSVGGraphicElement*>(mContent);
    nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();

    if (localTM)
      parentTM->Multiply(localTM, getter_AddRefs(mCanvasTM));
    else
      mCanvasTM = parentTM;
  }

  nsIDOMSVGMatrix* retval = mCanvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
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
