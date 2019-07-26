




#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "SVGGraphicsElement.h"

struct gfxMatrix;
template <class E> class nsTArray;

struct nsSVGMark {
  float x, y, angle;
  nsSVGMark(float aX, float aY, float aAngle) :
    x(aX), y(aY), angle(aAngle) {}
};

class gfxContext;

typedef mozilla::dom::SVGGraphicsElement nsSVGPathGeometryElementBase;

class nsSVGPathGeometryElement : public nsSVGPathGeometryElementBase
{
public:
  nsSVGPathGeometryElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx) = 0;
  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(const gfxMatrix &aMatrix);
};

#endif
