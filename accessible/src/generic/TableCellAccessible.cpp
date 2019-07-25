





#include "TableCellAccessible.h"

#include "Accessible-inl.h"
#include "TableAccessible.h"

using namespace mozilla;
using namespace mozilla::a11y;

void
TableCellAccessible::RowHeaderCells(nsTArray<Accessible*>* aCells)
{
  uint32_t rowIdx = RowIdx(), colIdx = ColIdx();
  TableAccessible* table = Table();
  if (!table)
    return;

  
  for (uint32_t curColIdx = colIdx - 1; curColIdx < colIdx; curColIdx--) {
    Accessible* cell = table->CellAt(rowIdx, curColIdx);
    if (!cell)
      continue;

    
    TableCellAccessible* tableCell = cell->AsTableCell();
    NS_ASSERTION(tableCell, "cell should be a table cell!");
    if (!tableCell)
      continue;

    
    
    if (tableCell->ColIdx() == curColIdx && cell->Role() == roles::ROWHEADER)
      aCells->AppendElement(cell);
  }
}

void
TableCellAccessible::ColHeaderCells(nsTArray<Accessible*>* aCells)
{
  uint32_t rowIdx = RowIdx(), colIdx = ColIdx();
  TableAccessible* table = Table();
  if (!table)
    return;

  
  for (uint32_t curRowIdx = rowIdx - 1; curRowIdx < rowIdx; curRowIdx--) {
    Accessible* cell = table->CellAt(curRowIdx, colIdx);
    if (!cell)
      continue;

    
    TableCellAccessible* tableCell = cell->AsTableCell();
    NS_ASSERTION(tableCell, "cell should be a table cell!");
    if (!tableCell)
      continue;

    
    
    if (tableCell->RowIdx() == curRowIdx && cell->Role() == roles::COLUMNHEADER)
      aCells->AppendElement(cell);
  }
}
