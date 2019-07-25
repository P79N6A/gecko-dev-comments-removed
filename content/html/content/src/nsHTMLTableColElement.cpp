



































#include "nsIDOMHTMLTableColElement.h"
#include "nsIDOMEventTarget.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsRuleData.h"



#define MAX_COLSPAN 1000

class nsHTMLTableColElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLTableColElement
{
public:
  nsHTMLTableColElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLTableColElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLECOLELEMENT

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableCol)


nsHTMLTableColElement::nsHTMLTableColElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTableColElement::~nsHTMLTableColElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableColElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableColElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLTableColElement, nsHTMLTableColElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLTableColElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLTableColElement,
                                   nsIDOMHTMLTableColElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTableColElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableColElement)

NS_IMPL_ELEMENT_CLONE(nsHTMLTableColElement)


NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Ch, _char)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, ChOff, charoff)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableColElement, Span, span, 1)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, VAlign, valign)
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Width, width)


bool
nsHTMLTableColElement::ParseAttribute(PRInt32 aNamespaceID,
                                      nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    
    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::span) {
      
      return aResult.ParseIntWithBounds(aValue, 1, MAX_COLSPAN);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableCellHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::valign) {
      return ParseTableVAlignValue(aValue, aResult);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Table)) {
    nsCSSValue *span = aData->ValueForSpan();
    if (span->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::span);
      if (value && value->Type() == nsAttrValue::eInteger) {
        PRInt32 val = value->GetIntegerValue();
        
        
        
        if (val > 0) {
          span->SetIntValue(value->GetIntegerValue(), eCSSUnit_Integer);
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value) {
        switch (value->Type()) {
        case nsAttrValue::ePercent: {
          width->SetPercentValue(value->GetPercentValue());
          break;
        }
        case nsAttrValue::eInteger: {
          width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
          break;
        }
        default:
          break;
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    nsCSSValue* textAlign = aData->ValueForTextAlign();
    if (textAlign->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        textAlign->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TextReset)) {
    nsCSSValue* verticalAlign = aData->ValueForVerticalAlign();
    if (verticalAlign->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::valign);
      if (value && value->Type() == nsAttrValue::eEnum)
        verticalAlign->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
nsHTMLTableColElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::align },
    { &nsGkAtoms::valign },
    { &nsGkAtoms::span },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLTableColElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
