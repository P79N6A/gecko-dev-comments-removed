



#ifndef mozilla_dom_HTMLTableCellElement_h
#define mozilla_dom_HTMLTableCellElement_h

#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLTableCellElement.h"

class nsIDOMHTMLTableRowElement;

namespace mozilla {
namespace dom {

class HTMLTableElement;

class HTMLTableCellElement : public nsGenericHTMLElement,
                             public nsIDOMHTMLTableCellElement
{
public:
  HTMLTableCellElement(already_AddRefed<nsINodeInfo> aNodeInfo)
    : nsGenericHTMLElement(aNodeInfo)
  {
    SetIsDOMBinding();
  }
  virtual ~HTMLTableCellElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE_TO_NSINODE

  
  NS_FORWARD_NSIDOMELEMENT_TO_GENERIC

  
  NS_FORWARD_NSIDOMHTMLELEMENT_TO_GENERIC

  
  NS_DECL_NSIDOMHTMLTABLECELLELEMENT

  uint32_t ColSpan() const
  {
    return GetIntAttr(nsGkAtoms::colspan, 1);
  }
  void SetColSpan(uint32_t aColSpan, ErrorResult& aError)
  {
    SetHTMLIntAttr(nsGkAtoms::colspan, aColSpan, aError);
  }
  uint32_t RowSpan() const
  {
    return GetIntAttr(nsGkAtoms::rowspan, 1);
  }
  void SetRowSpan(uint32_t aRowSpan, ErrorResult& aError)
  {
    SetHTMLIntAttr(nsGkAtoms::rowspan, aRowSpan, aError);
  }
  
  void GetHeaders(nsString& aHeaders)
  {
    GetHTMLAttr(nsGkAtoms::headers, aHeaders);
  }
  void SetHeaders(const nsAString& aHeaders, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::headers, aHeaders, aError);
  }
  int32_t CellIndex() const;

  void GetAbbr(nsString& aAbbr)
  {
    GetHTMLAttr(nsGkAtoms::abbr, aAbbr);
  }
  void SetAbbr(const nsAString& aAbbr, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::abbr, aAbbr, aError);
  }
  void GetScope(nsString& aScope)
  {
    GetHTMLAttr(nsGkAtoms::scope, aScope);
  }
  void SetScope(const nsAString& aScope, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::scope, aScope, aError);
  }
  void GetAlign(nsString& aAlign);
  void SetAlign(const nsAString& aAlign, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::align, aAlign, aError);
  }
  void GetAxis(nsString& aAxis)
  {
    GetHTMLAttr(nsGkAtoms::axis, aAxis);
  }
  void SetAxis(const nsAString& aAxis, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::axis, aAxis, aError);
  }
  void GetHeight(nsString& aHeight)
  {
    GetHTMLAttr(nsGkAtoms::height, aHeight);
  }
  void SetHeight(const nsAString& aHeight, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::height, aHeight, aError);
  }
  void GetWidth(nsString& aWidth)
  {
    GetHTMLAttr(nsGkAtoms::width, aWidth);
  }
  void SetWidth(const nsAString& aWidth, ErrorResult& aError)
  {
    SetHTMLAttr(nsGkAtoms::width, aWidth, aError);
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
  bool NoWrap()
  {
    return GetBoolAttr(nsGkAtoms::nowrap);
  }
  void SetNoWrap(bool aNoWrap, ErrorResult& aError)
  {
    SetHTMLBoolAttr(nsGkAtoms::nowrap, aNoWrap, aError);
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
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsIDOMNode* AsDOMNode() { return this; }
protected:
  virtual JSObject* WrapNode(JSContext *aCx, JSObject *aScope) MOZ_OVERRIDE;

  HTMLTableElement* GetTable() const;

  HTMLTableRowElement* GetRow() const;
};

} 
} 

#endif 
