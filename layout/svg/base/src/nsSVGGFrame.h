





































#ifndef NSSVGGFRAME_H
#define NSSVGGFRAME_H

#include "nsSVGContainerFrame.h"
#include "gfxMatrix.h"

typedef nsSVGDisplayContainerFrame nsSVGGFrameBase;

class nsSVGGFrame : public nsSVGGFrameBase
{
  friend nsIFrame*
  NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGGFrame(nsStyleContext* aContext) :
    nsSVGGFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGG"), aResult);
  }
#endif

  
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  
  virtual void NotifySVGChanged(PRUint32 aFlags);

  
  virtual gfxMatrix GetCanvasTM();

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
};

#endif
