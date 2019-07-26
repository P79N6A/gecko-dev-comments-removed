




#ifndef __NS_SVGMASKFRAME_H__
#define __NS_SVGMASKFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxPattern.h"
#include "gfxMatrix.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGUtils.h"

class gfxContext;
class nsRenderingContext;

typedef nsSVGContainerFrame nsSVGMaskFrameBase;

class nsSVGMaskFrame MOZ_FINAL : public nsSVGMaskFrameBase
{
  friend nsIFrame*
  NS_NewSVGMaskFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  nsSVGMaskFrame(nsStyleContext* aContext)
    : nsSVGMaskFrameBase(aContext)
    , mInUse(false)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  already_AddRefed<gfxPattern> GetMaskForMaskedFrame(gfxContext* aContext,
                                                     nsIFrame* aMaskedFrame,
                                                     const gfxMatrix &aMatrix,
                                                     float aOpacity);

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE {}

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGMask"), aResult);
  }
#endif

private:
  
  
  
  
  class MOZ_STACK_CLASS AutoMaskReferencer
  {
  public:
    AutoMaskReferencer(nsSVGMaskFrame *aFrame
                       MOZ_GUARD_OBJECT_NOTIFIER_PARAM)
       : mFrame(aFrame) {
      MOZ_GUARD_OBJECT_NOTIFIER_INIT;
      NS_ASSERTION(!mFrame->mInUse, "reference loop!");
      mFrame->mInUse = true;
    }
    ~AutoMaskReferencer() {
      mFrame->mInUse = false;
    }
  private:
    nsSVGMaskFrame *mFrame;
    MOZ_DECL_USE_GUARD_OBJECT_NOTIFIER
  };

  nsIFrame *mMaskParent;
  nsAutoPtr<gfxMatrix> mMaskParentMatrix;
  
  bool mInUse;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;
};

#endif
