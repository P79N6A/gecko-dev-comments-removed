




#ifndef __NS_SVGMASKFRAME_H__
#define __NS_SVGMASKFRAME_H__

#include "mozilla/Attributes.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
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

  typedef mozilla::gfx::Matrix Matrix;
  typedef mozilla::gfx::SourceSurface SourceSurface;

protected:
  explicit nsSVGMaskFrame(nsStyleContext* aContext)
    : nsSVGMaskFrameBase(aContext)
    , mInUse(false)
  {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  mozilla::TemporaryRef<SourceSurface>
  GetMaskForMaskedFrame(gfxContext* aContext,
                        nsIFrame* aMaskedFrame,
                        const gfxMatrix &aMatrix,
                        float aOpacity,
                        Matrix* aMaskTransform);

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
  




  gfxMatrix GetMaskTransform(nsIFrame* aMaskedFrame);

  
  
  
  
  class MOZ_STACK_CLASS AutoMaskReferencer
  {
  public:
    explicit AutoMaskReferencer(nsSVGMaskFrame *aFrame
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

  gfxMatrix mMatrixForChildren;
  
  bool mInUse;

  
  virtual gfxMatrix GetCanvasTM() MOZ_OVERRIDE;
};

#endif
