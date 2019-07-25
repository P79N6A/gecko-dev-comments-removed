






































#include "nsXULTreeGridAccessibleWrap.h"





nsXULTreeGridAccessibleWrap::
  nsXULTreeGridAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
  nsXULTreeGridAccessible(aContent, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridAccessibleWrap,
                             nsXULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable)






nsXULTreeGridCellAccessibleWrap::
  nsXULTreeGridCellAccessibleWrap(nsIContent *aContent,
                                  nsIWeakReference *aShell,
                                  nsXULTreeGridRowAccessible *aRowAcc,
                                  nsITreeBoxObject *aTree,
                                  nsITreeView *aTreeView,
                                  PRInt32 aRow, nsITreeColumn* aColumn) :
  nsXULTreeGridCellAccessible(aContent, aShell, aRowAcc, aTree, aTreeView,
                              aRow, aColumn)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridCellAccessibleWrap,
                             nsXULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridCellAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTableCell)
