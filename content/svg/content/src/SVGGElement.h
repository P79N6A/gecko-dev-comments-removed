




#ifndef mozilla_dom_SVGGElement_h
#define mozilla_dom_SVGGElement_h

#include "mozilla/dom/SVGGraphicsElement.h"

nsresult NS_NewSVGGElement(nsIContent **aResult,
                           already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

class SVGGElement MOZ_FINAL : public SVGGraphicsElement,
                              public nsIDOMSVGElement
{
protected:
  SVGGElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope, bool *triedToWrap) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGGElement(nsIContent **aResult,
                                       already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGGraphicsElement::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
