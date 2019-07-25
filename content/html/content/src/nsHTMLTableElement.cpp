



































#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableCaptionElem.h"
#include "nsIDOMHTMLTableSectionElem.h"
#include "nsCOMPtr.h"
#include "nsIDOMEventTarget.h"
#include "nsDOMError.h"
#include "nsContentList.h"
#include "nsMappedAttributes.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsPresContext.h"
#include "nsHTMLParts.h"
#include "nsRuleData.h"
#include "nsStyleContext.h"
#include "nsIDocument.h"


#include "nsIDOMElement.h"
#include "nsGenericHTMLElement.h"
#include "nsIHTMLCollection.h"


class TableRowsCollection;

class nsHTMLTableElement :  public nsGenericHTMLElement,
                            public nsIDOMHTMLTableElement
{
public:
  nsHTMLTableElement(already_AddRefed<nsINodeInfo> aNodeInfo);
  virtual ~nsHTMLTableElement();

  
  NS_DECL_ISUPPORTS_INHERITED

  
  NS_FORWARD_NSIDOMNODE(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMELEMENT(nsGenericHTMLElement::)

  
  NS_FORWARD_NSIDOMHTMLELEMENT(nsGenericHTMLElement::)

  
  NS_DECL_NSIDOMHTMLTABLEELEMENT

  virtual PRBool ParseAttribute(PRInt32 aNamespaceID,
                                nsIAtom* aAttribute,
                                const nsAString& aValue,
                                nsAttrValue& aResult);
  virtual nsMapRuleToAttributesFunc GetAttributeMappingFunction() const;
  NS_IMETHOD_(PRBool) IsAttributeMapped(const nsIAtom* aAttribute) const;

  virtual nsresult Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const;

  virtual nsXPCClassInfo* GetClassInfo();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED_NO_UNLINK(nsHTMLTableElement,
                                                     nsGenericHTMLElement)

  already_AddRefed<nsIDOMHTMLTableSectionElement> GetTHead() {
    return GetSection(nsGkAtoms::thead);
  }
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetTFoot() {
    return GetSection(nsGkAtoms::tfoot);
  }
  nsContentList* TBodies();
protected:
  already_AddRefed<nsIDOMHTMLTableSectionElement> GetSection(nsIAtom *aTag);

  nsRefPtr<nsContentList> mTBodies;
  nsRefPtr<TableRowsCollection> mRows;
};







class TableRowsCollection : public nsIHTMLCollection 
{
public:
  TableRowsCollection(nsHTMLTableElement *aParent);
  virtual ~TableRowsCollection();

  nsresult Init();

  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIDOMHTMLCOLLECTION

  virtual nsIContent* GetNodeAt(PRUint32 aIndex);
  virtual nsISupports* GetNamedItem(const nsAString& aName,
                                    nsWrapperCache **aCache);

  NS_IMETHOD    ParentDestroyed();

  NS_DECL_CYCLE_COLLECTION_CLASS(TableRowsCollection)

protected:
  
  nsRefPtr<nsContentList> mOrphanRows;  
  nsHTMLTableElement * mParent;
};


TableRowsCollection::TableRowsCollection(nsHTMLTableElement *aParent)
  : mParent(aParent)
{
}

TableRowsCollection::~TableRowsCollection()
{
  
  
  
  
}

NS_IMPL_CYCLE_COLLECTION_CLASS(TableRowsCollection)
NS_IMPL_CYCLE_COLLECTION_UNLINK_0(TableRowsCollection)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(TableRowsCollection)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mOrphanRows,
                                                       nsIDOMNodeList)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(TableRowsCollection)
NS_IMPL_CYCLE_COLLECTING_RELEASE(TableRowsCollection)

NS_INTERFACE_TABLE_HEAD(TableRowsCollection)
  NS_INTERFACE_TABLE2(TableRowsCollection, nsIHTMLCollection,
                      nsIDOMHTMLCollection)
  NS_INTERFACE_TABLE_TO_MAP_SEGUE_CYCLE_COLLECTION(TableRowsCollection)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(HTMLCollection)
