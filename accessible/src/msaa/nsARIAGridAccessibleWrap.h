







































#ifndef _NSARIAGRIDACCESSIBLEWRAP_H
#define _NSARIAGRIDACCESSIBLEWRAP_H

#include "nsARIAGridAccessible.h"
#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsARIAGridAccessibleWrap : public nsARIAGridAccessible,
                                 public CAccessibleTable
{
public:
  nsARIAGridAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsARIAGridAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsARIAGridCellAccessibleWrap : public nsARIAGridCellAccessible,
                                     public CAccessibleTableCell
{
public:
  nsARIAGridCellAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell) :
    nsARIAGridCellAccessible(aContent, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
