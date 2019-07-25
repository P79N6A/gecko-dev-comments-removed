



































#ifndef NS_SVGPOLYELEMENT_H_
#define NS_SVGPOLYELEMENT_H_

#include "nsSVGPathGeometryElement.h"
#include "nsIDOMSVGAnimatedPoints.h"
#include "SVGAnimatedPointList.h"

typedef nsSVGPathGeometryElement nsSVGPolyElementBase;

class gfxContext;

class nsSVGPolyElement : public nsSVGPolyElementBase,
                         public nsIDOMSVGAnimatedPoints
{
protected:
  nsSVGPolyElement(already_AddRefed<nsINodeInfo> aNodeInfo);

public:
  
  
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGANIMATEDPOINTS

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  virtual SVGAnimatedPointList* GetAnimatedPointList() {
    return &mPoints;
  }
  virtual nsIAtom* GetPointListAttrName() const {
    return nsGkAtoms::points;
  }

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName);
  virtual bool IsMarkable() { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

protected:
  SVGAnimatedPointList mPoints;
};

#endif 
