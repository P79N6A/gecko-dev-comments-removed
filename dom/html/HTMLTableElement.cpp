




#include "mozilla/dom/HTMLTableElement.h"
#include "nsAttrValueInlines.h"
#include "nsRuleData.h"
#include "nsHTMLStyleSheet.h"
#include "nsMappedAttributes.h"
#include "mozilla/dom/HTMLCollectionBinding.h"
#include "mozilla/dom/HTMLTableElementBinding.h"
#include "nsContentUtils.h"
#include "jsfriendapi.h"

NS_IMPL_NS_NEW_HTML_ELEMENT(Table)

namespace mozilla {
namespace dom {






class TableRowsCollection final : public nsIHTMLCollection,
                                  public nsWrapperCache
{
public:
  explicit TableRowsCollection(HTMLTableElement* aParent);

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual Element* GetElementAt(uint32_t aIndex) override;
  virtual nsINode* GetParentObject() override
  {
    return mParent;
  }

  virtual Element*
  GetFirstNamedElement(const nsAString& aName, bool& aFound) override;
  virtual void GetSupportedNames(unsigned aFlags,
                                 nsTArray<nsString>& aNames) override;

  NS_IMETHOD    ParentDestroyed();

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TableRowsCollection)

  
  using nsWrapperCache::GetWrapperPreserveColor;
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
protected:
  virtual ~TableRowsCollection();

