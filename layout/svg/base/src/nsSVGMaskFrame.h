



































#ifndef __NS_SVGMASKFRAME_H__
#define __NS_SVGMASKFRAME_H__

#include "nsSVGContainerFrame.h"
#include "cairo.h"

class gfxContext;

typedef nsSVGContainerFrame nsSVGMaskFrameBase;

class nsSVGMaskFrame : public nsSVGMaskFrameBase
{
  friend nsIFrame*
  NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

  NS_IMETHOD InitSVG();

 public:
  nsSVGMaskFrame(nsStyleContext* aContext) : nsSVGMaskFrameBase(aContext) {}

  
  cairo_pattern_t *ComputeMaskAlpha(nsSVGRenderState *aContext,
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
  PRUint16 GetMaskUnits();
  PRUint16 GetMaskContentUnits();

  nsISVGChildFrame *mMaskParent;
  nsCOMPtr<nsIDOMSVGMatrix> mMaskParentMatrix;

  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
};

nsSVGMaskFrame *
NS_GetSVGMaskFrame(nsIURI *aURI, nsIContent *aContent);

#endif
