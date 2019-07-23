






































#include "nsXULTreeGridAccessibleWrap.h"





nsXULTreeGridAccessibleWrap::
  nsXULTreeGridAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsXULTreeGridAccessible(aDOMNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridAccessibleWrap,
                             nsXULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable)






nsXULTreeGridCellAccessibleWrap::
  nsXULTreeGridCellAccessibleWrap(nsIDOMNode *aDOMNode,
                                  nsIWeakReference *aShell,
                                  nsXULTreeGridRowAccessible *aRowAcc,
                                  nsITreeBoxObject *aTree,
                                  nsITreeView *aTreeView,
                                  PRInt32 aRow, nsITreeColumn* aColumn) :
  nsXULTreeGridCellAccessible(aDOMNode, aShell, aRowAcc, aTree, aTreeView,
                              aRow, aColumn)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridCellAccessibleWrap,
                             nsXULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridCellAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTableCell)
