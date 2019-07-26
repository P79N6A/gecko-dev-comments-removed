




#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "mozilla/gfx/2D.h"
#include "SVGGraphicsElement.h"

struct gfxMatrix;
template <class E> class nsTArray;

struct nsSVGMark {
  enum Type {
    eStart,
    eMid,
    eEnd,

    eTypeCount
  };

  float x, y, angle;
  Type type;
  nsSVGMark(float aX, float aY, float aAngle, Type aType) :
    x(aX), y(aY), angle(aAngle), type(aType) {}
};

class gfxContext;

typedef mozilla::dom::SVGGraphicsElement nsSVGPathGeometryElementBase;

class nsSVGPathGeometryElement : public nsSVGPathGeometryElementBase
{
protected:
  typedef mozilla::gfx::FillRule FillRule;
  typedef mozilla::gfx::Float Float;
  typedef mozilla::gfx::Path Path;
  typedef mozilla::gfx::PathBuilder PathBuilder;

public:
  nsSVGPathGeometryElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);

  








  bool GeometryDependsOnCoordCtx();

  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx) = 0;

  



  virtual mozilla::TemporaryRef<Path> BuildPath() = 0;

  virtual mozilla::TemporaryRef<Path> GetPathForLengthOrPositionMeasuring();

  



  mozilla::TemporaryRef<PathBuilder> CreatePathBuilder();

  



  FillRule GetFillRule();

  





  Float GetStrokeWidth();
};

#endif
