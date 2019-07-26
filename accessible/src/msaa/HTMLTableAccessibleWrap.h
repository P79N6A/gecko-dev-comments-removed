






#ifndef mozilla_a11y_HTMLTableAccessibleWrap_h__
#define mozilla_a11y_HTMLTableAccessibleWrap_h__

#include "HTMLTableAccessible.h"

#include "ia2AccessibleTable.h"
#include "ia2AccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class HTMLTableAccessibleWrap : public HTMLTableAccessible,
                                public ia2AccessibleTable
{
public:
  HTMLTableAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableAccessible(aContent, aDoc), ia2AccessibleTable(this)  {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() MOZ_OVERRIDE;
};






class HTMLTableCellAccessibleWrap : public HTMLTableCellAccessible,
                                    public ia2AccessibleTableCell
{
public:
  HTMLTableCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class HTMLTableHeaderCellAccessibleWrap : public HTMLTableHeaderCellAccessible,
                                          public ia2AccessibleTableCell
{
public:
  HTMLTableHeaderCellAccessibleWrap(nsIContent* aContent,
                                    DocAccessible* aDoc) :
    HTMLTableHeaderCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

} 
} 

#endif

