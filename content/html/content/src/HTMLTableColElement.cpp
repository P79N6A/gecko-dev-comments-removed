




#include "mozilla/Util.h"

#include "mozilla/dom/HTMLTableColElement.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsRuleData.h"
#include "mozilla/dom/HTMLTableColElementBinding.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(TableCol)

namespace mozilla {
namespace dom {



#define MAX_COLSPAN 1000

HTMLTableColElement::~HTMLTableColElement()
{
}

JSObject*
HTMLTableColElement::WrapNode(JSContext *aCx, JSObject *aScope,
                              bool *aTriedToWrap)
{
  return HTMLTableColElementBinding::Wrap(aCx, aScope, this, aTriedToWrap);
}

NS_IMPL_ADDREF_INHERITED(HTMLTableColElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTableColElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLTableColElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLTableColElement,
                                   nsIDOMHTMLTableColElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTableColElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_MAP_END

NS_IMPL_ELEMENT_CLONE(HTMLTableColElement)

NS_IMETHODIMP
HTMLTableColElement::SetSpan(int32_t aSpan)
{
  ErrorResult rv;
  SetSpan(aSpan, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetSpan(int32_t* aSpan)
{
  *aSpan = Span();
  return NS_OK;
}

NS_IMETHODIMP
HTMLTableColElement::SetAlign(const nsAString& aAlign)
{
  ErrorResult rv;
  SetAlign(aAlign, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetAlign(nsAString& aAlign)
{
  nsString align;
  GetAlign(align);
  aAlign = align;
  return NS_OK;
}

NS_IMETHODIMP
HTMLTableColElement::SetVAlign(const nsAString& aVAlign)
{
  ErrorResult rv;
  SetVAlign(aVAlign, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetVAlign(nsAString& aVAlign)
{
  nsString vAlign;
  GetVAlign(vAlign);
  aVAlign = vAlign;
  return NS_OK;
}

NS_IMETHODIMP
HTMLTableColElement::SetCh(const nsAString& aCh)
{
  ErrorResult rv;
  SetCh(aCh, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetCh(nsAString& aCh)
{
  nsString ch;
  GetCh(ch);
  aCh = ch;
  return NS_OK;
}

NS_IMETHODIMP
HTMLTableColElement::SetChOff(const nsAString& aChOff)
{
  ErrorResult rv;
  SetChOff(aChOff, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetChOff(nsAString& aChOff)
{
  nsString chOff;
  GetChOff(chOff);
  aChOff = chOff;
  return NS_OK;
}

NS_IMETHODIMP
HTMLTableColElement::SetWidth(const nsAString& aWidth)
{
  ErrorResult rv;
  SetWidth(aWidth, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
HTMLTableColElement::GetWidth(nsAString& aWidth)
{
  nsString width;
  GetWidth(width);
  aWidth = width;
  return NS_OK;
}

bool
HTMLTableColElement::ParseAttribute(int32_t aNamespaceID,
                                    nsIAtom* aAttribute,
                                    const nsAString& aValue,
                                    nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    
    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::span) {
      
      return aResult.ParseIntWithBounds(aValue, 1, MAX_COLSPAN);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableCellHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::valign) {
      return ParseTableVAlignValue(aValue, aResult);
    }
  }

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Table)) {
    nsCSSValue *span = aData->ValueForSpan();
    if (span->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::span);
      if (value && value->Type() == nsAttrValue::eInteger) {
        int32_t val = value->GetIntegerValue();
        
        
        
        if (val > 0) {
          span->SetIntValue(value->GetIntegerValue(), eCSSUnit_Integer);
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value) {
        switch (value->Type()) {
        case nsAttrValue::ePercent: {
          width->SetPercentValue(value->GetPercentValue());
          break;
        }
        case nsAttrValue::eInteger: {
          width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
          break;
        }
        default:
          break;
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    nsCSSValue* textAlign = aData->ValueForTextAlign();
    if (textAlign->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        textAlign->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TextReset)) {
    nsCSSValue* verticalAlign = aData->ValueForVerticalAlign();
    if (verticalAlign->GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::valign);
      if (value && value->Type() == nsAttrValue::eEnum)
        verticalAlign->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLTableColElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::width },
    { &nsGkAtoms::align },
    { &nsGkAtoms::valign },
    { &nsGkAtoms::span },
    { nullptr }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}


nsMapRuleToAttributesFunc
HTMLTableColElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

} 
} 
