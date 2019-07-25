




































#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLHRElement.h"

#include "nsIDOMEventTarget.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsCSSProps.h"

class nsHTMLHRElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLHRElement
{
public:
  nsHTMLHRElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLHRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLHRELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(HR)


nsHTMLHRElement::nsHTMLHRElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLHRElement::~nsHTMLHRElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLHRElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLHRElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLHRElement, nsHTMLHRElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLHRElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLHRElement,
                                   nsIDOMHTMLHRElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLHRElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLHRElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLHRElement)


NS_IMPL_STRING_ATTR(nsHTMLHRElement, Align, align)
NS_IMPL_BOOL_ATTR(nsHTMLHRElement, NoShade, noshade)
NS_IMPL_STRING_ATTR(nsHTMLHRElement, Size, size)
NS_IMPL_STRING_ATTR(nsHTMLHRElement, Width, width)
NS_IMPL_STRING_ATTR(nsHTMLHRElement, Color, color)

static const nsAttrValue::EnumTable kAlignTable[] = {
  { "left", NS_STYLE_TEXT_ALIGN_LEFT },
  { "right", NS_STYLE_TEXT_ALIGN_RIGHT },
  { "center", NS_STYLE_TEXT_ALIGN_CENTER },
  { 0 }
};

PRBool
nsHTMLHRElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParseIntWithBounds(aValue, 1, 1000);
    }
    if (aAttribute == nsGkAtoms::align) {
      return aResult.ParseEnumValue(aValue, kAlignTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::color) {
      return aResult.ParseColor(aValue);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  PRBool noshade = PR_FALSE;

  const nsAttrValue* colorValue = aAttributes->GetAttr(nsGkAtoms::color);
  nscolor color;
  PRBool colorIsSet = colorValue && colorValue->GetColorValue(color);

  if (aData->mSIDs & (NS_STYLE_INHERIT_BIT(Position) |
                      NS_STYLE_INHERIT_BIT(Border))) {
    if (colorIsSet) {
      noshade = PR_TRUE;
    } else {
      noshade = !!aAttributes->GetAttr(nsGkAtoms::noshade);
    }
  }

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Margin)) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
    if (value && value->Type() == nsAttrValue::eEnum) {
      
      nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
      nsCSSValue* marginRight = aData->ValueForMarginRightValue();
      switch (value->GetEnumValue()) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetFloatValue(0.0f, eCSSUnit_Pixel);
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetAutoValue();
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetAutoValue();
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetFloatValue(0.0f, eCSSUnit_Pixel);
        break;
      case NS_STYLE_TEXT_ALIGN_CENTER:
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetAutoValue();
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetAutoValue();
        break;
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger) {
        width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      } else if (value && value->Type() == nsAttrValue::ePercent) {
        width->SetPercentValue(value->GetPercentValue());
      }
    }

    nsCSSValue* height = aData->ValueForHeight();
    if (height->GetUnit() == eCSSUnit_Null) {
      
      if (noshade) {
        
        height->SetAutoValue();
      } else {
        
        
        
        
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::size);
        if (value && value->Type() == nsAttrValue::eInteger) {
          height->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
        } 
      }
    }
  }
  if ((aData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) && noshade) { 
    
    
    float sizePerSide;
    PRBool allSides = PR_TRUE;
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::size);
    if (value && value->Type() == nsAttrValue::eInteger) {
      sizePerSide = (float)value->GetIntegerValue() / 2.0f;
      if (sizePerSide < 1.0f) {
        
        
        
        sizePerSide = 1.0f;
        allSides = PR_FALSE;
      }
    } else {
      sizePerSide = 1.0f; 
    }
    nsCSSValue* borderTopWidth = aData->ValueForBorderTopWidth();
    if (borderTopWidth->GetUnit() == eCSSUnit_Null) {
      borderTopWidth->SetFloatValue(sizePerSide, eCSSUnit_Pixel);
    }
    if (allSides) {
      nsCSSValue* borderRightWidth = aData->ValueForBorderRightWidthValue();
      if (borderRightWidth->GetUnit() == eCSSUnit_Null) {
        borderRightWidth->SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
      nsCSSValue* borderBottomWidth = aData->ValueForBorderBottomWidth();
      if (borderBottomWidth->GetUnit() == eCSSUnit_Null) {
        borderBottomWidth->SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
      nsCSSValue* borderLeftWidth = aData->ValueForBorderLeftWidthValue();
      if (borderLeftWidth->GetUnit() == eCSSUnit_Null) {
        borderLeftWidth->SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
    }

    nsCSSValue* borderTopStyle = aData->ValueForBorderTopStyle();
    if (borderTopStyle->GetUnit() == eCSSUnit_Null) {
      borderTopStyle->SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                  eCSSUnit_Enumerated);
    }
    if (allSides) {
      nsCSSValue* borderRightStyle = aData->ValueForBorderRightStyleValue();
      if (borderRightStyle->GetUnit() == eCSSUnit_Null) {
        borderRightStyle->SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                      eCSSUnit_Enumerated);
      }
      nsCSSValue* borderBottomStyle = aData->ValueForBorderBottomStyle();
      if (borderBottomStyle->GetUnit() == eCSSUnit_Null) {
        borderBottomStyle->SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                       eCSSUnit_Enumerated);
      }
      nsCSSValue* borderLeftStyle = aData->ValueForBorderLeftStyleValue();
      if (borderLeftStyle->GetUnit() == eCSSUnit_Null) {
        borderLeftStyle->SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                     eCSSUnit_Enumerated);
      }

      
      
      
      
      for (const nsCSSProperty* props =
            nsCSSProps::SubpropertyEntryFor(eCSSProperty_border_radius);
           *props != eCSSProperty_UNKNOWN; ++props) {
        nsCSSValue* dimen = aData->ValueFor(*props);
        if (dimen->GetUnit() == eCSSUnit_Null) {
          dimen->SetFloatValue(10000.0f, eCSSUnit_Pixel);
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    
    
    nsCSSValue* colorValue = aData->ValueForColor();
    if (colorIsSet &&
        colorValue->GetUnit() == eCSSUnit_Null &&
        aData->mPresContext->UseDocumentColors()) {
      colorValue->SetColorValue(color);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLHRElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { &nsGkAtoms::width },
    { &nsGkAtoms::size },
    { &nsGkAtoms::color },
    { &nsGkAtoms::noshade },
    { nsnull },
  };
  
  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLHRElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
