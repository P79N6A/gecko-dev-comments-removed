




#ifndef mozilla_dom_SVGLineElement_h
#define mozilla_dom_SVGLineElement_h

#include "nsSVGPathGeometryElement.h"
#include "nsSVGLength2.h"

nsresult NS_NewSVGLineElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef nsSVGPathGeometryElement SVGLineElementBase;

class SVGLineElement MOZ_FINAL : public SVGLineElementBase,
                                 public nsIDOMSVGElement
{
protected:
  SVGLineElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGLineElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGLineElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* name) const;

  
  virtual bool IsMarkable() { return true; }
  virtual void GetMarkPoints(nsTArray<nsSVGMark> *aMarks);
  virtual void ConstructPath(gfxContext *aCtx);

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<SVGAnimatedLength> X1();
  already_AddRefed<SVGAnimatedLength> Y1();
  already_AddRefed<SVGAnimatedLength> X2();
  already_AddRefed<SVGAnimatedLength> Y2();

protected:

  virtual LengthAttributesInfo GetLengthInfo();

  enum { ATTR_X1, ATTR_Y1, ATTR_X2, ATTR_Y2 };
  nsSVGLength2 mLengthAttributes[4];
  static LengthInfo sLengthInfo[4];
};

} 
} 

#endif 
