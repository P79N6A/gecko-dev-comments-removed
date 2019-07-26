




#ifndef mozilla_dom_SVGCircleElement_h
#define mozilla_dom_SVGCircleElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGCircleElement(nsIContent **aResult,
                                already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGPathGeometryElement SVGCircleElementBase;

namespace mozilla {
namespace dom {

class SVGCircleElement MOZ_FINAL : public SVGCircleElementBase,
                                   public nsIDOMSVGElement
{
protected:
  SVGCircleElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGCircleElement(nsIContent **aResult,
                                            already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGCircleElementBase::)

  
  virtual bool HasValidDimensions() const;

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<SVGAnimatedLength> Cx();
  already_AddRefed<SVGAnimatedLength> Cy();
  already_AddRefed<SVGAnimatedLength> R();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { ATTR_CX, ATTR_CY, ATTR_R };
  nsSVGLength2 mLengthAttributes[3];
  static LengthInfo sLengthInfo[3];
};

} 
} 

#endif 
