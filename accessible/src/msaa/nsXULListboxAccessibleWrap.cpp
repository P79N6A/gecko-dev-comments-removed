





































#include "nsXULListboxAccessibleWrap.h"





nsXULListboxAccessibleWrap::
  nsXULListboxAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsXULListboxAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULListboxAccessibleWrap,
                             nsXULListboxAccessible)

IMPL_IUNKNOWN_QUERY_HEAD(nsXULListboxAccessibleWrap)
IMPL_IUNKNOWN_QUERY_ENTRY_COND(CAccessibleTable, IsMulticolumn());
IMPL_IUNKNOWN_QUERY_ENTRY(nsAccessibleWrap)
IMPL_IUNKNOWN_QUERY_TAIL






nsXULListCellAccessibleWrap::
  nsXULListCellAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
  nsXULListCellAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULListCellAccessibleWrap,
                             nsXULListCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULListCellAccessibleWrap,
                         nsHyperTextAccessibleWrap,
                         CAccessibleTableCell)
