






#include "HTMLTableAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableAccessibleWrap,
                             HTMLTableAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTable)

void
HTMLTableAccessibleWrap::Shutdown()
{
  ia2AccessibleTable::mTable = nullptr;
  HTMLTableAccessible::Shutdown();
}





NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableCellAccessibleWrap,
                             HTMLTableCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)

void
HTMLTableCellAccessibleWrap::Shutdown()
{
  ia2AccessibleTableCell::mTableCell = nullptr;
  HTMLTableCellAccessible::Shutdown();
}





NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableHeaderCellAccessibleWrap,
                             HTMLTableHeaderCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableHeaderCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)

void
HTMLTableHeaderCellAccessibleWrap::Shutdown()
{
  ia2AccessibleTableCell::mTableCell = nullptr;
  HTMLTableHeaderCellAccessible::Shutdown();
}
