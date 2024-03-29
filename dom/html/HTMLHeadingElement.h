





#ifndef mozilla_dom_HTMLHeadingElement_h
#define mozilla_dom_HTMLHeadingElement_h

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLHeadingElement final : public nsGenericHTMLElement
{
public:
  explicit HTMLHeadingElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    return SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }
  void GetAlign(DOMString& aAlign) const
  {
    return GetHTMLAttr(nsGkAtoms::align, aAlign);
  }

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
