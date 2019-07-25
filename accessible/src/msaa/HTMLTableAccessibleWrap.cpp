






#include "HTMLTableAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableAccessibleWrap,
                             HTMLTableAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableAccessibleWrap,
                         AccessibleWrap,
                         CAccessibleTable)






NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableCellAccessibleWrap,
                             HTMLTableCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         CAccessibleTableCell)






NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableHeaderCellAccessibleWrap,
                             HTMLTableHeaderCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableHeaderCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         CAccessibleTableCell)
