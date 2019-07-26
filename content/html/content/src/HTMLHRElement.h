





#ifndef mozilla_dom_HTMLHRElement_h
#define mozilla_dom_HTMLHRElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLHRElement.h"
#include "nsIDOMEventTarget.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsRuleData.h"

namespace mozilla {
namespace dom {

class HTMLHRElement : public nsGenericHTMLElement,
                      public nsIDOMHTMLHRElement
{
public:
  HTMLHRElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~HTMLHRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLHRELEMENT

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsIDOMNode* AsDOMNode() { return this; }

  
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
  virtual JSObject* WrapNode(JSContext* aCx, JSObject* aScope) MOZ_OVERRIDE;
};

} 
} 

#endif 
