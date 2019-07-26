




#ifndef NSSVGGFRAME_H
#define NSSVGGFRAME_H

#include "gfxMatrix.h"
#include "nsSVGContainerFrame.h"

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
  virtual void Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
#endif

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGG"), aResult);
  }
#endif

  
  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  
  virtual void NotifySVGChanged(uint32_t aFlags);

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);

  nsAutoPtr<gfxMatrix> mCanvasTM;
};

#endif
