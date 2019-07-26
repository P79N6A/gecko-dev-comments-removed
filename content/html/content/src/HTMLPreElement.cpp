




#include "mozilla/dom/HTMLPreElement.h"
#include "mozilla/dom/HTMLPreElementBinding.h"

#include "nsAttrValueInlines.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Pre)

namespace mozilla {
namespace dom {

HTMLPreElement::~HTMLPreElement()
{
}

NS_IMPL_ISUPPORTS_INHERITED(HTMLPreElement, nsGenericHTMLElement,
                            nsIDOMHTMLPreElement)

NS_IMPL_ELEMENT_CLONE(HTMLPreElement)

NS_IMPL_INT_ATTR(HTMLPreElement, Width, width)

bool
HTMLPreElement::ParseAttribute(int32_t aNamespaceID,
                               nsIAtom* aAttribute,
                               const nsAString& aValue,
                               nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

void
HTMLPreElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                      nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    nsCSSValue* whiteSpace = aData->ValueForWhiteSpace();
    if (whiteSpace->GetUnit() == eCSSUnit_Null) {
      
      if (aAttributes->GetAttr(nsGkAtoms::wrap))
        whiteSpace->SetIntValue(NS_STYLE_WHITESPACE_PRE_WRAP, eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLPreElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::wrap },
    { nullptr },
  };
  
  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
HTMLPreElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

JSObject*
HTMLPreElement::WrapNode(JSContext *aCx)
{
  return HTMLPreElementBinding::Wrap(aCx, this);
}

} 
} 
