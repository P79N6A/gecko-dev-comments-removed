




#include <stdio.h>

#include "mozilla/Assertions.h"
#include "mozilla/dom/Selection.h"
#include "mozilla/dom/Element.h"
#include "nsAString.h"
#include "nsAlgorithm.h"
#include "nsCOMPtr.h"
#include "nsDebug.h"
#include "nsEditor.h"
#include "nsEditorUtils.h"
#include "nsError.h"
#include "nsGkAtoms.h"
#include "nsHTMLEditUtils.h"
#include "nsHTMLEditor.h"
#include "nsIAtom.h"
#include "nsIContent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNode.h"
#include "nsIDOMRange.h"
#include "nsIEditor.h"
#include "nsIFrame.h"
#include "nsIHTMLEditor.h"
#include "nsINode.h"
#include "nsIPresShell.h"
#include "nsISupportsUtils.h"
#include "nsITableCellLayout.h" 
#include "nsITableEditor.h"
#include "nsLiteralString.h"
#include "nsQueryFrame.h"
#include "nsString.h"
#include "nsTArray.h"
#include "nsTableCellFrame.h"
#include "nsTableOuterFrame.h"
#include "nscore.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;




class MOZ_STACK_CLASS nsSetSelectionAfterTableEdit
{
  private:
    nsCOMPtr<nsITableEditor> mEd;
    nsCOMPtr<nsIDOMElement> mTable;
    int32_t mCol, mRow, mDirection, mSelected;
  public:
    nsSetSelectionAfterTableEdit(nsITableEditor *aEd, nsIDOMElement* aTable, 
                                 int32_t aRow, int32_t aCol, int32_t aDirection, 
                                 bool aSelected) : 
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
    
    
    void CancelSetCaret() {mEd = nullptr; mTable = nullptr;}
};


class MOZ_STACK_CLASS nsSelectionBatcherForTable
{
private:
  nsCOMPtr<nsISelectionPrivate> mSelection;
public:
  explicit nsSelectionBatcherForTable(nsISelection *aSelection)
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
nsHTMLEditor::InsertCell(nsIDOMElement *aCell, int32_t aRowSpan, int32_t aColSpan, 
                         bool aAfter, bool aIsHeader, nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nullptr;

  
  nsCOMPtr<nsIDOMNode> cellParent;
  nsresult res = aCell->GetParentNode(getter_AddRefs(cellParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cellParent, NS_ERROR_NULL_POINTER);

  int32_t cellOffset = GetChildOffset(aCell, cellParent);

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

NS_IMETHODIMP nsHTMLEditor::SetColSpan(nsIDOMElement *aCell, int32_t aColSpan)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  nsAutoString newSpan;
  newSpan.AppendInt(aColSpan, 10);
  return SetAttribute(aCell, NS_LITERAL_STRING("colspan"), newSpan);
}

NS_IMETHODIMP nsHTMLEditor::SetRowSpan(nsIDOMElement *aCell, int32_t aRowSpan)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  nsAutoString newSpan;
  newSpan.AppendInt(aRowSpan, 10);
  return SetAttribute(aCell, NS_LITERAL_STRING("rowspan"), newSpan);
}





NS_IMETHODIMP
nsHTMLEditor::InsertTableCell(int32_t aNumber, bool aAfter)
{
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  nsCOMPtr<nsIDOMNode> cellParent;
  int32_t cellOffset, startRowIndex, startColIndex;
  nsresult res = GetCellContext(nullptr,
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                getter_AddRefs(cellParent), &cellOffset,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  int32_t curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex, &rowSpan, &colSpan,
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);
  int32_t newCellIndex = aAfter ? (startColIndex+colSpan) : startColIndex;
  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, newCellIndex, ePreviousColumn, false);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  int32_t i;
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

  *aRowNode = nullptr;  

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

      if (atom == nsGkAtoms::tr) {
        
        *aRowNode = tableChild;
        NS_ADDREF(*aRowNode);
        return NS_OK;
      }
      
      if (atom == nsGkAtoms::tbody ||
          atom == nsGkAtoms::thead ||
          atom == nsGkAtoms::tfoot) {
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

  *aRowNode = nullptr;  

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

  *aCellNode = nullptr;

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
nsHTMLEditor::InsertTableColumn(int32_t aNumber, bool aAfter)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  int32_t startRowIndex, startColIndex;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                nullptr, nullptr,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  int32_t curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex,
                      &rowSpan, &colSpan, 
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  
  if (aAfter)
  {
    startColIndex += actualColSpan;
    
    
    
    
    if (colSpan == 0)
      SetColSpan(curCell, actualColSpan);
  }
   
  int32_t rowCount, colCount, rowIndex;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, false);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  
  
  
  if (startColIndex >= colCount)
    NormalizeTable(table);

  nsCOMPtr<nsIDOMNode> rowNode;
  for ( rowIndex = 0; rowIndex < rowCount; rowIndex++)
  {
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
          res = InsertTableCell(aNumber, false);
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
          res = InsertTableCell(aNumber, true);
        }
      }
    }
  }
  return res;
}

