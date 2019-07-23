





































#ifndef __nsXULTreeAccessibleWrap_h__
#define __nsXULTreeAccessibleWrap_h__

#include "nsXULTreeAccessible.h"

typedef class nsXULTreeColumnsAccessible   nsXULTreeColumnsAccessibleWrap;

class nsXULTreeAccessibleWrap : public nsXULTreeAccessible
{
public:
  nsXULTreeAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell);
  virtual ~nsXULTreeAccessibleWrap() {}

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

class nsXULTreeitemAccessibleWrap : public nsXULTreeitemAccessible
{
public:
  nsXULTreeitemAccessibleWrap(nsIAccessible *aParent, nsIDOMNode *aDOMNode, nsIWeakReference *aShell, 
    PRInt32 aRow, nsITreeColumn* aColumn);
  virtual ~nsXULTreeitemAccessibleWrap() {}

  
  NS_IMETHOD GetBounds(PRInt32 *x, PRInt32 *y, PRInt32 *width, PRInt32 *height);
  NS_IMETHOD GetName(nsAString &aName);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
};

#endif
