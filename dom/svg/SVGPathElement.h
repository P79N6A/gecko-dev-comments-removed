





#ifndef mozilla_dom_SVGPathElement_h
#define mozilla_dom_SVGPathElement_h

#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsSVGNumber2.h"
#include "nsSVGPathGeometryElement.h"
#include "SVGAnimatedPathSegList.h"
#include "DOMSVGPathSeg.h"

nsresult NS_NewSVGPathElement(nsIContent **aResult,
                              already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo);

typedef nsSVGPathGeometryElement SVGPathElementBase;

namespace mozilla {

class nsISVGPoint;

namespace dom {

class SVGPathElement final : public SVGPathElementBase
{
friend class nsSVGPathFrame;

  typedef mozilla::gfx::Path Path;

protected:
  friend nsresult (::NS_NewSVGPathElement(nsIContent **aResult,
                                          already_AddRefed<mozilla::dom::NodeInfo>&& aNodeInfo));
  virtual JSObject* WrapNode(JSContext *cx, JS::Handle<JSObject*> aGivenProto) override;
  explicit SVGPathElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

public:
  
  NS_DECL_SIZEOF_EXCLUDING_THIS

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const override;

  
  virtual bool HasValidDimensions() const override;

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName) override;
  virtual bool IsMarkable() override;
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks) override;
  virtual already_AddRefed<Path> BuildPath(PathBuilder* aBuilder) override;

  




  virtual already_AddRefed<Path> GetOrBuildPathForMeasuring() override;

  
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  virtual SVGAnimatedPathSegList* GetAnimPathSegList() override {
    return &mD;
  }

  virtual nsIAtom* GetPathDataAttrName() const override {
    return nsGkAtoms::d;
  }

  enum PathLengthScaleForType {
    eForTextPath,
    eForStroking
  };

  




  float GetPathLengthScale(PathLengthScaleForType aFor);

  
  already_AddRefed<SVGAnimatedNumber> PathLength();
  float GetTotalLength();
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

  
  virtual NumberAttributesInfo GetNumberInfo() override;

  SVGAnimatedPathSegList mD;
  nsSVGNumber2 mPathLength;
  static NumberInfo sNumberInfo;
};

} 
} 

#endif 
