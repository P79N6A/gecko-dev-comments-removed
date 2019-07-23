






































#ifndef __nsXULTreeAccessibleWrap_h__
#define __nsXULTreeAccessibleWrap_h__

#include "nsXULTreeGridAccessible.h"

#include "CAccessibleTable.h"

typedef class nsXULTreeColumnsAccessible   nsXULTreeColumnsAccessibleWrap;





class nsXULTreeGridAccessibleWrap : public nsXULTreeGridAccessible,
                                    public CAccessibleTable
{
public:
  nsXULTreeGridAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);
  virtual ~nsXULTreeGridAccessibleWrap() {}

  
  DECL_IUNKNOWN_INHERITED

  
  NS_DECL_ISUPPORTS_INHERITED
};

#endif
