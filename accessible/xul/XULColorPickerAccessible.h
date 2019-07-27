




#ifndef mozilla_a11y_XULColorPickerAccessible_h__
#define mozilla_a11y_XULColorPickerAccessible_h__

#include "AccessibleWrap.h"

namespace mozilla {
namespace a11y {




class XULColorPickerTileAccessible : public AccessibleWrap
{
public:
  XULColorPickerTileAccessible(nsIContent* aContent,
                               DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue) MOZ_OVERRIDE;
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;
  virtual uint64_t NativeInteractiveState() const MOZ_OVERRIDE;

  
  virtual Accessible* ContainerWidget() const MOZ_OVERRIDE;
};





class XULColorPickerAccessible : public XULColorPickerTileAccessible
{
public:
  XULColorPickerAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole() MOZ_OVERRIDE;
  virtual uint64_t NativeState() MOZ_OVERRIDE;

  
  virtual bool IsWidget() const MOZ_OVERRIDE;
  virtual bool IsActiveWidget() const MOZ_OVERRIDE;
  virtual bool AreItemsOperable() const MOZ_OVERRIDE;

  virtual bool IsAcceptableChild(Accessible* aPossibleChild) const MOZ_OVERRIDE;
};

} 
} 

#endif  
