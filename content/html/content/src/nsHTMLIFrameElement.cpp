



































#include "nsIDOMHTMLIFrameElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMDocument.h"
#ifdef MOZ_SVG
#include "nsIDOMGetSVGDocument.h"
#include "nsIDOMSVGDocument.h"
#endif
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsMappedAttributes.h"
#include "nsDOMError.h"
#include "nsRuleData.h"
#include "nsStyleConsts.h"

class nsHTMLIFrameElement : public nsGenericHTMLFrameElement,
                            public nsIDOMHTMLIFrameElement
#ifdef MOZ_SVG
                            , public nsIDOMGetSVGDocument
#endif
{
public:
  nsHTMLIFrameElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLIFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFrameElement::)

  
  NS_DECL_NSIDOMHTMLIFRAMEELEMENT

#ifdef MOZ_SVG
  
  NS_DECL_NSIDOMGETSVGDOCUMENT
#endif

  
  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(IFrame)


nsHTMLIFrameElement::nsHTMLIFrameElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLFrameElement(aNodeInfo)
{
}

nsHTMLIFrameElement::~nsHTMLIFrameElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLIFrameElement,nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLIFrameElement,nsGenericElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLIFrameElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(nsHTMLIFrameElement)
    NS_INTERFACE_TABLE_ENTRY(nsHTMLIFrameElement, nsIDOMHTMLIFrameElement)
#ifdef MOZ_SVG
    NS_INTERFACE_TABLE_ENTRY(nsHTMLIFrameElement, nsIDOMGetSVGDocument)
#endif
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

#ifdef MOZ_SVG
NS_IMETHODIMP
nsHTMLIFrameElement::GetSVGDocument(nsIDOMDocument **aResult)
{
  return GetContentDocument(aResult);
}
#endif

PRBool
nsHTMLIFrameElement::ParseAttribute(PRInt32 aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::marginwidth) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::marginheight) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
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
        if (aData->mMarginData->mBorderWidth.mLeft.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderWidth.mLeft.SetFloatValue(0.0f, eCSSUnit_Pixel);
        if (aData->mMarginData->mBorderWidth.mRight.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderWidth.mRight.SetFloatValue(0.0f, eCSSUnit_Pixel);
        if (aData->mMarginData->mBorderWidth.mTop.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderWidth.mTop.SetFloatValue(0.0f, eCSSUnit_Pixel);
        if (aData->mMarginData->mBorderWidth.mBottom.GetUnit() == eCSSUnit_Null)
          aData->mMarginData->mBorderWidth.mBottom.SetFloatValue(0.0f, eCSSUnit_Pixel);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger)
        aData->mPositionData->mWidth.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        aData->mPositionData->mWidth.SetPercentValue(value->GetPercentValue());
    }

    
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        aData->mPositionData->mHeight.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        aData->mPositionData->mHeight.SetPercentValue(value->GetPercentValue());
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

