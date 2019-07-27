





#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsLiteralString.h"
#include "nsSVGEffects.h"
#include "nsSVGFilters.h"

typedef nsContainerFrame SVGFEContainerFrameBase;





class SVGFEContainerFrame : public SVGFEContainerFrameBase
{
  friend nsIFrame*
  NS_NewSVGFEContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit SVGFEContainerFrame(nsStyleContext* aContext)
    : SVGFEContainerFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return SVGFEContainerFrameBase::IsFrameOfType(
            aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGContainer));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFEContainer"), aResult);
  }
#endif

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
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
NS_NewSVGFEContainerFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGFEContainerFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGFEContainerFrame)

#ifdef DEBUG
void
SVGFEContainerFrame::Init(nsIContent*       aContent,
                          nsContainerFrame* aParent,
                          nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsNodeOfType(nsINode::eFILTER),
               "Trying to construct an SVGFEContainerFrame for a "
               "content element that doesn't support the right interfaces");

  SVGFEContainerFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
SVGFEContainerFrame::GetType() const
{
  return nsGkAtoms::svgFEContainerFrame;
}

nsresult
SVGFEContainerFrame::AttributeChanged(int32_t  aNameSpaceID,
                                      nsIAtom* aAttribute,
                                      int32_t  aModType)
{
  nsSVGFE *element = static_cast<nsSVGFE*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return SVGFEContainerFrameBase::AttributeChanged(aNameSpaceID,
                                                     aAttribute, aModType);
}
