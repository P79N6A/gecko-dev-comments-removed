






#include "HTMLTableAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableAccessibleWrap,
                             HTMLTableAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTable)






NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableCellAccessibleWrap,
                             HTMLTableCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)






NS_IMPL_ISUPPORTS_INHERITED0(HTMLTableHeaderCellAccessibleWrap,
                             HTMLTableHeaderCellAccessible)

IMPL_IUNKNOWN_INHERITED1(HTMLTableHeaderCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)
