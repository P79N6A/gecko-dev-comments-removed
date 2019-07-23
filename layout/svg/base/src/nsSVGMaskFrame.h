



































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

  NS_IMETHOD InitSVG();

  nsSVGMaskFrame(nsStyleContext* aContext) : nsSVGMaskFrameBase(aContext) {}

 public:
  
  already_AddRefed<gfxPattern> ComputeMaskAlpha(nsSVGRenderState *aContext,
                                                nsISVGChildFrame* aParent,
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

  nsISVGChildFrame *mMaskParent;
  nsCOMPtr<nsIDOMSVGMatrix> mMaskParentMatrix;
  
  PRPackedBool mInUse;

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
};

nsIContent *
NS_GetSVGMaskElement(nsIURI *aURI, nsIContent *aContent);

#endif