NS_INTERFACE_MAP_END

nsresult
TableRowsCollection::Init()
{
  mOrphanRows = new nsContentList(mParent,
                                  mParent->NodeInfo()->NamespaceID(),
                                  nsGkAtoms::tr,
                                  nsGkAtoms::tr,
                                  PR_FALSE);
  return NS_OK;
}






#define DO_FOR_EACH_ROWGROUP(_code)                                  \
  do {                                                               \
    if (mParent) {                                                   \
                                                          \
      nsCOMPtr<nsIDOMHTMLTableSectionElement> rowGroup;              \
      rowGroup = mParent->GetTHead();                                \
      nsCOMPtr<nsIDOMHTMLCollection> rows;                           \
      if (rowGroup) {                                                \
        rowGroup->GetRows(getter_AddRefs(rows));                     \
        do {                                      \
          _code                                                      \
        } while (0);                                                 \
      }                                                              \
                                                        \
      nsContentList *_tbodies = mParent->TBodies();                  \
      nsINode * _node;                                               \
      PRUint32 _tbodyIndex = 0;                                      \
      _node = _tbodies->GetNodeAt(_tbodyIndex);                      \
      while (_node) {                                                \
        rowGroup = do_QueryInterface(_node);                         \
        if (rowGroup) {                                              \
          rowGroup->GetRows(getter_AddRefs(rows));                   \
          do {                                    \
            _code                                                    \
          } while (0);                                               \
        }                                                            \
        _node = _tbodies->GetNodeAt(++_tbodyIndex);                  \
      }                                                              \
                                                    \
      rows = mOrphanRows;                                            \
      do {                                        \
        _code                                                        \
      } while (0);                                                   \
                                                          \
      rowGroup = mParent->GetTFoot();                                \
      rows = nsnull;                                                 \
      if (rowGroup) {                                                \
        rowGroup->GetRows(getter_AddRefs(rows));                     \
        do {                                      \
          _code                                                      \
        } while (0);                                                 \
      }                                                              \
    }                                                                \
  } while (0)

static PRUint32
CountRowsInRowGroup(nsIDOMHTMLCollection* rows)
{
  PRUint32 length = 0;
  
  if (rows) {
    rows->GetLength(&length);
  }
  
  return length;
}




NS_IMETHODIMP 
TableRowsCollection::GetLength(PRUint32* aLength)
{
  *aLength=0;

  DO_FOR_EACH_ROWGROUP(
    *aLength += CountRowsInRowGroup(rows);
  );

  return NS_OK;
}




static nsIContent*
GetItemOrCountInRowGroup(nsIDOMHTMLCollection* rows,
                         PRUint32 aIndex, PRUint32* aCount)
{
  *aCount = 0;

  if (rows) {
    rows->GetLength(aCount);
    if (aIndex < *aCount) {
      nsCOMPtr<nsINodeList> list = do_QueryInterface(rows);
      return list->GetNodeAt(aIndex);
    }
  }
  
  return nsnull;
}

nsIContent*
TableRowsCollection::GetNodeAt(PRUint32 aIndex)
{
  DO_FOR_EACH_ROWGROUP(
    PRUint32 count;
    nsIContent* node = GetItemOrCountInRowGroup(rows, aIndex, &count);
    if (node) {
      return node; 
    }

    NS_ASSERTION(count <= aIndex, "GetItemOrCountInRowGroup screwed up");
    aIndex -= count;
  );

  return nsnull;
}

NS_IMETHODIMP 
TableRowsCollection::Item(PRUint32 aIndex, nsIDOMNode** aReturn)
{
  nsISupports* node = GetNodeAt(aIndex);
  if (!node) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(node, aReturn);
}

static nsISupports*
GetNamedItemInRowGroup(nsIDOMHTMLCollection* aRows, const nsAString& aName,
                       nsWrapperCache** aCache)
{
  nsCOMPtr<nsIHTMLCollection> rows = do_QueryInterface(aRows);
  if (rows) {
    return rows->GetNamedItem(aName, aCache);
  }

  return nsnull;
}

