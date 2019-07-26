




#ifndef mozilla_dom_SVGLineElement_h
#define mozilla_dom_SVGLineElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGLineElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo>&& aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGPathGeometryElement SVGLineElementBase;

class SVGLineElement MOZ_FINAL : public SVGLineElementBase
{
protected:
  SVGLineElement(already_AddRefed<nsINodeInfo>& aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGLineElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo>&& aNodeInfo));

public:
  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const MOZ_OVERRIDE;

  
  virtual bool IsMarkable() MOZ_OVERRIDE { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks) MOZ_OVERRIDE;
  virtual void ConstructPath(gfxContext *aCtx) MOZ_OVERRIDE;
  virtual TemporaryRef<Path> BuildPath() MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  
  already_AddRefed<SVGAnimatedLength> X1();
  already_AddRefed<SVGAnimatedLength> Y1();
  already_AddRefed<SVGAnimatedLength> X2();
  already_AddRefed<SVGAnimatedLength> Y2();

protected:

  virtual LengthAttributesInfo GetLengthInfo() MOZ_OVERRIDE;

  enum { ATTR_X1, ATTR_Y1, ATTR_X2, ATTR_Y2 };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

} 
} 

#endif 
