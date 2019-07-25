





































#include "nscore.h"
#include "nsIDOMDocument.h"
#include "nsEditor.h"
#include "nsIDOMText.h"
#include "nsIDOMElement.h"
#include "nsIDOMAttr.h"
#include "nsIDOMNode.h"
#include "nsIDOMNodeList.h"
#include "nsIDOMRange.h"
#include "nsIFrame.h"
#include "nsIPresShell.h"
#include "nsISelection.h"
#include "nsISelectionPrivate.h"
#include "nsLayoutCID.h"
#include "nsIContent.h"
#include "nsIContentIterator.h"
#include "nsIAtom.h"
#include "nsIDOMHTMLTableElement.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsITableCellLayout.h" 
#include "nsITableLayout.h"     
#include "nsHTMLEditor.h"
#include "nsISelectionPrivate.h"  
#include "nsTArray.h"

#include "nsEditorUtils.h"
#include "nsTextEditUtils.h"
#include "nsHTMLEditUtils.h"
#include "nsLayoutErrors.h"






class NS_STACK_CLASS nsSetSelectionAfterTableEdit
{
  private:
    nsCOMPtr<nsITableEditor> mEd;
    nsCOMPtr<nsIDOMElement> mTable;
    PRInt32 mCol, mRow, mDirection, mSelected;
  public:
    nsSetSelectionAfterTableEdit(nsITableEditor *aEd, nsIDOMElement* aTable, 
                                 PRInt32 aRow, PRInt32 aCol, PRInt32 aDirection, 
                                 PRBool aSelected) : 
        mEd(do_QueryInterface(aEd))
    { 
      mTable = aTable; 
      mRow = aRow; 
      mCol = aCol; 
      mDirection = aDirection;
      mSelected = aSelected;
    } 
    
    ~nsSetSelectionAfterTableEdit() 
    { 
      if (mEd)
        mEd->SetSelectionAfterTableEdit(mTable, mRow, mCol, mDirection, mSelected);
    }
    
    
    void CancelSetCaret() {mEd = nsnull; mTable = nsnull;}
};


class NS_STACK_CLASS nsSelectionBatcherForTable
{
private:
  nsCOMPtr<nsISelectionPrivate> mSelection;
public:
  nsSelectionBatcherForTable(nsISelection *aSelection)
  {
    nsCOMPtr<nsISelection> sel(aSelection);
    mSelection = do_QueryInterface(sel);
    if (mSelection)  mSelection->StartBatchChanges();
  }
  virtual ~nsSelectionBatcherForTable() 
  { 
    if (mSelection) mSelection->EndBatchChanges();
  }
};



NS_IMETHODIMP
nsHTMLEditor::InsertCell(nsIDOMElement *aCell, PRInt32 aRowSpan, PRInt32 aColSpan, 
                         PRBool aAfter, PRBool aIsHeader, nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nsnull;

  
  nsCOMPtr<nsIDOMNode> cellParent;
  nsresult res = aCell->GetParentNode(getter_AddRefs(cellParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cellParent, NS_ERROR_NULL_POINTER);


  PRInt32 cellOffset;
  res = GetChildOffset(aCell, cellParent, cellOffset);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> newCell;
  if (aIsHeader)
    res = CreateElementWithDefaults(NS_LITERAL_STRING("th"), getter_AddRefs(newCell));
  else
    res = CreateElementWithDefaults(NS_LITERAL_STRING("td"), getter_AddRefs(newCell));
    
  if(NS_FAILED(res)) return res;
  if(!newCell) return NS_ERROR_FAILURE;

  
  if (aNewCell)
  {
    *aNewCell = newCell.get();
    NS_ADDREF(*aNewCell);
  }

  if( aRowSpan > 1)
  {
    
    nsAutoString newRowSpan;
    newRowSpan.AppendInt(aRowSpan, 10);
    newCell->SetAttribute(NS_LITERAL_STRING("rowspan"), newRowSpan);
  }
  if( aColSpan > 1)
  {
    
    nsAutoString newColSpan;
    newColSpan.AppendInt(aColSpan, 10);
    newCell->SetAttribute(NS_LITERAL_STRING("colspan"), newColSpan);
  }
  if(aAfter) cellOffset++;

  
  nsAutoTxnsConserveSelection dontChangeSelection(this);
  return InsertNode(newCell, cellParent, cellOffset);
}

NS_IMETHODIMP nsHTMLEditor::SetColSpan(nsIDOMElement *aCell, PRInt32 aColSpan)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  nsAutoString newSpan;
  newSpan.AppendInt(aColSpan, 10);
  return SetAttribute(aCell, NS_LITERAL_STRING("colspan"), newSpan);
}

NS_IMETHODIMP nsHTMLEditor::SetRowSpan(nsIDOMElement *aCell, PRInt32 aRowSpan)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  nsAutoString newSpan;
  newSpan.AppendInt(aRowSpan, 10);
  return SetAttribute(aCell, NS_LITERAL_STRING("rowspan"), newSpan);
}





NS_IMETHODIMP
nsHTMLEditor::InsertTableCell(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  nsCOMPtr<nsIDOMNode> cellParent;
  PRInt32 cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(nsnull,
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                getter_AddRefs(cellParent), &cellOffset,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex, &rowSpan, &colSpan,
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);
  PRInt32 newCellIndex = aAfter ? (startColIndex+colSpan) : startColIndex;
  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, newCellIndex, ePreviousColumn, PR_FALSE);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  PRInt32 i;
  for (i = 0; i < aNumber; i++)
  {
    nsCOMPtr<nsIDOMElement> newCell;
    res = CreateElementWithDefaults(NS_LITERAL_STRING("td"), getter_AddRefs(newCell));
    if (NS_SUCCEEDED(res) && newCell)
    {
      if (aAfter) cellOffset++;
      res = InsertNode(newCell, cellParent, cellOffset);
      if(NS_FAILED(res)) break;
    }
  }
  return res;
}


NS_IMETHODIMP 
nsHTMLEditor::GetFirstRow(nsIDOMElement* aTableElement, nsIDOMNode** aRowNode)
{
  NS_ENSURE_TRUE(aRowNode, NS_ERROR_NULL_POINTER);

  *aRowNode = nsnull;  

  NS_ENSURE_TRUE(aTableElement, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> tableElement;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), aTableElement, getter_AddRefs(tableElement));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(tableElement, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> tableChild;
  res = tableElement->GetFirstChild(getter_AddRefs(tableChild));
  NS_ENSURE_SUCCESS(res, res);

  while (tableChild)
  {
    nsCOMPtr<nsIContent> content = do_QueryInterface(tableChild);
    if (content)
    {
      nsIAtom *atom = content->Tag();

      if (atom == nsEditProperty::tr)
      {
        
        *aRowNode = tableChild;
        NS_ADDREF(*aRowNode);
        return NS_OK;
      }
      
      if (atom == nsEditProperty::tbody ||
          atom == nsEditProperty::thead ||
          atom == nsEditProperty::tfoot)
      {
        nsCOMPtr<nsIDOMNode> rowNode;
        res = tableChild->GetFirstChild(getter_AddRefs(rowNode));
        NS_ENSURE_SUCCESS(res, res);
        
        
        while (rowNode && !nsHTMLEditUtils::IsTableRow(rowNode))
        {
          nsCOMPtr<nsIDOMNode> nextNode;
          res = rowNode->GetNextSibling(getter_AddRefs(nextNode));
          NS_ENSURE_SUCCESS(res, res);

          rowNode = nextNode;
        }
        if(rowNode)
        {
          *aRowNode = rowNode.get();
          NS_ADDREF(*aRowNode);
          return NS_OK;
        }
      }
    }
    
    
    
    
    nsCOMPtr<nsIDOMNode> nextChild;
    res = tableChild->GetNextSibling(getter_AddRefs(nextChild));
    NS_ENSURE_SUCCESS(res, res);

    tableChild = nextChild;
  };
  
  return NS_EDITOR_ELEMENT_NOT_FOUND;
}

