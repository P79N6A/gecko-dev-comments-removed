



































#include "nsIDOMHTMLTableRowElement.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMEventTarget.h"
#include "nsDOMError.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsContentList.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsRuleData.h"

class nsHTMLTableRowElement : public nsGenericHTMLElement,
                              public nsIDOMHTMLTableRowElement
{
public:
  nsHTMLTableRowElement(nsINodeInfo *aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLEROWELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLTableRowElement,
                                                     nsGenericHTMLElement)

protected:
  nsresult GetSection(nsIDOMHTMLTableSectionElement** aSection);
  nsresult GetTable(nsIDOMHTMLTableElement** aTable);
  nsRefPtr<nsContentList> mCells;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableRow)


nsHTMLTableRowElement::nsHTMLTableRowElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLTableRowElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLTableRowElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mCells,
                                                       nsIDOMNodeList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLTableRowElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableRowElement, nsGenericElement) 



NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLTableRowElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLTableRowElement,
                                   nsIDOMHTMLTableRowElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTableRowElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableRowElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLTableRowElement)



nsresult
nsHTMLTableRowElement::GetSection(nsIDOMHTMLTableSectionElement** aSection)
{
  NS_ENSURE_ARG_POINTER(aSection);
  *aSection = nsnull;

  nsCOMPtr<nsIDOMNode> sectionNode;
  nsresult rv = GetParentNode(getter_AddRefs(sectionNode));
  if (NS_SUCCEEDED(rv) && sectionNode) {
    rv = CallQueryInterface(sectionNode, aSection);
  }

  return rv;
}


