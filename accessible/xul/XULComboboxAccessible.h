




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

  
  NS_IMETHOD DoAction(uint8_t aIndex);
  NS_IMETHOD GetActionName(uint8_t aIndex, nsAString& aName);

  
  virtual void Description(nsString& aDescription);
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual bool CanHaveAnonChildren();

  
  virtual uint8_t ActionCount();

  
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;
};

} 
} 

#endif
