




#include "XULTreeGridAccessibleWrap.h"

using namespace mozilla::a11y;





XULTreeGridAccessibleWrap::
  XULTreeGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
  XULTreeGridAccessible(aContent, aDoc)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridAccessibleWrap,
                             XULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridAccessibleWrap,
                         AccessibleWrap,
                         CAccessibleTable)






XULTreeGridCellAccessibleWrap::
  XULTreeGridCellAccessibleWrap(nsIContent* aContent,
                                DocAccessible* aDoc,
                                XULTreeGridRowAccessible* aRowAcc,
                                nsITreeBoxObject* aTree,
                                nsITreeView* aTreeView,
                                PRInt32 aRow, nsITreeColumn* aColumn) :
  XULTreeGridCellAccessible(aContent, aDoc, aRowAcc, aTree, aTreeView,
                            aRow, aColumn)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridCellAccessibleWrap,
                             XULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridCellAccessibleWrap,
                         AccessibleWrap,
                         CAccessibleTableCell)
