




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

  
  virtual void Description(nsString& aDescription) MOZ_OVERRIDE;
  virtual void Value(nsString& aValue) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual bool CanHaveAnonChildren() MOZ_OVERRIDE;

  
  virtual uint8_t ActionCount() MOZ_OVERRIDE;
  virtual void ActionNameAt(uint8_t aIndex, nsAString& aName) MOZ_OVERRIDE;
  virtual bool DoAction(uint8_t aIndex) MOZ_OVERRIDE;

  
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;
};

} 
} 

#endif
