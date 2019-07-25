




































#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "nsFrame.h"
#include "gfxMatrix.h"

class nsSVGPaintServerFrame;
class gfxContext;

typedef nsFrame nsSVGGeometryFrameBase;







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase
{
protected:
  NS_DECL_FRAMEARENA_HELPERS

  nsSVGGeometryFrame(nsStyleContext *aContext) : nsSVGGeometryFrameBase(aContext) {}

public:
  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG | nsIFrame::eSVGGeometry));
  }

  
  virtual gfxMatrix GetCanvasTM() = 0;
  PRUint16 GetClipRule();

  float GetStrokeWidth();

  



  PRBool SetupCairoFill(gfxContext *aContext);
  


  PRBool HasStroke();
  


  void SetupCairoStrokeGeometry(gfxContext *aContext);
  


  void SetupCairoStrokeHitGeometry(gfxContext *aContext);
  



  PRBool SetupCairoStroke(gfxContext *aContext);

protected:
  nsSVGPaintServerFrame *GetPaintServer(const nsStyleSVGPaint *aPaint,
                                        const FramePropertyDescriptor *aProperty);

private:
  nsresult GetStrokeDashArray(double **arr, PRUint32 *count);
  float GetStrokeDashoffset();

  






  float MaybeOptimizeOpacity(float aFillOrStrokeOpacity);
};

#endif 
