




#ifndef mozilla_dom_SVGPolylineElement_h
#define mozilla_dom_SVGPolylineElement_h

#include "nsSVGPolyElement.h"

nsresult NS_NewSVGPolylineElement(nsIContent **aResult,
                                  already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGPolyElement SVGPolylineElementBase;

namespace mozilla {
namespace dom {

class SVGPolylineElement MOZ_FINAL : public SVGPolylineElementBase,
                                     public nsIDOMSVGElement
{
protected:
  SVGPolylineElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual JSObject* WrapNode(JSContext *cx, JSObject *scope) MOZ_OVERRIDE;
  friend nsresult (::NS_NewSVGPolylineElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo));

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(SVGPolylineElementBase::)

  
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
