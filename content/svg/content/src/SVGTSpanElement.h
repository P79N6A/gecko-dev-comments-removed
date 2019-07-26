




#ifndef mozilla_dom_SVGTSpanElement_h
#define mozilla_dom_SVGTSpanElement_h

#include "mozilla/dom/SVGTextPositioningElement.h"

nsresult NS_NewSVGTSpanElement(nsIContent **aResult,
                               already_AddRefed<nsINodeInfo> aNodeInfo);

namespace mozilla {
namespace dom {

typedef SVGTextPositioningElement SVGTSpanElementBase;

class SVGTSpanElement MOZ_FINAL : public SVGTSpanElementBase,
                                  public nsIDOMSVGElement
{
protected:
  friend nsresult (::NS_NewSVGTSpanElement(nsIContent **aResult,
                                           already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGTSpanElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGTSpanElementBase::)

  
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:

  
  virtual bool IsEventName(nsIAtom* aName);
};

} 
} 

#endif 
