





































#ifndef _nsXULColorPickerAccessible_H_
#define _nsXULColorPickerAccessible_H_

#include "nsAccessibleWrap.h"




class nsXULColorPickerTileAccessible : public nsAccessibleWrap
{
public:
  nsXULColorPickerTileAccessible(nsIContent* aContent,
                                 nsDocAccessible* aDoc);

  
  virtual void Value(nsString& aValue);
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual nsAccessible* ContainerWidget() const;
};





class nsXULColorPickerAccessible : public nsXULColorPickerTileAccessible
{
public:
  nsXULColorPickerAccessible(nsIContent* aContent, nsDocAccessible* aDoc);

  
  virtual mozilla::a11y::role NativeRole();
  virtual PRUint64 NativeState();

  
  virtual bool IsWidget() const;
  virtual bool IsActiveWidget() const;
  virtual bool AreItemsOperable() const;

protected:

  
  virtual void CacheChildren();
};

#endif  
