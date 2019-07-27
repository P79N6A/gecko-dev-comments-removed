




#ifndef NSSVGINTEGRATIONUTILS_H_
#define NSSVGINTEGRATIONUTILS_H_

#include "gfxMatrix.h"
#include "GraphicsFilter.h"
#include "gfxRect.h"
#include "nsAutoPtr.h"

class gfxContext;
class gfxDrawable;
class nsDisplayList;
class nsDisplayListBuilder;
class nsIFrame;
class nsIntRegion;

struct nsRect;

namespace mozilla {
namespace gfx {
class DrawTarget;
}
namespace layers {
class LayerManager;
}
}

struct nsPoint;
struct nsSize;





class nsSVGIntegrationUtils final
{
  typedef mozilla::gfx::DrawTarget DrawTarget;

public:
  


  static bool
  UsingEffectsForFrame(const nsIFrame* aFrame);

  



  static nsSize
  GetContinuationUnionSize(nsIFrame* aNonSVGFrame);

  







  static mozilla::gfx::Size
  GetSVGCoordContextForNonSVGFrame(nsIFrame* aNonSVGFrame);

  








  static gfxRect
  GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame);

  












  static nsRect
  ComputePostEffectsVisualOverflowRect(nsIFrame* aFrame,
                                       const nsRect& aPreEffectsOverflowRect);

  











  static nsIntRegion
  AdjustInvalidAreaForSVGEffects(nsIFrame* aFrame, const nsPoint& aToReferenceFrame,
                                 const nsIntRegion& aInvalidRegion);

  



  static nsRect
  GetRequiredSourceForInvalidArea(nsIFrame* aFrame, const nsRect& aDamageRect);

  



  static bool
  HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt);

  


  static void
  PaintFramesWithEffects(gfxContext& aCtx,
                         nsIFrame* aFrame, const nsRect& aDirtyRect,
                         nsDisplayListBuilder* aBuilder,
                         mozilla::layers::LayerManager* aManager);

  





  static gfxMatrix
  GetCSSPxToDevPxMatrix(nsIFrame* aNonSVGFrame);

  


















  enum {
    FLAG_SYNC_DECODE_IMAGES = 0x01,
  };

  static already_AddRefed<gfxDrawable>
  DrawableFromPaintServer(nsIFrame*         aFrame,
                          nsIFrame*         aTarget,
                          const nsSize&     aPaintServerSize,
                          const gfxIntSize& aRenderSize,
                          const DrawTarget* aDrawTarget,
                          const gfxMatrix&  aContextMatrix,
                          uint32_t          aFlags);
};

#endif 
