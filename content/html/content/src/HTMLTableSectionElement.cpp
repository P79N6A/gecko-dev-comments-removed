




#include "mozilla/Util.h"

#include "mozilla/dom/HTMLTableSectionElement.h"
#include "nsMappedAttributes.h"
#include "nsAttrValueInlines.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsStyleConsts.h"
#include "nsContentList.h"
#include "nsRuleData.h"
#include "nsError.h"
#include "nsContentUtils.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(TableSection)
DOMCI_NODE_DATA(HTMLTableSectionElement, mozilla::dom::HTMLTableSectionElement)

namespace mozilla {
namespace dom {



HTMLTableSectionElement::HTMLTableSectionElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLTableSectionElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLTableSectionElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRows)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLTableSectionElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTableSectionElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLTableSectionElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(HTMLTableSectionElement,
                                   nsIDOMHTMLTableSectionElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(HTMLTableSectionElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableSectionElement)


NS_IMPL_ELEMENT_CLONE(HTMLTableSectionElement)


NS_IMPL_STRING_ATTR(HTMLTableSectionElement, Align, align)
NS_IMPL_STRING_ATTR(HTMLTableSectionElement, VAlign, valign)
NS_IMPL_STRING_ATTR(HTMLTableSectionElement, Ch, _char)
NS_IMPL_STRING_ATTR(HTMLTableSectionElement, ChOff, charoff)


NS_IMETHODIMP
HTMLTableSectionElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  if (!mRows) {
    mRows = new nsContentList(this,
                              mNodeInfo->NamespaceID(),
                              nsGkAtoms::tr,
                              nsGkAtoms::tr,
                              false);
  }

  NS_ADDREF(*aValue = mRows);
  return NS_OK;
}


NS_IMETHODIMP
HTMLTableSectionElement::InsertRow(int32_t aIndex,
                                   nsIDOMHTMLElement** aValue)
{
  *aValue = nullptr;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  uint32_t rowCount;
  rows->GetLength(&rowCount);

  if (aIndex > (int32_t)rowCount) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  bool doInsert = (aIndex < int32_t(rowCount)) && (aIndex != -1);

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                              getter_AddRefs(nodeInfo));

  nsCOMPtr<nsIContent> rowContent = NS_NewHTMLTableRowElement(nodeInfo.forget());
  if (!rowContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> rowNode(do_QueryInterface(rowContent));
  NS_ASSERTION(rowNode, "Should implement nsIDOMNode!");

  nsCOMPtr<nsIDOMNode> retChild;

  nsresult rv;
  if (doInsert) {
    nsCOMPtr<nsIDOMNode> refRow;
    rows->Item(aIndex, getter_AddRefs(refRow));

    rv = InsertBefore(rowNode, refRow, getter_AddRefs(retChild));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = AppendChild(rowNode, getter_AddRefs(retChild));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (retChild) {
    CallQueryInterface(retChild, aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
HTMLTableSectionElement::DeleteRow(int32_t aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  nsresult rv;
  uint32_t refIndex;
  if (aValue == -1) {
    rv = rows->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (uint32_t)aValue;
  }

  nsCOMPtr<nsIDOMNode> row;
  rv = rows->Item(refIndex, getter_AddRefs(row));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!row) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMNode> retChild;
  return RemoveChild(row, getter_AddRefs(retChild));
}

bool
HTMLTableSectionElement::ParseAttribute(int32_t aNamespaceID,
                                        nsIAtom* aAttribute,
                                        const nsAString& aValue,
                                        nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    


    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableCellHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::bgcolor) {
      return aResult.ParseColor(aValue);
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
void MapAttributesIntoRule(const nsMappedAttributes* aAttributes, nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* height = aData->ValueForHeight();
    if (height->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        height->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
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

  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLTableSectionElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align }, 
    { &nsGkAtoms::valign },
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
HTMLTableSectionElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

} 
} 
