



































#ifndef __NS_SVGCLIPPATHFRAME_H__
#define __NS_SVGCLIPPATHFRAME_H__

#include "nsSVGContainerFrame.h"

typedef nsSVGContainerFrame nsSVGClipPathFrameBase;

class nsSVGClipPathFrame : public nsSVGClipPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

  NS_IMETHOD InitSVG();

  nsSVGClipPathFrame(nsStyleContext* aContext) : nsSVGClipPathFrameBase(aContext) {}

 public:
  
  nsresult ClipPaint(nsSVGRenderState* aContext,
                     nsISVGChildFrame* aParent,
                     nsIDOMSVGMatrix *aMatrix);

  PRBool ClipHitTest(nsISVGChildFrame* aParent,
                     nsIDOMSVGMatrix *aMatrix,
                     float aX, float aY);

  
  
  
  PRBool IsTrivial();

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGClipPath"), aResult);
  }
#endif

 private:
  
  
  
  
  class AutoClipPathReferencer
  {
  public:
    AutoClipPathReferencer(nsSVGClipPathFrame *aFrame)
       : mFrame(aFrame) {
      NS_ASSERTION(mFrame->mInUse == PR_FALSE, "reference loop!");
      mFrame->mInUse = PR_TRUE;
    }
    ~AutoClipPathReferencer() {
      mFrame->mInUse = PR_FALSE;
    }
  private:
    nsSVGClipPathFrame *mFrame;
  };

  nsISVGChildFrame *mClipParent;
  nsCOMPtr<nsIDOMSVGMatrix> mClipParentMatrix;

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  PRPackedBool mInUse;
};

nsIContent *
NS_GetSVGClipPathElement(nsIURI *aURI, nsIContent *aContent);

#endif
