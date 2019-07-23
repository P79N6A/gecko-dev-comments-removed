





































#include "nsSVGGenericContainerFrame.h"
#include "nsSVGUtils.h"




nsIFrame*
NS_NewSVGGenericContainerFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGGenericContainerFrame(aContext);
}




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




already_AddRefed<nsIDOMSVGMatrix>
nsSVGGenericContainerFrame::GetCanvasTM()
{
  NS_ASSERTION(mParent, "null parent");
  
  nsSVGContainerFrame *containerFrame = static_cast<nsSVGContainerFrame*>
                                                   (mParent);

  return containerFrame->GetCanvasTM();  
}
