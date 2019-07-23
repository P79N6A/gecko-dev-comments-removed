






































#ifndef __nsXULTreeAccessibleWrap_h__
#define __nsXULTreeAccessibleWrap_h__

#include "nsXULTreeGridAccessible.h"

class nsXULTreeGridAccessibleWrap : public nsXULTreeGridAccessible
{
public:
  nsXULTreeGridAccessibleWrap(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);

  
  NS_IMETHOD GetColumnHeader(nsIAccessibleTable **aColumnHeader);
  NS_IMETHOD GetColumnDescription(PRInt32 aColumn, nsAString& aDescription);
};

class nsXULTreeColumnsAccessibleWrap : public nsXULTreeColumnsAccessible,
                                       public nsIAccessibleTable
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  nsXULTreeColumnsAccessibleWrap(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
};

#endif
