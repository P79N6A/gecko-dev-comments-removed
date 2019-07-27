





#ifndef mozilla_a11y_xpcAccessibleTable_h_
#define mozilla_a11y_xpcAccessibleTable_h_

#include "nsIAccessibleTable.h"
#include "xpcAccessibleGeneric.h"

namespace mozilla {
namespace a11y {




class xpcAccessibleTable : public xpcAccessibleGeneric,
                           public nsIAccessibleTable
{
public:
  explicit xpcAccessibleTable(Accessible* aIntl) :
    xpcAccessibleGeneric(aIntl) { }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetCaption(nsIAccessible** aCaption) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSummary(nsAString& aSummary) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetColumnCount(int32_t* aColumnCount) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRowCount(int32_t* aRowCount) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetCellAt(int32_t aRowIndex, int32_t aColumnIndex,
                       nsIAccessible** aCell) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetCellIndexAt(int32_t aRowIndex, int32_t aColumnIndex,
                            int32_t* aCellIndex) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetColumnIndexAt(int32_t aCellIndex, int32_t* aColumnIndex)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRowIndexAt(int32_t aCellIndex, int32_t* aRowIndex)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRowAndColumnIndicesAt(int32_t aCellIndex, int32_t* aRowIndex,
                                      int32_t* aColumnIndex)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetColumnExtentAt(int32_t row, int32_t column,
                               int32_t* aColumnExtent) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRowExtentAt(int32_t row, int32_t column,
                            int32_t* aRowExtent) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetColumnDescription(int32_t aColIdx, nsAString& aDescription)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetRowDescription(int32_t aRowIdx, nsAString& aDescription)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD IsColumnSelected(int32_t aColIdx, bool* _retval)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD IsRowSelected(int32_t aRowIdx, bool* _retval)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD IsCellSelected(int32_t aRowIdx, int32_t aColIdx, bool* _retval)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedCellCount(uint32_t* aSelectedCellCount)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedColumnCount(uint32_t* aSelectedColumnCount)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedRowCount(uint32_t* aSelectedRowCount)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedCells(nsIArray** aSelectedCell) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedCellIndices(uint32_t* aCellsArraySize,
                                    int32_t** aCellsArray)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedColumnIndices(uint32_t* aColsArraySize,
                                      int32_t** aColsArray)
    MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD GetSelectedRowIndices(uint32_t* aRowsArraySize,
                                   int32_t** aRowsArray) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD SelectColumn(int32_t aColIdx) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD SelectRow(int32_t aRowIdx) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD UnselectColumn(int32_t aColIdx) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD UnselectRow(int32_t aRowIdx) MOZ_FINAL MOZ_OVERRIDE;
  NS_IMETHOD IsProbablyForLayout(bool* aIsForLayout) MOZ_FINAL MOZ_OVERRIDE;

protected:
  virtual ~xpcAccessibleTable() {}

private:
  TableAccessible* Intl() { return mIntl->AsTable(); }

  xpcAccessibleTable(const xpcAccessibleTable&) = delete;
  xpcAccessibleTable& operator =(const xpcAccessibleTable&) = delete;
};

} 
} 

#endif 
