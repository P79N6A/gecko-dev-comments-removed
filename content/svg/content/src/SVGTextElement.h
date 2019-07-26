




#ifndef mozilla_dom_SVGTextElement_h
#define mozilla_dom_SVGTextElement_h

#include "mozilla/dom/SVGTextPositioningElement.h"

nsresult NS_NewSVGTextElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTextPositioningElement SVGTextElementBase;

class SVGTextElement MOZ_FINAL : public SVGTextElementBase,
                                 public nsIDOMSVGElement
{
protected:
  SVGTextElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

  friend nsresult (::NS_NewSVGTextElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGTextElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
