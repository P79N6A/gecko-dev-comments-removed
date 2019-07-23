





































#include "nsIDOMSVGStopElement.h"
#include "nsStyleContext.h"
#include "nsFrame.h"
#include "nsGkAtoms.h"
#include "nsSVGEffects.h"





typedef nsFrame  nsSVGStopFrameBase;

class nsSVGStopFrame : public nsSVGStopFrameBase
{
  friend nsIFrame*
  NS_NewSVGStopFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGStopFrame(nsStyleContext* aContext) : nsSVGStopFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
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
  nsCOMPtr<nsIDOMSVGStopElement> grad = do_QueryInterface(aContent);
  NS_ASSERTION(grad, "Content doesn't support nsIDOMSVGStopElement");

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
nsSVGStopFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                 nsIAtom*        aAttribute,
                                 PRInt32         aModType)
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
