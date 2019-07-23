





































#ifndef NSSVGFOREIGNOBJECTFRAME_H__
#define NSSVGFOREIGNOBJECTFRAME_H__

#include "nsContainerFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGMatrix.h"
#include "nsRegion.h"
#include "nsIPresShell.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

class nsSVGOuterSVGFrame;

typedef nsContainerFrame nsSVGForeignObjectFrameBase;

class nsSVGForeignObjectFrame : public nsSVGForeignObjectFrameBase,
                                public nsISVGChildFrame
{
  friend nsIFrame*
  NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGForeignObjectFrame(nsStyleContext* aContext);

public:
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD  Init(nsIContent* aContent,
                   nsIFrame*   aParent,
                   nsIFrame*   aPrevInFlow);
  virtual void Destroy();
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  virtual nsIFrame* GetContentInsertionFrame() {
    return GetFirstChild(nsnull)->GetContentInsertionFrame();
  }

  NS_IMETHOD Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  


  virtual PRBool IsTransformed() const
  {
    return PR_TRUE;
  }

  


  virtual gfxMatrix GetTransformMatrix(nsIFrame **aOutAncestor);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGForeignObjectFrameBase::IsFrameOfType(aFlags &
      ~(nsIFrame::eSVG | nsIFrame::eSVGForeignObject));
  }

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGForeignObject"), aResult);
  }
#endif

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext,
                      const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate);
  virtual PRBool GetMatrixPropagation();
  virtual gfxRect GetBBoxContribution(const gfxMatrix &aToBBoxUserspace);
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_TRUE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_TRUE; }

  gfxMatrix GetCanvasTM();

  
  void MaybeReflowFromOuterSVGFrame();

protected:
  
  void DoReflow();
  void RequestReflow(nsIPresShell::IntrinsicDirty aType);
  void UpdateGraphic();

  
  
  gfxMatrix GetCanvasTMForChildren();
  void InvalidateDirtyRect(nsSVGOuterSVGFrame* aOuter,
                           const nsRect& aRect, PRUint32 aFlags);
  void FlushDirtyRegion();

  
  PRBool IsDisabled() const { return mRect.width <= 0 || mRect.height <= 0; }

  nsCOMPtr<nsIDOMSVGMatrix> mCanvasTM;

  
  nsRegion mSameDocDirtyRegion;

  
  nsRegion mSubDocDirtyRegion;

  PRPackedBool mInReflow;
};

#endif
