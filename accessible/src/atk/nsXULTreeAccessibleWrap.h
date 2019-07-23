






































#ifndef __nsXULTreeAccessibleWrap_h__
#define __nsXULTreeAccessibleWrap_h__

#include "nsIAccessibleTable.h"
#include "nsXULTreeAccessible.h"

typedef class nsXULTreeitemAccessible nsXULTreeitemAccessibleWrap;

class nsXULTreeAccessibleWrap : public nsXULTreeAccessible,
                                public nsIAccessibleTable
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  nsXULTreeAccessibleWrap(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeAccessibleWrap() {}

  NS_IMETHOD GetChildCount(PRInt32 *_retval);
  NS_IMETHOD ChangeSelection(PRInt32 aIndex, PRUint8 aMethod, PRBool *aSelState);
};

class nsXULTreeColumnsAccessibleWrap : public nsXULTreeColumnsAccessible,
                                       public nsIAccessibleTable
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIACCESSIBLETABLE

  nsXULTreeColumnsAccessibleWrap(nsIDOMNode* aDOMNode, nsIWeakReference* aShell);
  virtual ~nsXULTreeColumnsAccessibleWrap() {}
};

#endif
