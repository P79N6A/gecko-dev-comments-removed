



































#include "nsIDOMHTMLTableColElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsRuleData.h"



#define MAX_COLSPAN 1000

class nsHTMLTableColElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLTableColElement
{
public:
  nsHTMLTableColElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLTableColElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLECOLELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableCol)


nsHTMLTableColElement::nsHTMLTableColElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTableColElement::~nsHTMLTableColElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableColElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableColElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLTableColElement,
                                    nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableColElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableColElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLTableColElement)


NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableColElement, Align, align, "left")
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableColElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, ChOff, charoff)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableColElement, Span, span, 1)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableColElement, VAlign, valign, "middle")
NS_IMPL_STRING_ATTR(nsHTMLTableColElement, Width, width)


PRBool
nsHTMLTableColElement::ParseAttribute(PRInt32 aNamespaceID,
                                      nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    
    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::span) {
      
      return aResult.ParseIntWithBounds(aValue, 1, MAX_COLSPAN);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
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
  if (aData->mSID == eStyleStruct_Position &&
      aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
    if (value) {
      switch (value->Type()) {
      case nsAttrValue::ePercent: {
        aData->mPositionData->mWidth.SetPercentValue(value->GetPercentValue());
        break;
      }
      case nsAttrValue::eInteger: {
        aData->mPositionData->mWidth.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
        break;
      }
      case nsAttrValue::eProportional: {
        aData->mPositionData->mWidth.SetFloatValue((float)value->GetProportionalValue(), eCSSUnit_Proportional);
        break;
      }
      default:
        break;
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Text) {
    if (aData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTextData->mTextAlign.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }
  else if (aData->mSID == eStyleStruct_TextReset) {
    if (aData->mTextData->mVerticalAlign.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::valign);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTextData->mVerticalAlign.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

static 
void ColMapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                              nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_Table && 
      aData->mTableData->mSpan.GetUnit() == eCSSUnit_Null) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::span);
    if (value && value->Type() == nsAttrValue::eInteger)
      aData->mTableData->mSpan.SetIntValue(value->GetIntegerValue(),
                                           eCSSUnit_Integer);
  }

  MapAttributesIntoRule(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableColElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::align },
    { &nsGkAtoms::valign },
    { nsnull }
  };

  static const MappedAttributeEntry span_attribute[] = {
    { &nsGkAtoms::span },
    { nsnull }
  };

  static const MappedAttributeEntry* const col_map[] = {
    attributes,
    span_attribute,
    sCommonAttributeMap,
  };

  static const MappedAttributeEntry* const colspan_map[] = {
    attributes,
    sCommonAttributeMap,
  };

  
  if (mNodeInfo->Equals(nsGkAtoms::col))
    return FindAttributeDependence(aAttribute, col_map,
                                   NS_ARRAY_LENGTH(col_map));
  return FindAttributeDependence(aAttribute, colspan_map,
                                 NS_ARRAY_LENGTH(colspan_map));
}


nsMapRuleToAttributesFunc
nsHTMLTableColElement::GetAttributeMappingFunction() const
{
  if (mNodeInfo->Equals(nsGkAtoms::col)) {
    return &ColMapAttributesIntoRule;
  }

  return &MapAttributesIntoRule;
}
