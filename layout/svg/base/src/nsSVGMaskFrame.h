



































#ifndef __NS_SVGMASKFRAME_H__
#define __NS_SVGMASKFRAME_H__

#include "nsSVGContainerFrame.h"
#include "gfxPattern.h"

class gfxContext;

typedef nsSVGContainerFrame nsSVGMaskFrameBase;

class nsSVGMaskFrame : public nsSVGMaskFrameBase
{
  friend nsIFrame*
  NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGMaskFrame(nsStyleContext* aContext) :
    nsSVGMaskFrameBase(aContext),
    mMaskParentMatrix(nsnull),
    mInUse(PR_FALSE) {}

public:
  
  already_AddRefed<gfxPattern> ComputeMaskAlpha(nsSVGRenderState *aContext,
                                                nsIFrame* aParent,
                                                nsIDOMSVGMatrix* aMatrix,
                                                float aOpacity = 1.0f);

  




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

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
};

#endif
