





#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGOuterSVGFrame.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGViewElement.h"

typedef nsFrame SVGViewFrameBase;

using namespace mozilla::dom;







class SVGViewFrame : public SVGViewFrameBase
{
  friend nsIFrame*
  NS_NewSVGViewFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit SVGViewFrame(nsStyleContext* aContext)
    : SVGViewFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
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
    return SVGViewFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGView"), aResult);
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
NS_NewSVGViewFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGViewFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGViewFrame)

#ifdef DEBUG
void
SVGViewFrame::Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::view),
               "Content is not an SVG view");

  SVGViewFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
SVGViewFrame::GetType() const
{
  return nsGkAtoms::svgViewFrame;
}

nsresult
SVGViewFrame::AttributeChanged(int32_t  aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t  aModType)
{
  
    
  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::preserveAspectRatio ||
       aAttribute == nsGkAtoms::viewBox ||
       aAttribute == nsGkAtoms::viewTarget)) {

    nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
    NS_ASSERTION(outerSVGFrame->GetContent()->IsSVG(nsGkAtoms::svg),
                 "Expecting an <svg> element");

    SVGSVGElement* svgElement =
      static_cast<SVGSVGElement*>(outerSVGFrame->GetContent());

    nsAutoString viewID;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::id, viewID);

    if (svgElement->IsOverriddenBy(viewID)) {
      
      
      outerSVGFrame->AttributeChanged(aNameSpaceID, aAttribute, aModType);
    }
  }

  return SVGViewFrameBase::AttributeChanged(aNameSpaceID,
                                            aAttribute, aModType);
}
