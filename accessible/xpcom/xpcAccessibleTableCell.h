





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

  
  NS_IMETHOD GetTable(nsIAccessibleTable** aTable) final override;
  NS_IMETHOD GetColumnIndex(int32_t* aColIdx) final override;
  NS_IMETHOD GetRowIndex(int32_t* aRowIdx) final override;
  NS_IMETHOD GetColumnExtent(int32_t* aExtent) final override;
  NS_IMETHOD GetRowExtent(int32_t* aExtent) final override;
  NS_IMETHOD GetColumnHeaderCells(nsIArray** aHeaderCells) final override;
  NS_IMETHOD GetRowHeaderCells(nsIArray** aHeaderCells) final override;
  NS_IMETHOD IsSelected(bool* aSelected) final override;

protected:
  virtual ~xpcAccessibleTableCell() {}

private:
  TableCellAccessible* Intl() { return mIntl->AsTableCell(); }

  xpcAccessibleTableCell(const xpcAccessibleTableCell&) = delete;
  xpcAccessibleTableCell& operator =(const xpcAccessibleTableCell&) = delete;
};

} 
} 

#endif 
