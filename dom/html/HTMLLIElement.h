




#ifndef mozilla_dom_HTMLLIElement_h
#define mozilla_dom_HTMLLIElement_h

#include "mozilla/Attributes.h"

#include "nsIDOMHTMLLIElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLLIElement final : public nsGenericHTMLElement,
                                public nsIDOMHTMLLIElement
{
public:
  explicit HTMLLIElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLLIELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  
  void GetType(DOMString& aType)
  {
    GetHTMLAttr(nsGkAtoms::type, aType);
  }
  void SetType(const nsAString& aType, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::type, aType, rv);
  }
  int32_t Value() const
  {
    return GetIntAttr(nsGkAtoms::value, 0);
  }
  void SetValue(int32_t aValue, mozilla::ErrorResult& rv)
  {
    SetHTMLIntAttr(nsGkAtoms::value, aValue, rv);
  }

protected:
  virtual ~HTMLLIElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
