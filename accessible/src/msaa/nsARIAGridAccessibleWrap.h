







































#ifndef _NSARIAGRIDACCESSIBLEWRAP_H
#define _NSARIAGRIDACCESSIBLEWRAP_H

#include "nsARIAGridAccessible.h"
#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsARIAGridAccessibleWrap : public nsARIAGridAccessible,
                                 public CAccessibleTable
{
public:
  nsARIAGridAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsARIAGridAccessible(aNode, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsARIAGridCellAccessibleWrap : public nsARIAGridCellAccessible,
                                     public CAccessibleTableCell
{
public:
  nsARIAGridCellAccessibleWrap(nsIDOMNode* aNode, nsIWeakReference* aShell) :
    nsARIAGridCellAccessible(aNode, aShell) {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
