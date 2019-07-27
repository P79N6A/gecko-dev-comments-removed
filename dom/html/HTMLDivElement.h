



#ifndef HTMLDivElement_h___
#define HTMLDivElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLDivElement final : public nsGenericHTMLElement
{
public:
  explicit HTMLDivElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  void GetAlign(DOMString& aAlign)
  {
    GetHTMLAttr(nsGkAtoms::align, aAlign);
  }
  void SetAlign(const nsAString& aAlign, mozilla::ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

protected:
  virtual ~HTMLDivElement();

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
