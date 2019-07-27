





#ifndef NS_SVGPOLYELEMENT_H_
#define NS_SVGPOLYELEMENT_H_

#include "mozilla/Attributes.h"
#include "nsSVGPathGeometryElement.h"
#include "SVGAnimatedPointList.h"

typedef nsSVGPathGeometryElement nsSVGPolyElementBase;

namespace mozilla {
class DOMSVGPointList;
} 

class nsSVGPolyElement : public nsSVGPolyElementBase
{
protected:
  explicit nsSVGPolyElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  virtual ~nsSVGPolyElement();

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const override;

  virtual SVGAnimatedPointList* GetAnimatedPointList() override {
    return &mPoints;
  }
  virtual nsIAtom* GetPointListAttrName() const override {
    return nsGkAtoms::points;
  }

  
  virtual bool HasValidDimensions() const override;

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName) override;
  virtual bool IsMarkable() override { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks) override;
  virtual bool GetGeometryBounds(Rect* aBounds, const StrokeOptions& aStrokeOptions,
                                 const Matrix& aTransform) override;

  
  already_AddRefed<mozilla::DOMSVGPointList> Points();
  already_AddRefed<mozilla::DOMSVGPointList> AnimatedPoints();

protected:
  SVGAnimatedPointList mPoints;
};

#endif 
