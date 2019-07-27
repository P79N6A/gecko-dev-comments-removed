



#ifndef HTMLDivElement_h___
#define HTMLDivElement_h___

#include "mozilla/Attributes.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLDivElement.h"

namespace mozilla {
namespace dom {

class HTMLDivElement MOZ_FINAL : public nsGenericHTMLElement,
                                 public nsIDOMHTMLDivElement
{
public:
  HTMLDivElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetAlign(nsAString& aAlign) MOZ_OVERRIDE
  {
    nsString align;
    GetAlign(align);
    aAlign = align;
    return NS_OK;
  }
  NS_IMETHOD SetAlign(const nsAString& aAlign) MOZ_OVERRIDE
  {
    mozilla::ErrorResult rv;
    SetAlign(aAlign, rv);
    return rv.ErrorCode();
  }

  void GetAlign(nsString& aAlign)
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
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

protected:
  virtual ~HTMLDivElement();

  virtual JSObject* WrapNode(JSContext *aCx) MOZ_OVERRIDE;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
