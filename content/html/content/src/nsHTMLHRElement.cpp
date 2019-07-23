



































#include "nsIDOMHTMLHRElement.h"
#include "nsIDOMNSHTMLHRElement.h"
#include "nsIDOMEventReceiver.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

class nsHTMLHRElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLHRElement,
                        public nsIDOMNSHTMLHRElement
{
public:
  nsHTMLHRElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLHRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLHRELEMENT

  
  NS_DECL_NSIDOMNSHTMLHRELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(HR)


nsHTMLHRElement::nsHTMLHRElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLHRElement::~nsHTMLHRElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLHRElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLHRElement, nsGenericElement) 



NS_HTML_CONTENT_INTERFACE_MAP_BEGIN(nsHTMLHRElement, nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLHRElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMNSHTMLHRElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLHRElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


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
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::size) {
      return aResult.ParseIntWithBounds(aValue, 1, 1000);
    }
    if (aAttribute == nsGkAtoms::align) {
      return aResult.ParseEnumValue(aValue, kAlignTable);
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
  PRBool noshade = PR_FALSE;

  const nsAttrValue* colorValue = aAttributes->GetAttr(nsGkAtoms::color);
  nscolor color;
  PRBool colorIsSet = colorValue && colorValue->GetColorValue(color);

  if (aData->mSID == eStyleStruct_Position ||
      aData->mSID == eStyleStruct_Border) {
    if (colorIsSet) {
      noshade = PR_TRUE;
    } else {
      noshade = !!aAttributes->GetAttr(nsGkAtoms::noshade);
    }
  }

  if (aData->mSID == eStyleStruct_Margin) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
    if (value && value->Type() == nsAttrValue::eEnum) {
      
      nsCSSRect& margin = aData->mMarginData->mMargin;
      switch (value->GetEnumValue()) {
      case NS_STYLE_TEXT_ALIGN_LEFT:
        if (margin.mLeft.GetUnit() == eCSSUnit_Null)
          margin.mLeft.SetFloatValue(0.0f, eCSSUnit_Pixel);
        if (margin.mRight.GetUnit() == eCSSUnit_Null)
          margin.mRight.SetAutoValue();
        break;
      case NS_STYLE_TEXT_ALIGN_RIGHT:
        if (margin.mLeft.GetUnit() == eCSSUnit_Null)
          margin.mLeft.SetAutoValue();
        if (margin.mRight.GetUnit() == eCSSUnit_Null)
          margin.mRight.SetFloatValue(0.0f, eCSSUnit_Pixel);
        break;
      case NS_STYLE_TEXT_ALIGN_CENTER:
        if (margin.mLeft.GetUnit() == eCSSUnit_Null)
          margin.mLeft.SetAutoValue();
        if (margin.mRight.GetUnit() == eCSSUnit_Null)
          margin.mRight.SetAutoValue();
        break;
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Position) {
    
    if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger) {
        aData->mPositionData->mWidth.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      } else if (value && value->Type() == nsAttrValue::ePercent) {
        aData->mPositionData->mWidth.SetPercentValue(value->GetPercentValue());
      }
    }

    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      
      if (noshade) {
        
        aData->mPositionData->mHeight.SetAutoValue();
      } else {
        
        
        
        
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::size);
        if (value && value->Type() == nsAttrValue::eInteger) {
          aData->mPositionData->mHeight.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
        } 
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Border && noshade) { 
    
    
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
    nsCSSRect& borderWidth = aData->mMarginData->mBorderWidth;
    if (borderWidth.mTop.GetUnit() == eCSSUnit_Null) {
      borderWidth.mTop.SetFloatValue(sizePerSide, eCSSUnit_Pixel);
    }
    if (allSides) {
      if (borderWidth.mRight.GetUnit() == eCSSUnit_Null) {
        borderWidth.mRight.SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
      if (borderWidth.mBottom.GetUnit() == eCSSUnit_Null) {
        borderWidth.mBottom.SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
      if (borderWidth.mLeft.GetUnit() == eCSSUnit_Null) {
        borderWidth.mLeft.SetFloatValue(sizePerSide, eCSSUnit_Pixel);
      }
    }

    nsCSSRect& borderStyle = aData->mMarginData->mBorderStyle;
    if (borderStyle.mTop.GetUnit() == eCSSUnit_Null) {
      borderStyle.mTop.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                   eCSSUnit_Enumerated);
    }
    if (allSides) {
      if (borderStyle.mRight.GetUnit() == eCSSUnit_Null) {
        borderStyle.mRight.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                       eCSSUnit_Enumerated);
      }
      if (borderStyle.mBottom.GetUnit() == eCSSUnit_Null) {
        borderStyle.mBottom.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                        eCSSUnit_Enumerated);
      }
      if (borderStyle.mLeft.GetUnit() == eCSSUnit_Null) {
        borderStyle.mLeft.SetIntValue(NS_STYLE_BORDER_STYLE_SOLID,
                                      eCSSUnit_Enumerated);
      }

      
      
      nsCSSRect& borderRadius = aData->mMarginData->mBorderRadius;
      if (borderRadius.mTop.GetUnit() == eCSSUnit_Null) {
        borderRadius.mTop.SetPercentValue(1.0f);
      }
      if (borderRadius.mRight.GetUnit() == eCSSUnit_Null) {
        borderRadius.mRight.SetPercentValue(1.0f);
      }
      if (borderRadius.mBottom.GetUnit() == eCSSUnit_Null) {
        borderRadius.mBottom.SetPercentValue(1.0f);
      }
      if (borderRadius.mLeft.GetUnit() == eCSSUnit_Null) {
        borderRadius.mLeft.SetPercentValue(1.0f);
      }
    }
  }
  else if (aData->mSID == eStyleStruct_Color) {
    
    
    if (colorIsSet &&
        aData->mColorData->mColor.GetUnit() == eCSSUnit_Null) {
      aData->mColorData->mColor.SetColorValue(color);
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
