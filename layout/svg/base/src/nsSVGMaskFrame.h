



































#ifndef __NS_SVGMASKFRAME_H__
#define __NS_SVGMASKFRAME_H__

#include "nsSVGContainerFrame.h"
#include "gfxPattern.h"
#include "gfxMatrix.h"

class gfxContext;

typedef nsSVGContainerFrame nsSVGMaskFrameBase;

class nsSVGMaskFrame : public nsSVGMaskFrameBase
{
  friend nsIFrame*
  NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGMaskFrame(nsStyleContext* aContext) :
    nsSVGMaskFrameBase(aContext),
    mMaskParentMatrix(nsnull),
    mInUse(PR_FALSE) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  already_AddRefed<gfxPattern> ComputeMaskAlpha(nsSVGRenderState *aContext,
                                                nsIFrame* aParent,
                                                const gfxMatrix &aMatrix,
                                                float aOpacity = 1.0f);

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMask"), aResult);
  }
#endif

private:
  
  
  
  
  class AutoMaskReferencer
  {
  public:
    AutoMaskReferencer(nsSVGMaskFrame *aFrame)
       : mFrame(aFrame) {
      NS_ASSERTION(mFrame->mInUse == PR_FALSE, "reference loop!");
      mFrame->mInUse = PR_TRUE;
    }
    ~AutoMaskReferencer() {
      mFrame->mInUse = PR_FALSE;
    }
  private:
    nsSVGMaskFrame *mFrame;
  };

  nsIFrame *mMaskParent;
  nsCOMPtr<nsIDOMSVGMatrix> mMaskParentMatrix;
  
  PRPackedBool mInUse;

  
  virtual gfxMatrix GetCanvasTM();
};

#endif
