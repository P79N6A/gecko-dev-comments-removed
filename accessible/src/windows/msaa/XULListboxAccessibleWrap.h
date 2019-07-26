




#ifndef mozilla_a11y_XULListboxAccessibleWrap_h__
#define mozilla_a11y_XULListboxAccessibleWrap_h__

#include "XULListboxAccessible.h"

#include "ia2AccessibleTable.h"
#include "ia2AccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class XULListboxAccessibleWrap : public XULListboxAccessible,
                                 public ia2AccessibleTable
{
public:
  XULListboxAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    XULListboxAccessible(aContent, aDoc), ia2AccessibleTable(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};





class XULListCellAccessibleWrap : public XULListCellAccessible,
                                  public ia2AccessibleTableCell
{
public:
  XULListCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    XULListCellAccessible(aContent, aDoc), ia2AccessibleTableCell(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};

} 
} 

#endif
