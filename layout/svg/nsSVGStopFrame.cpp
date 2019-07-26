





#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsStyleContext.h"
#include "nsSVGEffects.h"





typedef nsFrame  nsSVGStopFrameBase;

class nsSVGStopFrame : public nsSVGStopFrameBase
{
  friend nsIFrame*
  NS_NewSVGStopFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGStopFrame(nsStyleContext* aContext)
    : nsSVGStopFrameBase(aContext)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  




  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsSVGStopFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGStop"), aResult);
  }
#endif
};




NS_IMPL_FRAMEARENA_HELPERS(nsSVGStopFrame)




#ifdef DEBUG
NS_IMETHODIMP
nsSVGStopFrame::Init(nsIContent* aContent,
                     nsIFrame* aParent,
                     nsIFrame* aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::stop), "Content is not a stop element");

  return nsSVGStopFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

 void
nsSVGStopFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGStopFrameBase::DidSetStyleContext(aOldStyleContext);
  nsSVGEffects::InvalidateRenderingObservers(this);
}

nsIAtom *
nsSVGStopFrame::GetType() const
{
  return nsGkAtoms::svgStopFrame;
}

NS_IMETHODIMP
nsSVGStopFrame::AttributeChanged(int32_t         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::offset) {
    nsSVGEffects::InvalidateRenderingObservers(this);
  }

  return nsSVGStopFrameBase::AttributeChanged(aNameSpaceID,
                                              aAttribute, aModType);
}





nsIFrame* NS_NewSVGStopFrame(nsIPresShell*   aPresShell,
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGStopFrame(aContext);
}
