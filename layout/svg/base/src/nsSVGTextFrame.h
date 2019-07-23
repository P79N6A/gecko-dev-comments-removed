





































#ifndef NS_SVGTEXTFRAME_H
#define NS_SVGTEXTFRAME_H

#include "nsSVGTextContainerFrame.h"

typedef nsSVGTextContainerFrame nsSVGTextFrameBase;

class nsSVGTextFrame : public nsSVGTextFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGTextFrame(nsStyleContext* aContext)
    : nsSVGTextFrameBase(aContext),
      mMetricsState(unsuspended),
      mPropagateTransform(PR_TRUE),
      mPositioningDirty(PR_TRUE) {}

public:
  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGText"), aResult);
  }
#endif

  
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  virtual already_AddRefed<nsIDOMSVGMatrix> GetOverrideCTM();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  
  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint & aPoint);
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  
  
  NS_IMETHOD GetNumberOfChars(PRInt32 *_retval);
  NS_IMETHOD GetComputedTextLength(float *_retval);
  NS_IMETHOD GetSubStringLength(PRUint32 charnum, PRUint32 nchars, float *_retval);
  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval);
  NS_IMETHOD GetCharNumAtPosition(nsIDOMSVGPoint *point, PRInt32 *_retval);

  
  void NotifyGlyphMetricsChange();

private:
  




  void UpdateGlyphPositioning(PRBool aForceGlobalTransform);

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;

  enum UpdateState { unsuspended, suspended };
  UpdateState mMetricsState;

  PRPackedBool mPropagateTransform;
  PRPackedBool mPositioningDirty;
};

#endif
