







































#include "nsARIAGridAccessibleWrap.h"





NS_IMPL_ISUPPORTS_INHERITED0(nsARIAGridAccessibleWrap,
                             nsARIAGridAccessible)

IMPL_IUNKNOWN_INHERITED1(nsARIAGridAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable)






NS_IMPL_ISUPPORTS_INHERITED0(nsARIAGridCellAccessibleWrap,
                             nsARIAGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsARIAGridCellAccessibleWrap,
                         nsHyperTextAccessibleWrap,
                         CAccessibleTableCell)
