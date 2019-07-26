




#ifndef mozilla_dom_SVGEllipseElement_h
#define mozilla_dom_SVGEllipseElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGEllipseElement(nsIContent **aResult,
                                 already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGPathGeometryElement SVGEllipseElementBase;

class SVGEllipseElement MOZ_FINAL : public SVGEllipseElementBase
{
protected:
  SVGEllipseElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGEllipseElement(nsIContent **aResult,
                                             already_AddRefed<nsINodeInfo>&& aNodeInfo));

public:
  
  virtual bool HasValidDimensions() const MOZ_OVERRIDE;

  
  virtual void ConstructPath(gfxContext *aCtx) MOZ_OVERRIDE;
  virtual TemporaryRef<Path> BuildPath() MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  already_AddRefed<SVGAnimatedLength> Cx();
  already_AddRefed<SVGAnimatedLength> Cy();
  already_AddRefed<SVGAnimatedLength> Rx();
  already_AddRefed<SVGAnimatedLength> Ry();

protected:

  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;

  enum { CX, CY, RX, RY };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

} 
} 

#endif 
