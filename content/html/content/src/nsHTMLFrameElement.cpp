




































#include "mozilla/Util.h"

#include "nsIDOMHTMLFrameElement.h"
#include "nsGenericHTMLFrameElement.h"
#include "nsGkAtoms.h"
#include "nsIDocument.h"
#include "nsIDOMDocument.h"
#include "nsDOMError.h"

using namespace mozilla;
using namespace mozilla::dom;

class nsHTMLFrameElement : public nsGenericHTMLFrameElement,
                           public nsIDOMHTMLFrameElement
{
public:
  nsHTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                     mozilla::dom::FromParser aFromParser = mozilla::dom::NOT_FROM_PARSER);
  virtual ~nsHTMLFrameElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLFrameElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLFrameElement::)

  
  NS_DECL_NSIDOMHTMLFRAMEELEMENT

  
  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT_CHECK_PARSER(Frame)


nsHTMLFrameElement::nsHTMLFrameElement(already_AddRefed<nsINodeInfo> aNodeInfo,
                                       FromParser aFromParser)
  : nsGenericHTMLFrameElement(aNodeInfo, aFromParser)
{
}

nsHTMLFrameElement::~nsHTMLFrameElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLFrameElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLFrameElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLFrameElement, nsHTMLFrameElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLFrameElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLFrameElement, nsIDOMHTMLFrameElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLFrameElement,
                                               nsGenericHTMLFrameElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLFrameElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLFrameElement)


NS_IMPL_STRING_ATTR(nsHTMLFrameElement, FrameBorder, frameborder)
NS_IMPL_URI_ATTR(nsHTMLFrameElement, LongDesc, longdesc)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, MarginHeight, marginheight)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, MarginWidth, marginwidth)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, Name, name)
NS_IMPL_BOOL_ATTR(nsHTMLFrameElement, NoResize, noresize)
NS_IMPL_STRING_ATTR(nsHTMLFrameElement, Scrolling, scrolling)
NS_IMPL_URI_ATTR(nsHTMLFrameElement, Src, src)


NS_IMETHODIMP
nsHTMLFrameElement::GetContentDocument(nsIDOMDocument** aContentDocument)
{
  return nsGenericHTMLFrameElement::GetContentDocument(aContentDocument);
}

NS_IMETHODIMP
nsHTMLFrameElement::GetContentWindow(nsIDOMWindow** aContentWindow)
{
  return nsGenericHTMLFrameElement::GetContentWindow(aContentWindow);
}

bool
nsHTMLFrameElement::ParseAttribute(PRInt32 aNamespaceID,
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

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  nsGenericHTMLElement::MapScrollingAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
nsHTMLFrameElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sScrollingAttributeMap,
    sCommonAttributeMap,
  };
  
  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
nsHTMLFrameElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
