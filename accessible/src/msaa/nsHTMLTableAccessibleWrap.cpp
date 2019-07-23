







































#include "nsHTMLTableAccessibleWrap.h"

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTableAccessibleWrap,
                             nsHTMLTableAccessible)

IMPL_IUNKNOWN_INHERITED1(nsHTMLTableAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable);

NS_IMPL_ISUPPORTS_INHERITED0(nsHTMLTableHeadAccessibleWrap,
                             nsHTMLTableHeadAccessible)

IMPL_IUNKNOWN_INHERITED1(nsHTMLTableHeadAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable);

