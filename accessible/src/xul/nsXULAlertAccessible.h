




































#ifndef _nsXULAlertAccessible_H_
#define _nsXULAlertAccessible_H_

#include "nsAccessibleWrap.h"





class nsXULAlertAccessible : public nsAccessibleWrap
{
public:
  nsXULAlertAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  NS_DECL_ISUPPORTS_INHERITED

  
  NS_IMETHOD GetName(nsAString& aName);

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
};

#endif  
