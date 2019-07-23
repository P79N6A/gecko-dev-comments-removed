





































#ifndef NS_SVGTEXTFRAME_H
#define NS_SVGTEXTFRAME_H

#include "nsSVGTextContainerFrame.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

typedef nsSVGTextContainerFrame nsSVGTextFrameBase;

class nsSVGTextFrame : public nsSVGTextFrameBase
{
  friend nsIFrame*
  NS_NewSVGTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGTextFrame(nsStyleContext* aContext)
    : nsSVGTextFrameBase(aContext),
      mMetricsState(unsuspended),
      mPositioningDirty(PR_TRUE) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

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

  
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  
  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint & aPoint);
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace);
  
  
  virtual gfxMatrix GetCanvasTM();
  
  
  virtual PRUint32 GetNumberOfChars();
  virtual float GetComputedTextLength();
  virtual float GetSubStringLength(PRUint32 charnum, PRUint32 nchars);
  virtual PRInt32 GetCharNumAtPosition(nsIDOMSVGPoint *point);

  NS_IMETHOD GetStartPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetEndPositionOfChar(PRUint32 charnum, nsIDOMSVGPoint **_retval);
  NS_IMETHOD GetExtentOfChar(PRUint32 charnum, nsIDOMSVGRect **_retval);
  NS_IMETHOD GetRotationOfChar(PRUint32 charnum, float *_retval);

  
  void NotifyGlyphMetricsChange();

private:
  




  void UpdateGlyphPositioning(PRBool aForceGlobalTransform);

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;

  enum UpdateState { unsuspended, suspended };
  UpdateState mMetricsState;

  PRPackedBool mPositioningDirty;
};

#endif
