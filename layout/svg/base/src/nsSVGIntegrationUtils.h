




#ifndef NSSVGINTEGRATIONUTILS_H_
#define NSSVGINTEGRATIONUTILS_H_

#include "gfxMatrix.h"
#include "gfxPattern.h"
#include "gfxRect.h"
#include "nsRect.h"

class nsDisplayList;
class nsDisplayListBuilder;
class nsIFrame;
class nsRenderingContext;

struct nsPoint;
struct nsSize;





class nsSVGIntegrationUtils
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

  






  static nsRect
  ComputeFrameEffectsRect(nsIFrame* aFrame, const nsRect& aOverflowRect);
  


  static nsRect
  GetInvalidAreaForChangedSource(nsIFrame* aFrame, const nsRect& aInvalidRect);
  



  static nsRect
  GetRequiredSourceForInvalidArea(nsIFrame* aFrame, const nsRect& aDamageRect);
  



  static bool
  HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt);

  




  static void
  PaintFramesWithEffects(nsRenderingContext* aCtx,
                         nsIFrame* aEffectsFrame, const nsRect& aDirtyRect,
                         nsDisplayListBuilder* aBuilder,
                         nsDisplayList* aInnerList);

  static gfxMatrix
  GetInitialMatrix(nsIFrame* aNonSVGFrame);
  





  static gfxRect
  GetSVGRectForNonSVGFrame(nsIFrame* aNonSVGFrame);
  




  static gfxRect
  GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame);

  
















  static void
  DrawPaintServer(nsRenderingContext* aRenderingContext,
                  nsIFrame*            aTarget,
                  nsIFrame*            aPaintServer,
                  gfxPattern::GraphicsFilter aFilter,
                  const nsRect&        aDest,
                  const nsRect&        aFill,
                  const nsPoint&       aAnchor,
                  const nsRect&        aDirty,
                  const nsSize&        aPaintServerSize);
};

#endif 
