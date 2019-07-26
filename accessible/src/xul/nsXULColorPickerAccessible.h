




#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_

#include "AccessibleWrap.h"




class nsXULColorPickerTileAccessible : public AccessibleWrap
{
public:
  nsXULColorPickerTileAccessible(nsIContent* aContent,
                                 DocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();
  virtual PRUint64 NativeInteractiveState() const;

  
  virtual Accessible* ContainerWidget() const;
};





class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIContent* aContent, DocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

protected:

  
  virtual void CacheChildren();
};

#endif  
