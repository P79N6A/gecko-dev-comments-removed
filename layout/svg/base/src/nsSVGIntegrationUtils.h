




































#ifndef NSSVGINTEGRATIONUTILS_H_
#define NSSVGINTEGRATIONUTILS_H_

#include "nsPoint.h"
#include "nsRect.h"
#include "gfxRect.h"
#include "gfxMatrix.h"

class nsIFrame;
class nsDisplayListBuilder;
class nsDisplayList;
class nsIRenderingContext;



class nsSVGIntegrationUtils
{
public:
  


  static PRBool
  UsingEffectsForFrame(const nsIFrame* aFrame);

  






  static nsRect
  ComputeFrameEffectsRect(nsIFrame* aFrame, const nsRect& aOverflowRect);
  


  static nsRect
  GetInvalidAreaForChangedSource(nsIFrame* aFrame, const nsRect& aInvalidRect);
  



  static nsRect
  GetRequiredSourceForInvalidArea(nsIFrame* aFrame, const nsRect& aDamageRect);
  



  static PRBool
  HitTestFrameForEffects(nsIFrame* aFrame, const nsPoint& aPt);

  




  static void
  PaintFramesWithEffects(nsIRenderingContext* aCtx,
                         nsIFrame* aEffectsFrame, const nsRect& aDirtyRect,
                         nsDisplayListBuilder* aBuilder,
                         nsDisplayList* aInnerList);

  static gfxMatrix
  GetInitialMatrix(nsIFrame* aNonSVGFrame);
  





  static gfxRect
  GetSVGRectForNonSVGFrame(nsIFrame* aNonSVGFrame);
  




  static gfxRect
  GetSVGBBoxForNonSVGFrame(nsIFrame* aNonSVGFrame);
};

#endif 
