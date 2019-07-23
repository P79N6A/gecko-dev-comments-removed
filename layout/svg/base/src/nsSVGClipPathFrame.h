



































#ifndef __NS_SVGCLIPPATHFRAME_H__
#define __NS_SVGCLIPPATHFRAME_H__

#include "nsSVGContainerFrame.h"

typedef nsSVGContainerFrame nsSVGClipPathFrameBase;

class nsSVGClipPathFrame : public nsSVGClipPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

  NS_IMETHOD InitSVG();

 public:
  nsSVGClipPathFrame(nsStyleContext* aContext) : nsSVGClipPathFrameBase(aContext) {}

  
  NS_IMETHOD ClipPaint(nsSVGRenderState* aContext,
                       nsISVGChildFrame* aParent,
                       nsCOMPtr<nsIDOMSVGMatrix> aMatrix);

  NS_IMETHOD ClipHitTest(nsISVGChildFrame* aParent,
                         nsCOMPtr<nsIDOMSVGMatrix> aMatrix,
                         float aX, float aY, PRBool *aHit);

  
  
  
  NS_IMETHOD IsTrivial(PRBool *aTrivial);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGClipPath"), aResult);
  }
#endif

 private:
  nsISVGChildFrame *mClipParent;
  nsCOMPtr<nsIDOMSVGMatrix> mClipParentMatrix;

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  PRPackedBool mInUse;
};

nsSVGClipPathFrame *
NS_GetSVGClipPathFrame(nsIURI *aURI, nsIContent *aContent);

#endif
