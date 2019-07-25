






































#ifndef __nsXULComboboxAccessible_h__
#define __nsXULComboboxAccessible_h__

#include "nsCOMPtr.h"
#include "nsXULMenuAccessible.h"




class nsXULComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULComboboxAccessible(nsIContent *aContent, nsIWeakReference *aShell);

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD GetDescription(nsAString& aDescription);
  NS_IMETHOD DoAction(PRUint8 aIndex);
  NS_IMETHOD GetNumActions(PRUint8 *aNumActions);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual PRBool Init();

  
  virtual PRUint32 NativeRole();
  virtual PRUint64 NativeState();
  virtual PRBool GetAllowsAnonChildAccessibles();
};

#endif
