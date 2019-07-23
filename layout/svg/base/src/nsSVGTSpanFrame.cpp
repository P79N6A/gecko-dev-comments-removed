





































#include "nsIDOMSVGTSpanElement.h"
#include "nsSVGTSpanFrame.h"
#include "nsSVGUtils.h"
#include "nsSVGTextFrame.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGMatrix.h"




nsIFrame*
NS_NewSVGTSpanFrame(nsIPresShell* aPresShell, nsIContent* aContent,
                    nsIFrame* parentFrame, nsStyleContext* aContext)
{
  NS_ASSERTION(parentFrame, "null parent");
  nsISVGTextContentMetrics *metrics;
  CallQueryInterface(parentFrame, &metrics);
  if (!metrics) {
    NS_ERROR("trying to construct an SVGTSpanFrame for an invalid container");
    return nsnull;
  }
  
  nsCOMPtr<nsIDOMSVGTSpanElement> tspan = do_QueryInterface(aContent);
  if (!tspan) {
    NS_ERROR("Can't create frame! Content is not an SVG tspan");
    return nsnull;
  }

  return new (aPresShell) nsSVGTSpanFrame(aContext);
}

nsIAtom *
nsSVGTSpanFrame::GetType() const
{
  return nsGkAtoms::svgTSpanFrame;
}




NS_INTERFACE_MAP_BEGIN(nsSVGTSpanFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGGlyphFragmentNode)
NS_INTERFACE_MAP_END_INHERITING(nsSVGTSpanFrameBase)




NS_IMETHODIMP
nsSVGTSpanFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::x ||
       aAttribute == nsGkAtoms::y ||
       aAttribute == nsGkAtoms::dx ||
       aAttribute == nsGkAtoms::dy)) {
    UpdateGraphic();
  }

  return NS_OK;
}




NS_IMETHODIMP
nsSVGTSpanFrame::SetMatrixPropagation(PRBool aPropagate)
{
  mPropagateTransform = aPropagate;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGTSpanFrame::SetOverrideCTM(nsIDOMSVGMatrix *aCTM)
{
  mOverrideCTM = aCTM;
  return NS_OK;
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGTSpanFrame::GetCanvasTM()
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

  NS_ASSERTION(mParent, "null parent");
  nsSVGContainerFrame *containerFrame = static_cast<nsSVGContainerFrame*>
                                                   (mParent);
  return containerFrame->GetCanvasTM();  
}




NS_IMETHODIMP_(PRUint32)
nsSVGTSpanFrame::GetNumberOfChars()
{
  return nsSVGTSpanFrameBase::GetNumberOfChars();
}

NS_IMETHODIMP_(float)
nsSVGTSpanFrame::GetComputedTextLength()
{
  return nsSVGTSpanFrameBase::GetComputedTextLength();
}

NS_IMETHODIMP_(float)
nsSVGTSpanFrame::GetSubStringLength(PRUint32 charnum, PRUint32 nchars)
{
  return nsSVGTSpanFrameBase::GetSubStringLengthNoValidation(charnum, nchars);
}

NS_IMETHODIMP_(PRInt32)
nsSVGTSpanFrame::GetCharNumAtPosition(nsIDOMSVGPoint *point)
{
  return nsSVGTSpanFrameBase::GetCharNumAtPosition(point);
}

NS_IMETHODIMP_(nsISVGGlyphFragmentLeaf *)
nsSVGTSpanFrame::GetFirstGlyphFragment()
{
  
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGGlyphFragmentNode *node = nsnull;
    CallQueryInterface(kid, &node);
    if (node)
      return node->GetFirstGlyphFragment();
    kid = kid->GetNextSibling();
  }

  
  return GetNextGlyphFragment();

}

NS_IMETHODIMP_(nsISVGGlyphFragmentLeaf *)
nsSVGTSpanFrame::GetNextGlyphFragment()
{
  nsIFrame* sibling = mNextSibling;
  while (sibling) {
    nsISVGGlyphFragmentNode *node = nsnull;
    CallQueryInterface(sibling, &node);
    if (node)
      return node->GetFirstGlyphFragment();
    sibling = sibling->GetNextSibling();
  }

  
  
  NS_ASSERTION(mParent, "null parent");
  nsISVGGlyphFragmentNode *node = nsnull;
  CallQueryInterface(mParent, &node);
  return node ? node->GetNextGlyphFragment() : nsnull;
}

NS_IMETHODIMP_(void)
nsSVGTSpanFrame::SetWhitespaceHandling(PRUint8 aWhitespaceHandling)
{
  nsSVGTSpanFrameBase::SetWhitespaceHandling();
}
