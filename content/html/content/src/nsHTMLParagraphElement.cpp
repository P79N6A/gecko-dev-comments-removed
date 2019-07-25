



































#include "nsIDOMHTMLParagraphElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"




class nsHTMLParagraphElement : public nsGenericHTMLElement,
                               public nsIDOMHTMLParagraphElement
{
public:
  nsHTMLParagraphElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLParagraphElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLPARAGRAPHELEMENT

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Paragraph)


nsHTMLParagraphElement::nsHTMLParagraphElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLParagraphElement::~nsHTMLParagraphElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLParagraphElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLParagraphElement, nsGenericElement)

DOMCI_NODE_DATA(HTMLParagraphElement, nsHTMLParagraphElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLParagraphElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLParagraphElement,
                                   nsIDOMHTMLParagraphElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLParagraphElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLParagraphElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLParagraphElement)


NS_IMPL_STRING_ATTR(nsHTMLParagraphElement, Align, align)


bool
nsHTMLParagraphElement::ParseAttribute(PRInt32 aNamespaceID,
                                       nsIAtom* aAttribute,
                                       const nsAString& aValue,
                                       nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::align && aNamespaceID == kNameSpaceID_None) {
    return ParseDivAlignValue(aValue, aResult);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  nsGenericHTMLElement::MapDivAlignAttributeInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
nsHTMLParagraphElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry* const map[] = {
    sDivAlignAttributeMap,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}


nsMapRuleToAttributesFunc
nsHTMLParagraphElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
