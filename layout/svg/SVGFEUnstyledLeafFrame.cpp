





#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGEffects.h"
#include "nsSVGFilters.h"

typedef nsFrame SVGFEUnstyledLeafFrameBase;

class SVGFEUnstyledLeafFrame : public SVGFEUnstyledLeafFrameBase
{
  friend nsIFrame*
  NS_NewSVGFEUnstyledLeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  SVGFEUnstyledLeafFrame(nsStyleContext* aContext)
    : SVGFEUnstyledLeafFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT | NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return NS_OK;
  }

  virtual bool IsFrameOfType(uint32_t aFlags) const
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

  NS_IMETHOD AttributeChanged(int32_t  aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t  aModType);

  virtual bool UpdateOverflow() {
    
    return false;
  }
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
SVGFEUnstyledLeafFrame::AttributeChanged(int32_t  aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         int32_t  aModType)
{
  SVGFEUnstyledElement *element = static_cast<SVGFEUnstyledElement*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFEUnstyledLeafFrameBase::AttributeChanged(aNameSpaceID,
                                                        aAttribute, aModType);
}
