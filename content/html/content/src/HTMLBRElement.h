




#ifndef mozilla_dom_HTMLBRElement_h
#define mozilla_dom_HTMLBRElement_h

#include "mozilla/Attributes.h"
#include "nsIDOMHTMLBRElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"

namespace mozilla {
namespace dom {

class HTMLBRElement MOZ_FINAL : public nsGenericHTMLElement,
                                public nsIDOMHTMLBRElement
{
public:
  HTMLBRElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLBRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLBRELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  bool Clear()
  {
    return GetBoolAttr(nsGkAtoms::clear);
  }
  void SetClear(const nsAString& aClear, ErrorResult& aError)
  {
    return SetHTMLAttr(nsGkAtoms::clear, aClear, aError);
  }

  virtual JSObject* WrapNode(JSContext *aCx,
                             JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif

