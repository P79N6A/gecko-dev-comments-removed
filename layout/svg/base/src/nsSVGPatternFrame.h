





































#ifndef __NS_SVGPATTERNFRAME_H__
#define __NS_SVGPATTERNFRAME_H__

#include "nsSVGPaintServerFrame.h"
#include "gfxMatrix.h"
#include "nsIDOMSVGAnimTransformList.h"

class nsIFrame;
class nsSVGLength2;
class nsSVGElement;
class gfxContext;
class gfxASurface;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class SVGAnimatedTransformList;
} 

typedef nsSVGPaintServerFrame  nsSVGPatternFrameBase;





class nsSVGPatternFrame : public nsSVGPatternFrameBase
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewSVGPatternFrame(nsIPresShell* aPresShell,
                                         nsStyleContext* aContext);

  nsSVGPatternFrame(nsStyleContext* aContext);

  
  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          float aOpacity,
                          const gfxRect *aOverrideBounds);

public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  
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
  
  class AutoPatternReferencer;
  nsSVGPatternFrame* GetReferencedPattern();
  nsSVGPatternFrame* GetReferencedPatternIfNotInUse();

  
  PRUint16 GetEnumValue(PRUint32 aIndex, nsIContent *aDefault);
  PRUint16 GetEnumValue(PRUint32 aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  mozilla::SVGAnimatedTransformList* GetPatternTransformList(
      nsIContent* aDefault);
  gfxMatrix GetPatternTransform();
  const nsSVGViewBox &GetViewBox(nsIContent *aDefault);
  const nsSVGViewBox &GetViewBox() { return GetViewBox(mContent); }
  const SVGAnimatedPreserveAspectRatio &GetPreserveAspectRatio(
      nsIContent *aDefault);
  const SVGAnimatedPreserveAspectRatio &GetPreserveAspectRatio()
  {
    return GetPreserveAspectRatio(mContent);
  }
  const nsSVGLength2 *GetLengthValue(PRUint32 aIndex, nsIContent *aDefault);
  const nsSVGLength2 *GetLengthValue(PRUint32 aIndex)
  {
    return GetLengthValue(aIndex, mContent);
  }

  nsresult PaintPattern(gfxASurface **surface,
                        gfxMatrix *patternMatrix,
                        nsIFrame *aSource,
                        float aGraphicOpacity,
                        const gfxRect *aOverrideBounds);
  nsIFrame*  GetPatternFirstChild();
  gfxRect    GetPatternRect(const gfxRect &bbox,
                            const gfxMatrix &callerCTM,
                            nsIFrame *aTarget);
  gfxMatrix  GetPatternMatrix(const gfxMatrix &patternTransform,
                              const gfxRect &bbox,
                              const gfxRect &callerBBox,
                              const gfxMatrix &callerCTM);
  gfxMatrix  ConstructCTM(const gfxRect &callerBBox,
                          const gfxMatrix &callerCTM,
                          nsIFrame *aTarget);
  nsresult   GetTargetGeometry(gfxMatrix *aCTM,
                               gfxRect *aBBox,
                               nsIFrame *aTarget,
                               const gfxRect *aOverrideBounds);

private:
  
  
  
  nsSVGGeometryFrame               *mSource;
  nsAutoPtr<gfxMatrix>              mCTM;

protected:
  
  bool                              mLoopFlag;
  bool                              mNoHRefURI;
};

#endif
