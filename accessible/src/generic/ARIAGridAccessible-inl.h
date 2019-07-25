





#ifndef mozilla_a11y_ARIAGridAccessible_inl_h__
#define mozilla_a11y_ARIAGridAccessible_inl_h__

#include "ARIAGridAccessible.h"

#include "AccIterator.h"

inline Accessible*
mozilla::a11y::ARIAGridCellAccessible::TableFor(Accessible* aRow) const
{
  if (aRow) {
    Accessible* table = aRow->Parent();
    if (table) {
      roles::Role tableRole = table->Role();
      if (tableRole == roles::SECTION) { 
        table = table->Parent();
        if (table)
          tableRole = table->Role();
      }

      return tableRole == roles::TABLE || tableRole == roles::TREE_TABLE ?
        table : nullptr;
    }
  }

  return nullptr;
}

inline int32_t
mozilla::a11y::ARIAGridCellAccessible::RowIndexFor(Accessible* aRow) const
{
  Accessible* table = TableFor(aRow);
  if (table) {
    int32_t rowIdx = 0;
    Accessible* row = nullptr;
    AccIterator rowIter(table, filters::GetRow);
    while ((row = rowIter.Next()) && row != aRow)
      rowIdx++;

    if (row)
      return rowIdx;
  }

  return -1;
}

#endif
