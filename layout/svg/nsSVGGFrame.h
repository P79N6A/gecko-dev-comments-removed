




#ifndef NSSVGGFRAME_H
#define NSSVGGFRAME_H

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "nsSVGContainerFrame.h"

typedef nsSVGDisplayContainerFrame nsSVGGFrameBase;

class nsSVGGFrame : public nsSVGGFrameBase
{
  friend nsIFrame*
  NS_NewSVGGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGGFrame(nsStyleContext* aContext) :
    nsSVGGFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGG"), aResult);
  }
#endif

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  
  virtual void NotifySVGChanged(uint32_t aFlags) MOZ_OVERRIDE;

  
  virtual gfxMatrix GetCanvasTM() MOZ_OVERRIDE;

  nsAutoPtr<gfxMatrix> mCanvasTM;
};

#endif
