



































#ifndef NS_SVGCONTAINERFRAME_H
#define NS_SVGCONTAINERFRAME_H

#include "nsContainerFrame.h"
#include "nsISVGChildFrame.h"
#include "nsIDOMSVGMatrix.h"
#include "nsSVGSVGElement.h"

typedef nsContainerFrame nsSVGContainerFrameBase;

class nsSVGContainerFrame : public nsSVGContainerFrameBase
{
  friend nsIFrame* NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                                           nsIContent* aContent,
                                           nsStyleContext* aContext);
protected:
  nsSVGContainerFrame(nsStyleContext* aContext) :
    nsSVGContainerFrameBase(aContext) {}

public:
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM() { return nsnull; }

  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsIFrame*       aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGContainerFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }
};

class nsSVGDisplayContainerFrame : public nsSVGContainerFrame,
                                   public nsISVGChildFrame
{
protected:
  nsSVGDisplayContainerFrame(nsStyleContext* aContext) :
    nsSVGContainerFrame(aContext) {}

public:
  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

public:
  
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsIFrame*       aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);  
  NS_IMETHOD_(nsRect) GetCoveredRegion();
  NS_IMETHOD UpdateCoveredRegion();
  NS_IMETHOD InitialUpdate();
  virtual void NotifySVGChanged(PRUint32 aFlags);
  NS_IMETHOD NotifyRedrawSuspended();
  NS_IMETHOD NotifyRedrawUnsuspended();
  NS_IMETHOD SetMatrixPropagation(PRBool aPropagate) { return NS_ERROR_FAILURE; }
  NS_IMETHOD SetOverrideCTM(nsIDOMSVGMatrix *aCTM) { return NS_ERROR_FAILURE; }
  virtual already_AddRefed<nsIDOMSVGMatrix> GetOverrideCTM() { return nsnull; }
  NS_IMETHOD GetBBox(nsIDOMSVGRect **_retval);
  NS_IMETHOD_(PRBool) IsDisplayContainer() { return PR_TRUE; }
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_FALSE; }
};

#endif
