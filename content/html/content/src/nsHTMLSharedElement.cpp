



































#include "nsIDOMHTMLIsIndexElement.h"
#include "nsIDOMHTMLParamElement.h"
#include "nsIDOMHTMLBaseElement.h"
#include "nsIDOMHTMLDirectoryElement.h"
#include "nsIDOMHTMLMenuElement.h"
#include "nsIDOMHTMLQuoteElement.h"
#include "nsIDOMHTMLBaseFontElement.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsRuleData.h"
#include "nsMappedAttributes.h"
#include "nsStyleContext.h"


extern nsAttrValue::EnumTable kListTypeTable[];

class nsHTMLSharedElement : public nsGenericHTMLElement,
                            public nsIDOMHTMLIsIndexElement,
                            public nsIDOMHTMLParamElement,
                            public nsIDOMHTMLBaseElement,
                            public nsIDOMHTMLDirectoryElement,
                            public nsIDOMHTMLMenuElement,
                            public nsIDOMHTMLQuoteElement,
                            public nsIDOMHTMLBaseFontElement
{
public:
  nsHTMLSharedElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLSharedElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLISINDEXELEMENT

  
  NS_DECL_NSIDOMHTMLPARAMELEMENT

  
  NS_DECL_NSIDOMHTMLBASEELEMENT

  
  NS_DECL_NSIDOMHTMLDIRECTORYELEMENT

  
  

  
  NS_DECL_NSIDOMHTMLQUOTEELEMENT

  
  NS_DECL_NSIDOMHTMLBASEFONTELEMENT

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Shared)


nsHTMLSharedElement::nsHTMLSharedElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLSharedElement::~nsHTMLSharedElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLSharedElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLSharedElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_TABLE_AMBIGOUS_HEAD(nsHTMLSharedElement,
                                              nsGenericHTMLElement,
                                              nsIDOMHTMLParamElement)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLParamElement, param)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLIsIndexElement, isindex)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLBaseElement, base)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLDirectoryElement, dir)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLMenuElement, menu)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLQuoteElement, q)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLQuoteElement, blockquote)
  NS_INTERFACE_MAP_ENTRY_IF_TAG(nsIDOMHTMLBaseFontElement, basefont)

  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLParamElement, param)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLWBRElement, wbr)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLIsIndexElement, isindex)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLBaseElement, base)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLSpacerElement, spacer)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLDirectoryElement, dir)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLMenuElement, menu)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLQuoteElement, q)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLQuoteElement, blockquote)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO_IF_TAG(HTMLBaseFontElement, basefont)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLSharedElement)


NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Type, type)
NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Value, value)
NS_IMPL_STRING_ATTR(nsHTMLSharedElement, ValueType, valuetype)


NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Prompt, prompt)


NS_IMPL_BOOL_ATTR(nsHTMLSharedElement, Compact, compact)





NS_IMPL_URI_ATTR(nsHTMLSharedElement, Cite, cite)


NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Color, color)
NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Face, face)
NS_IMPL_INT_ATTR(nsHTMLSharedElement, Size, size)

NS_IMETHODIMP
nsHTMLSharedElement::GetForm(nsIDOMHTMLFormElement** aForm)
{
  *aForm = FindForm().get();

  return NS_OK;
}


NS_IMPL_URI_ATTR(nsHTMLSharedElement, Href, href)
NS_IMPL_STRING_ATTR(nsHTMLSharedElement, Target, target)

PRBool
nsHTMLSharedElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (mNodeInfo->Equals(nsGkAtoms::spacer)) {
      if (aAttribute == nsGkAtoms::size) {
        return aResult.ParseIntWithBounds(aValue, 0);
      }
      if (aAttribute == nsGkAtoms::align) {
        return ParseAlignValue(aValue, aResult);
      }
      if (aAttribute == nsGkAtoms::width ||
          aAttribute == nsGkAtoms::height) {
        return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
      }
    }
    else if (mNodeInfo->Equals(nsGkAtoms::dir) ||
             mNodeInfo->Equals(nsGkAtoms::menu)) {
      if (aAttribute == nsGkAtoms::type) {
        return aResult.ParseEnumValue(aValue, kListTypeTable);
      }
      if (aAttribute == nsGkAtoms::start) {
        return aResult.ParseIntWithBounds(aValue, 1);
      }
    }
    else if (mNodeInfo->Equals(nsGkAtoms::basefont)) {
      if (aAttribute == nsGkAtoms::size) {
        return aResult.ParseIntValue(aValue);
      }
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}