NS_IMETHODIMP
nsHTMLEditor::InsertTableRow(int32_t aNumber, bool aAfter)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> curCell;
  
  int32_t startRowIndex, startColIndex;
  nsresult res = GetCellContext(nullptr,
                                getter_AddRefs(table), 
                                getter_AddRefs(curCell), 
                                nullptr, nullptr, 
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(curCell, NS_EDITOR_ELEMENT_NOT_FOUND);

  
  int32_t curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  res = GetCellDataAt(table, startRowIndex, startColIndex,
                      getter_AddRefs(curCell),
                      &curStartRowIndex, &curStartColIndex,
                      &rowSpan, &colSpan, 
                      &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(curCell, NS_ERROR_FAILURE);
  
  int32_t rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  if (aAfter)
  {
    
    startRowIndex += actualRowSpan;

    
    
    
    
    if (rowSpan == 0)
      SetRowSpan(curCell, actualRowSpan);
  }

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, false);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMElement> cellForRowParent;
  int32_t cellsInRow = 0;
  if (startRowIndex < rowCount)
  {
    
    
    
    int32_t colIndex = 0;
    
    
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
    
    
    int32_t lastRow = rowCount-1;
    int32_t tempColIndex = 0;
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
    int32_t newRowOffset;

    NS_NAMED_LITERAL_STRING(trStr, "tr");
    if (cellForRowParent)
    {
      nsCOMPtr<nsIDOMElement> parentRow;
      res = GetElementOrParentByTagName(trStr, cellForRowParent, getter_AddRefs(parentRow));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(parentRow, NS_ERROR_NULL_POINTER);

      parentRow->GetParentNode(getter_AddRefs(parentOfRow));
      NS_ENSURE_TRUE(parentOfRow, NS_ERROR_NULL_POINTER);

      newRowOffset = GetChildOffset(parentRow, parentOfRow);
      
      
      if (aAfter && startRowIndex >= rowCount)
        newRowOffset++;
    }
    else
      return NS_ERROR_FAILURE;

    for (int32_t row = 0; row < aNumber; row++)
    {
      
      nsCOMPtr<nsIDOMElement> newRow;
      res = CreateElementWithDefaults(trStr, getter_AddRefs(newRow));
      if (NS_SUCCEEDED(res))
      {
        NS_ENSURE_TRUE(newRow, NS_ERROR_FAILURE);
      
        for (int32_t i = 0; i < cellsInRow; i++)
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

  return DeleteSelection(nsIEditor::eNext, nsIEditor::eStrip);
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTable()
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                nullptr, nullptr, nullptr, nullptr, nullptr);
    
  NS_ENSURE_SUCCESS(res, res);

  nsAutoEditBatch beginBatching(this);
  return DeleteTable2(table, selection);
}

NS_IMETHODIMP
nsHTMLEditor::DeleteTableCell(int32_t aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex;


  nsresult res = GetCellContext(getter_AddRefs(selection),
                         getter_AddRefs(table), 
                         getter_AddRefs(cell), 
                         nullptr, nullptr,
                         &startRowIndex, &startColIndex);

  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table && cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  int32_t rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    
    cell = firstCell;

    int32_t rowCount, colCount;
    res = GetTableSize(table, &rowCount, &colCount);
    NS_ENSURE_SUCCESS(res, res);

    
    res = GetCellIndexes(cell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);

    
    nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, false);
    nsAutoTxnsConserveSelection dontChangeSelection(this);

    bool    checkToDeleteRow = true;
    bool    checkToDeleteColumn = true;
    while (cell)
    {
      bool deleteRow = false;
      bool deleteCol = false;

      if (checkToDeleteRow)
      {
        
        
        checkToDeleteRow = false;

        deleteRow = AllCellsInRowSelected(table, startRowIndex, colCount);
        if (deleteRow)
        {
          
          
          int32_t nextRow = startRowIndex;
          while (nextRow == startRowIndex)
          {
            res = GetNextSelectedCell(nullptr, getter_AddRefs(cell));
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
            
            checkToDeleteRow = true;
          }
        }
      }
      if (!deleteRow)
      {
        if (checkToDeleteColumn)
        {
          
          
          checkToDeleteColumn = false;

          deleteCol = AllCellsInColumnSelected(table, startColIndex, colCount);
          if (deleteCol)
          {
            
            
            int32_t nextCol = startColIndex;
            while (nextCol == startColIndex)
            {
              res = GetNextSelectedCell(nullptr, getter_AddRefs(cell));
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
              
              checkToDeleteColumn = true;
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
  else for (int32_t i = 0; i < aNumber; i++)
  {
    res = GetCellContext(getter_AddRefs(selection),
                         getter_AddRefs(table), 
                         getter_AddRefs(cell), 
                         nullptr, nullptr,
                         &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
    
    NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

    if (1 == GetNumberOfCellsInRow(table, startRowIndex))
    {
      nsCOMPtr<nsIDOMElement> parentRow;
      res = GetElementOrParentByTagName(NS_LITERAL_STRING("tr"), cell, getter_AddRefs(parentRow));
      NS_ENSURE_SUCCESS(res, res);
      NS_ENSURE_TRUE(parentRow, NS_ERROR_NULL_POINTER);

      
      
      
      int32_t rowCount, colCount;
      res = GetTableSize(table, &rowCount, &colCount);
      NS_ENSURE_SUCCESS(res, res);
      
      if (rowCount == 1)
        return DeleteTable2(table, selection);
    
      
      res = DeleteTableRow(1);
      NS_ENSURE_SUCCESS(res, res);
    } 
    else
    {
      

      
      nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, false);
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
  int32_t startRowIndex, startColIndex;
  nsresult res;
  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell), 
                       nullptr, nullptr,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);


  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);
  
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

  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, false);

  while (cell)
  {
    DeleteCellContents(cell);
    if (firstCell)
    {
      
      res = GetNextSelectedCell(nullptr, getter_AddRefs(cell));
      NS_ENSURE_SUCCESS(res, res);
    }
    else
      cell = nullptr;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteCellContents(nsIDOMElement *aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);

  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMNode> child;
  bool hasChild;
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
nsHTMLEditor::DeleteTableColumn(int32_t aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex, rowCount, colCount;
  nsresult res = GetCellContext(getter_AddRefs(selection),
                                getter_AddRefs(table), 
                                getter_AddRefs(cell), 
                                nullptr, nullptr,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table && cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  if(startColIndex == 0 && aNumber >= colCount)
    return DeleteTable2(table, selection);

  
  aNumber = std::min(aNumber,(colCount-startColIndex));

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  
  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  int32_t rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    res = GetCellIndexes(firstCell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }
  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, false);

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
      
      
      int32_t nextCol = startColIndex;
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
  else for (int32_t i = 0; i < aNumber; i++)
  {
    res = DeleteColumn(table, startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::DeleteColumn(nsIDOMElement *aTable, int32_t aColIndex)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMElement> cellInDeleteCol;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  int32_t rowIndex = 0;
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

          
          
          
          int32_t rowCount, colCount;
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
nsHTMLEditor::DeleteTableRow(int32_t aNumber)
{
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex;
  int32_t rowCount, colCount;
  nsresult res =  GetCellContext(getter_AddRefs(selection),
                                 getter_AddRefs(table), 
                                 getter_AddRefs(cell), 
                                 nullptr, nullptr,
                                 &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  if(startRowIndex == 0 && aNumber >= rowCount)
    return DeleteTable2(table, selection);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> firstCell;
  nsCOMPtr<nsIDOMRange> range;
  res = GetFirstSelectedCell(getter_AddRefs(range), getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);

  int32_t rangeCount;
  res = selection->GetRangeCount(&rangeCount);
  NS_ENSURE_SUCCESS(res, res);

  if (firstCell && rangeCount > 1)
  {
    
    res = GetCellIndexes(firstCell, &startRowIndex, &startColIndex);
    NS_ENSURE_SUCCESS(res, res);
  }

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousRow, false);
  
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
      
      
      int32_t nextRow = startRowIndex;
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
    
    aNumber = std::min(aNumber,(rowCount-startRowIndex));

    for (int32_t i = 0; i < aNumber; i++)
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
nsHTMLEditor::DeleteRow(nsIDOMElement *aTable, int32_t aRowIndex)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  nsCOMPtr<nsIDOMElement> cell;
  nsCOMPtr<nsIDOMElement> cellInDeleteRow;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  int32_t colIndex = 0;
  nsresult res = NS_OK;
   
  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  
  
  nsTArray<nsCOMPtr<nsIDOMElement> > spanCellList;
  nsTArray<int32_t> newSpanList;

  int32_t rowCount, colCount;
  res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  
  
  do {
    if (aRowIndex >= rowCount || colIndex >= colCount)
      break;

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
          newSpanList.AppendElement(std::max((aRowIndex - startRowIndex), actualRowSpan-1));
        }
      }
      else 
      {
        if (rowSpan > 1)
        {
          
          
          
          res = SplitCellIntoRows(aTable, startRowIndex, startColIndex,
                                  aRowIndex - startRowIndex + 1, 
                                  actualRowSpan - 1, nullptr);    
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

  
  for (uint32_t i = 0, n = spanCellList.Length(); i < n; i++)
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
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), nullptr, getter_AddRefs(table));
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
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cell));
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

  int32_t startRowIndex, startColIndex, endRowIndex, endColIndex;

  
  res = GetCellIndexes(aStartCell, &startRowIndex, &startColIndex);
  if(NS_FAILED(res)) return res;

  res = GetCellIndexes(aEndCell, &endRowIndex, &endColIndex);
  if(NS_FAILED(res)) return res;

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  int32_t minColumn = std::min(startColIndex, endColIndex);
  int32_t minRow    = std::min(startRowIndex, endRowIndex);
  int32_t maxColumn   = std::max(startColIndex, endColIndex);
  int32_t maxRow      = std::max(startRowIndex, endRowIndex);

  nsCOMPtr<nsIDOMElement> cell;
  int32_t currentRowIndex, currentColIndex;
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

  int32_t rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  for (int32_t row = minRow; row <= maxRow; row++)
  {
    for(int32_t col = minColumn; col <= maxColumn; col += std::max(actualColSpan, 1))
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
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsCOMPtr<nsIDOMElement> startCell = cell;
  
  
  nsCOMPtr<nsIDOMElement> table;
  res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), cell, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  if(!table) return NS_ERROR_NULL_POINTER;

  int32_t rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsISelection> selection;
  res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  bool cellSelected = false;
  int32_t rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  bool    isSelected;
  for(int32_t row = 0; row < rowCount; row++)
  {
    for(int32_t col = 0; col < colCount; col += std::max(actualColSpan, 1))
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
        cellSelected = true;
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
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);
  nsCOMPtr<nsIDOMElement> startCell = cell;

  
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  int32_t startRowIndex, startColIndex;

  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell),
                       nullptr, nullptr,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);
  
  int32_t rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  
  

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  bool cellSelected = false;
  int32_t rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  bool    isSelected;
  for(int32_t col = 0; col < colCount; col += std::max(actualColSpan, 1))
  {
    res = GetCellDataAt(table, startRowIndex, col, getter_AddRefs(cell),
                        &currentRowIndex, &currentColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    if (NS_FAILED(res)) break;
    
    if (cell && currentRowIndex == startRowIndex && currentColIndex == col)
    {
      res = AppendNodeToSelectionAsRange(cell);
      if (NS_FAILED(res)) break;
      cellSelected = true;
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
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  
  
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  nsCOMPtr<nsIDOMElement> startCell = cell;
  
  
  nsCOMPtr<nsISelection> selection;
  nsCOMPtr<nsIDOMElement> table;
  int32_t startRowIndex, startColIndex;

  res = GetCellContext(getter_AddRefs(selection),
                       getter_AddRefs(table), 
                       getter_AddRefs(cell),
                       nullptr, nullptr,
                       &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);

  int32_t rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  
  nsSelectionBatcherForTable selectionBatcher(selection);

  
  
  res = ClearSelection();

  
  bool cellSelected = false;
  int32_t rowSpan, colSpan, actualRowSpan, actualColSpan, currentRowIndex, currentColIndex;
  bool    isSelected;
  for(int32_t row = 0; row < rowCount; row += std::max(actualRowSpan, 1))
  {
    res = GetCellDataAt(table, row, startColIndex, getter_AddRefs(cell),
                        &currentRowIndex, &currentColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    if (NS_FAILED(res)) break;
    
    if (cell && currentRowIndex == row && currentColIndex == startColIndex)
    {
      res = AppendNodeToSelectionAsRange(cell);
      if (NS_FAILED(res)) break;
      cellSelected = true;
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
  int32_t startRowIndex, startColIndex, actualRowSpan, actualColSpan;
  
  nsresult res = GetCellContext(nullptr,
                                getter_AddRefs(table), 
                                getter_AddRefs(cell),
                                nullptr, nullptr,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  if(!table || !cell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  
  res = GetCellSpansAt(table, startRowIndex, startColIndex, actualRowSpan, actualColSpan);
  NS_ENSURE_SUCCESS(res, res);

  
  if (actualRowSpan <= 1 && actualColSpan <= 1)
    return NS_OK;
  
  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  
  nsSetSelectionAfterTableEdit setCaret(this, table, startRowIndex, startColIndex, ePreviousColumn, false);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  nsCOMPtr<nsIDOMElement> newCell;
  int32_t rowIndex = startRowIndex;
  int32_t rowSpanBelow, colSpanAfter;

  
  
  for (rowSpanBelow = actualRowSpan-1; rowSpanBelow >= 0; rowSpanBelow--)
  {
    
    if (rowSpanBelow > 0)
    {
      res = SplitCellIntoRows(table, rowIndex, startColIndex, 1, rowSpanBelow, getter_AddRefs(newCell));
      NS_ENSURE_SUCCESS(res, res);
      CopyCellBackgroundColor(newCell, cell);
    }
    int32_t colIndex = startColIndex;
    
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
  bool isSet;
  nsresult res = GetAttributeValue(sourceCell, bgcolor, color, &isSet);

  if (NS_SUCCEEDED(res) && isSet)
    res = SetAttribute(destCell, bgcolor, color);

  return res;
}

NS_IMETHODIMP 
nsHTMLEditor::SplitCellIntoColumns(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aColIndex,
                                   int32_t aColSpanLeft, int32_t aColSpanRight,
                                   nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nullptr;

  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
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
  res = InsertCell(cell, actualRowSpan, aColSpanRight, true, false, getter_AddRefs(newCell));
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
nsHTMLEditor::SplitCellIntoRows(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aColIndex,
                                int32_t aRowSpanAbove, int32_t aRowSpanBelow, 
                                nsIDOMElement **aNewCell)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);
  if (aNewCell) *aNewCell = nullptr;

  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  nsresult res = GetCellDataAt(aTable, aRowIndex, aColIndex, getter_AddRefs(cell),
                               &startRowIndex, &startColIndex,
                               &rowSpan, &colSpan, 
                               &actualRowSpan, &actualColSpan, &isSelected);
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_ERROR_NULL_POINTER);
  
  
  if (actualRowSpan <= 1 || (aRowSpanAbove + aRowSpanBelow) > actualRowSpan)
    return NS_OK;

  int32_t rowCount, colCount;
  res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> cell2;
  nsCOMPtr<nsIDOMElement> lastCellFound;
  int32_t startRowIndex2, startColIndex2, rowSpan2, colSpan2, actualRowSpan2, actualColSpan2;
  bool    isSelected2;
  int32_t colIndex = 0;
  bool insertAfter = (startColIndex > 0);
  
  int32_t rowBelowIndex = startRowIndex+aRowSpanAbove;
  
  
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
          
          insertAfter = false;
          break;
        }
      }
      else
      {
        break; 
      }
      lastCellFound = cell2;
    }
    
    colIndex += std::max(actualColSpan2, 1);

    
    if (colIndex > colCount)
        break;

  } while(true);

  if (!cell2 && lastCellFound)
  {
    
    
    
    
    cell2 = lastCellFound;
    insertAfter = true; 
  }

  
  res = SetRowSpan(cell, aRowSpanAbove);
  NS_ENSURE_SUCCESS(res, res);


  
  
  nsCOMPtr<nsIDOMElement> newCell;
  res = InsertCell(cell2, aRowSpanBelow, actualColSpan, insertAfter, false, getter_AddRefs(newCell));
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
  nsCOMPtr<Element> sourceCell = do_QueryInterface(aSourceCell);
  NS_ENSURE_TRUE(sourceCell, NS_ERROR_NULL_POINTER);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  
  
  
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);
  nsAutoSelectionReset selectionResetter(selection, this);

  
  nsCOMPtr<nsIAtom> atom = nsEditor::GetTag(aSourceCell);
  nsIAtom* newCellType = atom == nsGkAtoms::td ? nsGkAtoms::th : nsGkAtoms::td;

  
  
  nsCOMPtr<Element> newNode = ReplaceContainer(sourceCell, newCellType,
      nullptr, nullptr, nsEditor::eCloneAttributes);
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
nsHTMLEditor::JoinTableCells(bool aMergeNonContiguousContents)
{
  nsCOMPtr<nsIDOMElement> table;
  nsCOMPtr<nsIDOMElement> targetCell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;
  nsCOMPtr<nsIDOMElement> cell2;
  int32_t startRowIndex2, startColIndex2, rowSpan2, colSpan2, actualRowSpan2, actualColSpan2;
  bool    isSelected2;

  
  nsresult res = GetCellContext(nullptr,
                                getter_AddRefs(table), 
                                getter_AddRefs(targetCell),
                                nullptr, nullptr,
                                &startRowIndex, &startColIndex);
  NS_ENSURE_SUCCESS(res, res);
  if(!table || !targetCell) return NS_EDITOR_ELEMENT_NOT_FOUND;

  nsAutoEditBatch beginBatching(this);
  
  nsAutoTxnsConserveSelection dontChangeSelection(this);

  
  
  

  nsCOMPtr<nsIDOMElement> firstCell;
  int32_t firstRowIndex, firstColIndex;
  res = GetFirstSelectedCellInTable(&firstRowIndex, &firstColIndex, getter_AddRefs(firstCell));
  NS_ENSURE_SUCCESS(res, res);
  
  bool joinSelectedCells = false;
  if (firstCell)
  {
    nsCOMPtr<nsIDOMElement> secondCell;
    res = GetNextSelectedCell(nullptr, getter_AddRefs(secondCell));
    NS_ENSURE_SUCCESS(res, res);

    
    joinSelectedCells = (secondCell != nullptr);
  }

  if (joinSelectedCells)
  {
    
    

    int32_t rowCount, colCount;
    res = GetTableSize(table, &rowCount, &colCount);
    NS_ENSURE_SUCCESS(res, res);

    
    int32_t firstRowSpan, firstColSpan;
    res = GetCellSpansAt( table, firstRowIndex, firstColIndex, firstRowSpan, firstColSpan);
    NS_ENSURE_SUCCESS(res, res);

    
    
    
    
    
    int32_t lastRowIndex = firstRowIndex;
    int32_t lastColIndex = firstColIndex;
    int32_t rowIndex, colIndex;

    
    
    
    for (rowIndex = firstRowIndex; rowIndex <= lastRowIndex; rowIndex++)
    {
      int32_t currentRowCount = rowCount;
      
      res = FixBadRowSpan(table, rowIndex, rowCount);
      NS_ENSURE_SUCCESS(res, res);
      
      lastRowIndex -= (currentRowCount-rowCount);

      bool cellFoundInRow = false;
      bool lastRowIsSet = false;
      int32_t lastColInRow = 0;
      int32_t firstColInRow = firstColIndex;
      for (colIndex = firstColIndex; colIndex < colCount; colIndex += std::max(actualColSpan2, 1))
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
            
            
            
            
            
            
            lastRowIndex = std::max(0,rowIndex - 1);
            lastRowIsSet = true;
            break;
          }
          
          lastColInRow = colIndex + (actualColSpan2-1);
          cellFoundInRow = true;
        }
        else if (cellFoundInRow)
        {
          
          
          if (rowIndex > (firstRowIndex+1) && colIndex <= lastColIndex)
          {
            
            
            lastRowIndex = std::max(0,rowIndex - 1);
            lastRowIsSet = true;
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
            
            
            
            lastRowIndex = std::max(0,rowIndex - 1);
          }
          else
          {
            
            lastRowIndex = rowIndex+1;
          }
        }
        
        lastColIndex = std::min(lastColIndex, lastColInRow);
      }
      else
      {
        
        
        lastRowIndex = std::max(0,rowIndex - 1);
      }
    }
  
    
    nsTArray<nsCOMPtr<nsIDOMElement> > deleteList;

    
    for (rowIndex = 0; rowIndex < rowCount; rowIndex++)
    {
      for (colIndex = 0; colIndex < colCount; colIndex += std::max(actualColSpan2, 1))
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
              
              
              int32_t extraColSpan = (startColIndex2 + actualColSpan2) - (lastColIndex+1);
              if ( extraColSpan > 0)
              {
                res = SplitCellIntoColumns(table, startRowIndex2, startColIndex2, 
                                           actualColSpan2-extraColSpan, extraColSpan, nullptr);
                NS_ENSURE_SUCCESS(res, res);
              }
            }

            res = MergeCells(firstCell, cell2, false);
            NS_ENSURE_SUCCESS(res, res);
            
            
            deleteList.AppendElement(cell2.get());
          }
          else if (aMergeNonContiguousContents)
          {
            
            res = MergeCells(firstCell, cell2, false);
            NS_ENSURE_SUCCESS(res, res);
          }
        }
      }
    }

    
    
    nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

    for (uint32_t i = 0, n = deleteList.Length(); i < n; i++)
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

    int32_t rangeCount;
    res = selection->GetRangeCount(&rangeCount);
    NS_ENSURE_SUCCESS(res, res);

    nsCOMPtr<nsIDOMRange> range;
    int32_t i;
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

    
    
    int32_t spanAboveMergedCell = startRowIndex - startRowIndex2;
    int32_t effectiveRowSpan2 = actualRowSpan2 - spanAboveMergedCell;

    if (effectiveRowSpan2 > actualRowSpan)
    {
      
      
      res = SplitCellIntoRows(table, startRowIndex2, startColIndex2,
                              spanAboveMergedCell+actualRowSpan, 
                              effectiveRowSpan2-actualRowSpan, nullptr);
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
                         bool aDeleteCellToMerge)
{
  nsCOMPtr<dom::Element> targetCell = do_QueryInterface(aTargetCell);
  nsCOMPtr<dom::Element> cellToMerge = do_QueryInterface(aCellToMerge);
  NS_ENSURE_TRUE(targetCell && cellToMerge, NS_ERROR_NULL_POINTER);

  
  nsAutoRules beginRulesSniffing(this, EditAction::deleteNode, nsIEditor::eNext);

  
  if (!IsEmptyCell(cellToMerge)) {
    
    
    
    int32_t insertIndex = 0;

    
    uint32_t len = targetCell->GetChildCount();
    if (len == 1 && IsEmptyCell(targetCell)) {
      
      nsIContent* cellChild = targetCell->GetFirstChild();
      nsresult res = DeleteNode(cellChild->AsDOMNode());
      NS_ENSURE_SUCCESS(res, res);
      insertIndex = 0;
    } else {
      insertIndex = (int32_t)len;
    }

    
    while (cellToMerge->HasChildren()) {
      nsCOMPtr<nsIDOMNode> cellChild = cellToMerge->GetLastChild()->AsDOMNode();
      nsresult res = DeleteNode(cellChild);
      NS_ENSURE_SUCCESS(res, res);

      res = InsertNode(cellChild, aTargetCell, insertIndex);
      NS_ENSURE_SUCCESS(res, res);
    }
  }

  
  if (aDeleteCellToMerge)
    return DeleteNode(aCellToMerge);

  return NS_OK;
}


NS_IMETHODIMP 
nsHTMLEditor::FixBadRowSpan(nsIDOMElement *aTable, int32_t aRowIndex, int32_t& aNewRowCount)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  int32_t rowCount, colCount;
  nsresult res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement>cell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;

  int32_t minRowSpan = -1;
  int32_t colIndex;
  
  for( colIndex = 0; colIndex < colCount; colIndex += std::max(actualColSpan, 1))
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
    
    
    int32_t rowsReduced = minRowSpan - 1;
    for(colIndex = 0; colIndex < colCount; colIndex += std::max(actualColSpan, 1))
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
nsHTMLEditor::FixBadColSpan(nsIDOMElement *aTable, int32_t aColIndex, int32_t& aNewColCount)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NULL_POINTER);

  int32_t rowCount, colCount;
  nsresult res = GetTableSize(aTable, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;

  int32_t minColSpan = -1;
  int32_t rowIndex;
  
  for( rowIndex = 0; rowIndex < rowCount; rowIndex += std::max(actualRowSpan, 1))
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
    
    
    int32_t colsReduced = minColSpan - 1;
    for(rowIndex = 0; rowIndex < rowCount; rowIndex += std::max(actualRowSpan, 1))
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
  nsRefPtr<Selection> selection = GetSelection();
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMElement> table;
  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"),
                                             aTable, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);
  
  NS_ENSURE_TRUE(table, NS_OK);

  int32_t rowCount, colCount, rowIndex, colIndex;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsAutoSelectionReset selectionResetter(selection, this);

  nsAutoEditBatch beginBatching(this);
  
  nsAutoRules beginRulesSniffing(this, EditAction::insertNode, nsIEditor::eNext);

  nsCOMPtr<nsIDOMElement> cell;
  int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;

  
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
          
          res = InsertCell(previousCellInRow, 1, 1, true, false, getter_AddRefs(cell));
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
                             int32_t *aRowIndex, int32_t *aColIndex)
{
  NS_ENSURE_ARG_POINTER(aRowIndex);
  *aColIndex=0; 
  NS_ENSURE_ARG_POINTER(aColIndex);
  *aRowIndex=0;
  nsresult res=NS_ERROR_NOT_INITIALIZED;
  if (!aCell)
  {
    
    nsCOMPtr<nsIDOMElement> cell;
    res = GetElementOrParentByTagName(NS_LITERAL_STRING("td"), nullptr, getter_AddRefs(cell));
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

nsTableOuterFrame*
nsHTMLEditor::GetTableFrame(nsIDOMElement* aTable)
{
  NS_ENSURE_TRUE(aTable, nullptr);

  nsCOMPtr<nsIContent> nodeAsContent( do_QueryInterface(aTable) );
  NS_ENSURE_TRUE(nodeAsContent, nullptr);
  return do_QueryFrame(nodeAsContent->GetPrimaryFrame());
}


int32_t nsHTMLEditor::GetNumberOfCellsInRow(nsIDOMElement* aTable, int32_t rowIndex)
{
  int32_t cellCount = 0;
  nsCOMPtr<nsIDOMElement> cell;
  int32_t colIndex = 0;
  nsresult res;
  do {
    int32_t startRowIndex, startColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
    bool    isSelected;
    res = GetCellDataAt(aTable, rowIndex, colIndex, getter_AddRefs(cell),
                        &startRowIndex, &startColIndex, &rowSpan, &colSpan, 
                        &actualRowSpan, &actualColSpan, &isSelected);
    NS_ENSURE_SUCCESS(res, 0);
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
                           int32_t* aRowCount, int32_t* aColCount)
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
  
  nsTableOuterFrame* tableFrame = GetTableFrame(table.get());
  NS_ENSURE_TRUE(tableFrame, NS_ERROR_FAILURE);

  *aRowCount = tableFrame->GetRowCount();
  *aColCount = tableFrame->GetColCount();

  return NS_OK;
}

NS_IMETHODIMP 
nsHTMLEditor::GetCellDataAt(nsIDOMElement* aTable, int32_t aRowIndex,
                            int32_t aColIndex, nsIDOMElement **aCell, 
                            int32_t* aStartRowIndex, int32_t* aStartColIndex, 
                            int32_t* aRowSpan, int32_t* aColSpan, 
                            int32_t* aActualRowSpan, int32_t* aActualColSpan, 
                            bool* aIsSelected)
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
  *aIsSelected = false;

  *aCell = nullptr;

  if (!aTable)
  {
    
    nsCOMPtr<nsIDOMElement> table;
    res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), nullptr, getter_AddRefs(table));
    NS_ENSURE_SUCCESS(res, res);
    if (table)
      aTable = table;
    else
      return NS_ERROR_FAILURE;
  }
  
  nsTableOuterFrame* tableFrame = GetTableFrame(aTable);
  NS_ENSURE_TRUE(tableFrame, NS_ERROR_FAILURE);

  nsTableCellFrame* cellFrame =
    tableFrame->GetCellFrameAt(aRowIndex, aColIndex);
  if (!cellFrame)
    return NS_ERROR_FAILURE;

  *aIsSelected = cellFrame->IsSelected();
  cellFrame->GetRowIndex(*aStartRowIndex);
  cellFrame->GetColIndex(*aStartColIndex);
  *aRowSpan = cellFrame->GetRowSpan();
  *aColSpan = cellFrame->GetColSpan();
  *aActualRowSpan = tableFrame->GetEffectiveRowSpanAt(aRowIndex, aColIndex);
  *aActualColSpan = tableFrame->GetEffectiveColSpanAt(aRowIndex, aColIndex);
  nsCOMPtr<nsIDOMElement> domCell = do_QueryInterface(cellFrame->GetContent());
  domCell.forget(aCell);

  return NS_OK;
}


