





#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGEffects.h"
#include "nsSVGFilters.h"

typedef nsFrame SVGFELeafFrameBase;





class SVGFELeafFrame : public SVGFELeafFrameBase
{
  friend nsIFrame*
  NS_NewSVGFELeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit SVGFELeafFrame(nsStyleContext* aContext)
    : SVGFELeafFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return SVGFELeafFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFELeaf"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) MOZ_OVERRIDE;

  virtual bool UpdateOverflow() MOZ_OVERRIDE {
    
    return false;
  }
};

nsIFrame*
NS_NewSVGFELeafFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFELeafFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFELeafFrame)

#ifdef DEBUG
void
SVGFELeafFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eFILTER),
               "Trying to construct an SVGFELeafFrame for a "
               "content element that doesn't support the right interfaces");

  SVGFELeafFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
SVGFELeafFrame::GetType() const
{
  return nsGkAtoms::svgFELeafFrame;
}

nsresult
SVGFELeafFrame::AttributeChanged(int32_t  aNameSpaceID,
                                 nsIAtom* aAttribute,
                                 int32_t  aModType)
{
  nsSVGFE *element = static_cast<nsSVGFE*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFELeafFrameBase::AttributeChanged(aNameSpaceID,
                                                aAttribute, aModType);
}
