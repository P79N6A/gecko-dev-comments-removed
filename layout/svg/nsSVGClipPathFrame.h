




#ifndef __NS_SVGCLIPPATHFRAME_H__
#define __NS_SVGCLIPPATHFRAME_H__

#include "mozilla/Attributes.h"
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
  explicit nsSVGClipPathFrame(nsStyleContext* aContext)
    : nsSVGClipPathFrameBase(aContext)
    , mInUse(false)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  

  










  nsresult ApplyClipOrPaintClipMask(nsRenderingContext* aContext,
                                    nsIFrame* aClippedFrame,
                                    const gfxMatrix &aMatrix);

  


  bool PointIsInsideClipPath(nsIFrame* aClippedFrame, const gfxPoint &aPoint);

  
  
  
  bool IsTrivial(nsISVGChildFrame **aSingleChild = nullptr);

  bool IsValid();

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGClipPath"), aResult);
  }
#endif

  SVGBBox 
  GetBBoxForClipPathFrame(const SVGBBox &aBBox, const gfxMatrix &aMatrix);

  





  gfxMatrix GetClipPathTransform(nsIFrame* aClippedFrame);

 private:
  
  
  
  
  class MOZ_STACK_CLASS AutoClipPathReferencer
  {
  public:
    explicit AutoClipPathReferencer(nsSVGClipPathFrame *aFrame
                                    MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
       : mFrame(aFrame) {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      NS_ASSERTION(!mFrame->mInUse, "reference loop!");
      mFrame->mInUse = true;
    }
    ~AutoClipPathReferencer() {
      mFrame->mInUse = false;
    }
  private:
    nsSVGClipPathFrame *mFrame;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  gfxMatrix mMatrixForChildren;
  
  bool mInUse;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
};

#endif
