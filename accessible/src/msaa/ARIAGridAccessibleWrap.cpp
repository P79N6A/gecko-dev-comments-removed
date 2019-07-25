







































#include "ARIAGridAccessibleWrap.h"

using namespace mozilla;
using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(ARIAGridAccessibleWrap,
                             ARIAGridAccessible)

IMPL_IUNKNOWN_INHERITED1(ARIAGridAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable)






NS_IMPL_ISUPPORTS_INHERITED0(ARIAGridCellAccessibleWrap,
                             ARIAGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(ARIAGridCellAccessibleWrap,
                         nsHyperTextAccessibleWrap,
                         CAccessibleTableCell)

