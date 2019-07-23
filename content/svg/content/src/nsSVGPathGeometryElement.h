



































#ifndef __NS_SVGPATHGEOMETRYELEMENT_H__
#define __NS_SVGPATHGEOMETRYELEMENT_H__

#include "nsSVGGraphicElement.h"
#include "nsTArray.h"
#include "gfxPath.h"

struct nsSVGMark {
  float x, y, angle;
  nsSVGMark(float aX, float aY, float aAngle) :
    x(aX), y(aY), angle(aAngle) {}
};

class nsIDOMSVGMatrix;
class gfxContext;

typedef nsSVGGraphicElement nsSVGPathGeometryElementBase;

class nsSVGPathGeometryElement : public nsSVGPathGeometryElementBase
{
public:
  nsSVGPathGeometryElement(nsINodeInfo *aNodeInfo);

  virtual PRBool IsDependentAttribute(nsIAtom *aName);
  virtual PRBool IsMarkable();
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx) = 0;
  virtual already_AddRefed<gfxFlattenedPath> GetFlattenedPath(nsIDOMSVGMatrix *aMatrix);
};

#endif