static void
SpacerMapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                            nsRuleData* aData)
{
  nsGenericHTMLElement::MapImageMarginAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageSizeAttributesInto(aAttributes, aData);

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    const nsStyleDisplay* display = aData->mStyleContext->GetStyleDisplay();

    PRBool typeIsBlock = (display->mDisplay == NS_STYLE_DISPLAY_BLOCK);

    if (typeIsBlock) {
      
      if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
        if (value && value->Type() == nsAttrValue::eInteger) {
          aData->mPositionData->
            mWidth.SetFloatValue((float)value->GetIntegerValue(),
                                 eCSSUnit_Pixel);
        } else if (value && value->Type() == nsAttrValue::ePercent) {
          aData->mPositionData->
            mWidth.SetPercentValue(value->GetPercentValue());
        }
      }

      
      if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
        if (value && value->Type() == nsAttrValue::eInteger) {
          aData->mPositionData->
            mHeight.SetFloatValue((float)value->GetIntegerValue(),
                                  eCSSUnit_Pixel);
        } else if (value && value->Type() == nsAttrValue::ePercent) {
          aData->mPositionData->
            mHeight.SetPercentValue(value->GetPercentValue());
        }
      }
    } else {
      
      if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::size);
        if (value && value->Type() == nsAttrValue::eInteger)
          aData->mPositionData->
            mWidth.SetFloatValue((float)value->GetIntegerValue(),
                                 eCSSUnit_Pixel);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Display)) {
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
    if (value && value->Type() == nsAttrValue::eEnum) {
      PRInt32 align = value->GetEnumValue();
      if (aData->mDisplayData->mFloat.GetUnit() == eCSSUnit_Null) {
        if (align == NS_STYLE_TEXT_ALIGN_LEFT)
          aData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_LEFT,
                                                  eCSSUnit_Enumerated);
        else if (align == NS_STYLE_TEXT_ALIGN_RIGHT)
          aData->mDisplayData->mFloat.SetIntValue(NS_STYLE_FLOAT_RIGHT,
                                                  eCSSUnit_Enumerated);
      }
    }

    if (aData->mDisplayData->mDisplay.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
      if (value && value->Type() == nsAttrValue::eString) {
        nsAutoString tmp(value->GetStringValue());
        if (tmp.LowerCaseEqualsLiteral("line") ||
            tmp.LowerCaseEqualsLiteral("vert") ||
            tmp.LowerCaseEqualsLiteral("vertical") ||
            tmp.LowerCaseEqualsLiteral("block")) {
          
          
          aData->mDisplayData->mDisplay.SetIntValue(NS_STYLE_DISPLAY_BLOCK,
                                                    eCSSUnit_Enumerated);
        }
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

static void
DirectoryMenuMapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                               nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(List)) {
    if (aData->mListData->mType.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
      if (value) {
        if (value->Type() == nsAttrValue::eEnum) {
          aData->mListData->mType.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
        } else {
          aData->mListData->mType.SetIntValue(NS_STYLE_LIST_STYLE_DISC, eCSSUnit_Enumerated);
        }
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLSharedElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  if (mNodeInfo->Equals(nsGkAtoms::spacer)) {
    static const MappedAttributeEntry attributes[] = {
      
      { &nsGkAtoms::usemap },
      { &nsGkAtoms::ismap },
      { &nsGkAtoms::align },
      { nsnull }
    };

    static const MappedAttributeEntry* const map[] = {
      attributes,
      sCommonAttributeMap,
      sImageMarginSizeAttributeMap,
      sImageBorderAttributeMap,
    };

    return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
  }

  if (mNodeInfo->Equals(nsGkAtoms::dir)) {
    static const MappedAttributeEntry attributes[] = {
      { &nsGkAtoms::type },
      
      { nsnull} 
    };
  
    static const MappedAttributeEntry* const map[] = {
      attributes,
      sCommonAttributeMap,
    };

    return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
  }

  return nsGenericHTMLElement::IsAttributeMapped(aAttribute);
}

nsMapRuleToAttributesFunc
nsHTMLSharedElement::GetAttributeMappingFunction() const
{
  if (mNodeInfo->Equals(nsGkAtoms::spacer)) {
    return &SpacerMapAttributesIntoRule;
  }
  else if (mNodeInfo->Equals(nsGkAtoms::dir) ||
           mNodeInfo->Equals(nsGkAtoms::menu)) {
    return &DirectoryMenuMapAttributesIntoRule;
  }

  return nsGenericHTMLElement::GetAttributeMappingFunction();
}
