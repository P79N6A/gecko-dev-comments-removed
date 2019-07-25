





































#ifndef __nsXULListboxAccessibleWrap_h__
#define __nsXULListboxAccessibleWrap_h__

#include "nsXULListboxAccessible.h"

#include "CAccessibleTable.h"
#include "CAccessibleTableCell.h"





class nsXULListboxAccessibleWrap : public nsXULListboxAccessible,
                                   public CAccessibleTable
{
public:
  nsXULListboxAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};





class nsXULListCellAccessibleWrap : public nsXULListCellAccessible,
                                    public CAccessibleTableCell
{
public:
  nsXULListCellAccessibleWrap(nsIContent *aContent, nsIWeakReference *aShell);

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
