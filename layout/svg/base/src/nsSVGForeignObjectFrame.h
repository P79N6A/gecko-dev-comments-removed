





































#ifndef NSSVGFOREIGNOBJECTFRAME_H__
#define NSSVGFOREIGNOBJECTFRAME_H__

#include "nsBlockFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGLength.h"
#include "nsRegion.h"

typedef nsContainerFrame nsSVGForeignObjectFrameBase;

class nsSVGForeignObjectFrame : public nsSVGForeignObjectFrameBase,
                                public nsISVGChildFrame
{
  friend nsIFrame*
  NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);
protected:
  nsSVGForeignObjectFrame(nsStyleContext* aContext);
  
  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  virtual void Destroy();
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

  NS_IMETHOD DidSetStyleContext();
  virtual void MarkIntrinsicWidthsDirty();

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGForeignObjectFrameBase::IsFrameOfType(aFlags &
      ~(nsIFrame::eSVG | nsIFrame::eSVGForeignObject));
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRBool aImmediate);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGForeignObject"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);  
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  NS_IMETHOD NotifyCanvasTMChanged(PRBool suppressInvalidation);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM);
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_TRUE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_FALSE; }

  
  



  nsPoint TransformPointFromOuter(nsPoint aPt);

  already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();
  
protected:
  
  void DoReflow();
  void PostChildDirty();
  void UpdateGraphic();
  
  void GetBBoxInternal(float* aX, float *aY, float* aWidth, float *aHeight);
  already_AddRefed<nsIDOMSVGMatrix> GetTMIncludingOffset();
  nsresult TransformPointFromOuterPx(float aX, float aY, nsPoint* aOut);
  void FlushDirtyRegion();

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;
  nsCOMPtr<nsIDOMSVGMatrix> mOverrideCTM;
  nsRegion                  mDirtyRegion;

  PRPackedBool mPropagateTransform;
  PRPackedBool mInReflow;
};

#endif
