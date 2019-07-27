





#ifndef mozilla_a11y_ARIAGridAccessible_inl_h__
#define mozilla_a11y_ARIAGridAccessible_inl_h__

#include "ARIAGridAccessible.h"

#include "AccIterator.h"
#include "nsAccUtils.h"

namespace mozilla {
namespace a11y {

inline int32_t
ARIAGridCellAccessible::RowIndexFor(Accessible* aRow) const
{
  Accessible* table = nsAccUtils::TableFor(aRow);
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

} 
} 

#endif