nsISupports* 
TableRowsCollection::GetNamedItem(const nsAString& aName,
                                  nsWrapperCache** aCache)
{
  DO_FOR_EACH_ROWGROUP(
    nsISupports* item = GetNamedItemInRowGroup(rows, aName, aCache);
    if (item) {
      return item;
    }
  );
  *aCache = nsnull;
  return nsnull;
}

NS_IMETHODIMP 
TableRowsCollection::NamedItem(const nsAString& aName,
                               nsIDOMNode** aReturn)
{
  nsWrapperCache *cache;
  nsISupports* item = GetNamedItem(aName, &cache);
  if (!item) {
    *aReturn = nsnull;

    return NS_OK;
  }

  return CallQueryInterface(item, aReturn);
}

NS_IMETHODIMP
TableRowsCollection::ParentDestroyed()
{
  
  mParent = nsnull;

  return NS_OK;
}





NS_IMPL_NS_NEW_HTML_ELEMENT(Table)


nsHTMLTableElement::nsHTMLTableElement(already_AddRefed<nsINodeInfo> aNodeInfo)
  : nsGenericHTMLElement(aNodeInfo)
{
}

nsHTMLTableElement::~nsHTMLTableElement()
{
  if (mRows) {
    mRows->ParentDestroyed();
  }
}


NS_IMPL_CYCLE_COLLECTION_CLASS(nsHTMLTableElement)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsHTMLTableElement,
                                                  nsGenericHTMLElement)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR_AMBIGUOUS(mTBodies,
                                                       nsIDOMNodeList)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mRows)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(nsHTMLTableElement, nsGenericElement) 
NS_IMPL_RELEASE_INHERITED(nsHTMLTableElement, nsGenericElement) 


DOMCI_NODE_DATA(HTMLTableElement, nsHTMLTableElement)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLTableElement)
  NS_HTML_CONTENT_INTERFACE_TABLE1(nsHTMLTableElement, nsIDOMHTMLTableElement)
  NS_HTML_CONTENT_INTERFACE_TABLE_TO_MAP_SEGUE(nsHTMLTableElement,
                                               nsGenericHTMLElement)
NS_HTML_CONTENT_INTERFACE_TABLE_TAIL_CLASSINFO(HTMLTableElement)


NS_IMPL_ELEMENT_CLONE(nsHTMLTableElement)






NS_IMPL_STRING_ATTR(nsHTMLTableElement, Align, align)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, BgColor, bgcolor)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Border, border)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, CellPadding, cellpadding)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, CellSpacing, cellspacing)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Frame, frame)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Rules, rules)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Summary, summary)
NS_IMPL_STRING_ATTR(nsHTMLTableElement, Width, width)


