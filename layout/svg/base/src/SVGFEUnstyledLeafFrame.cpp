



































#include "nsFrame.h"
#include "nsSVGFilters.h"
#include "nsSVGEffects.h"

typedef nsFrame SVGFEUnstyledLeafFrameBase;

class SVGFEUnstyledLeafFrame : public SVGFEUnstyledLeafFrameBase
{
  friend nsIFrame*
  NS_NewSVGFEUnstyledLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  SVGFEUnstyledLeafFrame(nsStyleContext* aContext) : SVGFEUnstyledLeafFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return SVGFEUnstyledLeafFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFEUnstyledLeaf"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const;

  NS_IMETHOD AttributeChanged(PRInt32  aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32  aModType);
};

nsIFrame*
NS_NewSVGFEUnstyledLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFEUnstyledLeafFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFEUnstyledLeafFrame)

nsIAtom *
SVGFEUnstyledLeafFrame::GetType() const
{
  return nsGkAtoms::svgFEUnstyledLeafFrame;
}

NS_IMETHODIMP
SVGFEUnstyledLeafFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         PRInt32  aModType)
{
  SVGFEUnstyledElement *element = static_cast<SVGFEUnstyledElement*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFEUnstyledLeafFrameBase::AttributeChanged(aNameSpaceID,
                                                        aAttribute, aModType);
}