NS_IMETHODIMP 
nsHTMLEditor::GetCellAt(nsIDOMElement* aTable, int32_t aRowIndex, int32_t aColIndex, nsIDOMElement **aCell)
{
  NS_ENSURE_ARG_POINTER(aCell);
  *aCell = nullptr;

  if (!aTable)
  {
    
    nsCOMPtr<nsIDOMElement> table;
    nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), nullptr, getter_AddRefs(table));
    NS_ENSURE_SUCCESS(res, res);
    NS_ENSURE_TRUE(table, NS_ERROR_FAILURE);
    aTable = table;
  }

  nsTableOuterFrame* tableFrame = GetTableFrame(aTable);
  if (!tableFrame)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMElement> domCell =
    do_QueryInterface(tableFrame->GetCellAt(aRowIndex, aColIndex));
  domCell.forget(aCell);

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLEditor::GetCellSpansAt(nsIDOMElement* aTable, int32_t aRowIndex, int32_t aColIndex, 
                             int32_t& aActualRowSpan, int32_t& aActualColSpan)
{
  nsTableOuterFrame* tableFrame = GetTableFrame(aTable);
  if (!tableFrame)
    return NS_ERROR_FAILURE;

  aActualRowSpan = tableFrame->GetEffectiveRowSpanAt(aRowIndex, aColIndex);
  aActualColSpan = tableFrame->GetEffectiveColSpanAt(aRowIndex, aColIndex);

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLEditor::GetCellContext(nsISelection **aSelection,
                             nsIDOMElement   **aTable,
                             nsIDOMElement   **aCell,
                             nsIDOMNode      **aCellParent, int32_t *aCellOffset,
                             int32_t *aRowIndex, int32_t *aColIndex)
{
  
  if (aSelection) *aSelection = nullptr;
  if (aTable) *aTable = nullptr;
  if (aCell) *aCell = nullptr;
  if (aCellParent) *aCellParent = nullptr;
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
    int32_t selectedCount;
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
    int32_t rowIndex, colIndex;
    
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

    if (aCellOffset) {
      *aCellOffset = GetChildOffset(cell, cellParent);
    }
  }

  return res;
}