  virtual JSObject* GetWrapperPreserveColorInternal() override
  {
    return nsWrapperCache::GetWrapperPreserveColor();
  }

  
  HTMLTableElement* mParent;
  nsRefPtr<nsContentList> mOrphanRows;  
};


TableRowsCollection::TableRowsCollection(HTMLTableElement *aParent)
  : mParent(aParent)
  , mOrphanRows(new nsContentList(mParent,
                                  kNameSpaceID_XHTML,
                                  nsGkAtoms::tr,
                                  nsGkAtoms::tr,
                                  false))
{
}

TableRowsCollection::~TableRowsCollection()
{
  
  
  
  
}

JSObject*
TableRowsCollection::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLCollectionBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(TableRowsCollection, mOrphanRows)
NS_IMPL_CYCLE_COLLECTING_ADDREF(TableRowsCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TableRowsCollection)

NS_INTERFACE_TABLE_HEAD(TableRowsCollection)
  NS_WRAPPERCACHE_INTERFACE_TABLE_ENTRY
  NS_INTERFACE_TABLE(TableRowsCollection, nsIHTMLCollection,
                     nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(TableRowsCollection)
NS_INTERFACE_MAP_END






#define DO_FOR_EACH_ROWGROUP(_code)                                  \
  do {                                                               \
    if (mParent) {                                                   \
      /* THead */                                                    \
      HTMLTableSectionElement* rowGroup = mParent->GetTHead();       \
      nsIHTMLCollection* rows;                                       \
      if (rowGroup) {                                                \
        rows = rowGroup->Rows();                                     \
        do { /* gives scoping */                                     \
          _code                                                      \
        } while (0);                                                 \
      }                                                              \
      /* TBodies */                                                  \
      for (nsIContent* _node = mParent->nsINode::GetFirstChild();    \
           _node; _node = _node->GetNextSibling()) {                 \
        if (_node->IsHTMLElement(nsGkAtoms::tbody)) {                \
          rowGroup = static_cast<HTMLTableSectionElement*>(_node);   \
          rows = rowGroup->Rows();                                   \
          do { /* gives scoping */                                   \
            _code                                                    \
          } while (0);                                               \
        }                                                            \
      }                                                              \
      /* orphan rows */                                              \
      rows = mOrphanRows;                                            \
      do { /* gives scoping */                                       \
        _code                                                        \
      } while (0);                                                   \
      /* TFoot */                                                    \
      rowGroup = mParent->GetTFoot();                                \
      rows = nullptr;                                                \
      if (rowGroup) {                                                \
        rows = rowGroup->Rows();                                     \
        do { /* gives scoping */                                     \
          _code                                                      \
        } while (0);                                                 \
      }                                                              \
    }                                                                \
  } while (0)

static uint32_t
CountRowsInRowGroup(nsIDOMHTMLCollection* rows)
{
  uint32_t length = 0;
  
  if (rows) {
    rows->GetLength(&length);
  }
  
  return length;
}




NS_IMETHODIMP 
TableRowsCollection::GetLength(uint32_t* aLength)
{
  *aLength=0;

  DO_FOR_EACH_ROWGROUP(
    *aLength += CountRowsInRowGroup(rows);
  );

  return NS_OK;
}




static Element*
GetItemOrCountInRowGroup(nsIDOMHTMLCollection* rows,
                         uint32_t aIndex, uint32_t* aCount)
{
  *aCount = 0;

  if (rows) {
    rows->GetLength(aCount);
    if (aIndex < *aCount) {
      nsIHTMLCollection* list = static_cast<nsIHTMLCollection*>(rows);
      return list->GetElementAt(aIndex);
    }
  }
  
  return nullptr;
}

Element*
TableRowsCollection::GetElementAt(uint32_t aIndex)
{
  DO_FOR_EACH_ROWGROUP(
    uint32_t count;
    Element* node = GetItemOrCountInRowGroup(rows, aIndex, &count);
    if (node) {
      return node; 
    }

    NS_ASSERTION(count <= aIndex, "GetItemOrCountInRowGroup screwed up");
    aIndex -= count;
  );

  return nullptr;
}

NS_IMETHODIMP 
TableRowsCollection::Item(uint32_t aIndex, nsIDOMNode** aReturn)
{
  nsISupports* node = GetElementAt(aIndex);
  if (!node) {
    *aReturn = nullptr;

    return NS_OK;
  }

  return CallQueryInterface(node, aReturn);
}

Element*
TableRowsCollection::GetFirstNamedElement(const nsAString& aName, bool& aFound)
{
  aFound = false;
  DO_FOR_EACH_ROWGROUP(
    Element* item = rows->NamedGetter(aName, aFound);
    if (aFound) {
      return item;
    }
  );
  return nullptr;
}

void
TableRowsCollection::GetSupportedNames(unsigned aFlags,
                                       nsTArray<nsString>& aNames)
{
  if (!(aFlags & JSITER_HIDDEN)) {
    return;
  }

  DO_FOR_EACH_ROWGROUP(
    nsTArray<nsString> names;
    nsCOMPtr<nsIHTMLCollection> coll = do_QueryInterface(rows);
    if (coll) {
      coll->GetSupportedNames(aFlags, names);
      for (uint32_t i = 0; i < names.Length(); ++i) {
        if (!aNames.Contains(names[i])) {
          aNames.AppendElement(names[i]);
        }
      }
    }
  );
}


NS_IMETHODIMP 
TableRowsCollection::NamedItem(const nsAString& aName,
                               nsIDOMNode** aReturn)
{
  DO_FOR_EACH_ROWGROUP(
    nsCOMPtr<nsIHTMLCollection> collection = do_QueryInterface(rows);
    if (collection) {
      nsresult rv = collection->NamedItem(aName, aReturn);
      if (NS_FAILED(rv) || *aReturn) {
        return rv;
      }
    }
  );

  *aReturn = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
TableRowsCollection::ParentDestroyed()
{
  
  mParent = nullptr;

  return NS_OK;
}



HTMLTableElement::HTMLTableElement(already_AddRefed<mozilla::dom::NodeInfo>& aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo),
    mTableInheritedAttributes(TABLE_ATTRS_DIRTY)
{
  SetHasWeirdParserInsertionMode();
}

HTMLTableElement::~HTMLTableElement()
{
  if (mRows) {
    mRows->ParentDestroyed();
  }
  ReleaseInheritedAttributes();
}

JSObject*
HTMLTableElement::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLTableElementBinding::Wrap(aCx, this, aGivenProto);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(HTMLTableElement)

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(HTMLTableElement, nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mTBodies)
  if (tmp->mRows) {
    tmp->mRows->ParentDestroyed();
  }
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mRows)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(HTMLTableElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mTBodies)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mRows)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(HTMLTableElement, Element)
NS_IMPL_RELEASE_INHERITED(HTMLTableElement, Element)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(HTMLTableElement)
  NS_INTERFACE_TABLE_INHERITED(HTMLTableElement, nsIDOMHTMLTableElement)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsGenericHTMLElement)


NS_IMPL_ELEMENT_CLONE(HTMLTableElement)






nsIHTMLCollection*
HTMLTableElement::Rows()
{
  if (!mRows) {
    mRows = new TableRowsCollection(this);
  }

  return mRows;
}