NS_IMETHODIMP 
nsHTMLEditor::GetNextRow(nsIDOMNode* aCurrentRowNode, nsIDOMNode **aRowNode)
{
  NS_ENSURE_TRUE(aRowNode, NS_ERROR_NULL_POINTER);

  *aRowNode = nsnull;  

  NS_ENSURE_TRUE(aCurrentRowNode, NS_ERROR_NULL_POINTER);

  if (!nsHTMLEditUtils::IsTableRow(aCurrentRowNode))
    return NS_ERROR_FAILURE;
  
  nsCOMPtr<nsIDOMNode> nextRow;
  nsresult res = aCurrentRowNode->GetNextSibling(getter_AddRefs(nextRow));
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> nextNode;

  
  while (nextRow && !nsHTMLEditUtils::IsTableRow(nextRow))
  {
    res = nextRow->GetNextSibling(getter_AddRefs(nextNode));
    NS_ENSURE_SUCCESS(res, res);
  
    nextRow = nextNode;
  }
  if(nextRow)
  {
    *aRowNode = nextRow.get();
    NS_ADDREF(*aRowNode);
    return NS_OK;
  }

  
  nsCOMPtr<nsIDOMNode> rowParent;
  res = aCurrentRowNode->GetParentNode(getter_AddRefs(rowParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(rowParent, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> parentSibling;
  res = rowParent->GetNextSibling(getter_AddRefs(parentSibling));
  NS_ENSURE_SUCCESS(res, res);

  while (parentSibling)
  {
    res = parentSibling->GetFirstChild(getter_AddRefs(nextRow));
    NS_ENSURE_SUCCESS(res, res);
  
    
    while (nextRow && !nsHTMLEditUtils::IsTableRow(nextRow))
    {
      res = nextRow->GetNextSibling(getter_AddRefs(nextNode));
      NS_ENSURE_SUCCESS(res, res);

      nextRow = nextNode;
    }
    if(nextRow)
    {
      *aRowNode = nextRow.get();
      NS_ADDREF(*aRowNode);
      return NS_OK;
    }
#ifdef DEBUG_cmanske
    printf("GetNextRow: firstChild of row's parent's sibling is not a TR!\n");
#endif
    
    
    
    res = parentSibling->GetNextSibling(getter_AddRefs(nextNode));
    NS_ENSURE_SUCCESS(res, res);

    parentSibling = nextNode;
  }
  
  return NS_EDITOR_ELEMENT_NOT_FOUND;
}

NS_IMETHODIMP 
nsHTMLEditor::GetLastCellInRow(nsIDOMNode* aRowNode, nsIDOMNode** aCellNode)
{
  NS_ENSURE_TRUE(aCellNode, NS_ERROR_NULL_POINTER);

  *aCellNode = nsnull;

  NS_ENSURE_TRUE(aRowNode, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMNode> rowChild;
  nsresult res = aRowNode->GetLastChild(getter_AddRefs(rowChild));
  NS_ENSURE_SUCCESS(res, res);

  while (rowChild && !nsHTMLEditUtils::IsTableCell(rowChild))
  {
    
    nsCOMPtr<nsIDOMNode> previousChild;
    res = rowChild->GetPreviousSibling(getter_AddRefs(previousChild));
    NS_ENSURE_SUCCESS(res, res);

    rowChild = previousChild;
  };
  if (rowChild)
  {
    *aCellNode = rowChild.get();
    NS_ADDREF(*aCellNode);
    return NS_OK;
  }
  
  return NS_EDITOR_ELEMENT_NOT_FOUND;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTableColumn(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  PRInt32 startRowIndex, startColIndex;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                nsnull, nsnull,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex,
                      &rowSpan, &colSpan, 
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  
  if (aAfter)
  {
    startColIndex += actualColSpan;
    
    
    
    
    if (colSpan == 0)
      SetColSpan(curCell, actualColSpan);
  }
   
  PRInt32 rowCount, colCount, rowIndex;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, PR_FALSE);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  
  
  
  if (startColIndex >= colCount)
    NormalizeTable(table);

  nsCOMPtr<nsIDOMNode> rowNode;
  for ( rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
#ifdef DEBUG_cmanske
    if (rowIndex == rowCount-1)
      printf(" ***InsertTableColumn: Inserting cell at last row: %d\n", rowIndex);
#endif

    if (startColIndex < colCount)
    {
      
      res = GetCellDataAt(table, rowIndex, startColIndex,
                          getter_AddRefs(curCell),
                          &curStartRowIndex, &curStartColIndex,
                          &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      NS_ENSURE_SUCCESS(res, res);

      
      
      if (curCell)
      {
        if (curStartColIndex < startColIndex)
        {
          
          
          
          
          if (colSpan > 0)
            SetColSpan(curCell, colSpan+aNumber);
        } else {
          
          
          
          selection->Collapse(curCell, 0);
          res = InsertTableCell(aNumber, PR_FALSE);
        }
      }
    } else {
      
      if(rowIndex == 0)
        res = GetFirstRow(table.get(), getter_AddRefs(rowNode));
      else
      {
        nsCOMPtr<nsIDOMNode> nextRow;
        res = GetNextRow(rowNode.get(), getter_AddRefs(nextRow));
        rowNode = nextRow;
      }
      NS_ENSURE_SUCCESS(res, res);

      if (rowNode)
      {
        nsCOMPtr<nsIDOMNode> lastCell;
        res = GetLastCellInRow(rowNode, getter_AddRefs(lastCell));
        NS_ENSURE_SUCCESS(res, res);
        NS_ENSURE_TRUE(lastCell, NS_ERROR_FAILURE);

        curCell = do_QueryInterface(lastCell);
        if (curCell)
        {
          
          
          
          
          
          selection->Collapse(curCell, 0);
          res = InsertTableCell(aNumber, PR_TRUE);
        }
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTableRow(PRInt32 aNumber, PRBool aAfter)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  
  PRInt32 startRowIndex, startColIndex;
  nsresult res = GetCellContext(nsnull,
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                nsnull, nsnull, 
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex,
                      &rowSpan, &colSpan, 
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);
  
  PRInt32 rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  if (aAfter)
  {
    
    startRowIndex += actualRowSpan;

    
    
    
    
    if (rowSpan == 0)
      SetRowSpan(curCell, actualRowSpan);
  }

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, PR_FALSE);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMElement> cellForRowParent;
  PRInt32 cellsInRow = 0;
  if (startRowIndex < rowCount)
  {
    
    
    
    PRInt32 colIndex = 0;
    
    
    while ( NS_OK == GetCellDataAt(table, startRowIndex, colIndex,
                                   getter_AddRefs(curCell), 
                                   &curStartRowIndex, &curStartColIndex,
                                   &rowSpan, &colSpan, 
                                   &actualRowSpan, &actualColSpan,
                                   &isSelected) )
    {
      if (curCell)
      {
        if (curStartRowIndex < startRowIndex)
        {
          
          
          
          
          if (rowSpan > 0)
            SetRowSpan(curCell, rowSpan+aNumber);
        } else {
          

          
          cellsInRow += actualColSpan;

          
          if (!cellForRowParent)
            cellForRowParent = curCell;
        }
        
        colIndex += actualColSpan;
      }
      else
        colIndex++;
    }
  } else {
    
    
    
    cellsInRow = colCount;
    
    
    PRInt32 lastRow = rowCount-1;
    PRInt32 tempColIndex = 0;
    while ( NS_OK == GetCellDataAt(table, lastRow, tempColIndex,
                                   getter_AddRefs(curCell), 
                                   &curStartRowIndex, &curStartColIndex,
                                   &rowSpan, &colSpan, 
                                   &actualRowSpan, &actualColSpan,
                                   &isSelected) )
    {
      if (rowSpan == 0)
        cellsInRow -= actualColSpan;
      
      tempColIndex += actualColSpan;

      
      if (!cellForRowParent && curStartRowIndex == lastRow)
        cellForRowParent = curCell;
    }
  }

  if (cellsInRow > 0)
  {
    
    nsCOMPtr<nsIDOMNode> parentOfRow;
    PRInt32 newRowOffset;

    NS_NAMED_LITERAL_STRING(trStr, "tr");
    if (cellForRowParent)
    {
      nsCOMPtr<nsIDOMElement> parentRow;
      res = GetElementOrParentByTagName(trStr, cellForRowParent, getter_AddRefs(parentRow));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(parentRow, NS_ERROR_NULL_POINTER);

      parentRow->GetParentNode(getter_AddRefs(parentOfRow));
      NS_ENSURE_TRUE(parentOfRow, NS_ERROR_NULL_POINTER);

      res = GetChildOffset(parentRow, parentOfRow, newRowOffset);
      NS_ENSURE_SUCCESS(res, res);
      
      
      if (aAfter && startRowIndex >= rowCount)
        newRowOffset++;
    }
    else
      return NS_ERROR_FAILURE;

    for (PRInt32 row = 0; row < aNumber; row++)
    {
      
      nsCOMPtr<nsIDOMElement> newRow;
      res = CreateElementWithDefaults(trStr, getter_AddRefs(newRow));
      if (NS_SUCCEEDED(res))
      {
        NS_ENSURE_TRUE(newRow, NS_ERROR_FAILURE);
      
        for (PRInt32 i = 0; i < cellsInRow; i++)
        {
          nsCOMPtr<nsIDOMElement> newCell;
          res = CreateElementWithDefaults(NS_LITERAL_STRING("td"), getter_AddRefs(newCell));
          NS_ENSURE_SUCCESS(res, res);
          NS_ENSURE_TRUE(newCell, NS_ERROR_FAILURE);

          
          nsCOMPtr<nsIDOMNode>resultNode;
          res = newRow->AppendChild(newCell, getter_AddRefs(resultNode));
          NS_ENSURE_SUCCESS(res, res);
        }
        
        
        res = InsertNode(newRow, parentOfRow, newRowOffset);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
  }
  return res;
}




NS_IMETHODIMP
nsHTMLEditor::DeleteTable2(nsIDOMElement *aTable, nsISelection *aSelection)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  
  nsresult res = ClearSelection();
  if (NS_SUCCEEDED(res))
    res = AppendNodeToSelectionAsRange(aTable);
  NS_ENSURE_SUCCESS(res, res);

  return DeleteSelection(nsIEditor::eNext);
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTable()
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                nsnull, nsnull, nsnull, nsnull, nsnull);
    
  NS_ENSURE_SUCCESS(res, res);

  nsAutoEditBatch beginBatching(this);
  return DeleteTable2(table, selection);
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableCell(PRInt32 aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex;


  nsresult res = GetCellContext(getter_AddRefs(selection),
                         getter_AddRefs(table), 
                         getter_AddRefs(cell), 
                         nsnull, nsnull,
                         &startRowIndex, &startColIndex);

  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table && cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  PRInt32 rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    
    cell = firstCell;

    PRInt32 rowCount, colCount;
    res = GetTableSize(table, &rowCount, &colCount);
    NS_ENSURE_SUCCESS(res, res);

    
    res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);

    
    nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, PR_FALSE);
    nsAutoTxnsConserveSelection dontChangeSelection(this);

    PRBool  checkToDeleteRow = PR_TRUE;
    PRBool  checkToDeleteColumn = PR_TRUE;
    while (cell)
    {
      PRBool deleteRow = PR_FALSE;
      PRBool deleteCol = PR_FALSE;

      if (checkToDeleteRow)
      {
        
        
        checkToDeleteRow = PR_FALSE;

        deleteRow = AllCellsInRowSelected(table, startRowIndex, colCount);
        if (deleteRow)
        {
          
          
          PRInt32 nextRow = startRowIndex;
          while (nextRow == startRowIndex)
          {
            res = GetNextSelectedCell(nsnull, getter_AddRefs(cell));
            NS_ENSURE_SUCCESS(res, res);
            if (!cell) break;
            res = GetCellIndexes(cell, &nextRow, &startColIndex);
            NS_ENSURE_SUCCESS(res, res);
          }
          
          res = DeleteRow(table, startRowIndex);          
          NS_ENSURE_SUCCESS(res, res);

          if (cell)
          {
            
            startRowIndex = nextRow - 1;
            
            checkToDeleteRow = PR_TRUE;
          }
        }
      }
      if (!deleteRow)
      {
        if (checkToDeleteColumn)
        {
          
          
          checkToDeleteColumn = PR_FALSE;

          deleteCol = AllCellsInColumnSelected(table, startColIndex, colCount);
          if (deleteCol)
          {
            
            
            PRInt32 nextCol = startColIndex;
            while (nextCol == startColIndex)
            {
              res = GetNextSelectedCell(nsnull, getter_AddRefs(cell));
              NS_ENSURE_SUCCESS(res, res);
              if (!cell) break;
              res = GetCellIndexes(cell, &startRowIndex, &nextCol);
              NS_ENSURE_SUCCESS(res, res);
            }
            
            res = DeleteColumn(table, startColIndex);          
            NS_ENSURE_SUCCESS(res, res);
            if (cell) 
            {
              
              startColIndex = nextCol - 1;
              
              checkToDeleteColumn = PR_TRUE;
            }
          }
        }
        if (!deleteCol)
        {
          
          nsCOMPtr<nsIDOMElement> nextCell;
          res = GetNextSelectedCell(getter_AddRefs(range), getter_AddRefs(nextCell));
          NS_ENSURE_SUCCESS(res, res);

          
          res = DeleteNode(cell);
          NS_ENSURE_SUCCESS(res, res);
          
          
          cell = nextCell;
          if (cell)
          {
            res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
      }
    }
  }
  else for (PRInt32 i = 0; i < aNumber; i++)
  {
    res = GetCellContext(getter_AddRefs(selection),
                         getter_AddRefs(table), 
                         getter_AddRefs(cell), 
                         nsnull, nsnull,
                         &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
    
    NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

    if (1 == GetNumberOfCellsInRow(table, startRowIndex))
    {
      nsCOMPtr<nsIDOMElement> parentRow;
      res = GetElementOrParentByTagName(NS_LITERAL_STRING("tr"), cell, getter_AddRefs(parentRow));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(parentRow, NS_ERROR_NULL_POINTER);

      
      
      
      PRInt32 rowCount, colCount;
      res = GetTableSize(table, &rowCount, &colCount);
      NS_ENSURE_SUCCESS(res, res);
      
      if (rowCount == 1)
        return DeleteTable2(table, selection);
    
      
      res = DeleteTableRow(1);
      NS_ENSURE_SUCCESS(res, res);
    } 
    else
    {
      

      
      nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, PR_FALSE);
      nsAutoTxnsConserveSelection dontChangeSelection(this);

      res = DeleteNode(cell);
      
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableCellContents()
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex;
  nsresult res;
  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell), 
                       nsnull, nsnull,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);


  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);


  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);


  if (firstCell)
  {
    cell = firstCell;
    res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }

  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, PR_FALSE);

  while (cell)
  {
    DeleteCellContents(cell);
    if (firstCell)
    {
      
      res = GetNextSelectedCell(nsnull, getter_AddRefs(cell));
      NS_ENSURE_SUCCESS(res, res);
    }
    else
      cell = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteCellContents(nsIDOMElement *aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);

  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMNode> child;
  PRBool hasChild;
  aCell->HasChildNodes(&hasChild);

  while (hasChild)
  {
    aCell->GetLastChild(getter_AddRefs(child));
    nsresult res = DeleteNode(child);
    NS_ENSURE_SUCCESS(res, res);
    aCell->HasChildNodes(&hasChild);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableColumn(PRInt32 aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowCount, colCount;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                getter_AddRefs(cell), 
                                nsnull, nsnull,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table && cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  if(startColIndex == 0 && aNumber >= colCount)
    return DeleteTable2(table, selection);

  
  aNumber = NS_MIN(aNumber,(colCount-startColIndex));

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  
  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  PRInt32 rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    res = GetCellIndexes(firstCell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, PR_FALSE);

  if (firstCell && rangeCount > 1)
  {
    
    cell = firstCell;

    while (cell)
    {
      if (cell != firstCell)
      {
        res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      
      PRInt32 nextCol = startColIndex;
      while (nextCol == startColIndex)
      {
        res = GetNextSelectedCell(getter_AddRefs(range), getter_AddRefs(cell));
        NS_ENSURE_SUCCESS(res, res);
        if (!cell) break;
        res = GetCellIndexes(cell, &startRowIndex, &nextCol);
        NS_ENSURE_SUCCESS(res, res);
      }
      res = DeleteColumn(table, startColIndex);          
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  else for (PRInt32 i = 0; i < aNumber; i++)
  {
    res = DeleteColumn(table, startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteColumn(nsIDOMElement *aTable, PRInt32 aColIndex)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMElement> cellInDeleteCol;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  PRInt32 rowIndex = 0;
  nsresult res = NS_OK;
   
  do {
    res = GetCellDataAt(aTable, rowIndex, aColIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);

    NS_ENSURE_SUCCESS(res, res);

    if (cell)
    {
      
      if (startColIndex < aColIndex || colSpan > 1 || colSpan == 0)
      {
        
        
        
        if (colSpan > 0)
        {
          NS_ASSERTION((colSpan > 1),"Bad COLSPAN in DeleteTableColumn");
          SetColSpan(cell, colSpan-1);
        }
        if (startColIndex == aColIndex)
        {
          
          
          
          DeleteCellContents(cell);
        }
        
        rowIndex += actualRowSpan;
      } 
      else 
      {
        
        if (1 == GetNumberOfCellsInRow(aTable, rowIndex))
        {
          
          nsCOMPtr<nsIDOMElement> parentRow;
          res = GetElementOrParentByTagName(NS_LITERAL_STRING("tr"), cell, getter_AddRefs(parentRow));
          NS_ENSURE_SUCCESS(res, res);
          if(!parentRow) return NS_ERROR_NULL_POINTER;

          
          
          
          PRInt32 rowCount, colCount;
          res = GetTableSize(aTable, &rowCount, &colCount);
          NS_ENSURE_SUCCESS(res, res);

          if (rowCount == 1)
          {
            nsCOMPtr<nsISelection> selection;
            res = GetSelection(getter_AddRefs(selection));
            NS_ENSURE_SUCCESS(res, res);
            NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
            return DeleteTable2(aTable, selection);
          }
    
          
          
          res = DeleteRow(aTable, startRowIndex);
          NS_ENSURE_SUCCESS(res, res);

          
          
          
        } 
        else 
        {
          
          res = DeleteNode(cell);
          NS_ENSURE_SUCCESS(res, res);

          
          rowIndex += actualRowSpan;
        }
      }
    }
  } while (cell);    

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableRow(PRInt32 aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex;
  PRInt32 rowCount, colCount;
  nsresult res =  GetCellContext(getter_AddRefs(selection),
                                 getter_AddRefs(table), 
                                 getter_AddRefs(cell), 
                                 nsnull, nsnull,
                                 &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  if(startRowIndex == 0 && aNumber >= rowCount)
    return DeleteTable2(table, selection);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  PRInt32 rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    res = GetCellIndexes(firstCell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, PR_FALSE);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  if (firstCell && rangeCount > 1)
  {
    
    cell = firstCell;

    while (cell)
    {
      if (cell != firstCell)
      {
        res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      
      PRInt32 nextRow = startRowIndex;
      while (nextRow == startRowIndex)
      {
        res = GetNextSelectedCell(getter_AddRefs(range), getter_AddRefs(cell));
        NS_ENSURE_SUCCESS(res, res);
        if (!cell) break;
        res = GetCellIndexes(cell, &nextRow, &startColIndex);
        NS_ENSURE_SUCCESS(res, res);
      }
      
      res = DeleteRow(table, startRowIndex);          
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  else
  {
    
    aNumber = NS_MIN(aNumber,(rowCount-startRowIndex));

    for (PRInt32 i = 0; i < aNumber; i++)
    {
      res = DeleteRow(table, startRowIndex);
      
      if (NS_FAILED(res))
        startRowIndex++;
    
      
      res = GetCellAt(table, startRowIndex, startColIndex, getter_AddRefs(cell));
      NS_ENSURE_SUCCESS(res, res);
      if(!cell)
        break;
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::DeleteRow(nsIDOMElement *aTable, PRInt32 aRowIndex)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMElement> cellInDeleteRow;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  PRInt32 colIndex = 0;
  nsresult res = NS_OK;
   
  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  
  
  nsTArray<nsIDOMElement*> spanCellList;
  nsTArray<PRInt32> newSpanList;

  
  
  
  do {
    res = GetCellDataAt(aTable, aRowIndex, colIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
  
    
    if(NS_FAILED(res)) return res;

    
    if (cell)
    {
      if (startRowIndex < aRowIndex)
      {
        
        
        
        
        if (rowSpan > 0)
        {
          
          
          
          spanCellList.AppendElement(cell);
          newSpanList.AppendElement(NS_MAX((aRowIndex - startRowIndex), actualRowSpan-1));
        }
      }
      else 
      {
        if (rowSpan > 1)
        {
          
          
          
          res = SplitCellIntoRows(aTable, startRowIndex, startColIndex,
                                  aRowIndex - startRowIndex + 1, 
                                  actualRowSpan - 1, nsnull);    
          NS_ENSURE_SUCCESS(res, res);
        }
        if (!cellInDeleteRow)
          cellInDeleteRow = cell; 
      }
      
      colIndex += actualColSpan;
    }
  } while (cell);

  
  NS_ENSURE_TRUE(cellInDeleteRow, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMElement> parentRow;
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("tr"), cellInDeleteRow, getter_AddRefs(parentRow));
  NS_ENSURE_SUCCESS(res, res);

  if (parentRow)
  {
    res = DeleteNode(parentRow);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  for (PRUint32 i = 0, n = spanCellList.Length(); i < n; i++)
  {
    nsIDOMElement *cellPtr = spanCellList[i];
    if (cellPtr)
    {
      res = SetRowSpan(cellPtr, newSpanList[i]);
      NS_ENSURE_SUCCESS(res, res);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP 
nsHTMLEditor::SelectTable()
{
  nsCOMPtr<nsIDOMElement> table;
  nsresult res = NS_ERROR_FAILURE;
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), nsnull, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table, NS_OK);

  res = ClearSelection();
  if (NS_SUCCEEDED(res))
    res = AppendNodeToSelectionAsRange(table);

  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableCell()
{
  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  res = ClearSelection();
  if (NS_SUCCEEDED(res))
    res = AppendNodeToSelectionAsRange(cell);

  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectBlockOfCells(nsIDOMElement *aStartCell, nsIDOMElement *aEndCell)
{
  NS_ENSURE_TRUE(aStartCell && aEndCell, NS_ERROR_NULL_POINTER);
  
  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  NS_NAMED_LITERAL_STRING(tableStr, "table");
  nsCOMPtr<nsIDOMElement> table;
  res = GetElementOrParentByTagName(tableStr, aStartCell, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMElement> endTable;
  res = GetElementOrParentByTagName(tableStr, aEndCell, getter_AddRefs(endTable));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(endTable, NS_ERROR_FAILURE);
  
  
  
  if (table != endTable) return NS_OK;

  PRInt32 startRowIndex, startColIndex, endRowIndex, endColIndex;

  
  res = GetCellIndexes(aStartCell, &startRowIndex, &startColIndex);
  if(NS_FAILED(res)) return res;

  res = GetCellIndexes(aEndCell, &endRowIndex, &endColIndex);
  if(NS_FAILED(res)) return res;

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  PRInt32 minColumn = NS_MIN(startColIndex, endColIndex);
  PRInt32 minRow    = NS_MIN(startRowIndex, endRowIndex);
  PRInt32 maxColumn   = NS_MAX(startColIndex, endColIndex);
  PRInt32 maxRow      = NS_MAX(startRowIndex, endRowIndex);

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 currentRowIndex, currentColIndex;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  if (res == NS_EDITOR_ELEMENT_NOT_FOUND) return NS_OK;

  while (cell)
  {
    res = GetCellIndexes(cell, &currentRowIndex, &currentColIndex);
    NS_ENSURE_SUCCESS(res, res);

    if (currentRowIndex < maxRow || currentRowIndex > maxRow || 
        currentColIndex < maxColumn || currentColIndex > maxColumn)
    {
      selection->RemoveRange(range);
      
      mSelectedCellIndex--;
    }    
    res = GetNextSelectedCell(getter_AddRefs(range), getter_AddRefs(cell));
    NS_ENSURE_SUCCESS(res, res);
  }

  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  for (PRInt32 row = minRow; row <= maxRow; row++)
  {
    for(PRInt32 col = minColumn; col <= maxColumn; col += NS_MAX(actualColSpan, 1))
    {
      res = GetCellDataAt(table, row, col, getter_AddRefs(cell),
                          &currentRowIndex, &currentColIndex,
                          &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      if (NS_FAILED(res)) break;
      
      if (!isSelected && cell && row == currentRowIndex && col == currentColIndex)
      {
        res = AppendNodeToSelectionAsRange(cell);
        if (NS_FAILED(res)) break;
      }
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectAllTableCells()
{
  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsCOMPtr<nsIDOMElement> startCell = cell;
  
  
  nsCOMPtr<nsIDOMElement> table;
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), cell, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  if(!table) return NS_ERROR_NULL_POINTER;

  PRInt32 rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsISelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  PRBool cellSelected = PR_FALSE;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  PRBool  isSelected;
  for(PRInt32 row = 0; row < rowCount; row++)
  {
    for(PRInt32 col = 0; col < colCount; col += NS_MAX(actualColSpan, 1))
    {
      res = GetCellDataAt(table, row, col, getter_AddRefs(cell),
                          &currentRowIndex, &currentColIndex,
                          &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      if (NS_FAILED(res)) break;
      
      if (cell && row == currentRowIndex && col == currentColIndex)
      {
        res =  AppendNodeToSelectionAsRange(cell);
        if (NS_FAILED(res)) break;
        cellSelected = PR_TRUE;
      }
    }
  }
  
  if (!cellSelected)
  {
    return AppendNodeToSelectionAsRange(startCell);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableRow()
{
  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);
  nsCOMPtr<nsIDOMElement> startCell = cell;

  
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  PRInt32 startRowIndex, startColIndex;

  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell),
                       nsnull, nsnull,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);
  
  PRInt32 rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  
  

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  PRBool cellSelected = PR_FALSE;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  PRBool  isSelected;
  for(PRInt32 col = 0; col < colCount; col += NS_MAX(actualColSpan, 1))
  {
    res = GetCellDataAt(table, startRowIndex, col, getter_AddRefs(cell),
                        &currentRowIndex, &currentColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    if (NS_FAILED(res)) break;
    
    if (cell && currentRowIndex == startRowIndex && currentColIndex == col)
    {
      res = AppendNodeToSelectionAsRange(cell);
      if (NS_FAILED(res)) break;
      cellSelected = PR_TRUE;
    }
  }
  
  if (!cellSelected)
  {
    return AppendNodeToSelectionAsRange(startCell);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SelectTableColumn()
{
  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsCOMPtr<nsIDOMElement> startCell = cell;
  
  
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  PRInt32 startRowIndex, startColIndex;

  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell),
                       nsnull, nsnull,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);

  PRInt32 rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  PRBool cellSelected = PR_FALSE;
  PRInt32 rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  PRBool  isSelected;
  for(PRInt32 row = 0; row < rowCount; row += NS_MAX(actualRowSpan, 1))
  {
    res = GetCellDataAt(table, row, startColIndex, getter_AddRefs(cell),
                        &currentRowIndex, &currentColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    if (NS_FAILED(res)) break;
    
    if (cell && currentRowIndex == row && currentColIndex == startColIndex)
    {
      res = AppendNodeToSelectionAsRange(cell);
      if (NS_FAILED(res)) break;
      cellSelected = PR_TRUE;
    }
  }
  
  if (!cellSelected)
  {
    return AppendNodeToSelectionAsRange(startCell);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SplitTableCell()
{
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, actualRowSpan, actualColSpan;
  
  nsresult res = GetCellContext(nsnull,
                                getter_AddRefs(table), 
                                getter_AddRefs(cell),
                                nsnull, nsnull,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  if(!table || !cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  
  res = GetCellSpansAt(table, startRowIndex, startColIndex, actualRowSpan, actualColSpan);
  NS_ENSURE_SUCCESS(res, res);

  
  if (actualRowSpan <= 1 && actualColSpan <= 1)
    return NS_OK;
  
  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, PR_FALSE);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMElement> newCell;
  PRInt32 rowIndex = startRowIndex;
  PRInt32 rowSpanBelow, colSpanAfter;

  
  
  for (rowSpanBelow = actualRowSpan-1; rowSpanBelow >= 0; rowSpanBelow--)
  {
    
    if (rowSpanBelow > 0)
    {
      res = SplitCellIntoRows(table, rowIndex, startColIndex, 1, rowSpanBelow, getter_AddRefs(newCell));
      NS_ENSURE_SUCCESS(res, res);
      CopyCellBackgroundColor(newCell, cell);
    }
    PRInt32 colIndex = startColIndex;
    
    for (colSpanAfter = actualColSpan-1; colSpanAfter > 0; colSpanAfter--)
    {
      res = SplitCellIntoColumns(table, rowIndex, colIndex, 1, colSpanAfter, getter_AddRefs(newCell));
      NS_ENSURE_SUCCESS(res, res);
      CopyCellBackgroundColor(newCell, cell);
      colIndex++;
    }
    
    rowIndex++;
  }
  return res;
}

nsresult
nsHTMLEditor::CopyCellBackgroundColor(nsIDOMElement *destCell, nsIDOMElement *sourceCell)
{
  NS_ENSURE_TRUE(destCell && sourceCell, NS_ERROR_NULL_POINTER);

  
  NS_NAMED_LITERAL_STRING(bgcolor, "bgcolor");
  nsAutoString color;
  PRBool isSet;
  nsresult res = GetAttributeValue(sourceCell, bgcolor, color, &isSet);

  if (NS_SUCCEEDED(res) && isSet)
    res = SetAttribute(destCell, bgcolor, color);

  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SplitCellIntoColumns(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aColIndex,
                                   PRInt32 aColSpanLeft, PRInt32 aColSpanRight,
                                   nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nsnull;

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  nsresult res = GetCellDataAt(aTable, aRowIndex, aColIndex, getter_AddRefs(cell),
                               &startRowIndex, &startColIndex,
                               &rowSpan, &colSpan, 
                               &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_ERROR_NULL_POINTER);
  
  
  if (actualColSpan <= 1 || (aColSpanLeft + aColSpanRight) > actualColSpan)
    return NS_OK;

  
  res = SetColSpan(cell, aColSpanLeft);
  NS_ENSURE_SUCCESS(res, res);
  
  
  
  nsCOMPtr<nsIDOMElement> newCell;
  res = InsertCell(cell, actualRowSpan, aColSpanRight, PR_TRUE, PR_FALSE, getter_AddRefs(newCell));
  NS_ENSURE_SUCCESS(res, res);
  if (newCell)
  {
    if (aNewCell)
    {
      *aNewCell = newCell.get();
      NS_ADDREF(*aNewCell);
    }
    res = CopyCellBackgroundColor(newCell, cell);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SplitCellIntoRows(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aColIndex,
                                PRInt32 aRowSpanAbove, PRInt32 aRowSpanBelow, 
                                nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nsnull;

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  nsresult res = GetCellDataAt(aTable, aRowIndex, aColIndex, getter_AddRefs(cell),
                               &startRowIndex, &startColIndex,
                               &rowSpan, &colSpan, 
                               &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_ERROR_NULL_POINTER);
  
  
  if (actualRowSpan <= 1 || (aRowSpanAbove + aRowSpanBelow) > actualRowSpan)
    return NS_OK;

  PRInt32 rowCount, colCount;
  res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> cell2;
  nsCOMPtr<nsIDOMElement> lastCellFound;
  PRInt32 startRowIndex2, startColIndex2, rowSpan2, colSpan2, actualRowSpan2, actualColSpan2;
  PRBool  isSelected2;
  PRInt32 colIndex = 0;
  PRBool insertAfter = (startColIndex > 0);
  
  PRInt32 rowBelowIndex = startRowIndex+aRowSpanAbove;
  
  
  do 
  {
    
    res = GetCellDataAt(aTable, rowBelowIndex, 
                        colIndex, getter_AddRefs(cell2),
                        &startRowIndex2, &startColIndex2, &rowSpan2, &colSpan2, 
                        &actualRowSpan2, &actualColSpan2, &isSelected2);
    
    
    if (NS_FAILED(res) || !cell) return NS_ERROR_FAILURE;

    
    if (cell2 && startRowIndex2 == rowBelowIndex)
    {
      if (insertAfter)
      {
        
        
        if ((startColIndex2 + actualColSpan2) == startColIndex)
          break;

        
        
        
        if (startColIndex2 > startColIndex)
        {
          
          insertAfter = PR_FALSE;
          break;
        }
      }
      else
      {
        break; 
      }
      lastCellFound = cell2;
    }
    
    colIndex += NS_MAX(actualColSpan2, 1);

    
    if (colIndex > colCount)
        break;

  } while(PR_TRUE);

  if (!cell2 && lastCellFound)
  {
    
    
    
    
    cell2 = lastCellFound;
    insertAfter = PR_TRUE; 
  }

  
  res = SetRowSpan(cell, aRowSpanAbove);
  NS_ENSURE_SUCCESS(res, res);


  
  
  nsCOMPtr<nsIDOMElement> newCell;
  res = InsertCell(cell2, aRowSpanBelow, actualColSpan, insertAfter, PR_FALSE, getter_AddRefs(newCell));
  NS_ENSURE_SUCCESS(res, res);
  if (newCell)
  {
    if (aNewCell)
    {
      *aNewCell = newCell.get();
      NS_ADDREF(*aNewCell);
    }
    res = CopyCellBackgroundColor(newCell, cell2);
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SwitchTableCellHeaderType(nsIDOMElement *aSourceCell, nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aSourceCell, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMNode> newNode;

  
  
  
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  nsAutoSelectionReset selectionResetter(selection, this);

  
  nsCOMPtr<nsIAtom> atom = nsEditor::GetTag(aSourceCell);
  nsString newCellType( (atom == nsEditProperty::td) ? NS_LITERAL_STRING("th") : NS_LITERAL_STRING("td"));

  
  
  res = ReplaceContainer(aSourceCell, address_of(newNode), newCellType, nsnull, nsnull, PR_TRUE);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(newNode, NS_ERROR_FAILURE);

  
  if (aNewCell)
  {
    nsCOMPtr<nsIDOMElement> newElement = do_QueryInterface(newNode);
    *aNewCell = newElement.get();
    NS_ADDREF(*aNewCell);
  }

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::JoinTableCells(PRBool aMergeNonContiguousContents)
{
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> targetCell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  nsCOMPtr<nsIDOMElement> cell2;
  PRInt32 startRowIndex2, startColIndex2, rowSpan2, colSpan2, actualRowSpan2, actualColSpan2;
  PRBool  isSelected2;

  
  nsresult res = GetCellContext(nsnull,
                                getter_AddRefs(table), 
                                getter_AddRefs(targetCell),
                                nsnull, nsnull,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  if(!table || !targetCell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  nsAutoEditBatch beginBatching(this);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  
  
  

  nsCOMPtr<nsIDOMElement> firstCell;
  PRInt32 firstRowIndex, firstColIndex;
  res = GetFirstSelectedCellInTable(&firstRowIndex, &firstColIndex, getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);
  
  PRBool joinSelectedCells = PR_FALSE;
  if (firstCell)
  {
    nsCOMPtr<nsIDOMElement> secondCell;
    res = GetNextSelectedCell(nsnull, getter_AddRefs(secondCell));
    NS_ENSURE_SUCCESS(res, res);

    
    joinSelectedCells = (secondCell != nsnull);
  }

  if (joinSelectedCells)
  {
    
    

    PRInt32 rowCount, colCount;
    res = GetTableSize(table, &rowCount, &colCount);
    NS_ENSURE_SUCCESS(res, res);

    
    PRInt32 firstRowSpan, firstColSpan;
    res = GetCellSpansAt( table, firstRowIndex, firstColIndex, firstRowSpan, firstColSpan);
    NS_ENSURE_SUCCESS(res, res);

    
    
    
    
    
    PRInt32 lastRowIndex = firstRowIndex;
    PRInt32 lastColIndex = firstColIndex;
    PRInt32 rowIndex, colIndex;

    
    
    
    for (rowIndex = firstRowIndex; rowIndex <= lastRowIndex; rowIndex++)
    {
      PRInt32 currentRowCount = rowCount;
      
      res = FixBadRowSpan(table, rowIndex, rowCount);
      NS_ENSURE_SUCCESS(res, res);
      
      lastRowIndex -= (currentRowCount-rowCount);

      PRBool cellFoundInRow = PR_FALSE;
      PRBool lastRowIsSet = PR_FALSE;
      PRInt32 lastColInRow = 0;
      PRInt32 firstColInRow = firstColIndex;
      for (colIndex = firstColIndex; colIndex < colCount; colIndex += NS_MAX(actualColSpan2, 1))
      {
        res = GetCellDataAt(table, rowIndex, colIndex, getter_AddRefs(cell2),
                            &startRowIndex2, &startColIndex2,
                            &rowSpan2, &colSpan2, 
                            &actualRowSpan2, &actualColSpan2, &isSelected2);
        NS_ENSURE_SUCCESS(res, res);

        if (isSelected2)
        {
          if (!cellFoundInRow)
            
            firstColInRow = colIndex;

          if (rowIndex > firstRowIndex && firstColInRow != firstColIndex)
          {
            
            
            
            
            
            
            lastRowIndex = NS_MAX(0,rowIndex - 1);
            lastRowIsSet = PR_TRUE;
            break;
          }
          
          lastColInRow = colIndex + (actualColSpan2-1);
          cellFoundInRow = PR_TRUE;
        }
        else if (cellFoundInRow)
        {
          
          
          if (rowIndex > (firstRowIndex+1) && colIndex <= lastColIndex)
          {
            
            
            lastRowIndex = NS_MAX(0,rowIndex - 1);
            lastRowIsSet = PR_TRUE;
          }
          
          break;
        }
      } 

      
      if (cellFoundInRow) 
      {
        if (rowIndex == firstRowIndex)
        {
          
          lastColIndex = lastColInRow;
        }

        
        if (!lastRowIsSet)
        {
          if (colIndex < lastColIndex)
          {
            
            
            
            lastRowIndex = NS_MAX(0,rowIndex - 1);
          }
          else
          {
            
            lastRowIndex = rowIndex+1;
          }
        }
        
        lastColIndex = NS_MIN(lastColIndex, lastColInRow);
      }
      else
      {
        
        
        lastRowIndex = NS_MAX(0,rowIndex - 1);
      }
    }
  
    
    nsTArray<nsIDOMElement*> deleteList;

    
    for (rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
      for (colIndex = 0; colIndex < colCount; colIndex += NS_MAX(actualColSpan2, 1))
      {
        res = GetCellDataAt(table, rowIndex, colIndex, getter_AddRefs(cell2),
                            &startRowIndex2, &startColIndex2,
                            &rowSpan2, &colSpan2, 
                            &actualRowSpan2, &actualColSpan2, &isSelected2);
        NS_ENSURE_SUCCESS(res, res);

        
        if (actualColSpan2 == 0)
          break;

        
        if (isSelected2 && cell2 != firstCell)
        {
          if (rowIndex >= firstRowIndex && rowIndex <= lastRowIndex && 
              colIndex >= firstColIndex && colIndex <= lastColIndex)
          {
            
            
            
            
            NS_ASSERTION(startRowIndex2 == rowIndex, "JoinTableCells: StartRowIndex is in row above");

            if (actualColSpan2 > 1)
            {
              
              
              PRInt32 extraColSpan = (startColIndex2 + actualColSpan2) - (lastColIndex+1);
              if ( extraColSpan > 0)
              {
                res = SplitCellIntoColumns(table, startRowIndex2, startColIndex2, 
                                           actualColSpan2-extraColSpan, extraColSpan, nsnull);
                NS_ENSURE_SUCCESS(res, res);
              }
            }

            res = MergeCells(firstCell, cell2, PR_FALSE);
            NS_ENSURE_SUCCESS(res, res);
            
            
            deleteList.AppendElement(cell2.get());
          }
          else if (aMergeNonContiguousContents)
          {
            
            res = MergeCells(firstCell, cell2, PR_FALSE);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
      }
    }

    
    
    nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

    for (PRUint32 i = 0, n = deleteList.Length(); i < n; i++)
    {
      nsIDOMElement *elementPtr = deleteList[i];
      if (elementPtr)
      {
        nsCOMPtr<nsIDOMNode> node = do_QueryInterface(elementPtr);
        res = DeleteNode(node);
        NS_ENSURE_SUCCESS(res, res);
      }
    }
    
    nsCOMPtr<nsISelection> selection;
    res = GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

    PRInt32 rangeCount;
    res = selection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMRange> range;
    PRInt32 i;
    for (i = 0; i < rangeCount; i++)
    {
      res = selection->GetRangeAt(i, getter_AddRefs(range));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

      nsCOMPtr<nsIDOMElement> deletedCell;
      res = GetCellFromRange(range, getter_AddRefs(deletedCell));
      if (!deletedCell)
      {
        selection->RemoveRange(range);
        rangeCount--;
        i--;
      }
    }

    
    res = SetRowSpan(firstCell, lastRowIndex-firstRowIndex+1);
    NS_ENSURE_SUCCESS(res, res);
    res = SetColSpan(firstCell, lastColIndex-firstColIndex+1);
    NS_ENSURE_SUCCESS(res, res);
    
    
    
    NormalizeTable(table);
  }
  else
  {
    
    res = GetCellDataAt(table, startRowIndex, startColIndex, getter_AddRefs(targetCell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(targetCell, NS_ERROR_NULL_POINTER);

    
    res = GetCellDataAt(table, startRowIndex, startColIndex+actualColSpan, getter_AddRefs(cell2),
                        &startRowIndex2, &startColIndex2, &rowSpan2, &colSpan2, 
                        &actualRowSpan2, &actualColSpan2, &isSelected2);
    NS_ENSURE_SUCCESS(res, res);
    if(!cell2) return NS_OK; 

    
    NS_ASSERTION((startRowIndex >= startRowIndex2),"JoinCells: startRowIndex < startRowIndex2");

    
    
    PRInt32 spanAboveMergedCell = startRowIndex - startRowIndex2;
    PRInt32 effectiveRowSpan2 = actualRowSpan2 - spanAboveMergedCell;

    if (effectiveRowSpan2 > actualRowSpan)
    {
      
      
      res = SplitCellIntoRows(table, startRowIndex2, startColIndex2,
                              spanAboveMergedCell+actualRowSpan, 
                              effectiveRowSpan2-actualRowSpan, nsnull);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    
    
    res = MergeCells(targetCell, cell2, 
                     (startRowIndex2 == startRowIndex) && 
                     (effectiveRowSpan2 >= actualRowSpan));
    NS_ENSURE_SUCCESS(res, res);

    if (effectiveRowSpan2 < actualRowSpan)
    {
      
      
      
      
      return NS_OK;
    }

    if( spanAboveMergedCell > 0 )
    {
      
      
      res = SetRowSpan(cell2, spanAboveMergedCell);
      NS_ENSURE_SUCCESS(res, res);
    }

    
    res = SetColSpan(targetCell, actualColSpan+actualColSpan2);
    NS_ENSURE_SUCCESS(res, res);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::MergeCells(nsCOMPtr<nsIDOMElement> aTargetCell, 
                         nsCOMPtr<nsIDOMElement> aCellToMerge,
                         PRBool aDeleteCellToMerge)
{
  NS_ENSURE_TRUE(aTargetCell && aCellToMerge, NS_ERROR_NULL_POINTER);

  nsresult res = NS_OK;

  
  nsAutoRules beginRulesSniffing(this, kOpDeleteNode, nsIEditor::eNext);

  
  if (!IsEmptyCell(aCellToMerge))
  {
    
    nsCOMPtr<nsIDOMNodeList> childNodes;
    nsCOMPtr<nsIDOMNode> cellChild;
    res = aTargetCell->GetChildNodes(getter_AddRefs(childNodes));
    
    
    PRInt32 insertIndex = 0;

    if ((NS_SUCCEEDED(res)) && (childNodes))
    {
      
      PRUint32 len;
      res = childNodes->GetLength(&len);
      NS_ENSURE_SUCCESS(res, res);
      if (len == 1 && IsEmptyCell(aTargetCell))
      {
          
          nsCOMPtr<nsIDOMNode> tempNode;
          res = childNodes->Item(0, getter_AddRefs(cellChild));
          NS_ENSURE_SUCCESS(res, res);
          res = DeleteNode(cellChild);
          NS_ENSURE_SUCCESS(res, res);
          insertIndex = 0;
      }
      else
        insertIndex = (PRInt32)len;
    }

    
    PRBool hasChild;
    aCellToMerge->HasChildNodes(&hasChild);
    while (hasChild)
    {
      aCellToMerge->GetLastChild(getter_AddRefs(cellChild));
      res = DeleteNode(cellChild);
      NS_ENSURE_SUCCESS(res, res);

      res = InsertNode(cellChild, aTargetCell, insertIndex);
      NS_ENSURE_SUCCESS(res, res);

      aCellToMerge->HasChildNodes(&hasChild);
    }
  }

  
  if (aDeleteCellToMerge)
    res = DeleteNode(aCellToMerge);

  return res;
}


NS_IMETHODIMP 
nsHTMLEditor::FixBadRowSpan(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32& aNewRowCount)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  PRInt32 rowCount, colCount;
  nsresult res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement>cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  PRInt32 minRowSpan = -1;
  PRInt32 colIndex;
  
  for( colIndex = 0; colIndex < colCount; colIndex += NS_MAX(actualColSpan, 1))
  {
    res = GetCellDataAt(aTable, aRowIndex, colIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    
    
    if(NS_FAILED(res)) return res;
    if (!cell) break;
    if(rowSpan > 0 && 
       startRowIndex == aRowIndex &&
       (rowSpan < minRowSpan || minRowSpan == -1))
    {
      minRowSpan = rowSpan;
    }
    NS_ASSERTION((actualColSpan > 0),"ActualColSpan = 0 in FixBadRowSpan");
  }
  if(minRowSpan > 1)
  {
    
    
    PRInt32 rowsReduced = minRowSpan - 1;
    for(colIndex = 0; colIndex < colCount; colIndex += NS_MAX(actualColSpan, 1))
    {
      res = GetCellDataAt(aTable, aRowIndex, colIndex, getter_AddRefs(cell),
                          &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      if(NS_FAILED(res)) return res;
      
      if(cell && rowSpan > 0 &&
         startRowIndex == aRowIndex && 
         startColIndex ==  colIndex )
      {
        res = SetRowSpan(cell, rowSpan-rowsReduced);
        if(NS_FAILED(res)) return res;
      }
      NS_ASSERTION((actualColSpan > 0),"ActualColSpan = 0 in FixBadRowSpan");
    }
  }
  return GetTableSize(aTable, &aNewRowCount, &colCount);
}

NS_IMETHODIMP 
nsHTMLEditor::FixBadColSpan(nsIDOMElement *aTable, PRInt32 aColIndex, PRInt32& aNewColCount)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  PRInt32 rowCount, colCount;
  nsresult res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  PRInt32 minColSpan = -1;
  PRInt32 rowIndex;
  
  for( rowIndex = 0; rowIndex < rowCount; rowIndex += NS_MAX(actualRowSpan, 1))
  {
    res = GetCellDataAt(aTable, rowIndex, aColIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    
    
    if(NS_FAILED(res)) return res;
    if (!cell) break;
    if(colSpan > 0 && 
       startColIndex == aColIndex &&
       (colSpan < minColSpan || minColSpan == -1))
    {
      minColSpan = colSpan;
    }
    NS_ASSERTION((actualRowSpan > 0),"ActualRowSpan = 0 in FixBadColSpan");
  }
  if(minColSpan > 1)
  {
    
    
    PRInt32 colsReduced = minColSpan - 1;
    for(rowIndex = 0; rowIndex < rowCount; rowIndex += NS_MAX(actualRowSpan, 1))
    {
      res = GetCellDataAt(aTable, rowIndex, aColIndex, getter_AddRefs(cell),
                          &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      if(NS_FAILED(res)) return res;
      
      if(cell && colSpan > 0 &&
         startColIndex == aColIndex && 
         startRowIndex ==  rowIndex )
      {
        res = SetColSpan(cell, colSpan-colsReduced);
        if(NS_FAILED(res)) return res;
      }
      NS_ASSERTION((actualRowSpan > 0),"ActualRowSpan = 0 in FixBadColSpan");
    }
  }
  return GetTableSize(aTable, &rowCount, &aNewColCount);
}

NS_IMETHODIMP 
nsHTMLEditor::NormalizeTable(nsIDOMElement *aTable)
{
  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMElement> table;
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), aTable, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table, NS_OK);

  PRInt32 rowCount, colCount, rowIndex, colIndex;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsAutoSelectionReset selectionResetter(selection, this);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, kOpInsertNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  
  for(rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    res = FixBadRowSpan(table, rowIndex, rowCount);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  for(colIndex = 0; colIndex < colCount; colIndex++)
  {
    res = FixBadColSpan(table, colIndex, colCount);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  for(rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
    nsCOMPtr<nsIDOMElement> previousCellInRow;

    for(colIndex = 0; colIndex < colCount; colIndex++)
    {
      res = GetCellDataAt(table, rowIndex, colIndex, getter_AddRefs(cell),
                          &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                          &actualRowSpan, &actualColSpan, &isSelected);
      
      
      if(NS_FAILED(res)) return res;
      if (!cell)
      {
        
#ifdef DEBUG
        printf("NormalizeTable found missing cell at row=%d, col=%d\n", rowIndex, colIndex);
#endif
        
        if(previousCellInRow)
        {
          
          res = InsertCell(previousCellInRow, 1, 1, PR_TRUE, PR_FALSE, getter_AddRefs(cell));
          NS_ENSURE_SUCCESS(res, res);

          
          if(cell)
            startRowIndex = rowIndex;   
        } else {
          
#ifdef DEBUG
          printf("NormalizeTable found no cells in row=%d, col=%d\n", rowIndex, colIndex);
#endif
          return NS_ERROR_FAILURE;
        }
      }
      
      if(startRowIndex == rowIndex)
      {
        previousCellInRow = cell;
      }
    }
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetCellIndexes(nsIDOMElement *aCell,
                             PRInt32 *aRowIndex, PRInt32 *aColIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aColIndex=0; 
  NS_ENSURE_ARG_POINTER(aColIndex);
  *aRowIndex=0;
  nsresult res=NS_ERROR_NOT_INITIALIZED;
  if (!aCell)
  {
    
    nsCOMPtr<nsIDOMElement> cell;
    res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nsnull, getter_AddRefs(cell));
    if (NS_SUCCEEDED(res) && cell)
      aCell = cell;
    else
      return NS_ERROR_FAILURE;
  }

  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIContent> nodeAsContent( do_QueryInterface(aCell) );
  NS_ENSURE_TRUE(nodeAsContent, NS_ERROR_FAILURE);
  
  nsIFrame *layoutObject = nodeAsContent->GetPrimaryFrame();
  NS_ENSURE_TRUE(layoutObject, NS_ERROR_FAILURE);

  nsITableCellLayout *cellLayoutObject = do_QueryFrame(layoutObject);
  NS_ENSURE_TRUE(cellLayoutObject, NS_ERROR_FAILURE);
  return cellLayoutObject->GetCellIndexes(*aRowIndex, *aColIndex);
}

NS_IMETHODIMP
nsHTMLEditor::GetTableLayoutObject(nsIDOMElement* aTable, nsITableLayout **tableLayoutObject)
{
  *tableLayoutObject = nsnull;
  NS_ENSURE_TRUE(aTable, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDocWeak, NS_ERROR_NOT_INITIALIZED);
  nsCOMPtr<nsIPresShell> ps = GetPresShell();
  NS_ENSURE_TRUE(ps, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsIContent> nodeAsContent( do_QueryInterface(aTable) );
  NS_ENSURE_TRUE(nodeAsContent, NS_ERROR_FAILURE);
  
  nsIFrame *layoutObject = nodeAsContent->GetPrimaryFrame();
  NS_ENSURE_TRUE(layoutObject, NS_ERROR_FAILURE);

  *tableLayoutObject = do_QueryFrame(layoutObject);
  return *tableLayoutObject ? NS_OK : NS_NOINTERFACE;
}


PRBool nsHTMLEditor::GetNumberOfCellsInRow(nsIDOMElement* aTable, PRInt32 rowIndex)
{
  PRInt32 cellCount = 0;
  nsCOMPtr<nsIDOMElement> cell;
  PRInt32 colIndex = 0;
  nsresult res;
  do {
    PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
    PRBool  isSelected;
    res = GetCellDataAt(aTable, rowIndex, colIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    NS_ENSURE_SUCCESS(res, res);
    if (cell)
    {
      
      if (startRowIndex == rowIndex)
        cellCount++;
      
      
      colIndex += actualColSpan;
    }
    else
      colIndex++;

  } while (cell);

  return cellCount;
}




NS_IMETHODIMP
nsHTMLEditor::GetTableSize(nsIDOMElement *aTable,
                           PRInt32* aRowCount, PRInt32* aColCount)
{
  NS_ENSURE_ARG_POINTER(aRowCount);
  NS_ENSURE_ARG_POINTER(aColCount);
  nsresult res;
  *aRowCount = 0;
  *aColCount = 0;
  nsCOMPtr<nsIDOMElement> table;
  
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), aTable, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);
  
  
  nsITableLayout *tableLayoutObject;
  res = GetTableLayoutObject(table.get(), &tableLayoutObject);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(tableLayoutObject, NS_ERROR_FAILURE);

  return tableLayoutObject->GetTableSize(*aRowCount, *aColCount); 
}

NS_IMETHODIMP 
nsHTMLEditor::GetCellDataAt(nsIDOMElement* aTable, PRInt32 aRowIndex,
                            PRInt32 aColIndex, nsIDOMElement **aCell, 
                            PRInt32* aStartRowIndex, PRInt32* aStartColIndex, 
                            PRInt32* aRowSpan, PRInt32* aColSpan, 
                            PRInt32* aActualRowSpan, PRInt32* aActualColSpan, 
                            PRBool* aIsSelected)
{
  NS_ENSURE_ARG_POINTER(aStartRowIndex);
  NS_ENSURE_ARG_POINTER(aStartColIndex);
  NS_ENSURE_ARG_POINTER(aRowSpan);
  NS_ENSURE_ARG_POINTER(aColSpan);
  NS_ENSURE_ARG_POINTER(aActualRowSpan);
  NS_ENSURE_ARG_POINTER(aActualColSpan);
  NS_ENSURE_ARG_POINTER(aIsSelected);
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);

  nsresult res=NS_ERROR_FAILURE;
  *aStartRowIndex = 0;
  *aStartColIndex = 0;
  *aRowSpan = 0;
  *aColSpan = 0;
  *aActualRowSpan = 0;
  *aActualColSpan = 0;
  *aIsSelected = PR_FALSE;

  *aCell = nsnull;

  if (!aTable)
  {
    
    nsCOMPtr<nsIDOMElement> table;
    res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), nsnull, getter_AddRefs(table));
    NS_ENSURE_SUCCESS(res, res);
    if (table)
      aTable = table;
    else
      return NS_ERROR_FAILURE;
  }
  
  
  nsITableLayout *tableLayoutObject;
  res = GetTableLayoutObject(aTable, &tableLayoutObject);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(tableLayoutObject, NS_ERROR_FAILURE);

  
  
  nsCOMPtr<nsIDOMElement> cell;
  res = tableLayoutObject->GetCellDataAt(aRowIndex, aColIndex,
                                         *getter_AddRefs(cell), 
                                         *aStartRowIndex, *aStartColIndex,
                                         *aRowSpan, *aColSpan, 
                                         *aActualRowSpan, *aActualColSpan, 
                                         *aIsSelected);
  if (cell)
  {
    *aCell = cell.get();
    NS_ADDREF(*aCell);
  }
  
  if (res == NS_TABLELAYOUT_CELL_NOT_FOUND) res = NS_EDITOR_ELEMENT_NOT_FOUND;
  return res;
}


NS_IMETHODIMP 
nsHTMLEditor::GetCellAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, nsIDOMElement **aCell)
{
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;
  return GetCellDataAt(aTable, aRowIndex, aColIndex, aCell, 
                       &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                       &actualRowSpan, &actualColSpan, &isSelected);
}


NS_IMETHODIMP
nsHTMLEditor::GetCellSpansAt(nsIDOMElement* aTable, PRInt32 aRowIndex, PRInt32 aColIndex, 
                             PRInt32& aActualRowSpan, PRInt32& aActualColSpan)
{
  nsCOMPtr<nsIDOMElement> cell;    
  PRInt32 startRowIndex, startColIndex, rowSpan, colSpan;
  PRBool  isSelected;
  return GetCellDataAt(aTable, aRowIndex, aColIndex, getter_AddRefs(cell), 
                       &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                       &aActualRowSpan, &aActualColSpan, &isSelected);
}

NS_IMETHODIMP
nsHTMLEditor::GetCellContext(nsISelection **aSelection,
                             nsIDOMElement   **aTable,
                             nsIDOMElement   **aCell,
                             nsIDOMNode      **aCellParent, PRInt32 *aCellOffset,
                             PRInt32 *aRowIndex, PRInt32 *aColIndex)
{
  
  if (aSelection) *aSelection = nsnull;
  if (aTable) *aTable = nsnull;
  if (aCell) *aCell = nsnull;
  if (aCellParent) *aCellParent = nsnull;
  if (aCellOffset) *aCellOffset = 0;
  if (aRowIndex) *aRowIndex = 0;
  if (aColIndex) *aColIndex = 0;

  nsCOMPtr <nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  if (aSelection)
  {
    *aSelection = selection.get();
    NS_ADDREF(*aSelection);
  }
  nsCOMPtr <nsIDOMElement> table;
  nsCOMPtr <nsIDOMElement> cell;

  
  if (aCell && *aCell)
    cell = *aCell;

  
  
  
  if (!cell)
  {
    
    nsCOMPtr<nsIDOMElement> cellOrTableElement;
    PRInt32 selectedCount;
    nsAutoString tagName;
    res = GetSelectedOrParentTableElement(tagName, &selectedCount,
                                          getter_AddRefs(cellOrTableElement));
    NS_ENSURE_SUCCESS(res, res);
    if (tagName.EqualsLiteral("table"))
    {
      
      if (aTable)
      {
        *aTable = cellOrTableElement.get();
        NS_ADDREF(*aTable);
      }
      return NS_OK;
    }
    if (!tagName.EqualsLiteral("td"))
      return NS_EDITOR_ELEMENT_NOT_FOUND;

    
    cell = cellOrTableElement;
  }
  if (aCell)
  {
    *aCell = cell.get();
    NS_ADDREF(*aCell);
  }

  
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), cell, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);
  if (aTable)
  {
    *aTable = table.get();
    NS_ADDREF(*aTable);
  }

  
  if (aRowIndex || aColIndex)
  {
    PRInt32 rowIndex, colIndex;
    
    res = GetCellIndexes(cell, &rowIndex, &colIndex);
    if(NS_FAILED(res)) return res;
    if (aRowIndex) *aRowIndex = rowIndex;
    if (aColIndex) *aColIndex = colIndex;
  }
  if (aCellParent)
  {
    nsCOMPtr <nsIDOMNode> cellParent;
    
    res = cell->GetParentNode(getter_AddRefs(cellParent));
    NS_ENSURE_SUCCESS(res, res);
    
    NS_ENSURE_TRUE(cellParent, NS_ERROR_FAILURE);

    *aCellParent = cellParent.get();
    NS_ADDREF(*aCellParent);

    if (aCellOffset)
      res = GetChildOffset(cell, cellParent, *aCellOffset);
  }

  return res;
}

nsresult 
nsHTMLEditor::GetCellFromRange(nsIDOMRange *aRange, nsIDOMElement **aCell)
{
  
  
  NS_ENSURE_TRUE(aRange && aCell, NS_ERROR_NULL_POINTER);

  *aCell = nsnull;

  nsCOMPtr<nsIDOMNode> startParent;
  nsresult res = aRange->GetStartContainer(getter_AddRefs(startParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);

  PRInt32 startOffset;
  res = aRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> childNode = GetChildAt(startParent, startOffset);
  
  NS_ENSURE_TRUE(childNode, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> endParent;
  res = aRange->GetEndContainer(getter_AddRefs(endParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);

  PRInt32 endOffset;
  res = aRange->GetEndOffset(&endOffset);
  NS_ENSURE_SUCCESS(res, res);
  
  
  
  
  if (startParent == endParent && 
      endOffset == startOffset+1 &&
      nsHTMLEditUtils::IsTableCell(childNode))
  {
    
    
    nsCOMPtr<nsIDOMElement> cellElement = do_QueryInterface(childNode);
    *aCell = cellElement.get();
    NS_ADDREF(*aCell);
    return NS_OK;
  }
  return NS_EDITOR_ELEMENT_NOT_FOUND;
}

NS_IMETHODIMP 
nsHTMLEditor::GetFirstSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  *aCell = nsnull;
  if (aRange) *aRange = nsnull;

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMRange> range;
  res = selection->GetRangeAt(0, getter_AddRefs(range));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

  mSelectedCellIndex = 0;

  res = GetCellFromRange(range, aCell);
  
  
  NS_ENSURE_SUCCESS(res, NS_EDITOR_ELEMENT_NOT_FOUND);
  
  NS_ENSURE_TRUE(*aCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  if (aRange)
  {
    *aRange = range.get();
    NS_ADDREF(*aRange);
  }

  
  mSelectedCellIndex = 1;

  return res;  
}

NS_IMETHODIMP
nsHTMLEditor::GetNextSelectedCell(nsIDOMRange **aRange, nsIDOMElement **aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  *aCell = nsnull;
  if (aRange) *aRange = nsnull;

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  PRInt32 rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  
  if (mSelectedCellIndex >= rangeCount) 
    return NS_EDITOR_ELEMENT_NOT_FOUND;

  
  nsCOMPtr<nsIDOMRange> range;
  for (; mSelectedCellIndex < rangeCount; mSelectedCellIndex++)
  {
    res = selection->GetRangeAt(mSelectedCellIndex, getter_AddRefs(range));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(range, NS_ERROR_FAILURE);

    res = GetCellFromRange(range, aCell);
    
    NS_ENSURE_SUCCESS(res, NS_EDITOR_ELEMENT_NOT_FOUND);
    
    
    if (*aCell) break;
#ifdef DEBUG_cmanske
    else
      printf("GetNextSelectedCell: Collapsed range found\n");
#endif

    
  }
  
  NS_ENSURE_TRUE(*aCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  if (aRange)
  {
    *aRange = range.get();
    NS_ADDREF(*aRange);
  }

  
  mSelectedCellIndex++;

  return res;  
}

NS_IMETHODIMP 
nsHTMLEditor::GetFirstSelectedCellInTable(PRInt32 *aRowIndex, PRInt32 *aColIndex, nsIDOMElement **aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  *aCell = nsnull;
  if (aRowIndex)
    *aRowIndex = 0;
  if (aColIndex)
    *aColIndex = 0;

  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetFirstSelectedCell(nsnull, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  *aCell = cell.get();
  NS_ADDREF(*aCell);

  
  if (aRowIndex || aColIndex)
  {
    PRInt32 startRowIndex, startColIndex;
    res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
    if(NS_FAILED(res)) return res;

    if (aRowIndex)
      *aRowIndex = startRowIndex;

    if (aColIndex)
      *aColIndex = startColIndex;
  }

  return res;
}

NS_IMETHODIMP
nsHTMLEditor::SetSelectionAfterTableEdit(nsIDOMElement* aTable, PRInt32 aRow, PRInt32 aCol, 
                                     PRInt32 aDirection, PRBool aSelected)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NOT_INITIALIZED);

  nsCOMPtr<nsISelection>selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  
  if (!selection)
  {
#ifdef DEBUG_cmanske
    printf("Selection not found after table manipulation!\n");
#endif
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMElement> cell;
  PRBool done = PR_FALSE;
  do {
    res = GetCellAt(aTable, aRow, aCol, getter_AddRefs(cell));
    if (NS_SUCCEEDED(res))
    {
      if (cell)
      {
        if (aSelected)
        {
          
          return SelectElement(cell);
        }
        else
        {
          
          
          
          
          return CollapseSelectionToDeepestNonTableFirstChild(selection, cell);
        }
      } else {
        
        
        
        switch (aDirection)
        {
          case ePreviousColumn:
            if (aCol == 0)
            {
              if (aRow > 0)
                aRow--;
              else
                done = PR_TRUE;
            }
            else
              aCol--;
            break;
          case ePreviousRow:
            if (aRow == 0)
            {
              if (aCol > 0)
                aCol--;
              else
                done = PR_TRUE;
            }
            else
              aRow--;
            break;
          default:
            done = PR_TRUE;
        }
      }
    }
    else
      break;
  } while (!done);

  
  
  nsCOMPtr<nsIDOMNode> tableParent;
  PRInt32 tableOffset;
  res = aTable->GetParentNode(getter_AddRefs(tableParent));
  if(NS_SUCCEEDED(res) && tableParent)
  {
    if(NS_SUCCEEDED(GetChildOffset(aTable, tableParent, tableOffset)))
      return selection->Collapse(tableParent, tableOffset);
  }
  
  
  return SetSelectionAtDocumentStart(selection);
}

NS_IMETHODIMP 
nsHTMLEditor::GetSelectedOrParentTableElement(nsAString& aTagName,
                                              PRInt32 *aSelectedCount,
                                              nsIDOMElement** aTableElement)
{
  NS_ENSURE_ARG_POINTER(aTableElement);
  NS_ENSURE_ARG_POINTER(aSelectedCount);
  *aTableElement = nsnull;
  aTagName.Truncate();
  *aSelectedCount = 0;

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMElement> tableOrCellElement;
  res = GetFirstSelectedCell(nsnull, getter_AddRefs(tableOrCellElement));
  NS_ENSURE_SUCCESS(res, res);

  NS_NAMED_LITERAL_STRING(tdName, "td");

  if (tableOrCellElement)
  {
      
      
      res = selection->GetRangeCount(aSelectedCount);
      NS_ENSURE_SUCCESS(res, res);
      aTagName = tdName;
  }
  else
  {
    nsCOMPtr<nsIDOMNode> anchorNode;
    res = selection->GetAnchorNode(getter_AddRefs(anchorNode));
    if(NS_FAILED(res)) return res;
    NS_ENSURE_TRUE(anchorNode, NS_ERROR_FAILURE);

    nsCOMPtr<nsIDOMNode> selectedNode;

    
    PRBool hasChildren;
    anchorNode->HasChildNodes(&hasChildren);

    if (hasChildren)
    {
      PRInt32 anchorOffset;
      res = selection->GetAnchorOffset(&anchorOffset);
      NS_ENSURE_SUCCESS(res, res);
      selectedNode = GetChildAt(anchorNode, anchorOffset);
      if (!selectedNode)
      {
        selectedNode = anchorNode;
        
        
      }
      else
      {
        nsCOMPtr<nsIAtom> atom = nsEditor::GetTag(selectedNode);

        if (atom == nsEditProperty::td)
        {
          tableOrCellElement = do_QueryInterface(selectedNode);
          aTagName = tdName;
          
          
          res = selection->GetRangeCount(aSelectedCount);
          NS_ENSURE_SUCCESS(res, res);
        }
        else if (atom == nsEditProperty::table)
        {
          tableOrCellElement = do_QueryInterface(selectedNode);
          aTagName.AssignLiteral("table");
          *aSelectedCount = 1;
        }
        else if (atom == nsEditProperty::tr)
        {
          tableOrCellElement = do_QueryInterface(selectedNode);
          aTagName.AssignLiteral("tr");
          *aSelectedCount = 1;
        }
      }
    }
    if (!tableOrCellElement)
    {
      
      res = GetElementOrParentByTagName(tdName, anchorNode, getter_AddRefs(tableOrCellElement));
      if(NS_FAILED(res)) return res;
      if (tableOrCellElement)
        aTagName = tdName;
    }
  }
  if (tableOrCellElement)
  {
    *aTableElement = tableOrCellElement.get();
    NS_ADDREF(*aTableElement);
  }
  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::GetSelectedCellsType(nsIDOMElement *aElement, PRUint32 *aSelectionType)
{
  NS_ENSURE_ARG_POINTER(aSelectionType);
  *aSelectionType = 0;

  
  
  nsCOMPtr<nsIDOMElement> table;

  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), aElement, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);

  PRInt32 rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMElement> selectedCell;
  res = GetFirstSelectedCell(nsnull, getter_AddRefs(selectedCell));
  NS_ENSURE_SUCCESS(res, res);
  if (res == NS_EDITOR_ELEMENT_NOT_FOUND) return NS_OK;
  
  
  *aSelectionType = nsISelectionPrivate::TABLESELECTION_CELL;

  
  nsTArray<PRInt32> indexArray;

  PRBool allCellsInRowAreSelected = PR_FALSE;
  PRBool allCellsInColAreSelected = PR_FALSE;
  while (NS_SUCCEEDED(res) && selectedCell)
  {
    
    PRInt32 startRowIndex, startColIndex;
    res = GetCellIndexes(selectedCell, &startRowIndex, &startColIndex);
    if(NS_FAILED(res)) return res;
    
    if (!indexArray.Contains(startColIndex))
    {
      indexArray.AppendElement(startColIndex);
      allCellsInRowAreSelected = AllCellsInRowSelected(table, startRowIndex, colCount);
      
      if (!allCellsInRowAreSelected) break;
    }
    res = GetNextSelectedCell(nsnull, getter_AddRefs(selectedCell));
  }

  if (allCellsInRowAreSelected)
  {
    *aSelectionType = nsISelectionPrivate::TABLESELECTION_ROW;
    return NS_OK;
  }
  

  
  indexArray.Clear();

  
  res = GetFirstSelectedCell(nsnull, getter_AddRefs(selectedCell));
  while (NS_SUCCEEDED(res) && selectedCell)
  {
    
    PRInt32 startRowIndex, startColIndex;
    res = GetCellIndexes(selectedCell, &startRowIndex, &startColIndex);
    if(NS_FAILED(res)) return res;
  
    if (!indexArray.Contains(startRowIndex))
    {
      indexArray.AppendElement(startColIndex);
      allCellsInColAreSelected = AllCellsInColumnSelected(table, startColIndex, rowCount);
      
      if (!allCellsInRowAreSelected) break;
    }
    res = GetNextSelectedCell(nsnull, getter_AddRefs(selectedCell));
  }
  if (allCellsInColAreSelected)
    *aSelectionType = nsISelectionPrivate::TABLESELECTION_COLUMN;

  return NS_OK;
}

PRBool 
nsHTMLEditor::AllCellsInRowSelected(nsIDOMElement *aTable, PRInt32 aRowIndex, PRInt32 aNumberOfColumns)
{
  NS_ENSURE_TRUE(aTable, PR_FALSE);

  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  for( PRInt32 col = 0; col < aNumberOfColumns; col += NS_MAX(actualColSpan, 1))
  {
    nsCOMPtr<nsIDOMElement> cell;    
    nsresult res = GetCellDataAt(aTable, aRowIndex, col, getter_AddRefs(cell),
                                 &curStartRowIndex, &curStartColIndex,
                                 &rowSpan, &colSpan,
                                 &actualRowSpan, &actualColSpan, &isSelected);
 
    NS_ENSURE_SUCCESS(res, PR_FALSE);
    
    
    NS_ENSURE_TRUE(cell, (col > 0) ? PR_TRUE : PR_FALSE);

    
    NS_ENSURE_TRUE(isSelected, PR_FALSE);

    NS_ASSERTION((actualColSpan > 0),"ActualColSpan = 0 in AllCellsInRowSelected");
  }
  return PR_TRUE;
}

PRBool 
nsHTMLEditor::AllCellsInColumnSelected(nsIDOMElement *aTable, PRInt32 aColIndex, PRInt32 aNumberOfRows)
{
  NS_ENSURE_TRUE(aTable, PR_FALSE);

  PRInt32 curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  PRBool  isSelected;

  for( PRInt32 row = 0; row < aNumberOfRows; row += NS_MAX(actualRowSpan, 1))
  {
    nsCOMPtr<nsIDOMElement> cell;    
    nsresult res = GetCellDataAt(aTable, row, aColIndex, getter_AddRefs(cell),
                                 &curStartRowIndex, &curStartColIndex,
                                 &rowSpan, &colSpan,
                                 &actualRowSpan, &actualColSpan, &isSelected);
    
    NS_ENSURE_SUCCESS(res, PR_FALSE);
    
    
    NS_ENSURE_TRUE(cell, (row > 0) ? PR_TRUE : PR_FALSE);

    
    NS_ENSURE_TRUE(isSelected, PR_FALSE);
  }
  return PR_TRUE;
}

PRBool 
nsHTMLEditor::IsEmptyCell(nsIDOMElement *aCell)
{
  nsCOMPtr<nsIDOMNode> cellChild;

  
  nsresult res = aCell->GetFirstChild(getter_AddRefs(cellChild));
  NS_ENSURE_SUCCESS(res, PR_FALSE);

  if (cellChild)
  {
    nsCOMPtr<nsIDOMNode> nextChild;
    res = cellChild->GetNextSibling(getter_AddRefs(nextChild));
    NS_ENSURE_SUCCESS(res, PR_FALSE);
    if (!nextChild)
    {
      
      
      PRBool isEmpty = nsTextEditUtils::IsBreak(cellChild);
      
      if (!isEmpty)
      {
        res = IsEmptyNode(cellChild, &isEmpty, PR_FALSE, PR_FALSE);
        NS_ENSURE_SUCCESS(res, PR_FALSE);
      }

      return isEmpty;
    }
  }
  return PR_FALSE;
}