nsresult
nsHTMLTableRowElement::GetTable(nsIDOMHTMLTableElement** aTable)
{
  NS_ENSURE_ARG_POINTER(aTable);
  *aTable = nsnull;

  nsCOMPtr<nsIDOMNode> sectionNode;
  nsresult rv = GetParentNode(getter_AddRefs(sectionNode));
  if (!sectionNode) {
    return rv;
  }

  
  rv = CallQueryInterface(sectionNode, aTable);
  if (NS_SUCCEEDED(rv)) {
    return rv;
  }

  nsCOMPtr<nsIDOMNode> tableNode;
  rv = sectionNode->GetParentNode(getter_AddRefs(tableNode));
  if (!tableNode) {
    return rv;
  }
  
  return CallQueryInterface(tableNode, aTable);
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetRowIndex(PRInt32* aValue)
{
  *aValue = -1;
  nsCOMPtr<nsIDOMHTMLTableElement> table;

  nsresult result = GetTable(getter_AddRefs(table));

  if (NS_SUCCEEDED(result) && table) {
    nsCOMPtr<nsIDOMHTMLCollection> rows;

    table->GetRows(getter_AddRefs(rows));

    PRUint32 numRows;
    rows->GetLength(&numRows);

    PRBool found = PR_FALSE;

    for (PRUint32 i = 0; (i < numRows) && !found; i++) {
      nsCOMPtr<nsIDOMNode> node;

      rows->Item(i, getter_AddRefs(node));

      if (node.get() == static_cast<nsIDOMNode *>(this)) {
        *aValue = i;
        found = PR_TRUE;
      }
    }
  }

  return result;
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetSectionRowIndex(PRInt32* aValue)
{
  *aValue = -1;

  nsCOMPtr<nsIDOMHTMLTableSectionElement> section;

  nsresult result = GetSection(getter_AddRefs(section));

  if (NS_SUCCEEDED(result) && section) {
    nsCOMPtr<nsIDOMHTMLCollection> rows;

    section->GetRows(getter_AddRefs(rows));

    PRBool found = PR_FALSE;
    PRUint32 numRows;

    rows->GetLength(&numRows);

    for (PRUint32 i = 0; (i < numRows) && !found; i++) {
      nsCOMPtr<nsIDOMNode> node;
      rows->Item(i, getter_AddRefs(node));

      if (node.get() == static_cast<nsIDOMNode *>(this)) {
        *aValue = i;
        found = PR_TRUE;
      }
    } 
  }

  return NS_OK;
}

static PRBool
IsCell(nsIContent *aContent, PRInt32 aNamespaceID,
       nsIAtom* aAtom, void *aData)
{
  nsIAtom* tag = aContent->Tag();

  return ((tag == nsGkAtoms::td || tag == nsGkAtoms::th) &&
          aContent->IsHTML());
}

NS_IMETHODIMP
nsHTMLTableRowElement::GetCells(nsIDOMHTMLCollection** aValue)
{
  if (!mCells) {
    mCells = new nsContentList(this,
                               IsCell,
                               nsnull, 
                               nsnull, 
                               PR_FALSE,
                               nsnull,
                               kNameSpaceID_XHTML,
                               PR_FALSE);

    NS_ENSURE_TRUE(mCells, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aValue = mCells);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableRowElement::InsertCell(PRInt32 aIndex, nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  nsCOMPtr<nsIDOMHTMLCollection> cells;
  nsresult rv = GetCells(getter_AddRefs(cells));
  if (NS_FAILED(rv)) {
    return rv;
  }

  NS_ASSERTION(mCells, "How did that happen?");

  nsCOMPtr<nsIDOMNode> nextSibling;
  
  if (aIndex != -1) {
    cells->Item(aIndex, getter_AddRefs(nextSibling));
    
    
    
    if (!nextSibling) {
      PRUint32 cellCount;
      cells->GetLength(&cellCount);
      if (aIndex > PRInt32(cellCount)) {
        return NS_ERROR_DOM_INDEX_SIZE_ERR;
      }
    }
  }

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::td,
                              getter_AddRefs(nodeInfo));

  nsCOMPtr<nsIContent> cellContent = NS_NewHTMLTableCellElement(nodeInfo);
  if (!cellContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsCOMPtr<nsIDOMNode> cellNode(do_QueryInterface(cellContent));
  NS_ASSERTION(cellNode, "Should implement nsIDOMNode!");

  nsCOMPtr<nsIDOMNode> retChild;
  InsertBefore(cellNode, nextSibling, getter_AddRefs(retChild));

  if (retChild) {
    CallQueryInterface(retChild, aValue);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableRowElement::DeleteCell(PRInt32 aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> cells;
  GetCells(getter_AddRefs(cells));

  nsresult rv;
  PRUint32 refIndex;
  if (aValue == -1) {
    rv = cells->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (PRUint32)aValue;
  }

  nsCOMPtr<nsIDOMNode> cell;
  rv = cells->Item(refIndex, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!cell) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMNode> retChild;
  return RemoveChild(cell, getter_AddRefs(retChild));
}

NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, Align, align, "left")
NS_IMPL_STRING_ATTR(nsHTMLTableRowElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableRowElement, ChOff, charoff)
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableRowElement, VAlign, valign, "middle")


PRBool
nsHTMLTableRowElement::ParseAttribute(PRInt32 aNamespaceID,
                                      nsIAtom* aAttribute,
                                      const nsAString& aValue,
                                      nsAttrValue& aResult)
{
  





  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::width) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE);
    }
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableCellHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::bgcolor) {
      return aResult.ParseColor(aValue, GetOwnerDoc());
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
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        aData->mPositionData->mHeight.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        aData->mPositionData->mHeight.SetPercentValue(value->GetPercentValue());
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Text)) {
    if (aData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTextData->mTextAlign.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TextReset)) {
    if (aData->mTextData->mVerticalAlign.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::valign);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTextData->mVerticalAlign.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }

  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableRowElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::align },
    { &nsGkAtoms::valign }, 
    { &nsGkAtoms::height },
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, NS_ARRAY_LENGTH(map));
}

nsMapRuleToAttributesFunc
nsHTMLTableRowElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
