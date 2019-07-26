



#ifndef nsHTMLDivElement_h___
#define nsHTMLDivElement_h___

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLDivElement.h"

class nsHTMLDivElement MOZ_FINAL : public nsGenericHTMLElement,
                                   public nsIDOMHTMLDivElement
{
public:
  nsHTMLDivElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLDivElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_IMETHOD GetAlign(nsAString& aAlign)
  {
    nsString align;
    GetAlign(align);
    aAlign = align;
    return NS_OK;
  }
  NS_IMETHOD SetAlign(const nsAString& aAlign)
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
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
  virtual nsIDOMNode* AsDOMNode() { return this; }

protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope,
                             bool *aTriedToWrap) MOZ_OVERRIDE;
};

#endif 
