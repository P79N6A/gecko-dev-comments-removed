




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
  XULListboxAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class XULListCellAccessibleWrap : public XULListCellAccessible,
                                  public ia2AccessibleTableCell
{
public:
  XULListCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif
