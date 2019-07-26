





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
  SVGViewFrame(nsStyleContext* aContext)
    : SVGViewFrameBase(aContext)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent* aContent,
                    nsIFrame*   aParent,
                    nsIFrame*   aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return SVGViewFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGView"), aResult);
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
NS_NewSVGViewFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) SVGViewFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(SVGViewFrame)

#ifdef DEBUG
void
SVGViewFrame::Init(nsIContent* aContent,
                   nsIFrame* aParent,
                   nsIFrame* aPrevInFlow)
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

NS_IMETHODIMP
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
