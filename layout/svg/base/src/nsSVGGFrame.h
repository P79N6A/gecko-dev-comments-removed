





































#ifndef NSSVGGFRAME_H
#define NSSVGGFRAME_H

#include "nsSVGContainerFrame.h"

typedef nsSVGDisplayContainerFrame nsSVGGFrameBase;

class nsSVGGFrame : public nsSVGGFrameBase
{
  friend nsIFrame*
  NS_NewSVGGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGGFrame(nsStyleContext* aContext) :
    nsSVGGFrameBase(aContext) {}

public:
  




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
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  virtual already_AddRefed<nsIDOMSVGMatrix> GetOverrideCTM();

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;
};

#endif
