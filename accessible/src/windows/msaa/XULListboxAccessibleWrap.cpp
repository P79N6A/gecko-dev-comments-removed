




#include "XULListboxAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(XULListboxAccessibleWrap,
                             XULListboxAccessible)

IMPL_IUNKNOWN_QUERY_HEAD(XULListboxAccessibleWrap)
IMPL_IUNKNOWN_QUERY_CLASS_COND(ia2AccessibleTable, IsMulticolumn());
IMPL_IUNKNOWN_QUERY_CLASS(AccessibleWrap)
IMPL_IUNKNOWN_QUERY_TAIL

void
XULListboxAccessibleWrap::Shutdown()
{
  ia2AccessibleTable::mTable = nullptr;
  XULListboxAccessible::Shutdown();
}





NS_IMPL_ISUPPORTS_INHERITED0(XULListCellAccessibleWrap,
                             XULListCellAccessible)

IMPL_IUNKNOWN_INHERITED1(XULListCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)

void
XULListCellAccessibleWrap::Shutdown()
{
  ia2AccessibleTableCell::mTableCell = nullptr;
  XULListCellAccessible::Shutdown();
}
