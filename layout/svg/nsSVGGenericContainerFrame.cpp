





#include "nsSVGGenericContainerFrame.h"
#include "nsSVGIntegrationUtils.h"




nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGGenericContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGenericContainerFrame)




nsresult
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
nsSVGGenericContainerFrame::GetCanvasTM(uint32_t aFor,
                                        nsIFrame* aTransformRoot)
{
  if (!(GetStateBits() & NS_FRAME_IS_NONDISPLAY) && !aTransformRoot) {
    if (aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
    if (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled()) {
      return gfxMatrix();
    }
  }

  NS_ASSERTION(GetParent(), "null parent");
  
  return static_cast<nsSVGContainerFrame*>(GetParent())->
      GetCanvasTM(aFor, aTransformRoot);
}
