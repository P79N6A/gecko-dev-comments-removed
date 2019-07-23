





































#ifndef __nsXULListboxAccessibleWrap_h__
#define __nsXULListboxAccessibleWrap_h__

#include "nsXULListboxAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsXULListboxAccessibleWrap : public nsXULListboxAccessible,
                                   public CAccessibleTable
{
public:
  nsXULListboxAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsXULListCellAccessibleWrap : public nsXULListCellAccessible,
                                    public CAccessibleTableCell
{
public:
  nsXULListCellAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
