




#ifndef __NS_SVGPATTERNFRAME_H__
#define __NS_SVGPATTERNFRAME_H__

#include "mozilla/Attributes.h"
#include "gfxMatrix.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsSVGPaintServerFrame.h"

class gfxASurface;
class gfxContext;
class nsIFrame;
class nsSVGElement;
class nsSVGLength2;
class nsSVGPathGeometryFrame;
class nsSVGViewBox;

namespace mozilla {
class SVGAnimatedPreserveAspectRatio;
class nsSVGAnimatedTransformList;
} 

typedef nsSVGPaintServerFrame  nsSVGPatternFrameBase;





class nsSVGPatternFrame : public nsSVGPatternFrameBase
{
  typedef mozilla::gfx::SourceSurface SourceSurface;

public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewSVGPatternFrame(nsIPresShell* aPresShell,
                                         nsStyleContext* aContext);

  explicit nsSVGPatternFrame(nsStyleContext* aContext);

  
  virtual already_AddRefed<gfxPattern>
    GetPaintServerPattern(nsIFrame *aSource,
                          const gfxMatrix& aContextMatrix,
                          nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
                          float aOpacity,
                          const gfxRect *aOverrideBounds) MOZ_OVERRIDE;

public:
  typedef mozilla::SVGAnimatedPreserveAspectRatio SVGAnimatedPreserveAspectRatio;

  
  virtual gfxMatrix GetCanvasTM(uint32_t aFor,
                                nsIFrame* aTransformRoot = nullptr) MOZ_OVERRIDE;

  
  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
#endif

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGPattern"), aResult);
  }
#endif 

protected:
  
  class AutoPatternReferencer;
  nsSVGPatternFrame* GetReferencedPattern();
  nsSVGPatternFrame* GetReferencedPatternIfNotInUse();

  
  uint16_t GetEnumValue(uint32_t aIndex, nsIContent *aDefault);
  uint16_t GetEnumValue(uint32_t aIndex)
  {
    return GetEnumValue(aIndex, mContent);
  }
  mozilla::nsSVGAnimatedTransformList* GetPatternTransformList(
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
  const nsSVGLength2 *GetLengthValue(uint32_t aIndex, nsIContent *aDefault);
  const nsSVGLength2 *GetLengthValue(uint32_t aIndex)
  {
    return GetLengthValue(aIndex, mContent);
  }

  mozilla::TemporaryRef<SourceSurface>
  PaintPattern(Matrix *patternMatrix,
               const Matrix &aContextMatrix,
               nsIFrame *aSource,
               nsStyleSVGPaint nsStyleSVG::*aFillOrStroke,
               float aGraphicOpacity,
               const gfxRect *aOverrideBounds);
  nsIFrame*  GetPatternFirstChild();
  gfxRect    GetPatternRect(uint16_t aPatternUnits,
                            const gfxRect &bbox,
                            const Matrix &callerCTM,
                            nsIFrame *aTarget);
  gfxMatrix  ConstructCTM(const nsSVGViewBox& aViewBox,
                          uint16_t aPatternContentUnits,
                          uint16_t aPatternUnits,
                          const gfxRect &callerBBox,
                          const Matrix &callerCTM,
                          nsIFrame *aTarget);

private:
  
  
  
  nsSVGPathGeometryFrame           *mSource;
  nsAutoPtr<gfxMatrix>              mCTM;

protected:
  
  bool                              mLoopFlag;
  bool                              mNoHRefURI;
};

#endif
