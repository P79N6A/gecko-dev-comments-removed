




#ifndef mozilla_dom_SVGRectElement_h
#define mozilla_dom_SVGRectElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGRectElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGPathGeometryElement SVGRectElementBase;

namespace mozilla {
namespace dom {

class SVGRectElement MOZ_FINAL : public SVGRectElementBase,
                                 public nsIDOMSVGElement
{
protected:
  SVGRectElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGRectElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGRectElementBase::)

  
  virtual bool HasValidDimensions() const;

  
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<SVGAnimatedLength> X();
  already_AddRefed<SVGAnimatedLength> Y();
  already_AddRefed<SVGAnimatedLength> Height();
  already_AddRefed<SVGAnimatedLength> Width();
  already_AddRefed<SVGAnimatedLength> Rx();
  already_AddRefed<SVGAnimatedLength> Ry();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { ATTR_X, ATTR_Y, ATTR_WIDTH, ATTR_HEIGHT, ATTR_RX, ATTR_RY };
  nsSVGLength2 mLengthAttributes[6];
  static LengthInfo sLengthInfo[6];
};

} 
} 

#endif 
