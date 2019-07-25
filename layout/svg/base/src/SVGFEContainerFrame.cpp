



































#include "nsContainerFrame.h"
#include "nsSVGEffects.h"
#include "nsSVGFilters.h"

typedef nsContainerFrame SVGFEContainerFrameBase;





class SVGFEContainerFrame : public SVGFEContainerFrameBase
{
  friend nsIFrame*
  NS_NewSVGFEContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  SVGFEContainerFrame(nsStyleContext* aContext) : SVGFEContainerFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return SVGFEContainerFrameBase::IsFrameOfType(
            aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGContainer));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFEContainer"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);
#endif
  




  virtual nsIAtom* GetType() const;

  NS_IMETHOD AttributeChanged(PRInt32  aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32  aModType);
};

nsIFrame*
NS_NewSVGFEContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFEContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFEContainerFrame)

 void
SVGFEContainerFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  SVGFEContainerFrameBase::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

#ifdef DEBUG
NS_IMETHODIMP
SVGFEContainerFrame::Init(nsIContent* aContent,
                          nsIFrame* aParent,
                          nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGFilterPrimitiveStandardAttributes> elem = do_QueryInterface(aContent);
  NS_ASSERTION(elem,
               "Trying to construct an SVGFEContainerFrame for a "
               "content element that doesn't support the right interfaces");

  return SVGFEContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
SVGFEContainerFrame::GetType() const
{
  return nsGkAtoms::svgFEContainerFrame;
}

NS_IMETHODIMP
SVGFEContainerFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                      nsIAtom* aAttribute,
                                      PRInt32  aModType)
{
  nsSVGFE *element = static_cast<nsSVGFE*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFEContainerFrameBase::AttributeChanged(aNameSpaceID,
                                                     aAttribute, aModType);
}
