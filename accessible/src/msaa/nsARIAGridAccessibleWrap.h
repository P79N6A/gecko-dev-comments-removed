







































#ifndef _NSARIAGRIDACCESSIBLEWRAP_H
#define _NSARIAGRIDACCESSIBLEWRAP_H

#include "nsARIAGridAccessible.h"
#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsARIAGridAccessibleWrap : public nsARIAGridAccessible,
                                 public CAccessibleTable
{
public:
  nsARIAGridAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsARIAGridAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsARIAGridCellAccessibleWrap : public nsARIAGridCellAccessible,
                                     public CAccessibleTableCell
{
public:
  nsARIAGridCellAccessibleWrap(nsIContent* aContent, nsDocAccessible* aDoc) :
    nsARIAGridCellAccessible(aContent, aDoc) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
