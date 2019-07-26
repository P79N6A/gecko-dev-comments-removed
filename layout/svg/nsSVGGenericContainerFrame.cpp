





#include "nsSVGGenericContainerFrame.h"
#include "nsSVGIntegrationUtils.h"




nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGGenericContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGenericContainerFrame)




NS_IMETHODIMP
nsSVGGenericContainerFrame::AttributeChanged(int32_t         aNameSpaceID,
                                             nsIAtom*        aAttribute,
                                             int32_t         aModType)
{
#ifdef DEBUG
    nsAutoString str;
    aAttribute->ToString(str);
    printf("** nsSVGGenericContainerFrame::AttributeChanged(%s)\n",
           NS_LossyConvertUTF16toASCII(str).get());
#endif

  return NS_OK;
}

nsIAtom *
nsSVGGenericContainerFrame::GetType() const
{
  return nsGkAtoms::svgGenericContainerFrame;
}




gfxMatrix
nsSVGGenericContainerFrame::GetCanvasTM(uint32_t aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }

  NS_ASSERTION(mParent, "null parent");
  
  return static_cast<nsSVGContainerFrame*>(mParent)->GetCanvasTM(aFor);
}