nsIHTMLCollection*
HTMLTableElement::TBodies()
{
  if (!mTBodies) {
    
    mTBodies = new nsContentList(this,
                                 kNameSpaceID_XHTML,
                                 nsGkAtoms::tbody,
                                 nsGkAtoms::tbody,
                                 false);
  }

  return mTBodies;
}

already_AddRefed<nsGenericHTMLElement>
HTMLTableElement::CreateTHead()
{
  nsRefPtr<nsGenericHTMLElement> head = GetTHead();
  if (!head) {
    
    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::thead,
                                getter_AddRefs(nodeInfo));

    head = NS_NewHTMLTableSectionElement(nodeInfo.forget());
    if (!head) {
      return nullptr;
    }

    ErrorResult rv;
    nsINode::InsertBefore(*head, nsINode::GetFirstChild(), rv);
  }
  return head.forget();
}

void
HTMLTableElement::DeleteTHead()
{
  HTMLTableSectionElement* tHead = GetTHead();
  if (tHead) {
    mozilla::ErrorResult rv;
    nsINode::RemoveChild(*tHead, rv);
    MOZ_ASSERT(!rv.Failed());
  }
}

already_AddRefed<nsGenericHTMLElement>
HTMLTableElement::CreateTFoot()
{
  nsRefPtr<nsGenericHTMLElement> foot = GetTFoot();
  if (!foot) {
    
    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tfoot,
                                getter_AddRefs(nodeInfo));

    foot = NS_NewHTMLTableSectionElement(nodeInfo.forget());
    if (!foot) {
      return nullptr;
    }
    AppendChildTo(foot, true);
  }

  return foot.forget();
}

void
HTMLTableElement::DeleteTFoot()
{
  HTMLTableSectionElement* tFoot = GetTFoot();
  if (tFoot) {
    mozilla::ErrorResult rv;
    nsINode::RemoveChild(*tFoot, rv);
    MOZ_ASSERT(!rv.Failed());
  }
}

already_AddRefed<nsGenericHTMLElement>
HTMLTableElement::CreateCaption()
{
  nsRefPtr<nsGenericHTMLElement> caption = GetCaption();
  if (!caption) {
    
    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::caption,
                                getter_AddRefs(nodeInfo));

    caption = NS_NewHTMLTableCaptionElement(nodeInfo.forget());
    if (!caption) {
      return nullptr;
    }

    AppendChildTo(caption, true);
  }
  return caption.forget();
}

void
HTMLTableElement::DeleteCaption()
{
  HTMLTableCaptionElement* caption = GetCaption();
  if (caption) {
    mozilla::ErrorResult rv;
    nsINode::RemoveChild(*caption, rv);
    MOZ_ASSERT(!rv.Failed());
  }
}

already_AddRefed<nsGenericHTMLElement>
HTMLTableElement::CreateTBody()
{
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo =
    OwnerDoc()->NodeInfoManager()->GetNodeInfo(nsGkAtoms::tbody, nullptr,
                                               kNameSpaceID_XHTML,
                                               nsIDOMNode::ELEMENT_NODE);
  MOZ_ASSERT(nodeInfo);

  nsRefPtr<nsGenericHTMLElement> newBody =
    NS_NewHTMLTableSectionElement(nodeInfo.forget());
  MOZ_ASSERT(newBody);

  nsIContent* referenceNode = nullptr;
  for (nsIContent* child = nsINode::GetLastChild();
       child;
       child = child->GetPreviousSibling()) {
    if (child->IsHTMLElement(nsGkAtoms::tbody)) {
      referenceNode = child->GetNextSibling();
      break;
    }
  }

  ErrorResult rv;
  nsINode::InsertBefore(*newBody, referenceNode, rv);

  return newBody.forget();
}

