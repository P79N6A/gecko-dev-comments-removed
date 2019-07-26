




#include "XULTreeGridAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridAccessibleWrap,
                             XULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTable)

void
XULTreeGridAccessibleWrap::Shutdown()
{
  ia2AccessibleTable::mTable = nullptr;
  XULTreeGridAccessible::Shutdown();
}





NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridCellAccessibleWrap,
                             XULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridCellAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTableCell)

void
XULTreeGridCellAccessibleWrap::Shutdown()
{
  ia2AccessibleTableCell::mTableCell = nullptr;
  XULTreeGridCellAccessible::Shutdown();
}