nsresult 
nsHTMLEditor::GetCellFromRange(nsIDOMRange *aRange, nsIDOMElement **aCell)
{
  
  
  NS_ENSURE_TRUE(aRange && aCell, NS_ERROR_NULL_POINTER);

  *aCell = nullptr;

  nsCOMPtr<nsIDOMNode> startParent;
  nsresult res = aRange->GetStartContainer(getter_AddRefs(startParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);

  int32_t startOffset;
  res = aRange->GetStartOffset(&startOffset);
  NS_ENSURE_SUCCESS(res, res);

  nsCOMPtr<nsIDOMNode> childNode = GetChildAt(startParent, startOffset);
  
  if (!childNode) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMNode> endParent;
  res = aRange->GetEndContainer(getter_AddRefs(endParent));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(startParent, NS_ERROR_FAILURE);

  int32_t endOffset;
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
  *aCell = nullptr;
  if (aRange) *aRange = nullptr;

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
  
  
  if (NS_FAILED(res)) {
    return NS_EDITOR_ELEMENT_NOT_FOUND;
  }
  
  if (!*aCell) {
    return NS_EDITOR_ELEMENT_NOT_FOUND;
  }

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
  *aCell = nullptr;
  if (aRange) *aRange = nullptr;

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  int32_t rangeCount;
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
nsHTMLEditor::GetFirstSelectedCellInTable(int32_t *aRowIndex, int32_t *aColIndex, nsIDOMElement **aCell)
{
  NS_ENSURE_TRUE(aCell, NS_ERROR_NULL_POINTER);
  *aCell = nullptr;
  if (aRowIndex)
    *aRowIndex = 0;
  if (aColIndex)
    *aColIndex = 0;

  nsCOMPtr<nsIDOMElement> cell;
  nsresult res = GetFirstSelectedCell(nullptr, getter_AddRefs(cell));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(cell, NS_EDITOR_ELEMENT_NOT_FOUND);

  *aCell = cell.get();
  NS_ADDREF(*aCell);

  
  if (aRowIndex || aColIndex)
  {
    int32_t startRowIndex, startColIndex;
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
nsHTMLEditor::SetSelectionAfterTableEdit(nsIDOMElement* aTable, int32_t aRow, int32_t aCol, 
                                     int32_t aDirection, bool aSelected)
{
  NS_ENSURE_TRUE(aTable, NS_ERROR_NOT_INITIALIZED);

  nsRefPtr<Selection> selection = GetSelection();
  nsresult res;
  
  if (!selection)
  {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIDOMElement> cell;
  bool done = false;
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
          
          
          
          
          nsCOMPtr<nsINode> cellNode = do_QueryInterface(cell);
          if (cellNode) {
            CollapseSelectionToDeepestNonTableFirstChild(selection, cellNode);
          }
          return NS_OK;
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
                done = true;
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
                done = true;
            }
            else
              aRow--;
            break;
          default:
            done = true;
        }
      }
    }
    else
      break;
  } while (!done);

  
  
  nsCOMPtr<nsIDOMNode> tableParent;
  res = aTable->GetParentNode(getter_AddRefs(tableParent));
  if(NS_SUCCEEDED(res) && tableParent)
  {
    int32_t tableOffset = GetChildOffset(aTable, tableParent);
    return selection->Collapse(tableParent, tableOffset);
  }
  
  
  return SetSelectionAtDocumentStart(selection);
}

