




#ifndef mozilla_dom_HTMLHeadingElement_h
#define mozilla_dom_HTMLHeadingElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLHeadingElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLHeadingElement final : public nsGenericHTMLElement,
			         public nsIDOMHTMLHeadingElement
{
public:
  explicit HTMLHeadingElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLHEADINGELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  

protected:
  virtual ~HTMLHeadingElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
