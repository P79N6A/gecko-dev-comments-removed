




#ifndef mozilla_dom_HTMLHeadingElement_h
#define mozilla_dom_HTMLHeadingElement_h

#include "nsIDOMHTMLHeadingElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLHeadingElement MOZ_FINAL : public nsGenericHTMLElement,
				     public nsIDOMHTMLHeadingElement
{
public:
  HTMLHeadingElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLHeadingElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLHEADINGELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  
  

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
