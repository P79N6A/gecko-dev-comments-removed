




#ifndef mozilla_dom_SVGDescElement_h
#define mozilla_dom_SVGDescElement_h

#include "nsSVGElement.h"

nsresult NS_NewSVGDescElement(nsIContent **aResult,
                              already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGElement SVGDescElementBase;

namespace mozilla {
namespace dom {

class SVGDescElement MOZ_FINAL : public SVGDescElementBase,
                                 public nsIDOMSVGElement
{
protected:
  friend nsresult (::NS_NewSVGDescElement(nsIContent **aResult,
                                          already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGDescElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGDescElementBase::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 

