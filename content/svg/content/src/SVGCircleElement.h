




#ifndef mozilla_dom_SVGCircleElement_h
#define mozilla_dom_SVGCircleElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGCircleElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo>&& aNodeInfo);

typedef nsSVGPathGeometryElement SVGCircleElementBase;

namespace mozilla {
namespace dom {

class SVGCircleElement MOZ_FINAL : public SVGCircleElementBase
{
protected:
  SVGCircleElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGCircleElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo>&& aNodeInfo));

public:
  
  virtual bool HasValidDimensions() const MOZ_OVERRIDE;

  
  virtual void ConstructPath(gfxContext *aCtx) MOZ_OVERRIDE;
  virtual TemporaryRef<Path> BuildPath() MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedLength> Cx();
  already_AddRefed<SVGAnimatedLength> Cy();
  already_AddRefed<SVGAnimatedLength> R();

protected:

  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;

  enum { ATTR_CX, ATTR_CY, ATTR_R };
  nsSVGLength2 mLengthAttributes[3];
  static LengthInfo sLengthInfo[3];
};

} 
} 

#endif 
