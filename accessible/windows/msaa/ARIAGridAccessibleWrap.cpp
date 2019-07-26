






#include "ARIAGridAccessibleWrap.h"

using namespace mozilla;
using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(ARIAGridAccessibleWrap,
                             ARIAGridAccessible)

IMPL_IUNKNOWN_INHERITED1(ARIAGridAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTable)

void
ARIAGridAccessibleWrap::Shutdown()
{
  ia2AccessibleTable::mTable = nullptr;
  ARIAGridAccessible::Shutdown();
}





NS_IMPL_ISUPPORTS_INHERITED0(ARIAGridCellAccessibleWrap,
                             ARIAGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(ARIAGridCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)

void
ARIAGridCellAccessibleWrap::Shutdown()
{
  ia2AccessibleTableCell::mTableCell = nullptr;
  ARIAGridCellAccessible::Shutdown();
}
