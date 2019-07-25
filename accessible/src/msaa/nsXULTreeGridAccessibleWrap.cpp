




#include "nsXULTreeGridAccessibleWrap.h"





nsXULTreeGridAccessibleWrap::
  nsXULTreeGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  nsXULTreeGridAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridAccessibleWrap,
                             nsXULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridAccessibleWrap,
                         AccessibleWrap,
                         CAccessibleTable)






nsXULTreeGridCellAccessibleWrap::
  nsXULTreeGridCellAccessibleWrap(nsIContent* aContent,
                                  DocAccessible* aDoc,
                                  nsXULTreeGridRowAccessible* aRowAcc,
                                  nsITreeBoxObject* aTree,
                                  nsITreeView* aTreeView,
                                  PRInt32 aRow, nsITreeColumn* aColumn) :
  nsXULTreeGridCellAccessible(aContent, aDoc, aRowAcc, aTree, aTreeView,
                              aRow, aColumn)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridCellAccessibleWrap,
                             nsXULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridCellAccessibleWrap,
                         AccessibleWrap,
                         CAccessibleTableCell)
