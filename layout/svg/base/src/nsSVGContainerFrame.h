



































#ifndef NS_SVGCONTAINERFRAME_H
#define NS_SVGCONTAINERFRAME_H

#include "nsContainerFrame.h"
#include "nsISVGChildFrame.h"
#include "nsSVGSVGElement.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

typedef nsContainerFrame nsSVGContainerFrameBase;

class nsSVGContainerFrame : public nsSVGContainerFrameBase
{
  friend nsIFrame* NS_NewSVGContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);
protected:
  nsSVGContainerFrame(nsStyleContext* aContext) :
    nsSVGContainerFrameBase(aContext) {}

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGContainerFrame)
  
  NS_DECL_QUERYFRAME

  
  virtual gfxMatrix GetCanvasTM() { return gfxMatrix(); }

  
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGContainerFrameBase::IsFrameOfType(
            aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGContainer));
  }
};

class nsSVGDisplayContainerFrame : public nsSVGContainerFrame,
                                   public nsISVGChildFrame
{
protected:
  nsSVGDisplayContainerFrame(nsStyleContext* aContext) :
    nsSVGContainerFrame(aContext) {}

public:
  NS_DECL_QUERYFRAME_TARGET(nsSVGDisplayContainerFrame)

  NS_DECL_QUERYFRAME

  
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  
  NS_IMETHOD PaintSVG(nsSVGRenderState* aContext,
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
  NS_IMETHOD_(PRBool) HasValidCoveredRect() { return PR_FALSE; }
};

#endif
