



































#include "nsIDOMHTMLPreElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

class nsHTMLPreElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLPreElement
{
public:
  nsHTMLPreElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLPreElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_IMETHOD GetWidth(PRInt32* aWidth);
  NS_IMETHOD SetWidth(PRInt32 aWidth);

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Pre)


nsHTMLPreElement::nsHTMLPreElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLPreElement::~nsHTMLPreElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLPreElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLPreElement, nsGenericElement)


DOMCI_NODE_DATA(HTMLPreElement, nsHTMLPreElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLPreElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLPreElement, nsIDOMHTMLPreElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLPreElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLPreElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLPreElement)


NS_IMPL_INT_ATTR(nsHTMLPreElement, Width, width)


bool
nsHTMLPreElement::ParseAttribute(PRInt32 aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::cols) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::width) {
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
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (!value || value->Type() != nsAttrValue::eInteger) {
        
        value = aAttributes->GetAttr(nsGkAtoms::cols);
      }

      if (value && value->Type() == nsAttrValue::eInteger)
        width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Char);
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    nsCSSValue* whiteSpace = aData->ValueForWhiteSpace();
    if (whiteSpace->GetUnit() == eCSSUnit_Null) {
      
      if (aAttributes->GetAttr(nsGkAtoms::wrap))
        whiteSpace->SetIntValue(NS_STYLE_WHITESPACE_PRE_WRAP, eCSSUnit_Enumerated);

      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (!value || value->Type() != nsAttrValue::eInteger) {
        
        value = aAttributes->GetAttr(nsGkAtoms::cols);
      }

      if (value && value->Type() == nsAttrValue::eInteger) {
        
        
        whiteSpace->SetIntValue(NS_STYLE_WHITESPACE_PRE_WRAP, eCSSUnit_Enumerated);
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
nsHTMLPreElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::wrap },
    { &nsGkAtoms::cols },
    { &nsGkAtoms::width },
    { nsnull },
  };
  
  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLPreElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
