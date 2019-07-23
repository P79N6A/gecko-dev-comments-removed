





































#include "nsSVGTSpanFrame.h"
#include "nsISVGGlyphFragmentNode.h"
#include "nsSVGGraphicElement.h"
#include "nsSVGMatrix.h"
#include "nsIDOMSVGAElement.h"
#include "nsSVGUtils.h"






typedef nsSVGTSpanFrame nsSVGAFrameBase;

class nsSVGAFrame : public nsSVGAFrameBase
{
  friend nsIFrame*
  NS_NewSVGAFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                  nsStyleContext* aContext);
protected:
  nsSVGAFrame(nsStyleContext* aContext) :
    nsSVGAFrameBase(aContext) {}

public:
  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  NS_IMETHOD DidSetStyleContext();

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGA"), aResult);
  }
#endif
  
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation);
  
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  
private:
  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
};




nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGAElement> elem = do_QueryInterface(aContent);
  if (!elem) {
    NS_ERROR("Trying to construct an SVGAFrame for a "
             "content element that doesn't support the right interfaces");
    return nsnull;
  }

  return new (aPresShell) nsSVGAFrame(aContext);
}




NS_IMETHODIMP
nsSVGAFrame::AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType)
{
  if (aNameSpaceID != kNameSpaceID_None)
    return NS_OK;

  if (aAttribute == nsGkAtoms::transform) {
    

    
    mCanvasTM = nsnull;
    
    nsIFrame* kid = mFrames.FirstChild();
    while (kid) {
      nsISVGChildFrame* SVGFrame = nsnull;
      CallQueryInterface(kid, &SVGFrame);
      if (SVGFrame)
        SVGFrame->NotifyCanvasTMChanged(PR_FALSE);
      kid = kid->GetNextSibling();
    }
  }

 return NS_OK;
}

NS_IMETHODIMP
nsSVGAFrame::DidSetStyleContext()
{
  nsSVGUtils::StyleEffects(this);

  return NS_OK;
}

nsIAtom *
nsSVGAFrame::GetType() const
{
  return nsGkAtoms::svgAFrame;
}




NS_IMETHODIMP
nsSVGAFrame::NotifyCanvasTMChanged(PRBool suppressInvalidation)
{
  
  mCanvasTM = nsnull;

  return nsSVGAFrameBase::NotifyCanvasTMChanged(suppressInvalidation);
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGAFrame::GetCanvasTM()
{
  if (!mPropagateTransform) {
    nsIDOMSVGMatrix *retval;
    if (mOverrideCTM) {
      retval = mOverrideCTM;
      NS_ADDREF(retval);
    } else {
      NS_NewSVGMatrix(&retval);
    }
    return retval;
  }

  if (!mCanvasTM) {
    
    NS_ASSERTION(mParent, "null parent");
    nsSVGContainerFrame *containerFrame = NS_STATIC_CAST(nsSVGContainerFrame*,
                                                         mParent);
    nsCOMPtr<nsIDOMSVGMatrix> parentTM = containerFrame->GetCanvasTM();
    NS_ASSERTION(parentTM, "null TM");

    
    nsSVGGraphicElement *element =
      NS_STATIC_CAST(nsSVGGraphicElement*, mContent);
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
