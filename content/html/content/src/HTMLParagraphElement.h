




#ifndef mozilla_dom_HTMLParagraphElement_h
#define mozilla_dom_HTMLParagraphElement_h

#include "mozilla/Attributes.h"
#include "mozilla/Util.h"

#include "nsIDOMHTMLParagraphElement.h"
#include "nsGenericHTMLElement.h"

namespace mozilla {
namespace dom {

class HTMLParagraphElement MOZ_FINAL : public nsGenericHTMLElement,
                                       public nsIDOMHTMLParagraphElement
{
public:
  HTMLParagraphElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }
  virtual ~HTMLParagraphElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLPARAGRAPHELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  
  void SetAlign(const nsAString& aValue, mozilla::ErrorResult& rv)
  {
    SetHTMLAttr(nsGkAtoms::align, aValue, rv);
  }

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
