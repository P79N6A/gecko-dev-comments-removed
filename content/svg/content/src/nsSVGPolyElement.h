




#ifndef NS_SVGPOLYELEMENT_H_
#define NS_SVGPOLYELEMENT_H_

#include "mozilla/Attributes.h"
#include "nsSVGPathGeometryElement.h"
#include "SVGAnimatedPointList.h"

typedef nsSVGPathGeometryElement nsSVGPolyElementBase;

class gfxContext;

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

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  virtual SVGAnimatedPointList* GetAnimatedPointList() {
    return &mPoints;
  }
  virtual nsIAtom* GetPointListAttrName() const {
    return nsGkAtoms::points;
  }

  
  virtual bool HasValidDimensions() const MOZ_OVERRIDE;

  
  virtual bool AttributeDefinesGeometry(const nsIAtom *aName) MOZ_OVERRIDE;
  virtual bool IsMarkable() MOZ_OVERRIDE { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks) MOZ_OVERRIDE;
  virtual void ConstructPath(gfxContext *aCtx) MOZ_OVERRIDE;
  virtual mozilla::TemporaryRef<Path> BuildPath(PathBuilder* aBuilder = nullptr) MOZ_OVERRIDE;

  
  already_AddRefed<mozilla::DOMSVGPointList> Points();
  already_AddRefed<mozilla::DOMSVGPointList> AnimatedPoints();

protected:
  SVGAnimatedPointList mPoints;
};

#endif 
