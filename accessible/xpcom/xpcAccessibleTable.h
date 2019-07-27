





#ifndef MOZILLA_A11Y_XPCOM_XPACCESSIBLETABLE_H_
#define MOZILLA_A11Y_XPCOM_XPACCESSIBLETABLE_H_

#include "nsAString.h"
#include "nscore.h"

class nsIAccessible;
class nsIArray;

namespace mozilla {
namespace a11y {

class TableAccessible;

class xpcAccessibleTable
{
public:
  explicit xpcAccessibleTable(TableAccessible* aTable) :
    mTable(aTable)
  {
  }

  nsresult GetCaption(nsIAccessible** aCaption);
  nsresult GetSummary(nsAString& aSummary);
  nsresult GetColumnCount(int32_t* aColumnCount);
  nsresult GetRowCount(int32_t* aRowCount);
  nsresult GetCellAt(int32_t aRowIndex, int32_t aColumnIndex,
                     nsIAccessible** aCell);
  nsresult GetCellIndexAt(int32_t aRowIndex, int32_t aColumnIndex,
                          int32_t* aCellIndex);
  nsresult GetColumnIndexAt(int32_t aCellIndex, int32_t* aColumnIndex);
  nsresult GetRowIndexAt(int32_t aCellIndex, int32_t* aRowIndex);
  nsresult GetRowAndColumnIndicesAt(int32_t aCellIndex, int32_t* aRowIndex,
                                    int32_t* aColumnIndex);
  nsresult GetColumnExtentAt(int32_t row, int32_t column,
                             int32_t* aColumnExtent);
  nsresult GetRowExtentAt(int32_t row, int32_t column,
                          int32_t* aRowExtent);
  nsresult GetColumnDescription(int32_t aColIdx, nsAString& aDescription);
  nsresult GetRowDescription(int32_t aRowIdx, nsAString& aDescription);
  nsresult IsColumnSelected(int32_t aColIdx, bool* _retval);
  nsresult IsRowSelected(int32_t aRowIdx, bool* _retval);
  nsresult IsCellSelected(int32_t aRowIdx, int32_t aColIdx, bool* _retval);
  nsresult GetSelectedCellCount(uint32_t* aSelectedCellCount);
  nsresult GetSelectedColumnCount(uint32_t* aSelectedColumnCount);
  nsresult GetSelectedRowCount(uint32_t* aSelectedRowCount);
  nsresult GetSelectedCells(nsIArray** aSelectedCell);
  nsresult GetSelectedCellIndices(uint32_t* aCellsArraySize,
                                  int32_t** aCellsArray);
  nsresult GetSelectedColumnIndices(uint32_t* aColsArraySize,
                                    int32_t** aColsArray);
  nsresult GetSelectedRowIndices(uint32_t* aRowsArraySize,
                                 int32_t** aRowsArray);
  nsresult SelectColumn(int32_t aColIdx);
  nsresult SelectRow(int32_t aRowIdx);
  nsresult UnselectColumn(int32_t aColIdx);
  nsresult UnselectRow(int32_t aRowIdx);
  nsresult IsProbablyForLayout(bool* aIsForLayout);

protected:
  mozilla::a11y::TableAccessible* mTable;
};

} 
} 

#endif 
