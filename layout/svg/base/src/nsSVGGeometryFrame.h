





#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "gfxMatrix.h"
#include "gfxTypes.h"
#include "nsFrame.h"
#include "nsIFrame.h"
#include "nsQueryFrame.h"
#include "nsRect.h"

class gfxContext;
class nsIContent;
class nsStyleContext;
class nsSVGPaintServerFrame;

struct nsStyleSVGPaint;

typedef nsFrame nsSVGGeometryFrameBase;

#define SVG_HIT_TEST_FILL        0x01
#define SVG_HIT_TEST_STROKE      0x02
#define SVG_HIT_TEST_CHECK_MRECT 0x04







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase
{
protected:
  NS_DECL_FRAMEARENA_HELPERS

  nsSVGGeometryFrame(nsStyleContext *aContext)
    : nsSVGGeometryFrameBase(aContext)
  {
    AddStateBits(NS_FRAME_SVG_LAYOUT);
  }

public:
  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGGeometry));
  }

  
  virtual gfxMatrix GetCanvasTM(PRUint32 aFor) = 0;
  PRUint16 GetClipRule();

  float GetStrokeWidth();

  



  bool SetupCairoFill(gfxContext *aContext);
  


  bool HasStroke();
  


  void SetupCairoStrokeGeometry(gfxContext *aContext);
  


  void SetupCairoStrokeHitGeometry(gfxContext *aContext);
  



  bool SetupCairoStroke(gfxContext *aContext);

protected:
  





  virtual PRUint16 GetHitTestFlags();

  






  float MaybeOptimizeOpacity(float aFillOrStrokeOpacity);

private:
  bool GetStrokeDashData(FallibleTArray<gfxFloat>& dashes, gfxFloat *dashOffset);
};

#endif 
