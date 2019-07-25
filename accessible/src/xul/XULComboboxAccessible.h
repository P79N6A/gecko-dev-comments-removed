




#ifndef mozilla_a11y_XULComboboxAccessible_h__
#define mozilla_a11y_XULComboboxAccessible_h__

#include "XULMenuAccessible.h"

namespace mozilla {
namespace a11y {




class XULComboboxAccessible : public AccessibleWrap
{
public:
  enum { eAction_Click = 0 };

  XULComboboxAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  NS_IMETHOD DoAction(PRUint8 aIndex);
  NS_IMETHOD GetActionName(PRUint8 aIndex, nsAString& aName);

  
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual PRUint8 ActionCount();

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};

} 
} 

#endif
