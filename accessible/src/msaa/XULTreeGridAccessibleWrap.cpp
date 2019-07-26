




#include "XULTreeGridAccessibleWrap.h"

using namespace mozilla::a11y;





NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridAccessibleWrap,
                             XULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTable)

void
XULTreeGridAccessibleWrap::Shutdown()
{
  ia2AccessibleTable::mTable = nullptr;
  XULTreeGridAccessibleWrap::Shutdown();
}





XULTreeGridCellAccessibleWrap::
  XULTreeGridCellAccessibleWrap(nsIContent* aContent,
                                DocAccessible* aDoc,
                                XULTreeGridRowAccessible* aRowAcc,
                                nsITreeBoxObject* aTree,
                                nsITreeView* aTreeView,
                                int32_t aRow, nsITreeColumn* aColumn) :
  XULTreeGridCellAccessible(aContent, aDoc, aRowAcc, aTree, aTreeView,
                            aRow, aColumn)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(XULTreeGridCellAccessibleWrap,
                             XULTreeGridCellAccessible)

IMPL_IUNKNOWN_INHERITED1(XULTreeGridCellAccessibleWrap,
                         AccessibleWrap,
                         ia2AccessibleTableCell)
