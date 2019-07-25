




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

  
  virtual void Value(nsString& aValue);
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();
  virtual uint64_t NativeInteractiveState() const;

  
  virtual Accessible* ContainerWidget() const;
};





class XULColorPickerAccessible : public XULColorPickerTileAccessible
{
public:
  XULColorPickerAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual a11y::role NativeRole();
  virtual uint64_t NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

protected:

  
  virtual void CacheChildren();
};

} 
} 

#endif  
