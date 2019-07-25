






































#ifndef __nsXULTreeGridAccessibleWrap_h__
#define __nsXULTreeGridAccessibleWrap_h__

#include "nsXULTreeGridAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsXULTreeGridAccessibleWrap : public nsXULTreeGridAccessible,
                                    public CAccessibleTable
{
public:
  nsXULTreeGridAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsXULTreeGridCellAccessibleWrap : public nsXULTreeGridCellAccessible,
                                        public CAccessibleTableCell
{
public:
  nsXULTreeGridCellAccessibleWrap(nsIContent *aContent,
                                  nsIWeakReference *aShell,
                                  nsXULTreeGridRowAccessible *aRowAcc,
                                  nsITreeBoxObject *aTree,
                                  nsITreeView *aTreeView,
                                  PRInt32 aRow, nsITreeColumn* aColumn);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