already_AddRefed<nsGenericHTMLElement>
HTMLTableElement::InsertRow(int32_t aIndex, ErrorResult& aError)
{
  







  if (aIndex < -1) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  nsIHTMLCollection* rows = Rows();
  uint32_t rowCount = rows->Length();
  if ((uint32_t)aIndex > rowCount && aIndex != -1) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return nullptr;
  }

  
  uint32_t refIndex = (uint32_t)aIndex;

  nsRefPtr<nsGenericHTMLElement> newRow;
  if (rowCount > 0) {
    if (refIndex == rowCount || aIndex == -1) {
      
      

      refIndex = rowCount - 1;
    }

    Element* refRow = rows->Item(refIndex);
    nsINode* parent = refRow->GetParentNode();

    
    nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                                getter_AddRefs(nodeInfo));

    newRow = NS_NewHTMLTableRowElement(nodeInfo.forget());

    if (newRow) {
      
      
      if (aIndex == -1 || uint32_t(aIndex) == rowCount) {
        parent->AppendChild(*newRow, aError);
      } else {
        
        parent->InsertBefore(*newRow, refRow, aError);
      }

      if (aError.Failed()) {
        return nullptr;
      }
    }
  } else {
    
    
    nsCOMPtr<nsIContent> rowGroup;
    for (nsIContent* child = nsINode::GetLastChild();
         child;
         child = child->GetPreviousSibling()) {
      if (child->IsHTMLElement(nsGkAtoms::tbody)) {
        rowGroup = child;
        break;
      }
    }

    if (!rowGroup) { 
      nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
      nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tbody,
                                  getter_AddRefs(nodeInfo));

      rowGroup = NS_NewHTMLTableSectionElement(nodeInfo.forget());
      if (rowGroup) {
        aError = AppendChildTo(rowGroup, true);
        if (aError.Failed()) {
          return nullptr;
        }
      }
    }

    if (rowGroup) {
      nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
      nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                                  getter_AddRefs(nodeInfo));

      newRow = NS_NewHTMLTableRowElement(nodeInfo.forget());
      if (newRow) {
        HTMLTableSectionElement* section =
          static_cast<HTMLTableSectionElement*>(rowGroup.get());
        nsIHTMLCollection* rows = section->Rows();
        rowGroup->InsertBefore(*newRow, rows->Item(0), aError);
      }
    }
  }

  return newRow.forget();
}

void
HTMLTableElement::DeleteRow(int32_t aIndex, ErrorResult& aError)
{
  if (aIndex < -1) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  nsIHTMLCollection* rows = Rows();
  uint32_t refIndex;
  if (aIndex == -1) {
    refIndex = rows->Length();
    if (refIndex == 0) {
      return;
    }

    --refIndex;
  } else {
    refIndex = (uint32_t)aIndex;
  }

  nsCOMPtr<nsIContent> row = rows->Item(refIndex);
  if (!row) {
    aError.Throw(NS_ERROR_DOM_INDEX_SIZE_ERR);
    return;
  }

  row->RemoveFromParent();
}

bool
HTMLTableElement::ParseAttribute(int32_t aNamespaceID,
                                 nsIAtom* aAttribute,
                                 const nsAString& aValue,
                                 nsAttrValue& aResult)
{
  
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::cellspacing ||
        aAttribute == nsGkAtoms::cellpadding ||
        aAttribute == nsGkAtoms::border) {
      return aResult.ParseNonNegativeIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::width) {
      if (aResult.ParseSpecialIntValue(aValue)) {
        
        nsAttrValue::ValueType type = aResult.Type();
        return !((type == nsAttrValue::eInteger &&
                  aResult.GetIntegerValue() == 0) ||
                 (type == nsAttrValue::ePercent &&
                  aResult.GetPercentValue() == 0.0f));
      }
      return false;
    }
    
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::bgcolor ||
        aAttribute == nsGkAtoms::bordercolor) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::hspace ||
        aAttribute == nsGkAtoms::vspace) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
  }

  return nsGenericHTMLElement::ParseBackgroundAttribute(aNamespaceID,
                                                        aAttribute, aValue,
                                                        aResult) ||
         nsGenericHTMLElement::ParseAttribute(aNamespaceID, aAttribute, aValue,
                                              aResult);
}



