





#ifndef mozilla_a11y_xpcom_xpcAccessibletableCell_h_
#define mozilla_a11y_xpcom_xpcAccessibletableCell_h_

#include "nsIAccessibleTable.h"

#include "xpcAccessibleHyperText.h"

namespace mozilla {
namespace a11y {




class xpcAccessibleTableCell : public xpcAccessibleHyperText,
                               public nsIAccessibleTableCell
{
public:
  explicit xpcAccessibleTableCell(Accessible* aIntl) :
    xpcAccessibleHyperText(aIntl) { }

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetTable(nsIAccessibleTable** aTable) MOZ_FINAL;
  NS_IMETHOD GetColumnIndex(int32_t* aColIdx) MOZ_FINAL;
  NS_IMETHOD GetRowIndex(int32_t* aRowIdx) MOZ_FINAL;
  NS_IMETHOD GetColumnExtent(int32_t* aExtent) MOZ_FINAL;
  NS_IMETHOD GetRowExtent(int32_t* aExtent) MOZ_FINAL;
  NS_IMETHOD GetColumnHeaderCells(nsIArray** aHeaderCells) MOZ_FINAL;
  NS_IMETHOD GetRowHeaderCells(nsIArray** aHeaderCells) MOZ_FINAL;
  NS_IMETHOD IsSelected(bool* aSelected) MOZ_FINAL;

protected:
  virtual ~xpcAccessibleTableCell() {}

private:
  TableCellAccessible* Intl() { return mIntl->AsTableCell(); }

  xpcAccessibleTableCell(const xpcAccessibleTableCell&) MOZ_DELETE;
  xpcAccessibleTableCell& operator =(const xpcAccessibleTableCell&) MOZ_DELETE;
};

} 
} 

#endif 
