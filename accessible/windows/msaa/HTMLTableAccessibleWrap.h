






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
  ~HTMLTableAccessibleWrap() {}

public:
  HTMLTableAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableAccessible(aContent, aDoc), ia2AccessibleTable(this)  {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() override;
};






class HTMLTableCellAccessibleWrap : public HTMLTableCellAccessible,
                                    public ia2AccessibleTableCell
{
  ~HTMLTableCellAccessibleWrap() {}

public:
  HTMLTableCellAccessibleWrap(nsIContent* aContent, DocAccessible* aDoc) :
    HTMLTableCellAccessible(aContent, aDoc), ia2AccessibleTableCell(this) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() override;
};






class HTMLTableHeaderCellAccessibleWrap : public HTMLTableHeaderCellAccessible,
                                          public ia2AccessibleTableCell
{
  ~HTMLTableHeaderCellAccessibleWrap() {}

public:
  HTMLTableHeaderCellAccessibleWrap(nsIContent* aContent,
                                    DocAccessible* aDoc) :
    HTMLTableHeaderCellAccessible(aContent, aDoc), ia2AccessibleTableCell(this)
  {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED

  virtual void Shutdown() override;
};

} 
} 

#endif

