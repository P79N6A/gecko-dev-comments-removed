



#ifndef mozilla_dom_HTMLTableSectionElement_h
#define mozilla_dom_HTMLTableSectionElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLTableSectionElement.h"
#include "nsContentList.h" 

namespace mozilla {
namespace dom {

class HTMLTableSectionElement : public nsGenericHTMLElement,
                                public nsIDOMHTMLTableSectionElement
{
public:
  HTMLTableSectionElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTABLESECTIONELEMENT

  nsIHTMLCollection* Rows();
  already_AddRefed<nsGenericHTMLElement>
    InsertRow(int32_t aIndex, ErrorResult& aError);
  void DeleteRow(int32_t aValue, ErrorResult& aError);

  void GetAlign(nsString& aAlign)
  {
    GetHTMLAttr(nsGkAtoms::align, aAlign);
  }
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }
  void GetCh(nsString& aCh)
  {
    GetHTMLAttr(nsGkAtoms::_char, aCh);
  }
  void SetCh(const nsAString& aCh, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::_char, aCh, aError);
  }
  void GetChOff(nsString& aChOff)
  {
    GetHTMLAttr(nsGkAtoms::charoff, aChOff);
  }
  void SetChOff(const nsAString& aChOff, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::charoff, aChOff, aError);
  }
  void GetVAlign(nsString& aVAlign)
  {
    GetHTMLAttr(nsGkAtoms::valign, aVAlign);
  }
  void SetVAlign(const nsAString& aVAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::valign, aVAlign, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                              nsIAtom* aAttribute,
                              const nsAString& aValue,
                              nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLTableSectionElement,
                                                     nsGenericHTMLElement)

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  JSObject* WrapNode(JSContext *aCx, JSObject *aScope, bool *aTriedToWrap);

  nsRefPtr<nsContentList> mRows;
};

} 
} 

#endif 
