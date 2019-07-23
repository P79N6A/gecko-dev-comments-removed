



































#ifndef __NS_SVGGEOMETRYFRAME_H__
#define __NS_SVGGEOMETRYFRAME_H__

#include "nsFrame.h"
#include "nsWeakReference.h"
#include "nsISVGValueObserver.h"

class nsSVGPaintServerFrame;
class gfxContext;

typedef nsFrame nsSVGGeometryFrameBase;







class nsSVGGeometryFrame : public nsSVGGeometryFrameBase,
                           public nsISVGValueObserver
{
protected:
  nsSVGGeometryFrame(nsStyleContext *aContext) : nsSVGGeometryFrameBase(aContext) {}

public:
  
  virtual void Destroy();
  NS_IMETHOD Init(nsIContent* aContent,
                  nsIFrame* aParent,
                  nsIFrame* aPrevInFlow);
  NS_IMETHOD DidSetStyleContext();

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSVGGeometryFrameBase::IsFrameOfType(aFlags & ~(nsIFrame::eSVG));
  }

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  NS_IMETHOD WillModifySVGObservable(nsISVGValue* observable,
                                     nsISVGValue::modificationType aModType);
  NS_IMETHOD DidModifySVGObservable(nsISVGValue* observable,
                                    nsISVGValue::modificationType aModType);

  
  NS_IMETHOD GetCanvasTM(nsIDOMSVGMatrix * *aCanvasTM) = 0;
  PRUint16 GetClipRule();
  PRBool IsClipChild(); 

  float GetStrokeWidth();

  
  
  
  
  PRBool HasFill();
  PRBool HasStroke();

  



  PRBool SetupCairoFill(gfxContext *aContext);

  
  void SetupCairoStrokeGeometry(gfxContext *aContext);

  
  void SetupCairoStrokeHitGeometry(gfxContext *aContext);

  



  PRBool SetupCairoStroke(gfxContext *aContext);

protected:
  nsSVGPaintServerFrame *GetPaintServer(const nsStyleSVGPaint *aPaint);

private:
  nsresult GetStrokeDashArray(double **arr, PRUint32 *count);
  float GetStrokeDashoffset();
  void RemovePaintServerProperties();

  
  
  
  
  
  
  float MaybeOptimizeOpacity(float aOpacity);
};

#endif 
