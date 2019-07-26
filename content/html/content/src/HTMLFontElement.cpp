




#include "mozilla/Util.h"

#include "HTMLFontElement.h"
#include "mozilla/dom/HTMLFontElementBinding.h"
#include "nsAttrValueInlines.h"
#include "nsMappedAttributes.h"
#include "nsRuleData.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Font)

namespace mozilla {
namespace dom {

HTMLFontElement::~HTMLFontElement()
{
}

JSObject*
HTMLFontElement::WrapNode(JSContext *aCx, JSObject *aScope)
{
  return HTMLFontElementBinding::Wrap(aCx, aScope, this);
}

NS_IMPL_ADDREF_INHERITED(HTMLFontElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLFontElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLFontElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLFontElement, nsIDOMHTMLFontElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLFontElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(HTMLFontElement)

NS_IMETHODIMP
HTMLFontElement::GetColor(nsAString& aColor)
{
  nsString color;
  GetColor(color);
  aColor = color;
  return NS_OK;
}

NS_IMETHODIMP 
HTMLFontElement::SetColor(const nsAString& aColor)
{
  ErrorResult rv;
  SetColor(aColor, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLFontElement::GetFace(nsAString& aFace)
{
  nsString face;
  GetFace(face);
  aFace = face;
  return NS_OK;
}

NS_IMETHODIMP 
HTMLFontElement::SetFace(const nsAString& aFace)
{
  ErrorResult rv;
  SetFace(aFace, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLFontElement::GetSize(nsAString& aSize)
{
  nsString size;
  GetSize(size);
  aSize = size;
  return NS_OK;
}

NS_IMETHODIMP 
HTMLFontElement::SetSize(const nsAString& aSize)
{
  ErrorResult rv;
  SetSize(aSize, rv);
  return rv.ErrorCode();
}

bool
HTMLFontElement::ParseAttribute(int32_t aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::size) {
      int32_t size = nsContentUtils::ParseLegacyFontSize(aValue);
      if (size) {
        aResult.SetTo(size, &aValue);
        return true;
      }
      return false;
    }
    if (aAttribute == nsGkAtoms::color) {
      return aResult.ParseColor(aValue);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static void
MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                      nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Font)) {
    
    nsCSSValue* family = aData->ValueForFontFamily();
    if (family->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::face);
      if (value && value->Type() == nsAttrValue::eString &&
          !value->IsEmptyString()) {
        family->SetStringValue(value->GetStringValue(), eCSSUnit_Families);
      }
    }

    
    nsCSSValue* fontSize = aData->ValueForFontSize();
    if (fontSize->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::size);
      if (value && value->Type() == nsAttrValue::eInteger) {
        fontSize->SetIntValue(value->GetIntegerValue(), eCSSUnit_Enumerated);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Color)) {
    nsCSSValue* colorValue = aData->ValueForColor();
    if (colorValue->GetUnit() == eCSSUnit_Null &&
        aData->mPresContext->UseDocumentColors()) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::color);
      nscolor color;
      if (value && value->GetColorValue(color)) {
        colorValue->SetColorValue(color);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TextReset) &&
      aData->mPresContext->CompatibilityMode() == eCompatibility_NavQuirks) {
    
    
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::color);
    nscolor color;
    if (value && value->GetColorValue(color)) {
      nsCSSValue* decoration = aData->ValueForTextDecorationLine();
      int32_t newValue = NS_STYLE_TEXT_DECORATION_LINE_OVERRIDE_ALL;
      if (decoration->GetUnit() == eCSSUnit_Enumerated) {
        newValue |= decoration->GetIntValue();
      }
      decoration->SetIntValue(newValue, eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLFontElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::face },
    { &nsGkAtoms::size },
    { &nsGkAtoms::color },
    { nullptr }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}


nsMapRuleToAttributesFunc
HTMLFontElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

} 
} 
