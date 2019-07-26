




#include "mozilla/dom/HTMLFrameElement.h"
#include "mozilla/dom/HTMLFrameElementBinding.h"
#include "mozilla/Util.h"

class nsIDOMDocument;

NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Frame)

namespace mozilla {
namespace dom {

HTMLFrameElement::HTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                   FromParser aFromParser)
  : nsGenericHTMLFrameElement(aNodeInfo, aFromParser)
{
}

HTMLFrameElement::~HTMLFrameElement()
{
}


NS_IMPL_ISUPPORTS_INHERITED1(HTMLFrameElement, nsGenericHTMLFrameElement,
                             nsIDOMHTMLFrameElement)

NS_IMPL_ELEMENT_CLONE(HTMLFrameElement)


NS_IMPL_STRING_ATTR(HTMLFrameElement, FrameBorder, frameborder)
NS_IMPL_URI_ATTR(HTMLFrameElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(HTMLFrameElement, MarginHeight, marginheight)
NS_IMPL_STRING_ATTR(HTMLFrameElement, MarginWidth, marginwidth)
NS_IMPL_STRING_ATTR(HTMLFrameElement, Name, name)
NS_IMPL_BOOL_ATTR(HTMLFrameElement, NoResize, noresize)
NS_IMPL_STRING_ATTR(HTMLFrameElement, Scrolling, scrolling)
NS_IMPL_URI_ATTR(HTMLFrameElement, Src, src)


NS_IMETHODIMP
HTMLFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  return nsGenericHTMLFrameElement::GetContentDocument(aContentDocument);
}

NS_IMETHODIMP
HTMLFrameElement::GetContentWindow(nsIDOMWindow** aContentWindow)
{
  return nsGenericHTMLFrameElement::GetContentWindow(aContentWindow);
}

bool
HTMLFrameElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::bordercolor) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::frameborder) {
      return ParseFrameborderValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::marginwidth) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::marginheight) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::scrolling) {
      return ParseScrollingValue(aValue, aResult);
    }
  }

  return nsGenericHTMLFrameElement::ParseAttribute(aNamespaceID, aAttribute,
                                                   aValue, aResult);
}

void
HTMLFrameElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData)
{
  nsGenericHTMLElement::MapScrollingAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLFrameElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sScrollingAttributeMap,
    sCommonAttributeMap,
  };
  
  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
HTMLFrameElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

JSObject*
HTMLFrameElement::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return HTMLFrameElementBinding::Wrap(aCx, aScope, this);
}

} 
} 
