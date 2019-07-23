




































#ifndef _nsXULAlertAccessible_H_
#define _nsXULAlertAccessible_H_

#include "nsAccessibleWrap.h"




class nsXULAlertAccessible : public nsAccessibleWrap
{
public:
  nsXULAlertAccessible(nsIDOMNode* aNode, nsIWeakReference* aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetName(nsAString& aName);

  
  virtual nsresult GetRoleInternal(PRUint32 *aRole);
  virtual nsresult GetStateInternal(PRUint32 *aState, PRUint32 *aExtraState);
};

#endif  
