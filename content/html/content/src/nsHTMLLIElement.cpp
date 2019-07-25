



































#include "nsIDOMHTMLLIElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

class nsHTMLLIElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLLIElement
{
public:
  nsHTMLLIElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLLIElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLLIELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(LI)


nsHTMLLIElement::nsHTMLLIElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLLIElement::~nsHTMLLIElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLLIElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLLIElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLLIElement, nsHTMLLIElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLLIElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLLIElement, nsIDOMHTMLLIElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLLIElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLLIElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLLIElement)


NS_IMPL_STRING_ATTR(nsHTMLLIElement, Type, type)
NS_IMPL_INT_ATTR(nsHTMLLIElement, Value, value)


static const nsAttrValue::EnumTable kUnorderedListItemTypeTable[] = {
  { "disc", NS_STYLE_LIST_STYLE_DISC },
  { "circle", NS_STYLE_LIST_STYLE_CIRCLE },
  { "round", NS_STYLE_LIST_STYLE_CIRCLE },
  { "square", NS_STYLE_LIST_STYLE_SQUARE },
  { 0 }
};


static const nsAttrValue::EnumTable kOrderedListItemTypeTable[] = {
  { "A", NS_STYLE_LIST_STYLE_UPPER_ALPHA },
  { "a", NS_STYLE_LIST_STYLE_LOWER_ALPHA },
  { "I", NS_STYLE_LIST_STYLE_UPPER_ROMAN },
  { "i", NS_STYLE_LIST_STYLE_LOWER_ROMAN },
  { "1", NS_STYLE_LIST_STYLE_DECIMAL },
  { 0 }
};

PRBool
nsHTMLLIElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::type) {
      return aResult.ParseEnumValue(aValue, kOrderedListItemTypeTable,
                                    PR_TRUE) ||
             aResult.ParseEnumValue(aValue, kUnorderedListItemTypeTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::value) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(List)) {
    nsCSSValue* listStyleType = aData->ValueForListStyleType();
    if (listStyleType->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::type);
      if (value && value->Type() == nsAttrValue::eEnum)
        listStyleType->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLLIElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::type },
    { nsnull },
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}



nsMapRuleToAttributesFunc
nsHTMLLIElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
