





#ifndef mozilla_dom_HTMLHRElement_h
#define mozilla_dom_HTMLHRElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLHRElement.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsRuleData.h"

namespace mozilla {
namespace dom {

class HTMLHRElement MOZ_FINAL : public nsGenericHTMLElement,
                                public nsIDOMHTMLHRElement
{
public:
  explicit HTMLHRElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_DECL_NSIDOMHTMLHRELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult) MOZ_OVERRIDE;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const MOZ_OVERRIDE;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const MOZ_OVERRIDE;
  virtual nsresult Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const MOZ_OVERRIDE;

  
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }

  
  void SetColor(const nsAString& aColor, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::color, aColor, aError);
  }

  bool NoShade() const
  {
   return GetBoolAttr(nsGkAtoms::noshade);
  }
  void SetNoShade(bool aNoShade, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::noshade, aNoShade, aError);
  }

  
  void SetSize(const nsAString& aSize, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::size, aSize, aError);
  }

  
  void SetWidth(const nsAString& aWidth, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::width, aWidth, aError);
  }

protected:
  virtual ~HTMLHRElement();

  virtual JSObject* WrapNode(JSContext* aCx) MOZ_OVERRIDE;

private:
  static void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData);
};

} 
} 

#endif 
