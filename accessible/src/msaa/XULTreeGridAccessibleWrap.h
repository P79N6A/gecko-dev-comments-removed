




#ifndef mozilla_a11y_XULTreeGridAccessibleWrap_h__
#define mozilla_a11y_XULTreeGridAccessibleWrap_h__

#include "XULTreeGridAccessible.h"

#include "ia2AccessibleTable.h"
#include "ia2AccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class XULTreeGridAccessibleWrap : public XULTreeGridAccessible,
                                  public ia2AccessibleTable
{
public:
  XULTreeGridAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    XULTreeGridAccessible(aContent, aDoc), ia2AccessibleTable(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};





class XULTreeGridCellAccessibleWrap : public XULTreeGridCellAccessible,
                                      public ia2AccessibleTableCell
{
public:
  XULTreeGridCellAccessibleWrap(nsIContent* aContent,
                                DocAccessible* aDoc,
                                XULTreeGridRowAccessible* aRowAcc,
                                nsITreeBoxObject* aTree,
                                nsITreeView* aTreeView,
                                int32_t aRow, nsITreeColumn* aColumn) :
    XULTreeGridCellAccessible(aContent, aDoc, aRowAcc, aTree, aTreeView, aRow,
                              aColumn), ia2AccessibleTableCell(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};

} 
} 

#endif
