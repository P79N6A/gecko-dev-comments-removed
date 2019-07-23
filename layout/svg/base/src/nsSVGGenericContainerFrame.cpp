





































#include "nsSVGGenericContainerFrame.h"
#include "nsSVGUtils.h"




nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGGenericContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGGenericContainerFrame)




NS_IMETHODIMP
nsSVGGenericContainerFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                             nsIAtom*        aAttribute,
                                             PRInt32         aModType)
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
nsSVGGenericContainerFrame::GetCanvasTM()
{
  NS_ASSERTION(mParent, "null parent");
  
  return static_cast<nsSVGContainerFrame*>(mParent)->GetCanvasTM();  
}
