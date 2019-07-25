




































#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "nsFrame.h"
#include "gfxMatrix.h"

class nsSVGPaintServerFrame;
class gfxContext;

typedef nsFrame nsSVGGeometryFrameBase;

#define SVG_HIT_TEST_FILL        0x01
#define SVG_HIT_TEST_STROKE      0x02
#define SVG_HIT_TEST_CHECK_MRECT 0x04







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase
{
protected:
  NS_DECL_FRAMEARENA_HELPERS

  nsSVGGeometryFrame(nsStyleContext *aContext) : nsSVGGeometryFrameBase(aContext) {}

public:
  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGGeometry));
  }

  
  virtual gfxMatrix GetCanvasTM() = 0;
  PRUint16 GetClipRule();

  float GetStrokeWidth();

  



  bool SetupCairoFill(gfxContext *aContext);
  


  bool HasStroke();
  


  void SetupCairoStrokeGeometry(gfxContext *aContext);
  


  void SetupCairoStrokeHitGeometry(gfxContext *aContext);
  



  bool SetupCairoStroke(gfxContext *aContext);

protected:
  nsSVGPaintServerFrame *GetPaintServer(const nsStyleSVGPaint *aPaint,
                                        const FramePropertyDescriptor *aProperty);

  





  virtual PRUint16 GetHitTestFlags();

private:
  nsresult GetStrokeDashArray(double **arr, PRUint32 *count);
  float GetStrokeDashoffset();

  






  float MaybeOptimizeOpacity(float aFillOrStrokeOpacity);
};

#endif 
