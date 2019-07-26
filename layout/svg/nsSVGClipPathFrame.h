




#ifndef __NS_SVGCLIPPATHFRAME_H__
#define __NS_SVGCLIPPATHFRAME_H__

#include "gfxMatrix.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class nsRenderingContext;
class nsISVGChildFrame;

typedef nsSVGContainerFrame nsSVGClipPathFrameBase;

class nsSVGClipPathFrame : public nsSVGClipPathFrameBase
{
  friend nsIFrame*
  NS_NewSVGClipPathFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGClipPathFrame(nsStyleContext* aContext)
    : nsSVGClipPathFrameBase(aContext)
    , mInUse(false)
  {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) {
    return NS_OK;
  }

  
  nsresult ClipPaint(nsRenderingContext* aContext,
                     nsIFrame* aParent,
                     const gfxMatrix &aMatrix);

  bool ClipHitTest(nsIFrame* aParent,
                     const gfxMatrix &aMatrix,
                     const nsPoint &aPoint);

  
  
  
  bool IsTrivial(nsISVGChildFrame **aSingleChild = nullptr);

  bool IsValid();

  
  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

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

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor);
};

#endif
