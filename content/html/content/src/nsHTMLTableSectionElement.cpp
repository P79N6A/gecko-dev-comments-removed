



































#include "nsIDOMHTMLTableSectionElem.h"
#include "nsIDOMEventReceiver.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsContentList.h"
#include "nsRuleData.h"
#include "nsDOMError.h"
#include "nsIDocument.h"



class nsHTMLTableSectionElement : public nsGenericHTMLElement,
                                  public nsIDOMHTMLTableSectionElement
{
public:
  nsHTMLTableSectionElement(nsINodeInfo *aNodeInfo);

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLESECTIONELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLTableSectionElement,
                                                     nsGenericHTMLElement)

protected:
  nsRefPtr<nsContentList> mRows;
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableSection)


nsHTMLTableSectionElement::nsHTMLTableSectionElement(nsINodeInfo *aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLTableSectionElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLTableSectionElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mRows,
                                                       nsBaseContentList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLTableSectionElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableSectionElement, nsGenericElement) 



NS_HTML_CONTENT_CC_INTERFACE_MAP_BEGIN(nsHTMLTableSectionElement,
                                       nsGenericHTMLElement)
  NS_INTERFACE_MAP_ENTRY(nsIDOMHTMLTableSectionElement)
  NS_INTERFACE_MAP_ENTRY_CONTENT_CLASSINFO(HTMLTableSectionElement)
NS_HTML_CONTENT_INTERFACE_MAP_END


NS_IMPL_ELEMENT_CLONE(nsHTMLTableSectionElement)


NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, Align, align, "left")
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, VAlign, valign, "middle")
NS_IMPL_STRING_ATTR_DEFAULT_VALUE(nsHTMLTableSectionElement, Ch, _char, ".")
NS_IMPL_STRING_ATTR(nsHTMLTableSectionElement, ChOff, charoff)


NS_IMETHODIMP
nsHTMLTableSectionElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  *aValue = nsnull;

  if (!mRows) {
    mRows = new nsContentList(this,
                              nsGkAtoms::tr,
                              mNodeInfo->NamespaceID(),
                              PR_FALSE);

    NS_ENSURE_TRUE(mRows, NS_ERROR_OUT_OF_MEMORY);
  }

  NS_ADDREF(*aValue = mRows);
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableSectionElement::InsertRow(PRInt32 aIndex,
                                     nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  PRUint32 rowCount;
  rows->GetLength(&rowCount);

  if (aIndex > (PRInt32)rowCount) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  PRBool doInsert = (aIndex < PRInt32(rowCount)) && (aIndex != -1);

  
  nsCOMPtr<nsINodeInfo> nodeInfo;
  nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                              getter_AddRefs(nodeInfo));

  nsCOMPtr<nsIContent> rowContent = NS_NewHTMLTableRowElement(nodeInfo);
  if (!nodeInfo) {
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
  } else {
    rv = AppendChild(rowNode, getter_AddRefs(retChild));
  }

  if (retChild) {
    CallQueryInterface(retChild, aValue);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableSectionElement::DeleteRow(PRInt32 aValue)
{
  if (aValue < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  nsresult rv;
  PRUint32 refIndex;
  if (aValue == -1) {
    rv = rows->GetLength(&refIndex);
    NS_ENSURE_SUCCESS(rv, rv);

    if (refIndex == 0) {
      return NS_OK;
    }

    --refIndex;
  }
  else {
    refIndex = (PRUint32)aValue;
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

PRBool
nsHTMLTableSectionElement::ParseAttribute(PRInt32 aNamespaceID,
                                          nsIAtom* aAttribute,
                                          const nsAString& aValue,
                                          nsAttrValue& aResult)
{
  if (aNamespaceID == kNameSpaceID_None) {
    


    if (aAttribute == nsGkAtoms::charoff) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue, PR_TRUE, PR_FALSE);
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
  if (aData->mSID == eStyleStruct_Position) {
    
    if (aData->mPositionData->mHeight.GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        aData->mPositionData->mHeight.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);   
    }
  }
  else if (aData->mSID == eStyleStruct_Text) {
    if (aData->mTextData->mTextAlign.GetUnit() == eCSSUnit_Null) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);
      if (value && value->Type() == nsAttrValue::eEnum)
        aData->mTextData->mTextAlign.SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
    }
  }
  else if (aData->mSID == eStyleStruct_TextReset) {
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
nsHTMLTableSectionElement::IsAttributeMapped(const nsIAtom* aAttribute) const
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
nsHTMLTableSectionElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
