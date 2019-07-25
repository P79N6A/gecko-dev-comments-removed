



































#include "nsFrame.h"
#include "nsSVGFilters.h"
#include "nsSVGEffects.h"

typedef nsFrame SVGFELeafFrameBase;





class SVGFELeafFrame : public SVGFELeafFrameBase
{
  friend nsIFrame*
  NS_NewSVGFELeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  SVGFELeafFrame(nsStyleContext* aContext) : SVGFELeafFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame*   aParent,
                  nsIFrame*   aPrevInFlow);
#endif

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return SVGFELeafFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFELeaf"), aResult);
  }
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  




  virtual nsIAtom* GetType() const;

  NS_IMETHOD AttributeChanged(PRInt32  aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32  aModType);
};

nsIFrame*
NS_NewSVGFELeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFELeafFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFELeafFrame)

 void
SVGFELeafFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  SVGFELeafFrameBase::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

#ifdef DEBUG
NS_IMETHODIMP
SVGFELeafFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  nsCOMPtr<nsIDOMSVGFilterPrimitiveStandardAttributes> elem = do_QueryInterface(aContent);
  NS_ASSERTION(elem,
               "Trying to construct an SVGFELeafFrame for a "
               "content element that doesn't support the right interfaces");

  return SVGFELeafFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
SVGFELeafFrame::GetType() const
{
  return nsGkAtoms::svgFELeafFrame;
}

NS_IMETHODIMP
SVGFELeafFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 PRInt32  aModType)
{
  nsSVGFE *element = static_cast<nsSVGFE*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFELeafFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}
