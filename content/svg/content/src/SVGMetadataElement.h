




#ifndef mozilla_dom_SVGMetadataElement_h
#define mozilla_dom_SVGMetadataElement_h

#include "nsSVGElement.h"

nsresult NS_NewSVGMetadataElement(nsIContent **aResult,
                                  already_AddRefed<nsINodeInfo> aNodeInfo);

typedef nsSVGElement SVGMetadataElementBase;

namespace mozilla {
namespace dom {

class SVGMetadataElement MOZ_FINAL : public SVGMetadataElementBase,
                                     public nsIDOMSVGElement
{
protected:
  friend nsresult (::NS_NewSVGMetadataElement(nsIContent **aResult,
                                              already_AddRefed<nsINodeInfo> aNodeInfo));
  SVGMetadataElement(already_AddRefed<nsINodeInfo> aNodeInfo);

  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
  nsresult Init();

public:
  

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC
  NS_FORWARD_NSIDOMSVGELEMENT(nsSVGElement::)

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
};

} 
} 

#endif 
