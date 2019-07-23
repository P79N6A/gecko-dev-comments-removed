



































#include "nsIDOMHTMLPreElement.h"
#include "nsIDOMEventTarget.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsCSSStruct.h"




class nsHTMLPreElement : public nsGenericHTMLElement,
                         public nsIDOMHTMLPreElement
{
public:
  nsHTMLPreElement(nsINodeInfo *aNodeInfo);
  virtual ~nsHTMLPreElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_IMETHOD GetWidth(PRInt32* aWidth);
  NS_IMETHOD SetWidth(PRInt32 aWidth);

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(Pre)


nsHTMLPreElement::nsHTMLPreElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLPreElement::~nsHTMLPreElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLPreElement, nsGenericElement)
NS_IMPL_RELEASE_INHERITED(nsHTMLPreElement, nsGenericElement)



NS_HTML_CONTENT_INTERFACE_TABLE_HEAD(nsHTMLPreElement, nsGenericHTMLElement)
  NS_INTERFACE_TABLE_INHERITED1(nsHTMLPreElement, nsIDOMHTMLPreElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLPreElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLPreElement)


NS_IMPL_INT_ATTR(nsHTMLPreElement, Width, width)


PRBool
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
  if (aData->mSID == eStyleStruct_Font) {
    
    if (aAttributes->GetAttr(nsGkAtoms::variable))
      aData->mFontData->mFamily.SetStringValue(NS_LITERAL_STRING("serif"),
                                               eCSSUnit_String);
  }
  else if (aData->mSID == eStyleStruct_Position) {
    if (aData->mPositionData->mWidth.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (!value || value->Type() != nsAttrValue::eInteger) {
        
        value = aAttributes->GetAttr(nsGkAtoms::cols);
      }

      if (value && value->Type() == nsAttrValue::eInteger)
        aData->mPositionData->mWidth.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Char);
    }
  }
  else if (aData->mSID == eStyleStruct_Text) {
    if (aData->mTextData->mWhiteSpace.GetUnit() == eCSSUnit_Null) {
      
      if (aAttributes->GetAttr(nsGkAtoms::wrap))
        aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_MOZ_PRE_WRAP, eCSSUnit_Enumerated);

      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (!value || value->Type() != nsAttrValue::eInteger) {
        
        value = aAttributes->GetAttr(nsGkAtoms::cols);
      }

      if (value && value->Type() == nsAttrValue::eInteger) {
        
        
        aData->mTextData->mWhiteSpace.SetIntValue(NS_STYLE_WHITESPACE_MOZ_PRE_WRAP, eCSSUnit_Enumerated);
      }
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLPreElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::variable },
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
