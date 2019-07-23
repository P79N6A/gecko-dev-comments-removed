



































#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "nsFrame.h"

class nsSVGPaintServerFrame;
class gfxContext;

typedef nsFrame nsSVGGeometryFrameBase;







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase
{
protected:
  nsSVGGeometryFrame(nsStyleContext *aContext) : nsSVGGeometryFrameBase(aContext) {}

public:
  
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

  
  NS_IMETHOD GetCanvasTM(nsIDOMSVGMatrix * *aCanvasTM) = 0;
  PRUint16 GetClipRule();
  PRBool IsClipChild(); 

  float GetStrokeWidth();

  



  PRBool SetupCairoFill(gfxContext *aContext);
  



  PRBool SetupCairoStrokeGeometry(gfxContext *aContext);
  



  PRBool SetupCairoStrokeHitGeometry(gfxContext *aContext);
  



  PRBool SetupCairoStroke(gfxContext *aContext);

protected:
  nsSVGPaintServerFrame *GetPaintServer(const nsStyleSVGPaint *aPaint,
                                        nsIAtom *aType);

private:
  nsresult GetStrokeDashArray(double **arr, PRUint32 *count);
  float GetStrokeDashoffset();

  
  
  
  
  
  
  float MaybeOptimizeOpacity(float aOpacity);
};

#endif 