void
HTMLTableElement::MapAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                        nsRuleData* aData)
{
  
  
  
  
  
  
  
  
  

  nsPresContext* presContext = aData->mPresContext;
  nsCompatibility mode = presContext->CompatibilityMode();

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TableBorder)) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::cellspacing);
    nsCSSValue* borderSpacing = aData->ValueForBorderSpacing();
    if (value && value->Type() == nsAttrValue::eInteger &&
        borderSpacing->GetUnit() == eCSSUnit_Null) {
      borderSpacing->
        SetFloatValue(float(value->GetIntegerValue()), eCSSUnit_Pixel);
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Margin)) {
    
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);

    if (value && value->Type() == nsAttrValue::eEnum) {
      if (value->GetEnumValue() == NS_STYLE_TEXT_ALIGN_CENTER ||
          value->GetEnumValue() == NS_STYLE_TEXT_ALIGN_MOZ_CENTER) {
        nsCSSValue* marginLeft = aData->ValueForMarginLeft();
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetAutoValue();
        nsCSSValue* marginRight = aData->ValueForMarginRight();
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetAutoValue();
      }
    }

    
    
    
    if (eCompatibility_NavQuirks == mode) {
      value = aAttributes->GetAttr(nsGkAtoms::hspace);

      if (value && value->Type() == nsAttrValue::eInteger) {
        nsCSSValue* marginLeft = aData->ValueForMarginLeft();
        if (marginLeft->GetUnit() == eCSSUnit_Null)
          marginLeft->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel); 
        nsCSSValue* marginRight = aData->ValueForMarginRight();
        if (marginRight->GetUnit() == eCSSUnit_Null)
          marginRight->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      }

      value = aAttributes->GetAttr(nsGkAtoms::vspace);

      if (value && value->Type() == nsAttrValue::eInteger) {
        nsCSSValue* marginTop = aData->ValueForMarginTop();
        if (marginTop->GetUnit() == eCSSUnit_Null)
          marginTop->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel); 
        nsCSSValue* marginBottom = aData->ValueForMarginBottom();
        if (marginBottom->GetUnit() == eCSSUnit_Null)
          marginBottom->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel); 
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    
    nsCSSValue* width = aData->ValueForWidth();
    if (width->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::width);
      if (value && value->Type() == nsAttrValue::eInteger)
        width->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        width->SetPercentValue(value->GetPercentValue());
    }

    
    nsCSSValue* height = aData->ValueForHeight();
    if (height->GetUnit() == eCSSUnit_Null) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::height);
      if (value && value->Type() == nsAttrValue::eInteger)
        height->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      else if (value && value->Type() == nsAttrValue::ePercent)
        height->SetPercentValue(value->GetPercentValue());
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::bordercolor);
    nscolor color;
    if (value && presContext->UseDocumentColors() &&
        value->GetColorValue(color)) {
      nsCSSValue* borderLeftColor = aData->ValueForBorderLeftColor();
      if (borderLeftColor->GetUnit() == eCSSUnit_Null)
        borderLeftColor->SetColorValue(color);
      nsCSSValue* borderRightColor = aData->ValueForBorderRightColor();
      if (borderRightColor->GetUnit() == eCSSUnit_Null)
        borderRightColor->SetColorValue(color);
      nsCSSValue* borderTopColor = aData->ValueForBorderTopColor();
      if (borderTopColor->GetUnit() == eCSSUnit_Null)
        borderTopColor->SetColorValue(color);
      nsCSSValue* borderBottomColor = aData->ValueForBorderBottomColor();
      if (borderBottomColor->GetUnit() == eCSSUnit_Null)
        borderBottomColor->SetColorValue(color);
    }

    
    const nsAttrValue* borderValue = aAttributes->GetAttr(nsGkAtoms::border);
    if (borderValue) {
      
      int32_t borderThickness = 1;

      if (borderValue->Type() == nsAttrValue::eInteger)
        borderThickness = borderValue->GetIntegerValue();

      
      nsCSSValue* borderLeftWidth = aData->ValueForBorderLeftWidth();
      if (borderLeftWidth->GetUnit() == eCSSUnit_Null)
        borderLeftWidth->SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
      nsCSSValue* borderRightWidth = aData->ValueForBorderRightWidth();
      if (borderRightWidth->GetUnit() == eCSSUnit_Null)
        borderRightWidth->SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
      nsCSSValue* borderTopWidth = aData->ValueForBorderTopWidth();
      if (borderTopWidth->GetUnit() == eCSSUnit_Null)
        borderTopWidth->SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
      nsCSSValue* borderBottomWidth = aData->ValueForBorderBottomWidth();
      if (borderBottomWidth->GetUnit() == eCSSUnit_Null)
        borderBottomWidth->SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
    }
  }
  nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
}

NS_IMETHODIMP_(bool)
HTMLTableElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::cellpadding },
    { &nsGkAtoms::cellspacing },
    { &nsGkAtoms::border },
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { &nsGkAtoms::hspace },
    { &nsGkAtoms::vspace },
    
    { &nsGkAtoms::bordercolor },
    
    { &nsGkAtoms::align },
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
HTMLTableElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}

