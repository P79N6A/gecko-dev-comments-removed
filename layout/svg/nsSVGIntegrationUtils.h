




#ifndef NSSVGINTEGRATIONUTILS_H_
#define NSSVGINTEGRATIONUTILS_H_

#include "gfxMatrix.h"
#include "GraphicsFilter.h"
#include "gfxRect.h"

class nsDisplayList;
class nsDisplayListBuilder;
class nsIFrame;
class nsRenderingContext;

struct nsRect;
struct nsIntRect;

namespace mozilla {
namespace layers {
class LayerManager;
}
}

struct nsPoint;
struct nsSize;





class nsSVGIntegrationUtils MOZ_FINAL
{
public:
  


  static bool
  UsingEffectsForFrame(const nsIFrame* aFrame);

  























  static nsPoint
  GetOffsetToUserSpace(nsIFrame* aFrame);

  



  static nsSize
  GetContinuationUnionSize(nsIFrame* aNonSVGFrame);

  







  static gfxSize
  GetSVGCoordContextForNonSVGFrame(nsIFrame* aNonSVGFrame);

  








  static gfxRect
  GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame);

  












  static nsRect
  ComputePostEffectsVisualOverflowRect(nsIFrame* aFrame,
                                       const nsRect& aPreEffectsOverflowRect);

  











  static nsIntRect
  AdjustInvalidAreaForSVGEffects(nsIFrame* aFrame, const nsPoint& aToReferenceFrame,
                                 const nsIntRect& aInvalidRect);

  



  static nsRect
  GetRequiredSourceForInvalidArea(nsIFrame* aFrame, const nsRect& aDamageRect);

  



  static bool
  HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt);

  


  static void
  PaintFramesWithEffects(nsRenderingContext* aCtx,
                         nsIFrame* aFrame, const nsRect& aDirtyRect,
                         nsDisplayListBuilder* aBuilder,
                         mozilla::layers::LayerManager* aManager);

  





  static gfxMatrix
  GetCSSPxToDevPxMatrix(nsIFrame* aNonSVGFrame);

  


















  enum {
    FLAG_SYNC_DECODE_IMAGES = 0x01,
  };
  static void
  DrawPaintServer(nsRenderingContext* aRenderingContext,
                  nsIFrame*            aTarget,
                  nsIFrame*            aPaintServer,
                  GraphicsFilter aFilter,
                  const nsRect&        aDest,
                  const nsRect&        aFill,
                  const nsPoint&       aAnchor,
                  const nsRect&        aDirty,
                  const nsSize&        aPaintServerSize,
                  uint32_t             aFlags);
};

#endif 
