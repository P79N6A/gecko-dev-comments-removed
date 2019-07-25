






#ifndef mozilla_a11y_HTMLTableAccessibleWrap_h__
#define mozilla_a11y_HTMLTableAccessibleWrap_h__

#include "HTMLTableAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"

namespace mozilla {
namespace a11y {





class HTMLTableAccessibleWrap : public HTMLTableAccessible,
                                public CAccessibleTable
{
public:
  HTMLTableAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class HTMLTableCellAccessibleWrap : public HTMLTableCellAccessible,
                                    public CAccessibleTableCell
{
public:
  HTMLTableCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};






class HTMLTableHeaderCellAccessibleWrap : public HTMLTableHeaderCellAccessible,
                                          public CAccessibleTableCell
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

