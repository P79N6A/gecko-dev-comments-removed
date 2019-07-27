





#ifndef mozilla_a11y_xpcom_xpcAccessibletableCell_h_
#define mozilla_a11y_xpcom_xpcAccessibletableCell_h_

#include "nscore.h"

class nsIAccessibleTable;
class nsIArray;

namespace mozilla {
namespace a11y {

class TableAccessible;
class TableCellAccessible;





class xpcAccessibleTableCell
{
public:
  explicit xpcAccessibleTableCell(mozilla::a11y::TableCellAccessible* aTableCell) :
    mTableCell(aTableCell) { }

  nsresult GetTable(nsIAccessibleTable** aTable);
  nsresult GetColumnIndex(int32_t* aColIdx);
  nsresult GetRowIndex(int32_t* aRowIdx);
  nsresult GetColumnExtent(int32_t* aExtent);
  nsresult GetRowExtent(int32_t* aExtent);
  nsresult GetColumnHeaderCells(nsIArray** aHeaderCells);
  nsresult GetRowHeaderCells(nsIArray** aHeaderCells);
  nsresult IsSelected(bool* aSelected);

protected:
  mozilla::a11y::TableCellAccessible* mTableCell;
};

} 
} 

#endif 
