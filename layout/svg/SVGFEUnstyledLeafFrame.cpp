





#include "nsContainerFrame.h"
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
  explicit SVGFEUnstyledLeafFrame(nsStyleContext* aContext)
    : SVGFEUnstyledLeafFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT | NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  virtual void BuildDisplayList(nsDisplayListBuilder* aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override {}

  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    return SVGFEUnstyledLeafFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGFEUnstyledLeaf"), aResult);
  }
#endif

  




  virtual nsIAtom* GetType() const override;

  virtual nsresult AttributeChanged(int32_t  aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t  aModType) override;

  virtual bool UpdateOverflow() override {
    
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

nsresult
SVGFEUnstyledLeafFrame::AttributeChanged(int32_t  aNameSpaceID,
                                         nsIAtom* aAttribute,
                                         int32_t  aModType)
{
  SVGFEUnstyledElement *element = static_cast<SVGFEUnstyledElement*>(mContent);
  if (element->AttributeAffectsRendering(aNameSpaceID, aAttribute)) {
    MOZ_ASSERT(GetParent()->GetParent()->GetType() == nsGkAtoms::svgFilterFrame,
               "Observers observe the filter, so that's what we must invalidate");
    nsSVGEffects::InvalidateDirectRenderingObservers(GetParent()->GetParent());
  }

  return SVGFEUnstyledLeafFrameBase::AttributeChanged(aNameSpaceID,
                                                        aAttribute, aModType);
}
