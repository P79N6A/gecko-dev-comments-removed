





#ifndef MOZILLA_A11Y_XPCOM_XPACESSIBLETABLECELL_H_
#define MOZILLA_A11Y_XPCOM_XPACESSIBLETABLECELL_H_

namespace mozilla {
namespace a11y {
class TableAccessible;
class TableCellAccessible;
}
}

class xpcAccessibleTableCell
{
public:
  xpcAccessibleTableCell(mozilla::a11y::TableCellAccessible* aTableCell) :
    mTableCell(aTableCell) { }

protected:
  mozilla::a11y::TableCellAccessible* mTableCell;
};

#define NS_DECL_OR_FORWARD_NSIACCESSIBLETABLECELL_WITH_XPCACCESSIBLETABLECELL \
  NS_IMETHOD GetTable(nsIAccessibleTable * *aTable); \
  NS_IMETHOD GetColumnIndex(PRInt32 *aColumnIndex); \
  NS_IMETHOD GetRowIndex(PRInt32 *aRowIndex); \
  NS_IMETHOD GetColumnExtent(PRInt32 *aColumnExtent); \
  NS_IMETHOD GetRowExtent(PRInt32 *aRowExtent); \
  NS_IMETHOD GetColumnHeaderCells(nsIArray * *aColumnHeaderCells); \
  NS_IMETHOD GetRowHeaderCells(nsIArray * *aRowHeaderCells); \
  NS_IMETHOD IsSelected(bool *_retval ); 

#endif 
