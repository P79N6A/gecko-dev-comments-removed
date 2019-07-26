




#ifndef mozilla_dom_SVGPathElement_h
#define mozilla_dom_SVGPathElement_h

#include "nsSVGNumber2.h"
#include "nsSVGPathGeometryElement.h"
#include "SVGAnimatedPathSegList.h"
#include "DOMSVGPathSeg.h"

nsresult NS_NewSVGPathElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

class gfxContext;

typedef nsSVGPathGeometryElement SVGPathElementBase;

namespace mozilla {

class nsISVGPoint;

namespace dom {

class SVGPathElement MOZ_FINAL : public SVGPathElementBase
{
friend class nsSVGPathFrame;

protected:
  friend nsresult (::NS_NewSVGPathElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  virtual JSObject* WrapNode(JSContext *cx,
                             JS::Handle<JSObject*> scope) MOZ_OVERRIDE;
  SVGPathElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const MOZ_OVERRIDE;

  
  virtual bool HasValidDimensions() const MOZ_OVERRIDE;

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName) MOZ_OVERRIDE;
  virtual bool IsMarkable() MOZ_OVERRIDE;
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks) MOZ_OVERRIDE;
  virtual void ConstructPath(gfxContext *aCtx) MOZ_OVERRIDE;

  virtual already_AddRefed<gfxPath> GetPath(const gfxMatrix &aMatrix) MOZ_OVERRIDE;

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  virtual SVGAnimatedPathSegList* GetAnimPathSegList() MOZ_OVERRIDE {
    return &mD;
  }

  virtual nsIAtom* GetPathDataAttrName() const MOZ_OVERRIDE {
    return nsGkAtoms::d;
  }

  enum PathLengthScaleForType {
    eForTextPath,
    eForStroking
  };

  




  gfxFloat GetPathLengthScale(PathLengthScaleForType aFor);

  
  already_AddRefed<SVGAnimatedNumber> PathLength();
  float GetTotalLength(ErrorResult& rv);
  already_AddRefed<nsISVGPoint> GetPointAtLength(float distance, ErrorResult& rv);
  uint32_t GetPathSegAtLength(float distance);
  already_AddRefed<DOMSVGPathSegClosePath> CreateSVGPathSegClosePath();
  already_AddRefed<DOMSVGPathSegMovetoAbs> CreateSVGPathSegMovetoAbs(float x, float y);
  already_AddRefed<DOMSVGPathSegMovetoRel> CreateSVGPathSegMovetoRel(float x, float y);
  already_AddRefed<DOMSVGPathSegLinetoAbs> CreateSVGPathSegLinetoAbs(float x, float y);
  already_AddRefed<DOMSVGPathSegLinetoRel> CreateSVGPathSegLinetoRel(float x, float y);
  already_AddRefed<DOMSVGPathSegCurvetoCubicAbs>
    CreateSVGPathSegCurvetoCubicAbs(float x, float y, float x1, float y1, float x2, float y2);
  already_AddRefed<DOMSVGPathSegCurvetoCubicRel>
    CreateSVGPathSegCurvetoCubicRel(float x, float y, float x1, float y1, float x2, float y2);
  already_AddRefed<DOMSVGPathSegCurvetoQuadraticAbs>
    CreateSVGPathSegCurvetoQuadraticAbs(float x, float y, float x1, float y1);
  already_AddRefed<DOMSVGPathSegCurvetoQuadraticRel>
    CreateSVGPathSegCurvetoQuadraticRel(float x, float y, float x1, float y1);
  already_AddRefed<DOMSVGPathSegArcAbs>
    CreateSVGPathSegArcAbs(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag);
  already_AddRefed<DOMSVGPathSegArcRel>
    CreateSVGPathSegArcRel(float x, float y, float r1, float r2, float angle, bool largeArcFlag, bool sweepFlag);
  already_AddRefed<DOMSVGPathSegLinetoHorizontalAbs> CreateSVGPathSegLinetoHorizontalAbs(float x);
  already_AddRefed<DOMSVGPathSegLinetoHorizontalRel> CreateSVGPathSegLinetoHorizontalRel(float x);
  already_AddRefed<DOMSVGPathSegLinetoVerticalAbs> CreateSVGPathSegLinetoVerticalAbs(float y);
  already_AddRefed<DOMSVGPathSegLinetoVerticalRel> CreateSVGPathSegLinetoVerticalRel(float y);
  already_AddRefed<DOMSVGPathSegCurvetoCubicSmoothAbs>
    CreateSVGPathSegCurvetoCubicSmoothAbs(float x, float y, float x2, float y2);
  already_AddRefed<DOMSVGPathSegCurvetoCubicSmoothRel>
    CreateSVGPathSegCurvetoCubicSmoothRel(float x, float y, float x2, float y2);
  already_AddRefed<DOMSVGPathSegCurvetoQuadraticSmoothAbs>
    CreateSVGPathSegCurvetoQuadraticSmoothAbs(float x, float y);
  already_AddRefed<DOMSVGPathSegCurvetoQuadraticSmoothRel>
    CreateSVGPathSegCurvetoQuadraticSmoothRel(float x, float y);
  already_AddRefed<DOMSVGPathSegList> PathSegList();
  already_AddRefed<DOMSVGPathSegList> AnimatedPathSegList();

protected:

  
  virtual NumberAttributesInfo GetNumberInfo() MOZ_OVERRIDE;

  SVGAnimatedPathSegList mD;
  nsSVGNumber2 mPathLength;
  static NumberInfo sNumberInfo;
};

} 
} 

#endif 