NS_IMETHODIMP
nsHTMLTableElement::GetCaption(nsIDOMHTMLTableCaptionElement** aValue)
{
  *aValue = nsnull;
  nsCOMPtr<nsIDOMNode> child;
  GetFirstChild(getter_AddRefs(child));

  while (child) {
    nsCOMPtr<nsIDOMHTMLTableCaptionElement> caption(do_QueryInterface(child));

    if (caption) {
      *aValue = caption;
      NS_ADDREF(*aValue);

      break;
    }

    nsIDOMNode *temp = child.get();
    temp->GetNextSibling(getter_AddRefs(child));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetCaption(nsIDOMHTMLTableCaptionElement* aValue)
{
  nsresult rv = DeleteCaption();

  if (NS_SUCCEEDED(rv)) {
    if (aValue) {
      nsCOMPtr<nsIDOMNode> resultingChild;
      AppendChild(aValue, getter_AddRefs(resultingChild));
    }
  }

  return rv;
}

already_AddRefed<nsIDOMHTMLTableSectionElement>
nsHTMLTableElement::GetSection(nsIAtom *aTag)
{
  PRUint32 childCount = GetChildCount();

  nsCOMPtr<nsIDOMHTMLTableSectionElement> section;

  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *child = GetChildAt(i);

    section = do_QueryInterface(child);

    if (section && child->NodeInfo()->Equals(aTag)) {
      nsIDOMHTMLTableSectionElement *result = section;
      NS_ADDREF(result);

      return result;
    }
  }

  return nsnull;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTHead(nsIDOMHTMLTableSectionElement** aValue)
{
  *aValue = GetTHead().get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetTHead(nsIDOMHTMLTableSectionElement* aValue)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aValue));
  NS_ENSURE_TRUE(content, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

  if (!content->NodeInfo()->Equals(nsGkAtoms::thead)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  
  nsresult rv = DeleteTHead();
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (aValue) {
    nsCOMPtr<nsIDOMNode> child;
    rv = GetFirstChild(getter_AddRefs(child));
    if (NS_FAILED(rv)) {
      return rv;
    }
     
    nsCOMPtr<nsIDOMNode> resultChild;
    rv = InsertBefore(aValue, child, getter_AddRefs(resultChild));
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTFoot(nsIDOMHTMLTableSectionElement** aValue)
{
  *aValue = GetTFoot().get();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::SetTFoot(nsIDOMHTMLTableSectionElement* aValue)
{
  nsCOMPtr<nsIContent> content(do_QueryInterface(aValue));
  NS_ENSURE_TRUE(content, NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);

  if (!content->NodeInfo()->Equals(nsGkAtoms::tfoot)) {
    return NS_ERROR_DOM_HIERARCHY_REQUEST_ERR;
  }
  
  nsresult rv = DeleteTFoot();
  if (NS_SUCCEEDED(rv)) {
    if (aValue) {
      nsCOMPtr<nsIDOMNode> resultingChild;
      AppendChild(aValue, getter_AddRefs(resultingChild));
    }
  }

  return rv;
}

NS_IMETHODIMP
nsHTMLTableElement::GetRows(nsIDOMHTMLCollection** aValue)
{
  if (!mRows) {
    
    mRows = new TableRowsCollection(this);
    NS_ENSURE_TRUE(mRows, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = mRows->Init();
    if (NS_FAILED(rv)) {
      mRows = nsnull;
      return rv;
    }
  }

  *aValue = mRows;
  NS_ADDREF(*aValue);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::GetTBodies(nsIDOMHTMLCollection** aValue)
{
  NS_ADDREF(*aValue = TBodies());
  return NS_OK;
}

nsContentList*
nsHTMLTableElement::TBodies()
{
  if (!mTBodies) {
    
    mTBodies = new nsContentList(this,
                                 mNodeInfo->NamespaceID(),
                                 nsGkAtoms::tbody,
                                 nsGkAtoms::tbody,
                                 PR_FALSE);
  }

  return mTBodies;
}

NS_IMETHODIMP
nsHTMLTableElement::CreateTHead(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableSectionElement> head;

  GetTHead(getter_AddRefs(head));

  if (head) { 
    CallQueryInterface(head, aValue);

    NS_ASSERTION(*aValue, "head must be a DOMHTMLElement");
  }
  else
  { 
    nsCOMPtr<nsINodeInfo> nodeInfo;

    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::thead,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> newHead = NS_NewHTMLTableSectionElement(nodeInfo.forget());

    if (newHead) {
      nsCOMPtr<nsIDOMNode> child;

      rv = GetFirstChild(getter_AddRefs(child));

      if (NS_FAILED(rv)) {
        return rv;
      }

      CallQueryInterface(newHead, aValue);

      nsCOMPtr<nsIDOMNode> resultChild;
      rv = InsertBefore(*aValue, child, getter_AddRefs(resultChild));
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteTHead()
{
  nsCOMPtr<nsIDOMHTMLTableSectionElement> childToDelete;
  nsresult rv = GetTHead(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::CreateTFoot(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableSectionElement> foot;

  GetTFoot(getter_AddRefs(foot));

  if (foot) { 
    CallQueryInterface(foot, aValue);

    NS_ASSERTION(*aValue, "foot must be a DOMHTMLElement");
  }
  else
  { 
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tfoot,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> newFoot = NS_NewHTMLTableSectionElement(nodeInfo.forget());

    if (newFoot) {
      rv = AppendChildTo(newFoot, PR_TRUE);
      CallQueryInterface(newFoot, aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteTFoot()
{
  nsCOMPtr<nsIDOMHTMLTableSectionElement> childToDelete;
  nsresult rv = GetTFoot(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::CreateCaption(nsIDOMHTMLElement** aValue)
{
  *aValue = nsnull;
  nsresult rv = NS_OK;
  nsCOMPtr<nsIDOMHTMLTableCaptionElement> caption;

  GetCaption(getter_AddRefs(caption));

  if (caption) { 
    CallQueryInterface(caption, aValue);

    NS_ASSERTION(*aValue, "caption must be a DOMHTMLElement");
  }
  else
  { 
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::caption,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> newCaption = NS_NewHTMLTableCaptionElement(nodeInfo.forget());

    if (newCaption) {
      rv = AppendChildTo(newCaption, PR_TRUE);
      CallQueryInterface(newCaption, aValue);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteCaption()
{
  nsCOMPtr<nsIDOMHTMLTableCaptionElement> childToDelete;
  nsresult rv = GetCaption(getter_AddRefs(childToDelete));

  if ((NS_SUCCEEDED(rv)) && childToDelete) {
    nsCOMPtr<nsIDOMNode> resultingChild;
    RemoveChild(childToDelete, getter_AddRefs(resultingChild));
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::InsertRow(PRInt32 aIndex, nsIDOMHTMLElement** aValue)
{
  







  *aValue = nsnull;

  if (aIndex < -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  nsresult rv;

  nsCOMPtr<nsIDOMHTMLCollection> rows;
  GetRows(getter_AddRefs(rows));

  PRUint32 rowCount;
  rows->GetLength(&rowCount);

  if ((PRUint32)aIndex > rowCount && aIndex != -1) {
    return NS_ERROR_DOM_INDEX_SIZE_ERR;
  }

  
  PRUint32 refIndex = (PRUint32)aIndex;

  if (rowCount > 0) {
    if (refIndex == rowCount || aIndex == -1) {
      
      

      refIndex = rowCount - 1;
    }

    nsCOMPtr<nsIDOMNode> refRow;
    rows->Item(refIndex, getter_AddRefs(refRow));

    nsCOMPtr<nsIDOMNode> parent;

    refRow->GetParentNode(getter_AddRefs(parent));
    
    nsCOMPtr<nsINodeInfo> nodeInfo;
    nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                                getter_AddRefs(nodeInfo));

    nsCOMPtr<nsIContent> newRow = NS_NewHTMLTableRowElement(nodeInfo.forget());

    if (newRow) {
      nsCOMPtr<nsIDOMNode> newRowNode(do_QueryInterface(newRow));
      nsCOMPtr<nsIDOMNode> retChild;

      
      
      if (aIndex == -1 || PRUint32(aIndex) == rowCount) {
        rv = parent->AppendChild(newRowNode, getter_AddRefs(retChild));
      }
      else
      {
        
        rv = parent->InsertBefore(newRowNode, refRow,
                                  getter_AddRefs(retChild));
      }

      if (retChild) {
        CallQueryInterface(retChild, aValue);
      }
    }
  }
  else
  { 
    
    nsCOMPtr<nsIDOMNode> rowGroup;

    PRInt32 namespaceID = mNodeInfo->NamespaceID();
    PRUint32 childCount = GetChildCount();
    for (PRUint32 i = 0; i < childCount; ++i) {
      nsIContent* child = GetChildAt(i);
      nsINodeInfo *childInfo = child->NodeInfo();
      nsIAtom *localName = childInfo->NameAtom();
      if (childInfo->NamespaceID() == namespaceID &&
          (localName == nsGkAtoms::thead ||
           localName == nsGkAtoms::tbody ||
           localName == nsGkAtoms::tfoot)) {
        rowGroup = do_QueryInterface(child);
        NS_ASSERTION(rowGroup, "HTML node did not QI to nsIDOMNode");
        break;
      }
    }

    if (!rowGroup) { 
      nsCOMPtr<nsINodeInfo> nodeInfo;
      nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tbody,
                                  getter_AddRefs(nodeInfo));

      nsCOMPtr<nsIContent> newRowGroup =
        NS_NewHTMLTableSectionElement(nodeInfo.forget());

      if (newRowGroup) {
        rv = AppendChildTo(newRowGroup, PR_TRUE);

        rowGroup = do_QueryInterface(newRowGroup);
      }
    }

    if (rowGroup) {
      nsCOMPtr<nsINodeInfo> nodeInfo;
      nsContentUtils::NameChanged(mNodeInfo, nsGkAtoms::tr,
                                  getter_AddRefs(nodeInfo));

      nsCOMPtr<nsIContent> newRow = NS_NewHTMLTableRowElement(nodeInfo.forget());
      if (newRow) {
        nsCOMPtr<nsIDOMNode> firstRow;

        nsCOMPtr<nsIDOMHTMLTableSectionElement> section =
          do_QueryInterface(rowGroup);

        if (section) {
          nsCOMPtr<nsIDOMHTMLCollection> rows;
          section->GetRows(getter_AddRefs(rows));
          if (rows) {
            rows->Item(0, getter_AddRefs(firstRow));
          }
        }
        
        nsCOMPtr<nsIDOMNode> retNode, newRowNode(do_QueryInterface(newRow));

        rowGroup->InsertBefore(newRowNode, firstRow, getter_AddRefs(retNode));

        if (retNode) {
          CallQueryInterface(retNode, aValue);
        }
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLTableElement::DeleteRow(PRInt32 aValue)
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

  nsCOMPtr<nsIDOMNode> parent;
  row->GetParentNode(getter_AddRefs(parent));
  NS_ENSURE_TRUE(parent, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIDOMNode> deleted_row;
  return parent->RemoveChild(row, getter_AddRefs(deleted_row));
}

static const nsAttrValue::EnumTable kFrameTable[] = {
  { "void",   NS_STYLE_TABLE_FRAME_NONE },
  { "above",  NS_STYLE_TABLE_FRAME_ABOVE },
  { "below",  NS_STYLE_TABLE_FRAME_BELOW },
  { "hsides", NS_STYLE_TABLE_FRAME_HSIDES },
  { "lhs",    NS_STYLE_TABLE_FRAME_LEFT },
  { "rhs",    NS_STYLE_TABLE_FRAME_RIGHT },
  { "vsides", NS_STYLE_TABLE_FRAME_VSIDES },
  { "box",    NS_STYLE_TABLE_FRAME_BOX },
  { "border", NS_STYLE_TABLE_FRAME_BORDER },
  { 0 }
};

static const nsAttrValue::EnumTable kRulesTable[] = {
  { "none",   NS_STYLE_TABLE_RULES_NONE },
  { "groups", NS_STYLE_TABLE_RULES_GROUPS },
  { "rows",   NS_STYLE_TABLE_RULES_ROWS },
  { "cols",   NS_STYLE_TABLE_RULES_COLS },
  { "all",    NS_STYLE_TABLE_RULES_ALL },
  { 0 }
};

static const nsAttrValue::EnumTable kLayoutTable[] = {
  { "auto",   NS_STYLE_TABLE_LAYOUT_AUTO },
  { "fixed",  NS_STYLE_TABLE_LAYOUT_FIXED },
  { 0 }
};


PRBool
nsHTMLTableElement::ParseAttribute(PRInt32 aNamespaceID,
                                   nsIAtom* aAttribute,
                                   const nsAString& aValue,
                                   nsAttrValue& aResult)
{
  
  if (aNamespaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::cellspacing ||
        aAttribute == nsGkAtoms::cellpadding) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::cols) {
      return aResult.ParseIntWithBounds(aValue, 0);
    }
    if (aAttribute == nsGkAtoms::border) {
      if (!aResult.ParseIntWithBounds(aValue, 0)) {
        
        aResult.SetTo(1);
      }

      return PR_TRUE;
    }
    if (aAttribute == nsGkAtoms::height) {
      return aResult.ParseSpecialIntValue(aValue);
    }
    if (aAttribute == nsGkAtoms::width) {
      if (aResult.ParseSpecialIntValue(aValue)) {
        
        nsAttrValue::ValueType type = aResult.Type();
        if ((type == nsAttrValue::eInteger &&
             aResult.GetIntegerValue() == 0) ||
            (type == nsAttrValue::ePercent &&
             aResult.GetPercentValue() == 0.0f)) {
          return PR_FALSE;
        }
      }
      return PR_TRUE;
    }
    
    if (aAttribute == nsGkAtoms::align) {
      return ParseTableHAlignValue(aValue, aResult);
    }
    if (aAttribute == nsGkAtoms::bgcolor ||
        aAttribute == nsGkAtoms::bordercolor) {
      return aResult.ParseColor(aValue);
    }
    if (aAttribute == nsGkAtoms::frame) {
      return aResult.ParseEnumValue(aValue, kFrameTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::layout) {
      return aResult.ParseEnumValue(aValue, kLayoutTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::rules) {
      return aResult.ParseEnumValue(aValue, kRulesTable, PR_FALSE);
    }
    if (aAttribute == nsGkAtoms::hspace ||
        aAttribute == nsGkAtoms::vspace) {
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
  
  
  
  
  
  
  
  
  

  nsPresContext* presContext = aData->mPresContext;
  nsCompatibility mode = presContext->CompatibilityMode();

  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(TableBorder)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::cellspacing);
      nsCSSValue* borderSpacing = aData->ValueForBorderSpacing();
      if (value && value->Type() == nsAttrValue::eInteger) {
        if (borderSpacing->GetUnit() == eCSSUnit_Null)
          borderSpacing->
            SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
      }
      else if (value && value->Type() == nsAttrValue::ePercent &&
               eCompatibility_NavQuirks == mode) {
        
        if (borderSpacing->GetUnit() == eCSSUnit_Null)
          borderSpacing->
            SetFloatValue(100.0f * value->GetPercentValue(), eCSSUnit_Pixel);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Table)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      const nsAttrValue* value;
      
      nsCSSValue* tableLayout = aData->ValueForTableLayout();
      if (tableLayout->GetUnit() == eCSSUnit_Null) {
        value = aAttributes->GetAttr(nsGkAtoms::layout);
        if (value && value->Type() == nsAttrValue::eEnum)
          tableLayout->SetIntValue(value->GetEnumValue(), eCSSUnit_Enumerated);
      }
      
      
      value = aAttributes->GetAttr(nsGkAtoms::cols);
      if (value) {
        nsCSSValue* cols = aData->ValueForCols();
        if (value->Type() == nsAttrValue::eInteger) 
          cols->SetIntValue(value->GetIntegerValue(), eCSSUnit_Integer);
        else 
          cols->SetIntValue(NS_STYLE_TABLE_COLS_ALL, eCSSUnit_Enumerated);
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Margin)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::align);

      if (value && value->Type() == nsAttrValue::eEnum) {
        if (value->GetEnumValue() == NS_STYLE_TEXT_ALIGN_CENTER ||
            value->GetEnumValue() == NS_STYLE_TEXT_ALIGN_MOZ_CENTER) {
          nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
          if (marginLeft->GetUnit() == eCSSUnit_Null)
            marginLeft->SetAutoValue();
          nsCSSValue* marginRight = aData->ValueForMarginRightValue();
          if (marginRight->GetUnit() == eCSSUnit_Null)
            marginRight->SetAutoValue();
        }
      }

      
      
      
      if (eCompatibility_NavQuirks == mode) {
        value = aAttributes->GetAttr(nsGkAtoms::hspace);

        if (value && value->Type() == nsAttrValue::eInteger) {
          nsCSSValue* marginLeft = aData->ValueForMarginLeftValue();
          if (marginLeft->GetUnit() == eCSSUnit_Null)
            marginLeft->SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel); 
          nsCSSValue* marginRight = aData->ValueForMarginRightValue();
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
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Padding)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay == NS_STYLE_DISPLAY_TABLE_CELL) {
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::cellpadding);
      if (value) {
        nsAttrValue::ValueType valueType = value->Type();
        if (valueType == nsAttrValue::eInteger || valueType == nsAttrValue::ePercent) {
          
          
          nsCSSValue padVal;
          if (valueType == nsAttrValue::eInteger)
            padVal.SetFloatValue((float)value->GetIntegerValue(), eCSSUnit_Pixel);
          else {
            
            float pctVal = value->GetPercentValue();
            
              
              padVal.SetFloatValue(100.0f * pctVal, eCSSUnit_Pixel);
            
            
            
            
          }
          nsCSSValue* paddingLeft = aData->ValueForPaddingLeftValue();
          if (paddingLeft->GetUnit() == eCSSUnit_Null)
            *paddingLeft = padVal;
          nsCSSValue* paddingRight = aData->ValueForPaddingRightValue();
          if (paddingRight->GetUnit() == eCSSUnit_Null)
            *paddingRight = padVal;
          nsCSSValue* paddingTop = aData->ValueForPaddingTop();
          if (paddingTop->GetUnit() == eCSSUnit_Null)
            *paddingTop = padVal;
          nsCSSValue* paddingBottom = aData->ValueForPaddingBottom();
          if (paddingBottom->GetUnit() == eCSSUnit_Null)
            *paddingBottom = padVal;
        }
      }
    }
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Position)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      
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
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Visibility)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL)
      nsGenericHTMLElement::MapCommonAttributesInto(aAttributes, aData);
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Border)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL) {
      
      const nsAttrValue* value = aAttributes->GetAttr(nsGkAtoms::bordercolor);
      nscolor color;
      if (value && presContext->UseDocumentColors() &&
          value->GetColorValue(color)) {
        nsCSSValue* borderLeftColor = aData->ValueForBorderLeftColorValue();
        if (borderLeftColor->GetUnit() == eCSSUnit_Null)
          borderLeftColor->SetColorValue(color);
        nsCSSValue* borderRightColor = aData->ValueForBorderRightColorValue();
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
        
        PRInt32 borderThickness = 1;

        if (borderValue->Type() == nsAttrValue::eInteger)
          borderThickness = borderValue->GetIntegerValue();

        
        nsCSSValue* borderLeftWidth = aData->ValueForBorderLeftWidthValue();
        if (borderLeftWidth->GetUnit() == eCSSUnit_Null)
          borderLeftWidth->SetFloatValue((float)borderThickness, eCSSUnit_Pixel);
        nsCSSValue* borderRightWidth = aData->ValueForBorderRightWidthValue();
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
  }
  if (aData->mSIDs & NS_STYLE_INHERIT_BIT(Background)) {
    const nsStyleDisplay* readDisplay = aData->mStyleContext->GetStyleDisplay();
  
    if (readDisplay->mDisplay != NS_STYLE_DISPLAY_TABLE_CELL)
      nsGenericHTMLElement::MapBackgroundAttributesInto(aAttributes, aData);
  }
}

NS_IMETHODIMP_(PRBool)
nsHTMLTableElement::IsAttributeMapped(const nsIAtom* aAttribute) const
{
  static const MappedAttributeEntry attributes[] = {
    { &nsGkAtoms::layout },
    { &nsGkAtoms::cellpadding },
    { &nsGkAtoms::cellspacing },
    { &nsGkAtoms::cols },
    { &nsGkAtoms::border },
    { &nsGkAtoms::width },
    { &nsGkAtoms::height },
    { &nsGkAtoms::hspace },
    { &nsGkAtoms::vspace },
    
    { &nsGkAtoms::bordercolor },
    
    { &nsGkAtoms::align },
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
nsHTMLTableElement::GetAttributeMappingFunction() const
{
  return &MapAttributesIntoRule;
}
