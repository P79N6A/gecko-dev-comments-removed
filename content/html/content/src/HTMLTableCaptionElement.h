



#ifndef mozilla_dom_HTMLTableCaptionElement_h
#define mozilla_dom_HTMLTableCaptionElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLTableCaptionElem.h"

namespace mozilla {
namespace dom {

class HTMLTableCaptionElement : public nsGenericHTMLElement,
                                public nsIDOMHTMLTableCaptionElement
{
public:
  HTMLTableCaptionElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLTableCaptionElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTABLECAPTIONELEMENT

  void GetAlign(nsString& aAlign)
  {
    GetHTMLAttr(nsGkAtoms::align, aAlign);
  }
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
