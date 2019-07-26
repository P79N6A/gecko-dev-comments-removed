




#include "mozilla/Util.h"

#include "mozilla/dom/HTMLIFrameElement.h"
#include "mozilla/dom/HTMLIFrameElementBinding.h"
#include "nsIDOMSVGDocument.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsError.h"
#include "nsRuleData.h"
#include "nsStyleConsts.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(IFrame)

DOMCI_NODE_DATA(HTMLIFrameElement, mozilla::dom::HTMLIFrameElement)

namespace mozilla {
namespace dom {

HTMLIFrameElement::HTMLIFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                     FromParser aFromParser)
  : nsGenericHTMLFrameElement(aNodeInfo, aFromParser)
{
  SetIsDOMBinding();
}

HTMLIFrameElement::~HTMLIFrameElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLIFrameElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLIFrameElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLIFrameElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_BEGIN(HTMLIFrameElement)
    NS_INTERFACE_TABLE_ENTRY(HTMLIFrameElement, nsIDOMHTMLIFrameElement)
    NS_INTERFACE_TABLE_ENTRY(HTMLIFrameElement, nsIDOMGetSVGDocument)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLIFrameElement,
                                               nsGenericHTMLFrameElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLIFrameElement)

NS_IMPL_ELEMENT_CLONE(HTMLIFrameElement)

NS_IMPL_STRING_ATTR(HTMLIFrameElement, Align, align)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, FrameBorder, frameborder)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, Height, height)
NS_IMPL_URI_ATTR(HTMLIFrameElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, MarginHeight, marginheight)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, MarginWidth, marginwidth)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, Name, name)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, Scrolling, scrolling)
NS_IMPL_URI_ATTR(HTMLIFrameElement, Src, src)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, Width, width)
NS_IMPL_BOOL_ATTR(HTMLIFrameElement, AllowFullscreen, allowfullscreen)
NS_IMPL_STRING_ATTR(HTMLIFrameElement, Sandbox, sandbox)

void
HTMLIFrameElement::GetItemValueText(nsAString& aValue)
{
  GetSrc(aValue);
}

void
HTMLIFrameElement::SetItemValueText(const nsAString& aValue)
{
  SetSrc(aValue);
}

NS_IMETHODIMP
HTMLIFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  return nsGenericHTMLFrameElement::GetContentDocument(aContentDocument);
}

NS_IMETHODIMP
HTMLIFrameElement::GetContentWindow(nsIDOMWindow** aContentWindow)
{
  return nsGenericHTMLFrameElement::GetContentWindow(aContentWindow);
}

NS_IMETHODIMP
HTMLIFrameElement::GetSVGDocument(nsIDOMDocument **aResult)
{
  return GetContentDocument(aResult);
}

bool
HTMLIFrameElement::ParseAttribute(int32_t aNamespaceID,
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
      int32_t frameborder = value->GetEnumValue();
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

NS_IMETHODIMP_(bool)
HTMLIFrameElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { &nsGkAtoms::frameborder },
    { nullptr },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sScrollingAttributeMap,
    sImageAlignAttributeMap,
    sCommonAttributeMap,
  };
  
  return FindAttributeDependence(aAttribute, map);
}



nsMapRuleToAttributesFunc
HTMLIFrameElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

nsresult
HTMLIFrameElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValue* aValue,
                                bool aNotify)
{
  if (aName == nsGkAtoms::sandbox && aNameSpaceID == kNameSpaceID_None) {
    
    
    if (mFrameLoader) {
      nsCOMPtr<nsIDocShell> docshell = mFrameLoader->GetExistingDocShell();

      if (docshell) {
        uint32_t newFlags = 0;
        
        
        if (aValue) {
          nsAutoString strValue;
          aValue->ToString(strValue);
          newFlags = nsContentUtils::ParseSandboxAttributeToFlags(
            strValue);
        }   
        docshell->SetSandboxFlags(newFlags);
      }
    }
  }
  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                            aNotify);
}

uint32_t
HTMLIFrameElement::GetSandboxFlags()
{
  nsAutoString sandboxAttr;

  if (GetAttr(kNameSpaceID_None, nsGkAtoms::sandbox, sandboxAttr)) {
    return nsContentUtils::ParseSandboxAttributeToFlags(sandboxAttr);
  }

  
  return 0;
}

JSObject*
HTMLIFrameElement::WrapNode(JSContext* aCx, JSObject* aScope)
{
  return HTMLIFrameElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
