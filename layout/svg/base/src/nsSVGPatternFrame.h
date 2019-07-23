





































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
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewSVGPatternFrame(nsIPresShell* aPresShell,
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
  
  virtual gfxMatrix GetCanvasTM();

  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

#ifdef DEBUG
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
#endif

  




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

  
  const nsSVGLength2 *GetX();
  const nsSVGLength2 *GetY();
  const nsSVGLength2 *GetWidth();
  const nsSVGLength2 *GetHeight();

  PRUint16 GetPatternUnits();
  PRUint16 GetPatternContentUnits();
  gfxMatrix GetPatternTransform();

  const nsSVGViewBox &GetViewBox();
  const nsSVGPreserveAspectRatio &GetPreserveAspectRatio();

  NS_IMETHOD GetPatternFirstChild(nsIFrame **kid);
  gfxRect    GetPatternRect(const gfxRect &bbox,
                            const gfxMatrix &callerCTM,
                            nsSVGElement *content);
  gfxMatrix  GetPatternMatrix(const gfxRect &bbox,
                              const gfxRect &callerBBox,
                              const gfxMatrix &callerCTM);
  gfxMatrix  ConstructCTM(const gfxRect &callerBBox,
                          const gfxMatrix &callerCTM,
                          nsSVGElement *aTargetContent);
  nsresult   GetTargetGeometry(gfxMatrix *aCTM,
                               gfxRect *aBBox,
                               nsSVGElement **aTargetContent,
                               nsSVGGeometryFrame *aTarget);

private:
  
  
  
  nsSVGGeometryFrame               *mSource;
  nsCOMPtr<nsIDOMSVGMatrix>         mCTM;

protected:
  
  PRPackedBool                      mLoopFlag;
  
  
  PRPackedBool                      mPaintLoopFlag;
  PRPackedBool                      mNoHRefURI;
};

#endif
