




#include "mozilla/Util.h"

#include "mozilla/dom/HTMLTableCellElement.h"
#include "mozilla/dom/HTMLTableElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsRuleData.h"
#include "nsRuleWalker.h"
#include "celldata.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(TableCell)
DOMCI_NODE_DATA(HTMLTableCellElement, mozilla::dom::HTMLTableCellElement)

namespace mozilla {
namespace dom {

HTMLTableCellElement::~HTMLTableCellElement()
{
}

NS_IMPL_ADDREF_INHERITED(HTMLTableCellElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTableCellElement, Element)


NS_INTERFACE_TABLE_HEAD(HTMLTableCellElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLTableCellElement,
                                   nsIDOMHTMLTableCellElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTableCellElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableCellElement)


NS_IMPL_ELEMENT_CLONE(HTMLTableCellElement)



already_AddRefed<nsIDOMHTMLTableRowElement>
HTMLTableCellElement::GetRow() const
{
  nsCOMPtr<nsIDOMHTMLTableRowElement> row = do_QueryInterface(GetParent());
  return row.forget();
}


HTMLTableElement*
HTMLTableCellElement::GetTable() const
{
  nsIContent *parent = GetParent();
  if (!parent) {
    return nullptr;
  }

  
  nsIContent* section = parent->GetParent();
  if (!section) {
    return nullptr;
  }

  if (section->IsHTML(nsGkAtoms::table)) {
    
    return static_cast<HTMLTableElement*>(section);
  }

  
  nsIContent* result = section->GetParent();
  if (result && result->IsHTML(nsGkAtoms::table)) {
    return static_cast<HTMLTableElement*>(result);
  }

  return nullptr;
}

NS_IMETHODIMP
HTMLTableCellElement::GetCellIndex(int32_t* aCellIndex)
{
  *aCellIndex = -1;

  nsCOMPtr<nsIDOMHTMLTableRowElement> row = GetRow();
  if (!row) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLCollection> cells;

  row->GetCells(getter_AddRefs(cells));

  if (!cells) {
    return NS_OK;
  }

  uint32_t numCells;
  cells->GetLength(&numCells);

  for (uint32_t i = 0; i < numCells; i++) {
    nsCOMPtr<nsIDOMNode> node;
    cells->Item(i, getter_AddRefs(node));

    if (node.get() == static_cast<nsIDOMNode *>(this)) {
      *aCellIndex = i;
      break;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
HTMLTableCellElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  nsresult rv = nsGenericHTMLElement::WalkContentStyleRules(aRuleWalker);
  NS_ENSURE_SUCCESS(rv, rv);

  if (HTMLTableElement* table = GetTable()) {
    nsMappedAttributes* tableInheritedAttributes =
      table->GetAttributesMappedForCell();
    if (tableInheritedAttributes) {
      aRuleWalker->Forward(tableInheritedAttributes);
    }
  }
  return NS_OK;
}


NS_IMPL_STRING_ATTR(HTMLTableCellElement, Abbr, abbr)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Axis, axis)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Ch, _char)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, ChOff, charoff)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(HTMLTableCellElement, ColSpan, colspan, 1)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Headers, headers)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Height, height)
NS_IMPL_BOOL_ATTR(HTMLTableCellElement, NoWrap, nowrap)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(HTMLTableCellElement, RowSpan, rowspan, 1)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Scope, scope)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, VAlign, valign)
NS_IMPL_STRING_ATTR(HTMLTableCellElement, Width, width)


NS_IMETHODIMP
HTMLTableCellElement::GetAlign(nsAString& aValue)
{
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::align, aValue)) {
    
    nsCOMPtr<nsIDOMHTMLTableRowElement> row = GetRow();
    if (row) {
      return row->GetAlign(aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLTableCellElement::SetAlign(const nsAString& aValue)
{
  return SetAttr(kNameSpaceID_None, nsGkAtoms::align, aValue, true);
}


static const nsAttrValue::EnumTable kCellScopeTable[] = {
  { "row",      NS_STYLE_CELL_SCOPE_ROW },
  { "col",      NS_STYLE_CELL_SCOPE_COL },
  { "rowgroup", NS_STYLE_CELL_SCOPE_ROWGROUP },
  { "colgroup", NS_STYLE_CELL_SCOPE_COLGROUP },
  { 0 }
};

bool
HTMLTableCellElement::ParseAttribute(int32_t aNamespaceID,
                                     nsIAtom* aAttribute,
                                     const nsAString& aValue,
                                     nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    


    if (aAttribute == nsGkAtoms::charoff) {
      
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::colspan) {
      bool res = aResult.ParseIntWithBounds(aValue, -1);
      if (res) {
        int32_t val = aResult.GetIntegerValue();
        
        
        if (val > MAX_COLSPAN || val < 0 ||
            (0 == val && InNavQuirksMode(OwnerDoc()))) {
          aResult.SetTo(1);
        }
      }
      return res;
    }
    if (aAttribute == nsGkAtoms::rowspan) {
      bool res = aResult.ParseIntWithBounds(aValue, -1, MAX_ROWSPAN);
      if (res) {
        int32_t val = aResult.GetIntegerValue();
        
        if (val < 0 || (0 == val && InNavQuirksMode(OwnerDoc()))) {
          aResult.SetTo(1);
        }
      }
      return res;
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableCellHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::bgcolor) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::scope) {
      return aResult.ParseEnumValue(aValue, kCellScopeTable, false);
    }
    if (aAttribute == nsGkAtoms::valign) {
      return ParseTableVAlignValue(aValue, aResult);
    }
  }

  return nsGenericHTMLElement::ParseBackgroundAttribute(aNamespaceID,
                                                        aAttribute, aValue,
                                                        aResult) ||
         nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}

static 
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                           nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger) {
        if (value->GetIntegerValue() > 0)
          width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel); 
        
      }
      else if (value && value->Type() == nsAttrValue::ePercent) {
        if (value->GetPercentValue() > 0.0f)
          width->SetPercentValue(value->GetPercentValue());
        
      }
    }

    
    nsCSSValue* height = aData->ValueForHeight();
    if (height->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger) {
        if (value->GetIntegerValue() > 0)
          height->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
        
      }
      else if (value && value->Type() == nsAttrValue::ePercent) {
        if (value->GetPercentValue() > 0.0f)
          height->SetPercentValue(value->GetPercentValue());
        
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

    nsCSSValue* whiteSpace = aData->ValueForWhiteSpace();
    if (whiteSpace->GetUnit() == eCSSUnit_Null) {
      
      if (aAttributes->GetAttr(nsGkAtoms::nowrap)) {
        
        const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
        nsCompatibility mode = aData->mPresContext->CompatibilityMode();
        if (!value || value->Type() != nsAttrValue::eInteger ||
            value->GetIntegerValue() == 0 ||
            eCompatibility_NavQuirks != mode) {
          whiteSpace->SetIntValue(NS_STYLE_WHITESPACE_NOWRAP, eCSSUnit_Enumerated);
        }
      }
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
  
  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLTableCellElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align }, 
    { &nsGkAtoms::valign },
    { &nsGkAtoms::nowrap },
#if 0
    
    
    { &nsGkAtoms::abbr },
    { &nsGkAtoms::axis },
    { &nsGkAtoms::headers },
    { &nsGkAtoms::scope },
#endif
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { nullptr }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map);
}

nsMapRuleToAttributesFunc
HTMLTableCellElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

} 
} 
