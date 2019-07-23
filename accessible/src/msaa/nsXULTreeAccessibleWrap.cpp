






































#include "nsXULTreeAccessibleWrap.h"





nsXULTreeGridAccessibleWrap::
  nsXULTreeGridAccessibleWrap(nsIDOMNode *aDOMNode, nsIWeakReference *aShell) :
  nsXULTreeGridAccessible(aDOMNode, aShell)
{
}

NS_IMPL_ISUPPORTS_INHERITED0(nsXULTreeGridAccessibleWrap,
                             nsXULTreeGridAccessible)

IMPL_IUNKNOWN_INHERITED1(nsXULTreeGridAccessibleWrap,
                         nsAccessibleWrap,
                         CAccessibleTable);
