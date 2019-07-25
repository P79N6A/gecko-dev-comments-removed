






































#ifndef __nsXULComboboxAccessible_h__
#define __nsXULComboboxAccessible_h__

#include "nsXULMenuAccessible.h"




class nsXULComboboxAccessible : public nsAccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  nsXULComboboxAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  NS_IMETHOD GetValue(nsAString& aValue);
  NS_IMETHOD DoAction(PRUint8 aIndex);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual void Description(nsString& aDescription);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool GetAllowsAnonChildAccessibles();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};

#endif
