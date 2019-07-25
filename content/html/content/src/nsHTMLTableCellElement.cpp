




































#include "mozilla/Util.h"

#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLTableRowElement.h"
#include "nsHTMLTableElement.h"
#include "nsIDOMHTMLCollection.h"
#include "nsIDOMEventTarget.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsRuleData.h"
#include "nsRuleWalker.h"
#include "nsIDocument.h"
#include "celldata.h"

using namespace mozilla;

class nsHTMLTableCellElement : public nsGenericHTMLElement,
                               public nsIDOMHTMLTableCellElement
{
public:
  nsHTMLTableCellElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLTableCellElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLECELLELEMENT

  virtual bool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD WalkContentStyleRules(nsRuleWalker* aRuleWalker);
  NS_IMETHOD_(bool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  
  
  void GetRow(nsIDOMHTMLTableRowElement** aRow);
  nsIContent * GetTable();
};


NS_IMPL_NS_NEW_HTML_ELEMENT(TableCell)


nsHTMLTableCellElement::nsHTMLTableCellElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTableCellElement::~nsHTMLTableCellElement()
{
}


NS_IMPL_ADDREF_INHERITED(nsHTMLTableCellElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableCellElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLTableCellElement, nsHTMLTableCellElement)


NS_INTERFACE_TABLE_HEAD(nsHTMLTableCellElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLTableCellElement,
                                   nsIDOMHTMLTableCellElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTableCellElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableCellElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLTableCellElement)



void
nsHTMLTableCellElement::GetRow(nsIDOMHTMLTableRowElement** aRow)
{
  *aRow = nsnull;

  nsCOMPtr<nsIDOMNode> rowNode;
  GetParentNode(getter_AddRefs(rowNode));

  if (rowNode) {
    CallQueryInterface(rowNode, aRow);
  }
}


nsIContent*
nsHTMLTableCellElement::GetTable()
{
  nsIContent *result = nsnull;

  nsIContent *parent = GetParent();
  if (parent) {  
    nsIContent* section = parent->GetParent();
    if (section) {
      if (section->IsHTML() &&
          section->NodeInfo()->Equals(nsGkAtoms::table)) {
        
        result = section;
      } else {
        
        result = section->GetParent();
      }
    }
  }
  return result;
}

NS_IMETHODIMP
nsHTMLTableCellElement::GetCellIndex(PRInt32* aCellIndex)
{
  *aCellIndex = -1;

  nsCOMPtr<nsIDOMHTMLTableRowElement> row;

  GetRow(getter_AddRefs(row));

  if (!row) {
    return NS_OK;
  }

  nsCOMPtr<nsIDOMHTMLCollection> cells;

  row->GetCells(getter_AddRefs(cells));

  if (!cells) {
    return NS_OK;
  }

  PRUint32 numCells;
  cells->GetLength(&numCells);

  bool found = false;
  PRUint32 i;

  for (i = 0; (i < numCells) && !found; i++) {
    nsCOMPtr<nsIDOMNode> node;
    cells->Item(i, getter_AddRefs(node));

    if (node.get() == static_cast<nsIDOMNode *>(this)) {
      *aCellIndex = i;
      found = true;
    }
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLTableCellElement::WalkContentStyleRules(nsRuleWalker* aRuleWalker)
{
  nsresult rv = nsGenericHTMLElement::WalkContentStyleRules(aRuleWalker);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIContent* node = GetTable();
  if (node && node->IsHTML(nsGkAtoms::table)) {
    nsHTMLTableElement* table = static_cast<nsHTMLTableElement*>(node);
    nsMappedAttributes* tableInheritedAttributes =
      table->GetAttributesMappedForCell();
    if (tableInheritedAttributes)
      aRuleWalker->Forward(tableInheritedAttributes);
  }
  return NS_OK;
}


NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Abbr, abbr)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Axis, axis)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Ch, _char)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, ChOff, charoff)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, ColSpan, colspan, 1)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Headers, headers)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Height, height)
NS_IMPL_BOOL_ATTR(nsHTMLTableCellElement, NoWrap, nowrap)
NS_IMPL_INT_ATTR_DEFAULT_VALUE(nsHTMLTableCellElement, RowSpan, rowspan, 1)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Scope, scope)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, VAlign, valign)
NS_IMPL_STRING_ATTR(nsHTMLTableCellElement, Width, width)


NS_IMETHODIMP
nsHTMLTableCellElement::GetAlign(nsAString& aValue)
{
  if (!GetAttr(kNameSpaceID_None, nsGkAtoms::align, aValue)) {
    

    nsCOMPtr<nsIDOMHTMLTableRowElement> row;
    GetRow(getter_AddRefs(row));

    if (row) {
      return row->GetAlign(aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableCellElement::SetAlign(const nsAString& aValue)
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
nsHTMLTableCellElement::ParseAttribute(PRInt32 aNamespaceID,
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
        PRInt32 val = aResult.GetIntegerValue();
        
        
        if (val > MAX_COLSPAN || val < 0 ||
            (0 == val && InNavQuirksMode(GetOwnerDoc()))) {
          aResult.SetTo(1);
        }
      }
      return res;
    }
    if (aAttribute == nsGkAtoms::rowspan) {
      bool res = aResult.ParseIntWithBounds(aValue, -1, MAX_ROWSPAN);
      if (res) {
        PRInt32 val = aResult.GetIntegerValue();
        
        if (val < 0 || (0 == val && InNavQuirksMode(GetOwnerDoc()))) {
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

  return nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
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
nsHTMLTableCellElement::IsAttributeMapped(const nsIAtom* aAttribute) const
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
    { nsnull }
  };

  static const MappedAttributeEntry* const map[] = {
    attributes,
    sCommonAttributeMap,
    sBackgroundAttributeMap,
  };

  return FindAttributeDependence(aAttribute, map, ArrayLength(map));
}

nsMapRuleToAttributesFunc
nsHTMLTableCellElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
