




#ifndef mozilla_dom_HTMLHeadingElement_h
#define mozilla_dom_HTMLHeadingElement_h

#include "mozilla/Attributes.h"
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
  }
  virtual ~HTMLHeadingElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLHEADINGELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  

protected:
  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
