




































#include "mozilla/Util.h"

#include "nsIDOMHTMLBRElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

using namespace mozilla;

class nsHTMLBRElement : public nsGenericHTMLElement,
                        public nsIDOMHTMLBRElement
{
public:
  nsHTMLBRElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLBRElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLBRELEMENT    

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
  virtual nsXPCClassInfo* GetClassInfo();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(BR)


nsHTMLBRElement::nsHTMLBRElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLBRElement::~nsHTMLBRElement()
{
}

NS_IMPL_ADDREF_INHERITED(nsHTMLBRElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLBRElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLBRElement, nsHTMLBRElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLBRElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLBRElement, nsIDOMHTMLBRElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLBRElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLBRElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLBRElement)


NS_IMPL_STRING_ATTR(nsHTMLBRElement, Clear, clear)

static const nsAttrValue::EnumTable kClearTable[] = {
  { "left", NS_STYLE_CLEAR_LEFT },
  { "right", NS_STYLE_CLEAR_RIGHT },
  { "all", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { "both", NS_STYLE_CLEAR_LEFT_AND_RIGHT },
  { 0 }
};

bool
nsHTMLBRElement::ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aAttribute == nsGkAtoms::clear && aNamespaceID == kNameSpaceID_None) {
    return aResult.ParseEnumValue(aValue, kClearTable, false);
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Display)) {
    nsCSSValue* clear = aData->ValueForClear();
    if (clear->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::clear);
      if (value && value->Type() == nsAttrValue::eEnum)
        clear->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
nsHTMLBRElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::clear },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, ArrayLength(map));
}

nsMapRuleToAttributesFunc
nsHTMLBRElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