NS_IMETHODIMP 
nsHTMLEditor::GetSelectedOrParentTableElement(nsAString& aTagName,
                                              int32_t *aSelectedCount,
                                              nsIDOMElement** aTableElement)
{
  NS_ENSURE_ARG_POINTER(aTableElement);
  NS_ENSURE_ARG_POINTER(aSelectedCount);
  *aTableElement = nullptr;
  aTagName.Truncate();
  *aSelectedCount = 0;

  nsCOMPtr<nsISelection> selection;
  nsresult res = GetSelection(getter_AddRefs(selection));
  NS_ENSURE_SUCCESS(res, res);
  NS_ENSURE_TRUE(selection, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIDOMElement> tableOrCellElement;
  res = GetFirstSelectedCell(nullptr, getter_AddRefs(tableOrCellElement));
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

    
    bool hasChildren;
    anchorNode->HasChildNodes(&hasChildren);

    if (hasChildren)
    {
      int32_t anchorOffset;
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

        if (atom == nsGkAtoms::td) {
          tableOrCellElement = do_QueryInterface(selectedNode);
          aTagName = tdName;
          
          
          res = selection->GetRangeCount(aSelectedCount);
          NS_ENSURE_SUCCESS(res, res);
        } else if (atom == nsGkAtoms::table) {
          tableOrCellElement = do_QueryInterface(selectedNode);
          aTagName.AssignLiteral("table");
          *aSelectedCount = 1;
        } else if (atom == nsGkAtoms::tr) {
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
nsHTMLEditor::GetSelectedCellsType(nsIDOMElement *aElement, uint32_t *aSelectionType)
{
  NS_ENSURE_ARG_POINTER(aSelectionType);
  *aSelectionType = 0;

  
  
  nsCOMPtr<nsIDOMElement> table;

  nsresult res = GetElementOrParentByTagName(NS_LITERAL_STRING("table"), aElement, getter_AddRefs(table));
  NS_ENSURE_SUCCESS(res, res);

  int32_t rowCount, colCount;
  res = GetTableSize(table, &rowCount, &colCount);
  NS_ENSURE_SUCCESS(res, res);

  
  nsCOMPtr<nsIDOMElement> selectedCell;
  res = GetFirstSelectedCell(nullptr, getter_AddRefs(selectedCell));
  NS_ENSURE_SUCCESS(res, res);
  if (res == NS_EDITOR_ELEMENT_NOT_FOUND) return NS_OK;
  
  
  *aSelectionType = nsISelectionPrivate::TABLESELECTION_CELL;

  
  nsTArray<int32_t> indexArray;

  bool allCellsInRowAreSelected = false;
  bool allCellsInColAreSelected = false;
  while (NS_SUCCEEDED(res) && selectedCell)
  {
    
    int32_t startRowIndex, startColIndex;
    res = GetCellIndexes(selectedCell, &startRowIndex, &startColIndex);
    if(NS_FAILED(res)) return res;
    
    if (!indexArray.Contains(startColIndex))
    {
      indexArray.AppendElement(startColIndex);
      allCellsInRowAreSelected = AllCellsInRowSelected(table, startRowIndex, colCount);
      
      if (!allCellsInRowAreSelected) break;
    }
    res = GetNextSelectedCell(nullptr, getter_AddRefs(selectedCell));
  }

  if (allCellsInRowAreSelected)
  {
    *aSelectionType = nsISelectionPrivate::TABLESELECTION_ROW;
    return NS_OK;
  }
  

  
  indexArray.Clear();

  
  res = GetFirstSelectedCell(nullptr, getter_AddRefs(selectedCell));
  while (NS_SUCCEEDED(res) && selectedCell)
  {
    
    int32_t startRowIndex, startColIndex;
    res = GetCellIndexes(selectedCell, &startRowIndex, &startColIndex);
    if(NS_FAILED(res)) return res;
  
    if (!indexArray.Contains(startRowIndex))
    {
      indexArray.AppendElement(startColIndex);
      allCellsInColAreSelected = AllCellsInColumnSelected(table, startColIndex, rowCount);
      
      if (!allCellsInRowAreSelected) break;
    }
    res = GetNextSelectedCell(nullptr, getter_AddRefs(selectedCell));
  }
  if (allCellsInColAreSelected)
    *aSelectionType = nsISelectionPrivate::TABLESELECTION_COLUMN;

  return NS_OK;
}

bool 
nsHTMLEditor::AllCellsInRowSelected(nsIDOMElement *aTable, int32_t aRowIndex, int32_t aNumberOfColumns)
{
  NS_ENSURE_TRUE(aTable, false);

  int32_t curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;

  for( int32_t col = 0; col < aNumberOfColumns; col += std::max(actualColSpan, 1))
  {
    nsCOMPtr<nsIDOMElement> cell;    
    nsresult res = GetCellDataAt(aTable, aRowIndex, col, getter_AddRefs(cell),
                                 &curStartRowIndex, &curStartColIndex,
                                 &rowSpan, &colSpan,
                                 &actualRowSpan, &actualColSpan, &isSelected);
 
    NS_ENSURE_SUCCESS(res, false);
    
    
    NS_ENSURE_TRUE(cell, (col > 0) ? true : false);

    
    NS_ENSURE_TRUE(isSelected, false);

    NS_ASSERTION((actualColSpan > 0),"ActualColSpan = 0 in AllCellsInRowSelected");
  }
  return true;
}

bool 
nsHTMLEditor::AllCellsInColumnSelected(nsIDOMElement *aTable, int32_t aColIndex, int32_t aNumberOfRows)
{
  NS_ENSURE_TRUE(aTable, false);

  int32_t curStartRowIndex, curStartColIndex, rowSpan, colSpan, actualRowSpan, actualColSpan;
  bool    isSelected;

  for( int32_t row = 0; row < aNumberOfRows; row += std::max(actualRowSpan, 1))
  {
    nsCOMPtr<nsIDOMElement> cell;    
    nsresult res = GetCellDataAt(aTable, row, aColIndex, getter_AddRefs(cell),
                                 &curStartRowIndex, &curStartColIndex,
                                 &rowSpan, &colSpan,
                                 &actualRowSpan, &actualColSpan, &isSelected);
    
    NS_ENSURE_SUCCESS(res, false);
    
    
    NS_ENSURE_TRUE(cell, (row > 0) ? true : false);

    
    NS_ENSURE_TRUE(isSelected, false);
  }
  return true;
}

bool 
nsHTMLEditor::IsEmptyCell(dom::Element* aCell)
{
  MOZ_ASSERT(aCell);

  
  nsCOMPtr<nsINode> cellChild = aCell->GetFirstChild();
  if (!cellChild) {
    return false;
  }

  nsCOMPtr<nsINode> nextChild = cellChild->GetNextSibling();
  if (nextChild) {
    return false;
  }

  
  
  if (cellChild->IsElement() && cellChild->AsElement()->IsHTML(nsGkAtoms::br)) {
    return true;
  }

  bool isEmpty;
  
  nsresult rv = IsEmptyNode(cellChild, &isEmpty, false, false);
  NS_ENSURE_SUCCESS(rv, false);
  return isEmpty;
}
