



#ifndef mozilla_dom_HTMLTableRowElement_h
#define mozilla_dom_HTMLTableRowElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLTableRowElement.h"

class nsIDOMHTMLTableElement;
class nsIDOMHTMLTableSectionElement;
class nsContentList;

namespace mozilla {
namespace dom {

class HTMLTableRowElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLTableRowElement
{
public:
  HTMLTableRowElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }

  NS_IMPL_FROMCONTENT_HTML_WITH_TAG(HTMLTableRowElement, tr)

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTABLEROWELEMENT

  int32_t RowIndex() const;
  int32_t SectionRowIndex() const;
  nsIHTMLCollection* Cells();
  already_AddRefed<nsGenericHTMLElement>
    InsertCell(int32_t aIndex, ErrorResult& aError);
  void DeleteCell(int32_t aValue, ErrorResult& aError);

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
  void GetBgColor(nsString& aBgColor)
  {
    GetHTMLAttr(nsGkAtoms::bgcolor, aBgColor);
  }
  void SetBgColor(const nsAString& aBgColor, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::bgcolor, aBgColor, aError);
  }

  virtual bool ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(HTMLTableRowElement,
                                                     nsGenericHTMLElement)

protected:
  JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  already_AddRefed<nsIDOMHTMLTableSectionElement> GetSection() const;
  HTMLTableElement* GetTable() const;
  nsRefPtr<nsContentList> mCells;
};

} 
} 

#endif 
