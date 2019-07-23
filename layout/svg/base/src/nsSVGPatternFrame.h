





































#ifndef __NS_SVGPATTERNFRAME_H__
#define __NS_SVGPATTERNFRAME_H__

#include "nsIDOMSVGMatrix.h"
#include "nsSVGPaintServerFrame.h"
#include "gfxMatrix.h"

class nsSVGPreserveAspectRatio;
class nsIFrame;
class nsSVGLength2;
class nsSVGElement;
class gfxContext;
class gfxASurface;

typedef nsSVGPaintServerFrame  nsSVGPatternFrameBase;





class nsSVGPatternFrame : public nsSVGPatternFrameBase
{
public:
  friend nsIFrame* NS_NewSVGPatternFrame(nsIPresShell* aPresShell,
                                         nsIContent*   aContent,
                                         nsStyleContext* aContext);

  nsSVGPatternFrame(nsStyleContext* aContext);

  nsresult PaintPattern(gfxASurface **surface,
                        gfxMatrix *patternMatrix,
                        nsSVGGeometryFrame *aSource,
                        float aGraphicOpacity);

  
  virtual PRBool SetupPaintServer(gfxContext *aContext,
                                  nsSVGGeometryFrame *aSource,
                                  float aGraphicOpacity);

public:
  
  virtual already_AddRefed<nsIDOMSVGMatrix> GetCanvasTM();

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPattern"), aResult);
  }
#endif 

protected:
  
  nsSVGPatternFrame* GetReferencedPattern();
  
  
  
  nsSVGPatternElement* GetPatternWithAttr(nsIAtom *aAttrName, nsIContent *aDefault);

  
  nsSVGLength2 *GetX();
  nsSVGLength2 *GetY();
  nsSVGLength2 *GetWidth();
  nsSVGLength2 *GetHeight();

  PRUint16 GetPatternUnits();
  PRUint16 GetPatternContentUnits();
  gfxMatrix GetPatternTransform();

  const nsSVGPreserveAspectRatio &GetPreserveAspectRatio();

  NS_IMETHOD GetPatternFirstChild(nsIFrame **kid);
  NS_IMETHOD GetViewBox(nsIDOMSVGRect * *aMatrix);
  nsresult   GetPatternRect(nsIDOMSVGRect **patternRect,
                            nsIDOMSVGRect *bbox,
                            nsIDOMSVGMatrix *callerCTM,
                            nsSVGElement *content);
  gfxMatrix  GetPatternMatrix(nsIDOMSVGRect *bbox,
                              nsIDOMSVGRect *callerBBox,
                              nsIDOMSVGMatrix *callerCTM);
  nsresult   ConstructCTM(nsIDOMSVGMatrix **ctm,
                          nsIDOMSVGRect *callerBBox,
                          nsIDOMSVGMatrix *callerCTM);
  nsresult   GetCallerGeometry(nsIDOMSVGMatrix **aCTM,
                               nsIDOMSVGRect **aBBox,
                               nsSVGElement **aContent,
                               nsSVGGeometryFrame *aSource);

private:
  
  
  
  nsSVGGeometryFrame               *mSource;
  nsCOMPtr<nsIDOMSVGMatrix>         mCTM;

protected:
  
  PRPackedBool                      mLoopFlag;
  
  
  PRPackedBool                      mPaintLoopFlag;
  PRPackedBool                      mNoHRefURI;
};

#endif
