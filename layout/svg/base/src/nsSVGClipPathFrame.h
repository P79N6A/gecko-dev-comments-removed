



































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
    mInUse(false) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  nsresult ClipPaint(nsSVGRenderState* aContext,
                     nsIFrame* aParent,
                     const gfxMatrix &aMatrix);

  bool ClipHitTest(nsIFrame* aParent,
                     const gfxMatrix &aMatrix,
                     const nsPoint &aPoint);

  
  
  
  bool IsTrivial();

  bool IsValid();

  
  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  




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
      NS_ASSERTION(!mFrame->mInUse, "reference loop!");
      mFrame->mInUse = true;
    }
    ~AutoClipPathReferencer() {
      mFrame->mInUse = false;
    }
  private:
    nsSVGClipPathFrame *mFrame;
  };

  nsIFrame *mClipParent;
  nsAutoPtr<gfxMatrix> mClipParentMatrix;
  
  bool mInUse;

  
  virtual gfxMatrix GetCanvasTM();
};

#endif