static void
MapInheritedTableAttributesIntoRule(const nsMappedAttributes* aAttributes,
                                    nsRuleData* aData)
{
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Padding)) {
    const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::cellpadding);
    if (value && value->Type() == nsAttrValue::eInteger) {
      
      
      nsCSSValue padVal(float(value->GetIntegerValue()), eCSSUnit_Pixel);

      nsCSSValue* paddingLeft = aData->ValueForPaddingLeft();
      if (paddingLeft->GetUnit() == eCSSUnit_Null) {
        *paddingLeft = padVal;
      }

      nsCSSValue* paddingRight = aData->ValueForPaddingRight();
      if (paddingRight->GetUnit() == eCSSUnit_Null) {
        *paddingRight = padVal;
      }

      nsCSSValue* paddingTop = aData->ValueForPaddingTop();
      if (paddingTop->GetUnit() == eCSSUnit_Null) {
        *paddingTop = padVal;
      }

      nsCSSValue* paddingBottom = aData->ValueForPaddingBottom();
      if (paddingBottom->GetUnit() == eCSSUnit_Null) {
        *paddingBottom = padVal;
      }
    }
  }
}

nsMappedAttributes*
HTMLTableElement::GetAttributesMappedForCell()
{
  if (mTableInheritedAttributes) {
    if (mTableInheritedAttributes == TABLE_ATTRS_DIRTY)
      BuildInheritedAttributes();
    if (mTableInheritedAttributes != TABLE_ATTRS_DIRTY)
      return mTableInheritedAttributes;
  }
  return nullptr;
}

void
HTMLTableElement::BuildInheritedAttributes()
{
  NS_ASSERTION(mTableInheritedAttributes == TABLE_ATTRS_DIRTY,
               "potential leak, plus waste of work");
  nsIDocument *document = GetComposedDoc();
  nsHTMLStyleSheet* sheet = document ?
                              document->GetAttributeStyleSheet() : nullptr;
  nsRefPtr<nsMappedAttributes> newAttrs;
  if (sheet) {
    const nsAttrValue* value = mAttrsAndChildren.GetAttr(nsGkAtoms::cellpadding);
    if (value) {
      nsRefPtr<nsMappedAttributes> modifiableMapped = new
      nsMappedAttributes(sheet, MapInheritedTableAttributesIntoRule);

      if (modifiableMapped) {
        nsAttrValue val(*value);
        modifiableMapped->SetAndTakeAttr(nsGkAtoms::cellpadding, val);
      }
      newAttrs = sheet->UniqueMappedAttributes(modifiableMapped);
      NS_ASSERTION(newAttrs, "out of memory, but handling gracefully");

      if (newAttrs != modifiableMapped) {
        
        
        
        
        
        modifiableMapped->DropStyleSheetReference();
      }
    }
    mTableInheritedAttributes = newAttrs;
    NS_IF_ADDREF(mTableInheritedAttributes);
  }
}

void
HTMLTableElement::ReleaseInheritedAttributes()
{
  if (mTableInheritedAttributes &&
      mTableInheritedAttributes != TABLE_ATTRS_DIRTY)
    NS_RELEASE(mTableInheritedAttributes);
  mTableInheritedAttributes = TABLE_ATTRS_DIRTY;
}

nsresult
HTMLTableElement::BindToTree(nsIDocument* aDocument, nsIContent* aParent,
                             nsIContent* aBindingParent,
                             bool aCompileEventHandlers)
{
  ReleaseInheritedAttributes();
  return nsGenericHTMLElement::BindToTree(aDocument, aParent,
                                          aBindingParent,
                                          aCompileEventHandlers);
}

void
HTMLTableElement::UnbindFromTree(bool aDeep, bool aNullParent)
{
  ReleaseInheritedAttributes();
  nsGenericHTMLElement::UnbindFromTree(aDeep, aNullParent);
}

nsresult
HTMLTableElement::BeforeSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                                const nsAttrValueOrString* aValue,
                                bool aNotify)
{
  if (aName == nsGkAtoms::cellpadding && aNameSpaceID == kNameSpaceID_None) {
    ReleaseInheritedAttributes();
  }
  return nsGenericHTMLElement::BeforeSetAttr(aNameSpaceID, aName, aValue,
                                             aNotify);
}

nsresult
HTMLTableElement::AfterSetAttr(int32_t aNameSpaceID, nsIAtom* aName,
                               const nsAttrValue* aValue,
                               bool aNotify)
{
  if (aName == nsGkAtoms::cellpadding && aNameSpaceID == kNameSpaceID_None) {
    BuildInheritedAttributes();
  }
  return nsGenericHTMLElement::AfterSetAttr(aNameSpaceID, aName, aValue,
                                            aNotify);
}

} 
} 
