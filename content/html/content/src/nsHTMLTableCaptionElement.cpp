



































#include "nsIDOMHTMLTableCaptionElem.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

class nsHTMLTableCaptionElement :  public nsGenericHTMLElement,
                                   public nsIDOMHTMLTableCaptionElement
{
public:
  nsHTMLTableCaptionElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLTableCaptionElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLECAPTIONELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableCaption)


nsHTMLTableCaptionElement::nsHTMLTableCaptionElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTableCaptionElement::~nsHTMLTableCaptionElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableCaptionElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLTableCaptionElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLTableCaptionElement,
                                     nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLTableCaptionElement,
                                nsIDOMHTMLTableCaptionElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableCaptionElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLTableCaptionElement)


NS_IMPL_STRING_ATTR(nsHTMLTableCaptionElement, Align, align)


static const nsAttrValue::EnumTable kCaptionAlignTable[] = {
  { "left",  NS_SIDE_LEFT },
  { "right", NS_SIDE_RIGHT },
  { "top",   NS_SIDE_TOP},
  { "bottom",NS_SIDE_BOTTOM},
  { 0 }
};

PRBool
nsHTMLTableCaptionElement::ParseAttribute(PRInt32 aNamespaceID,
                                          nsIAtom* aAttribute,
                                          const nsAString& aValue,
                                          nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::align && aNamespaceID == kNameSpaceID_None) {
    return aResult.ParseEnumValue(aValue, kCaptionAlignTable);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSID == eStyleStruct_TableBorder) {
    if (aData->mTableData->mCaptionSide.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTableData->mCaptionSide.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableCaptionElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}



nsMapRuleToAttributesFunc
nsHTMLTableCaptionElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
