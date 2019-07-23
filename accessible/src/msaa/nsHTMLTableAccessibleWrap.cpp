







































#include "nsHTMLTableAccessibleWrap.h"





NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTableAccessibleWrap,
                             nsHTMLTableAccessible)

IMPL_IUNKNOWN_INHERITED1(nsHTMLTableAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable)






NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTableCellAccessibleWrap,
                             nsHTMLTableCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsHTMLTableCellAccessibleWrap,
                         nsHyperTextAccessibleWrap,
                         CAccessibleTableCell)






NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTableHeaderCellAccessibleWrap,
                             nsHTMLTableHeaderCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsHTMLTableHeaderCellAccessibleWrap,
                         nsHyperTextAccessibleWrap,
                         CAccessibleTableCell)
