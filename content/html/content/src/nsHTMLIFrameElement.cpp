



































#include "nsIDOMHTMLIFrameElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMDocument.h"
#include "nsIDOMGetSVGDocument.h"
#include "nsIDOMSVGDocument.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsMappedAttributes.h"
#include "nsDOMError.h"
#include "nsRuleData.h"
#include "nsStyleConsts.h"

using namespace mozilla::dom;

class nsHTMLIFrameElement : public nsGenericHTMLFrameElement
                          , public nsIDOMHTMLIFrameElement
                          , public nsIDOMGetSVGDocument
{
public:
  nsHTMLIFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                      mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~nsHTMLIFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFrameElement::)

  
  NS_DECL_NSIDOMHTMLIFRAMEELEMENT

  
  NS_DECL_NSIDOMGETSVGDOCUMENT

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(IFrame)


nsHTMLIFrameElement::nsHTMLIFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                         FromParser aFromParser)
  : nsGenericHTMLFrameElement(aNodeInfo, aFromParser)
{
}

nsHTMLIFrameElement::~nsHTMLIFrameElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLIFrameElement,nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLIFrameElement,nsGenericElement)

DOMCI_NODE_DATA(HTMLIFrameElement, nsHTMLIFrameElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLIFrameElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(nsHTMLIFrameElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLIFrameElement, nsIDOMHTMLIFrameElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLIFrameElement, nsIDOMGetSVGDocument)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLIFrameElement,
                                               nsGenericHTMLFrameElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLIFrameElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLIFrameElement)


NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, FrameBorder, frameborder)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, Height, height)
NS_IMPL_URI_ATTR(nsHTMLIFrameElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, MarginHeight, marginheight)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, MarginWidth, marginwidth)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, Name, name)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, Scrolling, scrolling)
NS_IMPL_URI_ATTR(nsHTMLIFrameElement, Src, src)
NS_IMPL_STRING_ATTR(nsHTMLIFrameElement, Width, width)

NS_IMETHODIMP
nsHTMLIFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  return nsGenericHTMLFrameElement::GetContentDocument(aContentDocument);
}

NS_IMETHODIMP
nsHTMLIFrameElement::GetSVGDocument(nsIDOMDocument **aResult)
{
  return GetContentDocument(aResult);
}

PRBool
nsHTMLIFrameElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::marginwidth) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::marginheight) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::frameborder) {
      return ParseFrameborderValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::scrolling) {
      return ParseScrollingValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseAlignValue(aValue, aResult);
    }
  }

  return nsGenericHTMLFrameElement::ParseAttribute(aNamespaceID, aAttribute,
                                                   aValue, aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    
    
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::frameborder);
    if (value && value->Type() == nsAttrValue::eEnum) {
      PRInt32 frameborder = value->GetEnumValue();
      if (NS_STYLE_FRAME_0 == frameborder ||
          NS_STYLE_FRAME_NO == frameborder ||
          NS_STYLE_FRAME_OFF == frameborder) {
        nsCSSValue* borderLeftWidth = aData->ValueForBorderLeftWidthValue();
        if (borderLeftWidth->GetUnit() == eCSSUnit_Null)
          borderLeftWidth->SetFloatValue(0.0f, eCSSUnit_Pixel);
        nsCSSValue* borderRightWidth = aData->ValueForBorderRightWidthValue();
        if (borderRightWidth->GetUnit() == eCSSUnit_Null)
          borderRightWidth->SetFloatValue(0.0f, eCSSUnit_Pixel);
        nsCSSValue* borderTopWidth = aData->ValueForBorderTopWidth();
        if (borderTopWidth->GetUnit() == eCSSUnit_Null)
          borderTopWidth->SetFloatValue(0.0f, eCSSUnit_Pixel);
        nsCSSValue* borderBottomWidth = aData->ValueForBorderBottomWidth();
        if (borderBottomWidth->GetUnit() == eCSSUnit_Null)
          borderBottomWidth->SetFloatValue(0.0f, eCSSUnit_Pixel);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger)
        width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        width->SetPercentValue(value->GetPercentValue());
    }

    
    nsCSSValue* height = aData->ValueForHeight();
    if (height->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        height->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        height->SetPercentValue(value->GetPercentValue());
    }
  }

  nsGenericHTMLElement::MapScrollingAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapImageAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLIFrameElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { &nsGkAtoms::frameborder },
    { nsnull },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sScrollingAttributeMap,
    sImageAlignAttributeMap,
    sCommonAttributeMap,
  };
  
  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}



nsMapRuleToAttributesFunc
nsHTMLIFrameElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

