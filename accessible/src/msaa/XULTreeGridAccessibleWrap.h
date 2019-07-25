




#ifndef mozilla_a11y_XULTreeGridAccessibleWrap_h__
#define mozilla_a11y_XULTreeGridAccessibleWrap_h__

#include "XULTreeGridAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class XULTreeGridAccessibleWrap : public XULTreeGridAccessible,
                                  public CAccessibleTable
{
public:
  XULTreeGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class XULTreeGridCellAccessibleWrap : public XULTreeGridCellAccessible,
                                      public CAccessibleTableCell
{
public:
  XULTreeGridCellAccessibleWrap(nsIContent* aContent,
                                DocAccessible* aDoc,
                                XULTreeGridRowAccessible* aRowAcc,
                                nsITreeBoxObject* aTree,
                                nsITreeView* aTreeView,
                                int32_t aRow, nsITreeColumn* aColumn);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif
