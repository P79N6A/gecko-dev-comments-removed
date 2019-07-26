




#ifndef mozilla_dom_SVGStopElement_h
#define mozilla_dom_SVGStopElement_h

#include "nsSVGElement.h"
#include "nsSVGNumber2.h"
#include "nsIDOMSVGStopElement.h"

nsresult NS_NewSVGStopElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGElement SVGStopElementBase;

namespace mozilla {
namespace dom {

class SVGStopElement MOZ_FINAL : public SVGStopElementBase,
                                 public nsIDOMSVGStopElement
{
protected:
  friend nsresult (::NS_NewSVGStopElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGStopElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMSVGSTOPELEMENT

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGStopElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  already_AddRefed<nsIDOMSVGAnimatedNumber> Offset();

protected:

  virtual NumberAttributesInfo GetNumberInfo();
  
  nsSVGNumber2 mOffset;
  static NumberInfo sNumberInfo;
};

} 
} 

#endif 
