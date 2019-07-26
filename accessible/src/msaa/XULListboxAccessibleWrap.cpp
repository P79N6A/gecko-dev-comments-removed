




#include "XULListboxAccessibleWrap.h"

using namespace mozilla::a11y;





XULListboxAccessibleWrap::
  XULListboxAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  XULListboxAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULListboxAccessibleWrap,
                             XULListboxAccessible)

IMPL_IUNKNOWN_QUERY_HEAD(XULListboxAccessibleWrap)
IMPL_IUNKNOWN_QUERY_CLASS_COND(ia2AccessibleTable, IsMulticolumn());
IMPL_IUNKNOWN_QUERY_CLASS(AccessibleWrap)
IMPL_IUNKNOWN_QUERY_TAIL






XULListCellAccessibleWrap::
  XULListCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  XULListCellAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULListCellAccessibleWrap,
                             XULListCellAccessible)

IMPL_IUNKNOWN_INHERITED1(XULListCellAccessibleWrap,
                         HyperTextAccessibleWrap,
                         ia2AccessibleTableCell)
