





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
  explicit nsSVGStopFrame(nsStyleContext* aContext)
    : nsSVGStopFrameBase(aContext)
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

  void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                        const nsRect&           aDirtyRect,
                        const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsSVGStopFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGStop"), aResult);
  }
#endif
};




NS_IMPL_FRAMEARENA_HELPERS(nsSVGStopFrame)




#ifdef DEBUG
void
nsSVGStopFrame::Init(nsIContent*       aContent,
                     nsContainerFrame* aParent,
                     nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::stop), "Content is not a stop element");

  nsSVGStopFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif 

nsIAtom *
nsSVGStopFrame::GetType() const
{
  return nsGkAtoms::svgStopFrame;
}

nsresult
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
