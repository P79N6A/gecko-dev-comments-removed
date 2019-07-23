



































#include "nsCOMPtr.h"
#include "nsIDOMHTMLFontElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIDeviceContext.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsCSSStruct.h"
#include "nsRuleData.h"
#include "nsIDocument.h"

class nsHTMLFontElement : public nsGenericHTMLElement,
                          public nsIDOMHTMLFontElement
{
public:
  nsHTMLFontElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLFontElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLFONTELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Font)


nsHTMLFontElement::nsHTMLFontElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLFontElement::~nsHTMLFontElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLFontElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFontElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLFontElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLFontElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLFontElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLFontElement)


NS_IMPL_STRING_ATTR(nsHTMLFontElement, Color, color)
NS_IMPL_STRING_ATTR(nsHTMLFontElement, Face, face)
NS_IMPL_STRING_ATTR(nsHTMLFontElement, Size, size)

static const nsAttrValue::EnumTable kRelFontSizeTable[] = {
  { "-10", -10 },
  { "-9", -9 },
  { "-8", -8 },
  { "-7", -7 },
  { "-6", -6 },
  { "-5", -5 },
  { "-4", -4 },
  { "-3", -3 },
  { "-2", -2 },
  { "-1", -1 },
  { "-0", 0 },
  { "+0", 0 },
  { "+1", 1 },
  { "+2", 2 },
  { "+3", 3 },
  { "+4", 4 },
  { "+5", 5 },
  { "+6", 6 },
  { "+7", 7 },
  { "+8", 8 },
  { "+9", 9 },
  { "+10", 10 },
  { 0 }
};


PRBool
nsHTMLFontElement::ParseAttribute(PRInt32 aNamespaceID,
                                  nsIAtom* aAttribute,
                                  const nsAString& aValue,
                                  nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::size) {
      nsAutoString tmp(aValue);
      tmp.CompressWhitespace(PR_TRUE, PR_TRUE);
      PRUnichar ch = tmp.IsEmpty() ? 0 : tmp.First();
      if ((ch == '+' || ch == '-') &&
          aResult.ParseEnumValue(aValue, kRelFontSizeTable)) {
        return PR_TRUE;
      }

      return aResult.ParseIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::pointSize ||
        aAttribute == nsGkAtoms::fontWeight) {
      return aResult.ParseIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::color) {
      return aResult.ParseColor(aValue, GetOwnerDoc());
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_Font) {
    nsRuleDataFont& font = *(aData->mFontData);
    
    
    if (font.mFamily.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::face);
      if (value && value->Type() == nsAttrValue::eString &&
          !value->IsEmptyString()) {
        font.mFamily.SetStringValue(value->GetStringValue(), eCSSUnit_String);
        font.mFamilyFromHTML = PR_TRUE;
      }
    }

    
    if (font.mSize.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::pointSize);
      if (value && value->Type() == nsAttrValue::eInteger)
        font.mSize.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Point);
      else {
        
        value = aAttributes->GetAttr(nsGkAtoms::size);
        if (value) {
          nsAttrValue::ValueType unit = value->Type();
          if (unit == nsAttrValue::eInteger || unit == nsAttrValue::eEnum) { 
            PRInt32 size;
            if (unit == nsAttrValue::eEnum) 
              size = value->GetEnumValue() + 3;  
            else
              size = value->GetIntegerValue();

            size = ((0 < size) ? ((size < 8) ? size : 7) : 1); 
            font.mSize.SetIntValue(size, eCSSUnit_Enumerated);
          }
        }
      }
    }

    
    if (font.mWeight.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::fontWeight);
      if (value && value->Type() == nsAttrValue::eInteger) 
        font.mWeight.SetIntValue(value->GetIntegerValue(), eCSSUnit_Integer);
    }
  }
  else if (aData->mSID == eStyleStruct_Color) {
    if (aData->mColorData->mColor.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::color);
      nscolor color;
      if (value && value->GetColorValue(color)) {
        aData->mColorData->mColor.SetColorValue(color);
      }
    }
  }
  else if (aData->mSID == eStyleStruct_TextReset) {
    
    
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::color);
    nscolor color;
    if (value && value->GetColorValue(color)) {
      nsCSSValue& decoration = aData->mTextData->mDecoration;
      PRInt32 newValue = NS_STYLE_TEXT_DECORATION_OVERRIDE_ALL;
      if (decoration.GetUnit() == eCSSUnit_Enumerated) {
        newValue |= decoration.GetIntValue();
      }
      decoration.SetIntValue(newValue, eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLFontElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::face },
    { &nsGkAtoms::pointSize },
    { &nsGkAtoms::size },
    { &nsGkAtoms::fontWeight },
    { &nsGkAtoms::color },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLFontElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
