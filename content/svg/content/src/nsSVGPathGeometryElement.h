



































#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsTArray.h"
#include "gfxPath.h"
#include "gfxMatrix.h"

struct nsSVGMark {
  float x, y, angle;
  nsSVGMark(float aX, float aY, float aAngle) :
    x(aX), y(aY), angle(aAngle) {}
};

class gfxContext;

typedef nsSVGGraphicElement nsSVGPathGeometryElementBase;

class nsSVGPathGeometryElement : public nsSVGPathGeometryElementBase
{
public:
  nsSVGPathGeometryElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual bool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx) = 0;
  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(const gfxMatrix &aMatrix);
};

#endif
