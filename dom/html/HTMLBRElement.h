




#ifndef mozilla_dom_HTMLBRElement_h
#define mozilla_dom_HTMLBRElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLBRElement final : public nsGenericHTMLElement,
                            public nsIDOMHTMLBRElement
{
public:
  explicit HTMLBRElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLBRELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) override;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const override;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const override;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const override;

  bool Clear()
  {
    return GetBoolAttr(nsGkAtoms::clear);
  }
  void SetClear(const nsAString& aClear, ErrorResult& aError)
  {
    return SetHTMLAttr(nsGkAtoms::clear, aClear, aError);
  }

  virtual JSObject* WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  virtual ~HTMLBRElement();

  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif

