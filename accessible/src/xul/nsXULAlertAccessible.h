




































#ifndef _nsXULAlertAccessible_H_
#define _nsXULAlertAccessible_H_

#include "nsAccessibleWrap.h"




class nsXULAlertAccessible : public nsAccessibleWrap
{
public:
  nsXULAlertAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);
  NS_DECL_ISUPPORTS_INHERITED
  NS_IMETHOD GetRole(PRUint32 *aRole);
  NS_IMETHOD GetState(PRUint32 *aState);
};

#endif  
