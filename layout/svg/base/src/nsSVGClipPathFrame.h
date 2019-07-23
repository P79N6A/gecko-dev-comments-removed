



































#ifndef __NS_SVGCLIPPATHFRAME_H__
#define __NS_SVGCLIPPATHFRAME_H__

#include "nsSVGContainerFrame.h"
#include "gfxMatrix.h"

typedef nsSVGContainerFrame nsSVGClipPathFrameBase;

class nsSVGClipPathFrame : public nsSVGClipPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGClipPathFrame(nsStyleContext* aContext) :
    nsSVGClipPathFrameBase(aContext),
    mClipParentMatrix(nsnull),
    mInUse(PR_FALSE) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  nsresult ClipPaint(nsSVGRenderState* aContext,
                     nsIFrame* aParent,
                     const gfxMatrix &aMatrix);

  PRBool ClipHitTest(nsIFrame* aParent,
                     const gfxMatrix &aMatrix,
                     const nsPoint &aPoint);

  
  
  
  PRBool IsTrivial();

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




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

  nsIFrame *mClipParent;
  nsCOMPtr<nsIDOMSVGMatrix> mClipParentMatrix;
  
  PRPackedBool mInUse;

  
  virtual gfxMatrix GetCanvasTM();
};

#endif
